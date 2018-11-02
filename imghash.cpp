#include <stdint.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <utility>

#include <boost/filesystem.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>

#include <third_party/cxxopts/cxxopts.hpp>

#include "dct_perceptual_hasher.hpp"
#include "hash_delimeter.hpp"

using namespace imgdupl;

namespace bfs = boost::filesystem;

typedef DCTHasher<50, 64 * 2> Hasher;

std::pair<bool, PHash> calc_image_hash(const std::string& image_file, const Hasher& hasher);
void process_file(const bfs::path& file, const Hasher& hasher, std::ofstream& result);
void process_directory(std::string directory, const Hasher& hasher, std::ofstream& result);

std::ostream&
operator<<(std::ostream& out, const PHash& phash)
{
    bool is_first = true;

    BOOST_FOREACH (const PHash::value_type& h, phash) {
        if (is_first) {
            is_first = false;
        } else {
            out << HASH_PRINT_DELIMETER;
        }

        out << h;
    }

    return out;
}

void
process_file(const bfs::path& file, const Hasher& hasher, std::ofstream& result)
{
    if (bfs::exists(file) && bfs::is_regular_file(file)) {
        std::string filename = file.string();

        bool status;
        PHash phash;

        boost::tie(status, phash) = calc_image_hash(filename, hasher);
        if (status) {
            result << phash << '\t' << filename << std::endl;
        } else {
            std::cerr << "Failed at \'" << filename << "\'" << std::endl;
        }
    }
}

void
process_directory(std::string directory, const Hasher& hasher, std::ofstream& result)
{
    bfs::path root(directory);
    bfs::recursive_directory_iterator cur_it(root), end_it;

    for (; cur_it != end_it; ++cur_it) {
        process_file(cur_it->path(), hasher, result);
    }
}

std::pair<bool, PHash>
calc_image_hash(const std::string& image_file, const Hasher& hasher)
{
    PHash phash;
    bool status = true;

    Magick::Image image;

    try {
        image.read(image_file.c_str());
        image.trim();
    } catch (Magick::Exception&) {
        status = false;
        goto end;
    }

    boost::tie(status, phash) = hasher.hash(image);

end:
    return std::make_pair(status, phash);
}

int
main(int argc, char** argv)
{
    cxxopts::Options args(argv[0], "print clusters of perceptually similar images");

    // clang-format off
    args.add_options()
        ("h,help","show this help and exit")
        ("d,data", "path to a single image file or a directory with images", cxxopts::value<std::string>())
        ("r,result", "result file", cxxopts::value<std::string>())
        ;
    // clang-format on

    auto opts = args.parse(argc, argv);

    if (opts.count("help")) {
        std::cout << args.help() << std::endl;
        return EXIT_SUCCESS;
    }

    if ((opts.count("data") == 0 && opts.count("d") == 0) || (opts.count("result") == 0 && opts.count("r") == 0)) {
        std::cerr << std::endl
                  << "Error: you have to specify --data and --result parameters. Run with --help for help." << std::endl
                  << std::endl;
        return EXIT_FAILURE;
    }

    Magick::InitializeMagick(NULL);

    std::ofstream result(opts["result"].as<std::string>().c_str());
    if (result.fail()) {
        std::cerr << "Error: couldn't open file '" << opts["result"].as<std::string>() << "' for writing" << std::endl;
        return EXIT_FAILURE;
    }

    Hasher hasher;
    auto path = opts["data"].as<std::string>();

    if (bfs::exists(path)) {
        if (bfs::is_regular_file(path)) {
            process_file(path, hasher, result);
        } else if (bfs::is_directory(path)) {
            process_directory(path, hasher, result);
        }
    } else {
        std::cerr << path << " does not exist" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
