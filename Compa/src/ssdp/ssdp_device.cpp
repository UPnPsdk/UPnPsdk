/**************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (C) 2011-2012 France Telecom All rights reserved.
 * Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
 * Redistribution only with this Copyright remark. Last modified: 2025-08-15
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
// Last update from ./pupnp source file on 2025-07-26, ver 1.14.23
/*!
 * \file
 * \brief Manage "Step 1: Discovery" of the UPnP+™ specification for UPnP
 * Devices with SSDP.
 *
 * \ingroup compa-Discovery
 */

#include <ssdp_device.hpp>

#include <httpreadwrite.hpp>
#include <statcodes.hpp>
#include <upnpapi.hpp>
#include <webserver.hpp>

#include <umock/sys_socket.hpp>
#include <umock/netdb.hpp>

#ifndef COMPA_INTERNAL_CONFIG_HPP
#error "No or wrong config.hpp header file included."
#endif

/// \cond
#include <cassert>
#include <thread>
#ifdef _MSC_VER
#else
#include <net/if.h>
#endif
/// \endcond


namespace {

/*! \name Variables scope restricted to file
 * @{ */
/// @{
/// \brief Message type
constexpr int MSGTYPE_SHUTDOWN{0};
constexpr int MSGTYPE_ADVERTISEMENT{1};
constexpr int MSGTYPE_REPLY{2};
/// @}
/// @}

/*! \name Functions scope restricted to file
 * @{ */

int NewRequestHandlerIPv6(sockaddr* a_dest_saddr, int a_num_packet,
                          char** a_rq_packet) {
    SOCKET sockfd6{INVALID_SOCKET};
    int ret{UPNP_E_SUCCESS};

    // Get address info for passive listening on all local network interfaces.
    addrinfo hints{}, *res{nullptr};
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV;
    const std::string port = std::to_string(
        ntohs(reinterpret_cast<sockaddr_in6*>(a_dest_saddr)->sin6_port));
    // getaddrinfo()
    int rc = getaddrinfo(nullptr, port.c_str(), &hints, &res);
    if (rc != 0) {
        UPnPsdk_LOGERR("MSG1157") "getaddrinfo() fails with code="
            << rc << " - " << gai_strerror(rc) << '\n';
        return UPNP_E_SOCKET_ERROR;
    }
    if (res->ai_next != nullptr) {
        UPnPsdk_LOGERR("MSG11158") "getaddrinfo() fails with more than one "
                                   "address info. Expected is only one.\n";
        ret = UPNP_E_SOCKET_ERROR;
        goto exit_function;
    }

    // Get socket file descriptor, using address info.
    sockfd6 = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd6 == -1) {
        UPnPsdk_LOGERR("MSG1159") "socket() fails with errno="
            << errno << " - " << strerror(errno) << '\n';
        ret = UPNP_E_OUTOF_SOCKET;
        goto exit_function;
    }

    // Bind socket to local network addresses, using address info.
    rc = bind(sockfd6, res->ai_addr, static_cast<socklen_t>(res->ai_addrlen));
    if (rc == -1) {
        UPnPsdk_LOGERR("MSG1160") "bind() fails with errno=" << errno << " - "
                                                             << strerror(errno)
                                                             << '\n';
        ret = UPNP_E_SOCKET_BIND;
        goto exit_function;
    }

    for (int index{0}; index < a_num_packet; index++) {
        // Ignore invalid or empty strings.
        if ((*(a_rq_packet + index) == nullptr) ||
            (**(a_rq_packet + index) == '\0'))
            continue;

        // Send data. The sent string is not zero terminated.
        ssize_t bytes_sent = sendto(sockfd6, *(a_rq_packet + index),
                                    (SIZEP_T)strlen(*(a_rq_packet + index)), 0,
                                    a_dest_saddr, sizeof(sockaddr_in6));
        if (bytes_sent == -1) {
            UPnPsdk_LOGERR("MSG1161") "sendto() fails with errno="
                << errno << " - " << strerror(errno) << '\n';
            ret = UPNP_E_SOCKET_WRITE;
            goto exit_function;
        }
    }

exit_function:
    freeaddrinfo(res);
    CLOSE_SOCKET_P(sockfd6);

    return ret;
}

int NewRequestHandlerIPv4(sockaddr* a_dest_saddr, int a_num_packet,
                          char** a_rq_packet, const int a_ttl = -1) {
    SOCKET sockfd4{INVALID_SOCKET};
    int ret{UPNP_E_SUCCESS};

    // Get address info for passive listening on all local network interfaces.
    addrinfo hints{}, *res{nullptr};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV;
    const std::string port = std::to_string(
        ntohs(reinterpret_cast<sockaddr_in*>(a_dest_saddr)->sin_port));
    // getaddrinfo()
    int rc = getaddrinfo(nullptr, port.c_str(), &hints, &res);
    if (rc != 0) {
        UPnPsdk_LOGERR("MSG1041") "getaddrinfo() fails with code="
            << rc << " - " << gai_strerror(rc) << '\n';
        return UPNP_E_SOCKET_ERROR;
    }
    if (res->ai_next != nullptr) {
        UPnPsdk_LOGERR("MSG1045") "getaddrinfo() fails with more than one "
                                  "address info. Expected is only one.\n";
        ret = UPNP_E_SOCKET_ERROR;
        goto exit_function;
    }

    // Get socket file descriptor, using address info.
    sockfd4 = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd4 == -1) {
        UPnPsdk_LOGERR("MSG1046") "socket() fails with errno="
            << errno << " - " << strerror(errno) << '\n';
        ret = UPNP_E_OUTOF_SOCKET;
        goto exit_function;
    }

    // Set socket option ttl before binding soeket to local network addresses.
    // With ttl == 0 I also disable IP_MULTICAST_LOOP because it is intended to
    // be for testing without side effects to network environment.
    if (a_ttl >= 0) { // With invalid ttl do nothing and remain default setting.
        rc = setsockopt(sockfd4, IPPROTO_IP, IP_MULTICAST_TTL,
                        reinterpret_cast<const char*>(&a_ttl), sizeof(a_ttl));
        if (rc == -1) {
            UPnPsdk_LOGERR(
                "MSG1163") "setsockopt() IP_MULTICAST_TTL fails with errno="
                << errno << " - " << strerror(errno) << '\n';
            ret = UPNP_E_SOCKET_ERROR;
            goto exit_function;
        }
        if (a_ttl == 0) { // Disable also IP multicast loopback.
            int off{0};
            rc = setsockopt(sockfd4, IPPROTO_IP, IP_MULTICAST_LOOP,
                            reinterpret_cast<char*>(&off), sizeof(off));
            if (rc == -1) {
                UPnPsdk_LOGERR("MSG1164") "setsockopt() IP_MULTICAST_LOOP "
                                          "fails with errno="
                    << errno << " - " << strerror(errno) << '\n';
                ret = UPNP_E_SOCKET_ERROR;
                goto exit_function;
            }
        }
    }
    if (UPnPsdk::g_dbug) {
        int ttl{-1};
        int mcloop{-1};
        socklen_t optlen{sizeof(int)};
        getsockopt(sockfd4, IPPROTO_IP, IP_MULTICAST_TTL,
                   reinterpret_cast<char*>(&ttl), &optlen);
        getsockopt(sockfd4, IPPROTO_IP, IP_MULTICAST_LOOP,
                   reinterpret_cast<char*>(&mcloop), &optlen);
        UPnPsdk_LOGINFO("MSG1156") "Socket ttl="
            << ttl << ", multicast loopback=" << mcloop << '\n';
    }

    // Bind socket to local network addresses, using address info.
    rc = bind(sockfd4, res->ai_addr, static_cast<socklen_t>(res->ai_addrlen));
    if (rc == -1) {
        UPnPsdk_LOGERR("MSG1154") "bind() fails with errno=" << errno << " - "
                                                             << strerror(errno)
                                                             << '\n';
        ret = UPNP_E_SOCKET_BIND;
        goto exit_function;
    }

    for (int index{0}; index < a_num_packet; index++) {
        // Ignore invalid or empty strings.
        if ((*(a_rq_packet + index) == nullptr) ||
            (**(a_rq_packet + index) == '\0'))
            continue;

        // Send data. The sent string is not zero terminated.
        ssize_t bytes_sent = sendto(sockfd4, *(a_rq_packet + index),
                                    (SIZEP_T)strlen(*(a_rq_packet + index)), 0,
                                    a_dest_saddr, sizeof(sockaddr_in));
        if (bytes_sent == -1) {
            UPnPsdk_LOGERR("MSG1155") "sendto() fails with errno="
                << errno << " - " << strerror(errno) << '\n';
            ret = UPNP_E_SOCKET_WRITE;
            goto exit_function;
        }
    }

