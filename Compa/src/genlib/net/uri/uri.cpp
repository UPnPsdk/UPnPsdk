/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (c) 2012 France Telecom All rights reserved.
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
/*!
 * \file
 * \brief Modify and parse URIs.
 */

#include <upnp.hpp>
#include <uri.hpp>

#include <UPnPsdk/port_sock.hpp>
#include <umock/netdb.hpp>

/// \cond
#include <cassert>
#include <cstdio> // Needed if OpenSSL isn't compiled in.
/// \endcond

UPnPsdk_EXTERN unsigned gIF_INDEX;

using compa::pathType::ABS_PATH;
using compa::pathType::OPAQUE_PART;
using compa::pathType::REL_PATH;
using compa::uriType::Absolute;
using compa::uriType::Relative;

namespace {
/*! \name Scope restricted to file
 * @{
 */

/*!
 * \brief Check for a RESERVED character.
 *
 * Returns a 1 if a char is a RESERVED char as defined in
 * <a href="http://www.ietf.org/rfc/rfc2396.txt">RFC 2396 (explaining URIs)</a>.
 * Added {} for compatibility.
 *
 * \returns 1 if char is a RESERVED char, otherwise 0.
 */
int is_reserved(
    /*! [in] Char to be matched for RESERVED characters. */
    const unsigned char in) {
    if (strchr(";/?:@&=+$,{}", in)) {
        return 1;
    } else {
        return 0;
    }
}

/*!
 * \brief Check for a MARK character.
 *
 * Returns a 1 if a char is a MARK char as defined in
 * <a href="http://www.ietf.org/rfc/rfc2396.txt">RFC 2396 (explaining URIs)</a>.
 *
 * \returns 1 if char is a MARKED char, otherwise 0.
 */
int is_mark(
    /*! [in] Char to be matched for MARK characters. */
    const unsigned char in) {
    if (strchr("-_.!~*'()", in)) {
        return 1;
    } else {
        return 0;
    }
}

/*!
 * \brief Check for an UNRESERVED character.
 *
 * Returns a 1 if a char is an UNRESERVED char as defined in
 * <a href="http://www.ietf.org/rfc/rfc2396.txt">RFC 2396 (explaining URIs)</a>.
 *
 * \return 1 if char is a UNRESERVED char, otherwise 0.
 */
int is_unreserved(
    /*! [in] Char to be matched for UNRESERVED characters. */
    const unsigned char in) {
    if (isalnum(in) || is_mark(in)) {
        return 1;
    } else {
        return 0;
    }
}

/*!
 * \brief Check that a char[3] sequence is ESCAPED.
 *
 * Returns a 1 if a char[3] sequence is ESCAPED as defined in
 * <a href="http://www.ietf.org/rfc/rfc2396.txt">RFC 2396 (explaining URIs)</a>.
 *
 * \returns 1 if char is a ESCAPED char, otherwise 0.
 */
int is_escaped(
    /*! [in] Char sequence to be matched for ESCAPED characters. */
    const unsigned char* in) {
    if (in == nullptr)
        return 0;
    if (in[0] == '%' && isxdigit(in[1]) && isxdigit(in[2])) {
        return 1;
    } else {
        return 0;
    }
}

/*!
 * \brief Parses a string of uric characters starting at in[0].
 *
 * As defined in <a href="http://www.ietf.org/rfc/rfc2396.txt">RFC 2396
 * (explaining URIs)</a>.
 *
 * \returns ???
 */
size_t parse_uric(
    /*! [in] String of characters. */
    const char* in,
    /*! [in] Maximum limit. */
    size_t max,
    /*! [out] Token object where the string of characters is copied. */
    token* out) {
    size_t i{};

    while (i < max &&
           (is_unreserved((unsigned char)in[i]) ||
            is_reserved((unsigned char)in[i]) ||
            ((i + (size_t)2 < max) &&
             is_escaped(reinterpret_cast<const unsigned char*>(&in[i]))))) {
        i++;
    }

    out->size = i;
    out->buff = in;
    return i;
}

/*!
 * \brief Copy the offset and size from a token to another token.
 *
 * Tokens are generally pointers into other strings. This copies the offset and
 * size from a token (in) relative to one string (in_base) into a token (out)
 * relative to another string (out_base).
 */
void copy_token(
    /*! [in] Source token. */
    const token* in,
    /*! [in] */
    const char* in_base,
    /*! [out] Destination token. */
    token* out,
    /*! [in] */
    char* out_base) {
    out->size = in->size;
    out->buff = out_base + (in->buff - in_base);
}

/*!
 * \brief Parses a string with host and port and fills a hostport structure.
 *
 * Parses a string representing a host and port (e.g. "127.127.0.1:80" or
 * "localhost") and fills out a hostport_type struct with internet address and a
 * token representing the full host and port. Uses getaddrinfo() to resolve DNS
 * names. This may result in a longer delay until response from the internet.
 */
int parse_hostport(
    /*! [in] String of characters representing host and port. */
    const char* in,
    /*! [out] Output parameter where the host and port are represented as
     * an internet address. */
    unsigned short int defaultPort,
    /*! [out] The netaddress (with port) */
    hostport_type* out) {
    char workbuf[256];
    char* c;
    struct sockaddr_in* sai4 = (struct sockaddr_in*)&out->IPaddress;
    struct sockaddr_in6* sai6 = (struct sockaddr_in6*)&out->IPaddress;
    char* srvname = NULL;
    char* srvport = NULL;
    char* last_dot = NULL;
    unsigned short int port;
    int af = AF_UNSPEC;
    size_t hostport_size;
    int has_port = 0;
    int ret;

    memset(out, 0, sizeof(hostport_type));
    memset(workbuf, 0, sizeof(workbuf));
    /* Work on a copy of the input string. */
    strncpy(workbuf, in, sizeof(workbuf) - 1);
    c = workbuf;
    if (*c == '[') {
        /* IPv6 addresses are enclosed in square brackets. */
        srvname = ++c;
        while (*c != '\0' && *c != ']')
            c++;
        if (*c == '\0')
            /* did not find closing bracket. */
            return UPNP_E_INVALID_URL;
        /* NULL terminate the srvname and then increment c. */
        *c++ = '\0'; /* overwrite the ']' */
        if (*c == ':') {
            has_port = 1;
            c++;
        }
        af = AF_INET6;
    } else {
        /* IPv4 address -OR- host name. */
        srvname = c;
        while (*c != ':' && *c != '/' &&
               (isalnum(*c) || *c == '.' || *c == '-')) {
            if (*c == '.')
                last_dot = c;
            c++;
        }
        has_port = (*c == ':') ? 1 : 0;
        /* NULL terminate the srvname */
        *c = '\0';
        if (has_port == 1)
            c++;
        if (last_dot != NULL && isdigit(*(last_dot + 1)))
            /* Must be an IPv4 address. */
            af = AF_INET;
        else {
            /* Must be a host name. */
            struct addrinfo hints, *res, *res0;

            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;

            ret = umock::netdb_h.getaddrinfo(srvname, NULL, &hints, &res0);
            if (ret == 0) {
                for (res = res0; res; res = res->ai_next) {
                    switch (res->ai_family) {
                    case AF_INET:
                    case AF_INET6:
                        /* Found a valid IPv4 or IPv6
                         * address. */
                        memcpy(&out->IPaddress, res->ai_addr, res->ai_addrlen);
                        goto found;
                    }
                }
            found:
                umock::netdb_h.freeaddrinfo(res0);
                if (res == NULL)
                    /* Didn't find an AF_INET or AF_INET6
                     * address. */
                    return UPNP_E_INVALID_URL;
            } else
                /* getaddrinfo failed. */
                return UPNP_E_INVALID_URL;
        }
    }
    /* Check if a port is specified. */
    if (has_port == 1) {
        /* Port is specified. */
        srvport = c;
        while (*c != '\0' && isdigit(*c))
            c++;
        port = (unsigned short int)atoi(srvport);
        if (port == 0)
            /* Bad port number. */
            return UPNP_E_INVALID_URL;
    } else
        /* Port was not specified, use default port. */
        port = defaultPort;
    /* The length of the host and port string can be calculated by */
    /* subtracting pointers. */
    hostport_size = (size_t)c - (size_t)workbuf;
    /* Fill in the 'out' information. */
    switch (af) {
    case AF_INET:
        sai4->sin_family = (sa_family_t)af;
        sai4->sin_port = htons(port);
        ret = inet_pton(AF_INET, srvname, &sai4->sin_addr);
        break;
    case AF_INET6:
        sai6->sin6_family = (sa_family_t)af;
        sai6->sin6_port = htons(port);
        sai6->sin6_scope_id = gIF_INDEX;
        ret = inet_pton(AF_INET6, srvname, &sai6->sin6_addr);
        break;
    default:
        /* IP address was set by the hostname (getaddrinfo). */
        /* Override port: */
        if (out->IPaddress.ss_family == (sa_family_t)AF_INET)
            sai4->sin_port = htons(port);
        else
            sai6->sin6_port = htons(port);
        ret = 1;
    }
    /* Check if address was converted successfully. */
    if (ret <= 0)
        return UPNP_E_INVALID_URL;
    out->text.size = hostport_size;
    out->text.buff = in;

    return (int)hostport_size;
}

/*!
 * \brief parses a uri scheme starting at in[0].
 *
 * As defined in <a href="http://www.ietf.org/rfc/rfc2396.txt">RFC 2396
 * (explaining URIs)</a> (e.g. "http:" -> scheme= "http").\n
 * The funcion parses also opaque URIs. A URI is opaque if, and only if, it is
 * absolute and its scheme-specific part does not begin with a slash character
 * ('/'). An opaque URI has a scheme, a scheme-specific part, and possibly a
 * fragment; all other components are undefined. A typical example of an opaque
 * uri is a mail to url **mailto:a@b.com**.\n
 * REF: http://docs.oracle.com/javase/8/docs/api/java/net/URI.html#isOpaque--
 *
 * \note String MUST include ':' within the max charcters.
 *
 * \returns size of the scheme identifier (e.g. 4 for "http"). Will be 0 with
 * invalid scheme identifier.
 */
//
size_t parse_scheme(
    /*! [in] String of characters representing a scheme. */
    const char* in,
    /*! [in] Maximum number of characters. */
    size_t max,
    /*! [out] Output parameter whose buffer is filled in with the scheme. */
    token* out) {
    size_t i = (size_t)0;

    out->size = (size_t)0;
    out->buff = NULL;

    if ((max == (size_t)0) || (!isalpha(in[0])))
        return (size_t)0;

    i++;
    while ((i < max) && (in[i] != ':')) {
        if (!(isalnum(in[i]) || (in[i] == '+') || (in[i] == '-') ||
              (in[i] == '.')))
            return (size_t)0;
        i++;
    }
    if (i < max) {
        out->size = i;
        out->buff = &in[0];
        return i;
    }

    return (size_t)0;
}

/// \brief Check if end of a path.
inline int is_end_path(char c) {
    switch (c) {
    case '?':
    case '#':
    case '\0':
        return 1;
    }
    return 0;
}

/// @} // Scope restricted to file
} // anonymous namespace


