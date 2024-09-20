// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-09-20
/*!
 * \file
 * \brief Simple calls of API functions to test conditional compile and linking.
 */

#include <upnp.hpp>
#include <iostream>

namespace utest {
namespace {

// Step 0: Addressing
// ------------------
int UpnpInit2() {
    std::cerr << "Executing UpnpInit2()\n";
    int ret = ::UpnpInit2("", 0);
    if (ret != UPNP_E_INVALID_INTERFACE) {
        std::cerr << "Unexpected: ::UpnpInit2() == " << ret << '\n';
        ::UpnpFinish();
        return 1;
    }
    ::UpnpFinish();
    return 0;
}

int UpnpFinish() {
    std::cerr << "Executing UpnpFinish()\n";
    int ret = ::UpnpFinish();
    if (ret != UPNP_E_FINISH) { // ::UpnpInit2 has not been called
        std::cerr << "Unexpected: ::UpnpFinish() == " << ret << '\n';
        return 1;
    }
    return 0;
}

int UpnpGetServerPort() {
    std::cerr << "Executing UpnpGetServerPort()\n";
    in_port_t port = ::UpnpGetServerPort();
    if (port != 0) { // This should fail because ::UpnpInit2 has not succeeded.
        std::cerr << "Unexpected: ::UpnpGetServerPort() == " << port << '\n';
        return 1;
    }
    return 0;
}

int UpnpGetServerPort6() {
    std::cerr << "Executing UpnpGetServerPort6()\n";
    in_port_t port = ::UpnpGetServerPort6();
    if (port != 0) { // This should fail because ::UpnpInit2 has not succeeded.
        std::cerr << "Unexpected: ::UpnpGetServerPort6() == " << port << '\n';
        return 1;
    }
    return 0;
}

int UpnpGetServerUlaGuaPort6() {
    std::cerr << "Executing UpnpGetServerUlaGuaPort6()\n";
    in_port_t port = ::UpnpGetServerUlaGuaPort6();
    if (port != 0) { // This should fail because ::UpnpInit2 has not succeeded.
        std::cerr << "Unexpected: ::UpnpGetServerUlaGuaPort6() == " << port
                  << '\n';
        return 1;
    }
    return 0;
}

int UpnpGetServerIpAddress() {
    std::cerr << "Executing UpnpGetServerIpAddress()\n";
    const char* ipv4_addr = ::UpnpGetServerIpAddress();
    if (ipv4_addr != nullptr) {
        std::cerr << "Unexpected: ::UpnpGetServerIpAddress() == " << ipv4_addr
                  << '\n';
        return 1;
    }
    return 0;
}

int UpnpGetServerIp6Address() {
    std::cerr << "Executing UpnpGetServerIp6Address()\n";
    const char* ipv6_addr = ::UpnpGetServerIp6Address();
    if (ipv6_addr != nullptr) {
        std::cerr << "Unexpected: ::UpnpGetServerIp6Address() == " << ipv6_addr
                  << '\n';
        return 1;
    }
    return 0;
}

int UpnpGetServerUlaGuaIp6Address() {
    std::cerr << "Executing UpnpGetServerUlaGuaIp6Address()\n";
    const char* ipv6_addr = ::UpnpGetServerUlaGuaIp6Address();
    if (ipv6_addr != nullptr) {
        std::cerr << "Unexpected: ::UpnpGetServerUlaGuaIp6Address() == "
                  << ipv6_addr << '\n';
        return 1;
    }
    return 0;
}

[[maybe_unused]] int Fun([[maybe_unused]] Upnp_EventType EventType,
                         [[maybe_unused]] const void* Event,
                         [[maybe_unused]] void* Cookie) {
    return 0;
}

int UpnpRegisterRootDevice([[maybe_unused]] bool execute = true) {
    constexpr char function_name[]{"UpnpRegisterRootDevice()"};
#if defined(UPNP_HAVE_DEVICE) || defined(COMPA_HAVE_DEVICE_SSDP)
    if (!execute) {
        std::cerr << "Skip " << function_name << " due to access violation\n";
        return 0;
    }
    std::cerr << "Executing " << function_name << '\n';
    const char DescUrl[200]{};
    UpnpDevice_Handle Cookie{-1};
    UpnpDevice_Handle Hnd{-1};

    // On MS Windows terminate with -1073741819 (0xC0000005): access violation.
    // int ret = ::UpnpRegisterRootDevice(nullptr, nullptr, nullptr, &Hnd);
    // or
    int ret = ::UpnpRegisterRootDevice(DescUrl, Fun, &Cookie, &Hnd);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_DEVICE_SSDP not enabled\n";
#endif
    return 0;
}

int UpnpRegisterRootDevice2([[maybe_unused]] bool execute = true) {
    constexpr char function_name[]{"UpnpRegisterRootDevice2()"};
#if defined(UPNP_HAVE_DEVICE) || defined(COMPA_HAVE_DEVICE_DESCRIPTION)
    if (!execute) {
        std::cerr << "Skip " << function_name << " due to access violation\n";
        return 0;
    }
    std::cerr << "Executing " << function_name << '\n';
    const char DescUrl[200]{};
    UpnpDevice_Handle Cookie{-1};
    UpnpDevice_Handle Hnd{-1};

    // On MS Windows terminate with -1073741819 (0xC0000005): access violation.
    // int ret = ::UpnpRegisterRootDevice2(static_cast<Upnp_DescType>(0),
    // nullptr, 0, 0, nullptr, nullptr, nullptr);
    // or
    int ret = ::UpnpRegisterRootDevice2(UPNPREG_URL_DESC, DescUrl, 0, 0, Fun,
                                        &Cookie, &Hnd);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_DEVICE_DESCRIPTION not enabled\n";
#endif
    return 0;
}

int UpnpRegisterRootDevice3([[maybe_unused]] bool execute = true) {
    constexpr char function_name[]{"UpnpRegisterRootDevice3()"};
#if defined(UPNP_HAVE_DEVICE) || defined(COMPA_HAVE_DEVICE_SSDP)
    if (!execute) {
        std::cerr << "Skip " << function_name << " due to access violation\n";
        return 0;
    }
    std::cerr << "Executing " << function_name << '\n';
    const char DescUrl[200]{};
    UpnpDevice_Handle Cookie{-1};
    UpnpDevice_Handle Hnd{-1};

    // On MS Windows terminate with -1073741819 (0xC0000005): access violation.
    // int ret = ::UpnpRegisterRootDevice3(nullptr, nullptr, nullptr, nullptr,
    //                                     AF_UNSPEC);
    // or
    int ret = ::UpnpRegisterRootDevice3(DescUrl, Fun, &Cookie, &Hnd, AF_UNSPEC);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_DEVICE_SSDP not enabled\n";
#endif
    return 0;
}

int UpnpRegisterRootDevice4([[maybe_unused]] bool execute = true) {
    constexpr char function_name[]{"UpnpRegisterRootDevice4()"};
#if defined(UPNP_HAVE_DEVICE) || defined(COMPA_HAVE_DEVICE_SSDP)
    if (!execute) {
        std::cerr << "Skip " << function_name << " due to access violation\n";
        return 0;
    }
    std::cerr << "Executing " << function_name << '\n';
    const char DescUrl[200]{};
    UpnpDevice_Handle Cookie{-1};
    UpnpDevice_Handle Hnd{-1};

    // On MS Windows terminate with -1073741819 (0xC0000005): access violation.
    // int ret = ::UpnpRegisterRootDevice4(nullptr, nullptr, nullptr, nullptr,
    //                                     AF_UNSPEC, nullptr);
    // or
    int ret = ::UpnpRegisterRootDevice4(DescUrl, Fun, &Cookie, &Hnd, AF_UNSPEC,
                                        DescUrl);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_DEVICE_SSDP not enabled\n";
#endif
    return 0;
}

int UpnpUnRegisterRootDevice() {
    constexpr char function_name[]{"UpnpUnRegisterRootDevice()"};
#if defined(UPNP_HAVE_DEVICE) || defined(COMPA_HAVE_DEVICE_SSDP)
    std::cerr << "Executing " << function_name << '\n';
    int ret = ::UpnpUnRegisterRootDevice(-1);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_DEVICE_SSDP not enabled\n";
#endif
    return 0;
}

int UpnpUnRegisterRootDeviceLowPower() {
    constexpr char function_name[]{"UpnpUnRegisterRootDeviceLowPower()"};
#if defined(UPNP_HAVE_DEVICE) || defined(COMPA_HAVE_DEVICE_SSDP)
    std::cerr << "Executing " << function_name << '\n';
    int ret = ::UpnpUnRegisterRootDeviceLowPower(0, 0, 0, 0);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_DEVICE_SSDP not enabled\n";
#endif
    return 0;
}

int UpnpRegisterClient() {
    constexpr char function_name[]{"UpnpRegisterClient()"};
#if defined(UPNP_HAVE_CLIENT) || defined(COMPA_HAVE_CTRLPT_SSDP)
    std::cerr << "Executing " << function_name << '\n';
    int ret = ::UpnpRegisterClient(nullptr, nullptr, nullptr);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_CTRLPT_SSDP not enabled\n";
#endif
    return 0;
}

int UpnpUnRegisterClient() {
    constexpr char function_name[]{"UpnpUnRegisterClient()"};
#if defined(UPNP_HAVE_CLIENT) || defined(COMPA_HAVE_CTRLPT_SSDP)
    std::cerr << "Executing " << function_name << '\n';
    int ret = ::UpnpUnRegisterClient(0);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_CTRLPT_SSDP not enabled\n";
#endif
    return 0;
}

// Deprecated
int UpnpSetContentLength(bool execute = true) {
    constexpr char function_name[]{"UpnpSetContentLength()"};
    if (!execute) {
        std::cerr << "Skip " << function_name << " due to access violation\n";
        return 0;
    }
    std::cerr << "Executing " << function_name << '\n';

    // On MS Windows terminate with -1073741819 (0xC0000005): access violation.
    int ret = ::UpnpSetContentLength(0, 0);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
    return 0;
}

int UpnpSetMaxContentLength() {
    constexpr char function_name[]{"UpnpSetMaxContentLength()"};
    std::cerr << "Executing " << function_name << '\n';
    int ret = ::UpnpSetMaxContentLength(0);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
    return 0;
}


// Step 1: Discovery
// -----------------
int UpnpSearchAsync() {
    constexpr char function_name[]{"UpnpSearchAsync()"};
#if defined(UPNP_HAVE_CLIENT) || defined(COMPA_HAVE_CTRLPT_SSDP)
    std::cerr << "Executing " << function_name << '\n';
    // const char TvDeviceType[] = "urn:schemas-upnp-org:device:tvdevice:1";

    int ret = ::UpnpSearchAsync(0, 0, nullptr, nullptr);
    // int ret = ::UpnpSearchAsync(0, MIN_SEARCH_TIME, TvDeviceType, nullptr);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_CTRLPT_SSDP not enabled\n";
#endif
    return 0;
}

int UpnpSendAdvertisement() {
    constexpr char function_name[]{"UpnpSendAdvertisement()"};
#if defined(UPNP_HAVE_DEVICE) || defined(COMPA_HAVE_DEVICE_SSDP)
    std::cerr << "Executing " << function_name << '\n';

    int ret = ::UpnpSendAdvertisement(0, 0);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_DEVICE_SSDP not enabled\n";
#endif
    return 0;
}

int UpnpSendAdvertisementLowPower() {
    constexpr char function_name[]{"UpnpSendAdvertisementLowPower()"};
#if defined(UPNP_HAVE_DEVICE) || defined(COMPA_HAVE_DEVICE_SSDP)
    std::cerr << "Executing " << function_name << '\n';

    int ret = ::UpnpSendAdvertisementLowPower(0, 0, 0, 0, 0);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_DEVICE_SSDP not enabled\n";
#endif
    return 0;
}


// Step 3: Control
// ---------------
// Deprecated
int UpnpGetServiceVarStatus() {
    constexpr char function_name[]{"UpnpGetServiceVarStatus()"};
#if defined(UPNP_HAVE_SOAP) || defined(COMPA_HAVE_CTRLPT_SOAP)
    std::cerr << "Executing " << function_name << '\n';
    int ret = ::UpnpGetServiceVarStatus(0, nullptr, nullptr, nullptr);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_CTRLPT_SOAP not enabled\n";
#endif
    return 0;
}

// Deprecated
int UpnpGetServiceVarStatusAsync() {
    constexpr char function_name[]{"UpnpGetServiceVarStatusAsync()"};
#if defined(UPNP_HAVE_SOAP) || defined(COMPA_HAVE_CTRLPT_SOAP)
    std::cerr << "Executing " << function_name << '\n';
    int ret =
        ::UpnpGetServiceVarStatusAsync(0, nullptr, nullptr, nullptr, nullptr);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_CTRLPT_SOAP not enabled\n";
#endif
    return 0;
}

int UpnpSendAction() {
    constexpr char function_name[]{"UpnpSendAction()"};
#if defined(UPNP_HAVE_SOAP) || defined(COMPA_HAVE_CTRLPT_SOAP)
    std::cerr << "Executing " << function_name << '\n';
    IXML_Document* RespNodePtr{};

    int ret =
        ::UpnpSendAction(0, nullptr, nullptr, nullptr, nullptr, &RespNodePtr);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_CTRLPT_SOAP not enabled\n";
#endif
    return 0;
}

int UpnpSendActionEx() {
    constexpr char function_name[]{"UpnpSendActionEx()"};
#if defined(UPNP_HAVE_SOAP) || defined(COMPA_HAVE_CTRLPT_SOAP)
    std::cerr << "Executing " << function_name << '\n';
    IXML_Document* RespNodePtr{};

    int ret = ::UpnpSendActionEx(0, nullptr, nullptr, nullptr, nullptr, nullptr,
                                 &RespNodePtr);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_CTRLPT_SOAP not enabled\n";
#endif
    return 0;
}

int UpnpSendActionAsync() {
    constexpr char function_name[]{"UpnpSendActionAsync()"};
#if defined(UPNP_HAVE_SOAP) || defined(COMPA_HAVE_CTRLPT_SOAP)
    std::cerr << "Executing " << function_name << '\n';
    int ret = ::UpnpSendActionAsync(0, nullptr, nullptr, nullptr, nullptr,
                                    nullptr, nullptr);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_CTRLPT_SOAP not enabled\n";
#endif
    return 0;
}

int UpnpSendActionExAsync() {
    constexpr char function_name[]{"UpnpSendActionExAsync()"};
#if defined(UPNP_HAVE_SOAP) || defined(COMPA_HAVE_CTRLPT_SOAP)
    std::cerr << "Executing " << function_name << '\n';
    int ret = ::UpnpSendActionExAsync(0, nullptr, nullptr, nullptr, nullptr,
                                      nullptr, nullptr, nullptr);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_CTRLPT_SOAP not enabled\n";
#endif
    return 0;
}


// Step 4: Eventing
// ----------------
int UpnpAcceptSubscription() {
    constexpr char function_name[]{"UpnpAcceptSubscription()"};
#if defined(UPNP_HAVE_DEVICE) || defined(COMPA_HAVE_DEVICE_GENA)
    std::cerr << "Executing " << function_name << '\n';
    const char* dummy{nullptr};
    const Upnp_SID SubsId{};

    int ret = ::UpnpAcceptSubscription(0, nullptr, nullptr, &dummy, &dummy, 0,
                                       SubsId);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_DEVICE_GENA not enabled\n";
#endif
    return 0;
}

int UpnpAcceptSubscriptionExt() {
    constexpr char function_name[]{"UpnpAcceptSubscriptionExt()"};
#if defined(UPNP_HAVE_DEVICE) || defined(COMPA_HAVE_DEVICE_GENA)
    std::cerr << "Executing " << function_name << '\n';
    const Upnp_SID SubsId{};

    int ret = ::UpnpAcceptSubscriptionExt(0, nullptr, nullptr, nullptr, SubsId);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_DEVICE_GENA not enabled\n";
#endif
    return 0;
}

int UpnpNotify() {
    constexpr char function_name[]{"UpnpNotify()"};
#if defined(UPNP_HAVE_DEVICE) || defined(COMPA_HAVE_DEVICE_GENA)
    std::cerr << "Executing " << function_name << '\n';
    const char* dummy{nullptr};

    int ret = ::UpnpNotify(0, nullptr, nullptr, &dummy, &dummy, 0);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_DEVICE_GENA not enabled\n";
#endif
    return 0;
}

int UpnpNotifyExt() {
    constexpr char function_name[]{"UpnpNotifyExt()"};
#if defined(UPNP_HAVE_DEVICE) || defined(COMPA_HAVE_DEVICE_GENA)
    std::cerr << "Executing " << function_name << '\n';

    int ret = ::UpnpNotifyExt(0, nullptr, nullptr, nullptr);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_DEVICE_GENA not enabled\n";
#endif
    return 0;
}

int UpnpRenewSubscription() {
    constexpr char function_name[]{"UpnpRenewSubscription()"};
#if defined(UPNP_HAVE_CLIENT) || defined(COMPA_HAVE_CTRLPT_GENA)
    std::cerr << "Executing " << function_name << '\n';
    const Upnp_SID SubsId{};

    int ret = ::UpnpRenewSubscription(0, nullptr, SubsId);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_CTRLPT_GENA not enabled\n";
#endif
    return 0;
}

int UpnpRenewSubscriptionAsync() {
    constexpr char function_name[]{"UpnpRenewSubscriptionAsync()"};
#if defined(UPNP_HAVE_CLIENT) || defined(COMPA_HAVE_CTRLPT_GENA)
    std::cerr << "Executing " << function_name << '\n';
    Upnp_SID SubsId{};

    int ret = ::UpnpRenewSubscriptionAsync(0, 0, SubsId, Fun, nullptr);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_CTRLPT_GENA not enabled\n";
#endif
    return 0;
}

int UpnpSetMaxSubscriptions() {
    constexpr char function_name[]{"UpnpSetMaxSubscriptions()"};
#if defined(UPNP_HAVE_DEVICE) || defined(COMPA_HAVE_DEVICE_GENA)
    std::cerr << "Executing " << function_name << '\n';

    int ret = ::UpnpSetMaxSubscriptions(0, 0);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_DEVICE_GENA not enabled\n";
#endif
    return 0;
}

int UpnpSetMaxSubscriptionTimeOut() {
    constexpr char function_name[]{"UpnpSetMaxSubscriptionTimeOut()"};
#if defined(UPNP_HAVE_DEVICE) || defined(COMPA_HAVE_DEVICE_GENA)
    std::cerr << "Executing " << function_name << '\n';

    int ret = ::UpnpSetMaxSubscriptionTimeOut(0, 0);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_DEVICE_GENA not enabled\n";
#endif
    return 0;
}

int UpnpSubscribe() {
    constexpr char function_name[]{"UpnpSubscribe()"};
#if defined(UPNP_HAVE_CLIENT) || defined(COMPA_HAVE_CTRLPT_GENA)
    std::cerr << "Executing " << function_name << '\n';
    Upnp_SID SubsId{};

    int ret = ::UpnpSubscribe(0, nullptr, nullptr, SubsId);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_CTRLPT_GENA not enabled\n";
#endif
    return 0;
}

int UpnpSubscribeAsync() {
    constexpr char function_name[]{"UpnpSubscribeAsync()"};
#if defined(UPNP_HAVE_CLIENT) || defined(COMPA_HAVE_CTRLPT_GENA)
    std::cerr << "Executing " << function_name << '\n';

    int ret = ::UpnpSubscribeAsync(0, nullptr, 0, Fun, nullptr);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_CTRLPT_GENA not enabled\n";
#endif
    return 0;
}

int UpnpUnSubscribe() {
    constexpr char function_name[]{"UpnpUnSubscribe()"};
#if defined(UPNP_HAVE_CLIENT) || defined(COMPA_HAVE_CTRLPT_GENA)
    std::cerr << "Executing " << function_name << '\n';
    Upnp_SID SubsId{};

    int ret = ::UpnpUnSubscribe(0, SubsId);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_CTRLPT_GENA not enabled\n";
#endif
    return 0;
}

int UpnpUnSubscribeAsync() {
    constexpr char function_name[]{"UpnpUnSubscribeAsync()"};
#if defined(UPNP_HAVE_CLIENT) || defined(COMPA_HAVE_CTRLPT_GENA)
    std::cerr << "Executing " << function_name << '\n';
    Upnp_SID SubsId{};

    int ret = ::UpnpUnSubscribeAsync(0, SubsId, Fun, nullptr);
    if (ret != UPNP_E_FINISH) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
#else
    std::cerr << "Skip " << function_name
              << ": COMPA_HAVE_CTRLPT_GENA not enabled\n";
#endif
    return 0;
}


// \brief Initialize OpenSSL context and use freeSslCtx().
int UpnpInitSslContext() {
    constexpr char function_name[]{"UpnpInitSslContext()"};
#if defined(COMPA_HAVE_MINISERVER) && defined(UPNP_ENABLE_OPEN_SSL)
    std::cerr << "Executing " << function_name << '\n';

    int ret = ::UpnpInitSslContext(1, TLS_method());
    if (ret != UPNP_E_SUCCESS) {
        std::cerr << "Unexpected: ::" << function_name << " == " << ret << '\n';
        return 1;
    }
    ::freeSslCtx();
#else
    std::cerr << "Skip " << function_name
              << ": UPNP_ENABLE_OPEN_SSL not enabled\n";
#endif
    return 0;
}

} // anonymous namespace
} // namespace utest


