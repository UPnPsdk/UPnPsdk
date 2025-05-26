# Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2025-05-27

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(GTESTS_COMPA_UTIL VERSION 0002
                  DESCRIPTION "Tests for utilities and helper"
                  HOMEPAGE_URL "https://github.com/UPnPsdk")

# list
#=====
add_executable(test_list-psh
        ./test_list.cpp
)
target_link_libraries(test_list-psh
    PRIVATE pupnp_shared
)
add_test(NAME ctest_list-psh COMMAND test_list-psh --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(test_list-csh
        ./test_list.cpp
)
target_link_libraries(test_list-csh
    PRIVATE compa_shared
)
add_test(NAME ctest_list-csh COMMAND test_list-csh --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# membuffer
#==========
add_executable(test_membuffer-psh
        ./test_membuffer.cpp
)
target_include_directories(test_membuffer-psh
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_membuffer-psh
    PRIVATE pupnp_shared
    PRIVATE utest_shared
)
add_test(NAME ctest_membuffer-psh COMMAND test_membuffer-psh --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# strintmap
#==========
add_executable(test_strintmap-psh
    test_strintmap.cpp
)
target_link_libraries(test_strintmap-psh
    PRIVATE pupnp_shared
)
add_test(NAME ctest_strintmap-psh COMMAND test_strintmap-psh --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)