int replace_escaped(char* in, size_t index, size_t* max) {
    int tempInt = 0;
    char tempChar = 0;
    size_t i = (size_t)0;
    size_t j = (size_t)0;

    if (in[index] == '%' && isxdigit(in[index + (size_t)1]) &&
        isxdigit(in[index + (size_t)2])) {
        /* Note the "%2x", makes sure that we convert a maximum of two
         * characters. */
#ifdef _WIN32
        if (sscanf_s(&in[index + (size_t)1],
#else
        if (sscanf(&in[index + (size_t)1],
#endif
                     "%2x", (unsigned int*)&tempInt) != 1) {
            return 0;
        }
        tempChar = (char)tempInt;
        for (i = index + (size_t)3, j = index; j < *max; i++, j++) {
            in[j] = tempChar;
            if (i < *max) {
                tempChar = in[i];
            } else {
                tempChar = 0;
            }
        }
        *max -= (size_t)2;
        return 1;
    } else {
        return 0;
    }
}


int copy_URL_list(URL_list* in, URL_list* out) {
    size_t len = strlen(in->URLs) + (size_t)1;
    size_t i = (size_t)0;

    out->URLs = NULL;
    out->parsedURLs = NULL;
    out->size = (size_t)0;

    // clang-format off
// #ifdef UPNPLIB_PUPNP_BUG
    // Ingo - Error old code: this isn't fixed due to compatibility.
    out->URLs = (char*)malloc(len);
    out->parsedURLs = (uri_type*)malloc(sizeof(uri_type) * in->size);

    if (!out->URLs || !out->parsedURLs)
        return UPNP_E_OUTOF_MEMORY;
// #else
//     out->URLs = (char*)malloc(len);
//     if (!out->URLs)
//         return UPNP_E_OUTOF_MEMORY;
//     out->parsedURLs = (uri_type*)malloc(sizeof(uri_type) * in->size);
//     if (!out->parsedURLs) {
//         free(out->URLs);
//         return UPNP_E_OUTOF_MEMORY;
//     }
// #endif
    // clang-format on
    memcpy(out->URLs, in->URLs, len);
    for (i = (size_t)0; i < in->size; i++) {
        /*copy the parsed uri */
        out->parsedURLs[i].type = in->parsedURLs[i].type;
        copy_token(&in->parsedURLs[i].scheme, in->URLs,
                   &out->parsedURLs[i].scheme, out->URLs);
        out->parsedURLs[i].path_type = in->parsedURLs[i].path_type;
        copy_token(&in->parsedURLs[i].pathquery, in->URLs,
                   &out->parsedURLs[i].pathquery, out->URLs);
        copy_token(&in->parsedURLs[i].fragment, in->URLs,
                   &out->parsedURLs[i].fragment, out->URLs);
        copy_token(&in->parsedURLs[i].hostport.text, in->URLs,
                   &out->parsedURLs[i].hostport.text, out->URLs);
        memcpy(&out->parsedURLs[i].hostport.IPaddress,
               &in->parsedURLs[i].hostport.IPaddress,
               sizeof(struct sockaddr_storage));
    }
    out->size = in->size;

    return HTTP_SUCCESS;
}

void free_URL_list(URL_list* list) {
    if (list->URLs) {
        free(list->URLs);
    }
    if (list->parsedURLs) {
        free(list->parsedURLs);
    }
    list->size = (size_t)0;
}

/* This should use upnpdebug. Failing that, disable by default */
#if defined(DEBUG_URI) || defined(DOXYGEN_RUN)
void print_uri(uri_type* in) {
    print_token(&in->scheme);
    print_token(&in->hostport.text);
    print_token(&in->pathquery);
    print_token(&in->fragment);
}

void print_token(token* in) {
    size_t i = 0;

    fprintf(stderr, "Token Size : %" PRIzu "\n\'", in->size);
    for (i = 0; i < in->size; i++)
        putchar(in->buff[i]);
    putchar('\'');
    putchar('\n');
}
#endif /* DEBUG */

int token_string_casecmp(token* in1, const char* in2) {
    size_t in2_length = strlen(in2);
    if (in1->size != in2_length)
        return 1;
    else
        return strncasecmp(in1->buff, in2, in1->size);
}

int token_cmp(token* in1, token* in2) {
    if (in1->size != in2->size)
        return 1;
    else
        return memcmp(in1->buff, in2->buff, in1->size);
}


int remove_escaped_chars(char* in, size_t* size) {
    /// \todo Optimize with prechecking the delimiter '\%'.
    size_t i = (size_t)0;

    for (i = (size_t)0; i < *size; i++) {
        replace_escaped(in, i, size);
    }

    return UPNP_E_SUCCESS;
}


int remove_dots(char* buf, size_t size) {
    char* in = buf;
    char* out = buf;
    char* max = buf + size;

    while (!is_end_path(in[0])) {
        assert(buf <= out);
        assert(out <= in);
        assert(in < max);

        /* case 2.A: */
        if (strncmp(in, "./", 2) == 0) {
            in += 2;
        } else if (strncmp(in, "../", 3) == 0) {
            in += 3;
            /* case 2.B: */
        } else if (strncmp(in, "/./", 3) == 0) {
            in += 2;
        } else if (strncmp(in, "/.", 2) == 0 && is_end_path(in[2])) {
            in += 1;
            in[0] = '/';
            /* case 2.C: */
        } else if (strncmp(in, "/../", 4) == 0 ||
                   (strncmp(in, "/..", 3) == 0 && is_end_path(in[3]))) {
            /* Make the next character in the input buffer a '/': */
            if (is_end_path(in[3])) { /* terminating "/.." case */
                in += 2;
                in[0] = '/';
            } else { /* "/../" prefix case */
                in += 3;
            }
            /* Trim the last component from the output buffer, or
             * empty it. */
            while (buf < out)
                if (*--out == '/')
                    break;
#ifdef DEBUG
            if (out < in)
                out[0] = '\0';
#endif
            /* case 2.D: */
        } else if (strncmp(in, ".", 1) == 0 && is_end_path(in[1])) {
            in += 1;
        } else if (strncmp(in, "..", 2) == 0 && is_end_path(in[2])) {
            in += 2;
            /* case 2.E */
        } else {
            /* move initial '/' character (if any) */
            if (in[0] == '/')
                *out++ = *in++;
            /* move first segment up to, but not including, the next
             * '/' character */
            while (in < max && in[0] != '/' && !is_end_path(in[0]))
                *out++ = *in++;
#ifdef DEBUG
            if (out < in)
                out[0] = '\0';
#endif
        }
    }
    while (in < max)
        *out++ = *in++;
    if (out < max)
        out[0] = '\0';
    return UPNP_E_SUCCESS;
}

char* resolve_rel_url(char* base_url, char* rel_url) {
    uri_type base;
    uri_type rel;
    int rv;
    size_t len_rel;
    size_t len_base;
    size_t len;
    char* out;
    char* out_finger;
    char* path;
    size_t i;
    size_t prefix;

    if (!base_url) {
        if (!rel_url)
            return NULL;
        return strdup(rel_url);
    }

    // BUG! Following segfaults if rel_url is NULL
    len_rel = strlen(rel_url);
    if (parse_uri(rel_url, len_rel, &rel) != HTTP_SUCCESS)
        return NULL;
    if (rel.type == Absolute)
        return strdup(rel_url);

    len_base = strlen(base_url);
    if ((parse_uri(base_url, len_base, &base) != HTTP_SUCCESS) ||
        (base.type != Absolute))
        return NULL;
    if (len_rel == (size_t)0)
        return strdup(base_url);

    len = len_base + len_rel + (size_t)2;
    out = (char*)malloc(len);
    if (out == NULL)
        return NULL;
    memset(out, 0, len);
    out_finger = out;

    /* scheme */
    rv = snprintf(out_finger, len, "%.*s:", (int)base.scheme.size,
                  base.scheme.buff);
    if (rv < 0 || rv >= (int)len)
        goto error;
    out_finger += rv;
    len -= (size_t)rv;

    /* authority */
    if (rel.hostport.text.size > (size_t)0) {
        rv = snprintf(out_finger, len, "%s", rel_url);
        if (rv < 0 || rv >= (int)len)
            goto error;
        return out;
    }
    if (base.hostport.text.size > (size_t)0) {
        rv = snprintf(out_finger, len, "//%.*s", (int)base.hostport.text.size,
                      base.hostport.text.buff);
        if (rv < 0 || rv >= (int)len)
            goto error;
        out_finger += rv;
        len -= (size_t)rv;
    }

    /* path */
    path = out_finger;
    if (rel.path_type == ABS_PATH) {
        rv = snprintf(out_finger, len, "%s", rel_url);
    } else if (base.pathquery.size == (size_t)0) {
        rv = snprintf(out_finger, len, "/%s", rel_url);
    } else {
        if (rel.pathquery.size == (size_t)0) {
            rv = snprintf(out_finger, len, "%.*s", (int)base.pathquery.size,
                          base.pathquery.buff);
        } else {
            if (len < base.pathquery.size)
                goto error;
            i = 0;
            prefix = 1;
            while (i < base.pathquery.size) {
                out_finger[i] = base.pathquery.buff[i];
                switch (base.pathquery.buff[i++]) {
                case '/':
                    prefix = i;
                    /* fall-through */
                default:
                    continue;
                case '?': /* query */
                    if (rel.pathquery.buff[0] == '?')
                        prefix = --i;
                }
                break;
            }
            out_finger += prefix;
            len -= prefix;
            rv = snprintf(out_finger, len, "%.*s", (int)rel.pathquery.size,
                          rel.pathquery.buff);
        }
        if (rv < 0 || rv >= (int)len)
            goto error;
        out_finger += rv;
        len -= (size_t)rv;

        /* fragment */
        if (rel.fragment.size > (size_t)0)
            rv = snprintf(out_finger, len, "#%.*s", (int)rel.fragment.size,
                          rel.fragment.buff);
        else if (base.fragment.size > (size_t)0)
            rv = snprintf(out_finger, len, "#%.*s", (int)base.fragment.size,
                          base.fragment.buff);
        else
            rv = 0;
    }
    if (rv < 0 || rv >= (int)len)
        goto error;
    out_finger += rv;
    len -= (size_t)rv;

    if (remove_dots(path, (size_t)(out_finger - path)) != UPNP_E_SUCCESS)
        goto error;

    return out;

error:
    free(out);
    return NULL;
}

int parse_uri(const char* in, size_t max, uri_type* out) {
    int begin_path = 0;
    size_t begin_hostport = (size_t)0;
    size_t begin_fragment = (size_t)0;
    unsigned short int defaultPort = 80;

    begin_hostport = parse_scheme(in, max, &out->scheme);
    if (begin_hostport) {
        out->type = Absolute;
        out->path_type = OPAQUE_PART;
        begin_hostport++; // skip ':' scheme delimiter
    } else {
        out->type = Relative;
        out->path_type = REL_PATH;
    }
    if (begin_hostport + (size_t)1 < max && in[begin_hostport] == '/' &&
        in[begin_hostport + (size_t)1] == '/') {
        begin_hostport += (size_t)2;
        if (token_string_casecmp(&out->scheme, "https") == 0) {
            defaultPort = 443;
        }
        begin_path =
            parse_hostport(&in[begin_hostport], defaultPort, &out->hostport);
        if (begin_path >= 0) {
            begin_path += (int)begin_hostport;
        } else
            return begin_path; // error code from parse_hostport()
    } else {
        memset(&out->hostport, 0, sizeof(out->hostport));
        begin_path = (int)begin_hostport;
    }
    begin_fragment =
        parse_uric(&in[begin_path], max - (size_t)begin_path, &out->pathquery) +
        (size_t)begin_path;
    if (out->pathquery.size && out->pathquery.buff[0] == '/') {
        out->path_type = ABS_PATH;
    }
    if (begin_fragment < max && in[begin_fragment] == '#') {
        begin_fragment++;
        parse_uric(&in[begin_fragment], max - begin_fragment, &out->fragment);
    } else {
        out->fragment.buff = NULL;
        out->fragment.size = (size_t)0;
    }

    return HTTP_SUCCESS;
}
