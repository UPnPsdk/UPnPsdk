# Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2024-10-22

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(GTESTS_COMPA_HTTP_HTTPREADWRITE VERSION 0002
                  DESCRIPTION "Tests for the compa httpreadwrite module"
                  HOMEPAGE_URL "https://github.com/UPnPsdk")


# httpreadwrite incl. statcodes
#==============================
# Because we want to include the source file into the test to also test static
# functions, we cannot use shared libraries due to symbol import/export
# conflicts. We must use static libraries.

add_executable(test_httpreadwrite-pst
#------------------------------------
    ./test_httpreadwrite.cpp
)
target_include_directories(test_httpreadwrite-pst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_compile_options(test_httpreadwrite-pst
    PRIVATE
        # To fix these warnigs we have to modify the original pupnp code but
        # that isn't intended. So the warnings are suppressed.
        #
        # disable warning: 'sprintf' (and others) is deprecated: This function is
        # provided for compatibility reasons only.
        $<$<CXX_COMPILER_ID:AppleClang>:-Wno-deprecated-declarations>
        # disable warning C4706: assignment within conditional expression
        $<$<CXX_COMPILER_ID:MSVC>:/wd4706>
)
target_link_libraries(test_httpreadwrite-pst
    PRIVATE
        pupnp_static
        utest_static
)
add_test(NAME ctest_httpreadwrite-pst COMMAND test_httpreadwrite-pst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(test_httpreadwrite-cst
#------------------------------------
    ./test_httpreadwrite.cpp
)
target_include_directories(test_httpreadwrite-cst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_httpreadwrite-cst
    PRIVATE
        compa_static
        utest_static
)
add_test(NAME ctest_httpreadwrite-cst COMMAND test_httpreadwrite-cst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# httpreadwrite: netconnect
#==========================
# This is also part of httpreadwrite but splitted to have a better overview.
# It's intended to make netconnect a separate modul because it seems to be a
# general base function.
#
# Because we want to include the source file into the test to also test static
# functions, we cannot use shared libraries due to symbol import/export
# conflicts. We must use static libraries.
add_executable(test_netconnect-pst
#---------------------------------
    ./test_netconnect.cpp
)
target_include_directories(test_netconnect-pst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_compile_options(test_netconnect-pst
    PRIVATE
        # To fix these warnigs we have to modify the original pupnp code but
        # that isn't intended. So the warnings are suppressed.
        # disable warning: 'sprintf' (and others) is deprecated: This function is
        # provided for compatibility reasons only.
        $<$<CXX_COMPILER_ID:AppleClang>:-Wno-deprecated-declarations>
        # disable warning C4706: assignment within conditional expression
        $<$<CXX_COMPILER_ID:MSVC>:/wd4706>
)
target_link_libraries(test_netconnect-pst
    PRIVATE pupnp_static
    PRIVATE utest_static
)
add_test(NAME ctest_netconnect-pst COMMAND test_netconnect-pst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(test_netconnect-cst
#---------------------------------
    ./test_netconnect.cpp
)
target_include_directories(test_netconnect-cst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_netconnect-cst
    PRIVATE compa_static
    PRIVATE utest_static
)
add_test(NAME ctest_netconnect-cst COMMAND test_netconnect-cst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# httpreadwrite: http_Download
#=============================
# This is also part of httpreadwrite but splitted to have a better overview.
#
# Because we want to include the source file into the test to also test static
# functions, we cannot use shared libraries due to symbol import/export
# conflicts. We must use static libraries.
add_executable(test_http_Download-pst
#------------------------------------
    ./test_http_Download.cpp
)
target_include_directories(test_http_Download-pst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_compile_options(test_http_Download-pst
    PRIVATE
        # disable warning: 'sprintf' (and others) is deprecated: This function is
        # provided for compatibility reasons only.
        $<$<CXX_COMPILER_ID:AppleClang>:-Wno-deprecated-declarations>
        # disable warning C4706: assignment within conditional expression
        $<$<CXX_COMPILER_ID:MSVC>:/wd4706>
)
target_link_libraries(test_http_Download-pst
    PRIVATE
        pupnp_static
        utest_static
)
add_test(NAME ctest_http_Download-pst COMMAND test_http_Download-pst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(test_http_Download-cst
#------------------------------------
    ./test_http_Download.cpp
)
target_include_directories(test_http_Download-cst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_http_Download-cst
    PRIVATE
        compa_static
        utest_static
)
add_test(NAME ctest_http_Download-cst COMMAND test_http_Download-cst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)
