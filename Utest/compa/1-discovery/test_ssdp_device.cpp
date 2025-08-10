// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-08-15

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
#include <UPnPsdk/netadapter.hpp>

#include <utest/utest.hpp>
#include <utest/upnpdebug.hpp>


namespace utest {

using ::testing::ExitedWithCode;

using ::UPnPsdk::CNetadapter;
using ::UPnPsdk::errStrEx;
using ::UPnPsdk::g_dbug;
using ::UPnPsdk::SSockaddr;

// Real local ip addresses on this nodes network adapters, evaluated with
// CNetadapter.
struct SSaNadap {
    SSockaddr sa;
    unsigned int index{};
    unsigned int bitmask{};
    std::string name;
};
SSaNadap lo6Obj;
SSaNadap lo4Obj;
SSaNadap llaObj;
SSaNadap guaObj;
SSaNadap ip4Obj;

void get_netadapter() {
    // Getting information of the local network adapters is expensive because
    // it allocates memory to return the internal adapter list. So I do it one
    // time on start and provide the needed information.
    SSockaddr saObj;
    CNetadapter nadaptObj;

    nadaptObj.get_first();
    do {
        nadaptObj.sockaddr(saObj);
        if (lo6Obj.sa.ss.ss_family == AF_UNSPEC && saObj.is_loopback() &&
            saObj.ss.ss_family == AF_INET6) {
            // Found first ipv6 loopback address.
            lo6Obj.sa = saObj;
            lo6Obj.index = nadaptObj.index();
            lo6Obj.bitmask = nadaptObj.bitmask();
            lo6Obj.name = nadaptObj.name();
        } else if (lo4Obj.sa.ss.ss_family == AF_UNSPEC && saObj.is_loopback() &&
                   saObj.ss.ss_family == AF_INET) {
            // Found first ipv4 loopback address.
            lo4Obj.sa = saObj;
            lo4Obj.index = nadaptObj.index();
            lo4Obj.bitmask = nadaptObj.bitmask();
            lo4Obj.name = nadaptObj.name();
        } else if (llaObj.sa.ss.ss_family == AF_UNSPEC &&
                   saObj.ss.ss_family == AF_INET6 &&
                   IN6_IS_ADDR_LINKLOCAL(&saObj.sin6.sin6_addr) &&
                   nadaptObj.index() != 1) {
            // Found first LLA address without lla ("[fe80::1]" MacOS special)
            // on loopback interface.
            llaObj.sa = saObj;
            llaObj.index = nadaptObj.index();
            llaObj.bitmask = nadaptObj.bitmask();
            llaObj.name = nadaptObj.name();
        } else if (guaObj.sa.ss.ss_family == AF_UNSPEC &&
                   saObj.ss.ss_family == AF_INET6 &&
                   IN6_IS_ADDR_GLOBAL(&saObj.sin6.sin6_addr)) {
            // Found first GUA address.
            guaObj.sa = saObj;
            guaObj.index = nadaptObj.index();
            guaObj.bitmask = nadaptObj.bitmask();
            guaObj.name = nadaptObj.name();
        } else if (ip4Obj.sa.ss.ss_family == AF_UNSPEC &&
                   saObj.ss.ss_family == AF_INET && !saObj.is_loopback()) {
            // Found first IPv4 address.
            ip4Obj.sa = saObj;
            ip4Obj.index = nadaptObj.index();
            ip4Obj.bitmask = nadaptObj.bitmask();
            ip4Obj.name = nadaptObj.name();
        }
    } while (nadaptObj.get_next() && (lo6Obj.sa.ss.ss_family == AF_UNSPEC ||
                                      lo4Obj.sa.ss.ss_family == AF_UNSPEC ||
                                      llaObj.sa.ss.ss_family == AF_UNSPEC ||
                                      guaObj.sa.ss.ss_family == AF_UNSPEC ||
                                      ip4Obj.sa.ss.ss_family == AF_UNSPEC));
}


class SsdpDeviceFTestSuite : public ::testing::Test {
  protected:
    CPupnplog m_logObj; // Output only with build type DEBUG.

