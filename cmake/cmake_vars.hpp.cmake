#ifndef UPNPLIB_CMAKE_VARS_HPP
#define UPNPLIB_CMAKE_VARS_HPP
// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2022-07-15

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
// Path to the build directory of the project
#cmakedefine UPnPlib_BINARY_DIR "${UPnPlib_BINARY_DIR}"
// Path to sample source directory to access web subdirectory
#cmakedefine UPNPLIB_SAMPLE_SOURCE_DIR "${UPNPLIB_SAMPLE_SOURCE_DIR}"

/***************************************************************************
 * Library version
 ***************************************************************************/
// TODO: Check the version handling
/** The library version (string) e.g. "1.3.0" */
#cmakedefine UPNP_VERSION_STRING "${UPNP_VERSION_STRING}"
/** Major version of the library */
#cmakedefine UPNP_VERSION_MAJOR ${UPNP_VERSION_MAJOR}
/** Minor version of the library */
#define UPNP_VERSION_MINOR 0
/** Patch version of the library */
#define UPNP_VERSION_PATCH 0
/** The library version (numeric) e.g. 10300 means version 1.3.0 */
#define UPNP_VERSION \
((UPNP_VERSION_MAJOR * 10000 + UPNP_VERSION_MINOR) * 100 + \
UPNP_VERSION_PATCH)

/***************************************************************************
 * UPnPlib configuration settings
 ***************************************************************************/
/* Large file support
 * whether the system defaults to 32bit off_t but can do 64bit when requested
 * warning libupnp requires largefile mode - use AC_SYS_LARGEFILE */
#cmakedefine UPNP_LARGEFILE_SENSITIVE

/***************************************************************************
 * Other settings
 ***************************************************************************/
// Defined to ON if the library will use the static pthreads4W library
#cmakedefine PTW32_STATIC_LIB ${PTW32_STATIC_LIB}

#endif // UPNPLIB_CMAKE_VARS_HPP
