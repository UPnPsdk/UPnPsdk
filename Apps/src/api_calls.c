// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-07-02
/*!
 * \file
 * \brief Simple calls of API functions to test conditional compile and linking.
 *
 * Just calling every API function with default/empty arguments to ensure that
 * it will link successful and that there are no different symbols used when
 * linking with the pUPnP library and the compatible UPnPsdk. Extended Unit
 * Tests are done when compiled with CMake option "-D UPnPsdk_WITH_GOOGLETEST".
 */

#include <upnp.h>
#include <UPnPsdk/port_sock.hpp> // for in_port_t with _MSC_VER

#ifdef __cplusplus
#include <cstdio>  // for printf() and friends
#else
#include <stdio.h> // for printf() and friends
#include <stdbool.h>
#endif

// Step 0: Addressing
// ------------------
int UpnpInit2_utest(void) {
    fprintf(stderr, "Executing UpnpInit2()\n");
    const int ret = UpnpInit2(NULL, 0);
    if (ret != UPNP_E_SUCCESS) {
        fprintf(stderr, "    !--> unexpected return code == %d\n", ret);
        UpnpFinish();
        return 1;
    }
    UpnpFinish();
    return 0;
}

int UpnpFinish_utest(void) {
    fprintf(stderr, "Executing UpnpFinish()\n");
    const int ret = UpnpFinish();
    if (ret != UPNP_E_FINISH) { // ::UpnpInit2 has not been called
        fprintf(stderr, "    !--> unexpected return code == %d\n", ret);
        return 1;
    }
    return 0;
}

int UpnpGetServerPort_utest(void) {
    fprintf(stderr, "Executing UpnpGetServerPort()\n");
    in_port_t port = UpnpGetServerPort();
    if (port != 0) { // This should return 0, ::UpnpInit2 has not succeeded.
        fprintf(stderr, "    !--> unexpected returned port == %d\n", port);
        return 1;
    }
    return 0;
}

int UpnpGetServerPort6_utest(void) {
    fprintf(stderr, "Executing UpnpGetServerPort6()\n");
    in_port_t port = UpnpGetServerPort6();
    if (port != 0) { // This should return 0, ::UpnpInit2 has not succeeded.
        fprintf(stderr, "    !--> unexpected returned port == %d\n", port);
        return 1;
    }
    return 0;
}

int UpnpGetServerUlaGuaPort6_utest(void) {
    fprintf(stderr, "Executing UpnpGetServerUlaGuaPort6()\n");
    in_port_t port = UpnpGetServerUlaGuaPort6();
    if (port != 0) { // This should return 0, ::UpnpInit2 has not succeeded.
        fprintf(stderr, "    !--> unexpected returned port == %d\n", port);
        return 1;
    }
    return 0;
}

int UpnpGetServerIpAddress_utest(void) {
    fprintf(stderr, "Executing UpnpGetServerIpAddress()\n");
    const char* ipv4_addr = UpnpGetServerIpAddress();
    if (ipv4_addr != NULL) {
        fprintf(stderr, "    !--> unexpected returned ipv4_addr == %s\n",
                ipv4_addr);
        return 1;
    }
    return 0;
}

int UpnpGetServerIp6Address_utest(void) {
    fprintf(stderr, "Executing UpnpGetServerIp6Address()\n");
    const char* ipv6_addr = UpnpGetServerIp6Address();
    if (ipv6_addr != NULL) {
        fprintf(stderr, "    !--> unexpected returned ipv6_addr == %s\n",
                ipv6_addr);
        return 1;
    }
    return 0;
}

int UpnpGetServerUlaGuaIp6Address_utest(void) {
    fprintf(stderr, "Executing UpnpGetServerUlaGuaIp6Address()\n");
    const char* ipv6_addr = UpnpGetServerUlaGuaIp6Address();
    if (ipv6_addr != NULL) {
        fprintf(stderr, "    !--> unexpected returned ipv6_addr == %s\n",
                ipv6_addr);
        return 1;
    }
    return 0;
}

int Fun(Upnp_EventType EventType, const void* Event, void* Cookie) {
    (void)EventType, (void)Event, (void)Cookie;
    return 0;
}