    // Constructor
    SsdpDeviceFTestSuite() {
        if (g_dbug)
            m_logObj.enable(UPNP_ALL);

        // Destroy global variables to detect side effects.
        memset(&gIF_NAME, 0xAA, sizeof(gIF_NAME));
        memset(&gIF_IPV4, 0xAA, sizeof(gIF_IPV4));
        memset(&gIF_IPV4_NETMASK, 0xAA, sizeof(gIF_IPV4_NETMASK));
        memset(&gIF_IPV6, 0xAA, sizeof(gIF_IPV6));
        gIF_IPV6_PREFIX_LENGTH = ~0u;
        memset(&gIF_IPV6_ULA_GUA, 0xAA, sizeof(gIF_IPV6_ULA_GUA));
        gIF_IPV6_ULA_GUA_PREFIX_LENGTH = ~0u;
        gIF_INDEX = ~0u;
        // memset(&UpnpSdkInit, 0xAA, sizeof(UpnpSdkInit));
        memset(&errno, 0xAA, sizeof(errno));
        memset(&GlobalHndRWLock, 0xAA, sizeof(GlobalHndRWLock));
        memset(&gUpnpSdkNLSuuid, 0xAA, sizeof(gUpnpSdkNLSuuid));
        // memset(&HandleTable, 0xAA, sizeof(HandleTable));
        memset(&gSendThreadPool, 0xAA, sizeof(gSendThreadPool));
        memset(&gRecvThreadPool, 0xAA, sizeof(gRecvThreadPool));
        memset(&gMiniServerThreadPool, 0xAA, sizeof(gMiniServerThreadPool));
        memset(&gTimerThread, 0xAA, sizeof(gTimerThread));
        memset(&bWebServerState, 0xAA, sizeof(bWebServerState));
#if 0 // #ifdef UPnPsdk_WITH_NATIVE_PUPNP
        memset(&gWebMutex, 0xAA, sizeof(gWebMutex));
        memset(&GlobalClientSubscribeMutex, 0xAA,
               sizeof(GlobalClientSubscribeMutex));
#endif
    }
};
typedef SsdpDeviceFTestSuite SsdpDeviceFDeathTest;


TEST_F(SsdpDeviceFTestSuite, NewRequestHandler_successful) {
    // Used global variables:
    strcpy(gIF_IPV4, "0.0.0.0"); // Used to listen on all local net interfaces.
    gIF_INDEX = llaObj.index;

    // If I use productive ssdp multicast addresses with link-local scope
    // ("[ff02::c]", IPv4 ttl = 1), or site local scope ("[ff05::c], IPv4 ttl >
    // 1) I would spam the users network with this unusable multicast test
    // messages. So I use the destination address "[ff01::c]" and set the IPv4
    // ttl to 0. This sets the scope to interface-local. The messages don't
    // leave the interface.
    SSockaddr destaddr_ip6;
    // Use interface-scoped address here. I test only sending multicast data.
    // destaddr_ip6 = "[ff02::c]:1900";
    destaddr_ip6 = "[ff01::c]:1900";
    destaddr_ip6.sin6.sin6_scope_id = llaObj.index;
    SSockaddr destaddr_ip4;
    // destaddr_ip4 = "239.255.255.250:1900";
    destaddr_ip4 = "239.194.47.250:1900"; // My test multicast group
    constexpr int ip4ttl{1};

    char msg1[]{"Multicast message 1."};
    char msg2[]{""};
    char msg3[]{"Multicast message 3."};
    char* RqPacket[3]{msg1, msg2, msg3};

    if (old_code)
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": On MacOS sendto() must not fail due to wrong sizeof() "
                     "destination socket address.\n"; // Marked "Wrong!" below.

    int ret_NewRequestHandler{UPNP_E_INTERNAL_ERROR};
    // We have a trivial C-style array, so the size (num_pkg) must fit.
    // Otherwise we get segfaults.
    for (int num_pkg{1}; num_pkg <= 3; num_pkg++) {
        // Test Unit with IPv6 interface-local multicast.
        ret_NewRequestHandler =
            ::NewRequestHandler(&destaddr_ip6.sa, num_pkg, &RqPacket[0]);
#ifdef __APPLE__
        if (old_code)
            EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SOCKET_WRITE) // Wrong!
                << errStrEx(ret_NewRequestHandler, UPNP_E_SOCKET_WRITE);
        else
            EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SUCCESS)
                << errStrEx(ret_NewRequestHandler, UPNP_E_SUCCESS);
#else
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

TEST_F(SsdpDeviceFTestSuite, NewRequestHandler_with_ipv4_unicast_addr_fails) {
    // Used global variables:
    strcpy(gIF_IPV4, "0.0.0.0"); // Used to listen on all local net interfaces.
    gIF_INDEX = llaObj.index;

    if (!g_dbug) // Always enable for this test.
        m_logObj.enable(UPNP_ALL);

    constexpr int num_pkg{1};
    char msg1[]{"UPnPsdk test multicast message 1"};
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
    destaddr = "127.0.0.1";
#endif
    int ret_NewRequestHandler =
        ::NewRequestHandler(&destaddr.sa, num_pkg, &RqPacket[0]);
    EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SOCKET_WRITE)
        << errStrEx(ret_NewRequestHandler, UPNP_E_SOCKET_WRITE);
}

