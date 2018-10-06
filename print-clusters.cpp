#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>

#include <sqlite3.h>

#include <boost/lexical_cast.hpp>

#include <third_party/nlohmann/json.hpp>
#include <third_party/cxxopts/cxxopts.hpp>

#include "hash_delimeter.hpp"
#include "tokenizer.hpp"
#include "exc.hpp"

using namespace imgdupl;

// key is a cluster id, value is an array of images' filenames
using Cluster = std::map<std::string, std::vector<std::string>>;

// all clusters as an array of maps
using Clusters = std::vector<Cluster>;

void
usage(const char* program)
{
    std::cout << "Usage: " << program << " <db_file> <clusters_table> [<min_size>]" << std::endl;
    std::cout << std::endl << "Where:" << std::endl;
    std::cout << "  db_file         -- SQLite database file" << std::endl;
    std::cout << "  clusters_table  -- name of a table in SQLite database with clusters" << std::endl;
    std::cout << "  min_size        -- minimal cluster size (default: 1)" << std::endl;

    exit(0);
}

void
print(const cxxopts::ParseResult& opts)
{
    int min_cluster_size = 1;
    if (opts.count("min-size") > 0) {
        min_cluster_size = opts["min-size"].as<int>();
    }

    std::string db_file = opts["db-file"].as<std::string>();
    std::string clusters_table = opts["table"].as<std::string>();

    sqlite3* db = NULL;
    int rc;

    sqlite3_initialize();

    rc = sqlite3_open_v2(db_file.c_str(), &db, SQLITE_OPEN_READONLY, NULL);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_open_v2() failed");

    char iterate_over_clusters_st[512];
    sqlite3_stmt* iterate_over_clusters_stmt = NULL;

    snprintf(iterate_over_clusters_st, sizeof(iterate_over_clusters_st), "SELECT cluster_id,images FROM %s WHERE count >= %d",
        clusters_table.c_str(), min_cluster_size);

    rc = sqlite3_prepare_v2(db, iterate_over_clusters_st, strlen(iterate_over_clusters_st), &iterate_over_clusters_stmt, NULL);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_prepare_v2() failed: \"%s\"", sqlite3_errmsg(db));

    char select_path_st[512];
    sqlite3_stmt* select_path_stmt = NULL;

    snprintf(select_path_st, sizeof(select_path_st), "SELECT path FROM hashes WHERE id=?;");
    rc = sqlite3_prepare_v2(db, select_path_st, strlen(select_path_st), &select_path_stmt, NULL);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_prepare_v2() failed: \"%s\"", sqlite3_errmsg(db));

    Clusters clusters;

    for (;;) {
        rc = sqlite3_step(iterate_over_clusters_stmt);
        THROW_EXC_IF_FAILED(rc == SQLITE_ROW || rc == SQLITE_DONE, "sqlite3_step() failed: \"%s\"", sqlite3_errmsg(db));
        if (rc == SQLITE_DONE) {
            break;
        }

        uint32_t cluster_id = sqlite3_column_int(iterate_over_clusters_stmt, 0);
        std::string images = std::string(reinterpret_cast<const char*>(sqlite3_column_text(iterate_over_clusters_stmt, 1)),
            sqlite3_column_bytes(iterate_over_clusters_stmt, 1));

        Cluster cluster;

        std::string path;
        Tokens images_id;

        tokenize(images, images_id, ",");

        for (auto& v : images_id) {
            auto id = boost::lexical_cast<uint32_t>(v);

            rc = sqlite3_bind_int(select_path_stmt, 1, id);
            THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_bind_int() failed: \"%s\"", sqlite3_errmsg(db));

            rc = sqlite3_step(select_path_stmt);
            THROW_EXC_IF_FAILED(rc == SQLITE_ROW, "sqlite3_step() failed: \"%s\"", sqlite3_errmsg(db));

            path = std::string(
                reinterpret_cast<const char*>(sqlite3_column_text(select_path_stmt, 0)), sqlite3_column_bytes(select_path_stmt, 0));
            cluster[std::to_string(cluster_id)].push_back(path);

            rc = sqlite3_reset(select_path_stmt);
            THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_reset() failed: \"%s\"", sqlite3_errmsg(db));
        }

        clusters.push_back(cluster);
    }

    nlohmann::json js = clusters;
    std::cout << js.dump(4) << std::endl;

    rc = sqlite3_finalize(select_path_stmt);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_finalize() failed: \"%s\"", sqlite3_errmsg(db));

    rc = sqlite3_finalize(iterate_over_clusters_stmt);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_finalize() failed: \"%s\"", sqlite3_errmsg(db));

    rc = sqlite3_close(db);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_close() failed: %i", rc);

    sqlite3_shutdown();
}

int
main(int argc, char** argv)
{
    try {
        cxxopts::Options args(argv[0], "print clusters of perceptually similar images");

        // clang-format off
        args.add_options()
            ("h,help","show this help and exit")
            ("d,db-file", "SQLite database file", cxxopts::value<std::string>())
            ("t,table", "name of a table with clusters in database", cxxopts::value<std::string>())
            ("s,min-size", "minimal cluster size", cxxopts::value<int>())
            ;
        // clang-format on

        auto opts = args.parse(argc, argv);

        if (opts.count("help")) {
            std::cout << args.help() << std::endl;
            return EXIT_SUCCESS;
        }

        if ((opts.count("db-file") == 0 && opts.count("d") == 0) || (opts.count("table") == 0 && opts.count("t") == 0)) {
            std::cerr << std::endl
                      << "Error: you have to specify --db-file and --table parameters. Run with --help for help." << std::endl
                      << std::endl;
            return EXIT_FAILURE;
        }

        print(opts);
    } catch (std::exception& exc) {
        std::cerr << "Error: " << exc.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