exit_function:
    freeaddrinfo(res);
    CLOSE_SOCKET_P(sockfd4);

    return ret;
}

/*!
 * \brief Works as a request handler which passes the HTTP request string
 * to multicast channel.
 *
 * \returns
 *  On success: UPNP_E_SUCCESS\n
 *  On error:
 *  - UPNP_E_INVALID_PARAM
 *  - UPNP_E_OUTOF_SOCKET
 *  - UPNP_E_NETWORK_ERROR
 *  - UPNP_E_SOCKET_WRITE
 */
#if 0
int NewRequestHandler(
    /*! [in] Ip address, to send the reply. */
    struct sockaddr* DestAddr,
    /*! [in] Number of packets to be sent. */
    int NumPacket,
    /*! [in] Pointer to Array of pointer for multicast packets to send. */
    char** RqPacket,
    /*! [in] optional: time to live of multicast ip packets */
    int a_ttl = 4) {
    socklen_t socklen = sizeof(struct sockaddr_storage);
    int Index;
    in_addr replyAddr;
    /* a/c to UPNP Spec */
#ifdef UPNP_ENABLE_IPV6
    int hops = 1;
#endif
    char buf_ntop[INET6_ADDRSTRLEN];
    int ret = UPNP_E_SUCCESS;

    // Convert local IPv4 source netaddress to socket address.
    if (DestAddr == nullptr ||
        (strlen(gIF_IPV4) > 0u &&
         (inet_pton(AF_INET, gIF_IPV4, &replyAddr) != 1))) {
        return UPNP_E_INVALID_PARAM;
    }

    // Get address info for passive listening on all local network interfaces.
    addrinfo hints{}, *res{};
    hints.ai_family = DestAddr->sa_family;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV;
    int rc = umock::netdb_h.getaddrinfo(nullptr, SSDP_PORT_STR, &hints, &res);
    if (rc != 0) {
        UpnpPrintf(UPNP_INFO, SSDP, __FILE__, __LINE__,
                   "SSDP_LIB: New Request Handler:Error in getaddrinfo(): %s\n",
                   gai_strerror(rc));
        return UPNP_E_SOCKET_ERROR;
    }

    // Get socket file descriptor, using address info.
    SOCKET ReplySock = umock::sys_socket_h.socket(
        res->ai_family, res->ai_socktype, res->ai_protocol);
    if (ReplySock == INVALID_SOCKET) {
        char* errmsg = strerror(errno);
        UpnpPrintf(UPNP_INFO, SSDP, __FILE__, __LINE__,
                   "SSDP_LIB: New Request Handler: Error in socket(): %s\n",
                   errmsg);
        ret = UPNP_E_OUTOF_SOCKET;
        goto end_NewRequestHandler;
    }

    // Bind socket to local network addresses, using address info.
    rc = umock::sys_socket_h.bind(ReplySock, res->ai_addr,
                                  static_cast<socklen_t>(res->ai_addrlen));
    if (rc == -1) {
        char* errmsg = strerror(errno);
        UpnpPrintf(UPNP_INFO, SSDP, __FILE__, __LINE__,
                   "SSDP_LIB: New Request Handler:"
                   "Error in bind(): %s\n",
                   errmsg);
        ret = UPNP_E_SOCKET_BIND;
        goto end_NewRequestHandler;
    }

    switch (DestAddr->sa_family) {
    case AF_INET: {
        const char* pbuf_ntop =
            inet_ntop(AF_INET, &((struct sockaddr_in*)DestAddr)->sin_addr,
                      buf_ntop, sizeof(buf_ntop));
        if (pbuf_ntop == nullptr) {
            char* errmsg = strerror(errno);
            UpnpPrintf(UPNP_INFO, SSDP, __FILE__, __LINE__,
                       "SSDP_LIB: New Request Handler: Error ntop(): %s\n",
                       errmsg);
            ret = UPNP_E_SOCKET_ERROR;
            goto end_NewRequestHandler;
        }
        rc = umock::sys_socket_h.setsockopt(ReplySock, IPPROTO_IP,
                                            IP_MULTICAST_IF, &replyAddr,
                                            sizeof(replyAddr));
        if (rc == -1) {
            char* errmsg = strerror(errno);
            UpnpPrintf(UPNP_INFO, SSDP, __FILE__, __LINE__,
                       "SSDP_LIB: New Request Handler:"
                       "Error in setsockopt() IP_MULTICAST_IF: %s\n",
                       errmsg);
            ret = UPNP_E_SOCKET_ERROR;
            goto end_NewRequestHandler;
        }
        rc = umock::sys_socket_h.setsockopt(
            ReplySock, IPPROTO_IP, IP_MULTICAST_TTL, &a_ttl, sizeof(a_ttl));
        if (rc == -1) {
            char* errmsg = strerror(errno);
            UpnpPrintf(UPNP_INFO, SSDP, __FILE__, __LINE__,
                       "SSDP_LIB: New Request Handler:"
                       "Error in setsockopt() IP_MULTICAST_TTL: %s\n",
                       errmsg);
            ret = UPNP_E_SOCKET_ERROR;
            goto end_NewRequestHandler;
        }
        socklen = sizeof(struct sockaddr_in);
    } break;
#ifdef UPNP_ENABLE_IPV6
    case AF_INET6: {
        const char* pbuf_ntop =
            inet_ntop(AF_INET6, &((struct sockaddr_in6*)DestAddr)->sin6_addr,
                      buf_ntop, sizeof(buf_ntop));
        if (pbuf_ntop == nullptr) {
            char* errmsg = strerror(errno);
            UpnpPrintf(UPNP_INFO, SSDP, __FILE__, __LINE__,
                       "SSDP_LIB: New Request Handler: Error ntop(): %s\n",
                       errmsg);
            ret = UPNP_E_SOCKET_ERROR;
            goto end_NewRequestHandler;
        }
        rc = umock::sys_socket_h.setsockopt(ReplySock, IPPROTO_IPV6,
                                            IPV6_MULTICAST_IF, &gIF_INDEX,
                                            sizeof(gIF_INDEX));
        if (rc == -1) {
            char* errmsg = strerror(errno);
            UpnpPrintf(UPNP_INFO, SSDP, __FILE__, __LINE__,
                       "SSDP_LIB: New Request Handler:"
                       "Error in setsockopt() IPV6_MULTICAST_IF: %s\n",
                       errmsg);
            ret = UPNP_E_SOCKET_ERROR;
            goto end_NewRequestHandler;
        }
        rc = umock::sys_socket_h.setsockopt(
            ReplySock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hops, sizeof(hops));
        if (rc == -1) {
            char* errmsg = strerror(errno);
            UpnpPrintf(UPNP_INFO, SSDP, __FILE__, __LINE__,
                       "SSDP_LIB: New Request Handler:"
                       "Error in setsockopt() IPV6_MULTICAST_HOPS: %s\n",
                       errmsg);
            ret = UPNP_E_SOCKET_ERROR;
            goto end_NewRequestHandler;
        }
    } break;
#endif
    default:
        UpnpPrintf(UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
                   "Invalid destination address specified.");
        ret = UPNP_E_NETWORK_ERROR;
        goto end_NewRequestHandler;
    }

    for (Index = 0; Index < NumPacket; Index++) {
        if (*(RqPacket + Index) == nullptr)
            continue;

        UpnpPrintf(UPNP_INFO, SSDP, __FILE__, __LINE__,
                   ">>> SSDP SEND to %s >>>\n\"%s\"\n", buf_ntop,
                   *(RqPacket + Index));
        // send data
        ssize_t rc = umock::sys_socket_h.sendto(
            ReplySock, *(RqPacket + Index),
            (SIZEP_T)strlen(*(RqPacket + Index)), 0, DestAddr, socklen);
        if (rc == -1) {
            char* errmsg = strerror(errno);
            UpnpPrintf(UPNP_INFO, SSDP, __FILE__, __LINE__,
                       "SSDP_LIB: New Request Handler: Error sendto(): %s\n",
                       errmsg);
            ret = UPNP_E_SOCKET_WRITE;
            goto end_NewRequestHandler;
        }
    }

end_NewRequestHandler:
    umock::netdb_h.freeaddrinfo(res);
    umock::unistd_h.CLOSE_SOCKET_P(ReplySock);

    return ret;
}
#else
int NewRequestHandler(
    /*! [in] Ip address, to send the reply. */
    struct sockaddr* a_dest_saddr,
    /*! [in] Number of packets to be sent. */
    int a_num_packet,
    /*! [in] Pointer to Array of pointer for multicast packets to send. */
    char** a_rq_packet,
    /*! [in] optional: time to live of multicast ip packets */
    int a_ttl = -1) {
    switch (a_dest_saddr->sa_family) {
    case AF_INET6:
        return NewRequestHandlerIPv6(a_dest_saddr, a_num_packet, a_rq_packet);
    case AF_INET:
        return NewRequestHandlerIPv4(a_dest_saddr, a_num_packet, a_rq_packet,
                                     a_ttl);
    default:
        UPnPsdk_LOGCRIT(
            "MSG1162") "Invalid destination internet address-family="
            << a_dest_saddr->sa_family << "specified.\n";
    } // switch

    return UPNP_E_NETWORK_ERROR;
}
#endif

