// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-04-29

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

using ::testing::_;
using ::testing::Return;

using ::UPnPsdk::CNetadapter;
using ::UPnPsdk::netmask_to_bitmask;
using ::UPnPsdk::SSockaddr;
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
    lo_saObj = "[::1]";
    ASSERT_EQ(saObj, lo_saObj);
    EXPECT_NE(nadObj.name(), "");
    nadObj.socknetmask(saObj);
    EXPECT_EQ(saObj.netaddr(), "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]");
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
    ASSERT_TRUE(IN6_IS_ADDR_LINKLOCAL(&saObj.sin6.sin6_addr));
    EXPECT_NE(nadObj.name(), "");
    nadObj.socknetmask(saObj);
    EXPECT_EQ(saObj.netaddr(), "[ffff:ffff:ffff:ffff::]");
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
          //    family,      bitmask,       netmask,     exception
          const sa_family_t, const uint8_t, std::string, const Except>> {};

TEST_P(ToBitmaskAndToNetmaskTest, set_family_and_bitmask) {
    // Get parameter
    const std::tuple params = GetParam();
    const sa_family_t family = std::get<0>(params);
    const uint8_t bitmask = std::get<1>(params);
    const std::string netmask = std::get<2>(params);
    const Except except = std::get<3>(params);

    // Test bitmask_to_netmask.
    ::sockaddr_storage ss{}; // socket address the netmask is associated.
    ss.ss_family = family;
    if (except == Except::yes) {
        EXPECT_THROW(bitmask_to_netmask(/*in*/ &ss, bitmask,
                                        /*out netmask*/ saddrObj),
                     std::runtime_error);
        return;
    }
    bitmask_to_netmask(/*in*/ &ss, bitmask, /*out netmask*/ saddrObj);
    EXPECT_EQ(saddrObj.netaddr(), netmask);
    if (family == AF_INET6 || family == AF_INET || family == AF_UNSPEC)
        EXPECT_EQ(saddrObj.ss.ss_family, family);
    else
        EXPECT_EQ(saddrObj.ss.ss_family, AF_UNSPEC);

    // Test netmask_to_bitmask.
    ::sockaddr_storage ss_netmask{};
    ss_netmask.ss_family = family;
    switch (family) {
    case AF_INET6: {
        std::string netmsk = netmask.substr(1, netmask.size() - 2);
        inet_pton(AF_INET6, netmsk.c_str(),
                  &reinterpret_cast<::sockaddr_in6*>(&ss_netmask)->sin6_addr);
    } break;
    case AF_INET: {
        inet_pton(AF_INET, netmask.c_str(),
                  &reinterpret_cast<::sockaddr_in*>(&ss_netmask)->sin_addr);
    } break;
    } // switch

    if (except == Except::yes)
        EXPECT_THROW(UPnPsdk::netmask_to_bitmask(/*in*/ &ss_netmask),
                     std::runtime_error);
    else
        EXPECT_EQ(UPnPsdk::netmask_to_bitmask(/*in*/ &ss_netmask), bitmask);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(ToBitmaskAndToNetmask, ToBitmaskAndToNetmaskTest, ::testing::Values(
    std::make_tuple(AF_INET6, 64, "[ffff:ffff:ffff:ffff::]", Except::no),
    std::make_tuple(AF_INET6, -1, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]", Except::yes),
    std::make_tuple(AF_INET6, 0, "[::]", Except::no),
    std::make_tuple(AF_INET6, 1, "[8000::]", Except::no),
    std::make_tuple(AF_INET6, 2, "[c000::]", Except::no),
    std::make_tuple(AF_INET6, 15, "[fffe::]", Except::no),
    std::make_tuple(AF_INET6, 16, "[ffff::]", Except::no),
    std::make_tuple(AF_INET6, 17, "[ffff:8000::]", Except::no),
    std::make_tuple(AF_INET6, 126, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffc]", Except::no),
    std::make_tuple(AF_INET6, 127, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffe]", Except::no),
    std::make_tuple(AF_INET6, 128, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]", Except::no),
    std::make_tuple(AF_INET6, 129, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]", Except::yes),
    std::make_tuple(AF_INET6, 255, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]", Except::yes),
    // Here we have uint8_t overrun.
    std::make_tuple(AF_INET6, 256, "[::]", Except::no),
    std::make_tuple(AF_INET6, 257, "[8000::]", Except::no),
    std::make_tuple(AF_INET6, 511, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]", Except::yes),
    std::make_tuple(AF_INET6, 512, "[::]", Except::no),
    std::make_tuple(AF_INET6, 513, "[8000::]", Except::no),

    std::make_tuple(AF_INET, 24, "255.255.255.0", Except::no),
    std::make_tuple(AF_INET, -1, "255.255.255.255", Except::yes),
    std::make_tuple(AF_INET, 0, "0.0.0.0", Except::no),
    std::make_tuple(AF_INET, 1, "128.0.0.0", Except::no),
    std::make_tuple(AF_INET, 2, "192.0.0.0", Except::no),
    std::make_tuple(AF_INET, 30, "255.255.255.252", Except::no),
    std::make_tuple(AF_INET, 31, "255.255.255.254", Except::no),
    std::make_tuple(AF_INET, 32, "255.255.255.255", Except::no),
    std::make_tuple(AF_INET, 33, "255.255.255.255", Except::yes),
    std::make_tuple(AF_INET, 64, "255.255.255.255", Except::yes),
    std::make_tuple(AF_INET, 65, "255.255.255.255", Except::yes),
    std::make_tuple(AF_INET, 255, "255.255.255.255", Except::yes),
    std::make_tuple(AF_UNSPEC, 0, "", Except::no), // continues with AF_UNSPEC
    // Here we have uint8_t overrun.
    std::make_tuple(AF_INET, 256, "0.0.0.0", Except::no),
    std::make_tuple(AF_INET, 257, "128.0.0.0", Except::no)
));

TEST(NetadapterTestSuite, netmask_to_bitmask_fails) {
    UPnPsdk::sockaddr_t saddr{};
    saddr.ss.ss_family = AF_INET6;

    // Test Unit
    inet_pton(AF_INET6, "ffff:ffff:ffff:ffff:f0f0::", &saddr.sin6.sin6_addr);
    EXPECT_THROW(netmask_to_bitmask(&saddr.ss), std::runtime_error);

    saddr.ss.ss_family = static_cast<sa_family_t>(231);
    inet_pton(AF_INET6, "ffff:ffff:ffff:ffff::", &saddr.sin6.sin6_addr);
    EXPECT_THROW(netmask_to_bitmask(&saddr.ss), std::runtime_error);
}
// clang-format on

TEST(NetadapterTestSuite, bitmask_to_netmask_fails) {
    UPnPsdk::sockaddr_t saddr{};
    saddrObj = "";

    // Test Unit
    saddr.ss.ss_family = AF_UNSPEC;
    bitmask_to_netmask(&saddr.ss, 64, saddrObj);
    EXPECT_EQ(saddrObj.ss.ss_family, AF_UNSPEC);
    EXPECT_EQ(saddrObj.netaddrp(), ":0");

    saddr.ss.ss_family = static_cast<sa_family_t>(231);
    EXPECT_THROW(bitmask_to_netmask(&saddr.ss, 64, saddrObj),
                 std::runtime_error);

    EXPECT_THROW(bitmask_to_netmask(/*in*/ nullptr, /*in*/ 64,
                                    /*out netmask*/ saddrObj),
                 std::runtime_error);
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
    EXPECT_EQ(saddrObj.netaddrp(), ":0");
    nadaptObj.socknetmask(saddrObj);
    EXPECT_EQ(saddrObj.netaddrp(), ":0");
}

TEST(NetadapterTestSuite, find_next_adapters_info_without_get_first) {
    CNetadapter nadaptObj;
    EXPECT_FALSE(nadaptObj.find_next());
    EXPECT_EQ(nadaptObj.index(), 0);

    ASSERT_EQ(nadaptObj.name(), "");
    EXPECT_EQ(nadaptObj.bitmask(), 0);
    nadaptObj.sockaddr(saddrObj);
    EXPECT_EQ(saddrObj.netaddrp(), ":0");
    nadaptObj.socknetmask(saddrObj);
    EXPECT_EQ(saddrObj.netaddrp(), ":0");
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
    nadaptObj.socknetmask(saddrObj);
    EXPECT_NE(saddrObj.netaddr(), "");
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
    MOCK_METHOD(void, socknetmask, (SSockaddr&), (const, override));
    MOCK_METHOD(unsigned int, bitmask, (), (const, override));
    MOCK_METHOD(void, reset, (), (noexcept, override));
};

TEST(NetadapterTestSuite, mock_netadapter_default) {
    // clang-format off
    // Emulated network interfaces:
    // 1: lo0: <LOOPBACK,UP,LOWER_UP>
    SSockaddr loIp4SaObj; loIp4SaObj = "127.0.0.1";
    SSockaddr loGuaSaObj; loGuaSaObj = "[2001:db8::1]";
    SSockaddr loLlaSaObj; loLlaSaObj =  "[fe80::1%1]";
    SSockaddr loLopSaObj; loLopSaObj =  "[::1]";
    // 2: ens1: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens1LlaSaObj; ens1LlaSaObj = "[fe80::5054:ff:fe7f:c021%2]";
    SSockaddr ens1Ip4SaObj; ens1Ip4SaObj = "192.168.24.88";
    // 3: ens2: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens2GuaSaObj; ens2GuaSaObj = "[2001:db8::ff:fe7f:c021]";
    SSockaddr ens2LlaSaObj; ens2LlaSaObj = "[fe80::226:17ff:da9e%3]";
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
    SSockaddr loIp4SaObj; loIp4SaObj = "127.0.0.1";
    SSockaddr loGuaSaObj; loGuaSaObj = "[2001:db8::1]";
    SSockaddr loLlaSaObj; loLlaSaObj =  "[fe80::1%1]";
    SSockaddr loLopSaObj; loLopSaObj =  "[::1]";
    // 2: ens1: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens1GuaSaObj; ens1GuaSaObj = "[2001:db8::fe:fe7f:c021]";
    SSockaddr ens1Ip4SaObj; ens1Ip4SaObj = "192.168.24.88";
    SSockaddr ens1LlaSaObj; ens1LlaSaObj = "[fe80::5054:fe7f:c021%2]";
    // 3: ens2: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens2LlaSaObj; ens2LlaSaObj = "[fe80::226:17ff:da9e%3]";
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
    SSockaddr ens1LlaSaObj; ens1LlaSaObj = "[fe80::5054:fe7f:c021%1]";
    // 2: lo0: <LOOPBACK,UP,LOWER_UP>
    SSockaddr loIp4SaObj; loIp4SaObj = "127.0.0.1";
    SSockaddr loLlaSaObj; loLlaSaObj =  "[fe80::1%2]";
    SSockaddr loLopSaObj; loLopSaObj =  "[::1]";
    // 3: ens2: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens2LlaSaObj; ens2LlaSaObj = "[fe80::226:17ff:da9e%3]";
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
    SSockaddr ens1LlaSaObj; ens1LlaSaObj = "[fe80::5054:fe7f:c021%1]";
    // 2: lo0: <LOOPBACK,UP,LOWER_UP>
    SSockaddr loIp4SaObj; loIp4SaObj = "127.0.0.1";
    SSockaddr loLlaSaObj; loLlaSaObj =  "[fe80::1%2]";
    SSockaddr loLopSaObj; loLopSaObj =  "[::1]";
    // 3: ens2: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens2LlaSaObj; ens2LlaSaObj = "[fe80::226:17ff:da9e%3]";
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
    SSockaddr ens1LlaSaObj; ens1LlaSaObj = "[fe80::226:17ff:da9e%1]";
    // 2: lo0: <LOOPBACK,UP,LOWER_UP>
    SSockaddr loIp4SaObj; loIp4SaObj = "127.0.0.1";
    SSockaddr loGuaSaObj; loGuaSaObj = "[2001:db8::1]";
    SSockaddr loLlaSaObj; loLlaSaObj =  "[fe80::1%2]";
    SSockaddr loLopSaObj; loLopSaObj =  "[::1]";
    // 3: ens2: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens2LlaSaObj; ens2LlaSaObj = "[fe80::5054:fe7f:c021%3]";
    SSockaddr ens2Ip4SaObj; ens2Ip4SaObj = "192.168.24.88";
    SSockaddr ens2GuaSaObj; ens2GuaSaObj = "[2001:db8::fe:fe7f:c021]";
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
    SSockaddr ens1Ip4SaObj; ens1Ip4SaObj = "192.168.24.88";
    // 2: lo0: <LOOPBACK,UP,LOWER_UP>
    SSockaddr loGuaSaObj; loGuaSaObj = "[2001:db8::1]";
    SSockaddr loIp4SaObj; loIp4SaObj = "127.0.0.1";
    SSockaddr loLlaSaObj; loLlaSaObj =  "[fe80::1%2]";
    SSockaddr loLopSaObj; loLopSaObj =  "[::1]";
    // 3: ens2: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens2LlaSaObj; ens2LlaSaObj = "[fe80::5054:fe7f:c021%3]";
    SSockaddr ens2Ip4SaObj; ens2Ip4SaObj = "192.168.24.89";
    SSockaddr ens2GuaSaObj; ens2GuaSaObj = "[2001:db8::fe:fe7f:c021]";
    // 4: ens3: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens3Ip4SaObj; ens3Ip4SaObj = "192.168.24.90";
    SSockaddr ens3GuaSaObj; ens3GuaSaObj = "[2001:db8::fe:fe7f:c022]";
    SSockaddr ens3LlaSaObj; ens3LlaSaObj = "[fe80::5054:fe7f:c021%4]";
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
    SSockaddr ens1LlaSaObj; ens1LlaSaObj = "[fe80::5054:fe7f:c021%1]";
    SSockaddr ens1Ip4SaObj; ens1Ip4SaObj = "192.168.24.89";
    SSockaddr ens1GuaSaObj; ens1GuaSaObj = "[2001:db8::fe:fe7f:c021]";
    // 2: lo0: <LOOPBACK,UP,LOWER_UP>
    SSockaddr loGuaSaObj; loGuaSaObj = "[2001:db8::1]";
    SSockaddr loIp4SaObj; loIp4SaObj = "127.0.0.1";
    SSockaddr loLlaSaObj; loLlaSaObj =  "[fe80::1%2]";
    SSockaddr loLopSaObj; loLopSaObj =  "[::1]";
    // 3: ens2: <BROADCAST,MULTICAST,UP,LOWER_UP>
    SSockaddr ens2Ip4SaObj; ens2Ip4SaObj = "192.168.24.90";
    SSockaddr ens2GuaSaObj; ens2GuaSaObj = "[2001:db8::fe:fe7f:c022]";
    SSockaddr ens2LlaSaObj; ens2LlaSaObj = "[fe80::5054:fe7f:c021%3]";
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
