#ifndef UPNPLIB_CMAKE_VARS_HPP
#define UPNPLIB_CMAKE_VARS_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-09-08
/*!
 * \file
 * \brief Defines symbols for the compiler that are provided by CMake.
 * \cond
 * It isn't documented so far.
 */

/***************************************************************************
 * CMake configuration settings
 ***************************************************************************/
#cmakedefine CMAKE_VERSION "${CMAKE_VERSION}"
#cmakedefine CMAKE_CXX_COMPILER "${CMAKE_CXX_COMPILER}"
#cmakedefine CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}"
#cmakedefine CMAKE_CXX_COMPILER_VERSION "${CMAKE_CXX_COMPILER_VERSION}"
#cmakedefine CMAKE_GENERATOR "${CMAKE_GENERATOR}"

/***************************************************************************
 * Needed paths of the project
 ***************************************************************************/
// Path to the project directory and its length
#cmakedefine CMAKE_SOURCE_DIR "${CMAKE_SOURCE_DIR}"
#cmakedefine UPnPsdk_PROJECT_PATH_LENGTH ${UPnPsdk_PROJECT_PATH_LENGTH}
// Path to the build directory of the project
#cmakedefine CMAKE_BINARY_DIR "${CMAKE_BINARY_DIR}"
// Path to sample source directory to access web subdirectory
#cmakedefine SAMPLE_SOURCE_DIR "${SAMPLE_SOURCE_DIR}"

/***************************************************************************
 * Library version
 ***************************************************************************/
/** The pUPnP library version the fork is based on, e.g. "1.14.19" */
#cmakedefine PUPNP_VERSION_STRING "${PUPNP_VERSION_STRING}"

/***************************************************************************
 * UPNPLIB_PROJECT configuration settings
 ***************************************************************************/
/* Large file support
 * whether the system defaults to 32bit off_t but can do 64bit for off_t with
 * compile option _FILE_OFFSET_BITS=64 */
#cmakedefine UPNP_LARGEFILE_SENSITIVE ${UPNP_LARGEFILE_SENSITIVE}

/***************************************************************************
 * Other settings
 ***************************************************************************/
// Defined to ON if the library will use the static pthreads4W library
#cmakedefine PTW32_STATIC_LIB ${PTW32_STATIC_LIB}

/// \endcond
#endif // UPNPLIB_CMAKE_VARS_HPP
// vim: syntax=cpp