int UpnpRegisterRootDevice_utest(bool execute) {
    (void)execute;

    fprintf(stderr, "Executing UpnpRegisterRootDevice()\n");
#if defined(UPNP_HAVE_DEVICE) // || defined(COMPA_HAVE_DEVICE_SSDP)
    if (!execute) {
        fprintf(stderr, "    !--> skipped due to access violation\n");
        return 0;
    }
    const char DescUrl[200] = {0};
    UpnpDevice_Handle Cookie = -1;
    UpnpDevice_Handle Hnd = -1;

    // On MS Windows terminate with -1073741819 (0xC0000005): access violation.
    // const int ret = UpnpRegisterRootDevice(NULL, NULL, NULL, &Hnd);
    // or
    const int ret = UpnpRegisterRootDevice(DescUrl, Fun, &Cookie, &Hnd);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_DEVICE or COMPA_HAVE_DEVICE_SSDP "
            "not enabled\n");
#endif
    return 0;
}

int UpnpRegisterRootDevice2_utest(bool execute) {
    (void)execute;

    fprintf(stderr, "Executing UpnpRegisterRootDevice2()\n");
#if defined(UPNP_HAVE_DEVICE) // || defined(COMPA_HAVE_DEVICE_DESCRIPTION)
    if (!execute) {
        fprintf(stderr, "    !--> skipped due to access violation\n");
        return 0;
    }
    const char DescUrl[200] = {0};
    UpnpDevice_Handle Cookie = -1;
    UpnpDevice_Handle Hnd = -1;

    // On MS Windows terminate with -1073741819 (0xC0000005): access violation.
    // const int ret = UpnpRegisterRootDevice2(static_cast<Upnp_DescType>(0),
    //                                         NULL, 0, 0, NULL, NULL, NULL);
    // or
    const int ret = UpnpRegisterRootDevice2(UPNPREG_URL_DESC, DescUrl, 0, 0,
                                            Fun, &Cookie, &Hnd);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(
        stderr,
        "    !--> skipped: UPNP_HAVE_DEVICE or COMPA_HAVE_DEVICE_DESCRIPTION "
        "not enabled\n");
#endif
    return 0;
}

int UpnpRegisterRootDevice3_utest(bool execute) {
    (void)execute;

    fprintf(stderr, "Executing UpnpRegisterRootDevice3()\n");
#if defined(UPNP_HAVE_DEVICE) // || defined(COMPA_HAVE_DEVICE_SSDP)
    if (!execute) {
        fprintf(stderr, "    !--> skipped due to access violation\n");
        return 0;
    }
    const char DescUrl[200] = {0};
    UpnpDevice_Handle Cookie = -1;
    UpnpDevice_Handle Hnd = -1;

    // On MS Windows terminate with -1073741819 (0xC0000005): access violation.
    // const int ret = UpnpRegisterRootDevice3(NULL, NULL, NULL, NULL,
    //                                         AF_UNSPEC);
    // or
    const int ret =
        UpnpRegisterRootDevice3(DescUrl, Fun, &Cookie, &Hnd, AF_UNSPEC);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_DEVICE or COMPA_HAVE_DEVICE_SSDP "
            "not enabled\n");
#endif
    return 0;
}

int UpnpRegisterRootDevice4_utest(bool execute) {
    (void)execute;

    fprintf(stderr, "Executing UpnpRegisterRootDevice4()\n");
#if defined(UPNP_HAVE_DEVICE) // || defined(COMPA_HAVE_DEVICE_SSDP)
    if (!execute) {
        fprintf(stderr, "    !--> skipped due to access violation\n");
        return 0;
    }
    const char DescUrl[200] = {0};
    UpnpDevice_Handle Cookie = -1;
    UpnpDevice_Handle Hnd = -1;

    // On MS Windows terminate with -1073741819 (0xC0000005): access violation.
    // const int ret = UpnpRegisterRootDevice4(NULL, NULL, NULL, NULL,
    //                                         AF_UNSPEC, NULL);
    // or
    const int ret = UpnpRegisterRootDevice4(DescUrl, Fun, &Cookie, &Hnd,
                                            AF_UNSPEC, DescUrl);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_DEVICE or COMPA_HAVE_DEVICE_SSDP "
            "not enabled\n");
#endif
    return 0;
}

int UpnpUnRegisterRootDevice_utest(void) {
    fprintf(stderr, "Executing UpnpUnRegisterRootDevice()\n");
#if defined(UPNP_HAVE_DEVICE) // || defined(COMPA_HAVE_DEVICE_SSDP)
    const int ret = UpnpUnRegisterRootDevice(-1);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_DEVICE or COMPA_HAVE_DEVICE_SSDP "
            "not enabled\n");
