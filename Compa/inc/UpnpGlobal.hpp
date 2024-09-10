#ifndef COMPA_UPNPGLOBAL_HPP
#define COMPA_UPNPGLOBAL_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-09-11
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
// \brief Supply the PRIz* printf() macros.
#ifdef _MSC_VER
/* define some things the M$ VC++ doesn't know */
  typedef __int64 int64_t;
  #define PRIzd "ld"
  #define PRIzu "lu"
  #define PRIzx "lx"
#else
  #define PRIzd "zd"
  #define PRIzu "zu"
  #define PRIzx "zx"
#endif
// clang-format on

#endif /* COMPA_UPNPGLOBAL_HPP */
