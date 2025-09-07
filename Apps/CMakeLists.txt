# Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2025-09-14

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(APPS VERSION 0003
        DESCRIPTION "Applications using different UPnPsdk libraries"
        HOMEPAGE_URL "https://github.com/UPnPsdk")


# Info executable linked with the compatible shared library
# ---------------------------------------------------------
# This does not compile without error because it needs exported symbols from
# internal functions. But symbols from internal functions are not exported by
# design.
if(FALSE)
    add_executable(UPnPsdk-csh
            src/UPnPsdk_info.cpp
    )
    set_target_properties(UPnPsdk-csh PROPERTIES
            OUTPUT_NAME UPnPsdk_info-csh
    )
    target_link_libraries(UPnPsdk-csh
        PRIVATE compa_shared
    )

    # Installation
    install(TARGETS UPnPsdk-csh)
endif()

# Info executable linked with the compatible static library
#----------------------------------------------------------
add_executable(UPnPsdk-cst
        src/UPnPsdk_info.cpp
)
set_target_properties(UPnPsdk-cst PROPERTIES
        OUTPUT_NAME UPnPsdk_info
)
target_link_libraries(UPnPsdk-cst
    PRIVATE compa_static
)

# Installation
install(TARGETS UPnPsdk-cst)


# Simple API calls to test the UPnPsdk application interface without Googletest
# -----------------------------------------------------------------------------
if(UPnPsdk_WITH_NATIVE_PUPNP)
    add_executable(api_calls-psh
        src/api_calls.c
    )
    target_link_libraries(api_calls-psh
        PRIVATE pupnp_shared
    )

    # Installation
    #install(TARGETS api_calls-psh)
endif()


add_executable(api_calls-csh
    src/api_calls.cpp
)
target_link_libraries(api_calls-csh
    PRIVATE compa_shared
)

# Installation
install(TARGETS api_calls-csh)
