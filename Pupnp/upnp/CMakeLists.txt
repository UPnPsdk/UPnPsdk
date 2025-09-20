# Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2025-09-23

cmake_minimum_required(VERSION 3.23) # for FILE_SET
include(UPnPsdk-ProjectHeader)

project(PUPNP_UPNP VERSION "17.2.0"
        DESCRIPTION "Binary build of the UPnP library core"
        HOMEPAGE_URL "https://github.com/UPnPsdk")

message(STATUS "PUPNP_UPNP_VERSION set to \"${PUPNP_UPNP_VERSION}\"")

# This contains all settings to compile the library dynamically and static.
#==========================================================================

# Switches for this Subproject
#-----------------------------
# These switches are all handled only in this configuration file.

# Options selectable by user configuration
set(UPNP_HAVE_GOOGLETEST ${UPnPsdk_WITH_GOOGLETEST})
set(UPNP_HAVE_TOOLS YES) #${UPnPsdk_WITH_TOOLS})         # tools -> UPNP_ENABLE_HELPER_API_TOOLS
set(UPNP_HAVE_CLIENT YES) #${UPnPsdk_WITH_CLIENT})       # client -> UPNP_ENABLE_CLIENT_API
set(UPNP_HAVE_DEVICE YES) #${UPnPsdk_WITH_DEVICE})
set(UPNP_HAVE_WEBSERVER YES) #${UPnPsdk_WITH_WEBSERVER}) # webserver -> UPNP_ENABLE_WEBSERVER
                                                     #   only enabled with UPNP_HAVE_DEVICE
set(UPNP_HAVE_OPTSSDP YES) #${UPnPsdk_WITH_OPTSSDP})     # optssdp -> UPNP_ENABLE_OPTIONAL_SSDP_HEADERS
set(UPNP_HAVE_SOAP YES) #${UPnPsdk_WITH_SOAP})           # soap -> UPNP_ENABLE_SOAP
                                                     #   only enabled with UPNP_HAVE_WEBSERVER
set(UPNP_HAVE_GENA YES) #${UPnPsdk_WITH_GENA})           # gena -> UPNP_ENABLE_GENA
                                                     #   compiles only with UPNP_HAVE_CLIENT and UPNP_HAVE_DEVICE
set(UPNP_ENABLE_OPEN_SSL ${UPnPsdk_WITH_OPENSSL})     # open_ssl -> UPNP_ENABLE_OPEN_SSL
set(UPNP_ENABLE_BLOCKING_TCP_CONNECTIONS NO) #${UPnPsdk_WITH_BLOCKTCP})   # blocking_tcp_connections -> UPNP_ENABLE_BLOCKING_TCP_CONNECTIONS
set(UPNP_ENABLE_IPV6 YES)                           # Always available
#set(PUPNP_DEF_IXML YES) #${UPnPsdk_WITH_IXML_SCRIPT})   # scriptsupport -> IXML_HAVE_SCRIPTSUPPORT
#set(PUPNP_DEF_UNSPECSVR NO)                        # unspecified_server -> UPNP_ENABLE_UNSPECIFIED_SERVER
#set(PUPNP_DEF_REUSEADDR NO)                        # reuseaddr -> UPNP_MINISERVER_REUSEADDR
if(UPnPsdk_WITH_SAMPLES)                             # all samples -> UPNP_BUILD_SAMPLES
    set(PUPNP_HAVE_SAMPLES YES) # To make it separate selectable don't set it direct
endif()

# Dependencies of options
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(UPNP_HAVE_DEBUG YES)
endif()

if(UPNP_HAVE_GOOGLETEST)
    set(UPNP_HAVE_CLIENT YES)
    set(UPNP_HAVE_OPTSSDP YES)
    set(UPNP_HAVE_GENA YES)
endif()

if(PUPNP_HAVE_SAMPLES)
    set(UPNP_HAVE_TOOLS YES)
    set(UPNP_HAVE_CLIENT YES)
    set(UPNP_HAVE_GENA YES)
endif()

if(UPNP_HAVE_GENA)
    set(UPNP_HAVE_WEBSERVER YES)
    set(UPNP_HAVE_CLIENT YES)
endif()

if(UPNP_HAVE_CLIENT)
    set(UPNP_HAVE_SSDP YES)
endif()

if(UPNP_HAVE_DEVICE)
    set(UPNP_HAVE_WEBSERVER YES)
    set(UPNP_HAVE_SSDP YES)
endif()

if(UPNP_HAVE_WEBSERVER)
    set(UPNP_HAVE_DEVICE YES)
    set(UPNP_HAVE_SOAP YES)
