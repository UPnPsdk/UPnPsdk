/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (c) 2012 France Telecom All rights reserved.
 * Copyright (C) 2021 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
 * Redistribution only with this Copyright remark. Last modified: 2026-03-09
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

#include <UPnPsdk/uri.hpp>
#include <umock/netdb.hpp>

/// \cond
#include <cstdio> // Needed if OpenSSL isn't compiled in.
#include <climits>
#include <vector>
/// \endcond

UPnPsdk_EXTERN unsigned gIF_INDEX;

using pathType::ABS_PATH;
using pathType::OPAQUE_PART;
using pathType::REL_PATH;
using uriType::Absolute;
using uriType::Relative;

namespace {

/*! \name Scope restricted to file
 * @{
 */


/*!
 * \brief Copy the offset and size from a token to another token.
 *
 * Tokens are generally pointers into other strings. This copies the offset and
 * size from a token (in) relative to one string (in_base) into a token (out)
 * relative to another string (out_base).
 */
inline void copy_token(
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
 * \brief Replaces one single escaped character within a string with its
 * unescaped version.
 *
 * This is spezified in <a href="http://www.ietf.org/rfc/rfc2396.txt">RFC 2396
 * (explaining URIs)</a>. The index must exactly point to the \b '\%'
 * character, otherwise the function will return unsuccessful. Size of array is
 * NOT checked (MUST be checked by caller).
 *
 * \note This function modifies the string and the max size. If the sequence is
 * an escaped sequence it is replaced, the other characters in the string are
 * shifted over, and NULL characters are placed at the end of the string.
 *
 * \returns
 *   1 - if an escaped character was converted\n
 *   0 - otherwise
 */
int replace_escaped(
    /*! [in,out] String of characters. */
    char* in,
    /// [in] Index at which to start checking the characters; must point to '%'.
    size_t index,
    /*! [in,out] Maximal size of the string buffer will be reduced by 2 if a
       character is converted. */
    size_t* max) {
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

/// @} // Scope restricted to file
} // anonymous namespace


int create_url_list(memptr* a_url_list, URL_list* a_out) {
    if (!a_out)
        return 0;
    memset(a_out, 0, sizeof(*a_out));
    if (!a_url_list)
        return 0;

    std::string_view url_list_sv =
        std::string_view(a_url_list->buf, a_url_list->length);

    if (url_list_sv.empty() || url_list_sv == "<>")
        return 0;

    // Verify correct delimiter '<', '><', '>', and count URLs.
    auto it{url_list_sv.begin()};
    size_t url_count{1};
    auto ch0 = *it;
    for (++it; it < url_list_sv.end() - 1; it++) {
        if (*it == '<' || (*it == '>' && *++it != '<'))
            return UPNP_E_INVALID_URL;
        if (*it == '<')
            url_count++;
    }
    auto ch1 = *it;
    // Check begin and end delimiter. They must be '<'...'>'.
    // To be compatible with pUPnP we have different error messages.
    if ((ch0 == '<' && ch1 != '>') || (ch0 != '<' && ch1 == '>'))
        return UPNP_E_INVALID_URL;
    if (!(ch0 == '<' && ch1 == '>'))
        return 0;

    // Allocate memory and copy the serialized urls to it.
    auto url_list_size = url_list_sv.size(); // There is no terminating '\0'.
    char* base_urls = static_cast<char*>(malloc(url_list_size + 1));
    if (!base_urls)
        return UPNP_E_OUTOF_MEMORY;
    // copy and terminate url_list ("<url><url>\0").
    base_urls[url_list_sv.copy(base_urls, url_list_size)] = '\0';
    std::string_view base_urls_sv = std::string_view(base_urls, url_list_size);

    // Allocate memory for the parsed urls.
    uri_type* parsed_urls =
        static_cast<uri_type*>(malloc(sizeof(uri_type) * url_count));
    if (!parsed_urls) {
        free(base_urls);
        return UPNP_E_OUTOF_MEMORY;
    }

    // Create views into the base_urls to its components (scheme, hostport,
    // path, etc.) and store them into the allocated buffer as members of a
    // trivial C array.
    size_t store_idx{}; // We have 'uri_count' for URIs correct delimited with
                        // '<url><url>' but the URIs may be wrong. These are not
                        // parsed and returned. To count only the successful
                        // parsed URIs I use this index.
    std::string_view base_url_sv;
    uri_type splitted_url;
    for (size_t i{}; i < url_count; i++) {
        size_t pos = base_urls_sv.find_first_of('>');
        base_url_sv = base_urls_sv.substr(0, pos); // Extract current URL.
        base_url_sv.remove_prefix(1); // Remove leading '<' from current URL.
        base_urls_sv.remove_prefix(pos + 1); // Remove current URL from list.

        // parse_uri
        int return_code =
            parse_uri(base_url_sv.data(), base_url_sv.size(), &splitted_url);

        if (return_code == HTTP_SUCCESS &&
            splitted_url.hostport.text.size != 0) {
            // No errors detected, cache the parsed result. Only URLs with
            // network addresses are considered.
            parsed_urls[store_idx++] = splitted_url;

        } else if (return_code == UPNP_E_OUTOF_MEMORY) {
            free(base_urls);
            free(parsed_urls);
            return UPNP_E_OUTOF_MEMORY;
        }
    }

    // All values are available now. Fill the destination structure.
    a_out->parsedURLs = parsed_urls;
    a_out->URLs = base_urls;
    a_out->size = store_idx;

    if (store_idx > INT_MAX) // Paranoia checking due to cast.
        return UPNP_E_OUTOF_MEMORY;
    return static_cast<int>(store_idx);
}


int copy_URL_list(URL_list* in, URL_list* out) {
    out->URLs = nullptr;
    out->parsedURLs = nullptr;
    out->size = 0u;

    size_t len = strlen(in->URLs) + 1;
    out->URLs = static_cast<char*>(malloc(len));
    if (!out->URLs)
        return UPNP_E_OUTOF_MEMORY;
    // out->parsedURLs = static_cast<uri_type*>(calloc(in->size,
    // sizeof(uri_type)));
    out->parsedURLs =
        static_cast<uri_type*>(malloc(sizeof(uri_type) * in->size));
    if (!out->parsedURLs) {
        free(out->URLs);
        return UPNP_E_OUTOF_MEMORY;
    }

    memcpy(out->URLs, in->URLs, len);
    for (size_t i{0}; i < in->size; i++) {
        // Having pointer within the coppied structure a deep copy is needed.
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
    if (in1->buff == nullptr && in2_length == 0)
        return 0;
    else if (in1->buff == nullptr && in2_length != 0)
        return -1;
    else if (in1->size < in2_length)
        return -1;
    else if (in1->size > in2_length)
        return 1;

    return strncasecmp(in1->buff, in2, in1->size);
}

int token_cmp(token* in1, token* in2) {
    if (in1->size < in2->size)
        return -1;
    else if (in1->size > in2->size)
        return 1;
    else
        return memcmp(in1->buff, in2->buff, in1->size);
}


int remove_escaped_chars(char* in, size_t* size) {
    /// \todo Optimize with prechecking the delimiter '\%'.
    // REF:_[A_practical_guide_to_URI_encoding_and_URI_decoding](ttps://qqq.is/research/a-practical-guide-to-URI-encoding-and-URI-decoding)
    if (in != nullptr && size != nullptr) {
        for (size_t i{0u}; i < *size; i++) {
            replace_escaped(in, i, size);
        }
    }
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
    std::string pathObj;

    if ((!base_url || *base_url == '\0') && (!rel_url || *rel_url == '\0'))
        return nullptr;
    if (!base_url && rel_url)
        return strdup(rel_url);
    if (base_url && (!rel_url || *rel_url == '\0'))
        return strdup(base_url);

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

    pathObj = std::string(path, (size_t)(out_finger - path));
    UPnPsdk::remove_dot_segments(pathObj);
    pathObj.copy(path, pathObj.size());
    path[pathObj.size()] = '\0';

    return out;

error:
    free(out);
    return NULL;
}
