/**************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (C) 2012 France Telecom All rights reserved.
 * Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
 * Redistribution only with this Copyright remark. Last modified: 2025-04-20
 * Cloned from pupnp ver 1.14.15.
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
// Last compare with ./pupnp source file on 2023-08-25, ver 1.14.18
/*!
 * \file
 * \ingroup compa-Addressing
 * \brief Implements the functionality and utility functions used by the
 * Miniserver module.
 *
 * The miniserver is a central point for processing all network requests.
 * It is made of:
 *   - The HTTP listeners for description / control / eventing.
 *   - The SSDP sockets for discovery.
 */

#include <miniserver.hpp> // Needed for one of the compile options

#include <httpreadwrite.hpp>
#include <ssdp_common.hpp>
#include <statcodes.hpp>
#include <upnpapi.hpp>

#include <UPnPsdk/socket.hpp>

#include <umock/sys_socket.hpp>
#include <umock/winsock2.hpp>
#include <umock/stdlib.hpp>
#include <umock/pupnp_miniserver.hpp>
#include <umock/pupnp_ssdp.hpp>


/// \cond
#include <cstring>
#include <random>
/// \endcond

namespace {

/*! \brief miniserver received request message.
 * \details This defines the structure of a UPnP request that has income from a
 * remote control point.
 */
struct mserv_request_t {
    /// \todo Replace this structure with CSocket.
    /// \brief Connection socket file descriptor.
    SOCKET connfd;
    /// \brief Socket address of the remote control point.
    sockaddr_storage foreign_sockaddr;
};

/// \brief miniserver state
enum MiniServerState {
    MSERV_IDLE,    ///< miniserver is idle.
    MSERV_RUNNING, ///< miniserver is running.
    MSERV_STOPPING ///< miniserver is running to stop.
};

/*! \brief Port of the stop socket.
 *  \details With starting the miniserver there is also this port registered.
 * Its socket is listing for a "ShutDown" message from a local network address
 * (localhost). This is used to stop the miniserver from another thread.
 */
in_port_t miniStopSockPort;

/// \brief miniserver state
MiniServerState gMServState{MSERV_IDLE};
#ifdef COMPA_HAVE_WEBSERVER
/// \brief SOAP callback
MiniServerCallback gSoapCallback{nullptr};
/// \brief GENA callback
MiniServerCallback gGenaCallback{nullptr};

/*! \name Scope restricted to file
 * @{ */

/*!
 * \brief Check if a network address is numeric.
 *
 * An empty netaddress or an unspecified one ("[::]", "0.0.0.0") is not valid.
 */
int host_header_is_numeric(
    char* a_host_port,     ///< network address
    size_t a_host_port_len ///< length of a_host_port excl. terminating '\0'.
) {
    TRACE("Executing host_header_is_numeric()");
    if (a_host_port_len == 0 || strncmp(a_host_port, "[::]", 4) == 0 ||
        strncmp(a_host_port, "0.0.0.0", 7) == 0)
        return 0;

    UPnPsdk::SSockaddr saddrObj;
    try {
        saddrObj = std::string(a_host_port, a_host_port_len);
    } catch (const std::exception& e) {
        UPnPsdk_LOGCATCH("MSG1049") << e.what() << "\n";
        return 0;
    }
    return 1;
}

/*! \brief Returns the ip address with port as text that is bound to a socket.
 *
 * Example: may return "[2001:db8::ab]:50044" or "192.168.1.2:54321".
 *
 * \returns
 *   On success: **true**\n
 *   On error: **false**, The result buffer remains unmodified.
 */
int getNumericHostRedirection(
    SOCKET a_socket,   ///< [in] Socket file descriptor.
    char* a_host_port, ///< [in,out] Pointer to buffer that will be filled.
    size_t a_hp_size   ///< [in] size of the buffer.
) {
    TRACE("Executing getNumericHostRedirection()")
    UPnPsdk::CSocket_basic socketObj(a_socket);
    try {
        socketObj.load(); // UPnPsdk::CSocket_basic
        UPnPsdk::SSockaddr sa;
        socketObj.local_saddr(&sa);
        memcpy(a_host_port, sa.netaddrp().c_str(), a_hp_size);
        return true;

    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
    }
    return false;
}

/*!
 * \brief Based on the type of message, appropriate callback is issued.
 *
 * \returns
 *  On success: **0**\n
 *  On error: HTTP_INTERNAL_SERVER_ERROR if Callback is NULL.
 */
int dispatch_request(
    /*! [in] Socket Information object. */
    SOCKINFO* info,
    /*! [in] HTTP parser object. */
    http_parser_t* hparser) {
    memptr header;
    size_t min_size;
    http_message_t* request;
    MiniServerCallback callback;
    WebCallback_HostValidate host_validate_callback = 0;
    void* cookie{};
    int rc = UPNP_E_SUCCESS;
    /* If it does not fit in here, it is likely invalid anyway. */
    char host_port[NAME_SIZE];

    switch (hparser->msg.method) {
    /* Soap Call */
    case SOAPMETHOD_POST:
    case HTTPMETHOD_MPOST:
        callback = gSoapCallback;
        UpnpPrintf(UPNP_INFO, MSERV, __FILE__, __LINE__,
                   "miniserver %d: got SOAP msg\n", info->socket);
        break;
    /* Gena Call */
    case HTTPMETHOD_NOTIFY:
    case HTTPMETHOD_SUBSCRIBE:
    case HTTPMETHOD_UNSUBSCRIBE:
        callback = gGenaCallback;
        UpnpPrintf(UPNP_INFO, MSERV, __FILE__, __LINE__,
                   "miniserver %d: got GENA msg\n", info->socket);
        break;
    /* HTTP server call */
    case HTTPMETHOD_GET:
    case HTTPMETHOD_POST:
    case HTTPMETHOD_HEAD:
    case HTTPMETHOD_SIMPLEGET:
        callback = gGetCallback;
        host_validate_callback = gWebCallback_HostValidate;
        cookie = gWebCallback_HostValidateCookie;
        UPnPsdk_LOGINFO("MSG1107") "miniserver socket="
            << info->socket << ": got WEB server msg.\n";
        break;
    default:
        callback = 0;
    }
    if (!callback) {
        rc = HTTP_INTERNAL_SERVER_ERROR;
        goto ExitFunction;
    }
    request = &hparser->msg;
    if (UPnPsdk::g_dbug) {
        getNumericHostRedirection(info->socket, host_port, sizeof host_port);
        UPnPsdk_LOGINFO("MSG1113") "Redirect host_port=\"" << host_port
                                                           << "\"\n";
    }
    /* check HOST header for an IP number -- prevents DNS rebinding. */
    if (!httpmsg_find_hdr(request, HDR_HOST, &header)) {
        rc = UPNP_E_BAD_HTTPMSG;
        goto ExitFunction;
    }
    min_size = header.length < ((sizeof host_port) - 1)
                   ? header.length
                   : (sizeof host_port) - 1;
    memcpy(host_port, header.buf, min_size);
    host_port[min_size] = 0;
    if (host_validate_callback) {
        rc = host_validate_callback(host_port, cookie);
        if (rc == UPNP_E_BAD_HTTPMSG) {
            goto ExitFunction;
        }
    } else if (!host_header_is_numeric(host_port, min_size)) {
        if (!gAllowLiteralHostRedirection) {
            rc = UPNP_E_BAD_HTTPMSG;
            UpnpPrintf(UPNP_INFO, MSERV, __FILE__, __LINE__,
                       "Possible DNS Rebind attack prevented.\n");
            goto ExitFunction;
        } else {
            membuffer redir_buf;
            static const char* redir_fmt = "HTTP/1.1 307 Temporary Redirect\r\n"
                                           "Location: http://%s\r\n\r\n";
            char redir_str[NAME_SIZE];
            int timeout = HTTP_DEFAULT_TIMEOUT;

            getNumericHostRedirection(info->socket, host_port,
                                      sizeof host_port);
            membuffer_init(&redir_buf);
            snprintf(redir_str, NAME_SIZE, redir_fmt, host_port);
            membuffer_append_str(&redir_buf, redir_str);
            rc = http_SendMessage(info, &timeout, "b", redir_buf.buf,
                                  redir_buf.length);
            membuffer_destroy(&redir_buf);
            goto ExitFunction;
        }
    }
    callback(hparser, request, info);

ExitFunction:
    return rc;
}

/*!
 * \brief Free memory assigned for handling request and unitialize socket
 * functionality.
 */
void free_handle_request_arg(
    /*! [in] Request Message to be freed. */
    void* args) {
    TRACE("Executing free_handle_request_arg()")
    if (args == nullptr)
        return;

    sock_close(static_cast<mserv_request_t*>(args)->connfd);
    free(args);
}

/*!
 * \brief Receive the request and dispatch it for handling.
 */
void handle_request(
    /*! [in] Pointer to Received Request Message to be handled. It's expected to
       be mserv_request_t* */
    void* args) {
    TRACE("Executing handle_request()")
    int http_major_version{1};
    int http_minor_version{1};
    int ret_code;
    mserv_request_t* request_in = (mserv_request_t*)args;
    SOCKET connfd = request_in->connfd;

    if (UPnPsdk::g_dbug) {
        UPnPsdk::CSocket_basic sockObj(connfd);
        sockObj.load();
        UPnPsdk::SSockaddr local_saObj;
        sockObj.local_saddr(&local_saObj);
        UPnPsdk::SSockaddr remote_saObj;
        remote_saObj = request_in->foreign_sockaddr;
        UPnPsdk_LOGINFO("MSG1027") "UDevice socket="
            << connfd << ": READING request on local=\""
            << local_saObj.netaddrp() << "\" from control point remote=\""
            << remote_saObj.netaddrp() << "\".\n";
    }

    /* parser_request_init( &parser ); */ /* LEAK_FIX_MK */
    http_parser_t parser;
    http_message_t* hmsg = &parser.msg;
    SOCKINFO info;
    ret_code = sock_init_with_ip(
        &info, connfd,
        reinterpret_cast<sockaddr*>(&request_in->foreign_sockaddr));
    /// \todo Improve test to detect ret_code = UPNP_E_INTERNAL_ERROR;
    if (ret_code != UPNP_E_SUCCESS) {
        free(request_in);
        httpmsg_destroy(hmsg);
        return;
    }

    /* read */
    int timeout{HTTP_DEFAULT_TIMEOUT};
    int http_error_code; // Either negative UPNP_E_XXX error code,
                         // or positve HTTP error code (4XX or 5XX).
                         // Will be initialized by next function.
    ret_code = http_RecvMessage(&info, &parser, HTTPMETHOD_UNKNOWN, &timeout,
                                &http_error_code);
    if (ret_code == 0) {
        UPnPsdk_LOGINFO("MSG1106") "miniserver socket=" << connfd
                                                        << ": PROCESSING...\n";
        /* dispatch */
        http_error_code = dispatch_request(&info, &parser);
    }

    if (http_error_code > 0) { // only positive HTTP error codes (4XX or 5XX).
        if (hmsg) {
            http_major_version = hmsg->major_version;
            http_minor_version = hmsg->minor_version;
        }
        http_SendStatusResponse(&info, http_error_code, http_major_version,
                                http_minor_version);
    }
    sock_destroy(&info, SD_BOTH);
    httpmsg_destroy(hmsg);
    free(request_in);

    UPnPsdk_LOGINFO("MSG1058") "miniserver socket(" << connfd
                                                    << "); COMPLETE.\n";
}

/*!
 * \brief Initilize the thread pool to handle an incomming request, sets
 * priority for the job and adds the job to the thread pool.
 */
UPNP_INLINE void schedule_request_job(
    /*! [in] Socket Descriptor on which connection is accepted. */
    SOCKET a_connfd,
    /*! [in] Ctrlpnt address object. */
    UPnPsdk::SSockaddr& clientAddr) {
    TRACE("Executing schedule_request_job()")
    if (UPnPsdk::g_dbug) {
        UPnPsdk::CSocket_basic sockObj(a_connfd);
        sockObj.load();
        UPnPsdk::SSockaddr local_saObj;
        sockObj.local_saddr(&local_saObj);
        UPnPsdk::SSockaddr remote_saObj;
        remote_saObj = clientAddr.ss;
        UPnPsdk_LOGINFO(
            "MSG1042") "Schedule UDevice to read incomming request with socket("
            << a_connfd << ") local=\"" << local_saObj.netaddrp()
            << "\" remote=\"" << remote_saObj.netaddrp() << "\".\n";
    }

    ThreadPoolJob job{};
    mserv_request_t* request{
        static_cast<mserv_request_t*>(std::malloc(sizeof(mserv_request_t)))};
    if (request == nullptr) {
        UPnPsdk_LOGCRIT("MSG1024") "Socket(" << a_connfd
                                             << "): out of memory.\n";
        sock_close(a_connfd);
        return;
    }
    request->connfd = a_connfd;
    memcpy(&request->foreign_sockaddr, &clientAddr.ss,
           sizeof(request->foreign_sockaddr));
    TPJobInit(&job, (start_routine)handle_request, request);
    TPJobSetFreeFunction(&job, free_handle_request_arg);
    TPJobSetPriority(&job, MED_PRIORITY);
    if (ThreadPoolAdd(&gMiniServerThreadPool, &job, NULL) != 0) {
        UPnPsdk_LOGERR("MSG1025") "Socket("
            << a_connfd << "): failed to add job to miniserver threadpool.\n";
        free(request);
        sock_close(a_connfd);
    }
}
#endif // COMPA_HAVE_WEBSERVER

/*!
 * \brief Add a socket file descriptor to an \p 'fd_set' structure as needed for
 * \p \::select().
 *
 * **a_set** may already contain file descriptors. The given **a_sock** is added
 * to the set. It is ensured that \p \::select() is not fed with invalid socket
 * file descriptors, in particular with the EBADF error. That could mean: closed
 * socket, or an other network error was detected before adding to the set. It
 * checks that we do not use closed or unbind sockets. It also has a guard that
 * we do not exceed the maximum number FD_SETSIZE (1024) of selectable file
 * descriptors, as noted in "man select".
 *
 * **Returns**
 *  - Nothing. Not good, but needed for compatibility.\n
 * You can check if **a_sock** was added to **a_set**. There are messages to
 * stderr if verbose logging is enabled.
 */
void fdset_if_valid( //
    SOCKET a_sock,   ///< [in] socket file descriptor.
    fd_set* a_set /*!< [out] Pointer to an \p 'fd_set' structure as needed for
                     \p \::select(). The structure is modified as documented for
                     \p \::select(). */
) {
    TRACE("Executing fdset_if_valid(): check sockfd=" + std::to_string(a_sock))
    if (a_sock == INVALID_SOCKET)
        // This is a defined state and we return silently.
        return;

    if (a_sock < 3 || a_sock >= FD_SETSIZE) {
        UPnPsdk_LOGERR("MSG1005")
            << (a_sock < 0 ? "Invalid" : "Prohibited") << " socket " << a_sock
            << " not set to be monitored by ::select()"
            << (a_sock >= 3 ? " because it violates FD_SETSIZE.\n" : ".\n");
        return;
    }
    // Check if socket is valid and bound
    UPnPsdk::CSocket_basic sockObj(a_sock);
    try {
        sockObj.load(); // UPnPsdk::CSocket_basic
        if (sockObj.local_saddr())

            FD_SET(a_sock, a_set);

        else
            UPnPsdk_LOGINFO("MSG1002") "Unbound socket "
                << a_sock << " not set to be monitored by ::select().\n";

    } catch (const std::exception& e) {
        if (UPnPsdk::g_dbug)
            std::cerr << e.what();
        UPnPsdk_LOGCATCH("MSG1009") "Invalid socket "
            << a_sock << " not set to be monitored by ::select().\n";
    }
}

/*!
 * \brief Accept requested connection from a remote control point and run it in
 * a new thread.
 *
 * \returns
 *  On success: UPNP_E_SUCCESS\n
 *  On error:
 *  - UPNP_E_NO_WEB_SERVER
 *  - UPNP_E_SOCKET_ERROR
 *  - UPNP_E_SOCKET_ACCEPT
 */
int web_server_accept(
    /// [in] File descriptor of socket that is listening on incomming requests.
    [[maybe_unused]] SOCKET listen_sock,
    /// [out] Reference to a file descriptor set as needed for \::select().
    [[maybe_unused]] fd_set& set) {
#ifndef COMPA_HAVE_WEBSERVER
    return UPNP_E_NO_WEB_SERVER;
#else
    TRACE("Executing web_server_accept()")
    if (listen_sock == INVALID_SOCKET || !FD_ISSET(listen_sock, &set)) {
        UPnPsdk_LOGINFO("MSG1012") "Socket("
            << listen_sock << ") invalid or not in file descriptor set.\n";
        return UPNP_E_SOCKET_ERROR;
    }

    UPnPsdk::SSockaddr ctrlpnt_saObj;
    socklen_t ctrlpntLen = sizeof(ctrlpnt_saObj.ss);

    // accept a network request connection
    SOCKET conn_sock =
        umock::sys_socket_h.accept(listen_sock, &ctrlpnt_saObj.sa, &ctrlpntLen);
    if (conn_sock == INVALID_SOCKET) {
        UPnPsdk_LOGERR("MSG1022") "Error in ::accept(): "
            << std::strerror(errno) << ".\n";
        return UPNP_E_SOCKET_ACCEPT;
    }

    if (UPnPsdk::g_dbug) {
        // Some helpful status information.
        UPnPsdk::CSocket_basic listen_sockObj(listen_sock);
        listen_sockObj.load();
        UPnPsdk::SSockaddr listen_saObj;
        listen_sockObj.local_saddr(&listen_saObj);

        UPnPsdk::CSocket_basic conn_sockObj(conn_sock);
        conn_sockObj.load();
        UPnPsdk::SSockaddr conn_saObj;
        conn_sockObj.local_saddr(&conn_saObj);

        UPnPsdk_LOGINFO("MSG1023") "Listening socket("
            << listen_sock << ") on \"" << listen_saObj.netaddrp()
            << "\" accept connection socket(" << conn_sock << ") local=\""
            << conn_saObj.netaddrp() << "\" to remote=\""
            << ctrlpnt_saObj.netaddrp() << "\".\n";
    }

    // Schedule a job to manage the UPnP request from the remote control point.
    schedule_request_job(conn_sock, ctrlpnt_saObj);

    return UPNP_E_SUCCESS;
#endif /* COMPA_HAVE_WEBSERVER */
}

/*!
 * \brief Read data from the SSDP socket.
 */
void ssdp_read( //
    SOCKET* rsock,     ///< [in] Pointer to a Socket file descriptor.
    fd_set* set        /*!< [in] Pointer to a file descriptor set as needed for
                                 \::select(). */) {
    TRACE("Executing ssdp_read()")
    if (*rsock == INVALID_SOCKET || !FD_ISSET(*rsock, set))
        return;

#if defined(COMPA_HAVE_CTRLPT_SSDP) || defined(COMPA_HAVE_DEVICE_SSDP)
    if (readFromSSDPSocket(*rsock) != 0) {
        UpnpPrintf(UPNP_ERROR, MSERV, __FILE__, __LINE__,
                   "miniserver: Error in readFromSSDPSocket(%d): "
                   "closing socket\n",
                   *rsock);
        sock_close(*rsock);
        *rsock = INVALID_SOCKET;
    }
#else
    sock_close(*rsock);
    *rsock = INVALID_SOCKET;
#endif
}

/*!
 * \brief Check if we have received a packet that shall stop the miniserver.
 *
 * The received datagram must exactly match the shutdown_str from 127.0.0.1.
 * This is a security issue to avoid that the UPnPlib can be terminated from a
 * remote ip address. Receiving 0 bytes on a datagram (there's a datagram here)
 * indicates that a zero-length datagram was successful sent. This will not
 * stop the miniserver.
 *
 * \returns
 * - 1 - when the miniserver shall be stopped,
 * - 0 - otherwise.
 */
int receive_from_stopSock(
    SOCKET ssock, ///< [in] Socket file descriptor.
    fd_set* set   /*!< [in] Pointer to a file descriptor set as needed for
                            \::select(). */
) {
    TRACE("Executing receive_from_stopSock()")
    constexpr char shutdown_str[]{"ShutDown"};

    if (!FD_ISSET(ssock, set))
        return 0; // Nothing to do for this socket

    UPnPsdk::sockaddr_t clientAddr{};
    socklen_t clientLen{sizeof(clientAddr)}; // May be modified

    // The receive buffer is one byte greater with '\0' than the max receiving
    // bytes so the received message will always be terminated.
    char receiveBuf[sizeof(shutdown_str) + 1]{};
    char buf_ntop[INET6_ADDRSTRLEN];

    // receive from
    SSIZEP_T byteReceived = umock::sys_socket_h.recvfrom(
        ssock, receiveBuf, sizeof(shutdown_str), 0, &clientAddr.sa, &clientLen);
    if (byteReceived == SOCKET_ERROR ||
        inet_ntop(AF_INET, &clientAddr.sin.sin_addr, buf_ntop,
                  sizeof(buf_ntop)) == nullptr) {
        UPnPsdk_LOGCRIT("MSG1038") "Failed to receive data from socket "
            << ssock << ". Stop miniserver.\n";
        return 1;
    }

    // integer of "127.0.0.1" is 2130706433.
    if (clientAddr.sin.sin_addr.s_addr != htonl(2130706433) ||
        strcmp(receiveBuf, shutdown_str) != 0) //
    {
        char nullstr[]{"\\0"};
        if (byteReceived == 0 || receiveBuf[byteReceived - 1] != '\0')
            nullstr[0] = '\0';
        UPnPsdk_LOGERR("MSG1039") "Received \""
            << receiveBuf << nullstr << "\" from " << buf_ntop << ":"
            << ntohs(clientAddr.sin.sin_port)
            << ", must be \"ShutDown\\0\" from 127.0.0.1:*. Don't "
               "stopping miniserver.\n";
        return 0;
    }

    UPnPsdk_LOGINFO("MSG1040") "On socket "
        << ssock << " received ordinary datagram \"" << receiveBuf
        << "\\0\" from " << buf_ntop << ":" << ntohs(clientAddr.sin.sin_port)
        << ". Stop miniserver.\n";
    return 1;
}

/*!
 * \brief Run the miniserver.
 *
 * The MiniServer accepts a new request and schedules a thread to handle the
 * new request. It checks for socket state and invokes appropriate read and
 * shutdown actions for the Miniserver and SSDP sockets. This function itself
 * runs in its own thread.
 *
 * \attention The miniSock parameter must be allocated on the heap before
 * calling the function because it is freed by it.
 */
void RunMiniServer(
    /*! [in] Pointer to an Array containing valid sockets associated with
       different tasks like listen on a local interface for requests from
       control points or handle ssdp communication to a remote UPnP node. */
    MiniServerSockArray* miniSock) {
    UPnPsdk_LOGINFO("MSG1085") "Executing...\n";

#ifdef COMPA_HAVE_WEBSERVER
    // Move the current valid socket objects to this scope. They are owner of
    // the raw socket file descriptor and manage/close it when leaving this
    // scope.
    UPnPsdk::CSocket sockLlaObj;
    if (miniSock->pSockLlaObj != nullptr) {
        sockLlaObj = std::move(*miniSock->pSockLlaObj);
        miniSock->pSockLlaObj = &sockLlaObj;
    }
    UPnPsdk::CSocket sockGuaObj;
    if (miniSock->pSockGuaObj != nullptr) {
        sockGuaObj = std::move(*miniSock->pSockGuaObj);
        miniSock->pSockGuaObj = &sockGuaObj;
    }
    UPnPsdk::CSocket sockIp4Obj;
    if (miniSock->pSockIp4Obj != nullptr) {
        sockIp4Obj = std::move(*miniSock->pSockIp4Obj);
        miniSock->pSockIp4Obj = &sockIp4Obj;
    }
#endif

    fd_set expSet;
    fd_set rdSet;
    int stopSock = 0;

    // On MS Windows INVALID_SOCKET is unsigned -1 = 18446744073709551615 so we
    // get maxMiniSock with this big number even if there is only one
    // INVALID_SOCKET. Incrementing it at the end results in 0. To be portable
    // we must not assume INVALID_SOCKET to be -1. --Ingo
    SOCKET maxMiniSock = 0;
    maxMiniSock = //
        std::max(maxMiniSock, miniSock->miniServerSock4 == INVALID_SOCKET
                                  ? 0
                                  : miniSock->miniServerSock4);
    maxMiniSock = //
        std::max(maxMiniSock, miniSock->miniServerSock6 == INVALID_SOCKET
                                  ? 0
                                  : miniSock->miniServerSock6);
    maxMiniSock = //
        std::max(maxMiniSock, miniSock->miniServerSock6UlaGua == INVALID_SOCKET
                                  ? 0
                                  : miniSock->miniServerSock6UlaGua);
    maxMiniSock = //
        std::max(maxMiniSock, miniSock->miniServerStopSock == INVALID_SOCKET
                                  ? 0
                                  : miniSock->miniServerStopSock);
    maxMiniSock = //
        std::max(maxMiniSock, miniSock->ssdpSock4 == INVALID_SOCKET
                                  ? 0
                                  : miniSock->ssdpSock4);
    maxMiniSock = //
        std::max(maxMiniSock, miniSock->ssdpSock6 == INVALID_SOCKET
                                  ? 0
                                  : miniSock->ssdpSock6);
    maxMiniSock = //
        std::max(maxMiniSock, miniSock->ssdpSock6UlaGua == INVALID_SOCKET
                                  ? 0
                                  : miniSock->ssdpSock6UlaGua);
#ifdef COMPA_HAVE_CTRLPT_SSDP
    maxMiniSock = //
        std::max(maxMiniSock, miniSock->ssdpReqSock4 == INVALID_SOCKET
                                  ? 0
                                  : miniSock->ssdpReqSock4);
    maxMiniSock = //
        std::max(maxMiniSock, miniSock->ssdpReqSock6 == INVALID_SOCKET
                                  ? 0
                                  : miniSock->ssdpReqSock6);
#endif
    ++maxMiniSock;

    gMServState = MSERV_RUNNING;
    while (!stopSock) {
        FD_ZERO(&rdSet);
        FD_ZERO(&expSet);
        /* FD_SET()'s */
        FD_SET(miniSock->miniServerStopSock, &expSet);
        FD_SET(miniSock->miniServerStopSock, &rdSet);
        fdset_if_valid(miniSock->miniServerSock4, &rdSet);
        fdset_if_valid(miniSock->miniServerSock6, &rdSet);
        fdset_if_valid(miniSock->miniServerSock6UlaGua, &rdSet);
        fdset_if_valid(miniSock->ssdpSock4, &rdSet);
        fdset_if_valid(miniSock->ssdpSock6, &rdSet);
        fdset_if_valid(miniSock->ssdpSock6UlaGua, &rdSet);
#ifdef COMPA_HAVE_CTRLPT_SSDP
        fdset_if_valid(miniSock->ssdpReqSock4, &rdSet);
        fdset_if_valid(miniSock->ssdpReqSock6, &rdSet);
#endif

        /* select() */
        int ret = umock::sys_socket_h.select(static_cast<int>(maxMiniSock),
                                             &rdSet, NULL, &expSet, NULL);

        if (ret == SOCKET_ERROR) {
            if (errno == EINTR) {
                // A signal was caught, not for us. We ignore it and
                continue;
            }
            if (errno == EBADF) {
                // A closed socket file descriptor was given in one of the
                // sets. For details look at
                // REF:_[Should_I_assert_fail_on_select()_EBADF?](https://stackoverflow.com/q/28015859/5014688)
                // It is difficult to determine here what file descriptor
                // in rdSet or expSet is invalid. So I ensure that only valid
                // socket fds are given by checking them with fdset_if_valid()
                // before calling select(). Doing this I have to
                continue;
            }
            // All other errors EINVAL and ENOMEM are critical and cannot
            // continue run mininserver.
            UPnPsdk_LOGCRIT("MSG1021") "Error in ::select(): "
                << std::strerror(errno) << ".\n";
            break;
        }

        // Accept requested connection from a remote control point and run the
        // connection in a new thread. Due to side effects with threading we
        // need to avoid lazy evaluation with chained || because all
        // web_server_accept() must be called.
        // if (ret1 == UPNP_E_SUCCESS || ret2 == UPNP_E_SUCCESS ||
        //     ret3 == UPNP_E_SUCCESS) {
        [[maybe_unused]] int ret1 =
            web_server_accept(miniSock->miniServerSock4, rdSet);
        [[maybe_unused]] int ret2 =
            web_server_accept(miniSock->miniServerSock6, rdSet);
        [[maybe_unused]] int ret3 =
            web_server_accept(miniSock->miniServerSock6UlaGua, rdSet);
#ifdef COMPA_HAVE_CTRLPT_SSDP
        ssdp_read(&miniSock->ssdpReqSock4, &rdSet);
        ssdp_read(&miniSock->ssdpReqSock6, &rdSet);
#endif
        ssdp_read(&miniSock->ssdpSock4, &rdSet);
        ssdp_read(&miniSock->ssdpSock6, &rdSet);
        ssdp_read(&miniSock->ssdpSock6UlaGua, &rdSet);
        // }

        // Check if we have received a packet from
        // localhost(127.0.0.1) that will stop the miniserver.
        stopSock = receive_from_stopSock(miniSock->miniServerStopSock, &rdSet);
    } // while (!stopsock)

    /* Close all sockets. */
    sock_close(miniSock->miniServerSock4);
    sock_close(miniSock->miniServerSock6);
    sock_close(miniSock->miniServerSock6UlaGua);
    sock_close(miniSock->miniServerStopSock);
    sock_close(miniSock->ssdpSock4);
    sock_close(miniSock->ssdpSock6);
    sock_close(miniSock->ssdpSock6UlaGua);
#ifdef COMPA_HAVE_CTRLPT_SSDP
    sock_close(miniSock->ssdpReqSock4);
    sock_close(miniSock->ssdpReqSock6);
#endif
    /* Free minisock. */
    umock::stdlib_h.free(miniSock);
    gMServState = MSERV_IDLE;

    UPnPsdk_LOGINFO("MSG1060") "Finished.\n";
    return;
}

void RunMiniServer_f(MiniServerSockArray* miniSock) {
    umock::pupnp_miniserver.RunMiniServer(miniSock);
}


/*!
 * \brief Returns port to which socket, sockfd, is bound.
 *
 * \returns
 *  On success: **0**\n
 *  On error: **-1** with unmodified system error (errno or WSAGetLastError()).
 */
int get_port(
    /*! [in] Socket descriptor. */
    SOCKET sockfd,
    /*! [out] The port value if successful, otherwise, untouched. */
    uint16_t* port) {
    TRACE("Executing get_port(), calls system ::getsockname()")
    UPnPsdk::sockaddr_t sockinfo{};
    socklen_t len(sizeof sockinfo); // May be modified by getsockname()

    if (umock::sys_socket_h.getsockname(sockfd, &sockinfo.sa, &len) == -1)
        // system error (errno etc.) is expected to be unmodified on return.
        return -1;

    switch (sockinfo.ss.ss_family) {
    case AF_INET:
    case AF_INET6:
        *port = ntohs(sockinfo.sin6.sin6_port);
        break;
    default:
        // system error (errno etc.) is expected to be unmodified on return.
        return -1;
    }
    UPnPsdk_LOGINFO("MSG1063") "sockfd=" << sockfd << ", port=" << *port
                                         << ".\n";

    return 0;
}

#ifdef COMPA_HAVE_WEBSERVER
/*!
 * \brief Create STREAM sockets, binds to local network interfaces and listens
 * for incoming connections.
 *
 * This is part of starting the miniserver and only available with the webserver
 * compiled in.
 *
 * \returns
 *  On success: UPNP_E_SUCCESS\n
 *  On error:
 *  - UPNP_E_OUTOF_SOCKET - Failed to create a socket.
 *  - UPNP_E_SOCKET_BIND - Bind() failed.
 *  - UPNP_E_LISTEN - Listen() failed.
 *  - UPNP_E_INTERNAL_ERROR - Port returned by the socket layer is < 0.
 */
int get_miniserver_sockets(
    /*! [in,out] [in]Pointer to a Socket Array that following members will be
     * filled:
     *      - .miniServerSock6
     *      - .miniServerPort6
     *      - .miniServerSock6UlaGua
     *      - .miniServerPort6UlaGua
     *      - .miniServerSock4
     *      - .miniServerPort4 */
    MiniServerSockArray* out,
    /*! [in] port on which the UPnP Device shall listen for incoming IPv4
       connections. If **0** then a random port number is returned in **out**.
     */
    in_port_t listen_port4,
    /*! [in] port on which the UPnP Device shall listen for incoming IPv6
       [LLA](\ref glossary_ipv6addr) connections. If **0** then a random port
       number is returned in **out**. */
    in_port_t listen_port6,
    /*! [in] port on which the UPnP Device shall listen for incoming IPv6
       [UAD](\ref glossary_ipv6addr) connections. If **0** then a random port
       number is returned in ***out**. */
    in_port_t listen_port6UlaGua) {
    UPnPsdk_LOGINFO("MSG1109") "Executing with listen_port4="
        << listen_port4 << ", listen_port6=" << listen_port6
        << ", listen_port6UlaGua=" << listen_port6UlaGua << ".\n";

    int retval{UPNP_E_OUTOF_SOCKET};

    if (out->pSockLlaObj != nullptr && gIF_IPV6[0] != '\0') {
        try {
            UPnPsdk::SSockaddr saObj;
            if (::strncmp(gIF_IPV6, "::1", 3) == 0)
                // The loopback address belongs to a lla but 'bind()' on win32
                // does not accept it with scope id.
                saObj = '[' + std::string(gIF_IPV6) +
                        "]:" + std::to_string(listen_port6);
            else
                saObj = '[' + std::string(gIF_IPV6) + '%' +
                        std::to_string(gIF_INDEX) +
                        "]:" + std::to_string(listen_port6);
            out->pSockLlaObj->bind(SOCK_STREAM, &saObj, AI_PASSIVE);
            out->pSockLlaObj->listen();
            out->miniServerSock6 = *out->pSockLlaObj;
            out->pSockLlaObj->local_saddr(&saObj);
            out->miniServerPort6 = saObj.port();
            retval = UPNP_E_SUCCESS;
        } catch (const std::exception& ex) {
            UPnPsdk_LOGCATCH("MSG1110") "gIF_IPV6=\""
                << gIF_IPV6 << "\", catched next line...\n"
                << ex.what();
        }
    }

    // ss6.serverAddr6->sin6_scope_id = gIF_INDEX;
    // I could not find where sin6_scope_id is read elsewhere to use it.
    // It is never copied to 'out'. --Ingo

    if (out->pSockGuaObj != nullptr && gIF_IPV6_ULA_GUA[0] != '\0') {
        try {
            UPnPsdk::SSockaddr saObj;
            saObj = '[' + std::string(gIF_IPV6_ULA_GUA) +
                    "]:" + std::to_string(listen_port6UlaGua);
            out->pSockGuaObj->bind(SOCK_STREAM, &saObj, AI_PASSIVE);
            out->pSockGuaObj->listen();
            out->miniServerSock6UlaGua = *out->pSockGuaObj;
            out->pSockGuaObj->local_saddr(&saObj);
            out->miniServerPort6UlaGua = saObj.port();
            retval = UPNP_E_SUCCESS;
        } catch (const std::exception& ex) {
            UPnPsdk_LOGCATCH("MSG1117") "gIF_IPV6_ULA_GUA=\""
                << gIF_IPV6_ULA_GUA << "\", catched next line...\n"
                << ex.what();
        }
    }

    if (out->pSockIp4Obj != nullptr && gIF_IPV4[0] != '\0') {
        try {
            UPnPsdk::SSockaddr saObj;
            saObj = std::string(gIF_IPV4) + ':' + std::to_string(listen_port4);
            out->pSockIp4Obj->bind(SOCK_STREAM, &saObj, AI_PASSIVE);
            out->pSockIp4Obj->listen();
            out->miniServerSock4 = *out->pSockIp4Obj;
            out->pSockIp4Obj->local_saddr(&saObj);
            out->miniServerPort4 = saObj.port();
            retval = UPNP_E_SUCCESS;
        } catch (const std::exception& ex) {
            UPnPsdk_LOGCATCH("MSG1114") "gIF_IPV4=\""
                << gIF_IPV4 << "\", catched next line...\n"
                << ex.what();
        }
    }

    if (retval != UPNP_E_SUCCESS)
        UPnPsdk_LOGERR("MSG1065") "No valid IP address on a local network "
                                  "adapter found for listening.\n";
    return retval;
}
#endif /* COMPA_HAVE_WEBSERVER */

/*!
 * \brief Creates the miniserver STOP socket, usable to listen for stopping the
 * miniserver.
 *
 * This datagram socket is created and bound to "localhost". It does not listen
 * now but will later be used to listen on to know when it is time to stop the
 * Miniserver.
 *
 * \returns
 *  On success: UPNP_E_SUCCESS\n
 *  On error:
 *  - UPNP_E_OUTOF_SOCKET: Failed to create a socket.
 *  - UPNP_E_SOCKET_BIND: Bind() failed.
 *  - UPNP_E_INTERNAL_ERROR: Port returned by the socket layer is < 0.
 */
int get_miniserver_stopsock(
    /*! [out] Fills the socket file descriptor and the port it is bound to into
       the structure. */
    MiniServerSockArray* out) {
    TRACE("Executing get_miniserver_stopsock()");
    sockaddr_in stop_sockaddr;

    UPnPsdk::CSocketErr sockerrObj;
    SOCKET miniServerStopSock =
        umock::sys_socket_h.socket(AF_INET, SOCK_DGRAM, 0);
    if (miniServerStopSock == INVALID_SOCKET) {
        sockerrObj.catch_error();
        UPnPsdk_LOGCRIT("MSG1094") "Error in socket(): "
            << sockerrObj.error_str() << "\n";
        return UPNP_E_OUTOF_SOCKET;
    }
    /* Bind to local socket. */
    memset(&stop_sockaddr, 0, sizeof(stop_sockaddr));
    stop_sockaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &stop_sockaddr.sin_addr);
    int ret = umock::sys_socket_h.bind(
        miniServerStopSock, reinterpret_cast<sockaddr*>(&stop_sockaddr),
        sizeof(stop_sockaddr));
    if (ret == SOCKET_ERROR) {
        sockerrObj.catch_error();
        UPnPsdk_LOGCRIT("MSG1095") "Error in binding localhost: "
            << sockerrObj.error_str() << "\n";
        sock_close(miniServerStopSock);
        return UPNP_E_SOCKET_BIND;
    }
    ret = get_port(miniServerStopSock, &miniStopSockPort);
    if (ret < 0) {
        sock_close(miniServerStopSock);
        return UPNP_E_INTERNAL_ERROR;
    }
    out->miniServerStopSock = miniServerStopSock;
    out->stopPort = miniStopSockPort;

