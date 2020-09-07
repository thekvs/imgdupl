cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(imgdupl-superbuild)

include(ExternalProject)
include(ProcessorCount)

ProcessorCount(CPU_CORES)

set(CXXOPTS_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/third_party/cxxopts)
set(CXXOPTS_INSTALL_PREFIX ${CXXOPTS_PREFIX}/install)
ExternalProject_Add(
    cxxopts
    PREFIX ${CXXOPTS_PREFIX}
    URL "https://github.com/jarro2783/cxxopts/archive/v2.1.2.tar.gz"
    URL_HASH MD5=51af98ad0b7ec6fb7aa1cf96ea65fff8
    CMAKE_ARGS
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_INSTALL_PREFIX=${CXXOPTS_INSTALL_PREFIX}
    LOG_INSTALL ON
    LOG_DOWNLOAD ON
)

set(SPDLOG_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/third_party/spdlog)
set(SPDLOG_INSTALL_PREFIX ${SPDLOG_PREFIX}/install)
ExternalProject_Add(
    spdlog
    PREFIX ${SPDLOG_PREFIX}
    URL "https://github.com/gabime/spdlog/archive/v1.5.0.tar.gz"
    URL_HASH MD5=a966eea01f81551527853d282896cb4d
    CMAKE_ARGS
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DSPDLOG_BUILD_BENCH=OFF
        -DSPDLOG_BUILD_TESTS=OFF
        -DCMAKE_INSTALL_PREFIX=${SPDLOG_INSTALL_PREFIX}
    LOG_DOWNLOAD ON
    LOG_INSTALL ON
)

set(INDICATORS_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/third_party/indicators)
set(INDICATORS_INSTALL_PREFIX ${INDICATORS_PREFIX}/install)
ExternalProject_Add(
    indicators
    PREFIX ${INDICATORS_PREFIX}
    URL "https://github.com/p-ranav/indicators/archive/v1.9.tar.gz"
    URL_HASH MD5=012f7589be3e976dc193c51b766dd233
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${INDICATORS_INSTALL_PREFIX}
    LOG_INSTALL ON
)

set(NLOHMANN_JSON_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/third_party/json)
set(NLOHMANN_JSON_INSTALL_PREFIX ${NLOHMANN_JSON_PREFIX}/install)
ExternalProject_Add(
    nlohmann_json
    PREFIX ${NLOHMANN_JSON_PREFIX}
    URL "https://github.com/nlohmann/json/releases/download/v3.7.3/json.hpp"
    URL_HASH MD5=39b754f6834e64406d7eae9dfb9e5d9e
    LOG_DOWNLOAD ON
    DOWNLOAD_NO_EXTRACT ON
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND mkdir -p ${NLOHMANN_JSON_INSTALL_PREFIX}/include/nlohmann &&
        cp -a ${NLOHMANN_JSON_PREFIX}/src/json.hpp ${NLOHMANN_JSON_INSTALL_PREFIX}/include/nlohmann/
)
set(NLOHMANN_JSON_INCLUDE_DIRS ${NLOHMANN_JSON_INSTALL_PREFIX}/include)

ExternalProject_Add(
    imgdupl
    PREFIX imgdupl
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.."
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/imgdupl"
    INSTALL_COMMAND ""
    CMAKE_ARGS
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -Dcxxopts_DIR:PATH=${CXXOPTS_INSTALL_PREFIX}/lib/cmake/cxxopts/
        -Dspdlog_DIR:PATH=${SPDLOG_INSTALL_PREFIX}/lib/cmake/spdlog
        -Dindicators_DIR:PATH=${INDICATORS_INSTALL_PREFIX}/lib/cmake/indicators
        -DNLOHMANN_JSON_INCLUDE_DIRS:STRING=${NLOHMANN_JSON_INCLUDE_DIRS}
    BUILD_COMMAND $(CMAKE_COMMAND) --build . -- -j${CPU_CORES}
    DEPENDS
        cxxopts
        spdlog
        indicators
        nlohmann_json
)
