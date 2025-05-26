# Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2025-05-26

cmake_minimum_required(VERSION 3.23) # for FILE_SET
include(UPnPsdk-ProjectHeader)

project(UPnPsdk VERSION ${CMAKE_PROJECT_VERSION}
        DESCRIPTION "UPnP library program"
        HOMEPAGE_URL "https://github.com/UPnPsdk")

message(STATUS "UPnPsdk_VERSION set to \"${UPnPsdk_VERSION}\"")

set(UPnPsdk_SOURCE_FILES
    src/global.cpp
    src/api/upnptools.cpp
    src/net/sockaddr.cpp
    src/net/addrinfo.cpp
    src/net/socket.cpp
    src/net/connection_common.cpp
    src/net/upnp_device.cpp
    src/net/netadapter_if.cpp
    src/net/netadapter.cpp
    $<$<CXX_COMPILER_ID:MSVC>:src/net/netadapter_win32.cpp>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:src/net/netadapter_unix.cpp>
    src/net/http/webserver.cpp
    src/net/http/httpreadwrite.cpp
    src/net/pthread.cpp

    # Umock source files
    ${UMOCK_SOURCE_DIR}/src/arpa_inet.cpp
    $<$<NOT:$<PLATFORM_ID:Windows>>:${UMOCK_SOURCE_DIR}/src/ifaddrs.cpp>
    $<$<PLATFORM_ID:Windows>:${UMOCK_SOURCE_DIR}/src/iphlpapi.cpp>
    $<$<NOT:$<PLATFORM_ID:Windows>>:${UMOCK_SOURCE_DIR}/src/net_if.cpp>
    ${UMOCK_SOURCE_DIR}/src/netdb.cpp
    ${UMOCK_SOURCE_DIR}/src/pthread.cpp
    $<$<BOOL:${UPnPsdk_WITH_OPENSSL}>:${UMOCK_SOURCE_DIR}/src/ssl.cpp>
    ${UMOCK_SOURCE_DIR}/src/stdio.cpp
    ${UMOCK_SOURCE_DIR}/src/stdlib.cpp
    ${UMOCK_SOURCE_DIR}/src/stringh.cpp
    ${UMOCK_SOURCE_DIR}/src/sys_socket.cpp
    ${UMOCK_SOURCE_DIR}/src/sysinfo.cpp
    ${UMOCK_SOURCE_DIR}/src/unistd.cpp
    $<$<PLATFORM_ID:Windows>:${UMOCK_SOURCE_DIR}/src/winsock2.cpp>
)

set(UPnPsdk_INCLUDE_DIRECTORIES
    PUBLIC include
    PUBLIC $<$<CXX_COMPILER_ID:MSVC>:${UPnPsdk_SOURCE_DIR}/include/win32>
    PUBLIC $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:${UPnPsdk_SOURCE_DIR}/include/unix>
    PUBLIC ${UMOCK_SOURCE_DIR}/include
    PUBLIC ${CMAKE_BINARY_DIR}/include
    # if pthreads4w isn't installed this path is empty. The linker doesn't
    # find "pthread.h" and falls back to look at <pthread.h>.
    PUBLIC ${pthreads4w_SOURCE_DIR} # needed for pthread.h
)


# UPnPsdk libraries
#==================
# Have an object libray available for alternative testing.
if(FALSE)
add_library(UPnPsdk_object OBJECT
    ${UPnPsdk_SOURCE_FILES}
)
target_include_directories(UPnPsdk_object
    ${UPnPsdk_INCLUDE_DIRECTORIES}
)
target_compile_definitions(UPnPsdk_object
    PUBLIC UPnPsdk_SHARE
    PRIVATE UPnPsdk_EXPORTS
    PUBLIC $<$<BOOL:${UPnPsdk_WITH_OPENSSL}>:UPnPsdk_HAVE_OPENSSL>
)
target_link_libraries(UPnPsdk_object
    PUBLIC $<$<BOOL:${UPnPsdk_WITH_OPENSSL}>:OpenSSL::SSL>
)
endif()

