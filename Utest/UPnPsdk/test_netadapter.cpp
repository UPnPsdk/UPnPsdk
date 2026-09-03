// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-09-03

// There are additional Unit Tests at
// git commit a18cff7d3dfd3266ad63a9efacba672ab1bd88b2.
// They were to complex to reflect simple changes in the source code.

#ifdef _MSC_VER
#include <UPnPsdk/src/net/netadapter_win32.cpp>
#include <UPnPsdk/sockaddr.hpp>
#endif

#include <UPnPsdk/netadapter.hpp>
#include <utest/utest.hpp>

namespace utest {

using testing::_;
using testing::Return;

using UPnPsdk::bitmask_to_netmask;
using UPnPsdk::CNetadapter;
using UPnPsdk::netmask_to_bitmask;
using UPnPsdk::SInaddr;
using UPnPsdk::SSockaddr;
using ADDRS = UPnPsdk::CNetadapter::ADDRS;


// General used object for temporary results.
SSockaddr saddrObj;

#if 0
// Not really a Unit Test, only to look what's going on for debugging. Just get
// or find all real network adapters from this host.
TEST(NetadapterTestSuite, get_netadapter_list) {
    CNetadapter nadapObj;
    nadapObj.get_first();
    // nadapObj.find_first();
    int prio{};
    do {
        nadapObj.sockaddr(saddrObj);
        prio = std::abs(prio);
        if (saddrObj.is_loopback())
            prio = -prio;
        else
            prio++;

        std::cout << "prio=" << std::setw(2) << std::right
                  << (prio <= 0 ? "--" : std::to_string(prio))
                  << ", idx=" << std::setw(2) << nadapObj.index() << ", name=\""
                  << std::setw(7) << std::left << (nadapObj.name()+"\",")
                  << " addr=\"" << saddrObj << "\".\n";
    } while (nadapObj.get_next());
    // } while (nadapObj.find_next());
}
#endif


TEST(NetadapterTestSuite, find_loopback_and_lla) {
    // There should always be a loopback and an lla interface.
    // The index of the loopback interface is usually 1, but you cannot rely on
    // this. A network interface may have different interface indexes for the
    // IPv4 and IPv6 loopback interface.
    // REF: [Loopback Interface Index]
    // (https://learn.microsoft.com/en-us/dotnet/api/system.net.networkinformation.networkinterface.loopbackinterfaceindex)
    // (https://study-ccna.com/loopback-interface-loopback-address/)
    SSockaddr saObj, lo_saObj;
    CNetadapter nadObj;
    ASSERT_NO_THROW(nadObj.get_first());

    // Must always have a loopback address.
    ASSERT_TRUE(nadObj.find_first(ADDRS::lo));
    auto index = nadObj.index();
    ASSERT_GT(index, 0);
    nadObj.sockaddr(saObj);
    lo_saObj.sin6.sin6_addr.s6_addr[15] = 1; // "[::1]";
    ASSERT_EQ(saObj, lo_saObj);
    EXPECT_NE(nadObj.name(), "");
    EXPECT_EQ(nadObj.bitmask(), 128);

    // Find loopback interface by name, using name from prvious finding
    ASSERT_TRUE(nadObj.find_first(nadObj.name()));
    nadObj.sockaddr(saObj);
    ASSERT_TRUE(IN6_IS_ADDR_LOOPBACK(&saObj.sin6.sin6_addr));

    // Find loopback interface by index, using index from prvious finding
    ASSERT_TRUE(nadObj.find_first(index));
    nadObj.sockaddr(saObj);
    ASSERT_TRUE(IN6_IS_ADDR_LOOPBACK(&saObj.sin6.sin6_addr));

    // Must always have a link-local address.
    ASSERT_TRUE(nadObj.find_first(ADDRS::lla));
    ASSERT_GT(nadObj.index(), 0);
    nadObj.sockaddr(saObj);
    ASSERT_TRUE(UPnPsdk::IN6_ADDR_LINKLOCAL(&saObj.sin6.sin6_addr));
    EXPECT_NE(nadObj.name(), "");
    EXPECT_EQ(nadObj.bitmask(), 64);

    // Default lookup must not have loopback and v4mapped addresses.
    ASSERT_TRUE(nadObj.find_first());
    do {
        nadObj.sockaddr(saObj);
        ASSERT_FALSE(nadObj.index() == 0 ||
                     IN6_IS_ADDR_LOOPBACK(&saObj.sin6.sin6_addr) ||
                     IN6_IS_ADDR_V4MAPPED(&saObj.sin6.sin6_addr));
    } while (nadObj.find_next());

    // Even on the loopback interface must not be an IPv4 mapped IPv6 address.
    // But there may be also other addresses, e.g. on MacOS "[fe80::1%lo0]:0".
    ASSERT_TRUE(nadObj.find_first(ADDRS::lo));
    nadObj.sockaddr(saObj);
    // Find this to stay only on this netadapter.
    ASSERT_TRUE(nadObj.find_first(nadObj.index()));
    do { // Look for a IPv4 mapped IPv6 address.
        nadObj.sockaddr(saObj);
        ASSERT_FALSE(IN6_IS_ADDR_V4MAPPED(&saObj.sin6.sin6_addr));
    } while (nadObj.find_next());
}

#if 0
// Get Subnet mask from address prefix length, first version with two nested
// loops working on 128 bits. I have made a more performant version working on
// bytes but won't discard this version.
TEST(NetadapterTestSuite, ipv6_netmask_test) {
    char buf[INET6_ADDRSTRLEN]{};
    in6_addr netmask6;

    for (int i{}; i < sizeof(netmask6.s6_addr) / sizeof(netmask6.s6_addr)[0];
         i++)
        netmask6.s6_addr[i] = static_cast<uint8_t>(~0);

    int zerobits{128 - 45};
    for (int i{15}; i >= 0; i--) {
        for (int k{7}; k >= 0; k--) {
            if (zerobits <= 0)
                goto end_loops;
            netmask6.s6_addr[i] = netmask6.s6_addr[i] << 1;
            zerobits--;
        }
    }
end_loops:
    inet_ntop(AF_INET6, &netmask6, buf, sizeof(buf));
}
#endif

enum struct Except { yes, no };

class ToBitmaskAndToNetmaskTest
    : public ::testing::TestWithParam<std::tuple<
          //  bitmask1,      netmask            bitmask2
          const uint8_t, const std::string, const uint8_t>> {};

TEST_P(ToBitmaskAndToNetmaskTest, set_family_and_bitmask) {
    // Get parameter
    const std::tuple params = GetParam();
    const uint8_t bitmask1 = std::get<0>(params);
    const std::string netmask = std::get<1>(params);
    const uint8_t bitmask2 = std::get<2>(params);

    // Test bitmask_to_netmask.
    EXPECT_EQ(bitmask_to_netmask(bitmask1), netmask);

    // Test netmask_to_bitmask.
    EXPECT_EQ(netmask_to_bitmask(netmask), bitmask2);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(ToBitmaskAndToNetmask, ToBitmaskAndToNetmaskTest, ::testing::Values(
    std::make_tuple(64, "[ffff:ffff:ffff:ffff::]", 64),
    std::make_tuple(-1, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]", 128),
    std::make_tuple(0, "[::]", 0), // Default setting for empty sockaddr.
    std::make_tuple(1, "[8000::]", 1),
    std::make_tuple(2, "[c000::]", 2),
    std::make_tuple(15, "[fffe::]", 15),
    std::make_tuple(16, "[ffff::]", 16),
    std::make_tuple(17, "[ffff:8000::]", 17),
    std::make_tuple(126, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffc]", 126),
    std::make_tuple(127, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffe]", 127),
    std::make_tuple(128, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]", 128),
    std::make_tuple(129, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]", 128),
    std::make_tuple(255, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]", 128),
    // Here we have uint8_t overrun.
    std::make_tuple(256, "[::]", 0),
    std::make_tuple(257, "[8000::]", 1),
    std::make_tuple(511, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]", 128),
    std::make_tuple(512, "[::]", 0),
    std::make_tuple(513, "[8000::]", 1)
));
// clang-format on

TEST(NetadapterTestSuite, netmask_to_bitmask_fails) {
    // Result of 255 indicates an error.
    // Test Unit
    EXPECT_EQ(netmask_to_bitmask("ffff:ffff:ffff:ffff:f0f0::"), 255);
    EXPECT_EQ(netmask_to_bitmask(""), 255);
    EXPECT_EQ(netmask_to_bitmask("[:]"), 255);
    EXPECT_EQ(netmask_to_bitmask("ffff::]"), 255);
    EXPECT_EQ(netmask_to_bitmask("[ffff::"), 255);
}

#if 0
// This test is usually used only one time to get the binary values for
// netaddresses "127.0.0.0" and "127.255.255.255" to simply check for the range
// of all possible IPv4 loopback addresses. They are htonl(2130706432) and
// htonl(2147483647).
TEST(NetadapterTestSuite, af_inet_loopback_range) {
    ::in_addr low;
    ::in_addr high;
    inet_pton(AF_INET, "127.0.0.0", &low);
    inet_pton(AF_INET, "127.255.255.255", &high);
    std::cout << "\"127.0.0.0\" is " << ntohl(low.s_addr)
              << ", \"127.255.255.255\" is " << ntohl(high.s_addr) << ".\n";
}
#endif

TEST(NetadapterTestSuite, find_first_adapters_info_without_get_first) {
    // Test Unit
    CNetadapter nadaptObj;
    EXPECT_FALSE(nadaptObj.find_first());
    EXPECT_EQ(nadaptObj.index(), 0);

    ASSERT_EQ(nadaptObj.name(), "");
    EXPECT_EQ(nadaptObj.bitmask(), 0);
    nadaptObj.sockaddr(saddrObj);
    EXPECT_EQ(saddrObj.family, AF_INET6);
    EXPECT_TRUE(saddrObj.empty());
}

TEST(NetadapterTestSuite, find_next_adapters_info_without_get_first) {
    CNetadapter nadaptObj;
    EXPECT_FALSE(nadaptObj.find_next());
    EXPECT_EQ(nadaptObj.index(), 0);

    ASSERT_EQ(nadaptObj.name(), "");
    EXPECT_EQ(nadaptObj.bitmask(), 0);
    nadaptObj.sockaddr(saddrObj);
    EXPECT_TRUE(saddrObj.empty());
}

TEST(NetadapterTestSuite, find_next_adapters_info_without_find_first) {
    CNetadapter nadaptObj;
    // This addresses the very first address entry.
    ASSERT_NO_THROW(nadaptObj.get_first());
    SSockaddr saObj;
    nadaptObj.sockaddr(saObj);
    EXPECT_NE(nadaptObj.index(), 0);
    // There is no next adapter to find, only to get.
    EXPECT_FALSE(nadaptObj.find_next());
    // Search for an address was not specified. find_next() exit without
    // success.
    EXPECT_GT(nadaptObj.index(), 0);
    ASSERT_NE(nadaptObj.name(), "");
    EXPECT_GT(nadaptObj.bitmask(), 0);
    nadaptObj.sockaddr(saddrObj);
    EXPECT_NE(saddrObj.netaddrp(), ":0");
}

class CNetadapterMock : public UPnPsdk::INetadapter {
  public:
    CNetadapterMock() = default;
    virtual ~CNetadapterMock() override = default;
    MOCK_METHOD(void, get_first, (), (override));
    MOCK_METHOD(bool, get_next, (), (override));
    MOCK_METHOD(unsigned int, index, (), (const, override));
    MOCK_METHOD(std::string, name, (), (const, override));
    MOCK_METHOD(void, sockaddr, (SSockaddr&), (const, override));
    MOCK_METHOD(unsigned int, bitmask, (), (const, override));
    MOCK_METHOD(void, reset, (), (noexcept, override));
};

TEST(NetadapterTestSuite, mock_netadapter_default) {
    // clang-format off
    // Emulated network interfaces:
    // 1: lo0: <LOOPBACK,UP,LOWER_UP>
        // "127.0.0.1"
        SSockaddr loIp4SaObj; loIp4SaObj.ss.ss_family = AF_INET;
        ::inet_pton(loIp4SaObj.ss.ss_family, "127.0.0.1", &loIp4SaObj.sin.sin_addr);
        // "[2001:db8::1]"
        SSockaddr loGuaSaObj; loGuaSaObj.ss.ss_family = AF_INET6;
        ::inet_pton(loGuaSaObj.ss.ss_family, "[2001:db8::1]", &loGuaSaObj.sin6.sin6_addr);
        // "[fe80::1%1]"
        SSockaddr loLlaSaObj; loLlaSaObj.ss.ss_family = AF_INET6;
        ::inet_pton(loLlaSaObj.ss.ss_family, "[fe80::1]", &loLlaSaObj.sin6.sin6_addr);
        loLlaSaObj.sin6.sin6_scope_id = 1;
        // "[::1]"
        SSockaddr loLopSaObj; loLopSaObj.ss.ss_family = AF_INET6;
        loLopSaObj.sin6.sin6_addr.s6_addr[15] = 1;
    // 2: ens1: <BROADCAST,MULTICAST,UP,LOWER_UP>
        // "[fe80::5054:ff:fe7f:c021%2]"
        SSockaddr ens1LlaSaObj; ens1LlaSaObj.ss.ss_family = AF_INET6;
        ::inet_pton(ens1LlaSaObj.ss.ss_family, "[fe80::5054:ff:fe7f:c021]", &ens1LlaSaObj.sin6.sin6_addr);
        ens1LlaSaObj.sin6.sin6_scope_id = 2;
        // "192.168.24.88"
        SSockaddr ens1Ip4SaObj; ens1Ip4SaObj.ss.ss_family = AF_INET;
        ::inet_pton(ens1Ip4SaObj.ss.ss_family, "192.168.24.88", &ens1Ip4SaObj.sin.sin_addr);
    // 3: ens2: <BROADCAST,MULTICAST,UP,LOWER_UP>
        // "[2001:db8::ff:fe7f:c021]"
        SSockaddr ens2GuaSaObj; ens2GuaSaObj.ss.ss_family = AF_INET6;
        ::inet_pton(ens2GuaSaObj.ss.ss_family, "[2001:db8::ff:fe7f:c021]", &ens2GuaSaObj.sin6.sin6_addr);
        // "[fe80::226:17ff:da9e%3]"
        SSockaddr ens2LlaSaObj; ens2LlaSaObj.ss.ss_family = AF_INET6;
        ::inet_pton(ens2LlaSaObj.ss.ss_family, "[fe80::226:17ff:da9e]", &ens2LlaSaObj.sin6.sin6_addr);
        ens2LlaSaObj.sin6.sin6_scope_id = 3;
    // clang-format on

    // Create mocking di-service object and get the smart pointer to it.
    auto nadap_mockPtr = std::make_shared<CNetadapterMock>();
    // Inject mocking functions
    CNetadapter nadapObj(nadap_mockPtr);

    // Mock get_first()
    // ----------------
    EXPECT_CALL(*nadap_mockPtr, get_first()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(loLopSaObj));
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(1)); // Loopback interface may be >1.
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);

