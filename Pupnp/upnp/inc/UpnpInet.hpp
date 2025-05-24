// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-08-01
// Taken from authors who haven't made a note.
// Last compare with pupnp original source file on 2025-05-23, ver 1.14.20

#ifndef PUPNP_UPNPINET_HPP
#define PUPNP_UPNPINET_HPP

/*!
 * \addtogroup Sock
 *
 * @{
 *
 * \file
 *
 * \brief Provides a platform independent way to include TCP/IP types and
 * functions.
 */

#include "UpnpUniStd.hpp" /* for close() */
#include "umock/unistd.hpp"

#ifdef _WIN32
#include <stdarg.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define UpnpCloseSocket umock::unistd_h.closesocket

#if (_WIN32_WINNT < 0x0600)
typedef short sa_family_t;
#else
typedef ADDRESS_FAMILY sa_family_t;
#endif

#else /* _WIN32 */
#include <sys/param.h>
#if defined(__sun)
// #include <fcntl.h>
// #include <sys/sockio.h>
#elif (defined(BSD) && BSD >= 199306) || defined(__FreeBSD_kernel__)
// #include <ifaddrs.h>
/* Do not move or remove the include below for "sys/socket"!
 * Will break FreeBSD builds. */
// #include <sys/socket.h>
#endif
#include <arpa/inet.h> /* for inet_pton() */
#include <net/if.h>
// #include <netinet/in.h>

/*! This typedef makes the code slightly more WIN32 tolerant.
 * On WIN32 systems, SOCKET is unsigned and is not a file
 * descriptor. */
typedef int SOCKET;

/*! INVALID_SOCKET is unsigned on win32. */
#define INVALID_SOCKET (-1)

/*! select() returns SOCKET_ERROR on win32. */
#define SOCKET_ERROR (-1)

/*! Alias to close() to make code more WIN32 tolerant. */
#define UpnpCloseSocket umock::unistd_h.close
#endif /* _WIN32 */

/* @} Sock */

#endif /* PUPNP_UPNPINET_HPP */
