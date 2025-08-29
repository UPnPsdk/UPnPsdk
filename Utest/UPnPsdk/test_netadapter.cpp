// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-08-29

#ifdef _MSC_VER
#include <UPnPsdk/src/net/netadapter_win32.cpp>
#include <UPnPsdk/sockaddr.hpp>
#endif

#include <UPnPsdk/netadapter.hpp>
#include <utest/utest.hpp>

#include <iomanip>


namespace utest {

using ::testing::_;
using ::testing::AnyOf;
using ::testing::HasSubstr;
using ::testing::Return;

using ::UPnPsdk::bitmask_to_netmask;
using ::UPnPsdk::CNetadapter;
using ::UPnPsdk::netmask_to_bitmask;
using ::UPnPsdk::SSockaddr;


UPnPsdk::SSockaddr saddrObj;
UPnPsdk::SSockaddr snmskObj;

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

TEST(NetadapterTestSuite, get_adapters_info_successful) {
    // The index of the loopback interface is usually 1, but you cannot rely on
    // this. A network interface may have different interface indexes for the
    // IPv4 and IPv6 loopback interface.
    // REF: [Loopback Interface Index]
    // (https://learn.microsoft.com/en-us/dotnet/api/system.net.networkinformation.networkinterface.loopbackinterfaceindex)
    // (https://study-ccna.com/loopback-interface-loopback-address/)
    char addrStr[INET6_ADDRSTRLEN];
    char nmskStr[INET6_ADDRSTRLEN];
    char servStr[NI_MAXSERV];

    CNetadapter nadObj;
    ASSERT_NO_THROW(nadObj.get_first());

    do {
        ASSERT_FALSE(nadObj.name().empty());
        nadObj.sockaddr(saddrObj);
        ASSERT_THAT(saddrObj.ss.ss_family, AnyOf(AF_INET6, AF_INET));
        // Check valid ip address
        ASSERT_EQ(::getnameinfo(&saddrObj.sa, sizeof(saddrObj.ss), addrStr,
                                sizeof(addrStr), servStr, sizeof(servStr),
                                NI_NUMERICHOST),
                  0);
        nadObj.socknetmask(snmskObj);
        // Check valid netmask
        ASSERT_EQ(::getnameinfo(&snmskObj.sa, sizeof(snmskObj.ss), nmskStr,
                                sizeof(nmskStr), nullptr, 0, NI_NUMERICHOST),
                  0);
        ASSERT_NE(nadObj.index(), 0);
        if (saddrObj.netaddr() == "[::1]")
            EXPECT_EQ(nadObj.bitmask(), 128);
        if (saddrObj.sin.sin_family == AF_INET &&
            ntohl(saddrObj.sin.sin_addr.s_addr) == 2130706433) // "127.0.0.1"
            EXPECT_EQ(nadObj.bitmask(), 8);
#if 0
        // To show resolved iface names set first NI_NUMERICHOST above to 0.
        std::cout << "DEBUG: \"" << nadObj.name() << "\" address = " << addrStr
                  << "(" << saddrObj.netaddr()
                  << "), netmask = " << snmskObj.netaddr()
                  << ", service = " << servStr
                  << ", index = " << nadObj.index() << '\n';
#endif
    } while (nadObj.get_next());
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
    ASSERT_NO_THROW(nadaptObj.get_first());
    // There is no next adapter, but ...
    EXPECT_FALSE(nadaptObj.find_next());
    // ... very first adapter is selected from get_first().
    EXPECT_NE(nadaptObj.index(), 0);

    // and we get the values from the first ip address of the internal adapter
    // list.
    ASSERT_NE(nadaptObj.name(), "");
    EXPECT_NE(nadaptObj.bitmask(), 0);
    nadaptObj.sockaddr(saddrObj);
    EXPECT_NE(saddrObj.netaddrp(), ":0");
    nadaptObj.socknetmask(saddrObj);
    EXPECT_NE(saddrObj.netaddr(), "");
}

TEST(NetadapterTestSuite, find_first_adapters_info) {
    CNetadapter nadaptObj;
    ASSERT_NO_THROW(nadaptObj.get_first());
    EXPECT_NE(nadaptObj.index(), 0);

    // Index 1 must not be the loopback adapter so I ask for it before using
    // its name. The loopback address can be any ip address "127.0.0.0" to
    // "127.255.255.255" or "[::1]".
    EXPECT_TRUE(nadaptObj.find_first("loopback"));
    EXPECT_NE(nadaptObj.index(), 0);
    ASSERT_NE(nadaptObj.name(), "");
    std::string lo_name{nadaptObj.name()};
    EXPECT_NE(nadaptObj.bitmask(), 0);
    nadaptObj.sockaddr(saddrObj);
    EXPECT_NE(saddrObj.netaddrp(), ":0");
    nadaptObj.socknetmask(saddrObj);
    EXPECT_NE(saddrObj.netaddr(), "");

    // If the local network adapter has only loopback addresses, it is ignored.
    if (nadaptObj.find_first(lo_name)) {
        EXPECT_NE(nadaptObj.index(), 0);
        EXPECT_EQ(nadaptObj.name(), lo_name);
    } else {
        // But you get the values from the first entry of the internal network
        // adapter list because you started finding with 'get_first()'. If
        // 'find_first()' fails it doesn't modify any pointers to the list. So
        // these values are valid for the first entry but have no useful meaning
        // in this context. They can be from any of the available adapters. You
        // should never ignore the result from 'find_first()'.
        EXPECT_NE(nadaptObj.index(), 0);
        EXPECT_NE(nadaptObj.name(), "");
        EXPECT_NE(nadaptObj.bitmask(), 0);
        nadaptObj.sockaddr(saddrObj);
        EXPECT_NE(saddrObj.ss.ss_family, AF_UNSPEC);
        nadaptObj.socknetmask(saddrObj);
        EXPECT_NE(saddrObj.ss.ss_family, AF_UNSPEC);
    }
    ASSERT_TRUE(nadaptObj.find_first("::1"));
    EXPECT_NE(nadaptObj.index(), 0);
    EXPECT_EQ(nadaptObj.bitmask(), 128);
    nadaptObj.socknetmask(saddrObj);
    EXPECT_EQ(saddrObj.netaddr(), "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]");

    ASSERT_TRUE(nadaptObj.find_first("127.0.0.1"));
    EXPECT_NE(nadaptObj.index(), 0);
    EXPECT_EQ(nadaptObj.bitmask(), 8);
    nadaptObj.socknetmask(saddrObj);
    EXPECT_EQ(saddrObj.netaddr(), "255.0.0.0");

    EXPECT_FALSE(nadaptObj.find_first(0));
    EXPECT_EQ(nadaptObj.index(), 0);
    EXPECT_EQ(nadaptObj.name(), "");
    EXPECT_EQ(nadaptObj.bitmask(), 0);
    nadaptObj.sockaddr(saddrObj);
    EXPECT_EQ(saddrObj.netaddrp(), ":0");
    nadaptObj.socknetmask(saddrObj);
    EXPECT_EQ(saddrObj.netaddr(), "");

    EXPECT_FALSE(nadaptObj.find_first(~0u - 2));
    EXPECT_EQ(nadaptObj.index(), 0);
    EXPECT_EQ(nadaptObj.name(), "");
    EXPECT_EQ(nadaptObj.bitmask(), 0);
    nadaptObj.sockaddr(saddrObj);
    EXPECT_EQ(saddrObj.netaddrp(), ":0");
    nadaptObj.socknetmask(saddrObj);
    EXPECT_EQ(saddrObj.netaddr(), "");

    // This will return a prefered socket address from the operating system.
    EXPECT_TRUE(nadaptObj.find_first()); // or .find_first("");
    EXPECT_NE(nadaptObj.index(), 0);
    EXPECT_NE(nadaptObj.name(), "");
    EXPECT_NE(nadaptObj.bitmask(), 0);
    nadaptObj.sockaddr(saddrObj);
    EXPECT_NE(saddrObj.netaddrp(), ":0");
    nadaptObj.socknetmask(saddrObj);
    EXPECT_NE(saddrObj.netaddr(), "");
}

TEST(NetadapterTestSuite, find_loopback_adapter_info) {
    CNetadapter nadaptObj;
    ASSERT_NO_THROW(nadaptObj.get_first());
    unsigned int index{0};
    std::string name;

    EXPECT_TRUE(nadaptObj.find_first("loopback"));
    index = nadaptObj.index();
    ASSERT_NE(index, 0);
    name = nadaptObj.name();
    ASSERT_FALSE(name.empty());
    EXPECT_NE(nadaptObj.bitmask(), 0);
    nadaptObj.sockaddr(saddrObj);
    EXPECT_TRUE(saddrObj.is_loopback());
    nadaptObj.socknetmask(saddrObj);
    EXPECT_THAT(
        saddrObj.netaddr(),
        AnyOf("[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]", "255.0.0.0"));

    if (nadaptObj.find_next()) {
        EXPECT_EQ(nadaptObj.index(), index);
        EXPECT_EQ(nadaptObj.name(), name);
        EXPECT_NE(nadaptObj.bitmask(), 0);
        nadaptObj.sockaddr(saddrObj);
        EXPECT_THAT(saddrObj.netaddrp(), AnyOf("[::1]:0", "127.0.0.1:0"));
        nadaptObj.socknetmask(saddrObj);
        EXPECT_THAT(
            saddrObj.netaddr(),
            AnyOf("[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]", "255.0.0.0"));
    }
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

TEST(NetadapterTestSuite, mock_netadapter_successful) {
    // Create mocking di-service object and get the smart pointer to it.
    auto nadap_mockPtr = std::make_shared<CNetadapterMock>();
    SSockaddr sa_mockObj;
    sa_mockObj = "[fe80::2]:50122";
    SSockaddr nm_mockObj;
    nm_mockObj = "[ffff:ffff:ffff:ffff::]";

    EXPECT_CALL(*nadap_mockPtr, get_first()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, index()).WillOnce(Return(2));
    EXPECT_CALL(*nadap_mockPtr, name()).WillOnce(Return("eth0"));
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(sa_mockObj));
    EXPECT_CALL(*nadap_mockPtr, socknetmask(_))
        .WillOnce(SaddrCpyToArg<0>(nm_mockObj));
    EXPECT_CALL(*nadap_mockPtr, bitmask()).WillOnce(Return(64));

    // Test Unit
    CNetadapter nadapObj(nadap_mockPtr);
    nadapObj.get_first();

    EXPECT_EQ(nadapObj.index(), 2);
    EXPECT_EQ(nadapObj.name(), "eth0");
    saddrObj = "";
    nadapObj.sockaddr(saddrObj);
    EXPECT_EQ(saddrObj, sa_mockObj);
    nadapObj.socknetmask(saddrObj);
    EXPECT_TRUE(saddrObj == nm_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 64);
}

TEST(NetadapterTestSuite, mock_netadapter_get_address_sequence) {
    // Create mocking di-service object and get the smart pointer to it.
    auto nadap_mockPtr = std::make_shared<CNetadapterMock>();
    SSockaddr sa1_mockObj;
    sa1_mockObj = "[fe80::3]:50123";
    SSockaddr sa2_mockObj;
    sa2_mockObj = "[2001:db8::4]:50124";
    SSockaddr sa3_mockObj;
    sa3_mockObj = "192.168.74.224:50125";
    SSockaddr sa3map_mockObj;
    sa3map_mockObj = "[::ffff:192.168.74.224]:50125";

    EXPECT_CALL(*nadap_mockPtr, get_first()).Times(1);

    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(2))
        .WillOnce(Return(3))
        .WillOnce(Return(2));