# UPnPsdk SHared library: suffix -ush
#------------------------------------
add_library(UPnPsdk_shared SHARED
    ${UPnPsdk_SOURCE_FILES}
)
set_target_properties(UPnPsdk_shared PROPERTIES
    OUTPUT_NAME upnpsdk
    VERSION ${UPnPsdk_VERSION}
    SOVERSION ${UPnPsdk_VERSION_MAJOR}
)
target_include_directories(UPnPsdk_shared
    ${UPnPsdk_INCLUDE_DIRECTORIES}
)
target_compile_definitions(UPnPsdk_shared
    PUBLIC UPnPsdk_SHARE
    PRIVATE UPnPsdk_EXPORTS
    PUBLIC $<$<BOOL:${UPnPsdk_WITH_OPENSSL}>:UPnPsdk_HAVE_OPENSSL>
)
target_link_libraries(UPnPsdk_shared
    PUBLIC $<$<BOOL:${UPnPsdk_WITH_OPENSSL}>:OpenSSL::SSL>
    # In addition to pthreads we need some more system libraries on WIN32
    PUBLIC $<$<PLATFORM_ID:Windows>:ws2_32>   # winsock to support sockets
    PUBLIC $<$<PLATFORM_ID:Windows>:iphlpapi> # ip helper interface to get network
                                              # management tools
    # On MS Windows we link with the static pthreads4w library to avoid error
    # prone managing access to its .dll file.
    # On Unix platforms we cannot link with the static pthreads library because
    # then we have also to link with a static libc that's not available by
    # default. Pthreads on Unix are system wide available so it also makes
    # sense to use shared libs.
    PUBLIC $<IF:$<PLATFORM_ID:Windows>,${UPNP_PTHREADS_STATIC_LIBRARY},${UPNP_PTHREADS_SHARED_LIBRARY}>
    # On WIN32 disable warning LNK4217: symbol 'symbol' defined in
    # 'filename_1.obj' is imported by 'filename_2.obj' in function
    # 'function'.
    # Importing and defining locally the same symbol.
    # E.g. this is why a source is included into a library and a test
    # includes the source and also uses the library. The solution to avoid
    # the warning presented by Microsoft is to create an additional
    # library. But that isn't acceptable here because the additional
    # library must also given to the user. So I suppress this uncritical
    # warning because I know what I do. --Ingo
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:-ignore:4217>
)


# UPnPsdk STatic library: suffix -ust
#------------------------------------
add_library(UPnPsdk_static STATIC
    ${UPnPsdk_SOURCE_FILES}
)
set_target_properties(UPnPsdk_static PROPERTIES
    OUTPUT_NAME $<IF:$<PLATFORM_ID:Windows>,upnpsdk-static,upnpsdk>
)
target_include_directories(UPnPsdk_static
    ${UPnPsdk_INCLUDE_DIRECTORIES}
)
target_compile_definitions(UPnPsdk_static
    PUBLIC $<$<BOOL:${UPnPsdk_WITH_OPENSSL}>:UPnPsdk_HAVE_OPENSSL>
)
target_link_libraries(UPnPsdk_static
    PUBLIC $<$<BOOL:${UPnPsdk_WITH_OPENSSL}>:OpenSSL::SSL>
    # In addition to pthreads we need some more system libraries on WIN32
    PUBLIC $<$<PLATFORM_ID:Windows>:ws2_32>   # winsock to support sockets
    PUBLIC $<$<PLATFORM_ID:Windows>:iphlpapi> # ip helper interface to get network
                                              # management tools
    # On MS Windows we link with the static pthreads4w library to avoid error
    # prone managing access to its .dll file.
    # On Unix platforms we cannot link with the static pthreads library because
    # then we have also to link with a static libc that's not available by
    # default. Pthreads on Unix are system wide available so it also makes
    # sense to use shared libs.
    PUBLIC $<IF:$<PLATFORM_ID:Windows>,${UPNP_PTHREADS_STATIC_LIBRARY},${UPNP_PTHREADS_SHARED_LIBRARY}>
)


# Installation
# ============
set(UPnPsdk_PUBLIC_HEADERS
    include/UPnPsdk/port_sock.hpp
    include/UPnPsdk/visibility.hpp
)

if(NOT WIN32)

    target_sources(UPnPsdk_shared
        INTERFACE FILE_SET upnpsdk_public_headers TYPE HEADERS
                  BASE_DIRS include
                  FILES ${UPnPsdk_PUBLIC_HEADERS}
    )
    install(TARGETS UPnPsdk_shared
        FILE_SET upnpsdk_public_headers
            DESTINATION include
            COMPONENT Development
            EXCLUDE_FROM_ALL
    )

else()
    install(TARGETS UPnPsdk_shared)
endif()

    install(TARGETS UPnPsdk_static)
