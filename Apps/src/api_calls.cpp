// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-09-18
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

    // Terminate with -1073741819 (0xC0000005): access violation.
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

    // Terminate with -1073741819 (0xC0000005): access violation.
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

    // Terminate with -1073741819 (0xC0000005): access violation.
    // int ret = ::UpnpRegisterRootDevice3(nullptr, nullptr, nullptr, nullptr,
    // AF_UNSPEC); or
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

    // Terminate with -1073741819 (0xC0000005): access violation.
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
#if 0
/// \brief Initialize OpenSSL context.
int UpnpInitSslContext() {
#if defined(COMPA_HAVE_MINISERVER) && defined(UPNP_ENABLE_OPEN_SSL)
    int ret = ::UpnpInitSslContext(1, TLS_method());
    if (ret != UPNP_E_SUCCESS) {
        std::cerr << "Unexpected: UpnpInitSslContext() == " << ret << "\n";
        return 1;
    }
    ::freeSslCtx();
#endif
    return 0;
}
#endif

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
    ret += utest::UpnpSetMaxContentLength();

    // Step 1: Discovery


    // ret += utest::UpnpInitSslContext();

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