    ASSERT_NO_THROW(nadapObj.get_first());

    // Mock find_first() default
    // -------------------------
    // Test Unit find_first() should find "[fe80::5054:ff:fe7f:c021%2]".
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(1))
        .WillOnce(Return(1))
        .WillOnce(Return(1))
        .WillOnce(Return(1))
        .WillOnce(Return(1))
        .WillOnce(Return(2));
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(loIp4SaObj))
        .WillOnce(SaddrCpyToArg<0>(loGuaSaObj))
        .WillOnce(SaddrCpyToArg<0>(loLlaSaObj))
        .WillOnce(SaddrCpyToArg<0>(loLopSaObj))
        .WillOnce(SaddrCpyToArg<0>(ens1LlaSaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .Times(4)
        .WillRepeatedly(Return(true));

    ASSERT_TRUE(nadapObj.find_first());
    // std::cout << "------ found ------\n";

    // Mock find_next() default
    // ------------------------
    // Test Unit find_next() should find "[2001:db8::ff:fe7f:c021]".
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*nadap_mockPtr, index()).WillOnce(Return(3));
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens1Ip4SaObj))
        .WillOnce(SaddrCpyToArg<0>(ens2GuaSaObj));

    ASSERT_TRUE(nadapObj.find_next());
    // std::cout << "------ found ------\n";

    // Mock find_next() default
    // ------------------------
    // Test Unit find_next() should find "[fe80::226:17ff:da9e%3]".
    EXPECT_CALL(*nadap_mockPtr, get_next()).WillOnce(Return(true));
    EXPECT_CALL(*nadap_mockPtr, index()).WillOnce(Return(3));
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens2LlaSaObj));

    ASSERT_TRUE(nadapObj.find_next());
    // std::cout << "------ found ------\n";

    EXPECT_CALL(*nadap_mockPtr, get_next()).WillOnce(Return(false));
    ASSERT_FALSE(nadapObj.find_next());
    // std::cout << "------ finish ------\n";
}