    EXPECT_CALL(*nadap_mockPtr, name())
        .WillOnce(Return("eth0"))
        .WillOnce(Return("eth1"))
        .WillOnce(Return("eth0"));

    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(sa1_mockObj))
        .WillOnce(SaddrCpyToArg<0>(sa2_mockObj))
        .WillOnce(SaddrCpyToArg<0>(sa3_mockObj));

    EXPECT_CALL(*nadap_mockPtr, bitmask())
        .WillOnce(Return(64))
        .WillOnce(Return(64))
        .WillOnce(Return(64));

    EXPECT_CALL(*nadap_mockPtr, reset()).Times(0);

    EXPECT_CALL(*nadap_mockPtr, get_next())
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(false));

    // Test Unit
    CNetadapter nadapObj(nadap_mockPtr);
    nadapObj.get_first();

    EXPECT_EQ(nadapObj.index(), 2);
    EXPECT_EQ(nadapObj.name(), "eth0");
    nadapObj.sockaddr(saddrObj);
    EXPECT_TRUE(saddrObj == sa1_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 64);
    ASSERT_TRUE(nadapObj.get_next());

    EXPECT_EQ(nadapObj.index(), 3);
    EXPECT_EQ(nadapObj.name(), "eth1");
    nadapObj.sockaddr(saddrObj);
    EXPECT_TRUE(saddrObj == sa2_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 64);
    ASSERT_TRUE(nadapObj.get_next());

    EXPECT_EQ(nadapObj.index(), 2);
    EXPECT_EQ(nadapObj.name(), "eth0");
    nadapObj.sockaddr(saddrObj);
    EXPECT_TRUE(saddrObj == sa3map_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 64);
    ASSERT_FALSE(nadapObj.get_next());
}

