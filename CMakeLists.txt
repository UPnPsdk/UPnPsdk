# Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2025-06-01

cmake_minimum_required(VERSION 3.29) # For FetchContent_MakeAvailable()
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake CACHE INTERNAL "Path to custom UPnPsdk modules to be used with include() and find_package()." FORCE)
include(UPnPsdk-ProjectHeader)

project("UPnPsdk" VERSION "0.2.0"
                DESCRIPTION "UPnP Software Development Kit"
                HOMEPAGE_URL "https://github.com/UPnPsdk")

set(PUPNP_VERSION "1.14.20"
    CACHE INTERNAL "The current version of the forked pUPnP library." FORCE)
# Update pUPnP version number in README.md.
if(NOT PUPNP_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "${PUPNP_VERSION} is not a valid pUPnP version number.")
endif()
configure_file(${PROJECT_SOURCE_DIR}/cmake/README.md.cmake ${PROJECT_SOURCE_DIR}/README.md NEWLINE_STYLE UNIX)

enable_testing()

#################################
# Options                       #
#################################
option(UPnPsdk_WITH_NATIVE_PUPNP "Build the library with old native PUPNP code." OFF)
option(UPnPsdk_WITH_OPENSSL "SSL/TLS and Crypto support should always be ON if not restricted by law." ON)
option(UPnPsdk_WITH_MINISERVER "Provide the miniserver for low level network communication." OFF)
option(UPnPsdk_WITH_WEBSERVER "Provide a webserver." OFF)
option(UPnPsdk_WITH_CTRLPT_SSDP "Providing control point functionality to manage a remote UPnP device." OFF)
option(UPnPsdk_WITH_DEVICE_SSDP "Providing UPnP device functionality so remote control points can connect to it." OFF)
option(UPnPsdk_WITH_OPTION_SSDP "Enable optional SSDP headers." OFF)
option(UPnPsdk_WITH_CTRLPT_DESCRIPTION "Enable requesting from a control point for a devices UPnP description." OFF)
option(UPnPsdk_WITH_DEVICE_DESCRIPTION "Enable responding from a device to a control points UPnP description request." OFF)
option(UPnPsdk_WITH_CTRLPT_SOAP "Enable SOAP, the Simple Object Access Protocol." OFF)
option(UPnPsdk_WITH_DEVICE_SOAP "Enable SOAP, the Simple Object Access Protocol." OFF)
option(UPnPsdk_WITH_CTRLPT_GENA "Enable GENA, the General Event Notification Architecture." OFF)
option(UPnPsdk_WITH_DEVICE_GENA "Enable GENA, the General Event Notification Architecture." OFF)
option(UPnPsdk_WITH_IXML_SCRIPT "Enable IXML script support." OFF)
option(UPnPsdk_WITH_TOOLS "Make some not essential program features available like text for error numbers etc." OFF)
option(UPnPsdk_WITH_SAMPLES "Provide sample programs UPnP device and control point." OFF)
option(UPnPsdk_WITH_GOOGLETEST "Download and build Googletest." OFF)
option(UPnPsdk_WITH_TRACE "Compile trace messages into the libraries for analizing program execution." OFF)
# Because we link utests with shared and static libraries we need gtest/gmock
# libraries that link together with both versions of other libraries. These are
# the shared gtest/gmock libraries. Next option is only available for testing
# other combinations. It should be set to ON by default otherwise you may get
# compiling errors if you do not know what you are doing.
option(GTESTS_WITH_SHARED_LIBS "Build GOOGLETEST libraries shared. You should not disable this except you know what you doing." ON)
# Next is an option to set to "Unspecified" the OS name, OS version, product
# name and product version normally contained in the SERVER header of a UPnP
# http request as this could be used by an attacker.
#option(UPnPsdk_WITH_UNSPECIFIED_SERVER "Set information in UPnP SERVER http request header to \"Unspecified\"". OFF)
option(PT4W_BUILD_TESTING "Build the time and space consuming test suite of pthreads4w" OFF)


#################################
#  Manage 32 bit architecture   #
#################################
set(UPnPsdk_DEF_32BIT NO)
if(MSVC)
    if("Win32" IN_LIST CMAKE_GENERATOR_PLATFORM)
        set(UPnPsdk_DEF_32BIT YES)
    endif()
