project (siphon_tests)


include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

#include_directories("../FEC/include")
#include_directories("../FEC/src")
#include_directories ("/usr/local/include")
#include_directories("/usr/include")
#
#file(GLOB SIPHON_FEC_SOURCES
#        "../FEC/src/*.cpp"
#        )
#
#file(GLOB SIPHON_FEC_HEADERS
#        "../FEC/include/*.h"
#        )
#
#
#
#link_directories("/usr/local/lib")
#link_directories("/usr/lib")
#
#
#add_library(siphon_fec ${SIPHON_FEC_SOURCES})

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_ROOT_DIR}/bin)



####################
# Build Unit Tests #
####################
#file (GLOB testfiles "${CMAKE_CURRENT_SOURCE_DIR}/gtest_*.cpp")
#foreach (testfile ${testfiles})
#    get_filename_component (filename ${testfile} NAME_WE)
#    string (REPLACE "gtest_"
#            "" testname
#            ${filename})
#    string (TOUPPER ${testname} TESTNAME)
#    add_executable(${filename} "${filename}.cpp")
#    target_link_libraries(${filename}
#            gtest gtest_main siphon ${Boost_LIBRARIES} siphon_proto)
#    add_test(NAME "${TESTNAME}" COMMAND "${filename}")
#endforeach()
