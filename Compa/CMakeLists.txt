# Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2025-05-26

cmake_minimum_required(VERSION 3.23) # for FILE_SET
include(UPnPsdk-ProjectHeader)

project(COMPA VERSION ${CMAKE_PROJECT_VERSION}
              DESCRIPTION "UPnPsdk compatible library subproject"
              HOMEPAGE_URL "https://github.com/UPnPsdk")

message(STATUS "COMPA_VERSION set to \"${COMPA_VERSION}\"")

# Switches for this Subproject
#-----------------------------
# These switches are all handled only in this configuration file.

# Options selectable by user configuration
set(COMPA_DEF_OPENSSL ${UPnPsdk_WITH_OPENSSL})
set(COMPA_DEF_MINISERVER ${UPnPsdk_WITH_MINISERVER})
set(COMPA_DEF_WEBSERVER ${UPnPsdk_WITH_WEBSERVER})
set(COMPA_DEF_CTRLPT_SSDP ${UPnPsdk_WITH_CTRLPT_SSDP})  # client -> UPNP_ENABLE_CLIENT_API
set(COMPA_DEF_DEVICE_SSDP ${UPnPsdk_WITH_DEVICE_SSDP})
set(COMPA_DEF_OPTION_SSDP ${UPnPsdk_WITH_OPTION_SSDP}) # optssdp -> UPNP_ENABLE_OPTIONAL_SSDP_HEADERS
set(COMPA_DEF_CTRLPT_DESCRIPTION ${UPnPsdk_WITH_CTRLPT_DESCRIPTION})
set(COMPA_DEF_DEVICE_DESCRIPTION ${UPnPsdk_WITH_DEVICE_DESCRIPTION})
set(COMPA_DEF_CTRLPT_SOAP ${UPnPsdk_WITH_CTRLPT_SOAP})
set(COMPA_DEF_DEVICE_SOAP ${UPnPsdk_WITH_DEVICE_SOAP})
set(COMPA_DEF_CTRLPT_GENA ${UPnPsdk_WITH_CTRLPT_GENA})
set(COMPA_DEF_DEVICE_GENA ${UPnPsdk_WITH_DEVICE_GENA})
set(COMPA_DEF_TOOLS ${UPnPsdk_WITH_TOOLS})         # tools -> UPNP_ENABLE_HELPER_API_TOOLS
set(COMPA_DEF_GOOGLETEST ${UPnPsdk_WITH_GOOGLETEST})
#set(COMPA_DEF_BLOCKTCP ${UPnPsdk_WITH_BLOCKTCP})  # blocking_tcp_connections -> COMPA_ENABLE_BLOCKING_TCP_CONNECTIONS
#set(COMPA_DEF_UNSPECSVR ${UPnPsdk_WITH_UNSPECIFIED_SERVER}) # unspecified_server -> UPNP_ENABLE_UNSPECIFIED_SERVER
#set(COMPA_DEF_REUSEADDR NO)                       # reuseaddr -> UPNP_MINISERVER_REUSEADDR
if(UPnPsdk_WITH_SAMPLES)                           # all samples -> UPNP_BUILD_SAMPLES
    set(COMPA_DEF_SAMPLES YES) # To make it separate selectable don't set it direct
endif()


# Dependencies of options. The order is important.
#-------------------------------------------------
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(COMPA_DEF_DEBUG YES)
    set(COMPA_DEF_TOOLS YES)
endif()

if(COMPA_DEF_GOOGLETEST)
    set(COMPA_DEF_CTRLPT_GENA YES)
    set(COMPA_DEF_DEVICE_GENA YES)
    set(COMPA_DEF_OPTION_SSDP YES)
endif()

if(COMPA_DEF_SAMPLES)
    set(COMPA_DEF_CTRLPT_GENA YES)
    set(COMPA_DEF_DEVICE_GENA YES)
    set(COMPA_DEF_TOOLS YES)
endif()

if(COMPA_DEF_CTRLPT_GENA)
    set(COMPA_DEF_CTRLPT_SOAP YES)
endif()

if(COMPA_DEF_DEVICE_GENA)
    set(COMPA_DEF_DEVICE_SOAP YES)
endif()

if(COMPA_DEF_CTRLPT_SOAP)
    set(COMPA_DEF_CTRLPT_DESCRIPTION YES)
endif()

if(COMPA_DEF_DEVICE_SOAP)
    set(COMPA_DEF_DEVICE_DESCRIPTION YES)
endif()

if(COMPA_DEF_CTRLPT_DESCRIPTION)
    set(COMPA_DEF_CTRLPT_SSDP YES)
endif()

if(COMPA_DEF_DEVICE_DESCRIPTION)
    set(COMPA_DEF_DEVICE_SSDP YES)