else()
    if("-m32" IN_LIST CMAKE_CXX_FLAGS)
        set(UPnPsdk_DEF_32BIT YES)
    endif()
endif()
message(STATUS "Compile 32 bit architecture: ${UPnPsdk_DEF_32BIT}")

if(UPnPsdk_DEF_32BIT)
    if(UPnPsdk_WITH_OPENSSL)
        message(FATAL_ERROR "UPnPsdk_WITH_OPENSSL=ON: temporary not available due to missing 32 bit OpenSSL libraries.")
    endif()
    # Large-file-sensitivity on 32 bit architecture
    # Note: On Microsoft Windows 'off_t' is always long that would make it fix
    # 32-bit (even 64-bit Windows uses 32-bit for long). That means it is never
    # sensitive. We have explicit to use _fseeki64 and _ftelli64 to access
    # large files there.
    if(NOT MSVC)
        set (UPNP_LARGEFILE_SENSITIVE 1 CACHE INTERNAL "whether the system defaults to 32bit off_t but can do 64bit when requested" FORCE)
    endif()
endif()


#################################
# Paths                         #
#################################
# Set default POSIX Threads Libraries. May be overwritten if pthreads4w is used.
# See below on installing pthreads4w.
set(UPNP_PTHREADS_SHARED_LIBRARY pthread CACHE INTERNAL "Name of the linkable shared library for POSIX threads")  # Implies FORCE
set(UPNP_PTHREADS_STATIC_LIBRARY libpthread.a CACHE INTERNAL "Name of the linkable static library for POSIX threads")  # Implies FORCE
# the RPATH to be stored in executable and libraries when installing that is by
# default "/usr/local/lib"
# REF: [RPATH-handling](https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/RPATH-handling)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(UMOCK_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Umock")
set(IXML_TESTDATA_DIR "${CMAKE_SOURCE_DIR}/Pupnp/ixml/test/testdata")


#################################
# Compiling                     #
#################################
# Define custom cmake options for commandline and cmake-gui
#------------------------------------------------------------------------
# If you add or modify an option you should also have attention to a possible
# corresponding flag in "${PROJECT_SOURCE_DIR}/cmake/upnpconfig.hpp.cmake". These
# variables are exported at the end. See below at section "Export cmake
# variables".
# Variablename and its value-placeholder must be the same, e.g.
# #cmakedefine VAR1 ${VAR1}
# #cmakedefine VAR1 ${VAR} does not work.
# #cmakedefine VAR2 1  : VAR2 must be exist before even you set a constant.

# Set the default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None, Debug, Release, RelWithDebInfo, or MinSizeRel." FORCE)
    message(STATUS "No build type specified, defaulting to Release.")
endif()

# Check available library functions. We need that for later configuration.
include(CheckCXXSymbolExists)

# Check if strnlen and strndup are provided by the operating system.
check_cxx_symbol_exists(strnlen "cstring" UPnPsdk_HAVE_STRNLEN)
check_cxx_symbol_exists(strndup "cstring" UPnPsdk_HAVE_STRNDUP)

# Suffix on libraries having built with Debug information
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(UPNP_DEBUG_SUFFIX "d")
endif()

# We have visibility support as described at https://gcc.gnu.org/wiki/Visibility
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# Provide length of the path of the projects base directory. We need this
# mainly for the compile time short file name of Debug and TRACE messages.
string(LENGTH "${CMAKE_SOURCE_DIR}/" CMAKE_SOURCE_PATH_LENGTH)

# Set general compile definitions and options
#--------------------------------------------
add_compile_definitions(
        # Having STRNLEN and STRNDUP are validated above. Give it to the program.
        $<$<BOOL:${UPnPsdk_HAVE_STRNLEN}>:HAVE_STRNLEN>
        $<$<BOOL:${UPnPsdk_HAVE_STRNDUP}>:HAVE_STRNDUP>
        # General define DEBUG if build type is "Debug". Manage setting NDEBUG
        # is done by cmake by default.
        $<$<CONFIG:Debug>:DEBUG>

        # Set Large-file support if 32 bit architecture is used.
        $<$<BOOL:${UPNP_LARGEFILE_SENSITIVE}>:_FILE_OFFSET_BITS=64>

        # There are traditional min/max macros on MS Windows which conflict
        # with the C++ std::min|max functions. Following switch disable the
        # macros.
        $<$<CXX_COMPILER_ID:MSVC>:NOMINMAX>

        $<$<CXX_COMPILER_ID:MSVC>:UPNP_USE_MSVCPP>
        $<$<CXX_COMPILER_ID:MSVC>:FD_SETSIZE=1024>
        $<$<BOOL:${UPnPsdk_WITH_TRACE}>:UPnPsdk_WITH_TRACE>
)