/*!
 * \brief Extract IPv6 address.
 *
 * \returns **1** if an inet6 has been found, othherwise **0**.
 */
inline int extractIPv6address( //
    char* url,                 ///< [in]
    char* address              ///< [out] Extracted IPv6 address.
) {
    int i = 0;
    int j = 0;
    int ret = 0;

    while (url[i] != '[' && url[i] != '\0') {
        i++;
    }
    if (url[i] == '\0') {
        goto exit_function;
    }

    /* bracket has been found, we deal with an IPv6 address */
    i++;
    while (url[i] != '\0' && url[i] != ']') {
        address[j] = url[i];
        i++;
        j++;
    }
    if (url[i] == '\0') {
        goto exit_function;
    }

    if (url[i] == ']') {
        address[j] = '\0';
        ret = 1;
    }

exit_function:
    return ret;
}

/*!
 * \brief Test if a Url contains an ULA or GUA IPv6 address.
 *
 * \returns **1** if the Url contains an ULA or GUA IPv6 address, **0**
 * otherwise.
 */
int isUrlV6UlaGua(   //
    char* descdocUrl ///< [in] Url
) {
    char address[INET6_ADDRSTRLEN];
    struct in6_addr v6_addr;

    if (extractIPv6address(descdocUrl, address)) {
        inet_pton(AF_INET6, address, &v6_addr);
        return !IN6_IS_ADDR_LINKLOCAL(&v6_addr);
    }

    return 0;
}

/*!
 * \brief Creates a HTTP request packet.
 *
 * Depending on the input parameter, it either creates a service advertisement
 * request or service shutdown request etc.
 */
