#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>

#include <sqlite3.h>

#include <boost/lexical_cast.hpp>

#include "hash_delimeter.hpp"
#include "tokenizer.hpp"
#include "exc.hpp"

using namespace imgdupl;

class Args {
public:

    std::string data_type;
    std::string data_file;
    std::string db_file;
    std::string clusters_table;

    Args(int argc, char **argv)
    {
        data_type = argv[1];
        THROW_EXC_IF_FAILED(data_type == "hashes" || (data_type == "clusters" && argc == 5),
            "data type must be either 'hashes' or 'clusters'");

        data_file = argv[2];
        db_file = argv[3];
        clusters_table = (data_type == "clusters" ? argv[4] : "");        
    }

    Args()=delete;
};

void
usage(const char *program)
{
    std::cout << "Usage: " << program << " <data_type> <data_file> <db_file> [<clusters_table>]" << std::endl;
    std::cout << std::endl << "Where:" << std::endl;
    std::cout << "  data_type       -- string value, must be either 'hashes' or 'clusters'" << std::endl;
    std::cout << "  data_file       -- file with data to export" << std::endl;
    std::cout << "  db_file         -- SQLite database file" << std::endl;
    std::cout << "  clusters_table  -- name of a table in SQLite database with clusters" << std::endl;

    exit(0);
}

void
create_hashes_table(sqlite3 *db)
{
    std::string   st = "CREATE TABLE hashes (id INTEGER PRIMARY KEY AUTOINCREMENT, hash TEXT, path TEXT)";
    sqlite3_stmt *stmt = NULL;

    int rc = sqlite3_prepare_v2(db, st.c_str(), st.size(), &stmt, NULL);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_prepare_v2() failed: \"%s\"", sqlite3_errmsg(db));

    rc = sqlite3_step(stmt);
    THROW_EXC_IF_FAILED(rc == SQLITE_DONE, "sqlite3_step() failed: \"%s\"", sqlite3_errmsg(db));

    rc = sqlite3_finalize(stmt);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_finalize() failed: \"%s\"", sqlite3_errmsg(db));
}

void
create_clusters_table(sqlite3 *db, const Args &args)
{
    char          st[512];
    sqlite3_stmt *stmt = NULL;

    snprintf(st, sizeof(st), "CREATE TABLE %s (cluster_id INTEGER UNIQUE, count INTEGER, images TEXT)",
        args.clusters_table.c_str());
    
    int rc = sqlite3_prepare_v2(db, st, strlen(st), &stmt, NULL);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_prepare_v2() failed: \"%s\"", sqlite3_errmsg(db));

    rc = sqlite3_step(stmt);
    THROW_EXC_IF_FAILED(rc == SQLITE_DONE, "sqlite3_step() failed: \"%s\"", sqlite3_errmsg(db));

    rc = sqlite3_finalize(stmt);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_finalize() failed: \"%s\"", sqlite3_errmsg(db));   
}

sqlite3*
open_db(const Args &args)
{
    sqlite3 *db = NULL;

    int rc = sqlite3_open_v2(args.db_file.c_str(), &db, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE, NULL);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_open_v2() failed");

    if (args.data_type == "hashes") {
        create_hashes_table(db);
    } else {
        create_clusters_table(db, args);
    }

    return db;
}

void
close_db(sqlite3 *db)
{
    int rc = sqlite3_close(db);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_close() failed");
}

void
fill_hashes_db(sqlite3 *db, const Args &args)
{
    std::ifstream data(args.data_file.c_str(), std::ios::in);
    THROW_EXC_IF_FAILED(!data.fail(), "Couldn't open file \"%s\"", args.data_file.c_str());

    std::string   st = "INSERT INTO hashes (hash, path) VALUES(?, ?)";
    sqlite3_stmt *stmt = NULL;

    int rc = sqlite3_prepare_v2(db, st.c_str(), st.size(), &stmt, NULL);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_prepare_v2() failed: \"%s\"", sqlite3_errmsg(db));

    std::string line;
    Tokens      tokens;

    char *errmsg;

    rc = sqlite3_exec(db, "BEGIN", NULL, NULL, &errmsg);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_exec() failed: \"%s\"", errmsg);

    while (std::getline(data, line)) {
        tokenize(line, tokens, "\t");

        rc = sqlite3_bind_text(stmt, 1, tokens[0].c_str(), tokens[0].size(), SQLITE_TRANSIENT);
        THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_bind_text() failed: \"%s\"", sqlite3_errmsg(db));

        rc = sqlite3_bind_text(stmt, 2, tokens[1].c_str(), tokens[1].size(), SQLITE_TRANSIENT);
        THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_bind_text() failed: \"%s\"", sqlite3_errmsg(db));

        rc = sqlite3_step(stmt);
        THROW_EXC_IF_FAILED(rc == SQLITE_DONE, "sqlite3_step() failed: \"%s\"", sqlite3_errmsg(db));

        rc = sqlite3_reset(stmt);
        THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_reset() failed: \"%s\"", sqlite3_errmsg(db));
    }

    rc = sqlite3_exec(db, "COMMIT", NULL, NULL, &errmsg);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_exec() failed: \"%s\"", errmsg);

    rc = sqlite3_finalize(stmt);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_finalize() failed: \"%s\"", sqlite3_errmsg(db));
}