TEST(NetadapterTestSuite, mock_netadapter_find_default_with_1_addr) {
    // clang-format off
    // Network adapter: index, name, address
    /*1*/ char name1[]{"nad0"}; char addr1a[]{"[fe80::c021%1]:50011"};

    SSockaddr sa1a_mockObj; sa1a_mockObj = addr1a;
    // clang-format on

    // Create mocking di-service object and get the smart pointer to it.
    auto nadap_mockPtr = std::make_shared<CNetadapterMock>();

    EXPECT_CALL(*nadap_mockPtr, get_first()).Times(1); // Call below for testing

    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(1))  // Used in Unit.
        .WillOnce(Return(1)); // Repeat for EXPECT below.

    EXPECT_CALL(*nadap_mockPtr, name()).WillOnce(Return(name1));

    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(sa1a_mockObj))  // Used in Unit.
        .WillOnce(SaddrCpyToArg<0>(sa1a_mockObj)); // Repeat for EXPECT below.

    EXPECT_CALL(*nadap_mockPtr, bitmask()).WillOnce(Return(64));
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, get_next()).WillOnce(Return(false));

    // Test Unit
    CNetadapter nadapObj(nadap_mockPtr); // Inject mocked functions.
    nadapObj.get_first();
    nadapObj.find_first();

    EXPECT_EQ(nadapObj.index(), 1);
    EXPECT_EQ(nadapObj.name(), name1);
    nadapObj.sockaddr(saddrObj); // Triggers additional expected call.
    EXPECT_EQ(saddrObj, sa1a_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 64);
    ASSERT_FALSE(nadapObj.find_next());
}

TEST(NetadapterTestSuite, mock_netadapter_find_name_with_1_addr) {
    // clang-format off
    // Network adapter: index, name, address
    /*1*/ char name1[]{"nad0"}; char addr1a[]{"[fe80::c031%1]:50011"};

    SSockaddr sa1a_mockObj; sa1a_mockObj = addr1a;
    // clang-format on

    // Create mocking di-service object and get the smart pointer to it.
    auto nadap_mockPtr = std::make_shared<CNetadapterMock>();

    EXPECT_CALL(*nadap_mockPtr, get_first()).Times(1); // Call below for testing

    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(1))  // Used in Unit.
        .WillOnce(Return(1)); // Repeat for EXPECT below.

    EXPECT_CALL(*nadap_mockPtr, name())
        .WillOnce(Return(name1))  // Used in Unit.
        .WillOnce(Return(name1)); // Repeat for EXPECT below.

    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(sa1a_mockObj))  // Used in Unit.
        .WillOnce(SaddrCpyToArg<0>(sa1a_mockObj)); // Repeat for EXPECT below.

    EXPECT_CALL(*nadap_mockPtr, bitmask()).WillOnce(Return(64));
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, get_next()).WillOnce(Return(false));

    // Test Unit
    CNetadapter nadapObj(nadap_mockPtr); // Inject mocked functions.
    nadapObj.get_first();
    nadapObj.find_first("nad0");

    EXPECT_EQ(nadapObj.index(), 1);
    EXPECT_EQ(nadapObj.name(), name1);
    nadapObj.sockaddr(saddrObj); // Triggers additional expected call.
    EXPECT_EQ(saddrObj, sa1a_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 64);
    ASSERT_FALSE(nadapObj.find_next());
}

TEST(NetadapterTestSuite, mock_netadapter_find_index_with_1_addr) {
    // clang-format off
    // Network adapter: index, name, address
    /*1*/ char name1[]{"nad0"}; char addr1a[]{"[fe80::c041%1]:50011"};

    SSockaddr sa1a_mockObj; sa1a_mockObj = addr1a;
    // clang-format on

    // Create mocking di-service object and get the smart pointer to it.
    auto nadap_mockPtr = std::make_shared<CNetadapterMock>();

    EXPECT_CALL(*nadap_mockPtr, get_first()).Times(1); // Call below for testing

    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(1))  // Used in Unit.
        .WillOnce(Return(1))  // Used in Unit.
        .WillOnce(Return(1)); // Repeat for EXPECT below.

    EXPECT_CALL(*nadap_mockPtr, name()).WillOnce(Return(name1));

    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(sa1a_mockObj))  // Used in Unit.
        .WillOnce(SaddrCpyToArg<0>(sa1a_mockObj)); // Repeat for EXPECT below.

    EXPECT_CALL(*nadap_mockPtr, bitmask()).WillOnce(Return(64));
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1); // Used in Unit.
    EXPECT_CALL(*nadap_mockPtr, get_next()).WillOnce(Return(false));

    // Test Unit
    CNetadapter nadapObj(nadap_mockPtr); // Inject mocked functions.
    nadapObj.get_first();
    nadapObj.find_first(1);

    EXPECT_EQ(nadapObj.index(), 1);
    EXPECT_EQ(nadapObj.name(), name1);
    nadapObj.sockaddr(saddrObj); // Triggers additional expected call.
    EXPECT_EQ(saddrObj, sa1a_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 64);
    ASSERT_FALSE(nadapObj.find_next());
}

