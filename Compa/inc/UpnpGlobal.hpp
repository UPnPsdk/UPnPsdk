#ifndef COMPA_UPNPGLOBAL_HPP
#define COMPA_UPNPGLOBAL_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-05-29
// Taken from authors who haven't made a note.

// Include file for backward compatibility with old ixml code.

#include <UPnPsdk/visibility.hpp>

// Switch old pupnp definition to use new visibility support.
#define EXPORT_SPEC UPnPsdk_EXP

// \brief Supply the PRIz* printf() macros.
#ifdef _MSC_VER
// define some things the M$ VC++ doesn't know
typedef __int64 int64_t;
#endif

#endif /* COMPA_UPNPGLOBAL_HPP */
