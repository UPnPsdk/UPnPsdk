# Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2024-10-25

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(PUPNP_IXML_TEST VERSION 0007
                   DESCRIPTION "IXML library tests"
                   HOMEPAGE_URL "https://github.com/UPnPsdk")


# ixml test executable linked with shared library
#------------------------------------------------
add_executable (test_ixml-ish
    test_ixml.cpp
)
target_link_libraries (test_ixml-ish
    PRIVATE ixml_shared
)
add_test(NAME ctest_ixml-ish COMMAND test_ixml-ish --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

# ixml test executable linked with static library
#------------------------------------------------
add_executable (test_ixml-ist
    test_ixml.cpp
)
target_include_directories(test_ixml-ist
    PRIVATE ${CMAKE_SOURCE_DIR}/UPnPsdk/include
)
target_link_libraries(test_ixml-ist
    PRIVATE ixml_static
)
add_test(NAME ctest_ixml-ist COMMAND test_ixml-ist --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)
