#ifndef COMPA_SOAP_DEVICE_HPP
#define COMPA_SOAP_DEVICE_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-02-15
/*!
 * \file
 * \brief SOAP declarations for UPnP Devices using SOAP.
 */

#include <httpparser.hpp>
#include <sock.hpp>

#ifndef COMPA_INTERNAL_CONFIG_HPP
#error "No or wrong config.hpp header file included."
#endif

#ifdef COMPA_HAVE_DEVICE_SOAP

/*!
 * \brief This is a callback called by minisever.
 *
 * This is called after receiving the request from the control point. The
 * function will start processing the request. It calls handle_invoke_action()
 * to handle the SOAP action.
 */
void soap_device_callback(
    /*! [in] Parsed request received by the device. */
    http_parser_t* parser,
    /*! [in] HTTP request. */
    http_message_t* request,
    /*! [in,out] Socket info. */
    SOCKINFO* info);

#endif /* COMPA_HAVE_DEVICE_SOAP */
#endif // COMPA_SOAP_DEVICE_HPP
