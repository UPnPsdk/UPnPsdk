#ifndef UPNPLIB_UPNPUNISTD_HPP
#define UPNPLIB_UPNPUNISTD_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-01-26
// Taken from authors who haven't made a note.
/*!
 * \file
 * \brief Do not \#include <unistd.h> on WIN32
 */

#ifdef _WIN32
/* Do not #include <unistd.h> on WIN32. */
#else               /* _WIN32 */
#include <unistd.h> /* for close() */
#endif              /* _WIN32 */

#endif /* UPNPLIB_UPNPUNISTD_HPP */
