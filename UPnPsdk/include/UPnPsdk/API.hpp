#ifndef UPnPsdk_API_HPP
#define UPnPsdk_API_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-10-15
/*!
 * \file
 * \brief UPnPsdk API
 */

namespace UPnPsdk {

/*!
 * \brief Different HTTP methods.
 */
enum Upnp_HttpMethod {
    UPNP_HTTPMETHOD_PUT, ///< PUT
    UPNP_HTTPMETHOD_DELETE, ///< DELETE
    UPNP_HTTPMETHOD_GET, ///< GET
    UPNP_HTTPMETHOD_HEAD, ///< HEAD
    UPNP_HTTPMETHOD_POST ///< POST
};

} // namespace UPnPsdk

#endif // UPnPsdk_API_HPP
