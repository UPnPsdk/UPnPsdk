# Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2025-09-28

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(GTESTS_COMPA_URI VERSION 0004
                  DESCRIPTION "Tests for the uri module and url class"
                  HOMEPAGE_URL "https://github.com/UPnPsdk")


# uri
#====
# Because we want to include the source file into the test to also test static
# functions, we cannot use shared libraries due to symbol import/export
# conflicts. We must use static libraries.

add_executable(test_uri-pst
    ./test_uri.cpp
)
target_include_directories(test_uri-pst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_uri-pst
    PRIVATE pupnp_static
    PRIVATE UPnPsdk_static # win32 needs static
    PRIVATE utest_shared
)
add_test(NAME ctest_uri-pst COMMAND test_uri-pst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(test_uri-cst
    ./test_uri.cpp
)
target_include_directories(test_uri-cst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_uri-cst
    PRIVATE compa_static
    PRIVATE utest_shared
)
add_test(NAME ctest_uri-cst COMMAND test_uri-cst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# uri: uri_parse
#===============
# Because we want to include the source file into the test to also test static
# functions, we cannot use shared libraries due to symbol import/export
# conflicts. We must use static libraries.

add_executable(test_uri_parse-pst
    ./test_uri_parse.cpp
)
target_include_directories(test_uri_parse-pst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_uri_parse-pst
    PRIVATE
        pupnp_static
        utest_static
)
add_test(NAME ctest_uri_parse-pst COMMAND test_uri_parse-pst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(test_uri_parse-cst
    ./test_uri_parse.cpp
)
target_include_directories(test_uri_parse-cst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_uri_parse-cst
    PRIVATE
        compa_static
        utest_shared
)
add_test(NAME ctest_uri_parse-cst COMMAND test_uri_parse-cst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)