TEST(NetadapterTestSuite, mock_netadapter_with_adapter_name) {
    // clang-format off
    // Emulated network interfaces:
    // 1: lo0: <LOOPBACK,UP,LOWER_UP>
    SSockaddr loIp4SaObj; loIp4SaObj = SInaddr("127.0.0.1");
    SSockaddr loGuaSaObj; loGuaSaObj = SInaddr("[2001:db8::1]");
    SSockaddr loLlaSaObj; loLlaSaObj = SInaddr("[fe80::1%1]");
    SSockaddr loLopSaObj; loLopSaObj = SInaddr("[::1]");
    // 2: ens1: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens1GuaSaObj; ens1GuaSaObj = SInaddr("[2001:db8::fe:fe7f:c021]");
    SSockaddr ens1Ip4SaObj; ens1Ip4SaObj = SInaddr("192.168.24.88");
    SSockaddr ens1LlaSaObj; ens1LlaSaObj = SInaddr("[fe80::5054:fe7f:c021%2]");
    // 3: ens2: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens2LlaSaObj; ens2LlaSaObj = SInaddr("[fe80::226:17ff:da9e%3]");
    // clang-format on

    // Create mocking di-service object and get the smart pointer to it.
    auto nadap_mockPtr = std::make_shared<CNetadapterMock>();
    // Inject mocking functions
    CNetadapter nadapObj(nadap_mockPtr);

    // Mock get_first()
    // ----------------
    EXPECT_CALL(*nadap_mockPtr, get_first()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(loLopSaObj));
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(1)); // Loopback interface may be >1.
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);

    ASSERT_NO_THROW(nadapObj.get_first());

    // Mock find_first() with adapter name
    // -----------------------------------
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, name())
        .WillOnce(Return("lo0"))
        .WillOnce(Return("lo0"))
        .WillOnce(Return("lo0"))
        .WillOnce(Return("lo0"))
        .WillOnce(Return("ens1"));
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens1GuaSaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .Times(4)
        .WillRepeatedly(Return(true));
    // Index of Netadapter containing first found IP address, that is
    // ens1GuaSaObj.
    EXPECT_CALL(*nadap_mockPtr, index()).WillOnce(Return(2));

    ASSERT_TRUE(nadapObj.find_first("ens1"));

    // Mock find_next() with adapter name
    // ----------------------------------
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*nadap_mockPtr, index()).Times(2).WillRepeatedly(Return(2));
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens1Ip4SaObj))
        .WillOnce(SaddrCpyToArg<0>(ens1LlaSaObj));

    ASSERT_TRUE(nadapObj.find_next());

    // Mock find_next() with adapter name
    // ----------------------------------
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .Times(1)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*nadap_mockPtr, index()).Times(1).WillRepeatedly(Return(2));
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens2LlaSaObj));

    ASSERT_TRUE(nadapObj.find_next());

    EXPECT_CALL(*nadap_mockPtr, get_next()).WillOnce(Return(false));
    ASSERT_FALSE(nadapObj.find_next());
}

