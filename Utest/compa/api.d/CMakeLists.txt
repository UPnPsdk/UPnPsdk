# Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2024-11-20

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(GTESTS_COMPA_API VERSION 0003
                  DESCRIPTION "Tests for api module and compatible code"
                  HOMEPAGE_URL "https://github.com/UPnPsdk")

# upnpdebug
#==========
# Because we want to include the source file into the test to also test static
# functions, we cannot use shared libraries due to symbol import/export
# conflicts on Microsoft Windows (warning C4273: inconsistent dll linkage). We
# must use static libraries.
add_executable(test_upnpdebug-pst
#--------------------------------
        test_upnpdebug.cpp
)
target_include_directories(test_upnpdebug-pst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_compile_options(test_upnpdebug-pst
    PRIVATE
        # disable warning C4459: declaration of 'fp' hides global declaration
        # This is intended and not fixed because I won't modify pupnp code.
        $<$<CXX_COMPILER_ID:MSVC>:/wd4459>
)
target_link_libraries(test_upnpdebug-pst
    PRIVATE
        pupnp_static
        utest_shared
)
add_test(NAME ctest_upnpdebug-pst COMMAND test_upnpdebug-pst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(test_upnpdebug-cst
#--------------------------------
        test_upnpdebug.cpp
)
target_include_directories(test_upnpdebug-cst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_upnpdebug-cst
    PRIVATE
        compa_static
        utest_shared
)
add_test(NAME ctest_upnpdebug-cst COMMAND test_upnpdebug-cst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# upnptools
#==========
add_executable(test_upnptools-psh
#--------------------------------
        test_upnptools.cpp
)
target_link_libraries(test_upnptools-psh
    PRIVATE pupnp_shared
)
add_test(NAME ctest_upnptools-psh COMMAND test_upnptools-psh --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# UpnpString
#===========
add_executable(test_UpnpString-psh
#---------------------------------
    test_UpnpString.cpp
)
target_link_libraries(test_UpnpString-psh
    PRIVATE pupnp_shared
    PRIVATE utest_shared
)
add_test(NAME ctest_UpnpString-psh COMMAND test_UpnpString-psh --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(test_UpnpString-csh
#---------------------------------
    test_UpnpString.cpp
)
target_link_libraries(test_UpnpString-csh
    PRIVATE compa_shared
    PRIVATE utest_shared
)
add_test(NAME ctest_UpnpString-csh COMMAND test_UpnpString-csh --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# UpnpFileInfo
#=============
add_executable(test_UpnpFileInfo-psh
#-----------------------------------
    test_UpnpFileInfo.cpp
)
target_link_libraries(test_UpnpFileInfo-psh
    PRIVATE pupnp_shared
)
add_test(NAME ctest_UpnpFileInfo-psh COMMAND test_UpnpFileInfo-psh --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(test_UpnpFileInfo-csh
#-----------------------------------
    test_UpnpFileInfo.cpp
    ${COMPA_SOURCE_DIR}/src/genlib/util/list.cpp
)
target_link_libraries(test_UpnpFileInfo-csh
    PRIVATE compa_shared
)
add_test(NAME ctest_UpnpFileInfo-csh COMMAND test_UpnpFileInfo-csh --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# upnpapi
#========
# Because we want to include the source file into the test to also test static
# functions, we cannot use shared libraries due to symbol import/export
# conflicts on Microsoft Windows (warning C4273: inconsistent dll linkage). I
# must use static libraries.
add_executable(test_upnpapi-pst
#------------------------------
    test_upnpapi.cpp
)
target_include_directories(test_upnpapi-pst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_compile_options(test_upnpapi-pst
    # disable warning C4701: potentially uninitialized local variable
    # '<variable>' used
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/wd4701>
    # disable warning C4706: assignment within conditional expression
    $<$<CXX_COMPILER_ID:MSVC>:/wd4706>
)
target_link_libraries(test_upnpapi-pst
    PRIVATE
        pupnp_static
        utest_shared
)
add_test(NAME ctest_upnpapi-pst COMMAND test_upnpapi-pst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(test_upnpapi-cst
#------------------------------
    test_upnpapi.cpp
)
target_include_directories(test_upnpapi-cst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_upnpapi-cst
    PRIVATE
        compa_static
        utest_shared
)
add_test(NAME ctest_upnpapi-cst COMMAND test_upnpapi-cst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(test_upnpapi_platform-pst
#---------------------------------------
    $<$<PLATFORM_ID:Windows>:test_upnpapi_win32.cpp>
    $<$<NOT:$<PLATFORM_ID:Windows>>:test_upnpapi_unix.cpp>
)
target_include_directories(test_upnpapi_platform-pst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_compile_options(test_upnpapi_platform-pst
        # disable warning C4505: 'GetFreeHandle': unreferenced local function
        # has been removed This isn't used with WIN32 but on other platforms. I
        # won't modify pupnp code. --Ingo
        PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/wd4505>
)
target_link_libraries(test_upnpapi_platform-pst
    PRIVATE
        pupnp_static
        UPnPsdk_static
        utest_static
)
add_test(NAME ctest_upnpapi_platform-pst COMMAND test_upnpapi_platform-pst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(test_upnpapi_platform-cst
#---------------------------------------
    $<$<PLATFORM_ID:Windows>:test_upnpapi_win32.cpp>
    $<$<NOT:$<PLATFORM_ID:Windows>>:test_upnpapi_unix.cpp>
)
target_include_directories(test_upnpapi_platform-cst
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_upnpapi_platform-cst
    PRIVATE
        compa_static
        UPnPsdk_static
        utest_static
)
add_test(NAME ctest_upnpapi_platform-cst COMMAND test_upnpapi_platform-cst --gtest_shuffle
    WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)
