# Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2024-09-11

cmake_minimum_required(VERSION 3.23) # for FILE_SET
include(UPnPsdk-ProjectHeader)

project(UPnPsdk VERSION 0012
        DESCRIPTION "UPnP library program"
        HOMEPAGE_URL "https://github.com/UPnPsdk")

add_compile_definitions(
    # Disable warnings to use e.g. strncpy_s instead of strncpy
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
)

set(UPnPsdk_SOURCE_FILES
    src/api/upnptools.cpp
    src/net/netaddr.cpp
    src/net/sockaddr.cpp
    src/net/addrinfo.cpp
    src/net/socket.cpp
    src/net/connection_common.cpp
    src/net/http/webserver.cpp
    src/net/http/httpreadwrite.cpp
)

set(UPnPsdk_PUBLIC_HEADERS
    include/UPnPsdk/port_sock.hpp
    include/UPnPsdk/visibility.hpp
)

set(UPnPsdk_INCLUDE_DIRECTORIES
    PUBLIC include
)


if(NOT WIN32)

    # UPnPsdk libraries
    # =================

    # UPnPsdk SHared library: suffix -ush
    #------------------------------------
    add_library(UPnPsdk_shared SHARED
            ${UPnPsdk_SOURCE_FILES}
    )
    set_target_properties(UPnPsdk_shared PROPERTIES
            OUTPUT_NAME upnpsdk
    )
    target_include_directories(UPnPsdk_shared
            ${UPnPsdk_INCLUDE_DIRECTORIES}
    )
    target_compile_definitions(UPnPsdk_shared
        PRIVATE
            UPnPsdk_EXPORTS
            $<$<BOOL:${UPnPsdk_DEF_IXML}>:IXML_HAVE_SCRIPTSUPPORT>
    )
    target_link_libraries(UPnPsdk_shared
        PUBLIC umock_shared
        INTERFACE ixml_shared
    )
    target_sources(UPnPsdk_shared
        INTERFACE FILE_SET upnpsdk_public_headers TYPE HEADERS
                  BASE_DIRS include
                  FILES ${UPnPsdk_PUBLIC_HEADERS}
    )
    install(TARGETS UPnPsdk_shared
        FILE_SET UPnPsdk_public_headers
        DESTINATION include/upnp
    )

    # UPnPsdk STatic library: suffix -ust
    #------------------------------------
    add_library(UPnPsdk_static STATIC
            ${UPnPsdk_SOURCE_FILES}
    )
    set_target_properties(UPnPsdk_static PROPERTIES
            OUTPUT_NAME upnpsdk
    )
    target_include_directories(UPnPsdk_static
            ${UPnPsdk_INCLUDE_DIRECTORIES}
    )
    target_compile_definitions(UPnPsdk_static
        PRIVATE
            $<$<BOOL:${UPnPsdk_DEF_IXML}>:IXML_HAVE_SCRIPTSUPPORT>
    )
    target_link_libraries(UPnPsdk_static
        PUBLIC umock_static
        INTERFACE ixml_static
    )

    install(TARGETS UPnPsdk_shared UPnPsdk_static)

else() # WIN32

    # WIN32 UPnPsdk libraries
    #========================

    # WIN32 UPnPsdk SHared library: suffix -ush
    #------------------------------------------
    add_library(UPnPsdk_shared SHARED
            ${UPnPsdk_SOURCE_FILES}
    )
    set_target_properties(UPnPsdk_shared PROPERTIES
            OUTPUT_NAME UPnPsdk
    )
    target_include_directories(UPnPsdk_shared
        ${UPnPsdk_INCLUDE_DIRECTORIES}
    )
    target_compile_definitions(UPnPsdk_shared
        PRIVATE
            UPnPsdk_EXPORTS
            $<$<BOOL:${UPnPsdk_DEF_IXML}>:IXML_HAVE_SCRIPTSUPPORT>
    )
    target_link_libraries(UPnPsdk_shared
        PUBLIC umock_shared
    )

    # WIN32 UPnPsdk STatic library: suffix -ust
    #------------------------------------------
    add_library(UPnPsdk_static STATIC
            ${UPnPsdk_SOURCE_FILES}
    )
    set_target_properties(UPnPsdk_static PROPERTIES
            OUTPUT_NAME UPnPsdk-static
    )
    target_include_directories(UPnPsdk_static
        ${UPnPsdk_INCLUDE_DIRECTORIES}
    )
    target_compile_definitions(UPnPsdk_static
        PRIVATE
            $<$<BOOL:${UPnPsdk_DEF_IXML}>:IXML_HAVE_SCRIPTSUPPORT>
    )
    target_link_libraries(UPnPsdk_static
        PUBLIC umock_static
    )

    install(TARGETS UPnPsdk_shared UPnPsdk_static)


endif() # WIN32
