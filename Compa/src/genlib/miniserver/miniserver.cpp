/**************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (C) 2012 France Telecom All rights reserved.
 * Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
 * Redistribution only with this Copyright remark. Last modified: 2024-05-11
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

#include <miniserver.hpp>

#include <httpreadwrite.hpp>
#include <ssdp_common.hpp>
#include <statcodes.hpp>
#include <upnpapi.hpp>

#include <upnplib/socket.hpp>
#include <upnplib/global.hpp>
#include <upnplib/synclog.hpp>

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

#ifdef COMPA_HAVE_WEBSERVER
/*! \brief First dynamic and/or private port from 49152 to 65535 used by the
 * library.\n
 * Only available with webserver compiled in. */
constexpr in_port_t APPLICATION_LISTENING_PORT{49152};
#endif

/*! \brief miniserver received request message.
 * \details This defines the structure of a UPnP request that has income from a
 * remote control point.
 */
struct mserv_request_t {
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

/*! \brief Flag if to immediately reuse the netaddress of a just broken
 * connetion. Reuse address is not supported. */
#if !defined(UPNP_MINISERVER_REUSEADDR) || defined(DOXYGEN_RUN)
constexpr bool MINISERVER_REUSEADDR{false};
#else
constexpr bool MINISERVER_REUSEADDR{true};
#endif

/// \brief additional management information to a socket.
struct s_SocketStuff {
    /// @{
    /// \brief member variable
    int ip_version;
    const char* text_addr;
    sockaddr_storage ss;
    union {
        /// @{
        /// \brief member variable
        sockaddr* serverAddr;
        sockaddr_in* serverAddr4;
        sockaddr_in6* serverAddr6;
        /// @}
    };
    SOCKET fd;
    in_port_t try_port;
    in_port_t actual_port;
    socklen_t address_len;
    /// @}
};


/*! \name Scope restricted to file
 * @{ */

/*!
 * \brief Check if a network address is numeric.
 *
 * An empty netaddress or an unspecified one ("[::]", "0.0.0.0") is not valid.
 */
// No unit test needed. It's tested with SSockaddr.
int host_header_is_numeric(
    char* a_host_port,     ///< network address
    size_t a_host_port_len ///< length of a_host_port excl. terminating '\0'.
) {
    TRACE("Executing host_header_is_numeric()");
    if (a_host_port_len == 0 || strncmp(a_host_port, "[::]", 4) == 0 ||
        strncmp(a_host_port, "0.0.0.0", 7) == 0)
        return 0;

    upnplib::SSockaddr saddrObj;
    try {
        saddrObj = std::string(a_host_port, a_host_port_len);
    } catch (const std::exception& e) {
        UPNPLIB_LOGCATCH "MSG1049: " << e.what() << "\n";
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
    char* a_host_port, ///< [out] Pointer to buffer that will be filled.
    size_t a_hp_size   ///< [in] size of the buffer.
) {
    TRACE("Executing getNumericHostRedirection()")
    try {
        upnplib::CSocket_basic socketObj;
        socketObj.set(a_socket);
        upnplib::netaddr_t host_port = socketObj.get_netaddrp();
        memcpy(a_host_port, host_port.c_str(), a_hp_size);
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
        UPNPLIB_LOGINFO "MSG1107: miniserver socket="
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
    if (upnplib::g_dbug) {
        getNumericHostRedirection(info->socket, host_port, sizeof host_port);
        UPNPLIB_LOGINFO "MSG1113: Redirect host_port=\"" << host_port << "\"\n";
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
    /*! [in] Received Request Message to be handled. */
    void* args) { // Expected to be mserv_request_t*
    SOCKINFO info;
    int http_error_code;
    int ret_code;
    int major = 1;
    int minor = 1;
    http_parser_t parser;
    http_message_t* hmsg = NULL;
    int timeout = HTTP_DEFAULT_TIMEOUT;
    mserv_request_t* request_in = (mserv_request_t*)args;
    SOCKET connfd = request_in->connfd;

    UPNPLIB_LOGINFO "MSG1027: Miniserver socket "
        << connfd << ": READING request from client...\n";
    /* parser_request_init( &parser ); */ /* LEAK_FIX_MK */
    hmsg = &parser.msg;
    ret_code = sock_init_with_ip(&info, connfd,
                                 (sockaddr*)&request_in->foreign_sockaddr);
    if (ret_code != UPNP_E_SUCCESS) {
        free(request_in);
        httpmsg_destroy(hmsg);
        return;
    }

    /* read */
    ret_code = http_RecvMessage(&info, &parser, HTTPMETHOD_UNKNOWN, &timeout,
                                &http_error_code);
    if (ret_code != 0) {
        goto error_handler;
    }

    UPNPLIB_LOGINFO "MSG1106: miniserver socket=" << connfd
                                                  << ": PROCESSING...\n";
    /* dispatch */
    http_error_code = dispatch_request(&info, &parser);
    if (http_error_code != 0) {
        goto error_handler;
    }
    http_error_code = 0;

error_handler:
    if (http_error_code > 0) {
        if (hmsg) {
            major = hmsg->major_version;
            minor = hmsg->minor_version;
        }
        // BUG! Don't try to send a status response to a remote client with
        // http error (e.g. 400) if we have a socket error. It doesn't make
        // sense. --Ingo
        http_SendStatusResponse(&info, http_error_code, major, minor);
    }
    sock_destroy(&info, SD_BOTH);
    httpmsg_destroy(hmsg);
    free(request_in);

    UpnpPrintf(UPNP_INFO, MSERV, __FILE__, __LINE__,
               "miniserver %d: COMPLETE\n", connfd);
}

/*!
 * \brief Initilize the thread pool to handle a request, sets priority for the
 * job and adds the job to the thread pool.
 */
UPNP_INLINE void schedule_request_job(
    /*! [in] Socket Descriptor on which connection is accepted. */
    SOCKET connfd,
    /*! [in] Clients Address information. */
    sockaddr* clientAddr) {
    TRACE("Executing schedule_request_job()")
    UPNPLIB_LOGINFO "MSG1042: Schedule request job to host "
        << upnplib::to_netaddrp(
               reinterpret_cast<const sockaddr_storage*>(clientAddr))
        << " with socket " << connfd << ".\n";

    ThreadPoolJob job{};
    mserv_request_t* request{
        static_cast<mserv_request_t*>(std::malloc(sizeof(mserv_request_t)))};

    if (request == nullptr) {
        UPNPLIB_LOGCRIT "MSG1024: Socket " << connfd << ": out of memory.\n";
        sock_close(connfd);
        return;
    }

    request->connfd = connfd;
    memcpy(&request->foreign_sockaddr, clientAddr,
           sizeof(request->foreign_sockaddr));
    TPJobInit(&job, (start_routine)handle_request, request);
    TPJobSetFreeFunction(&job, free_handle_request_arg);
    TPJobSetPriority(&job, MED_PRIORITY);
    if (ThreadPoolAdd(&gMiniServerThreadPool, &job, NULL) != 0) {
        UPNPLIB_LOGERR "MSG1025: Socket " << connfd
                                          << ": cannot schedule request.\n";
        free(request);
        sock_close(connfd);
        return;
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
    UPNPLIB_LOGINFO "MSG1086: Check sockfd=" << a_sock << ".\n";
    if (a_sock == INVALID_SOCKET)
        // This is a defined state and we return silently.
        return;

    if (a_sock < 3 || a_sock >= FD_SETSIZE) {
        UPNPLIB_LOGERR "MSG1005: "
            << (a_sock < 0 ? "Invalid" : "Prohibited") << " socket " << a_sock
            << " not set to be monitored by ::select()"
            << (a_sock >= 3 ? " because it violates FD_SETSIZE.\n" : ".\n");
        return;
    }
    // Check if socket is valid and bound
    try {
        upnplib::CSocket_basic sockObj;
        sockObj.set(a_sock);
        if (sockObj.is_bound())

            FD_SET(a_sock, a_set);

        else
            UPNPLIB_LOGINFO "MSG1002: Unbound socket "
                << a_sock << " not set to be monitored by ::select().\n";

    } catch (const std::exception& e) {
        if (upnplib::g_dbug)
            std::clog << e.what();
        UPNPLIB_LOGCATCH "MSG1009: Invalid socket "
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
    /// [in] Socket file descriptor.
    [[maybe_unused]] SOCKET lsock,
    /// [out] Reference to a file descriptor set as needed for \::select().
    [[maybe_unused]] fd_set& set) {
#ifndef COMPA_HAVE_WEBSERVER
    return UPNP_E_NO_WEB_SERVER;
#else
    TRACE("Executing web_server_accept()")
    if (lsock == INVALID_SOCKET || !FD_ISSET(lsock, &set)) {
        UPNPLIB_LOGINFO "MSG1012: Socket("
            << lsock << ") invalid or not in file descriptor set.\n";
        return UPNP_E_SOCKET_ERROR;
    }

    SOCKET asock;
    upnplib::sockaddr_t clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    // accept a network request connection
    asock = umock::sys_socket_h.accept(lsock, &clientAddr.sa, &clientLen);
    if (asock == INVALID_SOCKET) {
        UPNPLIB_LOGERR "MSG1022: Error in ::accept(): " << std::strerror(errno)
                                                        << ".\n";
        return UPNP_E_SOCKET_ACCEPT;
    }

    // Schedule a job to manage a UPnP request from a remote host.
    char buf_ntop[INET6_ADDRSTRLEN + 7];
    inet_ntop(AF_INET, &clientAddr.sin.sin_addr, buf_ntop, sizeof(buf_ntop));
    UPNPLIB_LOGINFO "MSG1023: Connected to host "
        << buf_ntop << ":" << ntohs(clientAddr.sin.sin_port) << " with socket "
        << asock << ".\n";
    schedule_request_job(asock, &clientAddr.sa);

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

    upnplib::sockaddr_t clientAddr{};
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
        UPNPLIB_LOGCRIT "MSG1038: Failed to receive data from socket "
            << ssock << ". Stop miniserver.\n";
        return 1;
    }

    // 16777343 are netorder bytes of "127.0.0.1"
    if (clientAddr.sin.sin_addr.s_addr != 16777343 ||
        strcmp(receiveBuf, shutdown_str) != 0) //
    {
        char nullstr[]{"\\0"};
        if (byteReceived == 0 || receiveBuf[byteReceived - 1] != '\0')
            nullstr[0] = '\0';
        UPNPLIB_LOGERR "MSG1039: Received \""
            << receiveBuf << nullstr << "\" from " << buf_ntop << ":"
            << ntohs(clientAddr.sin.sin_port)
            << ", must be \"ShutDown\\0\" from 127.0.0.1:*. Don't "
               "stopping miniserver.\n";
        return 0;
    }

    UPNPLIB_LOGINFO "MSG1040: On socket "
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
    UPNPLIB_LOGINFO "MSG1085: Executing...\n";
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
            UPNPLIB_LOGCRIT "MSG1021: Error in ::select(): "
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

    return;
}

void RunMiniServer_f(MiniServerSockArray* miniSock) {
    umock::pupnp_miniserver.RunMiniServer(miniSock);
}


/*!
 * \brief Returns port to which socket, sockfd, is bound.
 *
 * \returns
 *  On success: **0**
 *  On error: **-1** with unmodified system error (errno or WSAGetLastError()).
 */
int get_port(
    /*! [in] Socket descriptor. */
    SOCKET sockfd,
    /*! [out] The port value if successful, otherwise, untouched. */
    uint16_t* port) {
    TRACE("Executing get_port(), calls system ::getsockname()")
    upnplib::sockaddr_t sockinfo{};
    socklen_t len(sizeof sockinfo); // May be modified by getsockname()

    if (umock::sys_socket_h.getsockname(sockfd, &sockinfo.sa, &len) == -1)
        // system error (errno etc.) is expected to be unmodified on return.
        return -1;

    switch (sockinfo.ss.ss_family) {
    case AF_INET:
        *port = ntohs(sockinfo.sin.sin_port);
        break;
    case AF_INET6:
        *port = ntohs(sockinfo.sin6.sin6_port);
        break;
    default:
        // system error (errno etc.) is expected to be unmodified on return.
        return -1;
    }
    UPNPLIB_LOGINFO "MSG1063: sockfd=" << sockfd << ", port=" << *port << ".\n";

    return 0;
}

#ifdef COMPA_HAVE_WEBSERVER
/*!
 * \brief Get valid sockets.
 *
 * \returns
 *  On success: **0**\n
 *  On error: **1**
 *
 *  \todo Detect wrong ip address, e.g. "2001:db8::1::2" on Microsoft Windows.
 */
int init_socket_suff(
    /*! [out] Pointer to a structure that will be filled with a valid socket
       file descriptor and additional management information. */
    s_SocketStuff* s,
    /*! [in] IP address as character string. */
    const char* text_addr,
    /*! [in] Version number 4 or 6 for the used ip stack. */
    int ip_version) {
    UPNPLIB_LOGINFO "MSG1067: Executing with ip_address=\""
        << text_addr << "\", ip_version=" << ip_version << ".\n";
    int sockError;
    sa_family_t domain{0};
    void* addr; // This holds a pointer to sin_addr
    int reuseaddr_on = MINISERVER_REUSEADDR;
    upnplib::CSocketErr sockerrObj;

    memset(s, 0, sizeof *s);
    s->fd = INVALID_SOCKET;
    s->ip_version = ip_version;
    s->text_addr = text_addr;
    s->serverAddr = (sockaddr*)&s->ss;
    switch (ip_version) {
    case 4:
        domain = AF_INET;
        s->serverAddr4->sin_family = domain;
        s->address_len = sizeof *s->serverAddr4;
        addr = &s->serverAddr4->sin_addr;
        break;
    case 6:
        domain = AF_INET6;
        s->serverAddr6->sin6_family = domain;
        s->address_len = sizeof *s->serverAddr6;
        addr = &s->serverAddr6->sin6_addr;
        break;
    default:
        UPNPLIB_LOGCRIT "MSG1050: Invalid IP version: " << ip_version << ".\n";
        goto error;
        break;
    }

    if (inet_pton(domain, text_addr, addr) <= 0) {
        UPNPLIB_LOGINFO "MSG1051: Invalid ip_address: \"" << text_addr
                                                          << "\".\n";
        // Here we also meet an empty ("") ip_address.
        goto error;
    }
    s->fd = umock::sys_socket_h.socket(domain, SOCK_STREAM, 0);

    if (s->fd == INVALID_SOCKET) {
        sockerrObj.catch_error();
        UPNPLIB_LOGERR "MSG1054: IPv"
            << ip_version
            << " socket not available: " << sockerrObj.get_error_str() << "\n";
        goto error;

    } else if (ip_version == 6) {
        int onOff = 1;

        sockError = umock::sys_socket_h.setsockopt(
            s->fd, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&onOff, sizeof(onOff));
        if (sockError == SOCKET_ERROR) {
            sockerrObj.catch_error();
            UPNPLIB_LOGCRIT "MSG1055: unable to set IPv6 socket protocol: "
                << sockerrObj.get_error_str() << "\n";
            goto error;
        }
    }
    /* Getting away with implementation of re-using address:port and
     * instead choosing to increment port numbers.
     * Keeping the re-use address code as an optional behaviour that
     * can be turned on if necessary.
     * TURN ON the reuseaddr_on option to use the option. */
    if (MINISERVER_REUSEADDR) {
        sockError = umock::sys_socket_h.setsockopt(
            s->fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseaddr_on,
            sizeof(int));
        if (sockError == SOCKET_ERROR) {
            sockerrObj.catch_error();
            UPNPLIB_LOGERR "MSG1056: unable to set SO_REUSEADDR: "
                << sockerrObj.get_error_str() << "\n";
            goto error;
        }
    }

    return 0;

error:
    if (s->fd != INVALID_SOCKET) {
        sock_close(s->fd);
    }
    s->fd = INVALID_SOCKET;

    // return errval; // errval is available here but to be compatible I use
    return 1; // --Ingo
}

/*!
 * \brief Bind a socket address to a local network interface.
 *
 * **s->port** will be one more than the used port in the end. This is
 * important, in case this function is called again. It is expected to have a
 * prechecked valid parameter.\n
 * Only available with the webserver compiled in.
 *
 * \returns
 *  On success: UPNP_E_SUCCESS\n
 *  On error:
 *  - UPNP_E_INVALID_PARAM
 *  - UPNP_E_SOCKET_BIND
 */
int do_bind(         //
    s_SocketStuff* s ///< [in] Information for the socket.
) {
    UPNPLIB_LOGINFO "MSG1026: Executing...\n";
    int ret_val{UPNP_E_SUCCESS};
    int bind_error;
    in_port_t original_listen_port = s->try_port;

    bool repeat_do{false};
    do {
        switch (s->ip_version) {
        case 4:
            // Compilation Error on macOS:
            // operation on 's->s_SocketStuff::try_port' may be undefined
            // [-Werror=sequence-point] s->serverAddr4->sin_port =
            // htons(s->try_port++); --Ingo
            s->serverAddr4->sin_port = htons(s->try_port);
            s->actual_port = s->try_port;
            s->try_port = s->try_port + 1;
            break;
        case 6:
            // Compilation Error on macOS:
            // operation on 's->s_SocketStuff::try_port' may be undefined
            // [-Werror=sequence-point] s->serverAddr6->sin6_port =
            // htons(s->try_port++); --Ingo
            s->serverAddr6->sin6_port = htons(s->try_port);
            s->actual_port = s->try_port;
            s->try_port = s->try_port + 1;
            break;
        default:
            ret_val = UPNP_E_INVALID_PARAM;
            goto error;
        }
        if (s->try_port == 0)
            s->try_port = APPLICATION_LISTENING_PORT;

        // Bind socket
        bind_error =
            umock::sys_socket_h.bind(s->fd, s->serverAddr, s->address_len);

        if (bind_error != 0)
#ifdef _MSC_VER
            repeat_do = (umock::winsock2_h.WSAGetLastError() == WSAEADDRINUSE)
                            ? true
                            : false;
#else
            repeat_do = (errno == EADDRINUSE) ? true : false;
#endif
    } while (repeat_do && s->try_port >= original_listen_port);

    if (bind_error != 0) {
#ifdef _MSC_VER
        UPNPLIB_LOGERR "MSG1029: Error with IPv"
            << s->ip_version << " returned from ::bind() = "
            << umock::winsock2_h.WSAGetLastError() << ".\n";
#else
        UPNPLIB_LOGERR "MSG1032: Error with IPv"
            << s->ip_version
            << " returned from ::bind() = " << std::strerror(errno) << ".\n";
#endif
        /* Bind failed. */
        ret_val = UPNP_E_SOCKET_BIND;
        goto error;
    }

    return UPNP_E_SUCCESS;

error:
    return ret_val;
}

/*!
 * \brief Listen on a local network interface to incomming requests.
 *
 * Only available with the webserver compiled in.
 *
 * \returns
 *  On success: UPNP_E_SUCCESS\n
 *  On error:
 *  - UPNP_E_LISTEN
 *  - UPNP_E_INTERNAL_ERROR
 */
int do_listen(       //
    s_SocketStuff* s ///< [in] Information for the socket.
) {
    UPNPLIB_LOGINFO "MSG1052: Executing...\n";
    int ret_val;
    int listen_error;
    int port_error;
    upnplib::CSocketErr sockerrObj;

    listen_error = umock::sys_socket_h.listen(s->fd, SOMAXCONN);
    if (listen_error == -1) {
        sockerrObj.catch_error();
        UPNPLIB_LOGERR "MSG1096: Netaddress=\""
            << s->text_addr << ":" << s->actual_port << "\", "
            << sockerrObj.get_error_str() << "\n";
        ret_val = UPNP_E_LISTEN;
        goto error;
    }
    port_error = get_port(s->fd, &s->actual_port);
    if (port_error < 0) {
        sockerrObj.catch_error();
        UPNPLIB_LOGERR "MSG1097: Error in IPv"
            << s->ip_version << " get_port(): " << sockerrObj.get_error_str()
            << "\n";
        ret_val = UPNP_E_INTERNAL_ERROR;
        goto error;
    }

    return UPNP_E_SUCCESS;

error:
    return ret_val;
}

/*!
 * \brief Close and (re)initialize a socket.
 *
 * Only available with the webserver compiled in.
 *
 * \returns
 *  On success: UPNP_E_SUCCESS\n
 *  On error: 1
 */
int do_reinit(       //
    s_SocketStuff* s ///< [in] Information for the socket.
) {
    TRACE("Executing do_reinit()");
    sock_close(s->fd);

    return init_socket_suff(s, s->text_addr, s->ip_version);
}

/*!
 * \brief Bind and listen to a socket address.
 *
 * Only available with the webserver compiled in.
 *
 * \returns
 *  On success: UPNP_E_SUCCESS\n
 *  On error.
 *  - UPNP_E_INVALID_PARAM
 *  - UPNP_E_SOCKET_BIND
 *  - UPNP_E_LISTEN
 *  - UPNP_E_INTERNAL_ERROR
 */
int do_bind_listen(  //
    s_SocketStuff* s ///< [in] Information for the socket.
) {
    UPNPLIB_LOGINFO "MSG1062: Executing...\n";

    int ret_val;
    int ok = 0;
    uint16_t original_port = s->try_port;

    while (!ok) {
        ret_val = do_bind(s);
        if (ret_val) {
            goto error;
        }
        ret_val = do_listen(s);
        if (ret_val) {
            if (errno == EADDRINUSE) {
                do_reinit(s);
                s->try_port = original_port;
                continue;
            }
            goto error;
        }
        ok = s->try_port >= original_port;
    }

    return UPNP_E_SUCCESS;

error:
    return ret_val;
}

/*!
 * \brief Creates a STREAM socket, binds to INADDR_ANY and listens for incoming
 * connections.
 *
 * Returns the actual port which the sockets sub-system returned. Also creates a
 * DGRAM socket, binds to the loop back address and returns the port allocated
 * by the socket sub-system.\n
 * Only available with the webserver compiled in.
 *
 * \returns
 *  On success: UPNP_E_SUCCESS\n
 *  On error:
 *  - UPNP_E_OUTOF_SOCKET - Failed to create a socket.
 *  - UPNP_E_SOCKET_BIND - Bind() failed.
 *  - UPNP_E_LISTEN - Listen() failed.
 *  - UPNP_E_INTERNAL_ERROR - Port returned by the socket layer is < 0.
 */
[[maybe_unused]] int get_miniserver_sockets(
    /*! [out] Pointer to a Socket Array that will be filled. */
    MiniServerSockArray* out,
    /*! [in] port on which the server is listening for incoming IPv4
     * connections. */
    in_port_t listen_port4,
    /*! [in] port on which the server is listening for incoming IPv6
     * ULA connections. */
    in_port_t listen_port6,
    /*! [in] port on which the server is listening for incoming
     * IPv6 ULA or GUA connections. */
    in_port_t listen_port6UlaGua) {
    UPNPLIB_LOGINFO "MSG1064: Executing with listen_port4="
        << listen_port4 << ", listen_port6=" << listen_port6
        << ", listen_port6UlaGua=" << listen_port6UlaGua << ".\n";
    int ret_val{UPNP_E_INTERNAL_ERROR};
    int err_init_4;
    int err_init_6;
    int err_init_6UlaGua;

    s_SocketStuff ss4;
    s_SocketStuff ss6;
    s_SocketStuff ss6UlaGua;

    /* Create listen socket for IPv4/IPv6. An error here may indicate
     * that we don't have an IPv4/IPv6 stack. */
    err_init_4 = init_socket_suff(&ss4, gIF_IPV4, 4);
    err_init_6 = init_socket_suff(&ss6, gIF_IPV6, 6);
    err_init_6UlaGua = init_socket_suff(&ss6UlaGua, gIF_IPV6_ULA_GUA, 6);
    ss6.serverAddr6->sin6_scope_id = gIF_INDEX;
    /* Check what happened. */
    if (err_init_4 && err_init_6 && err_init_6UlaGua) {
        UPNPLIB_LOGCRIT
        "MSG1070: No suitable IPv4 or IPv6 protocol stack available.\n";
        ret_val = UPNP_E_OUTOF_SOCKET;
        goto error;
    }
    /* As per the IANA specifications for the use of ports by applications
     * override the listen port passed in with the first available. */
    if (listen_port4 == 0) {
        listen_port4 = APPLICATION_LISTENING_PORT;
    }
    if (listen_port6 == 0) {
        listen_port6 = APPLICATION_LISTENING_PORT;
    }
    if (listen_port6UlaGua < APPLICATION_LISTENING_PORT) {
        /* Increment the port to make it harder to fail at first try */
        listen_port6UlaGua = listen_port6 + 1;
    }
    ss4.try_port = listen_port4;
    ss6.try_port = listen_port6;
    ss6UlaGua.try_port = listen_port6UlaGua;
    if (MINISERVER_REUSEADDR) {
        /* THIS ALLOWS US TO BIND AGAIN IMMEDIATELY AFTER OUR SERVER HAS BEEN
         * CLOSED THIS MAY CAUSE TCP TO BECOME LESS RELIABLE HOWEVER IT HAS BEEN
         * SUGESTED FOR TCP SERVERS. On UPnPlib I do not REUSEADDR in general
         * due to security issues. Instead I use another netaddress that is
         * already given by another port on the same ip_address. --Ingo */
        UPNPLIB_LOGCRIT "MSG1069: REUSEADDR is set. This is not supported.\n";
        ret_val = UPNP_E_OUTOF_SOCKET;
        goto error;
    }
    if (ss4.fd != INVALID_SOCKET) {
        ret_val = do_bind_listen(&ss4);
        if (ret_val) {
            goto error;
        }
    }
    if (ss6.fd != INVALID_SOCKET) {
        ret_val = do_bind_listen(&ss6);
        if (ret_val) {
            goto error;
        }
    }
    if (ss6UlaGua.fd != INVALID_SOCKET) {
        ret_val = do_bind_listen(&ss6UlaGua);
        if (ret_val) {
            goto error;
        }
    }
    // BUG! the following condition may be wrong, e.g. with ss4.fd ==
    // INVALID_SOCKET and ENABLE_IPV6 disabled but also others. --Ingo
    UPNPLIB_LOGINFO "MSG1065: Finished.\n";
    out->miniServerPort4 = ss4.actual_port;
    out->miniServerPort6 = ss6.actual_port;
    out->miniServerPort6UlaGua = ss6UlaGua.actual_port;
    out->miniServerSock4 = ss4.fd;
    out->miniServerSock6 = ss6.fd;
    out->miniServerSock6UlaGua = ss6UlaGua.fd;

    return UPNP_E_SUCCESS;

error:
    if (ss4.fd != INVALID_SOCKET) {
        sock_close(ss4.fd);
    }
    if (ss6.fd != INVALID_SOCKET) {
        sock_close(ss6.fd);
    }
    if (ss6UlaGua.fd != INVALID_SOCKET) {
        sock_close(ss6UlaGua.fd);
    }

    return ret_val;
}

/*!
 * \brief Create STREAM sockets, binds to INADDR_ANY and listens for incoming
 * connections.
 *
 * Returns the actual port which the sockets sub-system returned.
 *
 * Only available with the webserver compiled in.
 *
 * \returns
 *  On success: UPNP_E_SUCCESS\n
 *  On error:
 *  - UPNP_E_OUTOF_SOCKET - Failed to create a socket.
 *  - UPNP_E_SOCKET_BIND - Bind() failed.
 *  - UPNP_E_LISTEN - Listen() failed.
 *  - UPNP_E_INTERNAL_ERROR - Port returned by the socket layer is < 0.
 */
[[maybe_unused]] int get_miniserver_sockets2(
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
    /*! [in] port on which the UPnP Device shall listen for incoming IPv6 ULA
     * connections. If **0** then a random port number is returned in **out**.
     */
    in_port_t listen_port6,
    /*! [in] port on which the UPnP Device shall listen for incoming IPv6 ULA or
     *GUA connections. If **0** then a random port number is returned in
     ***out**.
     */
    in_port_t listen_port6UlaGua) {
    UPNPLIB_LOGINFO "MSG1109: Executing with listen_port4="
        << listen_port4 << ", listen_port6=" << listen_port6
        << ", listen_port6UlaGua=" << listen_port6UlaGua << ".\n";

    int retval{UPNP_E_OUTOF_SOCKET};

    if (gIF_IPV6[0] != '\0') {
        static upnplib::CSocket ss6Obj;
        try {
            ss6Obj.set(AF_INET6, SOCK_STREAM);
            ss6Obj.bind('[' + std::string(gIF_IPV6) + ']',
                        std::to_string(listen_port6));
            ss6Obj.listen();
            out->miniServerSock6 = ss6Obj;
            out->miniServerPort6 = ss6Obj.get_port();
            retval = UPNP_E_SUCCESS;
        } catch (const std::exception& e) {
            UPNPLIB_LOGCATCH "MSG1110: catched next line...\n" << e.what();
        }
    }

    // TODO: Check what's with
    // ss6.serverAddr6->sin6_scope_id = gIF_INDEX;
    // It is never copied to 'out'.

    if (gIF_IPV6_ULA_GUA[0] != '\0') {
        static upnplib::CSocket ss6UlaGuaObj;
        try {
            ss6UlaGuaObj.set(AF_INET6, SOCK_STREAM);
            ss6UlaGuaObj.bind('[' + std::string(gIF_IPV6_ULA_GUA) + ']',
                              std::to_string(listen_port6UlaGua));
            ss6UlaGuaObj.listen();
            out->miniServerSock6UlaGua = ss6UlaGuaObj;
            out->miniServerPort6UlaGua = ss6UlaGuaObj.get_port();
            retval = UPNP_E_SUCCESS;
        } catch (const std::exception& e) {
            UPNPLIB_LOGCATCH "MSG1117: catched next line...\n" << e.what();
        }
    }

    if (gIF_IPV4[0] != '\0') {
        static upnplib::CSocket ss4Obj;
        try {
            ss4Obj.set(AF_INET, SOCK_STREAM);
            ss4Obj.bind('[' + std::string(gIF_IPV4) + ']',
                        std::to_string(listen_port4));
            ss4Obj.listen();
            out->miniServerPort4 = ss4Obj.get_port();
            out->miniServerSock4 = ss4Obj;
            retval = UPNP_E_SUCCESS;
        } catch (const std::exception& e) {
            UPNPLIB_LOGCATCH "MSG1112: catched next line...\n" << e.what();
        }
    }

    // TODO: Check what's with the SOCK_DGRAM socket as noted in the doku.

    UPNPLIB_LOGINFO "MSG1065: Finished.\n";
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

    upnplib::CSocketErr sockerrObj;
    SOCKET miniServerStopSock =
        umock::sys_socket_h.socket(AF_INET, SOCK_DGRAM, 0);
    if (miniServerStopSock == INVALID_SOCKET) {
        sockerrObj.catch_error();
        UPNPLIB_LOGCRIT "MSG1094: Error in socket(): "
            << sockerrObj.get_error_str() << "\n";
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
        UPNPLIB_LOGCRIT "MSG1095: Error in binding localhost: "
            << sockerrObj.get_error_str() << "\n";
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

    UPNPLIB_LOGINFO "MSG1053: Bound stop socket="
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
    UPNPLIB_LOGINFO "MSG1068: Executing...\n";
    constexpr int max_count{10000};
    MiniServerSockArray* miniSocket;
    ThreadPoolJob job;
    int ret_code{UPNP_E_INTERNAL_ERROR};

    memset(&job, 0, sizeof(job));

    if (gMServState != MSERV_IDLE) {
        /* miniserver running. */
        UPNPLIB_LOGERR "MSG1087: Cannot start. Miniserver is running.\n";
        return UPNP_E_INTERNAL_ERROR;
    }
    miniSocket = (MiniServerSockArray*)malloc(sizeof(MiniServerSockArray));
    if (!miniSocket) {
        return UPNP_E_OUTOF_MEMORY;
    }
    InitMiniServerSockArray(miniSocket);

#ifdef COMPA_HAVE_WEBSERVER
    if (*listen_port4 == 0 || *listen_port6 == 0 || *listen_port6UlaGua == 0) {
        // Create a simple random number generator for port numbers. We need
        // this because we do not reuse addresses before TIME_WAIT has expired
        // (socket option SO_REUSEADDR = false). We need to use different
        // socket addresses and that is already given with different port
        // numbers.
        std::random_device rd;         // obtain a random number from hardware
        std::minstd_rand random(rd()); // seed the generator
        std::uniform_int_distribution<in_port_t> portno(
            APPLICATION_LISTENING_PORT, 65535); // used range

        in_port_t listen_port = portno(random);
        if (*listen_port4 == 0)
            *listen_port4 = listen_port;
        if (*listen_port6 == 0)
            *listen_port6 = listen_port;
        if (*listen_port6UlaGua == 0)
            *listen_port6UlaGua = listen_port;
    }
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
    struct sockaddr_in ssdpAddr;
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
                                   (struct sockaddr*)&ssdpAddr, socklen);
        imillisleep(1);
        if (gMServState == (MiniServerState)MSERV_IDLE) {
            break;
        }
        isleep(1u);
    }
    sock_close(sock);

    return 0;
}
