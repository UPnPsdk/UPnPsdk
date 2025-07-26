// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-08-05

// Include source code for testing. So we have also direct access to static
// functions which need to be tested.
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
#include <Pupnp/upnp/src/ssdp/ssdp_device.cpp>
#else
#include <Compa/src/ssdp/ssdp_device.cpp>
#endif

#include <UPnPsdk/global.hpp>
#include <UPnPsdk/upnptools.hpp> // for errStrEx
#include <UPnPsdk/sockaddr.hpp>

#include <utest/utest.hpp>
#include <utest/upnpdebug.hpp>


namespace utest {

using ::testing::ExitedWithCode;

using ::UPnPsdk::errStrEx;
using ::UPnPsdk::g_dbug;
using ::UPnPsdk::SSockaddr;


TEST(SsdpDeviceTestSuite, NewRequestHandler_successful) {
    CPupnplog logObj; // Output only with build type DEBUG.
    if (g_dbug)
        logObj.enable(UPNP_ALL);

    // If I use productive ssdp multicast addresses with link-local scope
    // ("[ff02::c]", IPv4 ttl = 1), or site local scope ("[ff05::c], IPv4 ttl >
    // 1) I would spam the users network with this unusable multicast test
    // messages. So I use the destination address "[ff01::c]" and set the IPv4
    // ttl to 0. This sets the scope to interface-local. The messages don't
    // leave the interface.
    SSockaddr destaddr_ip6;
    destaddr_ip6 = "[ff01::c]:1900";
#ifdef __APPLE__
    SSockaddr destaddr2_ip6;
    destaddr2_ip6 = "[ff02::c]:1900";
#endif
    SSockaddr destaddr_ip4;
    destaddr_ip4 = "239.255.255.250:1900";
    constexpr int ip4ttl{0};

    char msg1[]{"Multicast message 1."};
    char msg2[]{""};
    char msg3[]{"Multicast message 3."};
    char* RqPacket[3]{msg1, msg2, msg3};

    int ret_NewRequestHandler{UPNP_E_INTERNAL_ERROR};
    // We have a trivial C-style array, so the size (num_pkg) must fit.
    // Otherwise we get segfaults.
    for (int num_pkg{1}; num_pkg <= 3; num_pkg++) {
#ifdef __APPLE__
        // MacOS does not accept IPv6 interface-local multicast address
        // "[ff01::c]". That doesn't conform to the specification.
        ret_NewRequestHandler =
            ::NewRequestHandler(&destaddr_ip6.sa, num_pkg, &RqPacket[0]);
        EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR)
            << errStrEx(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR);
        // For testing I use the link-local multicast address "[ff02::c]". That
        // may spam the network, but sorry, MacOS should follow specification.
        ret_NewRequestHandler =
            ::NewRequestHandler(&destaddr2_ip6.sa, num_pkg, &RqPacket[0]);
        EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR)
            << errStrEx(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR);
#else
        // Test Unit with IPv6 interface-local multicast.
        ret_NewRequestHandler =
            ::NewRequestHandler(&destaddr_ip6.sa, num_pkg, &RqPacket[0]);
        EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SUCCESS)
            << errStrEx(ret_NewRequestHandler, UPNP_E_SUCCESS);
#endif
        // Test Unit with IPv4 TTL 0 multicast.
        ret_NewRequestHandler = ::NewRequestHandler(&destaddr_ip4.sa, num_pkg,
                                                    &RqPacket[0], ip4ttl);
        EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SUCCESS)
            << errStrEx(ret_NewRequestHandler, UPNP_E_SUCCESS);
    }
}

TEST(SsdpDeviceTestSuite, NewRequestHandler_with_unicast_addr_fails) {
    CPupnplog logObj; // Output only with build type DEBUG.
    logObj.enable(UPNP_ALL);

    constexpr int num_pkg{1};
    char msg1[]{"Multicast message 1."};
    char* RqPacket[num_pkg]{msg1};

    SSockaddr destaddr;

    if (old_code) {
        // This has to do with using strerror_r(). That has two versions
        // depending to be XSI-compliant or GNU-specific. The wrong version may
        // be selected. strerror() should be used.
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Enabled debug messages should not output garbage.\n";
    }
    // Test Unit
#ifdef _MSC_VER
    // win32 accepts the loopback address and link-local addresses as multicast
    // addresses, e.g.:
    // destaddr = "[::1]";
    // destaddr = "[fe80::123]";
    destaddr = "[2003:d5:271b:2d00:5054:ff:fe7f:c021]";
#else
    destaddr = "[::1]";
#endif
    int ret_NewRequestHandler =
        ::NewRequestHandler(&destaddr.sa, num_pkg, &RqPacket[0]);
#ifdef __APPLE__
    EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR)
        << errStrEx(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR);
#else
    EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SOCKET_WRITE)
        << errStrEx(ret_NewRequestHandler, UPNP_E_SOCKET_WRITE);
#endif
}