TEST(NetadapterTestSuite, mock_netadapter_with_adapter_name_lo0) {
    // clang-format off
    // Emulated network interfaces:
    // 1: ens1: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens1LlaSaObj; ens1LlaSaObj = SInaddr("[fe80::5054:fe7f:c021%1]");
    // 2: lo0: <LOOPBACK,UP,LOWER_UP>
    SSockaddr loIp4SaObj; loIp4SaObj = SInaddr("127.0.0.1");
    SSockaddr loLlaSaObj; loLlaSaObj = SInaddr("[fe80::1%2]");
    SSockaddr loLopSaObj; loLopSaObj = SInaddr("[::1]");
    // 3: ens2: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens2LlaSaObj; ens2LlaSaObj = SInaddr("[fe80::226:17ff:da9e%3]");
    // clang-format on

    // Create mocking di-service object and get the smart pointer to it.
    auto nadap_mockPtr = std::make_shared<CNetadapterMock>();
    // Inject mocking functions
    CNetadapter nadapObj(nadap_mockPtr);

    // Mock get_first()
    // ----------------
    EXPECT_CALL(*nadap_mockPtr, get_first()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(loLopSaObj));
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(2)); // Loopback interface may be >1.
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);

    ASSERT_NO_THROW(nadapObj.get_first());

    // Mock find_first() with adapter name
    // -----------------------------------
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, name())
        .WillOnce(Return("ens1"))
        .WillOnce(Return("lo0"))
        .WillOnce(Return("lo0"));
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(loIp4SaObj))
        .WillOnce(SaddrCpyToArg<0>(loLlaSaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .Times(2)
        .WillRepeatedly(Return(true));
    // Index of Netadapter containing first found IP address, that is
    // loLlaSaObj.
    EXPECT_CALL(*nadap_mockPtr, index()).WillOnce(Return(2));

    ASSERT_TRUE(nadapObj.find_first("lo0"));

    // Mock find_next() with adapter name
    // ----------------------------------
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .Times(1)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*nadap_mockPtr, index()).Times(1).WillRepeatedly(Return(2));
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(loLopSaObj));

    ASSERT_TRUE(nadapObj.find_next());

    EXPECT_CALL(*nadap_mockPtr, get_next()).WillOnce(Return(false));
    ASSERT_FALSE(nadapObj.find_next());
}

