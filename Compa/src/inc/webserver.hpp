#ifndef COMPA_NET_HTTP_WEBSERVER_HPP
#define COMPA_NET_HTTP_WEBSERVER_HPP
/**************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
 * Redistribution only with this Copyright remark. Last modified: 2025-05-13
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
// Last compare with ./Pupnp source file on 2025-04-10, ver 1.14.20
/*!
 * \file
 * \brief Internal Web Server and functions to carry out operations of it.
 */

#include <miniserver.hpp>
#include <sock.hpp>

/*! Global variable. A local dir which serves as webserver root. */
inline membuffer gDocumentRootDir;
/*! Global variable. A string which is set in the header field. */
inline membuffer gWebserverCorsString;

/// \brief Send instruction
struct SendInstruction {
    /// @{
    /// \brief member variable
    int IsVirtualFile;
    int IsChunkActive; // Chunked encoding should be used. This is only
                       // supported by HTTP 1.1 clients. But I do not support
                       // deprecated HTTP 1.0. This flag is obsolete.
    int IsRangeActive;
    int IsTrailers;
    char RangeHeader[200];
    char AcceptLanguageHeader[200];
    off_t RangeOffset;
    /// @}
    /*! \brief Read from local source and send on the network. */
    off_t ReadSendSize;
    /*! \brief Recv from the network and write into local file. */
    long RecvWriteSize;
    /*! CorsHeader. */
    const void* CorsHeader;
    /*! \brief Cookie associated with the virtualDir. */
    const void* Cookie;
    /*! \brief Cookie associated with the request. */
    const void* RequestCookie;
    /* Later few more member could be added depending
     * on the requirement.*/
};

#if !defined(X_USER_AGENT) || defined(DOXYGEN_RUN)
/*! \brief Can be overwritten by configure CFLAGS argument.
 *
 * If not already defined, the {`X_USER_AGENT`} constant specifies the value of
 * the X-User-Agent: HTTP header. The value "redsonic" is needed for the
 * DSM-320. See https://sourceforge.net/forum/message.php?msg_id=3166856 for
 * more information.
 *
 * \todo Check setting of X_USER_AGENT.
 */
#define X_USER_AGENT "redsonic"
#endif

/*!
 * \brief Initilialize root directory for web server and different documents.
 *
 * Initilialize the different documents. Initialize the memory for root
 * directory for web server. Call to initialize global XML document. Sets
 * bWebServerState to WEB_SERVER_ENABLED.
 *
 * \note alias_content is not freed here
 *
 * \return
 * \li \c 0 - OK
 * \li \c UPNP_E_OUTOF_MEMORY
 */
int web_server_init();

/*!
 * \brief Replaces current alias with the given alias.
 *
 * To remove the current alias, set **a_alias_name** to nullptr.
 * \return
 * \li \c UPNP_E_SUCCESS
 * \li \c UPNP_E_INVALID_ARGUMENT
 */
int web_server_set_alias(
    /*! [in] Pointer to webserver name of alias for a copy; the ownership of
     * this C string with terminating '\0' remains by the caller. He is still
     * responsible to manage it (e.g. deallocating if allocated etc.). */
    const char* a_alias_name,
    /*! [in,out] Pointer to the xml doc. This string must be allocated by the
     * caller. Ownership of this argument is then taken over by the web server
     * to manage it, in particular also deallocate it. The caller hasn't to
     * worry about memory management. */
    const char* a_alias_content,
    /*! [in,out] Length of alias body in bytes. Document can also contain '\0'
     * characters within its length. */
    size_t a_alias_content_length,
    /*! [in] Time when contents of alias were last changed (local time). */
    time_t a_last_modified);

/*!
 * \brief Assign the path to the global Document root directory.
 *
 * Also check for path names ending in '/'.
 *
 * \return On success: **0**\n
 *  On error:
 *  - UPNP_E_INVALID_ARGUMENT
 *  - UPNP_E_OUTOF_MEMORY
 */
int web_server_set_root_dir(
    /*! [in] String having the root directory for the document. */
    const char* root_dir);

/*!
 * \brief Assign the Access-Control-Allow-Origin specfied by the input
 * const char* cors_string parameterto the global CORS string
 *
 * \return Integer.
 */
int web_server_set_cors(
    /*! [in] String having the Access-Control-Allow-Origin string. */
    const char* cors_string);

/*!
 * \brief Main entry point into web server.
 *
 * Handles HTTP GET and HEAD requests.
 */
void web_server_callback(
    /*! [in] . */
    http_parser_t* parser,
    /*! [in] . */
    http_message_t* req,
    /*! [in,out] . */
    SOCKINFO* info);

/*!
 * \brief Set HTTP Get Callback.
 */
void SetHTTPGetCallback(
    /*! [in] HTTP Callback to be invoked. */
    MiniServerCallback callback);

/*!
 * \brief Release memory allocated for the global web server root
 * directory and the global XML document.
 *
 * Resets the flag bWebServerState to WEB_SERVER_DISABLED.
 */
void web_server_destroy();

#endif // COMPA_NET_HTTP_WEBSERVER_HPP