TEST(SsdpDeviceDeathTest, NewRequestHandler_with_no_message) {
    CPupnplog logObj; // Output only with build type DEBUG.
    if (g_dbug)
        logObj.enable(UPNP_ALL);

    strcpy(gIF_IPV4, "127.0.0.1");
    gIF_INDEX = 0;
    constexpr int num_pkg{1};
    char* RqPacket[num_pkg]{nullptr};

    constexpr int ip4ttl{0};
    SSockaddr destaddr_ip4;
    destaddr_ip4 = "239.255.255.250:1900";
    SSockaddr destaddr_ip6;

#ifdef __APPLE__
    destaddr_ip6 = "[ff02::c]:1900";

    int ret_NewRequestHandler =
        ::NewRequestHandler(&destaddr_ip6.sa, num_pkg, &RqPacket[0]);
    EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR)
        << errStrEx(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR);
    if (old_code) {
        // This expects segfault.
        EXPECT_DEATH(::NewRequestHandler(&destaddr_ip4.sa, num_pkg,
                                         &RqPacket[0], ip4ttl),
                     ".*");
    } else {
        // This expects NO segfault.
        ret_NewRequestHandler = ::NewRequestHandler(&destaddr_ip4.sa, num_pkg,
                                                    &RqPacket[0], ip4ttl);
        EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SUCCESS)
            << errStrEx(ret_NewRequestHandler, UPNP_E_SUCCESS);
    }

#else
    destaddr_ip6 = "[ff01::c]:1900";

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ]" CRES
                  << " Pointing to no message (nullptr) must not segfault.\n";
        // This expects segfault.
        EXPECT_DEATH(
            ::NewRequestHandler(&destaddr_ip6.sa, num_pkg, &RqPacket[0]), ".*");
        EXPECT_DEATH(::NewRequestHandler(&destaddr_ip4.sa, num_pkg,
                                         &RqPacket[0], ip4ttl),
                     ".*");
    } else {

        // This expects NO segfault.
        ASSERT_EXIT(
            (::NewRequestHandler(&destaddr_ip6.sa, num_pkg, &RqPacket[0]),
             exit(0)),
            ExitedWithCode(0), ".*");
        int ret_NewRequestHandler =
            ::NewRequestHandler(&destaddr_ip6.sa, num_pkg, &RqPacket[0]);
        EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SUCCESS)
            << errStrEx(ret_NewRequestHandler, UPNP_E_SUCCESS);

        ASSERT_EXIT((::NewRequestHandler(&destaddr_ip4.sa, num_pkg,
                                         &RqPacket[0], ip4ttl),
                     exit(0)),
                    ExitedWithCode(0), ".*");
        ret_NewRequestHandler = ::NewRequestHandler(&destaddr_ip4.sa, num_pkg,
                                                    &RqPacket[0], ip4ttl);
        EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SUCCESS)
            << errStrEx(ret_NewRequestHandler, UPNP_E_SUCCESS);
    }
#endif
}

TEST(SsdpDeviceTestSuite, NewRequestHandler_without_messages_succeeds) {
    CPupnplog logObj; // Output only with build type DEBUG.
    if (g_dbug)
        logObj.enable(UPNP_ALL);

    SSockaddr destaddr;

    // Test Unit
#ifdef __APPLE__
    destaddr = "[ff02::c]:1900";
    int ret_NewRequestHandler = ::NewRequestHandler(&destaddr.sa, 0, nullptr);
    EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR)
        << errStrEx(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR);
#else
    destaddr = "[ff01::c]:1900";
    int ret_NewRequestHandler = ::NewRequestHandler(&destaddr.sa, 0, nullptr);
    EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SUCCESS)
        << errStrEx(ret_NewRequestHandler, UPNP_E_SUCCESS);
#endif
    destaddr = "239.255.255.250:1900";
    ret_NewRequestHandler = ::NewRequestHandler(&destaddr.sa, 0, nullptr);
    EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SUCCESS)
        << errStrEx(ret_NewRequestHandler, UPNP_E_SUCCESS);
}

TEST(SsdpDeviceDeathTest, NewRequestHandler_without_dest_addr_fails) {
    CPupnplog logObj; // Output only with build type DEBUG.
    if (g_dbug)
        logObj.enable(UPNP_ALL);

    constexpr int num_pkg{1};
    char msg1[]{"Multicast message 1."};
    char* RqPacket[num_pkg]{msg1};

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ]" CRES
                  << " Pointing to no destination address (nullptr) must not "
                     "segfault.\n";
        // This expects segfault.
        EXPECT_DEATH(::NewRequestHandler(nullptr, num_pkg, &RqPacket[0]), ".*");

    } else {

        // This expects NO segfault.
        ASSERT_EXIT(
            (::NewRequestHandler(nullptr, num_pkg, &RqPacket[0]), exit(0)),
            ExitedWithCode(0), ".*");
        int ret_NewRequestHandler =
            ::NewRequestHandler(nullptr, num_pkg, &RqPacket[0]);
        EXPECT_EQ(ret_NewRequestHandler, UPNP_E_INVALID_PARAM)
            << errStrEx(ret_NewRequestHandler, UPNP_E_INVALID_PARAM);
    }
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