endif()

if(COMPA_DEF_CTRLPT_SSDP OR COMPA_DEF_DEVICE_SSDP)
    set(COMPA_DEF_WEBSERVER YES)
endif()

if(COMPA_DEF_WEBSERVER)
    set(COMPA_DEF_MINISERVER YES)
endif()


# Source files and compile settings
# =================================

set(COMPA_SOURCE_FILES
    src/genlib/util/membuffer.cpp
    src/genlib/util/list.cpp
    src/api/UpnpString.cpp
    src/genlib/net/sock.cpp
    src/api/upnpapi.cpp

    src/threadutil/FreeList.cpp # Only for LinkedList, ThreadPool, TimerThread
    src/threadutil/LinkedList.cpp
    src/threadutil/ThreadPool.cpp
    src/threadutil/TimerThread.cpp

    # Miniserver
    $<$<BOOL:${COMPA_DEF_MINISERVER}>:src/genlib/miniserver/miniserver.cpp>

    # Webserver
    $<$<BOOL:${COMPA_DEF_WEBSERVER}>:src/genlib/net/http/webserver.cpp>
    $<$<BOOL:${COMPA_DEF_WEBSERVER}>:src/genlib/net/http/httpparser.cpp>
    $<$<BOOL:${COMPA_DEF_WEBSERVER}>:src/genlib/net/http/httpreadwrite.cpp>
    $<$<BOOL:${COMPA_DEF_WEBSERVER}>:src/genlib/net/http/statcodes.cpp>
    $<$<BOOL:${COMPA_DEF_WEBSERVER}>:src/genlib/net/http/parsetools.cpp>
    $<$<BOOL:${COMPA_DEF_WEBSERVER}>:src/api/UpnpExtraHeaders.cpp>
    $<$<BOOL:${COMPA_DEF_WEBSERVER}>:src/api/UpnpFileInfo.cpp>
    $<$<OR:$<BOOL:${COMPA_DEF_WEBSERVER}>,$<BOOL:${COMPA_DEF_TOOLS}>>:src/genlib/net/uri/uri.cpp>

    # Common
    # -------------------------------------------------------------------------
    # SSDP
    $<$<OR:$<BOOL:${COMPA_DEF_CTRLPT_SSDP}>,$<BOOL:${COMPA_DEF_DEVICE_SSDP}>>:src/ssdp/ssdp_common.cpp>
    # GENA
    $<$<OR:$<BOOL:${COMPA_DEF_CTRLPT_GENA}>,$<BOOL:${COMPA_DEF_DEVICE_GENA}>>:src/gena/gena_callback2.cpp>
    $<$<OR:$<BOOL:${COMPA_DEF_CTRLPT_GENA}>,$<BOOL:${COMPA_DEF_DEVICE_GENA}>,$<BOOL:${COMPA_DEF_OPTION_SSDP}>>:src/uuid/md5.cpp> # needed only by uuid.cpp and sysdep.cpp
    $<$<OR:$<BOOL:${COMPA_DEF_CTRLPT_GENA}>,$<BOOL:${COMPA_DEF_DEVICE_GENA}>,$<BOOL:${COMPA_DEF_OPTION_SSDP}>>:src/uuid/sysdep.cpp> # needed only by uuid.cpp
    $<$<OR:$<BOOL:${COMPA_DEF_CTRLPT_GENA}>,$<BOOL:${COMPA_DEF_DEVICE_GENA}>,$<BOOL:${COMPA_DEF_OPTION_SSDP}>>:src/uuid/uuid.cpp>

    #Control Point
    # -------------------------------------------------------------------------
    # SSDP
    $<$<BOOL:${COMPA_DEF_CTRLPT_SSDP}>:src/ssdp/ssdp_ctrlpt.cpp>
    $<$<BOOL:${COMPA_DEF_CTRLPT_SSDP}>:src/ssdp/SSDPResultData.cpp>
    $<$<BOOL:${COMPA_DEF_CTRLPT_SSDP}>:src/ssdp/SSDPResultDataCallback.cpp>
    $<$<BOOL:${COMPA_DEF_CTRLPT_SSDP}>:src/api/UpnpDiscovery.cpp>
    # SOAP
    $<$<BOOL:${COMPA_DEF_CTRLPT_SOAP}>:src/soap/soap_ctrlpt.cpp>
    $<$<BOOL:${COMPA_DEF_CTRLPT_SOAP}>:src/api/UpnpActionComplete.cpp>
    $<$<BOOL:${COMPA_DEF_CTRLPT_SOAP}>:src/api/UpnpStateVarComplete.cpp>
    # GENA
    $<$<BOOL:${COMPA_DEF_CTRLPT_GENA}>:src/gena/gena_ctrlpt.cpp>
    $<$<BOOL:${COMPA_DEF_CTRLPT_GENA}>:src/genlib/client_table/GenlibClientSubscription.cpp>
    $<$<BOOL:${COMPA_DEF_CTRLPT_GENA}>:src/genlib/client_table/client_table.cpp>
    $<$<BOOL:${COMPA_DEF_CTRLPT_GENA}>:src/api/UpnpEventSubscribe.cpp>
    $<$<BOOL:${COMPA_DEF_CTRLPT_GENA}>:src/api/UpnpEvent.cpp>

    # UPnP Device
    # -------------------------------------------------------------------------
    # SSDP
    $<$<BOOL:${COMPA_DEF_DEVICE_SSDP}>:src/ssdp/ssdp_device.cpp>
    $<$<BOOL:${COMPA_DEF_DEVICE_SSDP}>:src/genlib/service-table/service_table.cpp>
    # Description
    $<$<BOOL:${COMPA_DEF_DEVICE_DESCRIPTION}>:src/urlconfig/urlconfig.cpp>
    # SOAP
    $<$<BOOL:${COMPA_DEF_DEVICE_SOAP}>:src/soap/soap_device.cpp>
    $<$<BOOL:${COMPA_DEF_DEVICE_SOAP}>:src/api/UpnpActionRequest.cpp>
    $<$<BOOL:${COMPA_DEF_DEVICE_SOAP}>:src/api/UpnpStateVarRequest.cpp>
    # GENA
    $<$<BOOL:${COMPA_DEF_DEVICE_GENA}>:src/gena/gena_device.cpp>
    $<$<BOOL:${COMPA_DEF_DEVICE_GENA}>:src/api/UpnpSubscriptionRequest.cpp>

    # ixml
    # -------------------------------------------------------------------------
    ${PUPNP_SOURCE_DIR}/ixml/src/attr.cpp
    ${PUPNP_SOURCE_DIR}/ixml/src/document.cpp
    ${PUPNP_SOURCE_DIR}/ixml/src/element.cpp
    ${PUPNP_SOURCE_DIR}/ixml/src/ixml.cpp
    $<$<CONFIG:Debug>:${PUPNP_SOURCE_DIR}/ixml/src/ixmldebug.cpp>
    ${PUPNP_SOURCE_DIR}/ixml/src/ixmlmembuf.cpp
    ${PUPNP_SOURCE_DIR}/ixml/src/ixmlparser.cpp
    ${PUPNP_SOURCE_DIR}/ixml/src/namedNodeMap.cpp
    ${PUPNP_SOURCE_DIR}/ixml/src/node.cpp
    ${PUPNP_SOURCE_DIR}/ixml/src/nodeList.cpp

    # Other sources (need rework)
    # -------------------------------------------------------------------------
    $<$<BOOL:${COMPA_DEF_TOOLS}>:src/api/upnptools.cpp>
    src/api/upnpdebug.cpp

    ${UMOCK_SOURCE_DIR}/src/pupnp_sock.cpp
    ${CMAKE_SOURCE_DIR}/Interface/src/pupnp-sock.cpp

    # Helper classes added here. They are assigned with the UPnPsdk rules, e.g.
    # throwing exceptions etc. They only encapsulate old functions mainly for
    # testing with init and shutdown following the RAII paradigm.
    ${PUPNP_SOURCE_DIR}/src/api/upnpdebug.cpp
    ${PUPNP_SOURCE_DIR}/src/threadpool_init.cpp
)

