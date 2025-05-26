# Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2025-05-25

cmake_minimum_required(VERSION 3.23) # for FILE_SET
include(UPnPsdk-ProjectHeader)

project(PUPNP_IXML VERSION "11.1.2"
                   DESCRIPTION "IXML library"
                   HOMEPAGE_URL "https://github.com/UPnPsdk")

message(STATUS "PUPNP_IXML_VERSION set to \"${PUPNP_IXML_VERSION}\"")

set (IXML_SOURCES
    src/attr.cpp
    src/document.cpp
    src/element.cpp
    src/ixml.cpp
    $<$<CONFIG:Debug>:src/ixmldebug.cpp>
    src/ixmlmembuf.cpp
    src/ixmlparser.cpp
    src/namedNodeMap.cpp
    src/node.cpp
    src/nodeList.cpp
)

set (IXML_INCLUDE_DIRECTORIES
    PUBLIC inc
    PRIVATE src/inc
    # Next variable is needed for one step configure. Don't use
    # PRIVATE ${PUPNP_UPNP_SOURCE_DIR}/inc
    PUBLIC ${PUPNP_SOURCE_DIR}/upnp/inc
    INTERFACE ${CMAKE_BINARY_DIR}/include
)

# ixml shared library
#--------------------
add_library (ixml_shared SHARED
    ${IXML_SOURCES}
)
set_target_properties(ixml_shared PROPERTIES
    OUTPUT_NAME ixml
)
target_include_directories(ixml_shared
    ${IXML_INCLUDE_DIRECTORIES}
)
target_compile_definitions(ixml_shared
    PRIVATE UPnPsdk_EXPORTS
    PRIVATE $<$<BOOL:${UPnPsdk_WITH_IXML_SCRIPT}>:IXML_HAVE_SCRIPTSUPPORT>
)
target_link_libraries(ixml_shared
    PUBLIC UPnPsdk_shared   # for Umock
)

# ixml static library
#--------------------
add_library (ixml_static STATIC
    ${IXML_SOURCES}
)
set_target_properties(ixml_static PROPERTIES
    OUTPUT_NAME $<IF:$<PLATFORM_ID:Windows>,ixml-static,ixml>
)
target_include_directories(ixml_static
    ${IXML_INCLUDE_DIRECTORIES}
)
target_compile_definitions(ixml_static
    PRIVATE $<$<BOOL:${UPnPsdk_WITH_IXML_SCRIPT}>:IXML_HAVE_SCRIPTSUPPORT>
)
target_link_libraries(ixml_static
    PRIVATE UPnPsdk_static   # for Umock
)

# Installation
# ------------
install(TARGETS ixml_shared ixml_static)
