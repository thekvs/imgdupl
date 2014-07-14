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

void
usage(const char *program)
{
    std::cout << "Usage: " << program << " <db_file> <clusters_table>" << std::endl;
    std::cout << std::endl << "Where:" << std::endl;
    std::cout << "  db_file         -- SQLite database file" << std::endl;
    std::cout << "  clusters_table  -- name of a table in SQLite database with clusters" << std::endl;

    exit(0);
}

void
print(sqlite3 *db, std::string clusters_table)
{
    char          iterate_over_clusters_st[512];
    sqlite3_stmt *iterate_over_clusters_stmt = NULL;

    snprintf(iterate_over_clusters_st, sizeof(iterate_over_clusters_st),
        "SELECT cluster_id,images FROM %s", clusters_table.c_str());

    int rc = sqlite3_prepare_v2(db, iterate_over_clusters_st, strlen(iterate_over_clusters_st),
        &iterate_over_clusters_stmt, NULL);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_prepare_v2() failed: \"%s\"", sqlite3_errmsg(db));

    char          select_path_st[512];
    sqlite3_stmt *select_path_stmt = NULL;

    snprintf(select_path_st, sizeof(select_path_st), "SELECT path FROM hashes WHERE id=?;");
    rc = sqlite3_prepare_v2(db, select_path_st, strlen(select_path_st), &select_path_stmt, NULL);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_prepare_v2() failed: \"%s\"", sqlite3_errmsg(db));

    for (;;) {
        rc = sqlite3_step(iterate_over_clusters_stmt);
        THROW_EXC_IF_FAILED(rc == SQLITE_ROW || rc == SQLITE_DONE,
            "sqlite3_step() failed: \"%s\"", sqlite3_errmsg(db));
        if (rc == SQLITE_DONE) {
            break;
        }

        uint32_t    cluster_id = sqlite3_column_int(iterate_over_clusters_stmt, 0);
        std::string images = std::string(
            reinterpret_cast<const char*>(sqlite3_column_text(iterate_over_clusters_stmt, 1)),
            sqlite3_column_bytes(iterate_over_clusters_stmt, 1));

        std::cout << "Cluster " << cluster_id << ":" << std::endl;
        
        std::string path;
        Tokens      images_id;

        tokenize(images, images_id, ",");

        for (auto &v: images_id) {
            auto id = boost::lexical_cast<uint32_t>(v);

            rc = sqlite3_bind_int(select_path_stmt, 1, id);
            THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_bind_int() failed: \"%s\"", sqlite3_errmsg(db));

            rc = sqlite3_step(select_path_stmt);
            THROW_EXC_IF_FAILED(rc == SQLITE_ROW, "sqlite3_step() failed: \"%s\"", sqlite3_errmsg(db));

            path = std::string(
                reinterpret_cast<const char*>(sqlite3_column_text(select_path_stmt, 0)),
                sqlite3_column_bytes(select_path_stmt, 0));

            std::cout << "  " << path << std::endl;

            rc = sqlite3_reset(select_path_stmt);
            THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_reset() failed: \"%s\"", sqlite3_errmsg(db));
        }

        std::cout << std::endl;
    }

    rc = sqlite3_finalize(select_path_stmt);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_finalize() failed: \"%s\"", sqlite3_errmsg(db));

    rc = sqlite3_finalize(iterate_over_clusters_stmt);
    THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_finalize() failed: \"%s\"", sqlite3_errmsg(db));
}

int
main(int argc, char **argv)
{
    if (argc < 3) {
        usage(argv[0]);
    }

    std::string db_file = argv[1];
    std::string clusters_table = argv[2];

    try {
        sqlite3_initialize();

        sqlite3 *db = NULL;
        int      rc;

        rc = sqlite3_open_v2(db_file.c_str(), &db, SQLITE_OPEN_READONLY, NULL);
        THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_open_v2() failed");

        print(db, clusters_table);

        rc = sqlite3_close(db);
        THROW_EXC_IF_FAILED(rc == SQLITE_OK, "sqlite3_close() failed: %i", rc);

        sqlite3_shutdown();
    } catch (std::exception &exc) {
        std::cerr << "Error: " << exc.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