TEST(NetadapterTestSuite, mock_netadapter_find_default_with_8_addr) {
    // clang-format off
    // Network adapter: index, name, address (attention to scope_id = idx?).
    unsigned int idx1{101}; char name1[]{"lo"};
                                                  char addr1a[]{"127.0.0.1:50011"};
                                                  char addr1b[]{"[::1]:50012"};
                                                  char addr1c[]{"[fe80::1%101]:50013"};
    unsigned int idx2{102}; char name2[]{"nad0"};
                                                  char addr2a[]{"[2001:db8::2:a]:50021"};
                                                  char addr2b[]{"[fe80::2:b%102]:50022"};
                                                  char addr2c[]{"192.168.2.3:50023"};
    unsigned int idx3{103}; char name3[]{"nad1"};
                                                  char addr3a[]{"192.168.3.1:50031"};
                                                  char addr3b[]{"[fe80::3:b%103]:50032"};
    (void)addr1a; (void)addr1b; // Unused variables.

    // Sequence, means priority, in internal netadapter list. This sequence is
    // realized with the mocked expectations below.
    // -----------------------------------------------------------------------
    // addr2a, addr3a, addr2c, addr1c, addr3b, addr2b, addr1b, addr1a


    // Instantiate socket addresses that will be returned instead of the real
    // ones from the operating system.
    // SSockaddr sa1a_mockObj; sa1a_mockObj = addr1a;
    // SSockaddr sa1b_mockObj; sa1b_mockObj = addr1b;
    SSockaddr sa1c_mockObj; sa1c_mockObj = addr1c;
    SSockaddr sa2a_mockObj; sa2a_mockObj = addr2a;
    SSockaddr sa2b_mockObj; sa2b_mockObj = addr2b;
    SSockaddr sa2c_mockObj; sa2c_mockObj = addr2c;
    SSockaddr sa3a_mockObj; sa3a_mockObj = addr3a;
    SSockaddr sa3b_mockObj; sa3b_mockObj = addr3b;
    // clang-format on

    // Create mocking di-service object and get the smart pointer to it.
    auto nadap_mockPtr = std::make_shared<CNetadapterMock>();

    EXPECT_CALL(*nadap_mockPtr, get_first()).Times(1);

    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(idx2))  // used in Unit one time for 'find_first()'.
        .WillOnce(Return(idx2))  // addr2a, returned for EXPECT below.
        .WillOnce(Return(idx3))  // addr3a, returned for EXPECT below.
        .WillOnce(Return(idx2))  // addr2c, returned for EXPECT below.
        .WillOnce(Return(idx1))  // addr1c, returned for EXPECT below.
        .WillOnce(Return(idx3))  // addr3b, returned for EXPECT below.
        .WillOnce(Return(idx2)); // addr2b, returned for EXPECT below.

    EXPECT_CALL(*nadap_mockPtr, name())
        .WillOnce(Return(name2))  // addr2a, returned for EXPECT below.
        .WillOnce(Return(name3))  // addr3a, returned for EXPECT below.
        .WillOnce(Return(name2))  // addr2c, returned for EXPECT below.
        .WillOnce(Return(name1))  // addr1c, returned for EXPECT below.
        .WillOnce(Return(name3))  // addr3b, returned for EXPECT below.
        .WillOnce(Return(name2)); // addr2b, returned for EXPECT below.

    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(sa2a_mockObj))  // used in Unit.
        .WillOnce(SaddrCpyToArg<0>(sa2a_mockObj))  // returned for EXPECT below.
        .WillOnce(SaddrCpyToArg<0>(sa3a_mockObj))  // used in Unit.
        .WillOnce(SaddrCpyToArg<0>(sa3a_mockObj))  // returned for EXPECT below.
        .WillOnce(SaddrCpyToArg<0>(sa2c_mockObj))  // used in Unit.
        .WillOnce(SaddrCpyToArg<0>(sa2c_mockObj))  // returned for EXPECT below.
        .WillOnce(SaddrCpyToArg<0>(sa1c_mockObj))  // used in Unit.
        .WillOnce(SaddrCpyToArg<0>(sa1c_mockObj))  // returned for EXPECT below.
        .WillOnce(SaddrCpyToArg<0>(sa3b_mockObj))  // used in Unit.
        .WillOnce(SaddrCpyToArg<0>(sa3b_mockObj))  // returned for EXPECT below.
        .WillOnce(SaddrCpyToArg<0>(sa2b_mockObj))  // used in Unit.
        .WillOnce(SaddrCpyToArg<0>(sa2b_mockObj)); // returned for EXPECT below.

    EXPECT_CALL(*nadap_mockPtr, bitmask())
        .WillOnce(Return(64))  // addr2a, returned for EXPECT below.
        .WillOnce(Return(64))  // addr3a, returned for EXPECT below.
        .WillOnce(Return(64))  // addr2c, returned for EXPECT below.
        .WillOnce(Return(64))  // addr1c, returned for EXPECT below.
        .WillOnce(Return(64))  // addr3b, returned for EXPECT below.
        .WillOnce(Return(64)); // addr2b, returned for EXPECT below.

    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);

    EXPECT_CALL(*nadap_mockPtr, get_next())
        .WillOnce(Return(true))   // gets addr3a, returned for EXPECT below.
        .WillOnce(Return(true))   // gets addr2c, returned for EXPECT below.
        .WillOnce(Return(true))   // gets addr1c, returned for EXPECT below.
        .WillOnce(Return(true))   // gets addr3b, returned for EXPECT below.
        .WillOnce(Return(true))   // gets addr2b, returned for EXPECT below.
        .WillOnce(Return(false)); // Finish, addr1b and addr1a are skipped
                                  // due to loopback addresses.

    // Test Unit
    CNetadapter nadapObj(nadap_mockPtr); // Inject pointer to mocked functions.
    nadapObj.get_first();
    nadapObj.find_first();

    // Should get addr2a.
    EXPECT_EQ(nadapObj.index(), idx2);
    EXPECT_EQ(nadapObj.name(), name2);
    nadapObj.sockaddr(saddrObj);
    EXPECT_TRUE(saddrObj == sa2a_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 64);

    // Should get addr3a.
    SSockaddr sa3a_map_mockObj;
    sa3a_map_mockObj = "[::ffff:192.168.3.1]:50031";
    EXPECT_TRUE(nadapObj.find_next());
    EXPECT_EQ(nadapObj.index(), idx3);
    EXPECT_EQ(nadapObj.name(), name3);
    nadapObj.sockaddr(saddrObj);
    EXPECT_TRUE(saddrObj == sa3a_map_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 64);

    // Should get addr2c.
    SSockaddr sa2c_map_mockObj;
    sa2c_map_mockObj = "[::ffff:192.168.2.3]:50023";
    EXPECT_TRUE(nadapObj.find_next());
    EXPECT_EQ(nadapObj.index(), idx2);
    EXPECT_EQ(nadapObj.name(), name2);
    nadapObj.sockaddr(saddrObj);
    EXPECT_TRUE(saddrObj == sa2c_map_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 64);

    // Should get addr1c.
    EXPECT_TRUE(nadapObj.find_next());
    EXPECT_EQ(nadapObj.index(), idx1);
    EXPECT_EQ(nadapObj.name(), name1);
    nadapObj.sockaddr(saddrObj);
    EXPECT_TRUE(saddrObj == sa1c_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 64);

    // Should get addr3b.
    EXPECT_TRUE(nadapObj.find_next());
    EXPECT_EQ(nadapObj.index(), idx3);
    EXPECT_EQ(nadapObj.name(), name3);
    nadapObj.sockaddr(saddrObj);
    EXPECT_TRUE(saddrObj == sa3b_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 64);

    // Should get addr2b.
    EXPECT_TRUE(nadapObj.find_next());
    EXPECT_EQ(nadapObj.index(), idx2);
    EXPECT_EQ(nadapObj.name(), name2);
    nadapObj.sockaddr(saddrObj);
    EXPECT_TRUE(saddrObj == sa2b_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 64);

    // Finish, addr1b and addr1a are skipped due to loopback addresses.
    EXPECT_FALSE(nadapObj.find_next());
}

