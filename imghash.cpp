#include <cstdint>
#include <fstream>
#include <iomanip>
#include <string>
#include <utility>
#include <tuple>

#include <boost/filesystem.hpp>

#include <cxxopts.hpp>
#include <spdlog/spdlog.h>
#include <indicators/progress_bar.hpp>
#include <indicators/cursor_control.hpp>

#include "dct_perceptual_hasher.hpp"
#include "hash_delimeter.hpp"

using namespace imgdupl;
using Hasher = DCTHasher<50, 64 * 2>;

namespace fs = boost::filesystem;

std::pair<bool, PHash> calc_image_hash(const std::string& image_file, const Hasher& hasher);
void process_file(const fs::path& file, const Hasher& hasher, std::ofstream& result);
void process_directory(std::string directory, const Hasher& hasher, std::ofstream& result);
size_t get_files_count(std::string directory);

std::ostream&
operator<<(std::ostream& out, const PHash& phash)
{
    out << fmt::format("{}", fmt::join(phash, ","));

    return out;
}

void
process_file(const fs::path& file, const Hasher& hasher, std::ofstream& result)
{
    if (fs::exists(file) && fs::is_regular_file(file)) {
        std::string filename = file.string();

        bool status;
        PHash phash;

        std::tie(status, phash) = calc_image_hash(filename, hasher);
        if (status) {
            result << phash << '\t' << filename << std::endl;
        } else {
            spdlog::error("failed at '{}'", filename);
        }
    }
}

size_t
get_files_count(std::string directory)
{
    size_t count = 0;

    fs::path root(directory);
    fs::recursive_directory_iterator cur_iter(root), end_iter;

    for (; cur_iter != end_iter; ++cur_iter) {
        if (fs::exists(cur_iter->path()) && fs::is_regular_file(cur_iter->path())) {
            count++;
        }
    }

    return count;
}

void
process_directory(std::string directory, const Hasher& hasher, std::ofstream& result)
{
    auto total = get_files_count(directory);
    if (total == 0) {
        return;
    }

    // clang-format off
    indicators::ProgressBar pb {
        indicators::option::BarWidth {80},
        indicators::option::Start {"["},
        indicators::option::Fill {"="},
        indicators::option::Lead {">"},
        indicators::option::Remainder {" "},
        indicators::option::End {"]"},
        indicators::option::ForegroundColor {indicators::Color::white},
        indicators::option::FontStyles {std::vector<indicators::FontStyle> {indicators::FontStyle::bold}},
        indicators::option::MaxProgress{total}
    };
    // clang-format on

    fs::path root(directory);
    fs::recursive_directory_iterator cur_iter(root), end_iter;

    for (size_t i = 1; cur_iter != end_iter; ++cur_iter, i++) {
        process_file(cur_iter->path(), hasher, result);
        pb.set_option(
            indicators::option::PostfixText {"Processing images: " + std::to_string(i) + "/" + std::to_string(total)});
        pb.tick();
    }

    indicators::show_console_cursor(true);
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

    std::tie(status, phash) = hasher.hash(image);

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
        fmt::print("{}\n", args.help());
        return EXIT_SUCCESS;
    }

    if ((opts.count("data") == 0 && opts.count("d") == 0) || (opts.count("result") == 0 && opts.count("r") == 0)) {
        spdlog::error("you have to specify --data and --result parameters. Run with --help for help.");
        return EXIT_FAILURE;
    }

    Magick::InitializeMagick(nullptr);

    std::ofstream result(opts["result"].as<std::string>().c_str());
    if (result.fail()) {
        spdlog::error("couldn't open file '{}' for writting!", opts["result"].as<std::string>());
        return EXIT_FAILURE;
    }

    Hasher hasher;
    auto path = opts["data"].as<std::string>();

    if (fs::exists(path)) {
        if (fs::is_regular_file(path)) {
            process_file(path, hasher, result);
        } else if (fs::is_directory(path)) {
            process_directory(path, hasher, result);
        }
    } else {
        spdlog::error("'{}' does not exist!\n", path);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
