cmake_minimum_required(VERSION 3.15)
project(siphon_demo C)

set(CMAKE_C_STANDARD 11)

# Directories
set (PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (EXECUTABLE_OUTPUT_PATH ${PROJECT_ROOT_DIR}/bin)

include_directories(
        "src"
)
add_subdirectory(src)

####################
# Build demos #
####################

file (GLOB demofiles "${CMAKE_CURRENT_SOURCE_DIR}/demos/example-*.c")

foreach (demofile ${demofiles})
    get_filename_component (filename ${demofile} NAME_WE)
    string (REPLACE "example-"
            "" demoname
            ${filename})
    string (TOUPPER ${demoname} DEMONAME)
    add_executable(${filename} "${CMAKE_CURRENT_SOURCE_DIR}/demos/${filename}.c" main.c)
    target_link_libraries(${filename}
            main_src)
    add_test(NAME "${DEMONAME}" COMMAND "${filename}")
endforeach()