endif()

if(UPNP_HAVE_SOAP)
    set(UPNP_HAVE_SSDP YES)
endif()


# Source files and compile settings
#----------------------------------

set(PUPNP_SOURCE_FILES
    src/threadutil/FreeList.cpp
    src/threadutil/LinkedList.cpp
    src/threadutil/ThreadPool.cpp
    src/threadutil/TimerThread.cpp

    src/genlib/net/sock.cpp
    src/genlib/net/uri/uri.cpp

    src/genlib/net/http/httpparser.cpp
    src/genlib/net/http/httpreadwrite.cpp
    src/genlib/net/http/statcodes.cpp
    src/genlib/net/http/webserver.cpp
    src/genlib/net/http/parsetools.cpp

    src/genlib/util/list.cpp
    src/genlib/util/membuffer.cpp
    src/genlib/util/strintmap.cpp
    $<$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_SOAP}>>:src/genlib/util/util.cpp>

    $<$<BOOL:${UPNP_HAVE_DEVICE}>:src/genlib/service-table/service_table.cpp>
    $<$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_GENA}>>:src/genlib/client_table/GenlibClientSubscription.cpp>
    $<$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_GENA}>>:src/genlib/client_table/client_table.cpp>
    src/genlib/miniserver/miniserver.cpp

    $<$<BOOL:${UPNP_HAVE_SSDP}>:src/ssdp/ssdp_server.cpp>
    $<$<BOOL:${UPNP_HAVE_DEVICE}>:src/ssdp/ssdp_device.cpp>
    $<$<BOOL:${UPNP_HAVE_CLIENT}>:src/ssdp/ssdp_ctrlpt.cpp>
    $<$<BOOL:${UPNP_HAVE_CLIENT}>:src/ssdp/SSDPResultData.cpp>
    $<$<BOOL:${UPNP_HAVE_CLIENT}>:src/ssdp/SSDPResultDataCallback.cpp>

    $<$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_SOAP}>>:src/soap/soap_device.cpp>
    $<$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_SOAP}>>:src/soap/soap_common.cpp>
    $<$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_SOAP}>>:src/soap/soap_ctrlpt.cpp>

    $<$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_GENA}>>:src/gena/gena_device.cpp>
    $<$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_GENA}>>:src/gena/gena_callback2.cpp>
    $<$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_GENA}>>:src/gena/gena_ctrlpt.cpp>

    $<$<OR:$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_GENA}>>,$<BOOL:${UPNP_HAVE_OPTSSDP}>>:src/uuid/uuid.cpp>
    $<$<OR:$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_GENA}>>,$<BOOL:${UPNP_HAVE_OPTSSDP}>>:src/uuid/md5.cpp>
    $<$<OR:$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_GENA}>>,$<BOOL:${UPNP_HAVE_OPTSSDP}>>:src/uuid/sysdep.cpp>

    src/api/UpnpExtraHeaders.cpp
    src/api/UpnpString.cpp
    src/api/upnpapi.cpp
    $<$<OR:$<BOOL:${UPNP_HAVE_TOOLS}>,$<BOOL:${UPNP_HAVE_DEBUG}>>:src/api/upnptools.cpp>
    $<$<BOOL:${UPNP_HAVE_WEBSERVER}>:src/api/UpnpFileInfo.cpp>
    $<$<BOOL:${UPNP_HAVE_CLIENT}>:src/api/UpnpDiscovery.cpp>
    #$<$<OR:$<BOOL:${UPNP_HAVE_CLIENT}>,$<BOOL:${UPNP_HAVE_DEBUG}>>:src/api/UpnpDiscovery.cpp>

    $<$<OR:$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_SOAP}>>,$<BOOL:${UPNP_HAVE_DEBUG}>>:src/api/UpnpActionRequest.cpp>
    $<$<OR:$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_SOAP}>>,$<BOOL:${UPNP_HAVE_DEBUG}>>:src/api/UpnpActionComplete.cpp>
    $<$<OR:$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_SOAP}>>,$<BOOL:${UPNP_HAVE_DEBUG}>>:src/api/UpnpStateVarRequest.cpp>
    $<$<OR:$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_SOAP}>>,$<BOOL:${UPNP_HAVE_DEBUG}>>:src/api/UpnpStateVarComplete.cpp>
    $<$<OR:$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_SOAP}>>,$<BOOL:${UPNP_HAVE_DEBUG}>>:src/api/UpnpSubscriptionRequest.cpp>
    $<$<OR:$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_SOAP}>>,$<BOOL:${UPNP_HAVE_DEBUG}>>:src/api/UpnpEvent.cpp>
    $<$<OR:$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_SOAP}>>,$<BOOL:${UPNP_HAVE_DEBUG}>>:src/api/UpnpEventSubscribe.cpp>
    $<$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_SOAP}>>:src/api/UpnpStateVarRequest.cpp>
    $<$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_SOAP}>>:src/api/UpnpActionComplete.cpp>
    $<$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_SOAP}>>:src/api/UpnpStateVarComplete.cpp>
    $<$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_GENA}>>:src/api/UpnpSubscriptionRequest.cpp>
    $<$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_GENA}>>:src/api/UpnpEventSubscribe.cpp>
    $<$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_GENA}>>:src/api/UpnpEvent.cpp>
    src/api/upnpdebug.cpp

    $<$<AND:$<BOOL:${UPNP_HAVE_DEVICE}>,$<BOOL:${UPNP_HAVE_WEBSERVER}>>:src/urlconfig/urlconfig.cpp>

    # ixml
    # -------------------------------------------------------------------------
    ${PUPNP_IXML_SOURCE_DIR}/src/attr.cpp
    ${PUPNP_IXML_SOURCE_DIR}/src/document.cpp
    ${PUPNP_IXML_SOURCE_DIR}/src/element.cpp
    ${PUPNP_IXML_SOURCE_DIR}/src/ixml.cpp
    $<$<CONFIG:Debug>:${PUPNP_IXML_SOURCE_DIR}/src/ixmldebug.cpp>
    ${PUPNP_IXML_SOURCE_DIR}/src/ixmlmembuf.cpp
    ${PUPNP_IXML_SOURCE_DIR}/src/ixmlparser.cpp
    ${PUPNP_IXML_SOURCE_DIR}/src/namedNodeMap.cpp
    ${PUPNP_IXML_SOURCE_DIR}/src/node.cpp
    ${PUPNP_IXML_SOURCE_DIR}/src/nodeList.cpp

    # Umock files
    # -------------------------------------------------------------------------
    $<$<NOT:$<PLATFORM_ID:Windows>>:${UMOCK_SOURCE_DIR}/src/ifaddrs.cpp>
    $<$<NOT:$<PLATFORM_ID:Windows>>:${UMOCK_SOURCE_DIR}/src/net_if.cpp>
    $<$<PLATFORM_ID:Windows>:${UMOCK_SOURCE_DIR}/src/iphlpapi.cpp>
    $<$<PLATFORM_ID:Windows>:${UMOCK_SOURCE_DIR}/src/winsock2.cpp>
    ${UMOCK_SOURCE_DIR}/src/arpa_inet.cpp
    ${UMOCK_SOURCE_DIR}/src/netdb.cpp
    ${UMOCK_SOURCE_DIR}/src/pthread.cpp
    ${UMOCK_SOURCE_DIR}/src/pupnp_sock.cpp
    $<$<BOOL:${UPNP_ENABLE_OPEN_SSL}>:${UMOCK_SOURCE_DIR}/src/ssl.cpp>
    ${UMOCK_SOURCE_DIR}/src/stdio.cpp
    ${UMOCK_SOURCE_DIR}/src/stdlib.cpp
    ${UMOCK_SOURCE_DIR}/src/stringh.cpp
    ${UMOCK_SOURCE_DIR}/src/sys_socket.cpp
    ${UMOCK_SOURCE_DIR}/src/sysinfo.cpp
    ${UMOCK_SOURCE_DIR}/src/unistd.cpp

    # Helper added but not original old pupnp code.
    # ---------------------------------------------
    ${CMAKE_SOURCE_DIR}/UPnPsdk/src/global.cpp # For general g_dbug flag
)

