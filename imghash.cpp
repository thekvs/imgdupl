#include <stdint.h>

#include <iostream>
#include <iomanip>
#include <string>
#include <utility>

#include <boost/filesystem.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>

#include "dct_perceptual_hasher.hpp"
#include "hash_delimeter.hpp"

using namespace imgdupl;

namespace bfs = boost::filesystem;

typedef DCTHasher<32, 11> Hasher;

void usage(const char *program);
std::pair<bool, PHash> calc_image_hash(const std::string &image_file, const Hasher &hasher);
void process_file(const bfs::path &file, const Hasher &hasher);
void process_directory(std::string directory, const Hasher &hasher);

std::ostream&
operator<<(std::ostream &out, const PHash &phash)
{
    bool is_first = true;

    BOOST_FOREACH(const PHash::value_type &h, phash) {
        if (!is_first) {
            out << HASH_PRINT_DELIMETER;
        } else {
            is_first = false;
        }
        
        out << h;
    }

    return out;
}

void
usage(const char *program)
{
    std::cout << "Usage: " << program << " <path>" << std::endl;
    std::cout << std::endl << "Where:" << std::endl;
    std::cout << "  path  -- single file or directory (in this case process all files)" << std::endl;
    
    exit(0);
}

void
process_file(const bfs::path &file, const Hasher &hasher)
{
    if (bfs::exists(file) && bfs::is_regular_file(file)) {
        std::string filename = file.string();

        bool  status;
        PHash phash;

        boost::tie(status, phash) = calc_image_hash(filename, hasher);
        if (status) {
            std::cout << phash << '\t' << filename << std::endl;
        } else {
            std::cerr << "Failed at \'" << filename << "\'" << std::endl;
        }
    }
}

void
process_directory(std::string directory, const Hasher &hasher)
{
    bfs::path root(directory);
    bfs::recursive_directory_iterator cur_it(root), end_it;

    for (; cur_it != end_it; ++cur_it) {
        process_file(cur_it->path(), hasher);
    }
}

std::pair<bool, PHash>
calc_image_hash(const std::string &image_file, const Hasher &hasher)
{
    PHash phash;
    bool  status = true;

    Magick::Image image;

    try {
        image.read(image_file.c_str());
        image.trim();
    } catch (Magick::Exception &e) {
        status = false;
        goto end;        
    }

    boost::tie(status, phash) = hasher.hash(image);

end:
    return std::make_pair(status, phash);
}

int
main(int argc, char **argv)
{
    if (argc < 2) {
        usage(argv[0]);
    }

    Magick::InitializeMagick(NULL);

    Hasher hasher;

    std::string path = argv[1];

    if (bfs::exists(path)) {
        if (bfs::is_regular_file(path)) {
            process_file(path, hasher);
        } else if (bfs::is_directory(path)) {
            process_directory(path, hasher);
        }
    } else {
        std::cerr << path << " does not exist" << std::endl;
    }

    return EXIT_SUCCESS;
}