#endif
    return 0;
}

int UpnpUnRegisterRootDeviceLowPower_utest(void) {
    fprintf(stderr, "Executing UpnpUnRegisterRootDeviceLowPower()\n");
#if defined(UPNP_HAVE_DEVICE) // || defined(COMPA_HAVE_DEVICE_SSDP)
    const int ret = UpnpUnRegisterRootDeviceLowPower(0, 0, 0, 0);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_DEVICE or COMPA_HAVE_DEVICE_SSDP "
            "not enabled\n");
#endif
    return 0;
}

int UpnpRegisterClient_utest(void) {
    fprintf(stderr, "Executing UpnpRegisterClient()\n");
#if defined(UPNP_HAVE_CLIENT) // || defined(COMPA_HAVE_CTRLPT_SSDP)
    const int ret = UpnpRegisterClient(NULL, NULL, NULL);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_CLIENT or COMPA_HAVE_CTRLPT_SSDP "
            "not enabled\n");
#endif
    return 0;
}

int UpnpUnRegisterClient_utest(void) {
    fprintf(stderr, "Executing UpnpUnRegisterClient()\n");
#if defined(UPNP_HAVE_CLIENT) // || defined(COMPA_HAVE_CTRLPT_SSDP)
    const int ret = UpnpUnRegisterClient(0);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_CLIENT or COMPA_HAVE_CTRLPT_SSDP "
            "not enabled\n");
#endif
    return 0;
}

// Deprecated
int UpnpSetContentLength_utest(bool execute) {
    fprintf(stderr, "Executing UpnpSetContentLength()\n");
    if (!execute) {
        fprintf(stderr, "    !--> skipped due to access violation\n");
        return 0;
    }

    // On MS Windows terminate with -1073741819 (0xC0000005): access violation.
    const int ret = UpnpSetContentLength(0, 0);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
    return 0;
}

int UpnpSetMaxContentLength_utest(void) {
    fprintf(stderr, "Executing UpnpSetMaxContentLength()\n");
    const int ret = UpnpSetMaxContentLength(0);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
    return 0;
}


// Step 1: Discovery
// -----------------
int UpnpSearchAsync_utest(void) {
    fprintf(stderr, "Executing UpnpSearchAsync()\n");
#if defined(UPNP_HAVE_CLIENT) // || defined(COMPA_HAVE_CTRLPT_SSDP)
    // const char TvDeviceType[] = "urn:schemas-upnp-org:device:tvdevice:1";

    const int ret = UpnpSearchAsync(0, 0, NULL, NULL);
    // const int ret = UpnpSearchAsync(0, MIN_SEARCH_TIME, TvDeviceType, NULL);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_CLIENT or COMPA_HAVE_CTRLPT_SSDP "
            "not enabled\n");
#endif
    return 0;
}

int UpnpSendAdvertisement_utest(void) {
    fprintf(stderr, "Executing UpnpSendAdvertisement()\n");
#if defined(UPNP_HAVE_DEVICE) // || defined(COMPA_HAVE_DEVICE_SSDP)
    const int ret = UpnpSendAdvertisement(0, 0);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_DEVICE or COMPA_HAVE_DEVICE_SSDP "
            "not enabled\n");
#endif
    return 0;
}

int UpnpSendAdvertisementLowPower_utest(void) {
    fprintf(stderr, "Executing UpnpSendAdvertisementLowPower()\n");
#if defined(UPNP_HAVE_DEVICE) // || defined(COMPA_HAVE_DEVICE_SSDP)
    const int ret = UpnpSendAdvertisementLowPower(0, 0, 0, 0, 0);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_DEVICE or COMPA_HAVE_DEVICE_SSDP "
            "not enabled\n");
#endif
    return 0;
}


// Step 3: Control
// ---------------
// Deprecated
int UpnpGetServiceVarStatus_utest(void) {
    fprintf(stderr, "Executing UpnpGetServiceVarStatus()\n");
#if defined(UPNP_HAVE_SOAP) // || defined(COMPA_HAVE_CTRLPT_SOAP)
    const int ret = UpnpGetServiceVarStatus(0, NULL, NULL, NULL);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_SOAP or COMPA_HAVE_CTRLPT_SOAP "
            "not enabled\n");
