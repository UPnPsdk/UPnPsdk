// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-05-30

// There is no include guard '#ifndef ...' because this file shouln't be
// included more than two times as given.

/*!
 * \file
 * \brief Global used constants, variables, functions and macros.
 */

// Due to the global nature of this header file additional #include statements
// should be taken with great care. They are included in nearly all other
// compile units.
#include <UPnPsdk/visibility.hpp>
/// \cond


// strndup() is a GNU extension.
// -----------------------------
#ifndef HAVE_STRNDUP
UPnPsdk_API char* strndup(const char* __string, size_t __n);
#endif


// Define missing and redefine buggy macros from system header 'netinet/in.h'.
// ---------------------------------------------------------------------------
#if 0
#ifndef _MSC_VER
// On win32 this is a function and will not be detected by macro conditional
// "if defined". For detailed information look at Unit Tests
// 'SockaddrTestSuite.check_in6_is_addr_*'.
#ifdef IN6_IS_ADDR_GLOBAL
#undef IN6_IS_ADDR_GLOBAL
#endif
#define IN6_IS_ADDR_GLOBAL(a)                                                  \
    ((((__const uint32_t*)(a))[0] & htonl((uint32_t)0xe0000000)) ==            \
     htonl((uint32_t)0x20000000))
#endif

// Redefine buggy macro: original accepts link-local address with subnet,
// e.g. "[fe80:1::]". This is out of specification.
#ifdef IN6_IS_ADDR_LINKLOCAL
#undef IN6_IS_ADDR_LINKLOCAL
#endif
#define IN6_IS_ADDR_LINKLOCAL(a)                                               \
    ((((__const uint32_t*)(a))[0] == htonl((uint32_t)0xfe800000)) &&           \
     (((__const uint32_t*)(a))[1] == htonl((uint32_t)0x00000000)))
#endif


namespace UPnPsdk {

// Global constants
// ================
// Default response timeout for UPnP messages as given by The UPnP™ Device
// Architecture 2.0, Document Revision Date: April 17, 2020.
inline constexpr int g_response_timeout{30};

// Info message about the library
// This is not the right place because I need to #include <cmake_vars.hpp>.
// Needs rework.
// inline constexpr std::string_view UPnPsdk_version{UPnPsdk_VERSION};
// inline constexpr std::string_view PUPNP_version{PUPNP_VERSION};

} // namespace UPnPsdk
/// \endcond
