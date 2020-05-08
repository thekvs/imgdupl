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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W")

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
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
find_package(Threads REQUIRED)

pkg_check_modules(GRAPHICSMAGICK REQUIRED GraphicsMagick++)
pkg_check_modules(EIGEN3 REQUIRED eigen3)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

add_library(
    imghash-static
        STATIC
            ${imghash_SOURCE_DIR}/tokenizer.cpp
)

target_include_directories(
    imghash-static
        SYSTEM PRIVATE
            ${imghash_SOURCE_DIR}
            ${Boost_INCLUDE_DIRS}
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

set(LINKLIBS
    ${GRAPHICSMAGICK_LIBRARIES}
    ${Boost_LIBRARIES}
    ${SQLITE3_LIBRARIES}
)

add_executable(
    imghash
    ${imghash_SOURCE_DIR}/imghash.cpp
)

target_include_directories(
    imghash
        SYSTEM PRIVATE
            ${imghash_SOURCE_DIR}
            ${CXXOPTS_ROOT}/include/
            ${Boost_INCLUDE_DIRS}
            ${GRAPHICSMAGICK_INCLUDE_DIRS}
            ${EIGEN3_INCLUDE_DIRS}
            ${SQLITE3_INCLUDE_DIRS}
            ${CURSES_INCLUDE_DIRS}
            ${PROGRESSBAR_INCLUDE_DIRS}
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
            ${PROGRESSBAR_LIBRARIES}
            imghash-static
            spdlog::spdlog
            ${GRAPHICSMAGICK_LIBRARIES}
            ${Boost_LIBRARIES}
            ${SQLITE3_LIBRARIES}
            ${CURSES_LIBRARIES}
)

add_executable(
    clusterizer
    ${imghash_SOURCE_DIR}/clusterizer.cpp
)

target_include_directories(
    clusterizer
        SYSTEM PRIVATE
            ${CXXOPTS_ROOT}/include/
            ${Boost_INCLUDE_DIRS}
            ${GRAPHICSMAGICK_INCLUDE_DIRS}
            ${EIGEN3_INCLUDE_DIRS}
            ${SQLITE3_INCLUDE_DIRS}
            ${CURSES_INCLUDE_DIRS}
)

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
            ${SQLITE3_LIBRARIES}
            Threads::Threads
)

add_executable(
    distance
    ${imghash_SOURCE_DIR}/distance.cpp
)

target_include_directories(
    distance
        SYSTEM PRIVATE
            ${CXXOPTS_ROOT}/include/
)

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
)

add_executable(
    export2db
    ${imghash_SOURCE_DIR}/export2db.cpp
)

target_include_directories(
    export2db
        SYSTEM PRIVATE
            ${CXXOPTS_ROOT}/include/
)

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
            ${SQLITE3_LIBRARIES}
)

add_executable(
    print-clusters
    ${imghash_SOURCE_DIR}/print-clusters.cpp
)

target_include_directories(
    print-clusters
        SYSTEM PRIVATE
            ${imghash_SOURCE_DIR}
            ${CXXOPTS_ROOT}/include/
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
            ${SQLITE3_LIBRARIES}
            spdlog::spdlog
            imghash-static
)
