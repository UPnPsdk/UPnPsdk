/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (C) 2011-2012 France Telecom All rights reserved.
 * Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
 * Redistribution only with this Copyright remark. Last modified: 2025-07-24
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * * Neither name of Intel Corporation nor the names of its contributors
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
// Last compare with pupnp original source file on 2025-07-16, ver 1.14.21

#ifndef PUPNP_UPNPAPI_HPP
#define PUPNP_UPNPAPI_HPP

/*!
 * \file
 */

#include "GenlibClientSubscription.hpp"
#include "TimerThread.hpp"
#include "VirtualDir.hpp" /* for struct VirtualDirCallbacks */
#include "service_table.hpp"

#define MAX_INTERFACES 256

#define DEV_LIMIT 200

#define DEFAULT_MX 5

#define DEFAULT_MAXAGE 1800

#define DEFAULT_SOAP_CONTENT_LENGTH 16000
#define MAX_SOAP_CONTENT_LENGTH (size_t)32000

#define NUM_HANDLE 200

extern size_t g_maxContentLength;
extern int g_UpnpSdkEQMaxLen;
extern int g_UpnpSdkEQMaxAge;

/* 30-second timeout */
#define UPNP_TIMEOUT 30

typedef enum { HND_INVALID = -1, HND_CLIENT, HND_DEVICE } Upnp_Handle_Type;

/* Data to be stored in handle table for */
struct Handle_Info {
    /*! . */
    Upnp_Handle_Type HType;
    /*! Callback function pointer. */
    Upnp_FunPtr Callback;
    /*! . */
    char* Cookie;
    /*! 0 = not installed; otherwise installed. */
    int aliasInstalled;

    /* Device Only */
#ifdef INCLUDE_DEVICE_APIS
    /*! URL for the use of SSDP. */
    char DescURL[LINE_SIZE];
    /*! URL for the use of SSDP when answering to legacy CPs (CP searching
     * for a v1 when the device is v2). */
    char LowerDescURL[LINE_SIZE];
    /*! XML file path for device description. */
    char DescXML[LINE_SIZE];
    /* Advertisement timeout */
    int MaxAge;
    /* Power State as defined by UPnP Low Power. */
    int PowerState;
    /* Sleep Period as defined by UPnP Low Power. */
    int SleepPeriod;
    /* Registration State as defined by UPnP Low Power. */
    int RegistrationState;
    /*! Description parsed in terms of DOM document. */
    IXML_Document* DescDocument;
    /*! List of devices in the description document. */
    IXML_NodeList* DeviceList;
    /*! List of services in the description document. */
    IXML_NodeList* ServiceList;
    /*! Table holding subscriptions and URL information. */
    service_table ServiceTable;
    /*! . */
    int MaxSubscriptions;
    /*! . */
    int MaxSubscriptionTimeOut;
    /*! Address family: AF_INET or AF_INET6. */
    int DeviceAf;
#endif

    /* Client only */
#ifdef INCLUDE_CLIENT_APIS
    /*! Client subscription list. */
    GenlibClientSubscription* ClientSubList;
    /*! Active SSDP searches. */
    LinkedList SsdpSearchList;
#endif
};

extern ithread_rwlock_t GlobalHndRWLock;

/*!
 * \brief Get handle information.
 *
 * \return HND_DEVICE, UPNP_E_INVALID_HANDLE
 */
static enum Upnp_LogLevel_e debug_handle = UPNP_NEVER;

Upnp_Handle_Type GetHandleInfo(
    /*! handle pointer (key for the client handle structure). */
    int Hnd,
    /*! handle structure passed by this function. */
    struct Handle_Info** HndInfo);

[[maybe_unused]] static void HandleUnlock(const char* file, int line) {
    UpnpPrintf(debug_handle, API, file, line, "Trying Unlock\n");
    ithread_rwlock_unlock(&GlobalHndRWLock);
    UpnpPrintf(debug_handle, API, file, line, "Unlocked rwlock\n");
}

[[maybe_unused]] static void HandleReadLock(const char* file, int line) {
    UpnpPrintf(debug_handle, API, file, line, "Trying a read lock\n");
    ithread_rwlock_rdlock(&GlobalHndRWLock);
    UpnpPrintf(debug_handle, API, file, line, "Read lock acquired\n");
}

[[maybe_unused]] static void HandleWriteLock(const char* file, int line) {
    UpnpPrintf(debug_handle, API, file, line, "Trying a write lock\n");
    ithread_rwlock_wrlock(&GlobalHndRWLock);
    UpnpPrintf(debug_handle, API, file, line, "Write lock acquired\n");
}

[[maybe_unused]] static void HandleLock(const char* file, int line) {
    HandleWriteLock(file, line);
}

/*!
 * \brief Get client handle info.
 *
 * \note The logic around the use of this function should be revised.
 *
 * \return HND_CLIENT, HND_INVALID
 */
Upnp_Handle_Type GetClientHandleInfo(
    /*! [in] client handle pointer (key for the client handle structure). */
    int* client_handle_out,
    /*! [out] Client handle structure passed by this function. */
    struct Handle_Info** HndInfo);
/*!
 * \brief Retrieves the device handle and information of the first device of
 *  the address family specified. The search begins at the 'start' index,
 * which should be 0 for the first call, then the last successful value
 * returned. This allows listing all entries for the address family.
 *
 * \return HND_DEVICE or HND_INVALID
 */
