/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
 * Redistribution only with this Copyright remark. Last modified: 2024-02-02
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
 * \brief Callback function to handle incoming GENA requests.
 */

#if EXCLUDE_GENA == 0
#include <gena.hpp>
#include <gena_ctrlpt.hpp>
#include <gena_device.hpp>

#include <httpparser.hpp>
#include <httpreadwrite.hpp>
#include <statcodes.hpp>
#include <unixutil.hpp>


void error_respond(SOCKINFO* info, int error_code, http_message_t* hmsg) {
    int major, minor;

    /* retrieve the minor and major version from the GENA request */
    http_CalcResponseVersion(hmsg->major_version, hmsg->minor_version, &major,
                             &minor);

    http_SendStatusResponse(info, error_code, major, minor);
}


void genaCallback(http_parser_t* parser, http_message_t* request,
                  SOCKINFO* info) {
    int found_function = 0;
    (void)parser;

    if (request->method == HTTPMETHOD_SUBSCRIBE) {
#ifdef INCLUDE_DEVICE_APIS
        found_function = 1;
        if (httpmsg_find_hdr(request, HDR_NT, NULL) == NULL) {
            /* renew subscription */
            gena_process_subscription_renewal_request(info, request);
        } else {
            /* subscribe */
            gena_process_subscription_request(info, request);
        }
        UpnpPrintf(UPNP_ALL, GENA, __FILE__, __LINE__,
                   "got subscription request\n");
    } else if (request->method == HTTPMETHOD_UNSUBSCRIBE) {
        found_function = 1;
        /* unsubscribe */
        gena_process_unsubscribe_request(info, request);
#endif
    } else if (request->method == HTTPMETHOD_NOTIFY) {
#ifdef INCLUDE_CLIENT_APIS
        found_function = 1;
        /* notify */
        gena_process_notification_event(info, request);
#endif
    }

    if (!found_function) {
        /* handle missing functions of device or ctrl pt */
        error_respond(info, HTTP_NOT_IMPLEMENTED, request);
    }
    return;
}
#endif /* EXCLUDE_GENA */
