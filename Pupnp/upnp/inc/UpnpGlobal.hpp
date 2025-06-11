#ifndef PUPNP_UPNPGLOBAL_HPP
#define PUPNP_UPNPGLOBAL_HPP
// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-12
// Taken from authors who haven't made a note.

/*!
 * \file
 *
 * \brief Defines constants that for some reason are not defined on some
 * systems.
 */
#include <stddef.h>

#if defined UPNP_LARGEFILE_SENSITIVE && _FILE_OFFSET_BITS + 0 != 64
#if defined __GNUC__
#warning libupnp requires largefile mode - use AC_SYS_LARGEFILE
#elif !defined _WIN32
#error libupnp requires largefile mode - use AC_SYS_LARGEFILE
#endif
#endif

// With C++ the inline statement belongs to the standard. There are no
// differences on different platforms anymore.
#define UPNP_INLINE inline

// \brief Supply the PRIz* printf() macros.
#ifdef _MSC_VER
// define some things the M$ VC++ doesn't know
typedef __int64 int64_t;
#endif
#define PRIzd "zd"
#define PRIzu "zu"
#define PRIzx "zx"

/*
 * Defining this macro here gives some interesting information about unused
 * functions in the code. Of course, this should never go uncommented on a
 * release.
 */
/*#define inline*/

//
// clang-format off
//
// C++ visibility support
//-----------------------
// Reference: https://gcc.gnu.org/wiki/Visibility
// Generic helper definitions for shared library support
#if defined _WIN32 || defined __CYGWIN__
  #define UPNP_HELPER_DLL_IMPORT __declspec(dllimport)
  #define UPNP_HELPER_DLL_EXPORT __declspec(dllexport)
  #define UPNP_HELPER_DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define UPNP_HELPER_DLL_IMPORT __attribute__ ((visibility ("default")))
    #define UPNP_HELPER_DLL_EXPORT __attribute__ ((visibility ("default")))
    #define UPNP_HELPER_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define UPNP_HELPER_DLL_IMPORT
    #define UPNP_HELPER_DLL_EXPORT
    #define UPNP_HELPER_DLL_LOCAL
  #endif
#endif

// Now we use the generic helper definitions above to define EXPORT_SPEC and
// UPnPsdk_LOCAL. EXPORT_SPEC is used for the public visible symbols. It either
// DLL imports or DLL exports (or does nothing for static build) UPnPsdk_LOCAL
// is used for non-api symbols.

#ifdef UPnPsdk_SHARE // defined if UPnPsdk is compiled as a shared library
  #ifdef UPnPsdk_EXPORTS // defined if we are building the UPnPsdk DLL (instead of using it)
    #define EXPORT_SPEC UPNP_HELPER_DLL_EXPORT
  #else
    #define EXPORT_SPEC UPNP_HELPER_DLL_IMPORT
  #endif // UPnPsdk_EXPORTS
  #define UPnPsdk_LOCAL UPNP_HELPER_DLL_LOCAL
#else // UPnPsdk_SHARE is not defined: this means UPnPsdk is a static lib.
  #define EXPORT_SPEC
  #define UPnPsdk_LOCAL
#endif // UPnPsdk_SHARE

/* EXPORT_SPEC: old visibility label in pUPnP from the fork. It does not clearly
                specify an API member. */

// clang-format on

#endif /* PUPNP_UPNPGLOBAL_HPP */