set(PUPNP_INCLUDE_DIRECTORIES
    PUBLIC inc
    PUBLIC src/inc
    PUBLIC src/threadutil
    PUBLIC ${CMAKE_BINARY_DIR}/include
    PUBLIC ${CMAKE_SOURCE_DIR}/UPnPsdk/include
    PUBLIC ${UMOCK_SOURCE_DIR}/include
    PUBLIC ${PUPNP_IXML_SOURCE_DIR}/inc
    PRIVATE ${PUPNP_IXML_SOURCE_DIR}/src/inc
    # if pthreads4w isn't installed this path is empty. The linker doesn't
    # find "pthread.h" and falls back to look at <pthread.h>.
    PUBLIC ${pthreads4w_SOURCE_DIR} # needed for pthread.h
)

set(PUPNP_COMPILE_DEFINITIONS
    PUBLIC
        UPnPsdk_WITH_NATIVE_PUPNP
        $<$<BOOL:${UPNP_HAVE_DEBUG}>:UPNP_HAVE_DEBUG>
        $<$<BOOL:${UPNP_ENABLE_IPV6}>:UPNP_ENABLE_IPV6>
        $<$<BOOL:${UPNP_ENABLE_OPEN_SSL}>:UPNP_ENABLE_OPEN_SSL>
        #$<$<BOOL:${PUPNP_DEF_IXML}>:IXML_HAVE_SCRIPTSUPPORT>
        $<$<BOOL:${UPNP_HAVE_TOOLS}>:UPNP_HAVE_TOOLS>
        $<$<BOOL:${UPNP_HAVE_WEBSERVER}>:UPNP_HAVE_WEBSERVER>
        $<$<BOOL:${UPNP_HAVE_SSDP}>:UPNP_HAVE_SSDP>
        $<$<BOOL:${UPNP_HAVE_OPTSSDP}>:UPNP_HAVE_OPTSSDP>
        $<$<BOOL:${UPNP_HAVE_SOAP}>:UPNP_HAVE_SOAP>
        $<$<BOOL:${UPNP_HAVE_GENA}>:UPNP_HAVE_GENA>
        $<$<BOOL:${UPNP_HAVE_CLIENT}>:UPNP_HAVE_CLIENT>
        $<$<BOOL:${UPNP_HAVE_DEVICE}>:UPNP_HAVE_DEVICE>
        $<$<BOOL:${UPNP_ENABLE_BLOCKING_TCP_CONNECTIONS}>:UPNP_ENABLE_BLOCKING_TCP_CONNECTIONS>
)
configure_file (${PUPNP_UPNP_SOURCE_DIR}/inc/upnpconfig.h.cm ${CMAKE_BINARY_DIR}/include/upnpconfig.h)