add_compile_options(
        # Most compiler warnings enabled
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        # all compiler warnings as errors
        $<$<CXX_COMPILER_ID:MSVC>:/WX>
        # disable warning C4996: 'strdup'(or others): The POSIX name for this
        # item is deprecated. Instead, use the ISO C and C++ conformant name:
        # _strdup. See online help for details. (This is Microsoft specific, no
        # need to fix it because the functions are general still valid).
        $<$<CXX_COMPILER_ID:MSVC>:/wd4996>
        # Disable warning C4456: declaration of 'var' hides previous local
        # declaration. I like to redeclare local variable names.
        $<$<CXX_COMPILER_ID:MSVC>:/wd4456>
        # lots of warnings and all warnings as errors
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wextra>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wpedantic>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wuninitialized>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-dangling-else>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wconversion>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wsign-conversion>
        $<$<CXX_COMPILER_ID:GNU>:-Wno-format-truncation>
        # Next is only valid for C++/ObjC++ but not for C
        $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU>>:-Wsuggest-override>
        $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU>>:-fstack-protector-all>
        $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU>>:-fstack-protector-strong>
        # Next is only to suppress a warning for the TRACE macro.
        #$<$<AND:$<CXX_COMPILER_ID:AppleClang>,$<BOOL:${UPnPsdk_WITH_TRACE}>>:-Wno-string-plus-int>

        #$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-fanalyzer>
        #$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wfatal-errors>
)

if(DEFINED ENV{GITHUB_ACTIONS})
    add_compile_options(
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-unused-result>)
endif()

add_link_options(
    $<$<CXX_COMPILER_ID:MSVC>:/WX>   # all linker warnings as errors
)


#################################
# Targets                       #
#################################
# Set target output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
if(WIN32)
    if(CMAKE_GENERATOR STREQUAL "NMake Makefiles")
        set(UPnPsdk_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    else()
        set(UPnPsdk_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE})
    endif()
else()
    set(UPnPsdk_RUNTIME_OUTPUT_DIRECTORY "")
endif()
# Output with cmake option --log-level=DEBUG
message(DEBUG "  DEBUG: UPnPsdk_RUNTIME_OUTPUT_DIRECTORY is: ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}")


##########################################
# POSIX Threads for Windows (pthreads4w) #
##########################################
# REF: [Install third party POSIX Threads for Windows](https://stackoverflow.com/q/69275376/5014688)
# This should be one of the first installations to have pthreads available.