    UPnPsdk_LOGINFO("MSG1053") "Bound stop socket="
        << miniServerStopSock << " to \"127.0.0.1:" << miniStopSockPort
        << "\".\n";

    return UPNP_E_SUCCESS;
}

/*!
 * \brief Initialize a miniserver Socket Array.
 */
void InitMiniServerSockArray(
    MiniServerSockArray* miniSocket ///< Pointer to a miniserver Socket Array.
) {
    TRACE("Executing InitMiniServerSockArray()");
    miniSocket->miniServerSock4 = INVALID_SOCKET;
    miniSocket->miniServerSock6 = INVALID_SOCKET;
    miniSocket->miniServerSock6UlaGua = INVALID_SOCKET;
    miniSocket->miniServerStopSock = INVALID_SOCKET;
    miniSocket->ssdpSock4 = INVALID_SOCKET;
    miniSocket->ssdpSock6 = INVALID_SOCKET;
    miniSocket->ssdpSock6UlaGua = INVALID_SOCKET;
    miniSocket->stopPort = 0u;
    miniSocket->miniServerPort4 = 0u;
    miniSocket->miniServerPort6 = 0u;
    miniSocket->miniServerPort6UlaGua = 0u;
#ifdef COMPA_HAVE_CTRLPT_SSDP
    miniSocket->ssdpReqSock4 = INVALID_SOCKET;
    miniSocket->ssdpReqSock6 = INVALID_SOCKET;
#endif
#ifdef COMPA_HAVE_WEBSERVER
    miniSocket->pSockLlaObj = nullptr;
    miniSocket->pSockGuaObj = nullptr;
    miniSocket->pSockIp4Obj = nullptr;
#endif
}