void CreateServicePacket(
    /*! [in] type of the message (Search Reply, Advertisement
     * or Shutdown). */
    int msg_type,
    /*! [in] ssdp type. */
    const char* nt,
    /*! [in] unique service name ( go in the HTTP Header). */
    char* usn,
    /*! [in] Location URL. */
    char* location,
    /*! [in] Service duration in sec. */
    int duration,
    /*! [out] Output buffer filled with HTTP statement. */
    char** packet,
    /*! [in] Address family of the HTTP request. */
    int AddressFamily,
    /*! [in] PowerState as defined by UPnP Low Power. */
    int PowerState,
    /*! [in] SleepPeriod as defined by UPnP Low Power. */
    int SleepPeriod,
    /*! [in] RegistrationState as defined by UPnP Low Power. */
    int RegistrationState) {
    int ret_code;
    const char* nts;
    membuffer buf;

    /* Notf == 0 means service shutdown,
     * Notf == 1 means service advertisement,
     * Notf == 2 means reply */
    membuffer_init(&buf);
    buf.size_inc = (size_t)30;
    *packet = NULL;
    if (msg_type == MSGTYPE_REPLY) {
        if (PowerState > 0) {
#ifdef COMPA_HAVE_OPTION_SSDP
            ret_code = http_MakeMessage(
                &buf, 1, 1,
                "R"
                "sdc"
                "D"
                "sc"
                "ssc"
                "ssc"
                "ssc"
                "S"
                "Xc"
                "ssc"
                "ssc"
                "sdc"
                "sdc"
                "sdcc",
                HTTP_OK, "CACHE-CONTROL: max-age=", duration,
                "EXT:", "LOCATION: ", location,
                "OPT: ", "\"http://schemas.upnp.org/upnp/1/0/\"; ns=01",
                "01-NLS: ", gUpnpSdkNLSuuid, X_USER_AGENT, "ST: ", nt,
                "USN: ", usn, "Powerstate: ", PowerState,
                "SleepPeriod: ", SleepPeriod,
                "RegistrationState: ", RegistrationState);
#else
            ret_code = http_MakeMessage(
                &buf, 1, 1,
                "R"
                "sdc"
                "D"
                "sc"
                "ssc"
                "S"
                "ssc"
                "ssc"
                "sdc"
                "sdc"
                "sdcc",
                HTTP_OK, "CACHE-CONTROL: max-age=", duration,
                "EXT:", "LOCATION: ", location, "ST: ", nt, "USN: ", usn,
                "Powerstate: ", PowerState, "SleepPeriod: ", SleepPeriod,
                "RegistrationState: ", RegistrationState);
#endif /* COMPA_HAVE_OPTION_SSDP */
        } else {
#ifdef COMPA_HAVE_OPTION_SSDP
            ret_code = http_MakeMessage(
                &buf, 1, 1,
                "R"
                "sdc"
                "D"
                "sc"
                "ssc"
                "ssc"
                "ssc"
                "S"
                "Xc"
                "ssc"
                "sscc",
                HTTP_OK, "CACHE-CONTROL: max-age=", duration,
                "EXT:", "LOCATION: ", location,
                "OPT: ", "\"http://schemas.upnp.org/upnp/1/0/\"; ns=01",
                "01-NLS: ", gUpnpSdkNLSuuid, X_USER_AGENT, "ST: ", nt,
                "USN: ", usn);
#else
            ret_code = http_MakeMessage(
                &buf, 1, 1,
                "R"
                "sdc"
                "D"
                "sc"
                "ssc"
                "S"
                "ssc"
                "sscc",
                HTTP_OK, "CACHE-CONTROL: max-age=", duration,
                "EXT:", "LOCATION: ", location, "ST: ", nt, "USN: ", usn);
#endif /* COMPA_HAVE_OPTION_SSDP */
        }
        if (ret_code != 0) {
            return;
        }
    } else if (msg_type == MSGTYPE_ADVERTISEMENT ||
               msg_type == MSGTYPE_SHUTDOWN) {
        const char* host = NULL;

        if (msg_type == MSGTYPE_ADVERTISEMENT)
            nts = "ssdp:alive";
        else
            /* shutdown */
            nts = "ssdp:byebye";
        /* NOTE: The CACHE-CONTROL and LOCATION headers are not present in a
         * shutdown msg, but are present here for MS WinMe interop. */
        switch (AddressFamily) {
        case AF_INET:
            host = SSDP_IP;
            break;
        default:
            if (isUrlV6UlaGua(location))
                host = "[" SSDP_IPV6_SITELOCAL "]";
            else
                host = "[" SSDP_IPV6_LINKLOCAL "]";
        }
        if (PowerState > 0) {
#ifdef COMPA_HAVE_OPTION_SSDP
            ret_code = http_MakeMessage(
                &buf, 1, 1,
                "Q"
                "sssdc"
                "sdc"
                "ssc"
                "ssc"
                "ssc"
                "ssc"
                "ssc"
                "S"
                "Xc"
                "ssc"
                "sdc"
                "sdc"
                "sdcc",
                HTTPMETHOD_NOTIFY, "*", (size_t)1, "HOST: ", host, ":",
                SSDP_PORT, "CACHE-CONTROL: max-age=", duration,
                "LOCATION: ", location,
                "OPT: ", "\"http://schemas.upnp.org/upnp/1/0/\"; ns=01",
                "01-NLS: ", gUpnpSdkNLSuuid, "NT: ", nt, "NTS: ", nts,
                X_USER_AGENT, "USN: ", usn, "Powerstate: ", PowerState,
                "SleepPeriod: ", SleepPeriod,
                "RegistrationState: ", RegistrationState);
#else
            ret_code = http_MakeMessage(
                &buf, 1, 1,
                "Q"
                "sssdc"
                "sdc"
                "ssc"
                "ssc"
                "ssc"
                "S"
                "ssc"
                "sdc"
                "sdc"
                "sdcc",
                HTTPMETHOD_NOTIFY, "*", (size_t)1, "HOST: ", host, ":",
                SSDP_PORT, "CACHE-CONTROL: max-age=", duration,
                "LOCATION: ", location, "NT: ", nt, "NTS: ", nts, "USN: ", usn,
                "Powerstate: ", PowerState, "SleepPeriod: ", SleepPeriod,
                "RegistrationState: ", RegistrationState);
#endif /* COMPA_HAVE_OPTION_SSDP */
        } else {
#ifdef COMPA_HAVE_OPTION_SSDP
            ret_code = http_MakeMessage(
                &buf, 1, 1,
                "Q"
                "sssdc"
                "sdc"
                "ssc"
                "ssc"
                "ssc"
                "ssc"
                "ssc"
                "S"
                "Xc"
                "sscc",
                HTTPMETHOD_NOTIFY, "*", (size_t)1, "HOST: ", host, ":",
                SSDP_PORT, "CACHE-CONTROL: max-age=", duration,
                "LOCATION: ", location,
                "OPT: ", "\"http://schemas.upnp.org/upnp/1/0/\"; ns=01",
                "01-NLS: ", gUpnpSdkNLSuuid, "NT: ", nt, "NTS: ", nts,
                X_USER_AGENT, "USN: ", usn);
#else
            ret_code = http_MakeMessage(
                &buf, 1, 1,
                "Q"
                "sssdc"
                "sdc"
                "ssc"
                "ssc"
                "ssc"
                "S"
                "sscc",
                HTTPMETHOD_NOTIFY, "*", (size_t)1, "HOST: ", host, ":",
                SSDP_PORT, "CACHE-CONTROL: max-age=", duration,
                "LOCATION: ", location, "NT: ", nt, "NTS: ", nts, "USN: ", usn);
#endif /* COMPA_HAVE_OPTION_SSDP */
        }
        if (ret_code)
            return;
    } else
        /* unknown msg */
        assert(0);
    /* return msg */
    *packet = membuffer_detach(&buf);
    membuffer_destroy(&buf);

    return;
}

/// @} // Functions scope restricted to file
} // anonymous namespace


void ssdp_handle_device_request(http_message_t* hmsg,
                                struct sockaddr_storage* dest_addr) {
    constexpr int MX_FUDGE_FACTOR{10};
    int handle, start;
    struct Handle_Info* dev_info = NULL;
    memptr hdr_value;
    int mx;
    char save_char;
    SsdpEvent event;
    int ret_code;
    SsdpSearchReply* threadArg = NULL;
    ThreadPoolJob job;
    int replyTime;
    int maxAge;

    memset(&job, 0, sizeof(job));

    /* check man hdr. */
    if (httpmsg_find_hdr(hmsg, HDR_MAN, &hdr_value) == NULL ||
        memptr_cmp(&hdr_value, "\"ssdp:discover\"") != 0)
        /* bad or missing hdr. */
        return;
    /* MX header. */
    if (httpmsg_find_hdr(hmsg, HDR_MX, &hdr_value) == NULL ||
        (mx = raw_to_int(&hdr_value, 10)) < 0)
        return;
    /* ST header. */
    if (httpmsg_find_hdr(hmsg, HDR_ST, &hdr_value) == NULL)
        return;
    save_char = hdr_value.buf[hdr_value.length];
    hdr_value.buf[hdr_value.length] = '\0';
    ret_code = ssdp_request_type(hdr_value.buf, &event);
    /* restore. */
    hdr_value.buf[hdr_value.length] = save_char;
    if (ret_code == -1)
        /* bad ST header. */
        return;

    start = 0;
    for (;;) {
        HandleLock();
        /* device info. */
        switch (GetDeviceHandleInfo(start, (int)dest_addr->ss_family, &handle,
                                    &dev_info)) {
        case HND_DEVICE:
            break;
        default:
            HandleUnlock();
            /* no info found. */
            return;
        }
        maxAge = dev_info->MaxAge;
        HandleUnlock();

        UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__, "MAX-AGE     =  %d\n",
                   maxAge);
        UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__, "MX     =  %d\n",
                   event.Mx);
        UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__, "DeviceType   =  %s\n",
                   event.DeviceType);
        UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__, "DeviceUuid   =  %s\n",
                   event.UDN);
        UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__, "ServiceType =  %s\n",
                   event.ServiceType);
        threadArg = (SsdpSearchReply*)malloc(sizeof(SsdpSearchReply));
        if (threadArg == NULL)
            return;
        threadArg->handle = handle;
        memcpy(&threadArg->dest_addr, dest_addr, sizeof(threadArg->dest_addr));
        threadArg->event = event;
        threadArg->MaxAge = maxAge;

        TPJobInit(&job, advertiseAndReplyThread, threadArg);
        TPJobSetFreeFunction(&job, (free_routine)free);

        /* Subtract a percentage from the mx to allow for network and processing
         * delays (i.e. if search is for 30 seconds, respond
         * within 0 - 27 seconds). */
        if (mx >= 2)
            mx -= std::max(1, mx / MX_FUDGE_FACTOR);
        if (mx < 1)
            mx = 1;
        replyTime = rand() % mx;
        TimerThreadSchedule(&gTimerThread, replyTime, REL_SEC, &job, SHORT_TERM,
                            NULL);
        start = handle;
    }
}