#endif
    return 0;
}

// Deprecated
int UpnpGetServiceVarStatusAsync_utest(void) {
    fprintf(stderr, "Executing UpnpGetServiceVarStatusAsync()\n");
#if defined(UPNP_HAVE_SOAP) // || defined(COMPA_HAVE_CTRLPT_SOAP)
    const int ret = UpnpGetServiceVarStatusAsync(0, NULL, NULL, NULL, NULL);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_SOAP or COMPA_HAVE_CTRLPT_SOAP "
            "not enabled\n");
#endif
    return 0;
}

int UpnpSendAction_utest(void) {
    fprintf(stderr, "Executing UpnpSendAction()\n");
#if defined(UPNP_HAVE_SOAP) // || defined(COMPA_HAVE_CTRLPT_SOAP)
    IXML_Document* RespNodePtr = NULL;
    const int ret = UpnpSendAction(0, NULL, NULL, NULL, NULL, &RespNodePtr);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_SOAP or COMPA_HAVE_CTRLPT_SOAP "
            "not enabled\n");
#endif
    return 0;
}

int UpnpSendActionEx_utest(void) {
    fprintf(stderr, "Executing UpnpSendActionEx()\n");
#if defined(UPNP_HAVE_SOAP) // || defined(COMPA_HAVE_CTRLPT_SOAP)
    IXML_Document* RespNodePtr = NULL;
    const int ret =
        UpnpSendActionEx(0, NULL, NULL, NULL, NULL, NULL, &RespNodePtr);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_SOAP or COMPA_HAVE_CTRLPT_SOAP "
            "not enabled\n");
#endif
    return 0;
}

int UpnpSendActionAsync_utest(void) {
    fprintf(stderr, "Executing UpnpSendActionAsync()\n");
#if defined(UPNP_HAVE_SOAP) // || defined(COMPA_HAVE_CTRLPT_SOAP)
    const int ret = UpnpSendActionAsync(0, NULL, NULL, NULL, NULL, NULL, NULL);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_SOAP or COMPA_HAVE_CTRLPT_SOAP "
            "not enabled\n");
#endif
    return 0;
}

int UpnpSendActionExAsync_utest(void) {
    fprintf(stderr, "Executing UpnpSendActionExAsync()\n");
#if defined(UPNP_HAVE_SOAP) // || defined(COMPA_HAVE_CTRLPT_SOAP)
    const int ret =
        UpnpSendActionExAsync(0, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_SOAP or COMPA_HAVE_CTRLPT_SOAP "
            "not enabled\n");
#endif
    return 0;
}


// Step 4: Eventing
// ----------------
int UpnpAcceptSubscription_utest(void) {
    fprintf(stderr, "Executing UpnpAcceptSubscription()\n");
#if defined(UPNP_HAVE_DEVICE) // || defined(COMPA_HAVE_DEVICE_GENA)
    const char* dummy = NULL;
    const Upnp_SID SubsId = {0};
    const int ret =
        UpnpAcceptSubscription(0, NULL, NULL, &dummy, &dummy, 0, SubsId);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_DEVICE or COMPA_HAVE_DEVICE_GENA "
            "not enabled\n");
#endif
    return 0;
}

int UpnpAcceptSubscriptionExt_utest(void) {
    fprintf(stderr, "Executing UpnpAcceptSubscriptionExt()\n");
#if defined(UPNP_HAVE_DEVICE) // || defined(COMPA_HAVE_DEVICE_GENA)
    const Upnp_SID SubsId = {0};
    const int ret = UpnpAcceptSubscriptionExt(0, NULL, NULL, NULL, SubsId);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_DEVICE or COMPA_HAVE_DEVICE_GENA "
            "not enabled\n");
#endif
    return 0;
}

int UpnpNotify_utest(void) {
    fprintf(stderr, "Executing UpnpNotify()\n");
#if defined(UPNP_HAVE_DEVICE) // || defined(COMPA_HAVE_DEVICE_GENA)
    const char* dummy = NULL;
    const int ret = UpnpNotify(0, NULL, NULL, &dummy, &dummy, 0);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_DEVICE or COMPA_HAVE_DEVICE_GENA "
            "not enabled\n");
#endif
    return 0;
}

