# Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2024-10-26

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(UTEST_ADDRESSING VERSION 0012
        DESCRIPTION "Tests for UPnP Addressing pupnp and compatible code"
        HOMEPAGE_URL "https://github.com/UPnPsdk")


# sock
#=====
add_executable(test_sock-psh
        test_sock.cpp
)
target_link_libraries(test_sock-psh
    PRIVATE
        pupnp_shared
        utest_shared
)
add_test(NAME ctest_sock-psh COMMAND test_sock-psh --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(test_sock-cst
        test_sock.cpp
)
target_link_libraries(test_sock-cst
    PRIVATE
        compa_static
        utest_static
)
add_test(NAME ctest_sock-cst COMMAND test_sock-cst --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# sock_ssl
#=========
if(UPnPsdk_WITH_OPENSSL)
    add_executable(test_sock_ssl-psh
        test_sock_ssl.cpp
    )
    target_link_libraries(test_sock_ssl-psh
        PRIVATE
            pupnp_shared
            utest_shared
    )
    add_test(NAME ctest_sock_ssl-psh COMMAND test_sock_ssl-psh --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
    )

    add_executable(test_sock_ssl-cst
        test_sock_ssl.cpp
    )
    target_link_libraries(test_sock_ssl-cst
        PRIVATE
            compa_static
            utest_static
    )
    add_test(NAME ctest_sock_ssl-cst COMMAND test_sock_ssl-cst --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
    )


# openssl_threaded
#=================
if(FALSE)
# TODO: Tests need to be improved not to need a delay to be usable as Unit
# test. They also fail on WIN32 with Exception: Exit code 0xc0000409 means
# STATUS_STACK_BUFFER_OVERRUN.
    add_executable(test_openssl_threaded-psh
        test_openssl_threaded.cpp
    )
    target_link_libraries(test_openssl_threaded-psh
        PRIVATE pupnp_shared
    )
    add_test(NAME ctest_openssl_threaded-psh COMMAND test_openssl_threaded-psh --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
    )

    add_executable(test_openssl_threaded-csh
        test_openssl_threaded.cpp
    )
    target_link_libraries(test_openssl_threaded-csh
        PRIVATE compa_shared
    )
    add_test(NAME ctest_openssl_threaded-csh COMMAND test_openssl_threaded-csh --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
    )
endif()
endif()


# miniserver
#===========
# Because we want to include the source file into the test to also test static
# functions, we cannot use shared libraries due to symbol import/export
# conflicts. We must use static libraries.
#
# Direct linking with source files can be last found in
# commit b9c8fd4ad5f721a893de2181f5638b72b3014f0e.

add_executable(test_miniserver-pst
#---------------------------------
    test_miniserver.cpp
)
target_include_directories(test_miniserver-pst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_compile_definitions(test_miniserver-pst
    PRIVATE INCLUDE_DEVICE_APIS
    PRIVATE INCLUDE_CLIENT_APIS
)
target_link_libraries(test_miniserver-pst
    PRIVATE
        pupnp_static
        utest_static
)
add_test(NAME ctest_miniserver-pst COMMAND test_miniserver-pst --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(test_miniserver-cst
#---------------------------------
    test_miniserver.cpp
)
target_include_directories(test_miniserver-cst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_miniserver-cst
    PRIVATE
        compa_static
        utest_static
)
add_test(NAME ctest_miniserver-cst COMMAND test_miniserver-cst --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# miniserver_run
#===============
add_executable(test_miniserver_run-pst
#-------------------------------------
    test_miniserver_run.cpp
)
target_include_directories(test_miniserver_run-pst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_miniserver_run-pst
    PRIVATE
        pupnp_static
        utest_static
)
add_test(NAME ctest_miniserver_run-pst COMMAND test_miniserver_run-pst --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(test_miniserver_run-cst
#-------------------------------------
    test_miniserver_run.cpp
)
target_include_directories(test_miniserver_run-cst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_miniserver_run-cst
    PRIVATE
        compa_static
        utest_static
)
add_test(NAME ctest_miniserver_run-cst COMMAND test_miniserver_run-cst --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)