# Create libraries
#=================
# Link with POSIX Threads for Windows (pthreads4w):
# Pthreads are not supported by MS Windows. So I have installed the
# third party library pthreads4w.

# Pupnp SHared library: -psh
#---------------------------
add_library(pupnp_shared SHARED
    ${PUPNP_SOURCE_FILES}
)
set_target_properties(pupnp_shared PROPERTIES
    OUTPUT_NAME upnpsdk-pupnp
    VERSION ${PUPNP_UPNP_VERSION}
    SOVERSION ${PUPNP_UPNP_VERSION_MAJOR}
)
target_include_directories(pupnp_shared
    ${PUPNP_INCLUDE_DIRECTORIES}
)
target_compile_options(pupnp_shared
    # disable warning: 'sprintf' (and friends) is deprecated: This function is
    # provided for compatibility reasons only.
    # I do not fix deprecated functions on old code.
    PUBLIC $<$<CXX_COMPILER_ID:AppleClang>:-Wno-deprecated-declarations>
)
target_compile_definitions(pupnp_shared
    ${PUPNP_COMPILE_DEFINITIONS}
    PUBLIC UPnPsdk_SHARE
    PRIVATE UPnPsdk_EXPORTS
)
target_link_libraries(pupnp_shared
    PUBLIC $<$<BOOL:${UPNP_ENABLE_OPEN_SSL}>:OpenSSL::SSL>
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


# Pupnp STatic library: suffix -pst
#----------------------------------
add_library(pupnp_static STATIC
    ${PUPNP_SOURCE_FILES}
)
set_target_properties(pupnp_static PROPERTIES
    OUTPUT_NAME $<IF:$<PLATFORM_ID:Windows>,upnpsdk-pupnp-static,upnpsdk-pupnp>
)
target_include_directories(pupnp_static
    ${PUPNP_INCLUDE_DIRECTORIES}
)
target_compile_options(pupnp_static
    # disable warning: 'sprintf' (and friends) is deprecated: This function is
    # provided for compatibility reasons only.
    # I do not fix deprecated functions on old code.
    PUBLIC $<$<CXX_COMPILER_ID:AppleClang>:-Wno-deprecated-declarations>
)
target_compile_definitions(pupnp_static
    ${PUPNP_COMPILE_DEFINITIONS}
)
target_link_libraries(pupnp_static
    PUBLIC $<$<BOOL:${UPNP_ENABLE_OPEN_SSL}>:OpenSSL::SSL>
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
install(TARGETS pupnp_shared pupnp_static)

# vim: nowrap