int DeviceAdvertisement(char* DevType, int RootDev, char* Udn, char* Location,
                        int Duration, int AddressFamily, int PowerState,
                        int SleepPeriod, int RegistrationState) {
    struct sockaddr_storage __ss;
    struct sockaddr_in* DestAddr4 = (struct sockaddr_in*)&__ss;
    struct sockaddr_in6* DestAddr6 = (struct sockaddr_in6*)&__ss;
    /* char Mil_Nt[LINE_SIZE] */
    char Mil_Usn[LINE_SIZE];
    char* msgs[3];
    int ret_code = UPNP_E_OUTOF_MEMORY;
    int rc = 0;

    UpnpPrintf(UPNP_INFO, SSDP, __FILE__, __LINE__,
               "In function DeviceAdvertisement\n");
    msgs[0] = NULL;
    msgs[1] = NULL;
    msgs[2] = NULL;
    memset(&__ss, 0, sizeof(__ss));
    switch (AddressFamily) {
    case AF_INET:
        DestAddr4->sin_family = (sa_family_t)AF_INET;
        inet_pton(AF_INET, SSDP_IP, &DestAddr4->sin_addr);
        DestAddr4->sin_port = htons(SSDP_PORT);
        break;
    case AF_INET6:
        DestAddr6->sin6_family = (sa_family_t)AF_INET6;
        inet_pton(AF_INET6,
                  (isUrlV6UlaGua(Location)) ? SSDP_IPV6_SITELOCAL
                                            : SSDP_IPV6_LINKLOCAL,
                  &DestAddr6->sin6_addr);
        DestAddr6->sin6_port = htons(SSDP_PORT);
        DestAddr6->sin6_scope_id = gIF_INDEX;
        break;
    default:
        UpnpPrintf(UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
                   "Invalid device address family.\n");
        ret_code = UPNP_E_INVALID_PARAM;
        goto error_handler;
    }
    // If device is a root device, here we need to send 3 advertisement or reply
    if (RootDev) {
        rc = snprintf(Mil_Usn, sizeof(Mil_Usn), "%s::upnp:rootdevice", Udn);
        if (rc < 0 || (unsigned int)rc >= sizeof(Mil_Usn))
            goto error_handler;
        CreateServicePacket(MSGTYPE_ADVERTISEMENT, "upnp:rootdevice", Mil_Usn,
                            Location, Duration, &msgs[0], AddressFamily,
                            PowerState, SleepPeriod, RegistrationState);
    }
    /* both root and sub-devices need to send these two messages */
    CreateServicePacket(MSGTYPE_ADVERTISEMENT, Udn, Udn, Location, Duration,
                        &msgs[1], AddressFamily, PowerState, SleepPeriod,
                        RegistrationState);
    rc = snprintf(Mil_Usn, sizeof(Mil_Usn), "%s::%s", Udn, DevType);
    if (rc < 0 || (unsigned int)rc >= sizeof(Mil_Usn))
        goto error_handler;
    CreateServicePacket(MSGTYPE_ADVERTISEMENT, DevType, Mil_Usn, Location,
                        Duration, &msgs[2], AddressFamily, PowerState,
                        SleepPeriod, RegistrationState);
    /* check error */
    if ((RootDev && msgs[0] == NULL) || msgs[1] == NULL || msgs[2] == NULL) {
        goto error_handler;
    }
    /* send packets */
    if (RootDev) {
        /* send 3 msg types */
        ret_code = NewRequestHandler((struct sockaddr*)&__ss, 3, &msgs[0]);
    } else { /* sub-device */

        /* send 2 msg types */
        ret_code = NewRequestHandler((struct sockaddr*)&__ss, 2, &msgs[1]);
    }

error_handler:
    /* free msgs */
    free(msgs[0]);
    free(msgs[1]);
    free(msgs[2]);

    return ret_code;
}

int SendReply(struct sockaddr* DestAddr, char* DevType, int RootDev, char* Udn,
              char* Location, int Duration, int ByType, int PowerState,
              int SleepPeriod, int RegistrationState) {
    int ret_code = UPNP_E_OUTOF_MEMORY;
    char* msgs[2];
    int num_msgs;
    char Mil_Usn[LINE_SIZE];
    int i;
    int rc = 0;

    msgs[0] = NULL;
    msgs[1] = NULL;
    if (RootDev) {
        /* one msg for root device */
        num_msgs = 1;

        rc = snprintf(Mil_Usn, sizeof(Mil_Usn), "%s::upnp:rootdevice", Udn);
        if (rc < 0 || (unsigned int)rc >= sizeof(Mil_Usn))
            goto error_handler;
        CreateServicePacket(MSGTYPE_REPLY, "upnp:rootdevice", Mil_Usn, Location,
                            Duration, &msgs[0], (int)DestAddr->sa_family,
                            PowerState, SleepPeriod, RegistrationState);
    } else {
        /* two msgs for embedded devices */
        num_msgs = 1;

        /*NK: FIX for extra response when someone searches by udn */
        if (!ByType) {
            CreateServicePacket(MSGTYPE_REPLY, Udn, Udn, Location, Duration,
                                &msgs[0], (int)DestAddr->sa_family, PowerState,
                                SleepPeriod, RegistrationState);
        } else {
            rc = snprintf(Mil_Usn, sizeof(Mil_Usn), "%s::%s", Udn, DevType);
            if (rc < 0 || (unsigned int)rc >= sizeof(Mil_Usn))
                goto error_handler;
            CreateServicePacket(MSGTYPE_REPLY, DevType, Mil_Usn, Location,
                                Duration, &msgs[0], (int)DestAddr->sa_family,
                                PowerState, SleepPeriod, RegistrationState);
        }
    }
    /* check error */
    for (i = 0; i < num_msgs; i++) {
        if (msgs[i] == NULL) {
            goto error_handler;
        }
    }
    /* send msgs */
    ret_code = NewRequestHandler(DestAddr, num_msgs, msgs);

error_handler:
    for (i = 0; i < num_msgs; i++) {
        if (msgs[i] != NULL)
            free(msgs[i]);
    }

    return ret_code;
}

int DeviceReply(struct sockaddr* DestAddr, char* DevType, int RootDev,
                char* Udn, char* Location, int Duration, int PowerState,
                int SleepPeriod, int RegistrationState) {
    char *szReq[3], Mil_Nt[LINE_SIZE], Mil_Usn[LINE_SIZE];
    int RetVal = UPNP_E_OUTOF_MEMORY;
    int rc = 0;

    szReq[0] = NULL;
    szReq[1] = NULL;
    szReq[2] = NULL;
    /* create 2 or 3 msgs */
    if (RootDev) {
        /* 3 replies for root device */
        memset(Mil_Nt, 0, sizeof(Mil_Nt));
        strncpy(Mil_Nt, "upnp:rootdevice", sizeof(Mil_Nt) - 1);
        rc = snprintf(Mil_Usn, sizeof(Mil_Usn), "%s::upnp:rootdevice", Udn);
        if (rc < 0 || (unsigned int)rc >= sizeof(Mil_Usn))
            goto error_handler;
        CreateServicePacket(MSGTYPE_REPLY, Mil_Nt, Mil_Usn, Location, Duration,
                            &szReq[0], (int)DestAddr->sa_family, PowerState,
                            SleepPeriod, RegistrationState);
    }
    rc = snprintf(Mil_Nt, sizeof(Mil_Nt), "%s", Udn);
    if (rc < 0 || (unsigned int)rc >= sizeof(Mil_Nt))
        goto error_handler;
    rc = snprintf(Mil_Usn, sizeof(Mil_Usn), "%s", Udn);
    if (rc < 0 || (unsigned int)rc >= sizeof(Mil_Usn))
        goto error_handler;
    CreateServicePacket(MSGTYPE_REPLY, Mil_Nt, Mil_Usn, Location, Duration,
                        &szReq[1], (int)DestAddr->sa_family, PowerState,
                        SleepPeriod, RegistrationState);
    rc = snprintf(Mil_Nt, sizeof(Mil_Nt), "%s", DevType);
    if (rc < 0 || (unsigned int)rc >= sizeof(Mil_Nt))
        goto error_handler;
    rc = snprintf(Mil_Usn, sizeof(Mil_Usn), "%s::%s", Udn, DevType);
    if (rc < 0 || (unsigned int)rc >= sizeof(Mil_Usn))
        goto error_handler;
    CreateServicePacket(MSGTYPE_REPLY, Mil_Nt, Mil_Usn, Location, Duration,
                        &szReq[2], (int)DestAddr->sa_family, PowerState,
                        SleepPeriod, RegistrationState);
    /* check error */
    if ((RootDev && szReq[0] == NULL) || szReq[1] == NULL || szReq[2] == NULL) {
        goto error_handler;
    }
    /* send replies */
    if (RootDev) {
        RetVal = NewRequestHandler(DestAddr, 3, szReq);
    } else {
        RetVal = NewRequestHandler(DestAddr, 2, &szReq[1]);
    }

error_handler:
    /* free */
    free(szReq[0]);
    free(szReq[1]);
    free(szReq[2]);

    return RetVal;
}