#if 0
TEST_F(SsdpDeviceFTestSuite, NewRequestHandler_with_unicast_addr_fails) {
    if (!g_dbug)      // Always enable for this test.
        m_logObj.enable(UPNP_ALL);

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

TEST_F(SsdpDeviceFDeathTest, NewRequestHandler_with_no_message) {
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

TEST_F(SsdpDeviceFTestSuite, NewRequestHandler_without_messages_succeeds) {
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

TEST_F(SsdpDeviceFDeathTest, NewRequestHandler_without_dest_addr_fails) {
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
#endif

#ifndef UPnPsdk_WITH_NATIVE_PUPNP
TEST_F(SsdpDeviceFTestSuite, NewRequestHandlerIPv4) {
    SSockaddr destaddr_ip4;
    // destaddr_ip4 = "239.255.255.250:1900";
    destaddr_ip4 = "239.194.47.250:1900"; // My test multicast group
    constexpr int ip4ttl{1};

    char msg1[]{"Multicast message 1"};
    char msg3[]{"mcast msg 3"};
    char msg4[]{""};
    char* msgs[4]{msg1, nullptr, msg3, msg4};

    // Test Unit
    int ret_NewRequestHandlerIPv4 =
        ::NewRequestHandlerIPv4(&destaddr_ip4.sa, 4, &msgs[0], ip4ttl);
    EXPECT_EQ(ret_NewRequestHandlerIPv4, UPNP_E_SUCCESS)
        << errStrEx(ret_NewRequestHandlerIPv4, UPNP_E_SUCCESS);
}

TEST_F(SsdpDeviceFTestSuite, NewRequestHandlerIPv6) {
    SSockaddr destaddr_ip6;
    // Use interface-scoped address here. I test only sending multicast data.
    // destaddr_ip6 = "[ff02::c]:1900";
    destaddr_ip6 = "[ff01::c]:1900";
#ifdef __APPLE__
    // MacOS needs this, otherwise ::sendto() will fail.
    // Other platforms will select an appropriate scope_id.
    destaddr_ip6.sin6.sin6_scope_id = llaObj.index;
#endif

    char msg1[]{"Multicast message 1"};
    char msg3[]{"mcast msg 3"};
    char msg4[]{""};
    char* msgs[4]{msg1, nullptr, msg3, msg4};

    // Test Unit
    int ret_NewRequestHandlerIPv6 =
        ::NewRequestHandlerIPv6(&destaddr_ip6.sa, 4, &msgs[0]);
    EXPECT_EQ(ret_NewRequestHandlerIPv6, UPNP_E_SUCCESS)
        << errStrEx(ret_NewRequestHandlerIPv6, UPNP_E_SUCCESS);
}
#endif

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
    utest::get_netadapter();
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