# Installation
set(COMPA_PUBLIC_HEADERS
    inc/API.hpp
    inc/Callback.hpp
    inc/list.hpp
    inc/upnp.h
    inc/upnp.hpp
    inc/UpnpActionComplete.hpp
    inc/UpnpActionRequest.hpp
    inc/UpnpDiscovery.hpp
    inc/UpnpEvent.hpp
    inc/UpnpEventSubscribe.hpp
    inc/UpnpExtraHeaders.hpp
    inc/UpnpFileInfo.hpp
    inc/UpnpGlobal.hpp
    #inc/UpnpInet.hpp
    inc/UpnpIntTypes.hpp
    inc/UpnpStateVarComplete.hpp
    inc/UpnpStateVarRequest.hpp
    #inc/UpnpStdInt.hpp
    inc/UpnpString.hpp
    inc/UpnpSubscriptionRequest.hpp
    inc/UpnpUniStd.hpp
    inc/upnptools.h
    inc/upnptools.hpp
    inc/upnpdebug.h
    inc/upnpdebug.hpp
    ${CMAKE_BINARY_DIR}/include/cmake_vars.hpp
)

set(COMPA_INCLUDE_DIRECTORIES
    PUBLIC inc
    PUBLIC src/inc
    PUBLIC src/threadutil
    PUBLIC ${PUPNP_SOURCE_DIR}/include
    PUBLIC ${CMAKE_SOURCE_DIR}/Interface/include

    # ixml include directories
    PUBLIC ${PUPNP_SOURCE_DIR}/ixml/inc
    PRIVATE ${PUPNP_SOURCE_DIR}/ixml/src/inc
)

