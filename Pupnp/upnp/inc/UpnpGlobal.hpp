#ifndef PUPNP_UPNPGLOBAL_HPP
#define PUPNP_UPNPGLOBAL_HPP
// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-10-26
// Taken from authors who haven't made a note.

/*!
 * \file
 *
 * \brief Defines constants that for some reason are not defined on some
 * systems.
 */
#include <cstddef>

#include <cmake_vars.hpp>
#if defined UPNP_LARGEFILE_SENSITIVE && _FILE_OFFSET_BITS + 0 != 64
#if defined __GNUC__
#warning libupnp requires largefile mode - use AC_SYS_LARGEFILE
#elif !defined _WIN32
#error libupnp requires largefile mode - use AC_SYS_LARGEFILE
#endif
#endif

#if 0
// BUG! must be
// #ifndef _WIN32 --Ingo
#ifdef _WIN32
/*
 * EXPORT_SPEC
 */
// #if defined _MSC_VER || defined __BORLANDC__
// #ifdef UPNP_STATIC_LIB
// #define EXPORT_SPEC
// #else /* UPNP_STATIC_LIB */
// #ifdef LIBUPNP_EXPORTS
// /*! set up declspec for dll export to make
//  * functions visible to library users */
// #define EXPORT_SPEC __declspec(dllexport)
// #else /* LIBUPNP_EXPORTS */
// #define EXPORT_SPEC __declspec(dllimport)
// #endif /* LIBUPNP_EXPORTS */
// #endif /* UPNP_STATIC_LIB */
// #else  /* _MSC_VER || __BORLANDC__ */
// #define EXPORT_SPEC
// #endif /* _MSC_VER || __BORLANDC__ */

/*
 * UPNP_INLINE
 * PRId64
 * PRIzd
 * PRIzu
 * PRIzx
 */
// clang-format off
#ifdef UPNP_USE_MSVCPP
  #if _MSC_VER > 1900
    #define UPNP_INLINE inline
    #define PRIzd "zd"
    #define PRIzu "zu"
    #define PRIzx "zx"
  #else
    /* define some things the M$ VC++ doesn't know */
    #define UPNP_INLINE _inline
    typedef __int64 int64_t;
    #define PRIzd "ld"
    #define PRIzu "lu"
    #define PRIzx "lx"
  #endif
#endif /* UPNP_USE_MSVCPP */
// clang-format on

#ifdef UPNP_USE_BCBPP
/* define some things Borland Builder doesn't know */
/* inconsistency between the httpparser.h and the .c file
   definition. Header is missing UPNP_INLINE prefix, so compiler
   is confused ... better remove it #define UPNP_INLINE inline
 */
#define UPNP_INLINE
typedef __int64 int64_t;
#warning The Borland C compiler is probably broken on PRId64,
#warning please someone provide a proper fix here
#define PRId64 "Ld"
#define PRIzd "ld"
#define PRIzu "lu"
#define PRIzx "lx"
#define SCNd64 "Ld"
#endif /* UPNP_USE_BCBPP */

#ifdef __GNUC__
#define UPNP_INLINE inline
/* Note with PRIzu that in the case of Mingw32, it's the MS C
 * runtime printf which ends up getting called, not the glibc
 * printf, so it genuinely doesn't have "zu"
 */
#define PRIzd "ld"
#define PRIzu "lu"
#define PRIzx "lx"
#endif /* __GNUC__ */
#else
/*!
 * \brief Export functions on WIN32 DLLs.
 *
 * Every funtion that belongs to the library API must use this
 * definition upon declaration or it will not be exported on WIN32
 * DLLs.
 */
// #define EXPORT_SPEC

/*!
 * \brief Declares an inline function.
 *
 * Surprisingly, there are some compilers that do not understand the
 * inline keyword. This definition makes the use of this keyword
 * portable to these systems.
 */
#ifdef __STRICT_ANSI__
#define UPNP_INLINE __inline__
#else
#define UPNP_INLINE inline
#endif

/*!
 * \brief Supply the PRId64 printf() macro.
 *
 * MSVC still does not know about this.
 */
/* #define PRId64 PRId64 */

/*!
 * \brief Supply the PRIz* printf() macros.
 *
 * These macros were invented so that we can live a little longer with
 * MSVC lack of C99. "z" is the correct printf() size specifier for
 * the size_t type.
 */
#define PRIzd "zd"
#define PRIzu "zu"
#define PRIzx "zx"
#endif
#endif // #if 0

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

// Now we use the generic helper definitions above to define UPnPsdk_API and
// UPnPsdk_LOCAL. UPnPsdk_API is used for the public API symbols. It either DLL imports
// or DLL exports (or does nothing for static build) UPnPsdk_LOCAL is used for
// non-api symbols.

#ifdef UPnPsdk_SHARE // defined if UPnPsdk is compiled as a shared library
  #ifdef UPnPsdk_EXPORTS // defined if we are building the UPnPsdk DLL (instead of using it)
    #define UPnPsdk_API UPNP_HELPER_DLL_EXPORT
  #else
    #define UPnPsdk_API UPNP_HELPER_DLL_IMPORT
  #endif // UPnPsdk_EXPORTS
  #define UPnPsdk_LOCAL UPNP_HELPER_DLL_LOCAL
#else // UPnPsdk_SHARE is not defined: this means UPnPsdk is a static lib.
  #define UPnPsdk_API
  #define UPnPsdk_LOCAL
#endif // UPnPsdk_SHARE

#if (defined _WIN32 || defined __CYGWIN__) && defined UPnPsdk_SHARE
  #define UPnPsdk_EXTERN __declspec(dllimport) extern
#else
  #define UPnPsdk_EXTERN UPnPsdk_API extern
#endif

// Switch old pupnp definition to use new visibility support
#define EXPORT_SPEC UPnPsdk_API
#define EXPORT_SPEC_LOCAL UPnPsdk_LOCAL
#define EXPORT_SPEC_EXTERN UPnPsdk_EXTERN

// clang-format on

#endif /* PUPNP_UPNPGLOBAL_HPP */
