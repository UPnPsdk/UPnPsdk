# Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2024-10-29

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(PUPNP_IXML_TEST VERSION 0008
                   DESCRIPTION "IXML library tests"
                   HOMEPAGE_URL "https://github.com/UPnPsdk")


#------------------------------------------------
add_executable (test_ixml_documents-ish
    ${PUPNP_IXML_SOURCE_DIR}/test/test_document.cpp
)
target_link_libraries (test_ixml_documents-ish
    PRIVATE ixml_shared
)
file(GLOB_RECURSE IXML_TESTDATA_FILES ${PUPNP_IXML_SOURCE_DIR}/test/*.xml
)
# ixml test executable linked with shared library
add_test(NAME ctest_ixml_documents-ish
    COMMAND test_ixml_documents-ish ${IXML_TESTDATA_FILES}
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

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