int ServiceAdvertisement(char* Udn, char* ServType, char* Location,
                         int Duration, int AddressFamily, int PowerState,
                         int SleepPeriod, int RegistrationState) {
    char Mil_Usn[LINE_SIZE];
    char* szReq[1];
    int RetVal = UPNP_E_OUTOF_MEMORY;
    struct sockaddr_storage __ss;
    struct sockaddr_in* DestAddr4 = (struct sockaddr_in*)&__ss;
    struct sockaddr_in6* DestAddr6 = (struct sockaddr_in6*)&__ss;
    int rc = 0;

    memset(&__ss, 0, sizeof(__ss));
    szReq[0] = NULL;
    switch (AddressFamily) {
    case AF_INET:
        DestAddr4->sin_family = (sa_family_t)AddressFamily;
        inet_pton(AF_INET, SSDP_IP, &DestAddr4->sin_addr);
        DestAddr4->sin_port = htons(SSDP_PORT);
        break;
    case AF_INET6:
        DestAddr6->sin6_family = (sa_family_t)AddressFamily;
        inet_pton(AF_INET6,
                  (isUrlV6UlaGua(Location)) ? SSDP_IPV6_SITELOCAL
                                            : SSDP_IPV6_LINKLOCAL,
                  &DestAddr6->sin6_addr);
        DestAddr6->sin6_port = htons(SSDP_PORT);
        DestAddr6->sin6_scope_id = gIF_INDEX;
        break;
    default:
        UpnpPrintf(UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
                   "Invalid device address family.\n");
    }
    rc = snprintf(Mil_Usn, sizeof(Mil_Usn), "%s::%s", Udn, ServType);
    if (rc < 0 || (unsigned int)rc >= sizeof(Mil_Usn))
        goto error_handler;
    /* CreateServiceRequestPacket(1,szReq[0],Mil_Nt,Mil_Usn,
     * Server,Location,Duration); */
    CreateServicePacket(MSGTYPE_ADVERTISEMENT, ServType, Mil_Usn, Location,
                        Duration, &szReq[0], AddressFamily, PowerState,
                        SleepPeriod, RegistrationState);
    if (szReq[0] == NULL) {
        goto error_handler;
    }
    RetVal = NewRequestHandler((struct sockaddr*)&__ss, 1, szReq);

error_handler:
    free(szReq[0]);

    return RetVal;
}

int ServiceReply(struct sockaddr* DestAddr, char* ServType, char* Udn,
                 char* Location, int Duration, int PowerState, int SleepPeriod,
                 int RegistrationState) {
    char Mil_Usn[LINE_SIZE];
    char* szReq[1];
    int RetVal = UPNP_E_OUTOF_MEMORY;
    int rc = 0;

    szReq[0] = NULL;
    rc = snprintf(Mil_Usn, sizeof(Mil_Usn), "%s::%s", Udn, ServType);
    if (rc < 0 || (unsigned int)rc >= sizeof(Mil_Usn))
        goto error_handler;
    CreateServicePacket(MSGTYPE_REPLY, ServType, Mil_Usn, Location, Duration,
                        &szReq[0], (int)DestAddr->sa_family, PowerState,
                        SleepPeriod, RegistrationState);
    if (szReq[0] == NULL)
        goto error_handler;
    RetVal = NewRequestHandler(DestAddr, 1, szReq);

error_handler:
    free(szReq[0]);

    return RetVal;
}

int ServiceShutdown(char* Udn, char* ServType, char* Location, int Duration,
                    int AddressFamily, int PowerState, int SleepPeriod,
                    int RegistrationState) {
    char Mil_Usn[LINE_SIZE];
    char* szReq[1];
    struct sockaddr_storage __ss;
    struct sockaddr_in* DestAddr4 = (struct sockaddr_in*)&__ss;
    struct sockaddr_in6* DestAddr6 = (struct sockaddr_in6*)&__ss;
    int RetVal = UPNP_E_OUTOF_MEMORY;
    int rc = 0;

    memset(&__ss, 0, sizeof(__ss));
    szReq[0] = NULL;
    switch (AddressFamily) {
    case AF_INET:
        DestAddr4->sin_family = (sa_family_t)AddressFamily;
        inet_pton(AF_INET, SSDP_IP, &DestAddr4->sin_addr);
        DestAddr4->sin_port = htons(SSDP_PORT);
        break;
    case AF_INET6:
        DestAddr6->sin6_family = (sa_family_t)AddressFamily;
        inet_pton(AF_INET6,
                  (isUrlV6UlaGua(Location)) ? SSDP_IPV6_SITELOCAL
                                            : SSDP_IPV6_LINKLOCAL,
                  &DestAddr6->sin6_addr);
        DestAddr6->sin6_port = htons(SSDP_PORT);
        DestAddr6->sin6_scope_id = gIF_INDEX;
        break;
    default:
        UpnpPrintf(UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
                   "Invalid device address family.\n");
    }
    /* sprintf(Mil_Nt,"%s",ServType); */
    rc = snprintf(Mil_Usn, sizeof(Mil_Usn), "%s::%s", Udn, ServType);
    if (rc < 0 || (unsigned int)rc >= sizeof(Mil_Usn))
        goto error_handler;
    /* CreateServiceRequestPacket(0,szReq[0],Mil_Nt,Mil_Usn,
     * Server,Location,Duration); */
    CreateServicePacket(MSGTYPE_SHUTDOWN, ServType, Mil_Usn, Location, Duration,
                        &szReq[0], AddressFamily, PowerState, SleepPeriod,
                        RegistrationState);
    if (szReq[0] == NULL)
        goto error_handler;
    RetVal = NewRequestHandler((struct sockaddr*)&__ss, 1, szReq);

error_handler:
    free(szReq[0]);

    return RetVal;
}

