# Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2025-05-31

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(GTESTS_COMPA_HTTP VERSION 0004
                  DESCRIPTION "Tests for the compa http module"
                  HOMEPAGE_URL "https://github.com/UPnPsdk")


# webserver
#==========
# Because we want to include the source file into the test to also test static
# functions, we cannot use shared libraries due to symbol import/export
# conflicts. We must use static libraries.
add_executable(test_webserver-pst
#--------------------------------
    ./test_webserver.cpp
)
target_include_directories(test_webserver-pst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_compile_options(test_webserver-pst
    PRIVATE
        # To fix these warnigs we have to modify the original pupnp code but
        # that isn't intended. So the warnings are suppressed.
        # 'buf’ may be used uninitialized in this function [-Werror=maybe-uninitialized]
        $<$<CXX_COMPILER_ID:GNU>:-Wno-maybe-uninitialized>
        # disable warning C4701: potentially uninitialized local variable 'subtype' used
        $<$<CXX_COMPILER_ID:MSVC>:/wd4701>
        # disable warning C4703: potentially uninitialized local pointer variable 'subtype' used
        $<$<CXX_COMPILER_ID:MSVC>:/wd4703>
)
target_link_libraries(test_webserver-pst
    PRIVATE pupnp_static
    UPnPsdk_static # must be static for win32
    PRIVATE utest_shared
)
add_test(NAME ctest_webserver-pst COMMAND test_webserver-pst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(test_webserver-cst
#--------------------------------
    ./test_webserver.cpp
)
target_include_directories(test_webserver-cst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_webserver-cst
    PRIVATE compa_static
)
add_test(NAME ctest_webserver-cst COMMAND test_webserver-cst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# httpparser
#===========
# Because we want to include the source file into the test to also test static
# functions, we cannot use shared libraries due to symbol import/export
# conflicts. We must use static libraries.
add_executable(test_httpparser-pst
#---------------------------------
    ./test_httpparser.cpp
)
target_include_directories(test_httpparser-pst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_compile_options(test_httpparser-pst
    # To fix these warnigs we have to modify the original pupnp code but
    # that isn't intended. So the warnings are suppressed.
    # disable warning C4706: assignment within conditional expression
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/wd4706>
    # disable warning C4701: potentially uninitialized local variable 'status' used
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/wd4701>
)
target_link_libraries(test_httpparser-pst
    PRIVATE pupnp_static
)
add_test(NAME ctest_httpparser-pst COMMAND test_httpparser-pst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(test_httpparser-cst
#---------------------------------
    ./test_httpparser.cpp
)
target_include_directories(test_httpparser-cst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_httpparser-cst
    PRIVATE compa_static
)
add_test(NAME ctest_httpparser-cst COMMAND test_httpparser-cst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


add_subdirectory(./httpreadwrite.d)