/// @} // Functions (scope restricted to file)
} // anonymous namespace


#ifdef COMPA_HAVE_DEVICE_SOAP
void SetSoapCallback(MiniServerCallback callback) {
    TRACE("Executing SetSoapCallback()");
    gSoapCallback = callback;
}
#endif

#ifdef COMPA_HAVE_DEVICE_GENA
void SetGenaCallback(MiniServerCallback callback) {
    TRACE("Executing SetGenaCallback()");
    gGenaCallback = callback;
}
#endif

int StartMiniServer([[maybe_unused]] in_port_t* listen_port4,
                    [[maybe_unused]] in_port_t* listen_port6,
                    [[maybe_unused]] in_port_t* listen_port6UlaGua) {
    UPnPsdk_LOGINFO("MSG1068") "Executing...\n";
    constexpr int max_count{10000};
    MiniServerSockArray* miniSocket;
    ThreadPoolJob job;
    int ret_code{UPNP_E_INTERNAL_ERROR};

    memset(&job, 0, sizeof(job));

    if (gMServState != MSERV_IDLE) {
        /* miniserver running. */
        UPnPsdk_LOGERR("MSG1087") "Cannot start. Miniserver is running.\n";
        return UPNP_E_INTERNAL_ERROR;
    }

    miniSocket = (MiniServerSockArray*)malloc(sizeof(MiniServerSockArray));
    if (!miniSocket) {
        return UPNP_E_OUTOF_MEMORY;
    }
    InitMiniServerSockArray(miniSocket);

#ifdef COMPA_HAVE_WEBSERVER
    // These socket objects must be valid until the miniserver is successful
    // started. They will be moved to the running miniserver thread.
    UPnPsdk::CSocket sockLlaObj;
    miniSocket->pSockLlaObj = &sockLlaObj;
    UPnPsdk::CSocket sockGuaObj;
    miniSocket->pSockGuaObj = &sockGuaObj;
    UPnPsdk::CSocket sockIp4Obj;
    miniSocket->pSockIp4Obj = &sockIp4Obj;

    /* V4 and V6 http listeners. */
    ret_code = get_miniserver_sockets(miniSocket, *listen_port4, *listen_port6,
                                      *listen_port6UlaGua);
    if (ret_code != UPNP_E_SUCCESS) {
        free(miniSocket);
        return ret_code;
    }
#endif

    /* Stop socket (To end miniserver processing). */
    ret_code = get_miniserver_stopsock(miniSocket);
    if (ret_code != UPNP_E_SUCCESS) {
        sock_close(miniSocket->miniServerSock4);
        sock_close(miniSocket->miniServerSock6);
        sock_close(miniSocket->miniServerSock6UlaGua);
        free(miniSocket);
        return ret_code;
    }
#if defined(COMPA_HAVE_CTRLPT_SSDP) || defined(COMPA_HAVE_DEVICE_SSDP)
    /* SSDP socket for discovery/advertising. */
    ret_code = umock::pupnp_ssdp.get_ssdp_sockets(miniSocket);
    if (ret_code != UPNP_E_SUCCESS) {
        sock_close(miniSocket->miniServerSock4);
        sock_close(miniSocket->miniServerSock6);
        sock_close(miniSocket->miniServerSock6UlaGua);
        sock_close(miniSocket->miniServerStopSock);
        free(miniSocket);
        return ret_code;
    }
#endif
    // Run miniserver in a new thread.
    TPJobInit(&job, (start_routine)RunMiniServer_f, (void*)miniSocket);
    TPJobSetPriority(&job, MED_PRIORITY);
    TPJobSetFreeFunction(&job, (free_routine)free);
    ret_code = ThreadPoolAddPersistent(&gMiniServerThreadPool, &job, NULL);
    if (ret_code != 0) {
        sock_close(miniSocket->miniServerSock4);
        sock_close(miniSocket->miniServerSock6);
        sock_close(miniSocket->miniServerSock6UlaGua);
        sock_close(miniSocket->miniServerStopSock);
        sock_close(miniSocket->ssdpSock4);
        sock_close(miniSocket->ssdpSock6);
        sock_close(miniSocket->ssdpSock6UlaGua);
#ifdef COMPA_HAVE_CTRLPT_SSDP
        sock_close(miniSocket->ssdpReqSock4);
        sock_close(miniSocket->ssdpReqSock6);
#endif
        free(miniSocket);
        return UPNP_E_OUTOF_MEMORY;
    }
    /* Wait for miniserver to start. */
    int count{0};
    while (gMServState != (MiniServerState)MSERV_RUNNING && count < max_count) {
        /* 0.05s */
        imillisleep(50);
        count++;
    }
    if (count >= max_count) {
        /* Took it too long to start that thread. */
        sock_close(miniSocket->miniServerSock4);
        sock_close(miniSocket->miniServerSock6);
        sock_close(miniSocket->miniServerSock6UlaGua);
        sock_close(miniSocket->miniServerStopSock);
        sock_close(miniSocket->ssdpSock4);
        sock_close(miniSocket->ssdpSock6);
        sock_close(miniSocket->ssdpSock6UlaGua);
#ifdef COMPA_HAVE_CTRLPT_SSDP
        sock_close(miniSocket->ssdpReqSock4);
        sock_close(miniSocket->ssdpReqSock6);
#endif
        return UPNP_E_INTERNAL_ERROR;
    }
#ifdef COMPA_HAVE_WEBSERVER
    *listen_port4 = miniSocket->miniServerPort4;
    *listen_port6 = miniSocket->miniServerPort6;
    *listen_port6UlaGua = miniSocket->miniServerPort6UlaGua;
#endif

    return UPNP_E_SUCCESS;
}

