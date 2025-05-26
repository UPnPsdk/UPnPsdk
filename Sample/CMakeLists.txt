# Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2025-05-28

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(SAMPLE VERSION 0011
               DESCRIPTION "Simple sample to use the library"
               HOMEPAGE_URL "https://github.com/UPnPsdk")

include_directories(
    common
)


# tv_ctrlpt
# ---------
set(TV_CTRLPT_SOURCE_FILES
    common/sample_util.cpp
    common/tv_ctrlpt.cpp
    linux/tv_ctrlpt_main.cpp
)

if(UPnPsdk_WITH_NATIVE_PUPNP)
    add_executable (tv_ctrlpt-psh
        ${TV_CTRLPT_SOURCE_FILES}
    )
    target_link_libraries(tv_ctrlpt-psh
        PRIVATE pupnp_shared
        PRIVATE UPnPsdk_shared
    )

    install(TARGETS tv_ctrlpt-psh)
endif()

add_executable (tv_ctrlpt-csh
    ${TV_CTRLPT_SOURCE_FILES}
)
target_link_libraries(tv_ctrlpt-csh
    PRIVATE compa_shared
)

install(TARGETS tv_ctrlpt-csh)


# ctrlpt and device
# -----------------
set(TV_COMBO_SOURCE_FILES
    common/sample_util.cpp
    common/tv_ctrlpt.cpp
    common/tv_device.cpp
    linux/tv_combo_main.cpp
)

if(UPnPsdk_WITH_NATIVE_PUPNP)
    add_executable (tv_combo-psh
        ${TV_COMBO_SOURCE_FILES}
    )
    target_link_libraries(tv_combo-psh
        PRIVATE pupnp_shared
        PRIVATE UPnPsdk_shared
    )

    install(TARGETS tv_combo-psh)
endif()

add_executable (tv_combo-csh
    ${TV_COMBO_SOURCE_FILES}
)
target_link_libraries(tv_combo-csh
    PRIVATE compa_shared
)

install(TARGETS tv_combo-csh)


# tv_device
# ---------
set(TV_DEVICE_SOURCE_FILES
    common/sample_util.cpp
    common/tv_device.cpp
    linux/tv_device_main.cpp
)

if(UPnPsdk_WITH_NATIVE_PUPNP)
    add_executable (tv_device-psh
        ${TV_DEVICE_SOURCE_FILES}
    )
    target_link_libraries(tv_device-psh
        PRIVATE pupnp_shared
        PRIVATE UPnPsdk_shared
    )

    install(TARGETS tv_device-psh)
endif()

add_executable (tv_device-csh
    ${TV_DEVICE_SOURCE_FILES}
)
target_link_libraries(tv_device-csh
    PRIVATE compa_shared
)

install(TARGETS tv_device-csh)
