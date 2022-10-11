cmake_minimum_required(VERSION 3.15)

#######################
# Dependencies
#######################

# Directories
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_ROOT_DIR}/bin)


#######################
# Library Definition
#######################

include_directories(".")

file(GLOB MAIN_SOURCES
        "FIFOqueue/*.c"
        "utils/*.c"
        )

file(GLOB MAIN_HEADERS
        "main_Macro.h"
        "message.h"
        "FIFOqueue/*.h"
        "utils/*.h"
        )

add_library(main_src ${MAIN_SOURCES})