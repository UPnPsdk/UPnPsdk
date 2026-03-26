// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-03-26

// This tests network communication. The usual way to do it is to use mocking to
// be independent from current hardware. But with mocking you can only test what
// you expect, not the behavior with the real system. Here with different
// platforms it is difficult to know it. So I decided to test with the real
// system calls and the current network adapter. To know the current hardware on
// different platforms I have made a function 'get_netadapter()' (see below) to
// provide the needed information. However, it must be noted that no DNS lookups
// triggered and that the current network communication isn't spammed.

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
#include <umock/netdb_mock.hpp>


namespace utest {

using ::testing::_;
using ::testing::DoAll;
using ::testing::ExitedWithCode;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;

using ::UPnPsdk::CNetadapter;
using ::UPnPsdk::errStrEx;
using ::UPnPsdk::g_dbug;
using ::UPnPsdk::SSockaddr;


// Interface-local multicast adress is not official defined for SSDP but
// setting its scope for testing works. I use it instead of the only
// specified link-local multicast address to not spam the users network.
constexpr char SSDP_MCAST_IFACE_LOCAL[]{"[ff01::c]:1900"};
[[maybe_unused]] constexpr char SSDP_MCAST_LINK_LOCAL[]{"[ff02::c]:1900"};

enum struct Idx {
    no,  // no address
    lo6, // IPv6 loopback address
    lo4, // IPv4 loopback address
    lla, // link-local address
    gua, // global unicast address
    ip4  // IPv4 address
};

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
    // Although looking for IPv4 addresses on a local network adapter for
    // completness this function should never find one because the used
    // CNetadapter class to get info maps it direct to an IPv6 address.
    //
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
        // Repeat only as long as an adapter type is unspecified.
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
        if (g_dbug) {
            m_logObj.enable(UPNP_ALL);
            // Don't remove second call. It's a test for fixed double enable
            // bug that randomely triggered an uncatchable critical exception
            // "stack overflow" with test crash.
            m_logObj.enable(UPNP_ALL);
        }

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
        memset(&GlobalClientSubscribeMutex, 0xAA,
               sizeof(GlobalClientSubscribeMutex));
#endif
    }
};
typedef SsdpDeviceFTestSuite SsdpDeviceFDeathTest;


TEST_F(SsdpDeviceFTestSuite, NewRequestHandler_successful) {
    // The points for old_code are:
    // * NewRequestHandler() has a bug. For an IPv6 destinnation address,
    //   system call '::sendto()' send with an oversized socket-address size
    //   'sockaddr_storage' instead of exactly 'sockaddr_in6'. This works
    //   except on MacOS. That platform expects 'sockaddr_in6" for sending to
    //   an IPv6 destination address. Sending an IPv6 on NacOs always fails.
    // * It is unclear what local network interface should be used. All
    //   possible local interfaces are got with system call '::getaddrinfo()'
    //   and in a loop they are '::bind()' and tried to send. But after binding
    //   the socket to address from '::getaddrinfo()', they are immediately
    //   overwritten with socket option IPV[6]_MULTICAST_IF with global address
    //   from gIF_IPv4 or gIF_IPv6. So the source is always the global
    //   gIF-address. '::getaddrinfo()' has no effect.

    //   One test message.
    constexpr int num_msg{1};
    char msg1[]{"UPnPsdk send test message UDP DGRAM"};
    char* msgs[num_msg]{msg1};

    // Prepare other used parameter.
    SSockaddr destsaObj;
    destsaObj = SSDP_MCAST_IFACE_LOCAL;
    if (old_code) {
        gIF_INDEX = llaObj.index;
        strcpy(gIF_IPV4, "0.0.0.0");
    } else {
#ifdef __APPLE__
        // Apple needs to know what local network interface to use.
        destsaObj.sin6.sin6_scope_id = llaObj.index;
#endif
    }

    // Test Unit
    int ret_NewRequestHandler =
        ::NewRequestHandler(&destsaObj.sa, num_msg, &msgs[0]);
#if defined(UPnPsdk_WITH_NATIVE_PUPNP) && defined(__APPLE__)
    EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR)
        << errStrEx(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR);
#else
    EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SUCCESS)
        << errStrEx(ret_NewRequestHandler, UPNP_E_SUCCESS);