TEST(NetadapterTestSuite, mock_netadapter_with_ip_address) {
    // clang-format off
    // Emulated network interfaces:
    // 1: ens1: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens1LlaSaObj; ens1LlaSaObj = SInaddr("[fe80::5054:fe7f:c021%1]");
    // 2: lo0: <LOOPBACK,UP,LOWER_UP>
    SSockaddr loIp4SaObj; loIp4SaObj = SInaddr("127.0.0.1");
    SSockaddr loLlaSaObj; loLlaSaObj = SInaddr("[fe80::1%2]");
    SSockaddr loLopSaObj; loLopSaObj = SInaddr("[::1]");
    // 3: ens2: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens2LlaSaObj; ens2LlaSaObj = SInaddr("[fe80::226:17ff:da9e%3]");
    // clang-format on

    // Create mocking di-service object and get the smart pointer to it.
    auto nadap_mockPtr = std::make_shared<CNetadapterMock>();
    // Inject mocking functions
    CNetadapter nadapObj(nadap_mockPtr);

    // Mock get_first()
    // ----------------
    EXPECT_CALL(*nadap_mockPtr, get_first()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(loLopSaObj));
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(2)); // Loopback interface may be >1.
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);

    ASSERT_NO_THROW(nadapObj.get_first());

    // Mock find_first() with ip address
    // ---------------------------------
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens1LlaSaObj))
        .WillOnce(SaddrCpyToArg<0>(loIp4SaObj))
        .WillOnce(SaddrCpyToArg<0>(loLlaSaObj))
        .WillOnce(SaddrCpyToArg<0>(loLopSaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .Times(3)
        .WillRepeatedly(Return(true));

    ASSERT_TRUE(nadapObj.find_first("[::1]"));

    ASSERT_FALSE(nadapObj.find_next());
}

TEST(NetadapterTestSuite, mock_netadapter_with_adapter_index) {
    // clang-format off
    // Emulated network interfaces:
    // 1: ens1: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens1LlaSaObj; ens1LlaSaObj = SInaddr("[fe80::226:17ff:da9e%1]");
    // 2: lo0: <LOOPBACK,UP,LOWER_UP>
    SSockaddr loIp4SaObj; loIp4SaObj = SInaddr("127.0.0.1");
    SSockaddr loGuaSaObj; loGuaSaObj = SInaddr("[2001:db8::1]");
    SSockaddr loLlaSaObj; loLlaSaObj = SInaddr("[fe80::1%2]");
    SSockaddr loLopSaObj; loLopSaObj = SInaddr("[::1]");
    // 3: ens2: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens2LlaSaObj; ens2LlaSaObj = SInaddr("[fe80::5054:fe7f:c021%3]");
    SSockaddr ens2Ip4SaObj; ens2Ip4SaObj = SInaddr("192.168.24.88");
    SSockaddr ens2GuaSaObj; ens2GuaSaObj = SInaddr("[2001:db8::fe:fe7f:c021]");
    // clang-format on

    // Create mocking di-service object and get the smart pointer to it.
    auto nadap_mockPtr = std::make_shared<CNetadapterMock>();
    // Inject mocking functions
    CNetadapter nadapObj(nadap_mockPtr);

    // Mock get_first()
    // ----------------
    EXPECT_CALL(*nadap_mockPtr, get_first()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(loLopSaObj));
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(2)); // Loopback interface may be >1.
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);

    ASSERT_NO_THROW(nadapObj.get_first());

    // Mock find_first() with netadapter index
    // ---------------------------------------
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(1))
        .WillOnce(Return(1))
        .WillOnce(Return(2))
        .WillOnce(Return(2))
        .WillOnce(Return(2))
        .WillOnce(Return(2))
        .WillOnce(Return(3));
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens2LlaSaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .Times(5)
        .WillRepeatedly(Return(true));

    ASSERT_TRUE(nadapObj.find_first(3));

    // Mock find_next() with netadapter index
    // --------------------------------------
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(3))
        .WillOnce(Return(3));
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens2Ip4SaObj))
        .WillOnce(SaddrCpyToArg<0>(ens2GuaSaObj));

    ASSERT_TRUE(nadapObj.find_next());

    EXPECT_CALL(*nadap_mockPtr, get_next()).WillOnce(Return(false));
    ASSERT_FALSE(nadapObj.find_next());
}

