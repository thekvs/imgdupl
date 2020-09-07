project(imghash)
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MODULE_PATH ${imghash_SOURCE_DIR}/cmake/)
include(FindSSE)

if (SSE2_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse -msse2 -mfpmath=sse")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2 -mfpmath=sse")
endif()

if (SSE3_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse3")
endif()

if (SSSE3_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mssse3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mssse3")
endif()

if(SSE4_1_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse4.1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.1")
endif()

if (SSE4_2_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse4.2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")
endif()

include(FindPkgConfig)
include(FindThreads)
include(CheckTypeSize)
include(CheckIncludeFile)
include(CheckCXXCompilerFlag)
include(ExternalProject)

find_package(Boost REQUIRED COMPONENTS thread filesystem system)
find_package(Curses REQUIRED)
find_package(cxxopts CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(indicators CONFIG REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(GRAPHICSMAGICK REQUIRED IMPORTED_TARGET GraphicsMagick++)
pkg_check_modules(EIGEN3 REQUIRED IMPORTED_TARGET eigen3)

add_library(
    imghash-static
        STATIC
            ${imghash_SOURCE_DIR}/tokenizer.cpp
)

target_include_directories(
    imghash-static
        SYSTEM PRIVATE
            ${imghash_SOURCE_DIR}
            Boost::boost
)

target_compile_options(imghash-static PRIVATE -W -Wall -Wextra)
set_target_properties(
    imghash-static
        PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
            OUTPUT_NAME imghash
)

add_executable(
    imghash
    ${imghash_SOURCE_DIR}/imghash.cpp
)

target_compile_options(imghash PRIVATE -W -Wall -Wextra)

target_include_directories(
    imghash
        SYSTEM PRIVATE
            ${imghash_SOURCE_DIR}
            ${CURSES_INCLUDE_DIRS}
)

set_target_properties(
    imghash
        PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
)

target_link_libraries(
    imghash
        PRIVATE
            imghash-static
            spdlog::spdlog
            cxxopts::cxxopts
            indicators::indicators
            SQLite::SQLite3
            Boost::filesystem
            PkgConfig::EIGEN3
            PkgConfig::GRAPHICSMAGICK
            ${CURSES_LIBRARIES}
)

add_executable(
    clusterizer
    ${imghash_SOURCE_DIR}/clusterizer.cpp
)

target_compile_options(clusterizer PRIVATE -W -Wall -Wextra)

set_target_properties(
    clusterizer
        PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
        COMPILE_FLAGS "-mpopcnt"
)

target_link_libraries(
    clusterizer
        PRIVATE
            imghash-static
            cxxopts::cxxopts
            SQLite::SQLite3
            Threads::Threads
)

add_executable(
    distance
    ${imghash_SOURCE_DIR}/distance.cpp
)

target_compile_options(distance PRIVATE -W -Wall -Wextra)

set_target_properties(
    distance
        PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
        COMPILE_FLAGS "-mpopcnt"
)

target_link_libraries(
    distance
        PRIVATE
            imghash-static
            cxxopts::cxxopts
)

add_executable(
    export2db
    ${imghash_SOURCE_DIR}/export2db.cpp
)

target_compile_options(export2db PRIVATE -W -Wall -Wextra)

set_target_properties(
    export2db
        PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
)

target_link_libraries(
    export2db
        PRIVATE
            imghash-static
            cxxopts::cxxopts
            SQLite::SQLite3
)

add_executable(
    print-clusters
    ${imghash_SOURCE_DIR}/print-clusters.cpp
)

target_compile_options(print-clusters PRIVATE -W -Wall -Wextra)

target_include_directories(
    print-clusters
        SYSTEM PRIVATE
            ${imghash_SOURCE_DIR}
            ${NLOHMANN_JSON_INCLUDE_DIRS}
)

set_target_properties(
    print-clusters
        PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
)

target_link_libraries(
    print-clusters
        PRIVATE
            imghash-static
            spdlog::spdlog
            cxxopts::cxxopts
            SQLite::SQLite3
)