void
insert_cluster(sqlite3 *db, sqlite3_stmt *stmt, uint32_t cluster_id, uint32_t count, const std::string &images)
{
    int rc;

    rc = sqlite3_bind_int(stmt, 1, cluster_id);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_bind_int() failed: \"%s\"", sqlite3_errmsg(db));

    rc = sqlite3_bind_int(stmt, 2, count);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_bind_int() failed: \"%s\"", sqlite3_errmsg(db));

    rc = sqlite3_bind_text(stmt, 3, images.c_str(), images.size(), SQLITE_TRANSIENT);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_bind_text() failed: \"%s\"", sqlite3_errmsg(db));

    rc = sqlite3_step(stmt);
    THROW_EXC_IF_FAILED(rc == SQLITE_DONE, "sqlite3_step() failed: \"%s\"", sqlite3_errmsg(db));

    rc = sqlite3_reset(stmt);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_reset() failed: \"%s\"", sqlite3_errmsg(db));
}

void
fill_clusters_db(sqlite3 *db, const Args &args)
{
    std::ifstream data(args.data_file.c_str(), std::ios::in);
    THROW_EXC_IF_FAILED(!data.fail(), "Couldn't open file \"%s\"", args.data_file.c_str());

    char          st[512];
    sqlite3_stmt *stmt = NULL;

    snprintf(st, sizeof(st), "INSERT INTO %s (cluster_id, count, images) VALUES(?, ?, ?)", args.clusters_table.c_str());

    int rc = sqlite3_prepare_v2(db, st, strlen(st), &stmt, NULL);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_prepare_v2() failed: \"%s\"", sqlite3_errmsg(db));

    std::string line;
    Tokens      tokens;

    char *errmsg;

    rc = sqlite3_exec(db, "BEGIN", NULL, NULL, &errmsg);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_exec() failed: \"%s\"", errmsg);

    uint32_t    cluster_id, prev_cluster_id = 0;
    std::string images;
    uint32_t    images_count = 0;

    while (std::getline(data, line)) {
        tokenize(line, tokens, "\t");

        cluster_id = boost::lexical_cast<uint32_t>(tokens[1]);

        if (prev_cluster_id != 0 && cluster_id != prev_cluster_id) {
            insert_cluster(db, stmt, prev_cluster_id, images_count, images);
            images = tokens[0];
            images_count = 1;
        } else {
            if (images.size() > 0) {
                images.append("," + tokens[0]);
                images_count++;
            } else {
                images = tokens[0];
                images_count = 1;
            }
        }

        prev_cluster_id = cluster_id;
    }

    // insert last cluster
    insert_cluster(db, stmt, cluster_id, images_count, images);

    rc = sqlite3_exec(db, "COMMIT", NULL, NULL, &errmsg);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_exec() failed: \"%s\"", errmsg);

    rc = sqlite3_finalize(stmt);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_finalize() failed: \"%s\"", sqlite3_errmsg(db));
}

void
fill_db(sqlite3 *db, const Args &args)
{
    if (args.data_type == "hashes") {
        fill_hashes_db(db, args);
    } else {
        fill_clusters_db(db, args);
    }
}

int
main(int argc, char **argv)
{
    if (argc < 4) {
        usage(argv[0]);
    }

    try {
        Args args(argc, argv);

        sqlite3_initialize();
        sqlite3 *db = open_db(args);
        fill_db(db, args);
        close_db(db);
        sqlite3_shutdown();
    } catch (std::exception &exc) {
        std::cerr << "Error: " << exc.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