int UpnpNotifyExt_utest(void) {
    fprintf(stderr, "Executing UpnpNotifyExt()\n");
#if defined(UPNP_HAVE_DEVICE) // || defined(COMPA_HAVE_DEVICE_GENA)
    const int ret = UpnpNotifyExt(0, NULL, NULL, NULL);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_DEVICE or COMPA_HAVE_DEVICE_GENA "
            "not enabled\n");
#endif
    return 0;
}

int UpnpRenewSubscription_utest(void) {
    fprintf(stderr, "Executing UpnpRenewSubscription()\n");
#if defined(UPNP_HAVE_CLIENT) // || defined(COMPA_HAVE_CTRLPT_GENA)
    const Upnp_SID SubsId = {0};
    const int ret = UpnpRenewSubscription(0, NULL, SubsId);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_CLIENT or COMPA_HAVE_CTRLPT_GENA "
            "not enabled\n");
#endif
    return 0;
}

int UpnpRenewSubscriptionAsync_utest(void) {
    fprintf(stderr, "Executing UpnpRenewSubscriptionAsync()\n");
#if defined(UPNP_HAVE_CLIENT) // || defined(COMPA_HAVE_CTRLPT_GENA)
    Upnp_SID SubsId = {0};
    const int ret = UpnpRenewSubscriptionAsync(0, 0, SubsId, Fun, NULL);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_CLIENT or COMPA_HAVE_CTRLPT_GENA "
            "not enabled\n");
#endif
    return 0;
}

int UpnpSetMaxSubscriptions_utest(void) {
    fprintf(stderr, "Executing UpnpSetMaxSubscriptions()\n");
#if defined(UPNP_HAVE_DEVICE) // || defined(COMPA_HAVE_DEVICE_GENA)
    const int ret = UpnpSetMaxSubscriptions(0, 0);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_DEVICE or COMPA_HAVE_DEVICE_GENA "
            "not enabled\n");
#endif
    return 0;
}

int UpnpSetMaxSubscriptionTimeOut_utest(void) {
    fprintf(stderr, "Executing UpnpSetMaxSubscriptionTimeOut()\n");
#if defined(UPNP_HAVE_DEVICE) // || defined(COMPA_HAVE_DEVICE_GENA)
    const int ret = UpnpSetMaxSubscriptionTimeOut(0, 0);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_DEVICE or COMPA_HAVE_DEVICE_GENA "
            "not enabled\n");
#endif
    return 0;
}

int UpnpSubscribe_utest(void) {
    fprintf(stderr, "Executing UpnpSubscribe()\n");
#if defined(UPNP_HAVE_CLIENT) // || defined(COMPA_HAVE_CTRLPT_GENA)
    Upnp_SID SubsId = {0};
    const int ret = UpnpSubscribe(0, NULL, NULL, SubsId);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_CLIENT or COMPA_HAVE_CTRLPT_GENA "
            "not enabled\n");
#endif
    return 0;
}

int UpnpSubscribeAsync_utest(void) {
    fprintf(stderr, "Executing UpnpSubscribeAsync()\n");
#if defined(UPNP_HAVE_CLIENT) // || defined(COMPA_HAVE_CTRLPT_GENA)
    const int ret = UpnpSubscribeAsync(0, NULL, 0, Fun, NULL);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_CLIENT or COMPA_HAVE_CTRLPT_GENA "
            "not enabled\n");
#endif
    return 0;
}

int UpnpUnSubscribe_utest(void) {
    fprintf(stderr, "Executing UpnpUnSubscribe()\n");
#if defined(UPNP_HAVE_CLIENT) // || defined(COMPA_HAVE_CTRLPT_GENA)
    const Upnp_SID SubsId = {0};
    const int ret = UpnpUnSubscribe(0, SubsId);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_CLIENT or COMPA_HAVE_CTRLPT_GENA "
            "not enabled\n");
#endif
    return 0;
}

int UpnpUnSubscribeAsync_utest(void) {
    fprintf(stderr, "Executing UpnpUnSubscribeAsync()\n");
#if defined(UPNP_HAVE_CLIENT) // || defined(COMPA_HAVE_CTRLPT_GENA)
    Upnp_SID SubsId = {0};
    const int ret = UpnpUnSubscribeAsync(0, SubsId, Fun, NULL);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_CLIENT or COMPA_HAVE_CTRLPT_GENA "
            "not enabled\n");