int StopMiniServer() {
    UpnpPrintf(UPNP_INFO, MSERV, __FILE__, __LINE__,
               "Inside StopMiniServer()\n");

    socklen_t socklen = sizeof(struct sockaddr_in);
    SOCKET sock;
    sockaddr_in ssdpAddr;
    char buf[256] = "ShutDown";
    // due to required type cast for 'sendto' on WIN32 bufLen must fit to an int
    size_t bufLen = strlen(buf);

    switch (gMServState) {
    case MSERV_RUNNING:
        gMServState = MSERV_STOPPING;
        break;
    default:
        return 0;
    }
    sock = umock::sys_socket_h.socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        UpnpPrintf(UPNP_ERROR, SSDP, __FILE__, __LINE__,
                   "SSDP_SERVER: StopSSDPServer: Error in socket() %s\n",
                   std::strerror(errno));
        return 1;
    }
    while (gMServState != (MiniServerState)MSERV_IDLE) {
        ssdpAddr.sin_family = (sa_family_t)AF_INET;
        inet_pton(AF_INET, "127.0.0.1", &ssdpAddr.sin_addr);
        ssdpAddr.sin_port = htons(miniStopSockPort);
        umock::sys_socket_h.sendto(sock, buf, (SIZEP_T)bufLen, 0,
                                   reinterpret_cast<sockaddr*>(&ssdpAddr),
                                   socklen);
        imillisleep(1);
        if (gMServState == (MiniServerState)MSERV_IDLE) {
            break;
        }
        isleep(1u);
    }

    sock_close(sock);
    return 0;
}
