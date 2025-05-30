/**************************************************************************
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
 **************************************************************************/
// Last compare with pupnp original source file on 2022-11-10, ver 1.14.14
/*!
 * \file
 * \ingroup Description-device
 * \brief Configure the full URL for the description document.
 */

#include <urlconfig.hpp>

#include <unixutil.hpp>
#include <upnpdebug.hpp>
#include <uri.hpp>
#include <webserver.hpp>

/// \cond
#include <cassert>
#include <cstdio>
/// \endcond


namespace {

using compa::uriType::Absolute;

/*! \name Scope restricted to file
 * @{ */

/*!
 * \brief Converts an Internet address to a string and stores it in a buffer.
 *
 * \returns
 *  On success: UPNP_E_SUCCESS\n
 *  On error: UPNP_E_BUFFER_TOO_SMALL - Given buffer doesn't have enough size.
 */
inline int addrToString(  //
    const sockaddr* addr, /*!< [in] Pointer to socket address with the IP
                                    Address and port information. */
    char* ipaddr_port, /*!< [out] Character array which will hold the IP Address
                                  in a string format. */
    size_t ipaddr_port_size) ///< [in] Ipaddr_port buffer size.
{
    char buf_ntop[INET6_ADDRSTRLEN];
    int rc = 0;

    if (addr->sa_family == AF_INET) {
        struct sockaddr_in* sa4 = (struct sockaddr_in*)addr;
        inet_ntop(AF_INET, &sa4->sin_addr, buf_ntop, sizeof(buf_ntop));
        rc = snprintf(ipaddr_port, ipaddr_port_size, "%s:%d", buf_ntop,
                      (int)ntohs(sa4->sin_port));
    } else if (addr->sa_family == AF_INET6) {
        struct sockaddr_in6* sa6 = (struct sockaddr_in6*)addr;
        inet_ntop(AF_INET6, &sa6->sin6_addr, buf_ntop, sizeof(buf_ntop));
        rc = snprintf(ipaddr_port, ipaddr_port_size, "[%s]:%d", buf_ntop,
                      (int)ntohs(sa6->sin6_port));
    }
    /// \todo Check bugfix with gtest.
    // if (rc < 0 || (unsigned int)rc >= ipaddr_port_size)
    if (rc < 0 || (unsigned int)rc > ipaddr_port_size)
        return UPNP_E_BUFFER_TOO_SMALL;
    return UPNP_E_SUCCESS;
}

/*!
 * \brief Determine alias based urlbase's root path.
 *
 * \returns
 *  On success: UPNP_E_SUCCESS\n
 *  On error: UPNP_E_OUTOF_MEMORY - On Failure to allocate memory for new *alias
 *
 *  \note Parameter 'newAlias' should be freed after use with using free().
 */
inline int calc_alias(    //
    const char* alias,    ///< [in] String containing the alias.
    const char* rootPath, ///< [in] String containing the root path.
    char** newAlias /*!< [out] String pointer to hold the modified new alias.
                               After use it should be freed using free(). */
) {
    const char* aliasPtr;
    size_t root_len;
    const char* temp_str;
    size_t new_alias_len;
    char* alias_temp;

    assert(rootPath);
    assert(alias);

    /* add / suffix, if missing */
    root_len = strlen(rootPath);
    if (root_len == 0 || rootPath[root_len - 1] != '/')
        temp_str = "/";
    else
        temp_str = ""; /* suffix already present */
    /* discard / prefix, if present */
    if (alias[0] == '/')
        aliasPtr = alias + 1;
    else
        aliasPtr = alias;
    new_alias_len = root_len + strlen(temp_str) + strlen(aliasPtr) + (size_t)1;
    alias_temp = (char*)malloc(new_alias_len);
    if (alias_temp == nullptr)
        return UPNP_E_OUTOF_MEMORY;
    memset(alias_temp, 0, new_alias_len);
    snprintf(alias_temp, new_alias_len, "%s%s%s", rootPath, temp_str, aliasPtr);

    *newAlias = alias_temp;
    return UPNP_E_SUCCESS;
}

/*!
 * \brief Determines the description URL.
 *
 * \returns
 *  On success: UPNP_E_SUCCESS\n
 *  On error: UPNP_E_URL_TOO_BIG - length of the URL is determined to be to
 *            exceeding the limit.
 */
inline int calc_descURL(    //
    const char* ipPortStr,  ///< [in] String containing the portnumber.
    const char* alias,      ///< [in] String containing the alias.
    char descURL[LINE_SIZE] /*!< [out] Buffer to hold the calculated description
                                       URL. */
) {
    size_t len;
    const char* http_scheme = "http://";

    assert(ipPortStr != NULL && strlen(ipPortStr) > 0);
    assert(alias != NULL && strlen(alias) > 0);

    len = strlen(http_scheme) + strlen(ipPortStr) + strlen(alias) + (size_t)1;
    if (len > (size_t)LINE_SIZE)
        return UPNP_E_URL_TOO_BIG;
    snprintf(descURL, len, "%s%s%s", http_scheme, ipPortStr, alias);
    UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__, "desc url: %s\n", descURL);