#endif
}

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
TEST_F(SsdpDeviceFTestSuite, NewRequestHandler_mock_successful) {
    // One test message.
    constexpr int num_msg{1};
    char msg1[]{"UPnPsdk test message mocking send UDP DGRAM"};
    char* msgs[num_msg]{msg1};

    // Prepare other used parameter.
    SSockaddr destsaObj;
    destsaObj = SSDP_MCAST_IFACE_LOCAL;
    gIF_INDEX = llaObj.index;
    strcpy(gIF_IPV4, "0.0.0.0");

    ::addrinfo res{};
    // res.ai_flags = 0;
    res.ai_family = AF_INET6;
    res.ai_socktype = SOCK_DGRAM;
    // res.ai_protocol = 0;
    res.ai_addrlen = llaObj.sa.sizeof_saddr();
    res.ai_addr = &llaObj.sa.sa;
    // res.ai_canonname = nullptr;
    // res.ai_next = nullptr;

    // Instantiate mocking object.
    StrictMock<umock::NetdbMock> netdbObj;
    // Inject the mocking object into the tested code.
    umock::Netdb netdb_injectObj = umock::Netdb(&netdbObj);
    ON_CALL(netdbObj, getaddrinfo(_, _, _, _))
        .WillByDefault(Return(EAI_FAMILY));

    // Mock 'CAddrinfo::get_first()'
    EXPECT_CALL(netdbObj, getaddrinfo(nullptr, Pointee(*"1900"), _, _))
        .WillOnce(DoAll(SetArgPointee<3>(&res), Return(0)));
    // Mock 'freeaddrinfo()'
    EXPECT_CALL(netdbObj, freeaddrinfo(&res)).Times(1);

    // Test Unit
    int ret_NewRequestHandler =
        ::NewRequestHandler(&destsaObj.sa, num_msg, &msgs[0]);
#ifdef __APPLE__
    // Due to bug with wrong socket address length on sendto().
    EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR)
        << errStrEx(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR);
#else
    EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SUCCESS)
        << errStrEx(ret_NewRequestHandler, UPNP_E_SUCCESS);
#endif
}
#endif


class SendStatelessTest
    : public ::testing::TestWithParam<std::tuple<
          // netaddr, netadapter index, retval old_code, retval new_code
          const std::string, const Idx, const int, const int>> {
  protected:
    CPupnplog m_logObj; // Output only with build type DEBUG.
};

TEST_P(SendStatelessTest, send_stateless) {
    // Get parameter
    const std::tuple params = GetParam();

    SSockaddr destsaObj;
    destsaObj = std::get<0>(params);

    uint32_t na_idx;
    switch (std::get<1>(params)) {
    case Idx::no:
        // If we want to test a GUA but there is no adapter with a global
        // unicast address I flag to skip the test.
        if (destsaObj.ss.ss_family == AF_INET6 &&
            IN6_IS_ADDR_GLOBAL(&destsaObj.sin6.sin6_addr) && guaObj.index == 0)
            // Flag to skip test.
            na_idx = 0u;
        else
            // Flag run test with scope_id set to 0.
            na_idx = ~0u;
        break;
    case Idx::lo6:
        na_idx = lo6Obj.index;
        break;
    case Idx::lo4:
        na_idx = lo4Obj.index;
        break;
    case Idx::lla:
        na_idx = llaObj.index;
        break;
    case Idx::gua:
        na_idx = guaObj.index;
        break;
    case Idx::ip4:
        na_idx = ip4Obj.index;
        break;
    default:
        GTEST_FAIL();
    }
    if (na_idx == 0)
        GTEST_SKIP() << "No local network adapter found with usable source "
                        "address to connect to remote \""
                     << destsaObj.netaddrp() << "\".";

    const int ret_old = std::get<2>(params);
    const int ret_new = std::get<3>(params);

    // One test message.
    constexpr int num_msg{1};
    char msg1[]{"UPnPsdk test sending stateless"};
    char* msgs[num_msg]{msg1};

    // Clear used global variable.
    gIF_INDEX = ~0u;
    // We need a valid IPv4 source address for old_code, also if there is only
    // IPv6 used. This is fixed in compatible code. I assume INADDR_ANY to be
    // valid.
    strcpy(gIF_IPV4, "0.0.0.0"); // INADDR_ANY

    switch (destsaObj.ss.ss_family) {
    case AF_INET6:
        gIF_INDEX = (na_idx == ~0u ? 0u : na_idx);
        destsaObj.sin6.sin6_scope_id = gIF_INDEX;
        break;
    case AF_INET:
        break;
    default:
        GTEST_FAIL();
    }
    std::cout << "             destsaObj=" << destsaObj << '\n';
    if (g_dbug)
        m_logObj.enable(UPNP_ALL);

    // Test Unit
    int ret_NewRequestHandler =
        ::NewRequestHandler(&destsaObj.sa, num_msg, &msgs[0]);

    if (old_code)
        EXPECT_EQ(ret_NewRequestHandler, ret_old)
            << errStrEx(ret_NewRequestHandler, ret_old);
    else
        EXPECT_EQ(ret_NewRequestHandler, ret_new)
            << errStrEx(ret_NewRequestHandler, ret_new);
}

