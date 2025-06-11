#ifndef COMPA_SOCK_API_HPP
#define COMPA_SOCK_API_HPP
// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-11
/*!
 * \file
 * \ingroup compa-Addressing
 * \brief Declaring only application interface parts from sock.cpp
 */

#include <UPnPsdk/visibility.hpp>

#ifdef UPnPsdk_HAVE_OPENSSL
#include <openssl/ssl.h>
#endif

/*! @{
 * \ingroup compaAPI */

/*!
 * \brief Initializes the OpenSSL library, and the OpenSSL context.
 *
 * \note This method is only available if the library is compiled with OpenSSL
 * support.
 *
 * \returns An integer representing one of the following:
 *   - UPNP_E_SUCCESS: The operation completed successfully.
 *   - UPNP_E_INIT: The SDK is already initialized.
 *   - UPNP_E_INIT_FAILED: The SDK initialization failed. Is OpenSSL available?
 */
#ifdef UPnPsdk_HAVE_OPENSSL
UPnPsdk_VIS int UpnpInitSslContext(
    /*! This argument is ignored and only provided for compatibility. It can be
     * set to any value. The library is initialzed automatically. */
    int initOpenSslLib,
    /*! The SSL_METHOD to use to create the context. See OpenSSL docs for more
     * info, that recommends to use TLS_method(), TLS_server_method() (for
     * client and server), or TLS_client_method() since OpenSSL 1.1.0 because
     * other methods are deprecated. */
    const SSL_METHOD* sslMethod);

/*!
 * \brief Free the OpenSSL context.
 *
 * \note This method is only available if the library is compiled with OpenSSL
 * support.
 */
UPnPsdk_VIS void freeSslCtx(void);
#endif

/// @} ingroup compaAPI

#endif // COMPA_SOCK_API_HPP
