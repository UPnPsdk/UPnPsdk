#ifndef UPnPsdk_HTTPPARSER_HPP
#define UPnPsdk_HTTPPARSER_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-10-15

/*!
 * \file
 * \brief Functions to parse UPnP messages like requests and responses.
 */

#include <UPnPsdk/API.hpp>

namespace UPnPsdk {

/*! \brief Method in a HTTP request.
 * \warning The enum values of the standard HTTP method should match
 * those of Upnp_HttpMethod enum defined in <UPnPsdk/API.hpp> */
enum http_method_t {
    HTTPMETHOD_PUT = UPNP_HTTPMETHOD_PUT,
    HTTPMETHOD_DELETE = UPNP_HTTPMETHOD_DELETE,
    HTTPMETHOD_GET = UPNP_HTTPMETHOD_GET,
    HTTPMETHOD_HEAD = UPNP_HTTPMETHOD_HEAD,
    HTTPMETHOD_POST = UPNP_HTTPMETHOD_POST,
    HTTPMETHOD_MPOST,
    HTTPMETHOD_SUBSCRIBE,
    HTTPMETHOD_UNSUBSCRIBE,
    HTTPMETHOD_NOTIFY,
    HTTPMETHOD_MSEARCH,
    HTTPMETHOD_UNKNOWN,
    SOAPMETHOD_POST,
    HTTPMETHOD_SIMPLEGET
};

} // namespace UPnPsdk

#endif // UPnPsdk_HTTPPARSER_HPP
