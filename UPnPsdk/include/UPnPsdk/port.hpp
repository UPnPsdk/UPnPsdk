#ifndef UPnPsdk_INCLUDE_PORT_HPP
#define UPnPsdk_INCLUDE_PORT_HPP
// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-02-21
/*!
 * \file
 * \brief Specifications to be portable between different platforms.
 * \cond
 * It isn't documented so far.
 */

// Header file for portable definitions
// ====================================
// This header should be includable into any source file to have portable
// definitions available. It should not build any inline code.

// clang-format off

// Check Debug settings. Exlusive NDEBUG or DEBUG must be set.
// -----------------------------------------------------------
#if defined(NDEBUG) && defined(DEBUG)
  #error "NDEBUG and DEBUG are defined. Only one is possible."
#endif
#if !defined(NDEBUG) && !defined(DEBUG)
  #error "Neither NDEBUG nor DEBUG is definded."
#endif

// Check that we have the correct bit size for Large-file support when this
// header is included into an application that uses the SDK
// ------------------------------------------------------------------------
#ifdef UPNP_LARGEFILE_SENSITIVE
#include <climits>
static_assert(sizeof(off_t) * CHAR_BIT == 64,
              "UPnPsdk has Large-file support on 32 bit architectures. "
              "Application MUST provide LFS.");
#endif

// Header file for portable <unistd.h>
// -----------------------------------
// On MS Windows <unistd.h> isn't available. We can use <io.h> instead for most
// functions but it's not 100% compatible.
#ifdef _MSC_VER
  #include <io.h>
  #define STDIN_FILENO 0
  #define STDOUT_FILENO 1
  #define STDERR_FILENO 2
#else
  #include <unistd.h>
#endif

// Make size_t and ssize_t portable
// --------------------------------
// no ssize_t defined for VC but SSIZE_T
#ifdef _MSC_VER
  #include <BaseTsd.h> // for SSIZE_T
   #define ssize_t SSIZE_T
  // Needed for some uncompatible arguments on MS Windows.
  #define SIZEP_T int
  #define SSIZEP_T int
#else
  #define SIZEP_T size_t
  #define SSIZEP_T ssize_t
#endif

/*!
 * \brief Declares an inline function.
 *
 * Surprisingly, there are some compilers that do not understand the
 * inline keyword. This definition makes the use of this keyword
 * portable to these systems.
 */
#ifdef __STRICT_ANSI__
  #define UPnPsdk_INLINE __inline__
#else
  #define UPnPsdk_INLINE inline
#endif

#ifdef _MSC_VER
  // POSIX names for functions
  #define strcasecmp _stricmp
  #define strncasecmp strnicmp
#endif

#if (!defined(_MSC_VER) && !defined(IN6_IS_ADDR_GLOBAL)) || defined(DOXYGEN_RUN)
// On win32 this is a function and will not be detected by macro conditional
// "if defined". For detailed information look at Unit Test
// 'AddrinfoTestSuite.check_in6_is_addr_global'.
/// \brief If IN6_IS_ADDR_GLOBAL is not available then this is defined.
#define IN6_IS_ADDR_GLOBAL(a)                                                  \
    ((((__const uint32_t*)(a))[0] & htonl((uint32_t)0xe0000000)) ==            \
     htonl((uint32_t)0x20000000))
#endif

// Macros to disable and enable compiler warnings
// ----------------------------------------------
// Warning 4251: 'type' : class 'type1' needs to have dll-interface to be used
// by clients of class 'type2'.
// This can be ignored for classes from the C++ STL (best if it is private).
#ifdef _MSC_VER
  #define SUPPRESS_MSVC_WARN_4251_NEXT_LINE \
    _Pragma("warning(suppress: 4251)")
#else
  #define SUPPRESS_MSVC_WARN_4251_NEXT_LINE
#endif

#ifdef _MSC_VER
  #define DISABLE_MSVC_WARN_4251 \
    _Pragma("warning(push)") \
    _Pragma("warning(disable: 4251)")
#else
  #define DISABLE_MSVC_WARN_4251
#endif

// Warning 4273: 'function' : inconsistent DLL linkage.
// This is expected on propagate global variables with included header file
// (__declspec(dllimport)) in its source file (__declspec(dllexport)).
#ifdef _MSC_VER
  #define SUPPRESS_MSVC_WARN_4273_NEXT_LINE \
    _Pragma("warning(suppress: 4273)")
#else
  #define SUPPRESS_MSVC_WARN_4273_NEXT_LINE
#endif

#ifdef _MSC_VER
  #define DISABLE_MSVC_WARN_4273 \
    _Pragma("warning(push)") \
    _Pragma("warning(disable: 4273)")
#else
  #define DISABLE_MSVC_WARN_4273
#endif

#ifdef _MSC_VER
  #define ENABLE_MSVC_WARN \
    _Pragma("warning(pop)")
#else
  #define ENABLE_MSVC_WARN
#endif

// This has been taken from removed Compa/src/inc/upnputil.hpp
/* C specific */
/* VC needs these in C++ mode too (do other compilers?) */
#if !defined(__cplusplus) || defined(UPNP_USE_MSVCPP)
#ifdef _WIN32
#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif
#define sleep(a) Sleep((a) * 1000)
#define usleep(a) Sleep((a) / 1000)
#define strerror_r(a, b, c) (strerror_s((b), (c), (a)))
#endif /* _WIN32 */
#endif /* !defined(__cplusplus) || defined(UPNP_USE_MSVCPP) */

// clang-format on

/// \endcond
#endif // UPnPsdk_INCLUDE_PORT_HPP