    return UPNP_E_SUCCESS;
}

/*!
 * \brief Configure the description document.
 *
 * Add the standard format and then add information from the root device and any
 * child nodes.
 *
 * \returns
 *  On success: UPNP_E_SUCCESS\n
 *  On error:
 *  - UPNP_E_OUTOF_MEMORY - Default Error
 *  - UPNP_E_INVALID_DESC - Invalid child node
 *  - UPNP_E_INVALID_URL - Invalid node information
 */
inline int config_description_doc(
    /// [in,out] IMXL description document to be configured.
    IXML_Document* doc,
    /// [in] ipaddress string.
    const char* ip_str,
    /*! [out] buffer to hold the root path of the configured description
              document. */
    char** root_path_str) {
    IXML_NodeList* baseList;
    IXML_Element* element = NULL;
    IXML_Node* textNode = NULL;
    IXML_Node* rootNode = NULL;
    IXML_Node* urlbase_node = NULL;
    const char* urlBaseStr = "URLBase";
    const DOMString domStr = NULL;
    uri_type uri;
    int err_code;
    int len;
    membuffer url_str;
    membuffer root_path;

    membuffer_init(&url_str);
    membuffer_init(&root_path);
    err_code = UPNP_E_OUTOF_MEMORY; /* default error */
    baseList = ixmlDocument_getElementsByTagName(doc, urlBaseStr);
    if (baseList == NULL) {
        /* urlbase not found -- create new one */
        element = ixmlDocument_createElement(doc, urlBaseStr);
        if (element == NULL) {
            goto error_handler;
        }
        if (membuffer_append_str(&url_str, "http://") != 0 ||
            membuffer_append_str(&url_str, ip_str) != 0 ||
            membuffer_append_str(&url_str, "/") != 0 ||
            membuffer_append_str(&root_path, "/") != 0) {
            goto error_handler;
        }
        rootNode = ixmlNode_getFirstChild((IXML_Node*)doc);
        if (rootNode == NULL) {
            err_code = UPNP_E_INVALID_DESC;
            goto error_handler;
        }
        err_code = ixmlNode_appendChild(rootNode, (IXML_Node*)element);
        if (err_code != IXML_SUCCESS) {
            err_code = UPNP_E_INVALID_DESC;
            goto error_handler;
        }
        textNode = ixmlDocument_createTextNode(doc, (char*)url_str.buf);
        if (textNode == NULL) {
            goto error_handler;
        }
        err_code = ixmlNode_appendChild((IXML_Node*)element, textNode);
        if (err_code != IXML_SUCCESS) {
            err_code = UPNP_E_INTERNAL_ERROR;
            goto error_handler;
        }
    } else {
        /* urlbase found */
        urlbase_node = ixmlNodeList_item(baseList, 0lu);
        assert(urlbase_node != NULL);
        textNode = ixmlNode_getFirstChild(urlbase_node);
        if (textNode == NULL) {
            err_code = UPNP_E_INVALID_DESC;
            goto error_handler;
        }
        domStr = ixmlNode_getNodeValue(textNode);
        if (domStr == NULL) {
            err_code = UPNP_E_INVALID_URL;
            goto error_handler;
        }
        len = parse_uri(domStr, strlen(domStr), &uri);
        if (len < 0 || uri.type != Absolute) {
            err_code = UPNP_E_INVALID_URL;
            goto error_handler;
        }
        if (membuffer_assign(&url_str, uri.scheme.buff, uri.scheme.size) != 0 ||
            membuffer_append_str(&url_str, "://") != 0 ||
            membuffer_append_str(&url_str, ip_str) != 0) {
            goto error_handler;
        }
        /* add leading '/' if missing from relative path */
        if ((uri.pathquery.size > 0 && uri.pathquery.buff[0] != '/') ||
            (uri.pathquery.size == 0)) {
            if (membuffer_append_str(&url_str, "/") != 0 ||
                membuffer_append_str(&root_path, "/") != 0) {
                goto error_handler;
            }
        }
        if (membuffer_append(&url_str, uri.pathquery.buff,
                             uri.pathquery.size) != 0 ||
            membuffer_append(&root_path, uri.pathquery.buff,
                             uri.pathquery.size) != 0) {
            goto error_handler;
        }
        /* add trailing '/' if missing */
        if (url_str.buf[url_str.length - 1] != '/') {
            if (membuffer_append(&url_str, "/", (size_t)1) != 0) {
                goto error_handler;
            }
        }
        err_code = ixmlNode_setNodeValue(textNode, url_str.buf);
        if (err_code != IXML_SUCCESS) {
            err_code = UPNP_E_OUTOF_MEMORY;
            goto error_handler;
        }
    }
    *root_path_str = membuffer_detach(&root_path); /* return path */
    err_code = UPNP_E_SUCCESS;

error_handler:
    if (err_code != UPNP_E_SUCCESS) {
        ixmlElement_free(element);
    }
    ixmlNodeList_free(baseList);
    membuffer_destroy(&root_path);
    membuffer_destroy(&url_str);

    return err_code;
}

