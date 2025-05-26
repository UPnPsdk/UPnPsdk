# Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2025-05-27

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(GTEST5_UPnPsdk VERSION 0006
      DESCRIPTION "Tests for UPnPsdk programs"
      HOMEPAGE_URL "https://github.com/UPnPsdk")


# upnptools
#==========
add_executable(test_upnptools-ush
#--------------------------------
    test_upnptools.cpp
)
target_link_libraries(test_upnptools-ush
    PRIVATE UPnPsdk_shared
)
add_test(NAME ctest_upnptools-ush COMMAND test_upnptools-ush --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(test_upnptools-ust
#---------------------------------
    test_upnptools.cpp
)
target_link_libraries(test_upnptools-ust
    PRIVATE UPnPsdk_static
)
add_test(NAME ctest_upnptools-ust COMMAND test_upnptools-ust --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# sockaddr
# ========
add_executable(test_sockaddr-ust
    test_sockaddr.cpp
)
target_include_directories(test_sockaddr-ust
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_sockaddr-ust
    PRIVATE UPnPsdk_static
    PRIVATE utest_shared
)
add_test(NAME ctest_sockaddr-ust COMMAND test_sockaddr-ust --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# CAddrinfo
# =========
add_executable(test_addrinfo-ust
    test_addrinfo.cpp
)
target_include_directories(test_addrinfo-ust
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_addrinfo-ust
    PRIVATE UPnPsdk_static
    PRIVATE utest_shared
)
add_test(NAME ctest_addrinfo-ust COMMAND test_addrinfo-ust --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# CSocket
# =======
add_executable(test_socket-ush
    test_socket.cpp
)
target_link_libraries(test_socket-ush
    PRIVATE UPnPsdk_shared
    PRIVATE utest_shared
)
add_test(NAME ctest_socket-ush COMMAND test_socket-ush --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# webserver
# =========
add_executable(test_webserver-ush
    test_webserver.cpp
)
target_link_libraries(test_webserver-ush
    PRIVATE UPnPsdk_shared
)
add_test(NAME ctest_webserver-ush COMMAND test_webserver-ush --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# general
# =======
if(NOT WIN32)
add_executable(test_general-ush
    test_general.cpp
)
target_link_libraries(test_general-ush
    PRIVATE UPnPsdk_shared
    PRIVATE utest_shared
)
add_test(NAME ctest_general-ush COMMAND test_general-ush --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)
endif()


# strintmap
# =========
add_executable(test_strintmap-ush
    test_strintmap.cpp
)
target_link_libraries(test_strintmap-ush
    PRIVATE UPnPsdk_shared
)
add_test(NAME ctest_strintmap-ush COMMAND test_strintmap-ush --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# netadapter
# ==========
add_executable(test_netadapter-ust
    test_netadapter.cpp
)
target_include_directories(test_netadapter-ust
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_netadapter-ust
    PRIVATE UPnPsdk_static
    PRIVATE utest_shared
)
add_test(NAME ctest_netadapter-ust COMMAND test_netadapter-ust --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# rootdevice
# ==========
add_executable(test_rootdevice-ush
    test_upnp_device.cpp
)
target_link_libraries(test_rootdevice-ush
    PRIVATE UPnPsdk_shared
    PRIVATE utest_shared
)
add_test(NAME ctest_rootdevice-ush COMMAND test_rootdevice-ush --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


add_subdirectory(test_uri.d)
