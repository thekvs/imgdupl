project(imghash)
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

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

pkg_check_modules(GRAPHICSMAGICK REQUIRED GraphicsMagick++)
pkg_check_modules(EIGEN3 REQUIRED eigen3)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

include_directories(${imghash_SOURCE_DIR})

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${GRAPHICSMAGICK_INCLUDE_DIRS})
include_directories(${EIGEN3_INCLUDE_DIRS})
include_directories(${SQLITE3_INCLUDE_DIRS})
include_directories(${CURSES_INCLUDE_DIRS})

link_directories(${GRAPHICSMAGICK_LIBRARY_DIRS})

set(PROGRESSBAR_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/third_party/progressbar)
set(PROGRESSBAR_INSTALL_PREFIX ${PROGRESSBAR_PREFIX}/install)
ExternalProject_Add(
    progressbar
    PREFIX ${PROGRESSBAR_PREFIX}
    URL "https://github.com/thekvs/progressbar/archive/master.zip"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PROGRESSBAR_INSTALL_PREFIX}
    LOG_INSTALL ON
)
set(PROGRESSBAR_INCLUDE_DIRS ${PROGRESSBAR_INSTALL_PREFIX}/include)
set(PROGRESSBAR_LIBRARIES ${PROGRESSBAR_INSTALL_PREFIX}/lib/libprogressbar.a)
include_directories(${PROGRESSBAR_INCLUDE_DIRS})
message(STATUS "Using progressbar headers from ${PROGRESSBAR_INCLUDE_DIRS}")
message(STATUS "Using progressbar library ${PROGRESSBAR_LIBRARIES}")

set(SOURCES
    ${imghash_SOURCE_DIR}/tokenizer.cpp
    ${imghash_SOURCE_DIR}/third_party/easylogging/easylogging++.cc
)

add_library(imghash-static STATIC ${SOURCES})
set_target_properties(imghash-static PROPERTIES COMPILE_FLAGS "-std=c++11")
set_target_properties(imghash-static PROPERTIES OUTPUT_NAME imghash)
set_target_properties(imghash-static PROPERTIES LINKER_LANGUAGE CXX)

set(LINKLIBS
    ${GRAPHICSMAGICK_LIBRARIES}
    ${Boost_LIBRARIES}
    ${SQLITE3_LIBRARIES}
)

add_executable(imghash ${imghash_SOURCE_DIR}/imghash.cpp)
set_target_properties(imghash PROPERTIES COMPILE_FLAGS "-std=c++11")
target_link_libraries(imghash ${LINKLIBS} ${PROGRESSBAR_LIBRARIES} ${CURSES_LIBRARIES} imghash-static)
add_dependencies(imghash progressbar)

add_executable(clusterizer ${imghash_SOURCE_DIR}/clusterizer.cpp)
set_target_properties(clusterizer PROPERTIES COMPILE_FLAGS "-std=c++11 -mpopcnt")
target_link_libraries (clusterizer imghash-static ${LINKLIBS} ${CMAKE_THREAD_LIBS_INIT})

add_executable(distance ${imghash_SOURCE_DIR}/distance.cpp)
set_target_properties(distance PROPERTIES COMPILE_FLAGS "-mpopcnt")
target_link_libraries(distance imghash-static)

add_executable(export2db ${imghash_SOURCE_DIR}/export2db.cpp)
set_target_properties(export2db PROPERTIES COMPILE_FLAGS "-std=c++11")
target_link_libraries(export2db ${LINKLIBS} imghash-static)

add_executable(print-clusters ${imghash_SOURCE_DIR}/print-clusters.cpp)
set_target_properties(print-clusters PROPERTIES COMPILE_FLAGS "-std=c++11")
target_link_libraries(print-clusters ${LINKLIBS} imghash-static)