// This is a complete test of the NewRequestHandler() for sending stateless IP
// packets (SOCK_DGRAM). Summary is that there is no difference between unicast
// and multicast destination addresses. Old native Pupnp code must have an
// initialized node local IPv4 network address (program global gIF_IPV4
// variable) even if only the IPv6 network stack is used. I use an
// interface-scoped address here for testing to avoid spamming users network:
// destaddr_ip6 = "[ff01::c]:1900", not destaddr_ip6 = "[ff02::c]:1900". Some
// IP addresses need a scope_id. That is the 'uint32_t sin6_scope_id' field in
// the 'sockaddr_in6' structure and set with the local netadapter index. The
// supported platforms have different behavior with using system call
// ::sendto(), that is for
//
// Unix/Linux like platforms:
// - Any destination internet address needs a port, otherwise ::sendto() fails.
// - No destination IPv6 address (LLA, GUA, MCAST) needs a scope_id, but may be
//   specified. If not given then the operating system will use an appropriate
//   one.
// - An IPv4 address (unicast and multicast) with port always succeeds.
// - Sending to the loopback device with port is supported.
// - Recommendation: Use a destination socket address always with a specified
//   port.
//
// MacOS:
// - Any destination internet address needs a port, otherwise ::sendto() fails.
// - A destination GUA does not need a scope_id, but may be specified. If not
//   given then the operating system will use an appropriate one.
// - A destination LLA and multicast address needs a scope_id, otherwise
//   ::sendto() fails.
// - An IPv4 address (unicast and multicast) with port always succeeds.
// - Sending to the loopback device with port is supported.
// - Recommendation: Use a destination socket address always with a specified
//   port. When using an LLA or multicast address then specify an additional
//   scope_id.
//
// Microsoft Windows Winsock library:
// - No destination internet address needs a port, but may be specified. If not
//   given then the operating system will use an appropriate one.
// - Sending to a destination GUA is only possible with no scope_id specified,
//   or with a scope_id to a local netadapter with a GUA. Other settings fail.
// - Sending to an LLA, to an IPv4 unicast address, or to any multicast address
//   (IPv6, IPv4) always succeeds. Any setting or not of a scope_id is ignored.
// - Sending to the loopback device with or without port, with no scope_id, or
//   with a scope_id to a local netadapter with a GUA is supported. Otherwise
//   it fails.
// - Recommendation: Using a destination socket address with unspecified
//   scope_id, and a port set or unset as you like will always succeed (without
//   other system errors).
//
// Common Recommendation for all:
//  It is that for MacOs. With its most restrictions it is the lowest common
//  denominator.
//
//  Helpful analysis on a second console with:
//  ~$ sudo tcpdump -nnni any udp or icmp or icmp6
//
// clang-format off
#if !defined(__APPLE__) && !defined(_MSC_VER)
INSTANTIATE_TEST_SUITE_P(SendStateless, SendStatelessTest, ::testing::Values(
    //     netaddr, local netadapter index, result old_code,     result new_code
    /*0*/ std::make_tuple("[::1]", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[::1]", Idx::lo6, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[::1]:1901", Idx::no, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[::1]:1902", Idx::lo6, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[::1]:1903", Idx::lla, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[::1]:1904", Idx::gua, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[::ffff:127.0.0.1]", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[::ffff:127.0.0.1]", Idx::lo6, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[::ffff:127.0.0.1]:1901", Idx::no, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[::ffff:127.0.0.1]:1902", Idx::lo6, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    /*10*/ std::make_tuple("[2001:db8:2747::c021]", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[2001:db8:2747::c021]", Idx::lla, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[2001:db8:2747::c022]", Idx::gua, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    // std::make_tuple("[2001:db8:2747::c023]:1900", Idx::no, UPNP_E_SUCCESS, UPNP_E_SUCCESS), // Spam to user network
    // std::make_tuple("[2001:db8:2747::c024]:1900", Idx::gua, UPNP_E_SUCCESS, UPNP_E_SUCCESS), // Spam to user network
    std::make_tuple("[fe80::20c:fe7f:c021]", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[fe80::20c:fe7f:c022]", Idx::lla, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[fe80::20c:fe7f:c023]", Idx::gua, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    // std::make_tuple("[fe80::20c:fe7f:c024]:1900", Idx::no, UPNP_E_SUCCESS, UPNP_E_SUCCESS), // Spam to user network with ICMP6, neighbor solicitation
    // std::make_tuple("[fe80::20c:fe7f:c025]:1900", Idx::lla, UPNP_E_SUCCESS, UPNP_E_SUCCESS), // Spam to user network with ICMP6, neighbor solicitation
    // std::make_tuple("[fe80::20c:fe7f:c026]:1900", Idx::gua, UPNP_E_SUCCESS, UPNP_E_SUCCESS), // Spam to user network with ICMP6, neighbor solicitation
    // Multicast interface-local
    std::make_tuple("[ff01::c]", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[ff01::c]", Idx::lla, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[ff01::c]", Idx::gua, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[ff01::c]:1901", Idx::no, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[ff01::c]:1902", Idx::lla, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[ff01::c]:1903", Idx::gua, UPNP_E_SUCCESS, UPNP_E_SUCCESS),

    std::make_tuple("[::ffff:10.178.1.1]", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[::ffff:10.178.1.2]", Idx::gua, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[::ffff:10.178.1.3]", Idx::ip4, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    // std::make_tuple("[::ffff:10.178.1.4]:1900", Idx::no, UPNP_E_SUCCESS, UPNP_E_SUCCESS), // Spam to user network
    // std::make_tuple("[::ffff:10.178.1.5]:1900", Idx::gua, UPNP_E_SUCCESS, UPNP_E_SUCCESS), // Spam to user network
    std::make_tuple("[::ffff:10.178.1.6]:1900", Idx::ip4, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE), // Due to IPV4_MAPPED_IPV6 never found on an adapter
    std::make_tuple("[::ffff:239.132.38.181]", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[::ffff:239.132.38.182]", Idx::gua, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE)
    // std::make_tuple("[::ffff:239.132.38.183]:1900", Idx::no, UPNP_E_SUCCESS, UPNP_E_SUCCESS) // Spam to user network
    // std::make_tuple("[::ffff:239.132.38.184]:1900", Idx::gua, UPNP_E_SUCCESS, UPNP_E_SUCCESS) // Spam to user network
));
#endif
#ifdef __APPLE__
INSTANTIATE_TEST_SUITE_P(SendStateless, SendStatelessTest, ::testing::Values(
    //     netaddr, local netadapter index, result old,          result new
    /*0*/ std::make_tuple("[::1]", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[::1]", Idx::lo6, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[::1]:1901", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SUCCESS), // old_code does not accept loopback as multicast soure address.
    std::make_tuple("[::1]:1902", Idx::lo6, UPNP_E_SOCKET_ERROR, UPNP_E_SUCCESS), // old_code bug: sendto() fails with errno=22 - Invalid argument
    std::make_tuple("[::1]:1903", Idx::lla, UPNP_E_SOCKET_ERROR, UPNP_E_SUCCESS), // old_code bug: sendto() fails with errno=22 - Invalid argument
    std::make_tuple("[::3]:1904", Idx::gua, UPNP_E_SOCKET_ERROR, UPNP_E_SUCCESS), // old_code bug: sendto() fails with errno=22 - Invalid argument
    std::make_tuple("[::ffff:127.0.0.1]", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[::ffff:127.0.0.1]", Idx::lo6, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[::ffff:127.0.0.1]:1901", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SUCCESS), // Old code failed due to bug with wrong address size
    std::make_tuple("[::ffff:127.0.0.1]:1902", Idx::lo6, UPNP_E_SOCKET_ERROR, UPNP_E_SUCCESS), // Old code failed due to bug with wrong address size
    /*10*/ std::make_tuple("[2001:db8:2747::c021]", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[2001:db8:2747::c021]", Idx::lla, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[2001:db8:2747::c022]", Idx::gua, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    // std::make_tuple("[2001:db8:2747::c023]:1900", Idx::no, UPNP_E_SUCCESS, UPNP_E_SUCCESS), // Spam to user network
    // std::make_tuple("[2001:db8:2747::c024]:1900", Idx::gua, UPNP_E_SUCCESS, UPNP_E_SUCCESS), // Spam to user network
    std::make_tuple("[fe80::20c:fe7f:c021]", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[fe80::20c:fe7f:c022]", Idx::lla, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[fe80::20c:fe7f:c023]", Idx::gua, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[fe80::20c:fe7f:c024]:1900", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE), // macOS always wants a scope_id on lla
    // std::make_tuple("[fe80::20c:fe7f:c025]:1900", Idx::lla, UPNP_E_SOCKET_ERROR, UPNP_E_SUCCESS), // old_code bug: sendto() fails with errno=22 - Invalid argument, new code: spam to user network with ICMP6, neighbor solicitation
    // /*20*/ std::make_tuple("[fe80::20c:fe7f:c026]:1900", Idx::gua, UPNP_E_SOCKET_ERROR, UPNP_E_SUCCESS), // old_code bug: sendto() fails with errno=22 - Invalid argument, new code: spam to user network with ICMP6, neighbor solicitation
    // Multicast interface-local
    std::make_tuple("[ff01::c]", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[ff01::c]", Idx::lla, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[ff01::c]", Idx::gua, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[ff01::c]:1901", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE), // macOS always wants a scope_id on multicast destination
    std::make_tuple("[ff01::c]:1902", Idx::lla, UPNP_E_SOCKET_ERROR, UPNP_E_SUCCESS), // old_code bug: sendto() fails with errno=22 - Invalid argument
    std::make_tuple("[ff01::c]:1903", Idx::gua, UPNP_E_SOCKET_ERROR, UPNP_E_SUCCESS), // old_code bug: sendto() fails with errno=22 - Invalid argument

    std::make_tuple("[::ffff:10.178.1.1]", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[::ffff:10.178.1.2]", Idx::gua, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[::ffff:10.178.1.3]", Idx::ip4, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    // std::make_tuple("[::ffff:10.178.1.4]:1900", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SUCCESS), // Old code failed due to bug with wrong address size, new code: spam to user network
    // /*30*/ std::make_tuple("[::ffff:10.178.1.5]:1900", Idx::gua, UPNP_E_SUCCESS, UPNP_E_SUCCESS), // Spam to user network
    std::make_tuple("[::ffff:10.178.1.6]:1900", Idx::ip4, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE), // Due to IPV4_MAPPED_IPV6 never found on an adapter
    std::make_tuple("[::ffff:239.132.38.181]", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE),
    std::make_tuple("[::ffff:239.132.38.182]", Idx::gua, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE)
    // std::make_tuple("[::ffff:239.132.38.183]:1900", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SUCCESS), // Old code failed due to bug with wrong address size, new code: spam to user network
    // std::make_tuple("[::ffff:239.132.38.184]:1900", Idx::gua, UPNP_E_SUCCESS, UPNP_E_SUCCESS) // Spam to user network
));
#endif
#ifdef _MSC_VER
INSTANTIATE_TEST_SUITE_P(SendStateless, SendStatelessTest, ::testing::Values(
    //     netaddr, local netadapter index, result old,          result new
    /*0*/ std::make_tuple("[::1]", Idx::no, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[::1]", Idx::lo6, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE), // with wrong scope_id is not accepted
    std::make_tuple("[::1]", Idx::lla, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE), // with wrong scope_id is not accepted
    std::make_tuple("[::1]", Idx::gua, UPNP_E_SUCCESS, UPNP_E_SUCCESS), // Curiously the loopback interface is accepted as gua
    std::make_tuple("[::1]:1900", Idx::no, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[::1]:1900", Idx::lo6, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE), // with wrong scope_id is not accepted
    std::make_tuple("[::1]:1900", Idx::lla, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE), // with wrong scope_id is not accepted
    std::make_tuple("[::1]:1900", Idx::gua, UPNP_E_SUCCESS, UPNP_E_SUCCESS), // Curiously the loopback interface is accepted as gua
    std::make_tuple("[::ffff:127.0.0.1]", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SUCCESS), // old code doesn't support IPv4 mapped IPv6
    std::make_tuple("[::ffff:127.0.0.1]", Idx::lo6, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE), // old code doesn't support IPv4 mapped IPv6
    /*10*/ std::make_tuple("[::ffff:127.0.0.1]:1900", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SUCCESS), // old code doesn't support IPv4 mapped IPv6
    std::make_tuple("[::ffff:127.0.0.1]:1900", Idx::lo6, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE), // old code doesn't support IPv4 mapped IPv6
    std::make_tuple("[2001:db8:2747::c021]", Idx::no, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[2001:db8:2747::c021]", Idx::lla, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE), // with wrong scope_id is not accepted
    std::make_tuple("[2001:db8:2747::c021]", Idx::gua, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[2001:db8:2747::c021]:1900", Idx::no, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[2001:db8:2747::c021]:1900", Idx::gua, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[fe80::20c:fe7f:c021]", Idx::no, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[fe80::20c:fe7f:c021]", Idx::lla, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[fe80::20c:fe7f:c021]", Idx::gua, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    /*20*/ std::make_tuple("[fe80::20c:fe7f:c021]:1900", Idx::no, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[fe80::20c:fe7f:c021]:1900", Idx::lla, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[fe80::20c:fe7f:c021]:1900", Idx::gua, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    // Multicast interface-local
    std::make_tuple("[ff01::c]", Idx::no, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[ff01::c]", Idx::lla, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[ff01::c]", Idx::gua, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[ff01::c]:1900", Idx::no, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[ff01::c]:1900", Idx::lla, UPNP_E_SUCCESS, UPNP_E_SUCCESS),
    std::make_tuple("[ff01::c]:1900", Idx::gua, UPNP_E_SUCCESS, UPNP_E_SUCCESS),

    std::make_tuple("[::ffff:10.178.1.1]", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SUCCESS), // old code doesn't support IPv4 mapped IPv6
    /*30*/ std::make_tuple("[::ffff:10.178.1.2]", Idx::lla, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE), // old code doesn't support IPv4 mapped IPv6
    std::make_tuple("[::ffff:10.178.1.3]", Idx::gua, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE), // old code doesn't support IPv4 mapped IPv6
    // std::make_tuple("[::ffff:10.178.1.4]", Idx::ip4, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE), // Due to IPV4_MAPPED_IPV6 never found on an adapter
    std::make_tuple("[::ffff:10.178.1.5]:1900", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SUCCESS), // old code doesn't support IPv4 mapped IPv6
    std::make_tuple("[::ffff:10.178.1.6]:1900", Idx::lla, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE), // old code doesn't support IPv4 mapped IPv6
    std::make_tuple("[::ffff:10.178.1.7]:1900", Idx::gua, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE), // old code doesn't support IPv4 mapped IPv6
    // std::make_tuple("[::ffff:10.178.1.8]:1900", Idx::ip4, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE), // Due to IPV4_MAPPED_IPV6 never found on an adapter
    std::make_tuple("[::ffff:239.132.38.179]", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SUCCESS), // old code doesn't support IPv4 mapped IPv6
    std::make_tuple("[::ffff:239.132.38.179]", Idx::gua, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE), // old code doesn't support IPv4 mapped IPv6
    /*40*/ std::make_tuple("[::ffff:239.132.38.179]:1900", Idx::no, UPNP_E_SOCKET_ERROR, UPNP_E_SUCCESS), // old code doesn't support IPv4 mapped IPv6
    std::make_tuple("[::ffff:239.132.38.179]:1900", Idx::gua, UPNP_E_SOCKET_ERROR, UPNP_E_SOCKET_WRITE) // old code doesn't support IPv4 mapped IPv6
));
#endif
// clang-format on


TEST_F(SsdpDeviceFTestSuite,
       NewRequestHandler_send_multiple_mulitcast_messages) {
    // Some test messages.
    // Fails
    constexpr int num_msg{3};
    char msg1[]{"UPnPsdk test multicast message 1"};
    char msg2[]{""};
    char msg3[]{"UPnPsdk test mcast msg 3"};
    char* msgs[num_msg]{msg1, msg2, msg3};

    // Try to find a random critical exception with MS Windows on githup action
    // with no avail. The exception has dissapeared. Maybe there was a problem
    // with the github runner image?
    // Succeeds
    // constexpr int num_msg{1};
    // char msg1[]{"UPnPsdk test multicast message 1"};
    // char* msgs[num_msg]{msg1};

    // Succeeds
    // constexpr int num_msg{1};
    // char msg1[]{""};
    // char* msgs[num_msg]{msg1};

    // Succeeds
    // constexpr int num_msg{2};
    // char msg1[]{"UPnPsdk test multicast message 1"};
    // char msg2[]{""};
    // char* msgs[num_msg]{msg1, msg2};

    // Succeeds
    // constexpr int num_msg{2};
    // char msg1[]{""};
    // char msg2[]{"UPnPsdk test multicast message 1"};
    // char* msgs[num_msg]{msg1, msg2};

    SSockaddr destaddr_ip6;
    destaddr_ip6 = SSDP_MCAST_IFACE_LOCAL; // interface-local

    if (old_code) {
        strcpy(gIF_IPV4, "0.0.0.0"); // INADDR_ANY
        gIF_INDEX = 0; // Can be 0. No need to specify correct llaObj.index;

        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": On MacOS sendto() must not fail due to wrong sizeof() "
                     "destination socket address.\n"; // Marked "Wrong!" below.

        // Test Unit with multicast address, with port, and lla scope_id.
        int ret_NewRequestHandler =
            ::NewRequestHandler(&destaddr_ip6.sa, num_msg, &msgs[0]);
#ifdef __APPLE__
        // Error due to bug wrong socket address length for IPv6 on sendto().
        EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR)
            << errStrEx(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR);
#else
        EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SUCCESS)
            << errStrEx(ret_NewRequestHandler, UPNP_E_SUCCESS);
#endif

    } else {

#ifdef __APPLE__
        destaddr_ip6.sin6.sin6_scope_id = llaObj.index;
#endif
        // Test Unit with multicast address, and with port.
        int ret_NewRequestHandler =
            ::NewRequestHandler(&destaddr_ip6.sa, num_msg, &msgs[0]);
        EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SUCCESS)
            << errStrEx(ret_NewRequestHandler, UPNP_E_SUCCESS);
    }
}

TEST_F(SsdpDeviceFTestSuite, NewRequestHandler_send_zero_messages_succeeds) {
    constexpr int num_msg{0}; // Zero messages selected.
    char msg1[]{"<not used>"};
    char* msgs[1]{msg1};

    SSockaddr destaddr_ip6;
    destaddr_ip6 = SSDP_MCAST_IFACE_LOCAL; // interface-local

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    strcpy(gIF_IPV4, "0.0.0.0"); // INADDR_ANY
    gIF_INDEX = 0; // Can be 0. No need to specify correct llaObj.index;
#endif

    // Test Unit
    int ret_NewRequestHandler =
        ::NewRequestHandler(&destaddr_ip6.sa, num_msg, &msgs[0]);
#ifdef __APPLE__
    if (old_code)
        EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR)
            << errStrEx(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR);
    else
#endif
        EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SUCCESS)
            << errStrEx(ret_NewRequestHandler, UPNP_E_SUCCESS);
}

TEST_F(SsdpDeviceFDeathTest, NewRequestHandler_no_messages_addressed_fails) {
    constexpr int num_msg{1};
    // char msg1[]{"UPnPsdk test multicast message 1"}; // unused
    // char* msgs[1]{msg1};

    SSockaddr destaddr;
    destaddr = SSDP_MCAST_IFACE_LOCAL;

    if (old_code) {
        strcpy(gIF_IPV4, "0.0.0.0"); // INADDR_ANY

        // Index greater than available interfaces fails before segfault.
        gIF_INDEX = ~0u;
        int ret_NewRequestHandler =
            ::NewRequestHandler(&destaddr.sa, num_msg, nullptr);
        EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR)
            << errStrEx(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR);

        std::cout
            << CYEL "[ BUGFIX   ]" CRES
            << " Pointing to no message array (nullptr) must not segfault.\n";

        gIF_INDEX = llaObj.index; // Trigger segfault with valid IPv6 interace.
        EXPECT_DEATH(::NewRequestHandler(&destaddr.sa, num_msg, nullptr), ".*");

    } else {

        ASSERT_EXIT(
            (::NewRequestHandler(&destaddr.sa, num_msg, nullptr), exit(0)),
            ExitedWithCode(0), ".*");
        int ret_NewRequestHandler =
            ::NewRequestHandler(&destaddr.sa, num_msg, nullptr);
        EXPECT_EQ(ret_NewRequestHandler, UPNP_E_INVALID_PARAM)
            << errStrEx(ret_NewRequestHandler, UPNP_E_INVALID_PARAM);
    }
}

TEST_F(SsdpDeviceFDeathTest, NewRequestHandler_send_no_message_succeeds) {
    constexpr int num_msg{1};
    char* msgs[num_msg]{nullptr};

    SSockaddr destaddr_ip6;
    destaddr_ip6 = SSDP_MCAST_IFACE_LOCAL; // interface-local

    if (old_code) {
        strcpy(gIF_IPV4, "0.0.0.0"); // INADDR_ANY
        gIF_INDEX = llaObj.index; // Trigger segfault with valid IPv6 interace.

        std::cout << CYEL "[ BUGFIX   ]" CRES
                  << " Pointing to no message (nullptr) must not segfault.\n";
        ASSERT_DEATH(::NewRequestHandler(&destaddr_ip6.sa, num_msg, &msgs[0]),
                     ".*");
    } else {

        // Test Unit with multicast address, and with port.
        ASSERT_EXIT(
            (::NewRequestHandler(&destaddr_ip6.sa, num_msg, &msgs[0]), exit(0)),
            ExitedWithCode(0), ".*");
        int ret_NewRequestHandler =
            ::NewRequestHandler(&destaddr_ip6.sa, num_msg, &msgs[0]);
        EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SUCCESS)
            << errStrEx(ret_NewRequestHandler, UPNP_E_SUCCESS);
    }
}

TEST_F(SsdpDeviceFDeathTest, NewRequestHandler_error_message_as_garbage) {
    constexpr int num_msg{1};
    char msg1[]{"Multicast message 1"};
    char* msgs[num_msg]{msg1};

    SSockaddr destaddr_ip6;
    // Without port, and with invalid scope_id triggers an error on all
    // platforms.
    destaddr_ip6 = "[::1]";

    int error_id;
    if (old_code) {
        strcpy(gIF_IPV4, "0.0.0.0"); // INADDR_ANY
        gIF_INDEX = ~0u;             // Invalid scope_id

        // This has to do with using strerror_r(). That has two versions
        // depending to be XSI-compliant or GNU-specific. The wrong version may
        // be selected. strerror() should be used.
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Enabled debug messages should not output garbage.\n";
        error_id = UPNP_E_SOCKET_ERROR;

    } else {

        destaddr_ip6.sin6.sin6_scope_id = ~0u; // Invalid scope_id
        error_id = UPNP_E_SOCKET_WRITE;
    }

    // Test Unit without port, and with wrong scope_id.
    int ret_NewRequestHandler =
        ::NewRequestHandler(&destaddr_ip6.sa, num_msg, &msgs[0]);
    EXPECT_EQ(ret_NewRequestHandler, error_id)
        << errStrEx(ret_NewRequestHandler, error_id);
}

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
// This test fails only on old_code. New code doesn't depend on global
// variable gIF_IPV4 and would succeed with spamming the user network.
TEST_F(SsdpDeviceFTestSuite,
       NewRequestHandler_send_with_uninitialized_global_ipv4_address) {
    constexpr int num_msg{1};
    char msg1[]{"Multicast message 1"};
    char* msgs[num_msg]{msg1};

    SSockaddr destaddr_ip4;
    destaddr_ip4 = "[::ffff:10.178.1.2]:1900";

    // strcpy(gIF_IPV4, "0.0.0.0"); // Not initialized.

    // Test Unit
    int ret_NewRequestHandler =
        ::NewRequestHandler(&destaddr_ip4.sa, num_msg, &msgs[0]);
    EXPECT_EQ(ret_NewRequestHandler, UPNP_E_INVALID_PARAM)
        << errStrEx(ret_NewRequestHandler, UPNP_E_INVALID_PARAM);
}
#endif

TEST_F(SsdpDeviceFDeathTest, NewRequestHandler_without_dest_addr_fails) {
    constexpr int num_msg{1};
    char msg1[]{"Multicast message 1"};
    char* msgs[num_msg]{msg1};

    strcpy(gIF_IPV4, "0.0.0.0"); // INADDR_ANY
    gIF_INDEX = 0; // Can be 0. No need to specify correct llaObj.index;

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ]" CRES
                  << " Pointing to no destination address (nullptr) must not "
                     "segfault.\n";
        // This expects segfault.
        EXPECT_DEATH(::NewRequestHandler(nullptr, num_msg, &msgs[0]), ".*");

    } else {

        // This expects NO segfault.
        ASSERT_EXIT((::NewRequestHandler(nullptr, num_msg, &msgs[0]), exit(0)),
                    ExitedWithCode(0), ".*");
        int ret_NewRequestHandler =
            ::NewRequestHandler(nullptr, num_msg, &msgs[0]);
        EXPECT_EQ(ret_NewRequestHandler, UPNP_E_INVALID_PARAM)
            << errStrEx(ret_NewRequestHandler, UPNP_E_INVALID_PARAM);
    }
}

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
TEST_F(SsdpDeviceFTestSuite, NewRequestHandler_with_multible_netadapter) {
    // One test message.
    constexpr int num_msg{1};
    char msg1[]{"UPnPsdk test message sending stateless"};
    char* msgs[num_msg]{msg1};

    // Prepare other used parameter.
    SSockaddr destsaObj;
    destsaObj = "[ff02::c]:1900";
    gIF_INDEX = llaObj.index;
    strcpy(gIF_IPV4, "0.0.0.0");

    SSockaddr sa0Obj;
    sa0Obj = "0.0.0.0"; // With AF_INET6, this should trigger an error.

    // Three possible local source addresses
    ::addrinfo res3{};
    res3.ai_flags = 0;
    res3.ai_family = AF_INET6; // ai_addr is AF_INET and trigger an error.
    res3.ai_socktype = SOCK_DGRAM;
    res3.ai_protocol = 0;
    res3.ai_addrlen = sa0Obj.sizeof_saddr();
    res3.ai_addr = &sa0Obj.sa; // This should trigger an error.
    res3.ai_canonname = nullptr;
    res3.ai_next = nullptr;

    ::addrinfo res2{};
    res2.ai_flags = 0;
    res2.ai_family = AF_INET6;
    res2.ai_socktype = SOCK_DGRAM;
    res2.ai_protocol = 0;
    res2.ai_addrlen = llaObj.sa.sizeof_saddr();
    res2.ai_addr = &llaObj.sa.sa; // This succeeds.
    res2.ai_canonname = nullptr;
    res2.ai_next = &res3;

    ::addrinfo res1{};
    res1.ai_flags = 0;
    res1.ai_family = AF_INET6; // ai_addr is AF_INET and trigger an error.
    res1.ai_socktype = SOCK_DGRAM;
    res1.ai_protocol = 0;
    res1.ai_addrlen = sa0Obj.sizeof_saddr();
    res1.ai_addr = &sa0Obj.sa; // This should trigger an error.
    res1.ai_canonname = nullptr;
    res1.ai_next = &res2;

    // Instantiate mocking object.
    StrictMock<umock::NetdbMock> netdbObj;
    // Inject the mocking object into the tested code.
    umock::Netdb netdb_injectObj = umock::Netdb(&netdbObj);
    ON_CALL(netdbObj, getaddrinfo(_, _, _, _))
        .WillByDefault(Return(EAI_FAMILY));

    // Mock 'CAddrinfo::get_first()'
    EXPECT_CALL(netdbObj, getaddrinfo(nullptr, Pointee(*"1900"), _, _))
        .WillOnce(DoAll(SetArgPointee<3>(&res1), Return(0)));
    // Mock 'freeaddrinfo()'
    EXPECT_CALL(netdbObj, freeaddrinfo(&res1)).Times(1);

    // Test Unit
    int ret_NewRequestHandler =
        ::NewRequestHandler(&destsaObj.sa, num_msg, &msgs[0]);
#ifdef __APPLE__
    // Due to bug with wrong socket address length on sendto().
    EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR)
        << errStrEx(ret_NewRequestHandler, UPNP_E_SOCKET_ERROR);
#else
    EXPECT_EQ(ret_NewRequestHandler, UPNP_E_SUCCESS)
        << errStrEx(ret_NewRequestHandler, UPNP_E_SUCCESS);
#endif
}
#endif

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
    utest::get_netadapter();
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