Upnp_Handle_Type GetDeviceHandleInfo(
    /*! [in] place to start the search (i.e. last value returned). */
    UpnpDevice_Handle start,
    /*! [in] Address family. */
    int AddressFamily,
    /*! [out] Device handle pointer. */
    int* device_handle_out,
    /*! [out] Device handle structure passed by this function. */
    struct Handle_Info** HndInfo);

/*!
 * \brief Retrieves the device handle and information of the first device of
 *  the address family specified, with a service having a controlURL or
 *  eventSubURL matching the path.
 *
 * \return HND_DEVICE or HND_INVALID
 */
Upnp_Handle_Type GetDeviceHandleInfoForPath(
    /*! The Uri path. */
    const char* path,
    /*! [in] Address family. */
    int AddressFamily,
    /*! [out] Device handle pointer. */
    int* device_handle_out,
    /*! [out] Device handle structure passed by this function. */
    struct Handle_Info** HndInfo,
    /*! [out] Service info for found path. */
    service_info** serv_info);

EXPORT_SPEC extern char gIF_NAME[LINE_SIZE];
EXPORT_SPEC extern char gIF_IPV4[INET_ADDRSTRLEN];
EXPORT_SPEC extern char gIF_IPV4_NETMASK[INET_ADDRSTRLEN];
EXPORT_SPEC extern char gIF_IPV6[INET6_ADDRSTRLEN];
EXPORT_SPEC extern unsigned gIF_IPV6_PREFIX_LENGTH;

EXPORT_SPEC extern char gIF_IPV6_ULA_GUA[INET6_ADDRSTRLEN];
EXPORT_SPEC extern unsigned gIF_IPV6_ULA_GUA_PREFIX_LENGTH;

EXPORT_SPEC extern unsigned gIF_INDEX;

EXPORT_SPEC extern unsigned short LOCAL_PORT_V4;
EXPORT_SPEC extern unsigned short LOCAL_PORT_V6;
EXPORT_SPEC extern unsigned short LOCAL_PORT_V6_ULA_GUA;

/*! NLS uuid. */
extern Upnp_SID gUpnpSdkNLSuuid;

extern TimerThread gTimerThread;
EXPORT_SPEC extern ThreadPool gRecvThreadPool;
EXPORT_SPEC extern ThreadPool gSendThreadPool;
EXPORT_SPEC extern ThreadPool gMiniServerThreadPool;

typedef enum {
    SUBSCRIBE,
    UNSUBSCRIBE,
    DK_NOTIFY,
    QUERY,
    ACTION,
    STATUS,
    DEVDESCRIPTION,
    SERVDESCRIPTION,
    MINI,
    RENEW
} UpnpFunName;

struct UpnpNonblockParam {
    UpnpFunName FunName;
    int Handle;
    int TimeOut;
    char VarName[NAME_SIZE];
    char NewVal[NAME_SIZE];
    char DevType[NAME_SIZE];
    char DevId[NAME_SIZE];
    char ServiceType[NAME_SIZE];
    char ServiceVer[NAME_SIZE];
    char Url[NAME_SIZE];
    Upnp_SID SubsId;
    char* Cookie;
    Upnp_FunPtr Fun;
    IXML_Document* Header;
    IXML_Document* Act;
    struct DevDesc* Devdesc;
};

extern virtualDirList* pVirtualDirList;
extern struct VirtualDirCallbacks virtualDirCallback;

typedef enum { WEB_SERVER_DISABLED, WEB_SERVER_ENABLED } WebServerState;

#define E_HTTP_SYNTAX -6

/*!
 * \brief Retrieve interface information and keep it in global variables.
 * If NULL, we'll find the first suitable interface for operation.
 *
 * The interface must fulfill these requirements:
 * \li Be UP.
 * \li Not be LOOPBACK.
 * \li Support MULTICAST.
 * \li Have a valid IPv4 or IPv6 address.
 *
 * We'll retrieve the following information from the interface:
 * \li gIF_NAME -> Interface name (by input or found).
 * \li gIF_IPV4 -> IPv4 address (if any).
 * \li gIF_IPV6 -> IPv6 address (if any).
 * \li gIF_IPV6_ULA_GUA -> ULA or GUA IPv6 address (if any)
 * \li gIF_INDEX -> Interface index number.
 *
 * \return UPNP_E_SUCCESS on success.
 */
EXPORT_SPEC int UpnpGetIfInfo(
    /*! [in] Interface name (can be NULL). */
    const char* IfName);

void UpnpThreadDistribution(struct UpnpNonblockParam* Param);

/*!
 * \brief This function is a timer thread scheduled by UpnpSendAdvertisement
 * to the send advetisement again.
 */
void AutoAdvertise(
    /*! [in] Information provided to the thread. */
    void* input);

/*!
 * \brief Print handle info.
 *
 * \return UPNP_E_SUCCESS if successful, otherwise returns appropriate error.
 */
int PrintHandleInfo(
    /*! [in] Handle index. */
    UpnpClient_Handle Hnd);

/*! */
extern WebServerState bWebServerState;

/*! */
EXPORT_SPEC extern WebCallback_HostValidate gWebCallback_HostValidate;

/*! */
EXPORT_SPEC extern void* gWebCallback_HostValidateCookie;

/*! */
EXPORT_SPEC extern int gAllowLiteralHostRedirection;

#endif /* PUPNP_UPNPAPI_HPP */