#endif
    return 0;
}


// Control Point http API
// ----------------------
int UpnpDownloadUrlItem_utest(void) {
    fprintf(stderr, "Executing UpnpDownloadUrlItem()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpDownloadUrlItem(NULL, NULL, NULL);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpOpenHttpGet_utest(void) {
    fprintf(stderr, "Executing UpnpOpenHttpGet()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpOpenHttpGet(NULL, NULL, NULL, NULL, NULL, 0);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpOpenHttpGetProxy_utest(void) {
    fprintf(stderr, "Executing UpnpOpenHttpGetProxy()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpOpenHttpGetProxy(NULL, NULL, NULL, NULL, NULL, NULL, 0);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpOpenHttpGetEx_utest(void) {
    fprintf(stderr, "Executing UpnpOpenHttpGetEx()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpOpenHttpGetEx(NULL, NULL, NULL, NULL, NULL, 0, 0, 0);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpReadHttpGet_utest(void) {
    fprintf(stderr, "Executing UpnpReadHttpGet()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpReadHttpGet(NULL, NULL, NULL, 0);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpHttpGetProgress_utest(void) {
    fprintf(stderr, "Executing UpnpHttpGetProgress()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpHttpGetProgress(NULL, NULL, NULL);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpCancelHttpGet_utest(void) {
    fprintf(stderr, "Executing UpnpCancelHttpGet()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpCancelHttpGet(NULL);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpCloseHttpGet_utest(void) {
    fprintf(stderr, "Executing UpnpCloseHttpGet()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpCloseHttpGet(NULL);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpOpenHttpPost_utest(void) {
    fprintf(stderr, "Executing UpnpOpenHttpPost()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpOpenHttpPost(NULL, NULL, NULL, 0, 0);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpWriteHttpPost_utest(void) {
    fprintf(stderr, "Executing UpnpWriteHttpPost()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpWriteHttpPost(NULL, NULL, NULL, 0);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpCloseHttpPost_utest(void) {
    fprintf(stderr, "Executing UpnpCloseHttpPost()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpCloseHttpPost(NULL, NULL, 0);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpOpenHttpConnection_utest(void) {
    fprintf(stderr, "Executing UpnpOpenHttpConnection()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpOpenHttpConnection(NULL, NULL, 0);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpMakeHttpRequest_utest(void) {
    fprintf(stderr, "Executing UpnpMakeHttpRequest()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpMakeHttpRequest(UPNP_HTTPMETHOD_DELETE, NULL, NULL,
                                        NULL, NULL, 0, 0);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpWriteHttpRequest_utest(void) {
    fprintf(stderr, "Executing UpnpWriteHttpRequest()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpWriteHttpRequest(NULL, NULL, NULL, 0);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpEndHttpRequest_utest(void) {
    fprintf(stderr, "Executing UpnpEndHttpRequest()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpEndHttpRequest(NULL, 0);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpGetHttpResponse_utest(bool execute) {
    (void)execute;

    fprintf(stderr, "Executing UpnpGetHttpResponse()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    if (!execute) {
        fprintf(stderr,
                "    !--> skipped: will fail with \"Segmentation fault\".\n");
        return 0;
    }
    int handle = 0;
    char* contentType;
    int contentLength;
    int httpStatus;

    // parse_status_t parser_parse_responseline(http_parser_t*): Assertion
    // `parser->position == POS_RESPONSE_LINE' failed.
    const int ret = UpnpGetHttpResponse(&handle, NULL, &contentType,
                                        &contentLength, &httpStatus, 0);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpReadHttpResponse_utest(void) {
    fprintf(stderr, "Executing UpnpReadHttpResponse()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpReadHttpResponse(NULL, NULL, NULL, 0);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpCloseHttpConnection_utest(void) {
    fprintf(stderr, "Executing UpnpCloseHttpConnection()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpCloseHttpConnection(NULL);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpDownloadXmlDoc_utest(void) {
    fprintf(stderr, "Executing UpnpDownloadXmlDoc()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpDownloadXmlDoc(NULL, NULL);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}


// Web Server API
// --------------
int UpnpSetWebServerRootDir_utest(void) {
    fprintf(stderr, "Executing UpnpSetWebServerRootDir()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpSetWebServerRootDir(NULL);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpVirtualDir_set_GetInfoCallback_utest(void) {
    fprintf(stderr, "Executing UpnpVirtualDir_set_GetInfoCallback()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpVirtualDir_set_GetInfoCallback(NULL);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpVirtualDir_set_OpenCallback_utest(void) {
    fprintf(stderr, "Executing UpnpVirtualDir_set_OpenCallback()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpVirtualDir_set_OpenCallback(NULL);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpVirtualDir_set_ReadCallback_utest(void) {
    fprintf(stderr, "Executing UpnpVirtualDir_set_ReadCallback()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpVirtualDir_set_ReadCallback(NULL);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpVirtualDir_set_WriteCallback_utest(void) {
    fprintf(stderr, "Executing UpnpVirtualDir_set_WriteCallback()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpVirtualDir_set_WriteCallback(NULL);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpVirtualDir_set_SeekCallback_utest(void) {
    fprintf(stderr, "Executing UpnpVirtualDir_set_SeekCallback()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpVirtualDir_set_SeekCallback(NULL);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpVirtualDir_set_CloseCallback_utest(void) {
    fprintf(stderr, "Executing UpnpVirtualDir_set_CloseCallback()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpVirtualDir_set_CloseCallback(NULL);
    if (ret != UPNP_E_INVALID_PARAM) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpEnableWebserver_utest(void) {
    fprintf(stderr, "Executing UpnpEnableWebserver()\n");
#if defined(UPNP_HAVE_WEBSERVER) // || defined(COMPA_HAVE_WEBSERVER)
    const int ret = UpnpEnableWebserver(0);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
#else
    fprintf(stderr,
            "    !--> skipped: UPNP_HAVE_WEBSERVER or COMPA_HAVE_WEBSERVER "
            "not enabled\n");
#endif
    return 0;
}

int UpnpIsWebserverEnabled_utest(void) {
    fprintf(stderr, "Executing UpnpIsWebserverEnabled()\n");
    const int ret = UpnpIsWebserverEnabled();
    if (ret != 0) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
    return 0;
}

int UpnpSetHostValidateCallback_utest(void) {
    fprintf(stderr, "Executing UpnpSetHostValidateCallback()\n");
    UpnpSetHostValidateCallback(NULL, NULL);
    return 0;
}

int UpnpSetAllowLiteralHostRedirection_utest(void) {
    fprintf(stderr, "Executing UpnpSetAllowLiteralHostRedirection()\n");
    UpnpSetAllowLiteralHostRedirection(0);
    return 0;
}

int UpnpAddVirtualDir_utest(void) {
    fprintf(stderr, "Executing UpnpAddVirtualDir()\n");
    const int ret = UpnpAddVirtualDir(NULL, NULL, NULL);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
    return 0;
}

int UpnpRemoveVirtualDir_utest(void) {
    fprintf(stderr, "Executing UpnpRemoveVirtualDir()\n");
    const int ret = UpnpRemoveVirtualDir(NULL);
    if (ret != UPNP_E_FINISH) {
        fprintf(stderr, "    !--> unexpected returned value == %d\n", ret);
        return 1;
    }
    return 0;
}

int UpnpRemoveAllVirtualDirs_utest(void) {
    fprintf(stderr, "Executing UpnpRemoveAllVirtualDirs()\n");
    UpnpRemoveAllVirtualDirs();
    return 0;
}


// IXML API
// --------
int ixmlNode_getNodeName_utest(void) {
    fprintf(stderr, "Executing ixmlNode_getNodeName()\n");
    const DOMString ret = ixmlNode_getNodeName(NULL);
    if (ret != NULL) {
        fprintf(stderr, "    !--> unexpected returned value == %s\n", ret);
        return 1;
    }
    return 0;
}


/// \brief Main entry
// ==================
int main(void) {
    int ret = 0;

    // Step 0: Addressing
    ret += UpnpInit2_utest(); // Execution fails
    ret += UpnpFinish_utest();
    ret += UpnpGetServerPort_utest();
    ret += UpnpGetServerPort6_utest();
    ret += UpnpGetServerUlaGuaPort6_utest();
    ret += UpnpGetServerIpAddress_utest();
    ret += UpnpGetServerIp6Address_utest();
    ret += UpnpGetServerUlaGuaIp6Address_utest();
#ifdef _MSC_VER
    ret += UpnpRegisterRootDevice_utest(true);  // access violation?
    ret += UpnpRegisterRootDevice2_utest(true); // access violation?
    ret += UpnpRegisterRootDevice3_utest(true); // access violation?
    ret += UpnpRegisterRootDevice4_utest(true); // access violation?
#else
    ret += UpnpRegisterRootDevice_utest(true);
    ret += UpnpRegisterRootDevice2_utest(true);
    ret += UpnpRegisterRootDevice3_utest(true);
    ret += UpnpRegisterRootDevice4_utest(true);
#endif
    ret += UpnpUnRegisterRootDevice_utest();
    ret += UpnpUnRegisterRootDeviceLowPower_utest();
    ret += UpnpRegisterClient_utest();
    ret += UpnpUnRegisterClient_utest();
#ifdef _MSC_VER
    ret += UpnpSetContentLength_utest(true); // access violation?
#else
    ret += UpnpSetContentLength_utest(true);
#endif
    ret += UpnpSetMaxContentLength_utest();

    // Step 1: Discovery
    ret += UpnpSearchAsync_utest();
    ret += UpnpSendAdvertisement_utest();
    ret += UpnpSendAdvertisementLowPower_utest();

    // Step 3: Control
    ret += UpnpGetServiceVarStatus_utest();
    ret += UpnpGetServiceVarStatusAsync_utest();
    ret += UpnpSendAction_utest();
    ret += UpnpSendActionEx_utest();
    ret += UpnpSendActionAsync_utest();
    ret += UpnpSendActionExAsync_utest();

    // Step 4: Eventing
    ret += UpnpAcceptSubscription_utest();
    ret += UpnpAcceptSubscriptionExt_utest();
    ret += UpnpNotify_utest();
    ret += UpnpNotifyExt_utest();
    ret += UpnpRenewSubscription_utest();
    ret += UpnpRenewSubscriptionAsync_utest();
    ret += UpnpSetMaxSubscriptions_utest();
    ret += UpnpSetMaxSubscriptionTimeOut_utest();
    ret += UpnpSubscribe_utest();
    ret += UpnpSubscribeAsync_utest();
    ret += UpnpUnSubscribe_utest();
    ret += UpnpUnSubscribeAsync_utest();

    // Control Point http API
    ret += UpnpDownloadUrlItem_utest();
    ret += UpnpOpenHttpGet_utest();
    ret += UpnpOpenHttpGetProxy_utest();
    ret += UpnpOpenHttpGetEx_utest();
    ret += UpnpReadHttpGet_utest();
    ret += UpnpHttpGetProgress_utest();
    ret += UpnpCancelHttpGet_utest();
    ret += UpnpCloseHttpGet_utest();
    ret += UpnpOpenHttpPost_utest();
    ret += UpnpWriteHttpPost_utest();
    ret += UpnpCloseHttpPost_utest();
    ret += UpnpOpenHttpConnection_utest();
    ret += UpnpMakeHttpRequest_utest();
    ret += UpnpWriteHttpRequest_utest();
    ret += UpnpEndHttpRequest_utest();
    ret += UpnpGetHttpResponse_utest(false); // Aborted: assertion failed.
    ret += UpnpReadHttpResponse_utest();
    ret += UpnpCloseHttpConnection_utest();
    ret += UpnpDownloadXmlDoc_utest();

    // Web Server API
    ret += UpnpSetWebServerRootDir_utest();
    ret += UpnpVirtualDir_set_GetInfoCallback_utest();
    ret += UpnpVirtualDir_set_OpenCallback_utest();
    ret += UpnpVirtualDir_set_ReadCallback_utest();
    ret += UpnpVirtualDir_set_WriteCallback_utest();
    ret += UpnpVirtualDir_set_SeekCallback_utest();
    ret += UpnpVirtualDir_set_CloseCallback_utest();
    ret += UpnpEnableWebserver_utest();
    ret += UpnpIsWebserverEnabled_utest();
    ret += UpnpSetHostValidateCallback_utest();
    ret += UpnpSetAllowLiteralHostRedirection_utest();
    ret += UpnpAddVirtualDir_utest();
    ret += UpnpRemoveVirtualDir_utest();
    ret += UpnpRemoveAllVirtualDirs_utest();

    // IXML API
    ret += ixmlNode_getNodeName_utest();

    // returns number of failed tests.
    printf("Failed API calls = %d\n", ret);
    return ret;
}