set(COMPA_COMPILE_DEFINITIONS
    PUBLIC
        UPNP_ENABLE_IPV6
        COMPA_ENABLE_BLOCKING_TCP_CONNECTIONS

        $<$<BOOL:${COMPA_DEF_MINISERVER}>:COMPA_HAVE_MINISERVER>

        $<$<BOOL:${COMPA_DEF_WEBSERVER}>:COMPA_HAVE_WEBSERVER>

        $<$<BOOL:${COMPA_DEF_CTRLPT_SSDP}>:COMPA_HAVE_CTRLPT_SSDP>
        $<$<BOOL:${COMPA_DEF_DEVICE_SSDP}>:COMPA_HAVE_DEVICE_SSDP>
        $<$<BOOL:${COMPA_DEF_OPTION_SSDP}>:COMPA_HAVE_OPTION_SSDP>

        $<$<BOOL:${COMPA_DEF_CTRLPT_DESCRIPTION}>:COMPA_HAVE_CTRLPT_DESCRIPTION>
        $<$<BOOL:${COMPA_DEF_DEVICE_DESCRIPTION}>:COMPA_HAVE_DEVICE_DESCRIPTION>

        $<$<BOOL:${COMPA_DEF_CTRLPT_SOAP}>:COMPA_HAVE_CTRLPT_SOAP>
        $<$<BOOL:${COMPA_DEF_DEVICE_SOAP}>:COMPA_HAVE_DEVICE_SOAP>

        $<$<BOOL:${COMPA_DEF_CTRLPT_GENA}>:COMPA_HAVE_CTRLPT_GENA>
        $<$<BOOL:${COMPA_DEF_DEVICE_GENA}>:COMPA_HAVE_DEVICE_GENA>

        $<$<BOOL:${COMPA_DEF_TOOLS}>:UPNP_HAVE_TOOLS>
        $<$<BOOL:${COMPA_DEF_DEBUG}>:UPNP_HAVE_DEBUG>
)


set_source_files_properties(
    src/ssdp/ssdp_device.cpp
    PROPERTIES
        COMPILE_OPTIONS $<$<CXX_COMPILER_ID:AppleClang>:-Wno-shorten-64-to-32>
)


# Create libraries
#=================
# Link with POSIX Threads for Windows (pthreads4w)
# Pthreads are not supported by MS Windows. So we have installed the
# third party library pthreads4w.

# Compa SHared library: -csh
#---------------------------
add_library(compa_shared SHARED
    ${COMPA_SOURCE_FILES}
)
set_target_properties(compa_shared PROPERTIES
    OUTPUT_NAME upnpsdk-compa
    VERSION ${COMPA_VERSION}
    SOVERSION ${COMPA_VERSION_MAJOR}
)
target_include_directories(compa_shared
    ${COMPA_INCLUDE_DIRECTORIES}
)
target_compile_definitions(compa_shared
    ${COMPA_COMPILE_DEFINITIONS}
    PRIVATE UPnPsdk_EXPORTS
)
target_link_libraries(compa_shared
    PUBLIC UPnPsdk_shared
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


# Compa STatic library: suffix -cst
#----------------------------------
add_library(compa_static STATIC
    ${COMPA_SOURCE_FILES}
)
set_target_properties(compa_static PROPERTIES
    OUTPUT_NAME $<IF:$<PLATFORM_ID:Windows>,upnpsdk-compa-static,upnpsdk-compa>
)
target_include_directories(compa_static
    ${COMPA_INCLUDE_DIRECTORIES}
)
target_compile_definitions(compa_static
    ${COMPA_COMPILE_DEFINITIONS}
)
target_link_libraries(compa_static
    PUBLIC UPnPsdk_static
)


# Installation
# ============
target_sources(compa_shared
    INTERFACE FILE_SET compa_public_headers TYPE HEADERS
              BASE_DIRS inc
                        ${CMAKE_BINARY_DIR}/include
              FILES ${COMPA_PUBLIC_HEADERS}
)
install(TARGETS compa_shared
    FILE_SET compa_public_headers
        DESTINATION $<IF:$<PLATFORM_ID:Windows>,include,include/UPnPsdk>
        COMPONENT Development
        EXCLUDE_FROM_ALL
)

install(TARGETS compa_static)

# vim: nowrap
