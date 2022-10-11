cmake_minimum_required(VERSION 3.12)

project (siphon)
set (siphon_VERSION_MAJOR 2)
set (siphon_VERSION_MINOR 0)

set(CMAKE_CXX_STANDARD 17)
set (CMAKE_BUILD_TYPE Debug)

# Directories
set (PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (EXECUTABLE_OUTPUT_PATH ${PROJECT_ROOT_DIR}/bin)

#######################
# Libraries
#######################

find_package (Boost 1.67.0 COMPONENTS REQUIRED
        system date_time thread log coroutine)

include_directories(
        "siphon"
        "siphon/third_party/rapidjson/include"
        "/usr/local/include"
        "siphon/FEC/include"
        "siphon/icoder/include"
)
add_subdirectory("unit-testing")
link_directories("/usr/local/lib")
add_subdirectory(siphon)
add_executable(siphon_datapath main.cpp)
target_link_libraries (siphon_datapath
        siphon
        siphon_proto
        ${Boost_LIBRARIES}
        isal
        )

#add_executable(sender_only_siphon_datapath main_sender_only.cpp)
#target_link_libraries (sender_only_siphon_datapath
#        siphon
#        siphon_proto
#        ${Boost_LIBRARIES}
#        isal
#        )

# enable_testing()