if(WIN32)
    # This dependent project must use generator "NMake Makefiles" that differs
    # from default generator "Visual Studio *". NMake Makefiles does not
    # support platform specification with CMake option '-A'. So we cannot use
    # add_directory() because this will also compile the dependent project with
    # "Visual Studio *". We have to configure and build it in separate
    # processes.

    # PTW32_STATIC_LIB - for using the static pthreads4W library. I do that.
    # The option is contained in cmake/upnpconfig.hpp.cmake but has no effect with
    # the current implementation. Only for the completeness:
    set(PTW32_STATIC_LIB ON CACHE BOOL "For using the static pthreads4w library" FORCE)

    include(FetchContent)

    message(CHECK_START "Download and configuring POSIX Threads for Windows")

    FetchContent_Declare(
        pthreads4w
        GIT_REPOSITORY  https://github.com/jwinarske/pthreads4w.git
        GIT_TAG         origin/cmake
        GIT_SHALLOW     ON)

    # Check if population has already been performed
    FetchContent_GetProperties(pthreads4w)
    if(NOT pthreads4w_POPULATED)
        # Fetch the content using previously declared details.
        # This caches pthreads4w_SOURCE_DIR, pthreads4w_BINARY_DIR and pthreads4w_POPULATED.
        FetchContent_Populate(pthreads4w)

        execute_process(COMMAND cmake -S ${pthreads4w_SOURCE_DIR} -B ${pthreads4w_BINARY_DIR} -G "NMake Makefiles" -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -D BUILD_TESTING=${PT4W_BUILD_TESTING}
                        WORKING_DIRECTORY ${pthreads4w_SOURCE_DIR}
                        RESULT_VARIABLE PT4W_RETURN_CODE
                        ERROR_VARIABLE PT4W_ERROR_MESSAGE
                        COMMAND_ECHO STDOUT)
        if(NOT ${PT4W_RETURN_CODE} EQUAL 0)
            # This will stop the installation
            message(FATAL_ERROR "${PT4W_RETURN_CODE}: ${PT4W_ERROR_MESSAGE}")
        endif()

        message(CHECK_PASS "done")
        message(CHECK_START "Building POSIX Threads for Windows libraries")

        # Manage output for cmake option --log-level=DEBUG
        cmake_language(GET_MESSAGE_LOG_LEVEL MSG_LOG_LEVEL)
        if(MSG_LOG_LEVEL STREQUAL "DEBUG")
            set(PT4W_COMMAND COMMAND_ECHO)
            set(PT4W_COMMAND_OUTPUT "STDOUT")
        else()
            set(PT4W_COMMAND OUTPUT_QUIET)
            set(PT4W_COMMAND_OUTPUT "")
        endif()
        execute_process(COMMAND cmake --build ${pthreads4w_BINARY_DIR} --config ${CMAKE_BUILD_TYPE}
                        WORKING_DIRECTORY ${pthreads4w_SOURCE_DIR}
                        RESULT_VARIABLE PT4W_RETURN_CODE
                        ERROR_VARIABLE PT4W_ERROR_MESSAGE
                        ${PT4W_COMMAND} ${PT4W_COMMAND_OUTPUT})
        if(NOT ${PT4W_RETURN_CODE} EQUAL 0)
            # This will stop the installation
            message(FATAL_ERROR "${PT4W_RETURN_CODE}: ${PT4W_ERROR_MESSAGE}")
        endif()

        set(UPNP_PTHREADS_SHARED_LIBRARY "${pthreads4w_BINARY_DIR}/pthreadVC3${UPNP_DEBUG_SUFFIX}.lib"
            CACHE INTERNAL "Name of the linkable shared library for POSIX threads")  # Implies FORCE
        set(UPNP_PTHREADS_STATIC_LIBRARY "${pthreads4w_BINARY_DIR}/libpthreadVC3${UPNP_DEBUG_SUFFIX}.lib"
            CACHE INTERNAL "Name of the linkable static library for POSIX threads")  # Implies FORCE

        message(CHECK_PASS "done")

    endif()

    # Output with cmake option --log-level=DEBUG
    message(DEBUG "  DEBUG: pthreads4w_SOURCE_DIR is: ${pthreads4w_SOURCE_DIR}")
    message(DEBUG "  DEBUG: pthreads4w_BINARY_DIR is: ${pthreads4w_BINARY_DIR}")

endif(WIN32)


#################################
# OpenSSL                       #
#################################
if(UPnPsdk_WITH_OPENSSL)
    #set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")
    find_package(OpenSSL REQUIRED)
    # Output with cmake option --log-level=DEBUG
    message(DEBUG "  DEBUG: OPENSSL_INCLUDE_DIR is: ${OPENSSL_INCLUDE_DIR}")
endif()


