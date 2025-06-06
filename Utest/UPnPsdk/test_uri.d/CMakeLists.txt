# Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2024-10-07

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(GTESTS_UPnPsdk_URI VERSION 0003
                  DESCRIPTION "Tests for the uri module and url class"
                  HOMEPAGE_URL "https://github.com/UPnPsdk")


# uri: urlparser
#===============
add_executable(test_urlparser
    ./test_urlparser.cpp
)
target_include_directories(test_urlparser
    PRIVATE ${CMAKE_SOURCE_DIR}
    PRIVATE ${CMAKE_SOURCE_DIR}/UPnPsdk/include
)
add_test(NAME ctest_urlparser COMMAND test_urlparser --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# uri: url_class
#===============
if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
# This Unit needs rework on WIN32 with CMAKE_BUILD_TYPE=Debug due to several
# iterator bounding asserts. Because this Unit is currently not used I disable
# its test. --Ingo
else()
add_executable(test_url_class
    ./test_url_class.cpp
)
target_include_directories(test_url_class
    PRIVATE ${CMAKE_SOURCE_DIR}
    PRIVATE ${CMAKE_SOURCE_DIR}/UPnPsdk/include
)
add_test(NAME ctest_url_class COMMAND test_url_class --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)
endif()
