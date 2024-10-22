# Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2024-10-22

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(GTESTS_COMPA_SSDP VERSION 0002
                  DESCRIPTION "Tests for the compa ssdp module"
                  HOMEPAGE_URL "https://github.com/UPnPsdk")


# ssdp
#=====
# Because we want to include the source file into the test to also test static
# functions, we cannot use shared libraries due to symbol import/export
# conflicts. We must use static libraries.

add_executable(test_ssdp-pst
#---------------------------
    ./test_ssdp.cpp
)
target_include_directories(test_ssdp-pst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_ssdp-pst
    PRIVATE
        pupnp_static
        utest_static
)
add_test(NAME ctest_ssdp-pst COMMAND test_ssdp-pst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

if(FALSE)
add_executable(test_ssdp-cst
#---------------------------
    ./test_ssdp.cpp
)
target_compile_definitions(test_ssdp-cst
    PRIVATE
        COMPA_HAVE_CTRLPT_SSDP
        COMPA_HAVE_DEVICE_SSDP
)
target_link_libraries(test_ssdp-cst
    PRIVATE
        compa_static
        UPnPsdk_static
)
add_test(NAME ctest_ssdp-cst COMMAND test_ssdp-cst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)
endif()
