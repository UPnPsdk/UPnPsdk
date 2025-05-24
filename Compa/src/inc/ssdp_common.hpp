#if defined(COMPA_HAVE_CTRLPT_SSDP) || defined(COMPA_HAVE_DEVICE_SSDP)

#ifndef COMPA_SSDP_COMMON_HPP
#define COMPA_SSDP_COMMON_HPP
/**************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (C) 2011-2012 France Telecom All rights reserved.
 * Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
 * Redistribution only with this Copyright remark. Last modified: 2025-05-21
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
// Last compare with ./pupnp source file on 2023-08-30, ver 1.14.18
/*!
 * \file
 * \brief Manage "Step 1: Discovery" of the UPnP+™ specification with SSDP.
 * \ingroup compa-Discovery
 */

#include <miniserver.hpp>


/*! \name SSDP constants.
 * @{ */
/// constant
#define BUFSIZE (size_t)2500
/// constant
#define SSDP_IP "239.255.255.250"
/// constant
#define SSDP_IPV6_LINKLOCAL "FF02::C"
/// constant
#define SSDP_IPV6_SITELOCAL "FF05::C"
/// constant
#define SSDP_PORT 1900
/// constant
#define NUM_TRY 3
/// constant
#define THREAD_LIMIT 50
/// constant
#define COMMAND_LEN 300
/// @}

/*! \name SSDP Error codes.
 * @{ */
#define NO_ERROR_FOUND 0
/// error code
#define E_REQUEST_INVALID -3
/// error code
#define E_RES_EXPIRED -4
/// error code
#define E_MEM_ALLOC -5
/// error code
#define E_HTTP_SYNTEX -6
/// error code
#define E_SOCKET -7
/// @}

/// timeout
#define RQST_TIMEOUT 20

/// \brief Enumeration to define all different types of ssdp searches.
enum SsdpSearchType {
    SSDP_SERROR = -1, ///< Unknown search command.
    SSDP_ALL,         ///< Part of SType.
    SSDP_ROOTDEVICE,  ///< Part of SType.
    SSDP_DEVICEUDN,   ///< Part of SType.
    SSDP_DEVICETYPE,  ///< Part of SType.
    SSDP_SERVICE      ///< Part of SType.
};

/// \brief Structure to store the SSDP information.
struct SsdpEvent {
    /// @{
    /// \brief Part of SSDP Event
    enum SsdpSearchType RequestType;
    int ErrCode;
    int MaxAge;
    int Mx;
    char UDN[LINE_SIZE];
    char DeviceType[LINE_SIZE];
    /* NT or ST */
    char ServiceType[LINE_SIZE];
    char Location[LINE_SIZE];
    char HostAddr[LINE_SIZE];
    char Os[LINE_SIZE];
    char Ext[LINE_SIZE];
    char Date[LINE_SIZE];
    struct sockaddr* DestAddr;
    void* Cookie;
    /// @}
};

/// \brief Maybe a callback function?
typedef void (*SsdpFunPtr)(SsdpEvent*);

/// \brief thread data.
struct ThreadData {
    /// @{
    /// \brief part of thread data
    int Mx;
    void* Cookie;
    char* Data;
    struct sockaddr_storage DestAddr;
    /// @}
};

/// \brief SSDP search reply.
struct SsdpSearchReply {
    /// @{
    /// \brief part of search reply
    int MaxAge;
    UpnpDevice_Handle handle;
    struct sockaddr_storage dest_addr;
    SsdpEvent event;
    /// @}
};

/// \brief SSDP search exp argument.
struct SsdpSearchExpArg {
    int handle;         ///< \brief handle
    int timeoutEventId; ///< \brief timeout event id
};

/// \brief SSDP thread data.
struct ssdp_thread_data {
    http_parser_t parser;       ///< parser
    sockaddr_storage dest_addr; ///< destination socket address
};

/* globals */

/*! \brief If control point API is compiled in, this is the global IPv4 socket
 * for it. */
inline SOCKET gSsdpReqSocket4;
#ifdef UPNP_ENABLE_IPV6
/*! \brief If control point API is compiled in, this is the global IPv6 socket
 * for it. */
inline SOCKET gSsdpReqSocket6;
#endif /* UPNP_ENABLE_IPV6 */

/// \brief Maybe a callback function?
typedef int (*ParserFun)(char*, SsdpEvent*);


/*! @{
 * \ingroup SSDP-common_functions */

/*!
 * \brief Fills the fields of the event structure like DeviceType, Device UDN
 * and Service Type.
 *
 * \returns
 *  On success: **0**\n
 *  On error: **-1**
 */
int unique_service_name(
    /*! [in] Service Name string. */
    char* cmd,
    /*! [out] The SSDP event structure partially filled by all the function. */
    SsdpEvent* Evt);

/*!
 * \brief This function figures out the type of the SSDP search in the request.
 *
 * \returns
 *  On success: Returns appropriate search type.\n
 *  On error: SSDP_ERROR
 */
SsdpSearchType ssdp_request_type1(
    /*! [in] command came in the ssdp request. */
    char* cmd);

/*!
 * \brief Starts filling the SSDP event structure based upon the request
 * received.
 *
 * \returns
 *  On success: **0**\n
 *  On error: **-1**
 */
int ssdp_request_type(
    /*! [in] command came in the ssdp request. */
    char* cmd,
    /*! [out] The event structure partially filled by this function. */
    SsdpEvent* Evt);

/*!
 * \brief This function reads the data from the ssdp socket.
 *
 * \returns
 *  On success: **0**\n
 *  On error: **-1**
 */
PUPNP_EXP int readFromSSDPSocket(
    /*! [in] SSDP socket. */
    SOCKET socket);

/*!
 * \brief Creates the IPv4 and IPv6 ssdp sockets required by the
 *  control point and device operation.
 *
 * \returns
 *  On success: UPNP_E_SUCCESS\n
 *  On error:
 *  - UPNP_E_OUTOF_SOCKET
 *  - UPNP_E_SOCKET_ERROR
 *  - UPNP_E_SOCKET_BIND
 *  - UPNP_E_NETWORK_ERROR
 */
int get_ssdp_sockets(
    /*! [out] Array of SSDP sockets. */
    MiniServerSockArray* out);

/// @} SSDP Common Functions

#endif /* COMPA_SSDP_COMMON_HPP */
#endif // defined(COMPA_HAVE_CTRLPT_SSDP) || defined(COMPA_HAVE_DEVICE_SSDP)