int DeviceShutdown(char* DevType, int RootDev, char* Udn, char* Location,
                   int Duration, int AddressFamily, int PowerState,
                   int SleepPeriod, int RegistrationState) {
    struct sockaddr_storage __ss;
    struct sockaddr_in* DestAddr4 = (struct sockaddr_in*)&__ss;
    struct sockaddr_in6* DestAddr6 = (struct sockaddr_in6*)&__ss;
    char* msgs[3];
    char Mil_Usn[LINE_SIZE];
    int ret_code = UPNP_E_OUTOF_MEMORY;
    int rc = 0;

    msgs[0] = NULL;
    msgs[1] = NULL;
    msgs[2] = NULL;
    memset(&__ss, 0, sizeof(__ss));
    switch (AddressFamily) {
    case AF_INET:
        DestAddr4->sin_family = (sa_family_t)AddressFamily;
        inet_pton(AF_INET, SSDP_IP, &DestAddr4->sin_addr);
        DestAddr4->sin_port = htons(SSDP_PORT);
        break;
    case AF_INET6:
        DestAddr6->sin6_family = (sa_family_t)AddressFamily;
        inet_pton(AF_INET6,
                  (isUrlV6UlaGua(Location)) ? SSDP_IPV6_SITELOCAL
                                            : SSDP_IPV6_LINKLOCAL,
                  &DestAddr6->sin6_addr);
        DestAddr6->sin6_port = htons(SSDP_PORT);
        DestAddr6->sin6_scope_id = gIF_INDEX;
        break;
    default:
        UpnpPrintf(UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
                   "Invalid device address family.\n");
    }
    /* root device has one extra msg */
    if (RootDev) {
        rc = snprintf(Mil_Usn, sizeof(Mil_Usn), "%s::upnp:rootdevice", Udn);
        if (rc < 0 || (unsigned int)rc >= sizeof(Mil_Usn))
            goto error_handler;
        CreateServicePacket(MSGTYPE_SHUTDOWN, "upnp:rootdevice", Mil_Usn,
                            Location, Duration, &msgs[0], AddressFamily,
                            PowerState, SleepPeriod, RegistrationState);
    }
    UpnpPrintf(UPNP_INFO, SSDP, __FILE__, __LINE__,
               "In function DeviceShutdown\n");
    /* both root and sub-devices need to send these two messages */
    CreateServicePacket(MSGTYPE_SHUTDOWN, Udn, Udn, Location, Duration,
                        &msgs[1], AddressFamily, PowerState, SleepPeriod,
                        RegistrationState);
    rc = snprintf(Mil_Usn, sizeof(Mil_Usn), "%s::%s", Udn, DevType);
    if (rc < 0 || (unsigned int)rc >= sizeof(Mil_Usn))
        goto error_handler;
    CreateServicePacket(MSGTYPE_SHUTDOWN, DevType, Mil_Usn, Location, Duration,
                        &msgs[2], AddressFamily, PowerState, SleepPeriod,
                        RegistrationState);
    /* check error */
    if ((RootDev && msgs[0] == NULL) || msgs[1] == NULL || msgs[2] == NULL) {
        goto error_handler;
    }
    /* send packets */
    if (RootDev) {
        /* send 3 msg types */
        ret_code = NewRequestHandler((struct sockaddr*)&__ss, 3, &msgs[0]);
    } else {
        /* sub-device */
        /* send 2 msg types */
        ret_code = NewRequestHandler((struct sockaddr*)&__ss, 2, &msgs[1]);
    }

error_handler:
    /* free msgs */
    free(msgs[0]);
    free(msgs[1]);
    free(msgs[2]);

    return ret_code;
}

