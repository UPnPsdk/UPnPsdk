# Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2024-09-28

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(APPS VERSION 0002
        DESCRIPTION "Applications using different UPnPsdk libraries"
        HOMEPAGE_URL "https://github.com/UPnPsdk")


if(UPnPsdk_WITH_NATIVE_PUPNP)
# Info executable linked with the pupnp shared library
# ----------------------------------------------------
add_executable(UPnPsdk-psh
        src/info_UPnPsdk.cpp
)
set_target_properties(UPnPsdk-psh PROPERTIES
        OUTPUT_NAME info_pupnp-shared
)
target_link_libraries(UPnPsdk-psh
    PRIVATE pupnp_shared
)

# Info executable linked with the pupnp static library
#-----------------------------------------------------
add_executable(UPnPsdk-pst
        src/info_UPnPsdk.cpp
)
set_target_properties(UPnPsdk-pst PROPERTIES
        OUTPUT_NAME info_pupnp-static
)
target_link_libraries(UPnPsdk-pst
    PRIVATE
        pupnp_static
        # On Linux we cannot link with the static pthreads library because then
        # we have also to link with a static libc that's not available by
        # default.
        $<$<NOT:$<PLATFORM_ID:Windows>>:${UPNP_PTHREADS_SHARED_LIBRARY}>
)
endif()


if(FALSE)
# TODO: create info function in the library and call that to avoid exporting private symbols.
# Info executable linked with the compatible shared library
# ---------------------------------------------------------
add_executable(UPnPsdk-csh
        src/info_UPnPsdk.cpp
)
set_target_properties(UPnPsdk-csh PROPERTIES
        OUTPUT_NAME info_compa-shared
)
target_link_libraries(UPnPsdk-csh
    PRIVATE
        compa_shared
)
endif()

# Info executable linked with the compatible static library
#----------------------------------------------------------
add_executable(UPnPsdk-cst
        src/info_UPnPsdk.cpp
)
set_target_properties(UPnPsdk-cst PROPERTIES
        OUTPUT_NAME info_compa-static
)
target_link_libraries(UPnPsdk-cst
    PRIVATE
        compa_static
        UPnPsdk_static
        # On Linux we cannot link with the static pthreads library because then
        # we have also to link with a static libc that's not available by
        # default.
        $<$<NOT:$<PLATFORM_ID:Windows>>:${UPNP_PTHREADS_SHARED_LIBRARY}>
)


# Simple API calls to test the UPnPsdk application interface without Googletest
# -----------------------------------------------------------------------------
if(UPnPsdk_WITH_NATIVE_PUPNP)
add_executable(api_calls-psh
    src/api_calls.cpp
)
target_link_libraries(api_calls-psh
    PRIVATE pupnp_shared
)
# Installation
install(TARGETS api_calls-psh
    COMPONENT Development
    EXCLUDE_FROM_ALL
)
endif()

add_executable(api_calls-csh
    src/api_calls.cpp
)
target_link_libraries(api_calls-csh
    PRIVATE compa_shared
    PRIVATE UPnPsdk_shared
)

#add_executable(api_calls-csh
#    src/api_calls.cpp
#)
#target_include_directories(api_calls-csh
#    PRIVATE ${COMPA_SOURCE_DIR}/inc
#    PRIVATE ${COMPA_SOURCE_DIR}/src/inc
#    PRIVATE ${CMAKE_SOURCE_DIR}/UPnPsdk/include
#    PRIVATE ${PUPNP_SOURCE_DIR}/ixml/inc
#)
#find_library(COMPA_SHARED upnpsdk-compa)
#find_library(UPNPSDK_SHARED upnpsdk
#)
#target_link_libraries(api_calls-csh
#    PRIVATE "${COMPA_SHARED}"
#    PRIVATE "${UPNPSDK_SHARED}"
#)

# Installation
install(TARGETS api_calls-csh
    COMPONENT Development
    EXCLUDE_FROM_ALL
)