TEST(NetadapterTestSuite, mock_netadapter_with_address_groups_first) {
    // Skip loopback interface on first calls.

    // clang-format off
    // Emulated network interfaces:
    // 1: ens1: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens1Ip4SaObj; ens1Ip4SaObj = SInaddr("192.168.24.88");
    // 2: lo0: <LOOPBACK,UP,LOWER_UP>
    SSockaddr loGuaSaObj; loGuaSaObj = SInaddr("[2001:db8::1]");
    SSockaddr loIp4SaObj; loIp4SaObj = SInaddr("127.0.0.1");
    SSockaddr loLlaSaObj; loLlaSaObj = SInaddr("[fe80::1%2]");
    SSockaddr loLopSaObj; loLopSaObj = SInaddr("[::1]");
    // 3: ens2: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens2LlaSaObj; ens2LlaSaObj = SInaddr("[fe80::5054:fe7f:c021%3]");
    SSockaddr ens2Ip4SaObj; ens2Ip4SaObj = SInaddr("192.168.24.89");
    SSockaddr ens2GuaSaObj; ens2GuaSaObj = SInaddr("[2001:db8::fe:fe7f:c021]");
    // 4: ens3: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens3Ip4SaObj; ens3Ip4SaObj = SInaddr("192.168.24.90");
    SSockaddr ens3GuaSaObj; ens3GuaSaObj = SInaddr("[2001:db8::fe:fe7f:c022]");
    SSockaddr ens3LlaSaObj; ens3LlaSaObj = SInaddr("[fe80::5054:fe7f:c021%4]");
    // clang-format on

    // Create mocking di-service object and get the smart pointer to it.
    auto nadap_mockPtr = std::make_shared<CNetadapterMock>();
    // Inject mocking functions
    CNetadapter nadapObj(nadap_mockPtr);

    // Mock get_first()
    // ----------------
    EXPECT_CALL(*nadap_mockPtr, get_first()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(loLopSaObj));
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(2)); // Loopback interface may be >1.
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);

    ASSERT_NO_THROW(nadapObj.get_first());

    // Mock find_first() with netadapter ADDRS::lla
    // --------------------------------------------
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(1))
        .WillOnce(Return(2))
        .WillOnce(Return(3));
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens1Ip4SaObj))
        .WillOnce(SaddrCpyToArg<0>(loGuaSaObj))
        .WillOnce(SaddrCpyToArg<0>(loIp4SaObj))
        .WillOnce(SaddrCpyToArg<0>(loLlaSaObj))
        .WillOnce(SaddrCpyToArg<0>(loLopSaObj))
        .WillOnce(SaddrCpyToArg<0>(ens2LlaSaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .Times(5)
        .WillRepeatedly(Return(true));

    ASSERT_TRUE(nadapObj.find_first(ADDRS::lla));
    // std::cout << "------ found ------\n";

    // Mock find_next() with netadapter ADDRS::lla
    // -------------------------------------------
    EXPECT_CALL(*nadap_mockPtr, index()).WillOnce(Return(4));
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens2Ip4SaObj))
        .WillOnce(SaddrCpyToArg<0>(ens2GuaSaObj))
        .WillOnce(SaddrCpyToArg<0>(ens3Ip4SaObj))
        .WillOnce(SaddrCpyToArg<0>(ens3GuaSaObj))
        .WillOnce(SaddrCpyToArg<0>(ens3LlaSaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .Times(5)
        .WillRepeatedly(Return(true));

    ASSERT_TRUE(nadapObj.find_next());
    // std::cout << "------ found ------\n";

    EXPECT_CALL(*nadap_mockPtr, get_next()).WillOnce(Return(false));
    ASSERT_FALSE(nadapObj.find_next());

    // Mock find_first() with netadapter ADDRS::gua
    // --------------------------------------------
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(1))
        .WillOnce(Return(2))
        .WillOnce(Return(3));
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens1Ip4SaObj))
        .WillOnce(SaddrCpyToArg<0>(loGuaSaObj))
        .WillOnce(SaddrCpyToArg<0>(loIp4SaObj))
        .WillOnce(SaddrCpyToArg<0>(loLlaSaObj))
        .WillOnce(SaddrCpyToArg<0>(loLopSaObj))
        .WillOnce(SaddrCpyToArg<0>(ens2LlaSaObj))
        .WillOnce(SaddrCpyToArg<0>(ens2Ip4SaObj))
        .WillOnce(SaddrCpyToArg<0>(ens2GuaSaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .Times(7)
        .WillRepeatedly(Return(true));

    ASSERT_TRUE(nadapObj.find_first(ADDRS::gua));
    // std::cout << "------ found ------\n";

    // Mock find_next() with netadapter ADDRS::gua
    // -------------------------------------------
    EXPECT_CALL(*nadap_mockPtr, index()).WillOnce(Return(4));
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens3Ip4SaObj))
        .WillOnce(SaddrCpyToArg<0>(ens3GuaSaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .Times(2)
        .WillRepeatedly(Return(true));

    ASSERT_TRUE(nadapObj.find_next());
    // std::cout << "------ found ------\n";

    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens3LlaSaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    ASSERT_FALSE(nadapObj.find_next());

    // Mock find_first() with netadapter ADDRS::map4
    // ---------------------------------------------
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(1))
        .WillOnce(Return(1));
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens1Ip4SaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next()).Times(0);

    ASSERT_TRUE(nadapObj.find_first(ADDRS::map4));
    // std::cout << "------ found ------\n";

    // Mock find_next() with netadapter ADDRS::map4
    // --------------------------------------------
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(2))
        .WillOnce(Return(3));
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(loGuaSaObj))
        .WillOnce(SaddrCpyToArg<0>(loIp4SaObj))
        .WillOnce(SaddrCpyToArg<0>(loLlaSaObj))
        .WillOnce(SaddrCpyToArg<0>(loLopSaObj))
        .WillOnce(SaddrCpyToArg<0>(ens2LlaSaObj))
        .WillOnce(SaddrCpyToArg<0>(ens2Ip4SaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .Times(6)
        .WillRepeatedly(Return(true));

    ASSERT_TRUE(nadapObj.find_next());
    // std::cout << "------ found ------\n";

    // Mock find_next() with netadapter ADDRS::map4
    // --------------------------------------------
    EXPECT_CALL(*nadap_mockPtr, index()).WillOnce(Return(4));
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens2GuaSaObj))
        .WillOnce(SaddrCpyToArg<0>(ens3Ip4SaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .Times(2)
        .WillRepeatedly(Return(true));

    ASSERT_TRUE(nadapObj.find_next());
    // std::cout << "------ found ------\n";

    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens3GuaSaObj))
        .WillOnce(SaddrCpyToArg<0>(ens3LlaSaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(false));

    ASSERT_FALSE(nadapObj.find_next());
}