/// \brief Main entry
int main() {
    int ret{};

    // Step 0: Addressing
    ret += utest::UpnpInit2();
    ret += utest::UpnpFinish();
    ret += utest::UpnpGetServerPort();
    ret += utest::UpnpGetServerPort6();
    ret += utest::UpnpGetServerUlaGuaPort6();
    ret += utest::UpnpGetServerIpAddress();
    ret += utest::UpnpGetServerIp6Address();
    ret += utest::UpnpGetServerUlaGuaIp6Address();
#ifdef _MSC_VER
    ret += utest::UpnpRegisterRootDevice(false);  // access violation
    ret += utest::UpnpRegisterRootDevice2(false); // access violation
    ret += utest::UpnpRegisterRootDevice3(false); // access violation
    ret += utest::UpnpRegisterRootDevice4(false); // access violation
#else
    ret += utest::UpnpRegisterRootDevice();
    ret += utest::UpnpRegisterRootDevice2();
    ret += utest::UpnpRegisterRootDevice3();
    ret += utest::UpnpRegisterRootDevice4();
#endif
    ret += utest::UpnpUnRegisterRootDevice();
    ret += utest::UpnpUnRegisterRootDeviceLowPower();
    ret += utest::UpnpRegisterClient();
    ret += utest::UpnpUnRegisterClient();
#ifdef _MSC_VER
    ret += utest::UpnpSetContentLength(false); // access violation
#else
    ret += utest::UpnpSetContentLength();
#endif
    ret += utest::UpnpSetMaxContentLength();

    // Step 1: Discovery
    ret += utest::UpnpSearchAsync();
    ret += utest::UpnpSendAdvertisement();
    ret += utest::UpnpSendAdvertisementLowPower();

    // Step 3: Control
    ret += utest::UpnpGetServiceVarStatus();
    ret += utest::UpnpGetServiceVarStatusAsync();
    ret += utest::UpnpSendAction();
    ret += utest::UpnpSendActionEx();
    ret += utest::UpnpSendActionAsync();
    ret += utest::UpnpSendActionExAsync();

    // Step 4: Eventing
    ret += utest::UpnpAcceptSubscription();
    ret += utest::UpnpAcceptSubscriptionExt();
    ret += utest::UpnpNotify();
    ret += utest::UpnpNotifyExt();
    ret += utest::UpnpRenewSubscription();
    ret += utest::UpnpRenewSubscriptionAsync();
    ret += utest::UpnpSetMaxSubscriptions();
    ret += utest::UpnpSetMaxSubscriptionTimeOut();
    ret += utest::UpnpSubscribe();
    ret += utest::UpnpSubscribeAsync();
    ret += utest::UpnpUnSubscribe();
    ret += utest::UpnpUnSubscribeAsync();

    ret += utest::UpnpInitSslContext();

    // returns number of failed tests.
    return ret;

    // clang-format off
// #ifdef UPnPsdk_WITH_NATIVE_PUPNP
//     std::cerr << "DEBUG! ret = " << ret << "\n";
//     return 1;
// #else
//     return ret;
// #endif
    // clang-format on
}
