# Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2025-06-10

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(SAMPLE VERSION 0011
               DESCRIPTION "Simple sample to use the library"
               HOMEPAGE_URL "https://github.com/UPnPsdk")


# Executable |               |       |
#    code    | with library  | build | mode
# -----------+---------------+-------+-----------------------
#    old     |     pupnp     |  yes  | verify
#    old     |     compa     |  yes  | compatibility
#    new     |     pupnp     |  no   | not supported
#    new     |     compa     |  yes  | portability to UPnPsdk


set(PUPNP_COMPILE_OPTIONS
    # ‘%d’ directive writing between 1 and 11 bytes into a region of size 5
    PRIVATE $<$<PLATFORM_ID:Linux>:-Wno-format-overflow>
    # Disable warning C4706: assignment within conditional expression
    PRIVATE $<$<PLATFORM_ID:Windows>:/wd4706>
)

# tv_ctrlpt
# =========
set(TV_CTRLPT_PUPNP_SOURCE_FILES
    ${PUPNP_SOURCE_DIR}/sample/common/sample_util.c
    ${PUPNP_SOURCE_DIR}/sample/common/tv_ctrlpt.c
    ${PUPNP_SOURCE_DIR}/sample/linux/tv_ctrlpt_main.c
)
set(TV_CTRLPT_COMPA_SOURCE_FILES
    common/sample_util.cpp
    common/tv_ctrlpt.cpp
    linux/tv_ctrlpt_main.cpp
)

if(UPnPsdk_WITH_NATIVE_PUPNP)
    add_executable (tv_ctrlpt_pupnp-psh
    # ---------------------------------
        ${TV_CTRLPT_PUPNP_SOURCE_FILES}
    )
    target_include_directories(tv_ctrlpt_pupnp-psh
        PRIVATE ${PUPNP_SOURCE_DIR}/sample/common
    )
    target_compile_options(tv_ctrlpt_pupnp-psh
        ${PUPNP_COMPILE_OPTIONS}
    )
    target_link_libraries(tv_ctrlpt_pupnp-psh
        PRIVATE pupnp_shared
    )

    install(TARGETS tv_ctrlpt_pupnp-psh)
endif()

add_executable (tv_ctrlpt-psh
# ---------------------------
    ${TV_CTRLPT_PUPNP_SOURCE_FILES}
)
target_include_directories(tv_ctrlpt-psh
    PRIVATE ${PUPNP_SOURCE_DIR}/sample/common
    PRIVATE ${COMPA_SOURCE_DIR}/inc/ixml
)
target_compile_options(tv_ctrlpt-psh
    ${PUPNP_COMPILE_OPTIONS}
)
target_link_libraries(tv_ctrlpt-psh
    PRIVATE compa_shared
)

install(TARGETS tv_ctrlpt-psh)

add_executable (tv_ctrlpt-csh
# ---------------------------
    ${TV_CTRLPT_COMPA_SOURCE_FILES}
)
target_include_directories(tv_ctrlpt-csh
    PRIVATE common
)
target_link_libraries(tv_ctrlpt-csh
    PRIVATE compa_shared
)

install(TARGETS tv_ctrlpt-csh)


# tv_ctrlpt and tv_device
# =======================
set(TV_COMBO_PUPNP_SOURCE_FILES
    ${PUPNP_SOURCE_DIR}/sample/common/sample_util.c
    ${PUPNP_SOURCE_DIR}/sample/common/tv_ctrlpt.c
    ${PUPNP_SOURCE_DIR}/sample/common/tv_device.c
    ${PUPNP_SOURCE_DIR}/sample/linux/tv_combo_main.c
)
set(TV_COMBO_COMPA_SOURCE_FILES
    common/sample_util.cpp
    common/tv_ctrlpt.cpp
    common/tv_device.cpp
    linux/tv_combo_main.cpp
)

if(UPnPsdk_WITH_NATIVE_PUPNP)
    add_executable (tv_combo_pupnp-psh
    # --------------------------------
        ${TV_COMBO_PUPNP_SOURCE_FILES}
    )
    target_include_directories(tv_combo_pupnp-psh
        PRIVATE ${PUPNP_SOURCE_DIR}/sample/common
    )
    target_compile_options(tv_combo_pupnp-psh
        ${PUPNP_COMPILE_OPTIONS}
    )
    target_link_libraries(tv_combo_pupnp-psh
        PRIVATE pupnp_shared
    )

    install(TARGETS tv_combo_pupnp-psh)
endif()

add_executable (tv_combo-psh
# --------------------------
    ${TV_COMBO_PUPNP_SOURCE_FILES}
)
target_include_directories(tv_combo-psh
    PRIVATE ${PUPNP_SOURCE_DIR}/sample/common
    PRIVATE ${COMPA_SOURCE_DIR}/inc/ixml
)
target_compile_options(tv_combo-psh
    ${PUPNP_COMPILE_OPTIONS}
)
target_link_libraries(tv_combo-psh
    PRIVATE compa_shared
)

install(TARGETS tv_combo-psh)

add_executable (tv_combo-csh
# --------------------------
    ${TV_COMBO_COMPA_SOURCE_FILES}
)
target_include_directories(tv_combo-csh
    PRIVATE common
)
target_link_libraries(tv_combo-csh
    PRIVATE compa_shared
)

install(TARGETS tv_combo-csh)


# tv_device
# =========
set(TV_DEVICE_PUPNP_SOURCE_FILES
    ${PUPNP_SOURCE_DIR}/sample/common/sample_util.c
    ${PUPNP_SOURCE_DIR}/sample/common/tv_device.c
    ${PUPNP_SOURCE_DIR}/sample/linux/tv_device_main.c
)
set(TV_DEVICE_COMPA_SOURCE_FILES
    common/sample_util.cpp
    common/tv_device.cpp
    linux/tv_device_main.cpp
)

if(UPnPsdk_WITH_NATIVE_PUPNP)
    add_executable (tv_device_pupnp-psh
    # ---------------------------------
    ${TV_DEVICE_PUPNP_SOURCE_FILES}
    )
    target_include_directories(tv_device_pupnp-psh
        PRIVATE ${PUPNP_SOURCE_DIR}/sample/common
    )
    target_compile_options(tv_device_pupnp-psh
        ${PUPNP_COMPILE_OPTIONS}
    )
    target_link_libraries(tv_device_pupnp-psh
        PRIVATE pupnp_shared
    )

    install(TARGETS tv_device_pupnp-psh)
endif()

add_executable (tv_device-psh
# ---------------------------
    ${TV_DEVICE_PUPNP_SOURCE_FILES}
)
target_include_directories(tv_device-psh
    PRIVATE ${PUPNP_SOURCE_DIR}/sample/common
    PRIVATE ${COMPA_SOURCE_DIR}/inc/ixml
)
target_compile_options(tv_device-psh
    ${PUPNP_COMPILE_OPTIONS}
)
target_link_libraries(tv_device-psh
    PRIVATE compa_shared
)

install(TARGETS tv_device-psh)

add_executable (tv_device-csh
# ---------------------------
    ${TV_DEVICE_COMPA_SOURCE_FILES}
)
target_include_directories(tv_device-csh
    PRIVATE common
)
target_link_libraries(tv_device-csh
    PRIVATE compa_shared
)

install(TARGETS tv_device-csh)
