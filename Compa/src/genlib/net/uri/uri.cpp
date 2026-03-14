/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (c) 2012 France Telecom All rights reserved.
 * Copyright (C) 2021 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
 * Redistribution only with this Copyright remark. Last modified: 2026-03-15
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
#include <UPnPsdk/synclog.hpp>
#include <UPnPsdk/addrinfo.hpp>

/// \cond
#include <cstdio>   // Needed if OpenSSL isn't compiled in.
#include <climits>
#include <iostream> // DEBUG!
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


char* resolve_rel_url(char* a_base_url, char* a_rel_url) {
    std::string out_str;

    do { // This loop is used one time and only to have one exit target for
         // breaks. There are two special cases that are not direct handled by
         // the CUri class.
        try {
            using STATE = UPnPsdk::CComponent::STATE;
            if (a_base_url == nullptr && a_rel_url != nullptr) {
                // If the base_url is a nullptr, then a copy of the rel_url is
                // passed back.
                UPnPsdk::CUriRef uri_refObj(a_rel_url);
                if (uri_refObj.scheme.state() == STATE::avail) {
                    // An absolute URI is checked against DNS.
                    auto& host = uri_refObj.authority.host;
                    auto& port = uri_refObj.authority.port;
                    UPnPsdk::CAddrinfo aiObj(host.str(), port.str());
                    if (!aiObj.get_first())
                        return nullptr;
                }

                out_str = uri_refObj.str();
                break;
            }

            if (a_base_url != nullptr && a_rel_url != nullptr) {
                // If the rel_url is absolute (with a valid base_url), then a
                // copy of the rel_url is passed back. I have first to check
                // both URIs if they are absolute, means if they have a scheme.
                UPnPsdk::CUriRef uri_relObj(a_rel_url);
                if (uri_relObj.scheme.state() == STATE::avail) {
                    UPnPsdk::CUriRef uri_baseObj(a_base_url);
                    if (uri_baseObj.scheme.state() == STATE::avail) {
                        // The condition is given and I return the "relative"
                        // URI, that is available with an absolute scheme, but
                        // only if it is registered on DNS.
                        auto& host = uri_relObj.authority.host;
                        auto& port = uri_relObj.authority.port;
                        UPnPsdk::CAddrinfo aiObj(host.str(), port.str());
                        if (!aiObj.get_first())
                            return nullptr;

                        out_str = uri_relObj.str();
                        break;
                    }
                }
            }

            // Handle the rest of relative parsing. That is covered by the CUri
            // class.
            UPnPsdk::CUri uriObj(a_base_url == nullptr ? "" : a_base_url);
            uriObj = a_rel_url == nullptr ? "" : a_rel_url;

            // Accept a merged target URI only if it is registered on DNS.
            auto& host = uriObj.target.authority.host;
            auto& port = uriObj.target.authority.port;
            UPnPsdk::CAddrinfo aiObj(host.str(), port.str());
            if (!aiObj.get_first())
                return nullptr;

            out_str = uriObj.str();

        } catch (const std::invalid_argument& ex) {
            UPnPsdk_LOGCATCH("MSG1046") "Catched next line...\n"
                << ex.what() << '\n';
            return nullptr;
        }
    } while (false);


    char* out = static_cast<char*>(malloc(out_str.size() + 1));
    if (out == nullptr)
        return nullptr;
    out_str.copy(out, std::string::npos);
    out[out_str.size()] = '\0';

    return out;
}
