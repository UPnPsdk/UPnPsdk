#ifndef COMPA_UPNPGLOBAL_HPP
#define COMPA_UPNPGLOBAL_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-09-08
// Taken from authors who haven't made a note.

// Include file for backward compatibility with old ixml code.

#include <UPnPsdk/visibility.hpp>

// With C++ the inline statement belongs to the standard. There are no
// differences on different platforms anymore.
#define UPNP_INLINE inline

// Switch old pupnp definition to use new visibility support.
#define EXPORT_SPEC UPNPLIB_API
#define EXPORT_SPEC_LOCAL UPNPLIB_LOCAL
#define EXPORT_SPEC_EXTERN UPNPLIB_EXTERN

// clang-format off
// Some different format specifications for printf() and friends
// -------------------------------------------------------------
#ifdef _MSC_VER
/* \brief Supply the PRIz* printf() macros.
 *
 * These macros were invented so that we can live a little longer with
 * MSVC lack of C99. "z" is the correct printf() size specifier for
 * the size_t type. */
  #define PRIzd "zd"
  #define PRIzu "zu"
  #define PRIzx "zx"
#else
/* Note with PRIzu that in the case of Mingw32, it's the MS C
 * runtime printf which ends up getting called, not the glibc
 * printf, so it genuinely doesn't have "zu"
 */
  #define PRIzd "ld"
  #define PRIzu "lu"
  #define PRIzx "lx"
#endif
// clang-format on

#endif /* COMPA_UPNPGLOBAL_HPP */
