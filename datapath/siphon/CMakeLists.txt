cmake_minimum_required(VERSION 3.12)

#######################
# Dependencies
#######################

## boost
find_package(Boost 1.67.0 COMPONENTS REQUIRED
        system date_time thread log coroutine)
include_directories(${Boost_INCLUDES})
include_directories("/usr/local/include")
set(Boost_USE_STATIC_LIBS ON) # enable dynamic linking
set(Boost_USE_MULTITHREAD ON) # enable multithreading

##Isa-l
include_directories ("/usr/local/include")
link_directories("/usr/local/lib")

# Directories
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_ROOT_DIR}/bin)

#######################
# Child CMakeLists
#######################

add_subdirectory("proto")

## rapidjson
set(RAPIDJSON_USE_SSE42 ON)
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL " ")
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL " ")
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL " ")
add_subdirectory("third_party/rapidjson")
include_directories("third_party/rapidjson/include")

#######################
# Library Definition
#######################

include_directories(".")

file(GLOB SIPHON_SOURCES
        "aggregator.cpp"
        "apps/*.cpp"
        "base/*.cpp"
        "coder/*.cpp"
        "icoder/src/*.cpp"
        "controller/*.cpp"
        "networking/*.cpp"
        "util/*.cpp"
        "util/*.c"
        "FEC/src/*.cpp"
        )

file(GLOB SIPHON_HEADERS
        "aggregator.hpp"
        "apps/*.hpp"
        "base/*.hpp"
        "base/*.h"
        "coder/*.hpp"
        "coder/*.h"
        "icoder/include/*.h"
        "controller/*.hpp"
        "networking/*.hpp"
        "util/*.hpp"
        "util/*.h"
        "FEC/include/*.h"
        )

add_library(siphon ${SIPHON_SOURCES})
add_dependencies(siphon siphon_proto)

########################
## Tests
########################
##if (test)
#enable_testing()
#set(BUILD_GMOCK OFF CACHE BOOL " ")
#set(BUILD_GTEST ON CACHE BOOL " ")
#add_subdirectory("third_party/googletest")
#add_subdirectory("tests")
##endif()
