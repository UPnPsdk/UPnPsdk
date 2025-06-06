/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (C) 2021 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
 * Redistribution only with this Copyright remark. Last modified: 2025-05-16
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * - Neither name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

#ifndef UPNPLIB_GENLIB_NET_URI_HPP
#define UPNPLIB_GENLIB_NET_URI_HPP

/*!
 * \file
 */

#if !defined(_WIN32)
// #include <sys/param.h>
#endif

#include "UpnpGlobal.hpp" /* for EXPORT_SPEC */
// #include "UpnpInet.hpp"

#include <ctype.h>
// #include <errno.h>
// #include <fcntl.h>
// #include <stdlib.h>
#include <string.h>
// #include <sys/types.h>
// #include <time.h>

#ifdef _WIN32
#if !defined(UPNP_USE_MSVCPP) && !defined(UPNP_USE_BCBPP)
/* VC Winsocks2 includes these functions */
// #include "inet_pton.h"
#endif
#else
#include <netdb.h> /* for struct addrinfo */
#endif

#ifdef _WIN32
#define strncasecmp strnicmp
#else
/* Other systems have strncasecmp */
#endif

#define MARK "-_.!~*'()"

/*! added {} for compatibility */
#define RESERVED ";/?:@&=+$,{}"

#define HTTP_SUCCESS 1

enum hostType { HOSTNAME, IPv4address };

enum pathType { ABS_PATH, REL_PATH, OPAQUE_PART };

// Must not use ABSOLUTE, RELATIVE; already defined in Win32 for other meaning.
enum uriType { Absolute, Relative };

/*!
 * \brief Buffer used in parsinghttp messages, urls, etc. generally this simply
 * holds a pointer into a larger array.
 */
typedef struct TOKEN {
    const char* buff;
    size_t size;
} token;

/*!
 * \brief Represents a host port: e.g. "127.127.0.1:80" text is a token
 * pointing to the full string representation.
 */
typedef struct HOSTPORT {
    /*! Full host port. */
    token text;
    /* Network Byte Order */
    struct sockaddr_storage IPaddress;
} hostport_type;

/*!
 * \brief Represents a URI used in parse_uri and elsewhere
 */
typedef struct URI {
    enum uriType type;
    token scheme;
    enum pathType path_type;
    token pathquery;
    token fragment;
    hostport_type hostport;
} uri_type;

/*!
 * \brief Represents a list of URLs as in the "callback" header of SUBSCRIBE
 * message in GENA. "char *" URLs holds dynamic memory.
 */
typedef struct URL_LIST {
    /*! */
    size_t size;
    /*! All the urls, delimited by <> */
    char* URLs;
    /*! */
    uri_type* parsedURLs;
} URL_list;

/*!
 * \brief Replaces one single escaped character within a string with its
 * unescaped version as in http://www.ietf.org/rfc/rfc2396.txt (RFC explaining
 * URIs). The index must exactly point to the '%' character, otherwise the
 * function will return unsuccessful.
 *
 * Size of array is NOT checked (MUST be checked by caller).
 *
 * \note This function modifies the string and the max size. If the sequence is
 * an escaped sequence it is replaced, the other characters in the string are
 * shifted over, and NULL characters are placed at the end of the string.
 *
 * \return 1 if an escaped character was converted, otherwise return 0.
 */
EXPORT_SPEC int replace_escaped(
    /*! [in,out] String of characters. */
    char* in,
    /*! [in] Index at which to start checking the characters. */
    size_t index,
    /*! [in,out] Maximal size of the string buffer will be reduced by 2 if a
       character is converted. */
    size_t* max);

/*!
 * \brief Copies one URL_list into another.
 *
 * This includes dynamically allocating the out->URLs field (the full string),
 * and the structures used to hold the parsedURLs. This memory MUST be freed
 * by the caller through: free_URL_list(&out).
 *
 * \return
 * 	\li HTTP_SUCCESS - On Success.
 * 	\li UPNP_E_OUTOF_MEMORY - On Failure to allocate memory.
 */
EXPORT_SPEC int copy_URL_list(
    /*! [in] Source URL list. */
    URL_list* in,
    /*! [out] Destination URL list. */
    URL_list* out);

/*!
 * \brief Frees the memory associated with a URL_list.
 *
 * Frees the dynamically allocated members of of list. Does NOT free the
 * pointer to the list itself ( i.e. does NOT free(list)).
 */
EXPORT_SPEC void free_URL_list(
    /*! [in] URL list object. */
    URL_list* list);

/*!
 * \brief Function useful in debugging for printing a parsed uri.
 */
#ifdef DEBUG
void print_uri(
    /*! [in] URI object to print. */
    uri_type* in);
#else
#define print_uri(in)                                                          \
    do {                                                                       \
    } while (0)