int AdvertiseAndReply(int AdFlag, UpnpDevice_Handle Hnd,
                      enum SsdpSearchType SearchType, struct sockaddr* DestAddr,
                      char* DeviceType, char* DeviceUDN, char* ServiceType,
                      int Exp) {
    constexpr char SERVICELIST_STR[] = "serviceList";
    int retVal = UPNP_E_SUCCESS;
    long unsigned int i;
    long unsigned int j;
    int defaultExp = DEFAULT_MAXAGE;
    struct Handle_Info* SInfo = NULL;
    char UDNstr[100];
    char devType[100];
    char servType[100];
    IXML_NodeList* nodeList = NULL;
    IXML_NodeList* tmpNodeList = NULL;
    IXML_Node* tmpNode = NULL;
    IXML_Node* tmpNode2 = NULL;
    IXML_Node* textNode = NULL;
    const DOMString tmpStr;
    const DOMString dbgStr;
    int NumCopy = 0;

    memset(UDNstr, 0, sizeof(UDNstr));
    memset(devType, 0, sizeof(devType));
    memset(servType, 0, sizeof(servType));

    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
               "Inside AdvertiseAndReply with AdFlag = %d\n", AdFlag);

    /* Use a read lock */
    HandleReadLock();
    if (GetHandleInfo(Hnd, &SInfo) != HND_DEVICE) {
        retVal = UPNP_E_INVALID_HANDLE;
        goto end_function;
    }
    defaultExp = SInfo->MaxAge;
    /* parse the device list and send advertisements/replies */
    while (NumCopy == 0 || (AdFlag && NumCopy < NUM_SSDP_COPY)) {
        if (NumCopy != 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(SSDP_PAUSE));
        NumCopy++;
        for (i = 0lu;; i++) {
            UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
                       "Entering new device list with i = %lu\n\n", i);
            tmpNode = ixmlNodeList_item(SInfo->DeviceList, i);
            if (!tmpNode) {
                UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
                           "Exiting new device list with i = "
                           "%lu\n\n",
                           i);
                break;
            }
            dbgStr = ixmlNode_getNodeName(tmpNode);
            UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
                       "Extracting device type once for %s\n", dbgStr);
            ixmlNodeList_free(nodeList);
            nodeList = ixmlElement_getElementsByTagName((IXML_Element*)tmpNode,
                                                        "deviceType");
            if (!nodeList)
                continue;
            UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
                       "Extracting UDN for %s\n", dbgStr);
            UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
                       "Extracting device type\n");
            tmpNode2 = ixmlNodeList_item(nodeList, 0lu);
            if (!tmpNode2)
                continue;
            textNode = ixmlNode_getFirstChild(tmpNode2);
            if (!textNode)
                continue;
            UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
                       "Extracting device type \n");
            tmpStr = ixmlNode_getNodeValue(textNode);
            if (!tmpStr)
                continue;
            strncpy(devType, tmpStr, sizeof(devType) - 1);
            UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
                       "Extracting device type = %s\n", devType);
            if (!tmpNode) {
                UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
                           "TempNode is NULL\n");
            }
            dbgStr = ixmlNode_getNodeName(tmpNode);
            UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
                       "Extracting UDN for %s\n", dbgStr);
            ixmlNodeList_free(nodeList);
            nodeList =
                ixmlElement_getElementsByTagName((IXML_Element*)tmpNode, "UDN");
            if (!nodeList) {
                UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
                           "UDN not found!\n");
                continue;
            }
            tmpNode2 = ixmlNodeList_item(nodeList, 0lu);
            if (!tmpNode2) {
                UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
                           "UDN not found!\n");
                continue;
            }
            textNode = ixmlNode_getFirstChild(tmpNode2);
            if (!textNode) {
                UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
                           "UDN not found!\n");
                continue;
            }
            tmpStr = ixmlNode_getNodeValue(textNode);
            if (!tmpStr) {
                UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
                           "UDN not found!\n");
                continue;
            }
            strncpy(UDNstr, tmpStr, sizeof(UDNstr) - 1);
            UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
                       "Sending UDNStr = %s \n", UDNstr);
            if (AdFlag) {
                /* send the device advertisement */
                if (AdFlag == 1) {
                    DeviceAdvertisement(devType, i == 0lu, UDNstr,
                                        SInfo->DescURL, Exp, SInfo->DeviceAf,
                                        SInfo->PowerState, SInfo->SleepPeriod,
                                        SInfo->RegistrationState);
                } else {
                    /* AdFlag == -1 */
                    DeviceShutdown(devType, i == 0lu, UDNstr, SInfo->DescURL,
                                   Exp, SInfo->DeviceAf, SInfo->PowerState,
                                   SInfo->SleepPeriod,
                                   SInfo->RegistrationState);
                }
            } else {
                switch (SearchType) {
                case SSDP_ALL:
                    DeviceReply(DestAddr, devType, i == 0lu, UDNstr,
                                SInfo->DescURL, defaultExp, SInfo->PowerState,
                                SInfo->SleepPeriod, SInfo->RegistrationState);
                    break;
                case SSDP_ROOTDEVICE:
                    if (i == 0lu) {
                        SendReply(DestAddr, devType, 1, UDNstr, SInfo->DescURL,
                                  defaultExp, 0, SInfo->PowerState,
                                  SInfo->SleepPeriod, SInfo->RegistrationState);
                    }
                    break;
                case SSDP_DEVICEUDN: {
                    /* clang-format off */
                    if (DeviceUDN && strlen(DeviceUDN) != (size_t)0) {
                        if (strcasecmp(DeviceUDN, UDNstr)) {
                            UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
                                "DeviceUDN=%s and search UDN=%s DID NOT match\n",
                                UDNstr, DeviceUDN);
                        } else {
                            UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
                                "DeviceUDN=%s and search UDN=%s MATCH\n",
                                UDNstr, DeviceUDN);
                            SendReply(DestAddr, devType, 0, UDNstr, SInfo->DescURL, defaultExp, 0,
                                SInfo->PowerState,
                                SInfo->SleepPeriod,
                                SInfo->RegistrationState);
                        }
                    }
                    /* clang-format on */
                    break;
                }
                case SSDP_DEVICETYPE: {
                    /* clang-format off */
                    if (!strncasecmp(DeviceType, devType, strlen(DeviceType) - (size_t)2)) {
                        if (atoi(strrchr(DeviceType, ':') + 1)
                            < atoi(&devType[strlen(devType) - (size_t)1])) {
                            /* the requested version is lower than the device version
                             * must reply with the lower version number and the lower
                             * description URL */
                            UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
                                   "DeviceType=%s and search devType=%s MATCH\n",
                                   devType, DeviceType);
                            SendReply(DestAddr, DeviceType, 0, UDNstr, SInfo->LowerDescURL,
                                  defaultExp, 1,
                                  SInfo->PowerState,
                                  SInfo->SleepPeriod,
                                  SInfo->RegistrationState);
                        } else if (atoi(strrchr(DeviceType, ':') + 1)
                               == atoi(&devType[strlen(devType) - (size_t)1])) {
                            UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
                                   "DeviceType=%s and search devType=%s MATCH\n",
                                   devType, DeviceType);
                            SendReply(DestAddr, DeviceType, 0, UDNstr, SInfo->DescURL,
                                  defaultExp, 1,
                                  SInfo->PowerState,
                                  SInfo->SleepPeriod,
                                  SInfo->RegistrationState);
                        } else {
                            UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
                                   "DeviceType=%s and search devType=%s DID NOT MATCH\n",
                                   devType, DeviceType);
                        }
                    } else {
                        UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
                               "DeviceType=%s and search devType=%s DID NOT MATCH\n",
                               devType, DeviceType);
                    }
                    /* clang-format on */
                    break;
                }
                default:
                    break;
                }
            }
            /* send service advertisements for services
             * corresponding to the same device */
            UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
                       "Sending service Advertisement\n");
            /* Correct service traversal such that each device's
             * serviceList is directly traversed as a child of its
             * parent device. This ensures that the service's alive
             * message uses the UDN of the parent device. */
            tmpNode = ixmlNode_getFirstChild(tmpNode);
            while (tmpNode) {
                dbgStr = ixmlNode_getNodeName(tmpNode);
                if (!strncmp(dbgStr, SERVICELIST_STR, sizeof SERVICELIST_STR)) {
                    break;
                }
                tmpNode = ixmlNode_getNextSibling(tmpNode);
            }
            ixmlNodeList_free(nodeList);
            if (!tmpNode) {
                nodeList = NULL;
                continue;
            }
            nodeList = ixmlElement_getElementsByTagName((IXML_Element*)tmpNode,
                                                        "service");
            if (!nodeList) {
                UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
                           "Service not found 3\n");
                continue;
            }
            for (j = 0lu;; j++) {
                tmpNode = ixmlNodeList_item(nodeList, j);
                if (!tmpNode) {
                    break;
                }
                ixmlNodeList_free(tmpNodeList);
                tmpNodeList = ixmlElement_getElementsByTagName(
                    (IXML_Element*)tmpNode, "serviceType");
                if (!tmpNodeList) {
                    UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
                               "ServiceType not found \n");
                    continue;
                }
                tmpNode2 = ixmlNodeList_item(tmpNodeList, 0lu);
                if (!tmpNode2)
                    continue;
                textNode = ixmlNode_getFirstChild(tmpNode2);
                if (!textNode)
                    continue;
                /* servType is of format
                 * Servicetype:ServiceVersion */
                tmpStr = ixmlNode_getNodeValue(textNode);
                if (!tmpStr)
                    continue;
                strncpy(servType, tmpStr, sizeof(servType) - 1);
                UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
                           "ServiceType = %s\n", servType);
                if (AdFlag) {
                    if (AdFlag == 1) {
                        ServiceAdvertisement(
                            UDNstr, servType, SInfo->DescURL, Exp,
                            SInfo->DeviceAf, SInfo->PowerState,
                            SInfo->SleepPeriod, SInfo->RegistrationState);
                    } else {
                        /* AdFlag == -1 */
                        ServiceShutdown(UDNstr, servType, SInfo->DescURL, Exp,
                                        SInfo->DeviceAf, SInfo->PowerState,
                                        SInfo->SleepPeriod,
                                        SInfo->RegistrationState);
                    }
                } else {
                    switch (SearchType) {
                    case SSDP_ALL:
                        ServiceReply(DestAddr, servType, UDNstr, SInfo->DescURL,
                                     defaultExp, SInfo->PowerState,
                                     SInfo->SleepPeriod,
                                     SInfo->RegistrationState);
                        break;
                    case SSDP_SERVICE:
                        /* clang-format off */
                        if (ServiceType) {
                            if (!strncasecmp(ServiceType, servType, strlen(ServiceType) - (size_t)2)) {
                                if (atoi(strrchr(ServiceType, ':') + 1) <
                                    atoi(&servType[strlen(servType) - (size_t)1])) {
                                    /* the requested version is lower than the service version
                                     * must reply with the lower version number and the lower
                                     * description URL */
                                    UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
                                           "ServiceType=%s and search servType=%s MATCH\n",
                                           ServiceType, servType);
                                    SendReply(DestAddr, ServiceType, 0, UDNstr, SInfo->LowerDescURL,
                                          defaultExp, 1,
                                          SInfo->PowerState,
                                          SInfo->SleepPeriod,
                                          SInfo->RegistrationState);
                                } else if (atoi(strrchr (ServiceType, ':') + 1) ==
                                       atoi(&servType[strlen(servType) - (size_t)1])) {
                                    UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
                                           "ServiceType=%s and search servType=%s MATCH\n",
                                           ServiceType, servType);
                                    SendReply(DestAddr, ServiceType, 0, UDNstr, SInfo->DescURL,
                                          defaultExp, 1,
                                          SInfo->PowerState,
                                          SInfo->SleepPeriod,
                                          SInfo->RegistrationState);
                                } else {
                                    UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
                                       "ServiceType=%s and search servType=%s DID NOT MATCH\n",
                                       ServiceType, servType);
                                }
                            } else {
                                UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
                                       "ServiceType=%s and search servType=%s DID NOT MATCH\n",
                                       ServiceType, servType);
                            }
                        }
                        /* clang-format on */
                        break;
                    default:
                        break;
                    }
                }
            }
            ixmlNodeList_free(tmpNodeList);
            tmpNodeList = NULL;
            ixmlNodeList_free(nodeList);
            nodeList = NULL;
        }
    }

end_function:
    ixmlNodeList_free(tmpNodeList);
    ixmlNodeList_free(nodeList);
    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
               "Exiting AdvertiseAndReply.\n");
    HandleUnlock();

    return retVal;
}

void advertiseAndReplyThread(void* data) {
    SsdpSearchReply* arg = (SsdpSearchReply*)data;

    AdvertiseAndReply(0, arg->handle, arg->event.RequestType,
                      (struct sockaddr*)&arg->dest_addr, arg->event.DeviceType,
                      arg->event.UDN, arg->event.ServiceType, arg->MaxAge);
    free(arg);
}