TEST(NetadapterTestSuite, mock_netadapter_with_address_groups_next) {
    // Skip loopback interface on next calls.

    // clang-format off
    // Emulated network interfaces:
    // 1: ens1: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens1LlaSaObj; ens1LlaSaObj = SInaddr("[fe80::5054:fe7f:c021%1]");
    SSockaddr ens1Ip4SaObj; ens1Ip4SaObj = SInaddr("192.168.24.89");
    SSockaddr ens1GuaSaObj; ens1GuaSaObj = SInaddr("[2001:db8::fe:fe7f:c021]");
    // 2: lo0: <LOOPBACK,UP,LOWER_UP>
    SSockaddr loGuaSaObj; loGuaSaObj = SInaddr("[2001:db8::1]");
    SSockaddr loIp4SaObj; loIp4SaObj = SInaddr("127.0.0.1");
    SSockaddr loLlaSaObj; loLlaSaObj = SInaddr("[fe80::1%2]");
    SSockaddr loLopSaObj; loLopSaObj = SInaddr("[::1]");
    // 3: ens2: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens2Ip4SaObj; ens2Ip4SaObj = SInaddr("192.168.24.90");
    SSockaddr ens2GuaSaObj; ens2GuaSaObj = SInaddr("[2001:db8::fe:fe7f:c022]");
    SSockaddr ens2LlaSaObj; ens2LlaSaObj = SInaddr("[fe80::5054:fe7f:c021%3]");
    // clang-format on

    // Create mocking di-service object and get the smart pointer to it.
    auto nadap_mockPtr = std::make_shared<CNetadapterMock>();
    // Inject mocking functions
    CNetadapter nadapObj(nadap_mockPtr);

    // Mock get_first()
    // ----------------
    EXPECT_CALL(*nadap_mockPtr, get_first()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(loLopSaObj));
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(2)); // Loopback interface may be >1.
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);

    ASSERT_NO_THROW(nadapObj.get_first());

    // Mock find_first() with netadapter ADDRS::lla
    // --------------------------------------------
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(1))
        .WillOnce(Return(1)); // On matched addres "[fe80::5054:fe7f:c021%1]"
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens1LlaSaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next()).Times(0);

    ASSERT_TRUE(nadapObj.find_first(ADDRS::lla));
    // std::cout << "------ found ------\n";

    // Mock find_next() with netadapter ADDRS::lla
    // -------------------------------------------
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(2))  // On matched address "[fe80::1%2]"
        .WillOnce(Return(3)); // On matched address "[fe80::5054:fe7f:c021%3]"
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens1Ip4SaObj))
        .WillOnce(SaddrCpyToArg<0>(ens1GuaSaObj))
        .WillOnce(SaddrCpyToArg<0>(loGuaSaObj))
        .WillOnce(SaddrCpyToArg<0>(loIp4SaObj))
        .WillOnce(SaddrCpyToArg<0>(loLlaSaObj))
        .WillOnce(SaddrCpyToArg<0>(loLopSaObj))
        .WillOnce(SaddrCpyToArg<0>(ens2Ip4SaObj))
        .WillOnce(SaddrCpyToArg<0>(ens2GuaSaObj))
        .WillOnce(SaddrCpyToArg<0>(ens2LlaSaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .Times(9)
        .WillRepeatedly(Return(true));

    ASSERT_TRUE(nadapObj.find_next());
    // std::cout << "------ found ------\n";

    // Finish last checks.
    EXPECT_CALL(*nadap_mockPtr, get_next()).WillOnce(Return(false));
    ASSERT_FALSE(nadapObj.find_next());
    // std::cout << "------ finish ------\n";

    // Mock find_first() with netadapter ADDRS::gua
    // --------------------------------------------
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(1))
        .WillOnce(Return(1)); // On matched address "[2001:db8::fe:fe7f:c021]"
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens1LlaSaObj))
        .WillOnce(SaddrCpyToArg<0>(ens1Ip4SaObj))
        .WillOnce(SaddrCpyToArg<0>(ens1GuaSaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .Times(2)
        .WillRepeatedly(Return(true));

    ASSERT_TRUE(nadapObj.find_first(ADDRS::gua));
    // std::cout << "------ found ------\n";

    // Mock find_next() with netadapter ADDRS::gua
    // -------------------------------------------
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(2))  // On ingnored address "[2001:db8::1]"
        .WillOnce(Return(3)); // On matched address "[2001:db8::fe:fe7f:c022]"
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(loGuaSaObj))
        .WillOnce(SaddrCpyToArg<0>(loIp4SaObj))
        .WillOnce(SaddrCpyToArg<0>(loLlaSaObj))
        .WillOnce(SaddrCpyToArg<0>(loLopSaObj))
        .WillOnce(SaddrCpyToArg<0>(ens2Ip4SaObj))
        .WillOnce(SaddrCpyToArg<0>(ens2GuaSaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .Times(6)
        .WillRepeatedly(Return(true));

    ASSERT_TRUE(nadapObj.find_next());
    // std::cout << "------ found ------\n";

    // Finish last checks.
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens2LlaSaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    ASSERT_FALSE(nadapObj.find_next());
    // std::cout << "------ finish ------\n";

    // Mock find_first() with netadapter ADDRS::map4
    // ---------------------------------------------
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(1))
        .WillOnce(Return(1)); // On matched address "192.168.24.89"
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens1LlaSaObj))
        .WillOnce(SaddrCpyToArg<0>(ens1Ip4SaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next()).WillOnce(Return(true));

    ASSERT_TRUE(nadapObj.find_first(ADDRS::map4));
    // std::cout << "------ found ------\n";

    // Mock find_next() with netadapter ADDRS::map4
    // --------------------------------------------
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(2))  // On ignored address "127.0.0.1"
        .WillOnce(Return(3)); // On matched address "192.168.24.90"
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens1GuaSaObj))
        .WillOnce(SaddrCpyToArg<0>(loGuaSaObj))
        .WillOnce(SaddrCpyToArg<0>(loIp4SaObj))
        .WillOnce(SaddrCpyToArg<0>(loLlaSaObj))
        .WillOnce(SaddrCpyToArg<0>(loLopSaObj))
        .WillOnce(SaddrCpyToArg<0>(ens2Ip4SaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .Times(6)
        .WillRepeatedly(Return(true));

    ASSERT_TRUE(nadapObj.find_next());
    // std::cout << "------ found ------\n";

    // Finish last checks.
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(ens2GuaSaObj))
        .WillOnce(SaddrCpyToArg<0>(ens2LlaSaObj));
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    ASSERT_FALSE(nadapObj.find_next());
    // std::cout << "------ finish ------\n";
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
