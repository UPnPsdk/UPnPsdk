# Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2024-10-05

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(GTESTS_SAMPLE VERSION 0007
        DESCRIPTION "Tests for upnplib sample programs"
        HOMEPAGE_URL "https://github.com/UPnPsdk")


if(NOT WIN32)
# test tv_device build with pupnp shared library
#-----------------------------------------------
add_executable(test_tv_device-psh
    ${SAMPLE_SOURCE_DIR}/common/sample_util.cpp
    ./test_tv_device.cpp
)
target_include_directories(test_tv_device-psh
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_compile_options(test_tv_device-psh
    # disable warning C4706: assignment within conditional expression.
    # This is intended but bad practice. I won't modify pupnp code. --Ingo
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/wd4706>
)
target_link_libraries(test_tv_device-psh
    PRIVATE
        pupnp_shared
        UPnPsdk_shared
        utest_shared
)
add_test(NAME ctest_tv_device-psh COMMAND test_tv_device-psh --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

# test tv_device build with compatible shared library
#----------------------------------------------------
add_executable(test_tv_device-csh
    ${SAMPLE_SOURCE_DIR}/common/sample_util.cpp
    ./test_tv_device.cpp
)
target_include_directories(test_tv_device-csh
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_tv_device-csh
    PRIVATE
        compa_shared
        UPnPsdk_shared
        utest_shared
)
add_test(NAME ctest_tv_device-csh COMMAND test_tv_device-csh --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)
endif()