TEST(NetadapterTestSuite, mock_netadapter_find_name_with_8_addr) {
    // clang-format off
    // Network adapter: index, name, address (attention to scope_id = idx?).
    unsigned int idx1{101}; char name1[]{"lo"};
                                                  char addr1a[]{"127.0.0.1:50011"};
                                                  char addr1b[]{"[::1]:50012"};
                                                  char addr1c[]{"[fe80::1%101]:50013"};
    unsigned int idx2{102}; char name2[]{"nad0"};
                                                  char addr2a[]{"[2001:db8::2:a]:50021"};
                                                  char addr2b[]{"[fe80::2:b%102]:50022"};
                                                  char addr2c[]{"192.168.2.3:50023"};
    unsigned int idx3{103}; char name3[]{"nad1"};
                                                  char addr3a[]{"192.168.3.1:50031"};
                                                  char addr3b[]{"[fe80::3:b%103]:50032"};
    (void)addr1a; (void)addr1b; (void)addr1c; (void)addr3a; (void)addr3b;
    (void)name1; (void)name3; // Unused variables.

    // Sequence, means priority, in internal netadapter list. This sequence is
    // realized with the mocked expectations below.
    // -----------------------------------------------------------------------
    // addr2a, addr3a, addr2c, addr1c, addr3b, addr2b, addr1b, addr1a


    // Instantiate socket addresses that will be returned instead of the real
    // ones from the operating system.
    // SSockaddr sa1a_mockObj; sa1a_mockObj = addr1a;
    // SSockaddr sa1b_mockObj; sa1b_mockObj = addr1b;
    // SSockaddr sa1c_mockObj; sa1c_mockObj = addr1c;
    SSockaddr sa2a_mockObj; sa2a_mockObj = addr2a;
    SSockaddr sa2b_mockObj; sa2b_mockObj = addr2b;
    SSockaddr sa2c_mockObj; sa2c_mockObj = addr2c;
    // SSockaddr sa3a_mockObj; sa3a_mockObj = addr3a;
    // SSockaddr sa3b_mockObj; sa3b_mockObj = addr3b;
    // clang-format on

    // Create mocking di-service object and get the smart pointer to it.
    auto nadap_mockPtr = std::make_shared<CNetadapterMock>();

    EXPECT_CALL(*nadap_mockPtr, get_first()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(idx2))  // addr2a
        .WillOnce(Return(idx2))  // Returned for EXPECT below.
        .WillOnce(Return(idx3))  // addr3a, intern skipped.
        .WillOnce(Return(idx2))  // addr2c, matched.
        .WillOnce(Return(idx2))  // Returned for EXPECT below.
        .WillOnce(Return(idx1))  // addr1c, intern skipped.
        .WillOnce(Return(idx3))  // addr3b, intern skipped.
        .WillOnce(Return(idx2))  // addr2b, matched.
        .WillOnce(Return(idx2))  // Returned for EXPECT below.
        .WillOnce(Return(idx1))  // addr1b, intern skipped.
        .WillOnce(Return(idx1)); // addr1a, intern skipped.

    EXPECT_CALL(*nadap_mockPtr, name())
        .WillOnce(Return(name2))  // addr2a, find_first() match.
        .WillOnce(Return(name2))  // addr2a, returned for EXPECT below.
        .WillOnce(Return(name2))  // addr2c, returned for EXPECT below.
        .WillOnce(Return(name2)); // addr2b, returned for EXPECT below.

    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(sa2a_mockObj)) // Intern check loopback addr.
        .WillOnce(SaddrCpyToArg<0>(sa2a_mockObj)) // Returned for EXPECT below.
        .WillOnce(SaddrCpyToArg<0>(sa2c_mockObj)) // Intern check loopback addr.
        .WillOnce(SaddrCpyToArg<0>(sa2c_mockObj)) // Returned for EXPECT below.
        .WillOnce(SaddrCpyToArg<0>(sa2b_mockObj)) // Intern check loopback addr.
        .WillOnce(SaddrCpyToArg<0>(sa2b_mockObj)); // Returned for EXPECT below.

    EXPECT_CALL(*nadap_mockPtr, bitmask())
        .WillOnce(Return(64)) // Returned for EXPECT below.
        .WillOnce(Return(64))
        .WillOnce(Return(64));

    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .WillOnce(Return(true))   // gets addr3a, doesn't match index=2.
        .WillOnce(Return(true))   // gets addr2c, does match index=2.
        .WillOnce(Return(true))   // gets addr1c, doesn't match index=2.
        .WillOnce(Return(true))   // gets addr3b, doesn't match index=2.
        .WillOnce(Return(true))   // gets addr2b, does match index=2.
        .WillOnce(Return(true))   // gets addr1b, doesn't match index=2.
        .WillOnce(Return(true))   // gets addr1a, doesn't match index=2.
        .WillOnce(Return(false)); // Finish.

    // Test Unit
    CNetadapter nadapObj(nadap_mockPtr); // Inject pointer to mocked functions.
    nadapObj.get_first();
    nadapObj.find_first("nad0");

    // Should get addr2a.
    EXPECT_EQ(nadapObj.index(), idx2);
    EXPECT_EQ(nadapObj.name(), name2);
    nadapObj.sockaddr(saddrObj);
    EXPECT_TRUE(saddrObj == sa2a_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 64);

    // Should get addr2c, skipping addr3a.
    SSockaddr sa2c_map_mockObj;
    sa2c_map_mockObj = "[::ffff:192.168.2.3]:50023";
    EXPECT_TRUE(nadapObj.find_next());
    EXPECT_EQ(nadapObj.index(), idx2);
    EXPECT_EQ(nadapObj.name(), name2);
    nadapObj.sockaddr(saddrObj);
    EXPECT_TRUE(saddrObj == sa2c_map_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 64);

    // Should get addr2b, skipping addr1c and addr3b.
    EXPECT_TRUE(nadapObj.find_next());
    EXPECT_EQ(nadapObj.index(), idx2);
    EXPECT_EQ(nadapObj.name(), name2);
    nadapObj.sockaddr(saddrObj);
    EXPECT_TRUE(saddrObj == sa2b_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 64);

    // Should skip addr1b and addr1a.
    EXPECT_FALSE(nadapObj.find_next());
    // Should do nothing.
    EXPECT_FALSE(nadapObj.find_next());
}