#endif

/*!
 * \brief Function useful in debugging for printing a token.
 */
#ifdef DEBUG
void print_token(
    /*! [in] Token object to print. */
    token* in);
#else
#define print_token(in)                                                        \
    do {                                                                       \
    } while (0)
#endif

/*!
 * \brief Compares buffer in the token object with the buffer in in2 case
 * insensitive.
 *
 * \return
 * 	\li < 0, if string1 is less than string2.
 * 	\li == 0, if string1 is identical to string2 .
 * 	\li > 0, if string1 is greater than string2.
 */
EXPORT_SPEC int token_string_casecmp(
    /*! [in] Token object whose buffer is to be compared. */
    token* in1,
    /*! [in] String of characters to compare with. */
    const char* in2);

/*!
 * \brief Compares two tokens.
 *
 * \return
 * 	\li < 0, if string1 is less than string2.
 * 	\li == 0, if string1 is identical to string2 .
 * 	\li > 0, if string1 is greater than string2.
 */
EXPORT_SPEC int token_cmp(
    /*! [in] First token object whose buffer is to be compared. */
    token* in1,
    /*! [in] Second token object used for the comparison. */
    token* in2);

/*!
 * \brief Removes http escaped characters such as: "%20" and replaces them with
 * their character representation. i.e. "hello%20foo" -> "hello foo".
 *
 * The input IS MODIFIED in place (shortened). Extra characters are replaced
 * with \b NULL.
 *
 * \return UPNP_E_SUCCESS.
 */
EXPORT_SPEC int remove_escaped_chars(
    /*! [in,out] String of characters to be modified. */
    char* in,
    /*! [in,out] Size limit for the number of characters. */
    size_t* size);

/*!
 * \brief Removes ".", and ".." from a path.
 *
 * If a ".." can not be resolved (i.e. the .. would go past the root of the
 * path) an error is returned.
 *
 * The input IS modified in place.)
 *
 * \note Examples
 * 	char path[30]="/../hello";
 * 	remove_dots(path, strlen(path)) -> UPNP_E_INVALID_URL
 * 	char path[30]="/./hello";
 * 	remove_dots(path, strlen(path)) -> UPNP_E_SUCCESS,
 * 	in = "/hello"
 * 	char path[30]="/./hello/foo/../goodbye" ->
 * 	UPNP_E_SUCCESS, in = "/hello/goodbye"
 *
 * \return
 * 	\li UPNP_E_SUCCESS - On Success.
 * 	\li UPNP_E_OUTOF_MEMORY - On failure to allocate memory.
 * 	\li UPNP_E_INVALID_URL - Failure to resolve URL.
 */
EXPORT_SPEC int remove_dots(
    /*! [in] String of characters from which "dots" have to be removed. */
    char* in,
    /*! [in] Size limit for the number of characters. */
    size_t size);

/*!
 * \brief resolves a relative url with a base url returning a NEW (dynamically
 * allocated with malloc) full url.
 *
 * If the base_url is \b NULL, then a copy of the  rel_url is passed back if
 * the rel_url is absolute then a copy of the rel_url is passed back if neither
 * the base nor the rel_url are Absolute then NULL is returned. Otherwise it
 * tries and resolves the relative url with the base as described in
 * http://www.ietf.org/rfc/rfc2396.txt (RFCs explaining URIs).
 *
 * The resolution of '..' is NOT implemented, but '.' is resolved.
 *
 * \return
 */
EXPORT_SPEC char* resolve_rel_url(
    /*! [in] Base URL. */
    char* base_url,
    /*! [in] Relative URL. */
    char* rel_url);

/*!
 * \brief Parses a uri as defined in http://www.ietf.org/rfc/rfc2396.txt
 * (RFC explaining URIs).
 *
 * Handles absolute, relative, and opaque uris. Parses into the following
 * pieces: scheme, hostport, pathquery, fragment (host with port and path with
 * query are treated as one token). Strings in output uri_type are treated as
 * token with character chain and size. They are not null ('\0') terminated.
 *
 * Caller should check for the pieces they require.
 *
 * \return HTTP_SUCCESS or UPNP_E_INVALID_URL
 */
EXPORT_SPEC int parse_uri(
    /*! [in] Character string containing uri information to be parsed. */
    const char* in,
    /*! [in] Number of characters (strlen()) of the input string. */
    size_t max,
    /*! [out] Output parameter which will have the parsed uri information.
     */
    uri_type* out);

/*!
 * \brief
 *
 * \return
 */
int parse_token(
    /*! [in] . */
    char* in,
    /*! [out] . */
    token* out,
    /*! [in] . */
    int max_size);

#endif /* UPNPLIB_GENLIB_NET_URI_HPP */
