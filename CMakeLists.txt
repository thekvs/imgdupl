project(imghash)
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

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

find_package(cxxopts CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(indicators CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread filesystem system)
find_package(Eigen3 CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(GRAPHICSMAGICK REQUIRED IMPORTED_TARGET GraphicsMagick++)

add_library(imghash-static STATIC ${imghash_SOURCE_DIR}/tokenizer.cpp)

target_include_directories(imghash-static SYSTEM PRIVATE ${imghash_SOURCE_DIR})
target_compile_options(imghash-static PRIVATE -W -Wall -Wextra)
set_target_properties(imghash-static PROPERTIES
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
target_include_directories(imghash SYSTEM PRIVATE ${imghash_SOURCE_DIR} ${GRAPHICSMAGICK_INCLUDE_DIRS})
set_target_properties(imghash PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_link_libraries(imghash PRIVATE
    imghash-static
    spdlog::spdlog
    cxxopts::cxxopts
    indicators::indicators
    unofficial::sqlite3::sqlite3
    Eigen3::Eigen
    Boost::filesystem
    fmt::fmt
    PkgConfig::GRAPHICSMAGICK
    ${CMAKE_DL_LIBS}
    Threads::Threads
)

add_executable(
    clusterizer
    ${imghash_SOURCE_DIR}/clusterizer.cpp
)

target_compile_options(clusterizer PRIVATE -W -Wall -Wextra)

set_target_properties(clusterizer PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    COMPILE_FLAGS "-mpopcnt"
)

target_link_libraries(clusterizer PRIVATE
    imghash-static
    cxxopts::cxxopts
    unofficial::sqlite3::sqlite3
    Threads::Threads
)

add_executable(distance
    ${imghash_SOURCE_DIR}/distance.cpp
)

target_compile_options(distance PRIVATE -W -Wall -Wextra)

set_target_properties(distance PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    COMPILE_FLAGS "-mpopcnt"
)

target_link_libraries(distance PRIVATE
    imghash-static
    cxxopts::cxxopts
)

add_executable(
    export2db
    ${imghash_SOURCE_DIR}/export2db.cpp
)

target_compile_options(export2db PRIVATE -W -Wall -Wextra)

set_target_properties(export2db PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_link_libraries(export2db PRIVATE
    imghash-static
    cxxopts::cxxopts
    unofficial::sqlite3::sqlite3
)

add_executable(
    print-clusters
    ${imghash_SOURCE_DIR}/print-clusters.cpp
)

target_compile_options(print-clusters PRIVATE -W -Wall -Wextra)

target_include_directories(print-clusters SYSTEM PRIVATE ${imghash_SOURCE_DIR})
set_target_properties(print-clusters PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_link_libraries(print-clusters PRIVATE
    imghash-static
    spdlog::spdlog
    cxxopts::cxxopts
    nlohmann_json::nlohmann_json
    unofficial::sqlite3::sqlite3
)
