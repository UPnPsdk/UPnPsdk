# Copyright (C) 2026+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2026-03-15

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(GTESTS_COMPA_URI VERSION 0001
                  DESCRIPTION "Tests for the uri module and url class"
                  HOMEPAGE_URL "https://github.com/UPnPsdk")

# uri
#====
# Because we want to include the source file into the test to also test static
# functions, we cannot use shared libraries due to symbol import/export
# conflicts. We must use static libraries.

add_executable(test_uri-ost
    ./test_uri.cpp
)
target_include_directories(test_uri-ost
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_uri-ost
    PRIVATE pupnp_static
    PRIVATE utest_shared
)
add_test(NAME ctest_uri-ost COMMAND test_uri-ost --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)
