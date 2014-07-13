#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>

#include <boost/lexical_cast.hpp>

#include <sqlite3.h>

#include "hash_delimeter.hpp"
#include "tokenizer.hpp"
#include "phash.hpp"
#include "exc.hpp"

using namespace imgdupl;

template<typename Data>
class ConcurrentQueue {
public:

    typedef Data value_type;

    ConcurrentQueue() {}

    void push(const Data &data)
    {
        std::unique_lock<std::mutex> lock(mutex);

        queue.push(data);
        lock.unlock();
        condvar.notify_one();
    }

    bool empty() const
    {
        std::unique_lock<std::mutex> lock(mutex);
        return queue.empty();
    }

    bool try_pop(Data &popped_value)
    {
        std::unique_lock<std::mutex> lock(mutex);
        
        if (queue.empty()) {
            return false;
        }

        popped_value = queue.front();
        queue.pop();

        return true;
    }

    void wait_and_pop(Data &popped_value)
    {
        std::unique_lock<std::mutex> lock(mutex);

        while (queue.empty()) {
            condvar.wait(lock);
        }

        popped_value = queue.front();
        queue.pop();
    }

    size_t size() const
    {
        std::unique_lock<std::mutex> lock(mutex);

        return queue.size();
    }

    ConcurrentQueue(ConcurrentQueue const&)=delete;
    ConcurrentQueue& operator=(ConcurrentQueue const&)=delete;

private:

    std::queue<Data>        queue;
    mutable std::mutex      mutex;
    std::condition_variable condvar;
};

class Image {
public:

    PHash    hash;
    uint32_t image_id;
    uint32_t processed;

    Image():
        image_id(0),
        processed(0)
    {
    }

    Image(const PHash &hash_, uint32_t image_id_, uint32_t processed_=0):
        hash(hash_),
        image_id(image_id_),
        processed(processed_)
    {
    }
};

typedef std::vector<Image> Images;

class ClusterEntry {
public:

    PHash    hash;
    uint32_t image_id;

    ClusterEntry():
        image_id(0)
    {
    }

    ClusterEntry(const PHash &hash_, uint32_t image_id_):
        hash(hash_),
        image_id(image_id_)
    {
    }
};

typedef std::vector<ClusterEntry> ClusterEntries;

class Task {
public:

    PHash            cluster_base_hash;
    Images::iterator cur_it;
    Images::iterator end_it;
    ClusterEntries   cluster_entries;

    Task()
    {
        cluster_base_hash.reserve(16);
    }

    Task(const PHash &cluster_base_hash_, Images::iterator cur_it_, Images::iterator end_it_):
        cluster_base_hash(cluster_base_hash_),
        cur_it(cur_it_),
        end_it(end_it_)
    {
    }
};

typedef std::shared_ptr<Task>         TaskSmartPtr;
typedef ConcurrentQueue<TaskSmartPtr> TasksQueue;

std::ostream&
operator<<(std::ostream &out, const PHash &mhash)
{
    bool is_first = true;

    for (auto &v: mhash) {
        if (!is_first) {
            out << HASH_PRINT_DELIMETER;
        } else {
            is_first = false;
        }
        
        out << v;
    }

    return out;
}

void
usage(const char *program)
{
    std::cout << "Usage: " << program << " <data> <threshold> <threads>" << std::endl << std::endl;
    std::cout << "Where: " << std::endl;
    std::cout << " data      -- SQLite database with perceptual hashes" << std::endl;
    std::cout << " threshold -- distance between two hashes" << std::endl;
    std::cout << " threads   -- number of threads to run" << std::endl << std::endl;

    std::cout << "Example: " << program << " hashes.db 22 8" << std::endl << std::endl;

    exit(0);
}

PHash
make_hash(const std::string &data)
{
    PHash hash;

    Separator separator(HASH_PRINT_DELIMETER);
    Tokenizer tokenizer(data, separator);

    for (auto &v: tokenizer) {
        hash.push_back(boost::lexical_cast<uint64_t>(v));   
    }

    return hash;
}

void
read_data_from_db(std::string name, Images &images)
{
    sqlite3 *db = NULL;

    int rc = sqlite3_initialize();
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_initialize() failed");

    rc = sqlite3_open_v2(name.c_str(), &db, SQLITE_OPEN_READONLY, NULL);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_open_v2() failed");

    std::string st = "SELECT id, hash FROM hashes";
    sqlite3_stmt *stmt = NULL;

    rc = sqlite3_prepare_v2(db, st.c_str(), st.size(), &stmt, NULL);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_prepare_v2() failed: \"%s\"", sqlite3_errmsg(db));

    for (;;) {
        rc = sqlite3_step(stmt);
        THROW_EXC_IF_FAILED(rc == SQLITE_ROW || rc == SQLITE_DONE,
            "sqlite3_step() failed: \"%s\"", sqlite3_errmsg(db));
        if (rc == SQLITE_DONE) {
            break;
        }

        uint32_t    image_id = sqlite3_column_int(stmt, 0);
        std::string hash_data = std::string(
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
            sqlite3_column_bytes(stmt, 1));
        PHash       hash = make_hash(hash_data);

        images.push_back(Image(hash, image_id));
    }

    rc = sqlite3_finalize(stmt);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_finalize() failed: \"%s\"", sqlite3_errmsg(db));

    sqlite3_close(db);
    sqlite3_shutdown();
}

