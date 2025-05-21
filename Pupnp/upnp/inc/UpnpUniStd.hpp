// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2022-02-27
// Taken from authors who haven't made a note.
// Last compare with pupnp original source file on 2025-05-22, ver 1.14.20

#ifndef UPNPLIB_UPNPUNISTD_HPP
#define UPNPLIB_UPNPUNISTD_HPP

#ifdef _WIN32
/* Do not #include <unistd.h> on WIN32. */
#else               /* _WIN32 */
#include <unistd.h> /* for close() */
#endif              /* _WIN32 */

#endif              /* UPNPLIB_UPNPUNISTD_HPP */