TEST(NetadapterTestSuite, mock_netadapter_find_index_with_8_addr) {
    // clang-format off
    // Network adapter: index, name, address (attention to scope_id = idx?).
    unsigned int idx1{101}; char name1[]{"lo"};
                                                  char addr1a[]{"127.0.0.1:50011"};
                                                  char addr1b[]{"[::1]:50012"};
                                                  char addr1c[]{"[fe80::1%101]:50013"};
    unsigned int idx2{102}; char name2[]{"nad0"};
                                                  char addr2a[]{"[2001:db8::2:a]:50021"};
                                                  char addr2b[]{"[fe80::2:b%102]:50022"};
                                                  char addr2c[]{"192.168.2.3:50023"};
    unsigned int idx3{103}; char name3[]{"nad1"};
                                                  char addr3a[]{"192.168.3.1:50031"};
                                                  char addr3b[]{"[fe80::3:b%103]:50032"};
    (void)addr1a; (void)addr1b; (void)addr1c; (void)addr3a; (void)addr3b;
    (void)name1; (void)name3; // Unused variables.

    // Sequence, means priority, in internal netadapter list. This sequence is
    // realized with the mocked expectations below.
    // -----------------------------------------------------------------------
    // addr2a, addr3a, addr2c, addr1c, addr3b, addr2b, addr1b, addr1a


    // Instantiate socket addresses that will be returned instead of the real
    // ones from the operating system.
    // SSockaddr sa1a_mockObj; sa1a_mockObj = addr1a;
    // SSockaddr sa1b_mockObj; sa1b_mockObj = addr1b;
    // SSockaddr sa1c_mockObj; sa1c_mockObj = addr1c;
    SSockaddr sa2a_mockObj; sa2a_mockObj = addr2a;
    SSockaddr sa2b_mockObj; sa2b_mockObj = addr2b;
    SSockaddr sa2c_mockObj; sa2c_mockObj = addr2c;
    // SSockaddr sa3a_mockObj; sa3a_mockObj = addr3a;
    // SSockaddr sa3b_mockObj; sa3b_mockObj = addr3b;
    // clang-format on

    // Create mocking di-service object and get the smart pointer to it.
    auto nadap_mockPtr = std::make_shared<CNetadapterMock>();

    EXPECT_CALL(*nadap_mockPtr, get_first()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(idx2))  // used in Unit one time for 'find_first()'.
        .WillOnce(Return(idx2))  // addr2a
        .WillOnce(Return(idx2))  // Returned for EXPECT below.
        .WillOnce(Return(idx3))  // addr3a, intern skipped.
        .WillOnce(Return(idx2))  // addr2c, matched.
        .WillOnce(Return(idx2))  // Returned for EXPECT below.
        .WillOnce(Return(idx1))  // addr1c, intern skipped.
        .WillOnce(Return(idx3))  // addr3b, intern skipped.
        .WillOnce(Return(idx2))  // addr2b, matched.
        .WillOnce(Return(idx2))  // Returned for EXPECT below.
        .WillOnce(Return(idx1))  // addr1b, intern skipped.
        .WillOnce(Return(idx1)); // addr1a, intern skipped.

    EXPECT_CALL(*nadap_mockPtr, name())
        .WillOnce(Return(name2))  // addr2a, returned for EXPECT below.
        .WillOnce(Return(name2))  // addr2c, returned for EXPECT below.
        .WillOnce(Return(name2)); // addr2b, returned for EXPECT below.

    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(sa2a_mockObj)) // Intern check loopback addr
        .WillOnce(SaddrCpyToArg<0>(sa2a_mockObj)) // Returned for EXPECT below.
        .WillOnce(SaddrCpyToArg<0>(sa2c_mockObj))
        .WillOnce(SaddrCpyToArg<0>(sa2c_mockObj))
        .WillOnce(SaddrCpyToArg<0>(sa2b_mockObj))
        .WillOnce(SaddrCpyToArg<0>(sa2b_mockObj));

    EXPECT_CALL(*nadap_mockPtr, bitmask())
        .WillOnce(Return(64)) // Returned for EXPECT below.
        .WillOnce(Return(64))
        .WillOnce(Return(64));

    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .WillOnce(Return(true))   // gets addr3a, doesn't match index=2.
        .WillOnce(Return(true))   // gets addr2c, does match index=2.
        .WillOnce(Return(true))   // gets addr1c, doesn't match index=2.
        .WillOnce(Return(true))   // gets addr3b, doesn't match index=2.
        .WillOnce(Return(true))   // gets addr2b, does match index=2.
        .WillOnce(Return(true))   // gets addr1b, doesn't match index=2.
        .WillOnce(Return(true))   // gets addr1a, doesn't match index=2.
        .WillOnce(Return(false)); // Finish.

    // Test Unit
    CNetadapter nadapObj(nadap_mockPtr); // Inject pointer to mocked functions.
    nadapObj.get_first();
    nadapObj.find_first(102);

    // Should get addr2a.
    EXPECT_EQ(nadapObj.index(), idx2);
    EXPECT_EQ(nadapObj.name(), name2);
    nadapObj.sockaddr(saddrObj);
    EXPECT_TRUE(saddrObj == sa2a_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 64);

    // Should get addr2c, skipping addr3a.
    SSockaddr sa2c_map_mockObj;
    sa2c_map_mockObj = "[::ffff:192.168.2.3]:50023";
    EXPECT_TRUE(nadapObj.find_next());
    EXPECT_EQ(nadapObj.index(), idx2);
    EXPECT_EQ(nadapObj.name(), name2);
    nadapObj.sockaddr(saddrObj);
    EXPECT_TRUE(saddrObj == sa2c_map_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 64);

    // Should get addr2b, skipping addr1c and addr3b.
    EXPECT_TRUE(nadapObj.find_next());
    EXPECT_EQ(nadapObj.index(), idx2);
    EXPECT_EQ(nadapObj.name(), name2);
    nadapObj.sockaddr(saddrObj);
    EXPECT_TRUE(saddrObj == sa2b_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 64);

    // Should skip addr1b and addr1a.
    EXPECT_FALSE(nadapObj.find_next());
    // Should do nothing.
    EXPECT_FALSE(nadapObj.find_next());
}

