#ifndef UPnPsdk_INCLUDE_VISIBILITY_HPP
#define UPnPsdk_INCLUDE_VISIBILITY_HPP
// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-12
/*!
 * \file
 * \brief Macros to support visibility of external symbols.
 *
 * To help finding errors you can list all exported symbols\n
 * on Linux with:\n
 * nm -C -D \<library\>,\n
 * on Microsoft Windows with e.g.:\n
 * dumpbin.exe /EXPORTS .\\build\\lib\\Release\\UPnPsdk_shared.lib
 *
 * Reference: https://gcc.gnu.org/wiki/Visibility
 */

/*! \def UPnPsdk_VIS
 * \brief Prefix to export symbol for external use. */
/*! \def UPnPsdk_LOCAL
 * \brief Prefix to NOT export symbol of a local method for external use. */
/*! \def UPnPsdk_EXTERN
 * \brief Prefix for a portable 'extern' declaration. */


// C++ visibility support
//-----------------------
// clang-format off
/// \cond
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
/// \endcond

// Now we use the generic helper definitions above to define UPnPsdk_VIS and
// UPnPsdk_LOCAL. UPnPsdk_VIS is used for the public visible symbols. It either
// DLL imports or DLL exports (or does nothing for static build) UPnPsdk_LOCAL
// is used for non-api symbols.

#ifdef UPnPsdk_SHARE // defined if UPnPsdk is compiled as a shared library
  #ifdef UPnPsdk_EXPORTS // defined if we are building the UPnPsdk DLL (instead of using it)
    #define UPnPsdk_VIS UPNP_HELPER_DLL_EXPORT
  #else
    #define UPnPsdk_VIS UPNP_HELPER_DLL_IMPORT
  #endif // UPnPsdk_EXPORTS
  #define UPnPsdk_LOCAL UPNP_HELPER_DLL_LOCAL
#else // UPnPsdk_SHARE is not defined: this means UPnPsdk is a static lib.
  #define UPnPsdk_VIS
  #define UPnPsdk_LOCAL
#endif // UPnPsdk_SHARE

#if (defined _WIN32 || defined __CYGWIN__) && defined UPnPsdk_SHARE
  #define UPnPsdk_EXTERN __declspec(dllimport) extern
#else
  #define UPnPsdk_EXTERN UPnPsdk_VIS extern
#endif

/// \cond
/* UPnPsdk_VIS: symbol (function, class, struct) is not a member of the API. The
                symbol is visible for internal use only. */

/* UPnPsdk_API: symbol (function, class, struct) belongs to the UPnPsdk class
                library and is tested. */
#define UPnPsdk_API UPnPsdk_VIS

/* PUPNP_API:   symbol (function, class, struct) belongs to the old pUPnP
                library, is emulated for backward compatibility and tested. */
#define PUPNP_API UPnPsdk_VIS

/* PUPNP_Api:   symbol (function, class, struct) belongs to the old pUPnP
                library but is not tested so far. */
#define PUPNP_Api UPnPsdk_VIS

/* EXPORT_SPEC: old visibility label in pUPnP from the fork. It does not clearly
                specify an API member. */

/// \endcond

// clang-format on

#endif // UPnPsdk_INCLUDE_VISIBILITY_HPP
