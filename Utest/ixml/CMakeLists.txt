# Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2024-10-09

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(PUPNP_IXML_TEST VERSION 0006
                   DESCRIPTION "IXML library tests"
                   HOMEPAGE_URL "https://github.com/UPnPsdk")


set(IXML_TEST_INCLUDE_DIRECTORIES
    PRIVATE ${CMAKE_BINARY_DIR}/include
)

# ixml test executable linked with shared library
#------------------------------------------------
add_executable (test_ixml_shared
    test_ixml.cpp
)
target_include_directories(test_ixml_shared
    ${IXML_TEST_INCLUDE_DIRECTORIES}
)
target_compile_definitions (test_ixml_shared
    PRIVATE $<$<CONFIG:Debug>:DEBUG>
)
target_link_libraries (test_ixml_shared
    PRIVATE ixml_shared
)
add_test(NAME ctest_ixml-ish COMMAND test_ixml-ish --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

# ixml test executable linked with static library
#------------------------------------------------
add_executable (test_ixml_static
    test_ixml.cpp
)
target_include_directories(test_ixml_static
    ${IXML_TEST_INCLUDE_DIRECTORIES}
)
target_compile_definitions (test_ixml_static
    PRIVATE $<$<CONFIG:Debug>:DEBUG>
)
target_link_libraries(test_ixml_static
    PRIVATE ixml_static
)
add_test(NAME ctest_ixml-ist COMMAND test_ixml-ist --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)