TEST(NetadapterTestSuite, mock_netadapter_find_loopback_with_8_addr) {
    // clang-format off
    // Network adapter: index, name, address (attention to scope_id = idx?).
    unsigned int idx1{101}; char name1[]{"lo"};
                                                  char addr1a[]{"127.0.0.1:50011"};
                                                  char addr1b[]{"[::1]:50012"};
                                                  char addr1c[]{"[fe80::1%101]:50013"};
    unsigned int idx2{102}; char name2[]{"nad0"};
                                                  char addr2a[]{"[2001:db8::2:a]:50021"};
                                                  char addr2b[]{"[fe80::2:b%102]:50022"};
                                                  char addr2c[]{"192.168.2.3:50023"};
    unsigned int idx3{103}; char name3[]{"nad1"};
                                                  char addr3a[]{"192.168.3.1:50031"};
                                                  char addr3b[]{"[fe80::3:b%103]:50032"};
    (void)name2; (void)name3; // Unused variables.
    (void)idx2; (void)idx3;

    // Sequence, means priority, in internal netadapter list. This sequence is
    // realized with the mocked expectations below.
    // -----------------------------------------------------------------------
    // addr2a, addr3a, addr1b, addr1c, addr1a, addr2b, addr2c, addr3b

    // Instantiate socket addresses that will be returned instead of the real
    // ones from the operating system.
    SSockaddr sa1a_mockObj; sa1a_mockObj = addr1a;
    SSockaddr sa1b_mockObj; sa1b_mockObj = addr1b;
    SSockaddr sa1c_mockObj; sa1c_mockObj = addr1c;
    SSockaddr sa2a_mockObj; sa2a_mockObj = addr2a;
    SSockaddr sa2b_mockObj; sa2b_mockObj = addr2b;
    SSockaddr sa2c_mockObj; sa2c_mockObj = addr2c;
    SSockaddr sa3a_mockObj; sa3a_mockObj = addr3a;
    SSockaddr sa3b_mockObj; sa3b_mockObj = addr3b;
    // clang-format on

    // Create mocking di-service object and get the smart pointer to it.
    auto nadap_mockPtr = std::make_shared<CNetadapterMock>();

    // This determins the sequence of the mocked internal local adapter address
    // list.
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        .WillOnce(SaddrCpyToArg<0>(sa2a_mockObj))  // Used in Unit.
        .WillOnce(SaddrCpyToArg<0>(sa3a_mockObj))  // Used in Unit.
        .WillOnce(SaddrCpyToArg<0>(sa1b_mockObj))  // Used in Unit.
        .WillOnce(SaddrCpyToArg<0>(sa1b_mockObj))  // Returned for EXPECT below.
        .WillOnce(SaddrCpyToArg<0>(sa1c_mockObj))  // Used in Unit.
        .WillOnce(SaddrCpyToArg<0>(sa1a_mockObj))  // Used in Unit.
        .WillOnce(SaddrCpyToArg<0>(sa1a_mockObj))  // Returned for EXPECT below.
        .WillOnce(SaddrCpyToArg<0>(sa2b_mockObj))  // Used in Unit.
        .WillOnce(SaddrCpyToArg<0>(sa2c_mockObj))  // Used in Unit.
        .WillOnce(SaddrCpyToArg<0>(sa3b_mockObj)); // Used in Unit.

    EXPECT_CALL(*nadap_mockPtr, get_next())
        .WillOnce(Return(true))   // gets addr3a, doesn't match.
        .WillOnce(Return(true))   // gets addr1b, match.
        .WillOnce(Return(true))   // gets addr1c, doesn't match.
        .WillOnce(Return(true))   // gets addr1a, match.
        .WillOnce(Return(true))   // gets addr2b, doesn't match.
        .WillOnce(Return(true))   // gets addr2c, doesn't match.
        .WillOnce(Return(true))   // gets addr3b, doesn't match.
        .WillOnce(Return(false)); // Finish.

    EXPECT_CALL(*nadap_mockPtr, get_first()).Times(1);
    EXPECT_CALL(*nadap_mockPtr, reset()).Times(1);

    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(idx1))  // Returned for EXPECT below.
        .WillOnce(Return(idx1)); // Returned for EXPECT below.

    EXPECT_CALL(*nadap_mockPtr, name())
        .WillOnce(Return(name1))  // addr1b, returned for EXPECT below.
        .WillOnce(Return(name1)); // addr1a, returned for EXPECT below.

    EXPECT_CALL(*nadap_mockPtr, bitmask())
        .WillOnce(Return(64))  // Returned for EXPECT below.
        .WillOnce(Return(24)); // Returned for EXPECT below.

    // Test Unit
    CNetadapter nadapObj(nadap_mockPtr); // Inject pointer to mocked functions.
    nadapObj.get_first();
    nadapObj.find_first("loopback");

    // Should get addr1c, skipping addr2a, addr3a, addr1b.
    EXPECT_EQ(nadapObj.index(), idx1);
    EXPECT_EQ(nadapObj.name(), name1);
    nadapObj.sockaddr(saddrObj);
    EXPECT_TRUE(saddrObj == sa1b_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 64);

    // Should get addr1a, skipping addr1c.
    EXPECT_TRUE(nadapObj.find_next());
    EXPECT_EQ(nadapObj.index(), idx1);
    EXPECT_EQ(nadapObj.name(), name1);
    nadapObj.sockaddr(saddrObj);
    EXPECT_TRUE(saddrObj == sa1a_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 24);

    EXPECT_FALSE(nadapObj.find_next());
}