/// @} // Functions (scope restricted to file)
} // anonymous namespace

int configure_urlbase(IXML_Document* doc, const sockaddr* serverAddr,
                      const char* alias, time_t last_modified,
                      char docURL[LINE_SIZE]) {
    char* root_path = NULL;
    char* new_alias = NULL;
    char* xml_str = NULL;
    int err_code;
    char ipaddr_port[LINE_SIZE];

    /* get IP address and port */
    err_code = addrToString(serverAddr, ipaddr_port, sizeof(ipaddr_port));
    if (err_code != UPNP_E_SUCCESS) {
        goto error_handler;
    }

    /* config url-base in 'doc' */
    err_code = config_description_doc(doc, ipaddr_port, &root_path);
    if (err_code != UPNP_E_SUCCESS) {
        goto error_handler;
    }
    /* calc alias */
    err_code = calc_alias(alias, root_path, &new_alias);
    if (err_code != UPNP_E_SUCCESS) {
        goto error_handler;
    }
    /* calc full url for desc doc */
    err_code = calc_descURL(ipaddr_port, new_alias, docURL);
    if (err_code != UPNP_E_SUCCESS) {
        goto error_handler;
    }
    /* xml doc to str */
    xml_str = ixmlPrintDocument(doc);
    if (xml_str == NULL) {
        goto error_handler;
    }

    UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__, "desc url: %s\n", docURL);
    UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__, "doc = %s\n", xml_str);
    /* store in web server */
    err_code = web_server_set_alias(new_alias, xml_str, strlen(xml_str),
                                    last_modified);

error_handler:
    free(root_path);
    free(new_alias);

    if (err_code != UPNP_E_SUCCESS) {
        ixmlFreeDOMString(xml_str);
    }
    return err_code;
}