#################################
# Googletest                    #
#################################
# Download, configure and build
if(UPnPsdk_WITH_GOOGLETEST)

    # It is important to have the old library available with the tests because
    # the new code is compared with it.
    set (UPnPsdk_WITH_NATIVE_PUPNP ON CACHE BOOL
        "Build the library with old native PUPNP code." FORCE)
    set (UPnPsdk_WITH_SAMPLES ON CACHE BOOL
         "Provide sample programs UPnP device and control point." FORCE)

    if(GTESTS_WITH_SHARED_LIBS)
        set (BUILD_SHARED_LIBS ON CACHE INTERNAL
            "This option belongs only to GOOGLETEST and build its libraries shared.")  # Implies FORCE
    endif()

    # gtest_disable_pthreads is an option of the GoogleTest package set to OFF
    # by default so it always compiles with PThreads. This does not compiles on
    # MS Windows with pthreads4w. So I must disable it there.
    if(WIN32)
        set(gtest_disable_pthreads ON CACHE BOOL "Disable uses of pthreads in gtest.")  # Implies FORCE

    else()
        #It may already included to install pthreads4w on WIN32
        include(FetchContent)
    endif()

    message(CHECK_START "Download and configuring GoogleTest")

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY    https://github.com/google/googletest.git
        GIT_TAG           origin/main
        #GIT_TAG           release-1.11.0
        GIT_SHALLOW       ON # comment if using GIT_TAG hash below
        #GIT_TAG           ec7b38672c04c8af3cfb078214edf6751b841014
        # Next commit crashes gtest.dll (shared library build enabled) on MS
        # Windows with build-type DEBUG.
        #GIT_TAG           d3a29ff624e0984a40a60dbc1f03ac74b8d03658
    )
    # We have googletest embedded so we turn installation off.
    set (INSTALL_GTEST OFF CACHE BOOL
    "Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.)" FORCE)

    if(WIN32)

        # Reference: build/_deps/googletest-src/googletest/README.md
        # Must be set before FetchContent_MakeAvailable().
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

        # This caches FETCHCONTENT_BASE_DIR (e.g. ${PROJECT_BINARY_DIR}/_deps)),
        # googletest_SOURCE_DIR, googletest_BINARY_DIR and googletest_POPULATED.
        FetchContent_MakeAvailable(googletest)

        # Due to issue https://github.com/google/googletest/issues/1325#issuecomment-903884914
        # it should only link with the gmock libraries. They contain also gtest.

        message(CHECK_PASS "done")


        ##################################
        # Build Unit Tests on MS Windows #
        ##################################

        message(CHECK_START "configuring Unit Tests")

        # Create symlinks for shared gtest .dlls so the Unit Tests can find them.
        if(BUILD_SHARED_LIBS)
            if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
                # Make Directory is necessary because sub directory ${CMAKE_BUILD_TYPE} isn't available yet.
                file(MAKE_DIRECTORY ${GTESTS_BINARY_DIR}/${CMAKE_BUILD_TYPE})
                file(CREATE_LINK ${PROJECT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/gtest.dll
                                 $ENV{SystemRoot}/System32/gtest.dll SYMBOLIC)
                file(CREATE_LINK ${PROJECT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/gmock.dll
                                 $ENV{SystemRoot}/System32/gmock.dll SYMBOLIC)
                file(CREATE_LINK ${PROJECT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/gtest_main.dll
                                 $ENV{SystemRoot}/System32/gtest_main.dll SYMBOLIC)
                file(CREATE_LINK ${PROJECT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/gmock_main.dll
                                 $ENV{SystemRoot}/System32/gmock_main.dll SYMBOLIC)
            else()
                file(CREATE_LINK ${PROJECT_BINARY_DIR}/bin/gtest.dll
                                 $ENV{SystemRoot}/System32/gtest.dll SYMBOLIC)
                file(CREATE_LINK ${PROJECT_BINARY_DIR}/bin/gmock.dll
                                 $ENV{SystemRoot}/System32/gmock.dll SYMBOLIC)
                file(CREATE_LINK ${PROJECT_BINARY_DIR}/bin/gtest_main.dll
                                 $ENV{SystemRoot}/System32/gtest_main.dll SYMBOLIC)
                file(CREATE_LINK ${PROJECT_BINARY_DIR}/bin/gmock_main.dll
                                 $ENV{SystemRoot}/System32/gmock_main.dll SYMBOLIC)
            endif()
        endif(BUILD_SHARED_LIBS)

    else(WIN32) # AND UPnPsdk_WITH_GOOGLETEST

        # This caches FETCHCONTENT_BASE_DIR (e.g. ${PROJECT_BINARY_DIR}/_deps)),
        # googletest_SOURCE_DIR, googletest_BINARY_DIR and googletest_POPULATED.
        FetchContent_MakeAvailable(googletest)

        # Due to issue https://github.com/google/googletest/issues/1325#issuecomment-903884914
        # it should only link with the gmock libraries. They contain also gtest.

        message(CHECK_PASS "done")


        ##################################
        # Build default Unit Tests       #
        ##################################

        message(CHECK_START "configuring Unit Tests")
        message(CHECK_PASS "done")

    endif(WIN32)

    # Output with cmake option --log-level=DEBUG
    message(DEBUG "  DEBUG: googletest_SOURCE_DIR is: ${googletest_SOURCE_DIR}")
    message(DEBUG "  DEBUG: googletest_BINARY_DIR is: ${googletest_BINARY_DIR}")

endif(UPnPsdk_WITH_GOOGLETEST)


#################################
# UPnPsdk subdirectories        #
#################################
# This caches <subprojectname>_SOURCE_DIR and <subprojectname_BINARY_DIR
# e.g. PUPNP_UPNP_SOURCE_DIR and PUPNP_UPNP_BINARY_DIR
# For a successful one step configure the order is important.

add_subdirectory(${PROJECT_SOURCE_DIR}/UPnPsdk)
add_subdirectory(${PROJECT_SOURCE_DIR}/Pupnp)
add_subdirectory(${PROJECT_SOURCE_DIR}/Compa)
if (UPnPsdk_WITH_SAMPLES)
    add_subdirectory(${PROJECT_SOURCE_DIR}/Sample)
endif()
add_subdirectory(${PROJECT_SOURCE_DIR}/Apps)
if(googletest_POPULATED)
    add_subdirectory(${PROJECT_SOURCE_DIR}/Utest)
endif()


#################################
# Export cmake variables        #
#################################
# This should be the last statement
#----------------------------------
# to compiler macros (#define ...) for conditions to the program source.
# Hope we have all needed variables defined so we can write cmake_vars.hpp now.
# To meet all variables this should be the last statement.
# Variablename and its value-placeholder must be the same, e.g.
# #cmakedefine VAR1 ${VAR1}
# #cmakedefine VAR1 ${VAR} does not work.
# #cmakedefine VAR2 1  : VAR2 must be exist before even you set a constant.
configure_file(${PROJECT_SOURCE_DIR}/cmake/cmake_vars.hpp.cmake
               ${PROJECT_BINARY_DIR}/include/cmake_vars.hpp
)

# RPATH settings to find shared libraries
# reference: https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/RPATH-handling
# Output with cmake option --log-level=DEBUG
message(DEBUG "\n     DEBUG: RPATH settings\n     ---------------------")
message(DEBUG "  DEBUG: CMAKE_SKIP_RPATH is ................: ${CMAKE_SKIP_RPATH}")
message(DEBUG "  DEBUG: CMAKE_SKIP_INSTALL_RPATH is ........: ${CMAKE_SKIP_INSTALL_RPATH}")
message(DEBUG "  DEBUG: CMAKE_MACOSX_RPATH is ..............: ${CMAKE_MACOSX_RPATH}")
message(DEBUG "  DEBUG: CMAKE_INSTALL_NAME_DIR is ..........: ${CMAKE_INSTALL_NAME_DIR}")
message(DEBUG "  DEBUG: CMAKE_INSTALL_RPATH is .............: ${CMAKE_INSTALL_RPATH}")
message(DEBUG "  DEBUG: CMAKE_SKIP_BUILD_RPATH is ..........: ${CMAKE_SKIP_BUILD_RPATH}")
message(DEBUG "  DEBUG: CMAKE_BUILD_WITH_INSTALL_RPATH is ..: ${CMAKE_BUILD_WITH_INSTALL_RPATH}")
message(DEBUG "  DEBUG: CMAKE_INSTALL_RPATH_USE_LINK_PATH is: ${CMAKE_INSTALL_RPATH_USE_LINK_PATH}")

message(STATUS "PUPNP_VERSION set to \"${PUPNP_VERSION}\"")
message(STATUS "CMAKE_PROJECT_VERSION set to \"${CMAKE_PROJECT_VERSION}\"")
message(STATUS "Build type is ${CMAKE_BUILD_TYPE}")


#################################
# Configure and build tests     #
#################################
# To test builds with CTest using different options you can find an example at
# git commit 533548e97765d6eafeafd4a478a48e8ec097f713. These tests have been
# moved to github workflow.