TEST(NetadapterTestSuite, mock_netadapter_find_default_ignores_loopback_adapt) {
    // If an adapter only contains loopback addresses, the adapter is ignored
    // as long as it isn't queried with "loopback". This test finds "lop1"
    // (contains also an lla) but not "lop2".

    // clang-format off
    // Network adapter: index, name, address (attention to scope_id = idx?).
    unsigned int idx1{101}; char name1[]{"lop1"};
                                                  char addr1a[]{"[::1]:50011"};
                                                  char addr1b[]{"127.0.0.1:50012"};
                                                  char addr1c[]{"[fe80::1%101]:50013"};
    unsigned int idx2{102}; char name2[]{"lop2"};
                                                  char addr2a[]{"127.128.129.130:50021"};
                                                  char addr2b[]{"[::1]:50022"};
    // Unused variables.
    (void)idx2; (void)addr2a; (void)addr1b; (void)addr1c;

    // Sequence, means priority, in internal netadapter list. This sequence is
    // realized with the mocked expectations below.
    // -----------------------------------------------------------------------
    // addr1b, addr2b, addr1a, addr2a, addr1c

    // Instantiate socket addresses that will be returned instead of the real
    // ones from the operating system.
    SSockaddr sa1a_mockObj; sa1a_mockObj = addr1a;
    SSockaddr sa2a_mockObj; sa2a_mockObj = addr2a;
    SSockaddr sa2b_mockObj; sa2b_mockObj = addr2b;

    // Create mocking di-service object and get the smart pointer to it.
    auto nadap_mockPtr = std::make_shared<CNetadapterMock>();

    // This determins the sequence of the mocked internal local adapter address
    // list.
    EXPECT_CALL(*nadap_mockPtr, sockaddr(_))
        // .WillOnce(SaddrCpyToArg<0>(sa1b_mockObj)) // Used in Unit.
           .WillOnce(SaddrCpyToArg<0>(sa2b_mockObj)) // Used in Unit.
        // .WillOnce(SaddrCpyToArg<0>(sa1a_mockObj)) // Used in Unit.
           .WillOnce(SaddrCpyToArg<0>(sa2a_mockObj)) // Used in Unit.
        // .WillOnce(SaddrCpyToArg<0>(sa2c_mockObj)) // Used in Unit.
        // Finish, nothing found, first entry returned for EXPECT below.
           .WillOnce(SaddrCpyToArg<0>(sa1a_mockObj));
    // clang-format on

    EXPECT_CALL(*nadap_mockPtr, get_first()).Times(1);
    //                            // gets addr1b, doesn't match.
    EXPECT_CALL(*nadap_mockPtr, get_next())
        .WillOnce(Return(true))   // gets addr2b, doesn't match.
        .WillOnce(Return(true))   // gets addr1a, doesn't match.
        .WillOnce(Return(true))   // gets addr2a, doesn't match.
        .WillOnce(Return(true))   // gets addr1c, doesn't match.
        .WillOnce(Return(false)); // Finish.

    EXPECT_CALL(*nadap_mockPtr, reset()).Times(2);
    EXPECT_CALL(*nadap_mockPtr, index())
        .WillOnce(Return(idx1)); // Returned for EXPECT below, nothing found.

    EXPECT_CALL(*nadap_mockPtr, name())
        .WillOnce(Return(name1))  // addr1b, find_first() lookup.
        .WillOnce(Return(name2))  // addr2b, find_first() lookup.
        .WillOnce(Return(name1))  // addr1a, find_first() lookup.
        .WillOnce(Return(name2))  // addr2a, find_first() lookup.
        .WillOnce(Return(name1))  // addr1c, find_first() lookup.
        .WillOnce(Return(name1)); // addr1a, returned for EXPECT below.

    EXPECT_CALL(*nadap_mockPtr, bitmask())
        .WillOnce(Return(128)); // Returned for EXPECT below, first entry.

    // Test Unit
    CNetadapter nadapObj(nadap_mockPtr); // Inject pointer to mocked functions.
    nadapObj.get_first();

    // We don't find the selected adapter because it's only loopback addresses.
    EXPECT_FALSE(nadapObj.find_first(name2));

    // We only have the first entry, initial selected with 'get_first()'.
    EXPECT_EQ(nadapObj.index(), idx1);
    EXPECT_EQ(nadapObj.name(), name1);
    nadapObj.sockaddr(saddrObj);
    EXPECT_EQ(saddrObj, sa1a_mockObj);
    EXPECT_EQ(nadapObj.bitmask(), 128);

    EXPECT_FALSE(nadapObj.find_next());
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
