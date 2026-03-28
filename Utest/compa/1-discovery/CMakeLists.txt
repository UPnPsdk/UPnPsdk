# Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2026-03-27

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(GTESTS_COMPA_SSDP VERSION 0002
                  DESCRIPTION "Tests for the compa ssdp module"
                  HOMEPAGE_URL "https://github.com/UPnPsdk")


# ssdp_common
#============
# Because we want to include the source file into the test to also test static
# functions, we cannot use shared libraries due to symbol import/export
# conflicts. We must use static libraries.

add_executable(test_ssdp_common-pst
#---------------------------
    ./test_ssdp_common.cpp
)
target_include_directories(test_ssdp_common-pst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_ssdp_common-pst
    PRIVATE
        pupnp_static
        UPnPsdk_static # static needed for win32
        utest_shared
)
add_test(NAME ctest_ssdp_common-pst COMMAND test_ssdp_common-pst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(test_ssdp_common-cst
#---------------------------
    ./test_ssdp_common.cpp
)
target_include_directories(test_ssdp_common-cst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_ssdp_common-cst
    PRIVATE
        compa_static
        utest_shared
)
add_test(NAME ctest_ssdp_common-cst COMMAND test_ssdp_common-cst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# ssdp_device
#============
add_executable(test_ssdp_device-pst
#----------------------------------
    ./test_ssdp_device.cpp
)
target_include_directories(test_ssdp_device-pst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_ssdp_device-pst
    PRIVATE
        pupnp_static
        UPnPsdk_static # static needed for win32
        utest_shared
)
add_test(NAME ctest_ssdp_device-pst COMMAND test_ssdp_device-pst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(test_ssdp_device-cst
#----------------------------------
    ./test_ssdp_device.cpp
)
target_include_directories(test_ssdp_device-cst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_ssdp_device-cst
    PRIVATE
        compa_static
        utest_shared
)
add_test(NAME ctest_ssdp_device-cst COMMAND test_ssdp_device-cst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)