int
hamming_distance(uint64_t hash1, uint64_t hash2)
{
    uint64_t x = hash1 ^ hash2;

    return __builtin_popcountll(x);
}

bool
distance(const PHash &mh1, const PHash &mh2, int threshold)
{
    size_t hash_size = mh1.size();
    int    dist = 0;

    for (size_t i = 0; i < hash_size; i++) {
        dist += hamming_distance(mh1[i], mh2[i]);
    }

    bool result = (dist <= threshold);

    return result;
}

void
make_cluster(const PHash &cluster_base_hash, Images::iterator cur_it, Images::iterator end_it,
    int threshold, ClusterEntries &entries)
{
    assert(cur_it != end_it);

    for (;cur_it != end_it; ++cur_it) {
        if (!cur_it->processed && distance(cluster_base_hash, cur_it->hash, threshold)) {
            entries.push_back(ClusterEntry(cur_it->hash, cur_it->image_id));
            cur_it->processed = 1;
        }
    }
}

void
worker(int threshold, TasksQueue &new_tasks_queue, TasksQueue &accomplished_tasks_queue)
{
    TasksQueue::value_type task;

    for (;;) {
        new_tasks_queue.wait_and_pop(task);
        make_cluster(task->cluster_base_hash, task->cur_it, task->end_it, threshold,
            task->cluster_entries);
        accomplished_tasks_queue.push(task);
    }
}

void
output_cluster(uint64_t &cluster_id, ClusterEntries const &entries)
{
    cluster_id++;

    for (auto &v: entries) {
        std::cout << v.image_id << '\t' << cluster_id << std::endl << std::flush;   
    }
}

Images
compactify(Images::iterator cur_it, Images::iterator end_it)
{
    Images images;
    images.reserve(std::distance(cur_it, end_it));

    std::copy_if(cur_it, end_it, std::back_inserter(images), 
        [](Images::value_type const &v){return v.processed == 0;});

    return images;
}

void
compactification_reminder(int interval, bool &deflate_data)
{
    for (;;) {
        sleep(interval);
        deflate_data = true;
    }
}

int
main(int argc, char **argv)
{
    if (argc < 4) {
        usage(argv[0]);
    }

    std::string datafile = argv[1];
    int         threshold = boost::lexical_cast<int>(argv[2]);
    unsigned    threads_num = boost::lexical_cast<unsigned>(argv[3]);

    Images images;

    read_data_from_db(datafile, images);

    TasksQueue new_tasks_queue;
    TasksQueue accomplished_tasks_queue;

    for (unsigned i = 0; i < threads_num; i++) {
        std::thread w(worker, threshold, std::ref(new_tasks_queue), std::ref(accomplished_tasks_queue));
        w.detach();
    }

    bool deflate_data = false;

    std::thread reminder(compactification_reminder, 60*30, std::ref(deflate_data));
    reminder.detach();

    Images::iterator cur_task_it, end_task_it;

    uint64_t cluster_id = 0;
    PHash    cluster_base_hash;

    size_t distance;
    size_t job_length;
    size_t jobs_count;

    ClusterEntries entries;

    auto cur_it = images.begin();
    auto end_it = images.end();

    for (; cur_it != end_it;) {
        if (cur_it->processed || cur_it->hash[0] == 0) {
            cur_it++;
        } else {
            cluster_base_hash = cur_it->hash;

            entries.clear();
            entries.push_back(ClusterEntry(cluster_base_hash, cur_it->image_id));

            cur_it->processed = 1;
            cur_it++;

            if (cur_it == end_it) {
                output_cluster(cluster_id, entries);
                break;
            }

            distance = std::distance(cur_it, end_it);

            if (distance > threads_num * 1000) {
                jobs_count = threads_num;
            } else {
                jobs_count = 1;
            }

            job_length = distance / jobs_count;
            cur_task_it = end_task_it = cur_it;
            std::advance(end_task_it, job_length);

            for (size_t i = 0; i < jobs_count; i++) {
                TaskSmartPtr task = TaskSmartPtr(new Task(cluster_base_hash,
                    cur_task_it, end_task_it));
                new_tasks_queue.push(task);

                cur_task_it = end_task_it;
                if (i == jobs_count - 1) { // last task maybe a little lengthy
                    end_task_it = end_it;
                } else {
                    std::advance(end_task_it, job_length);
                }
            }

            // gather results
            TaskSmartPtr task;
            for (size_t i = 0; i < jobs_count; i++) {
                accomplished_tasks_queue.wait_and_pop(task);

                auto cur_cluster_it = task->cluster_entries.begin();
                auto end_cluster_it = task->cluster_entries.end();

                for (;cur_cluster_it != end_cluster_it; ++cur_cluster_it) {
                    entries.push_back(*cur_cluster_it);
                }
            }

            output_cluster(cluster_id, entries);

            if (deflate_data) {
                deflate_data = false;

                Images compactified_images;
                compactified_images = compactify(cur_it, end_it);

                images = compactified_images;

                cur_it = images.begin();
                end_it = images.end();
            }
        }
    }

    return EXIT_SUCCESS;
}
