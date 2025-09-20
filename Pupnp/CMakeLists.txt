# Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2025-09-23

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(PUPNP VERSION "1.14.20"
        DESCRIPTION "Old code from the pupnp library"
        HOMEPAGE_URL "https://github.com/UPnPsdk")


add_compile_options(
    # Default warning level
    $<$<CXX_COMPILER_ID:MSVC>:/W1>
)

if(UPnPsdk_WITH_NATIVE_PUPNP)
    add_subdirectory(${PROJECT_SOURCE_DIR}/ixml)
    add_subdirectory(${PROJECT_SOURCE_DIR}/upnp)
endif()
