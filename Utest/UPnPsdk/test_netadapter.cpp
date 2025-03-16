// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-03-16

#ifdef _MSC_VER
#include <UPnPsdk/src/net/netadapter_win32.cpp>
#include <UPnPsdk/sockaddr.hpp>
#endif

#include <UPnPsdk/netadapter.hpp>
#include <utest/utest.hpp>


namespace utest {

using ::testing::AnyOf;
using ::testing::HasSubstr;

using ::UPnPsdk::bitmask_to_netmask;
using ::UPnPsdk::netmask_to_bitmask;
using ::UPnPsdk::SSockaddr;


UPnPsdk::SSockaddr saddrObj;
UPnPsdk::SSockaddr snmskObj;

TEST(NetadapterTestSuite, get_adapters_info_successful) {
    // The index of the loopback interface is usually 1, but you cannot rely on
    // this. A network interface may have different interface indexes for the
    // IPv4 and IPv6 loopback interface. Loopback addresses are not limited to
    // the 127.0.0.0/8 block.
    // REF: [Loopback Interface Index]
    // (https://learn.microsoft.com/en-us/dotnet/api/system.net.networkinformation.networkinterface.loopbackinterfaceindex)
    // (https://study-ccna.com/loopback-interface-loopback-address/)
    char addrStr[INET6_ADDRSTRLEN];
    char nmskStr[INET6_ADDRSTRLEN];
    char servStr[NI_MAXSERV];

    UPnPsdk::CNetadapter nadObj;
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
        if (saddrObj.netaddr() == "[::1]") {
            ASSERT_GT(nadObj.index(), 0)
                << "index should be greater 0 for \"[::1]\"";
            EXPECT_EQ(nadObj.bitmask(), 128);
        }
        // TODO: Loopback addresses are not limited to the 127.0.0.0/8 block.
        if (saddrObj.sin.sin_family == AF_INET &&
            ntohl(saddrObj.sin.sin_addr.s_addr) == 2130706433) { // "127.0.0.1"
            ASSERT_GT(nadObj.index(), 0)
                << "index should be greater 0 for \"127.0.0.1\"";
            EXPECT_EQ(nadObj.bitmask(), 8);
        }
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

TEST(NetadapterTestSuite, find_first_adapters_info) {
    UPnPsdk::CNetadapter nadaptObj;
    ASSERT_NO_THROW(nadaptObj.get_first());

    // I do not expect index 1 to be the loopback adapter. It can be any ip
    // address 127.0.0.1 to 127.255.255.255.
    EXPECT_TRUE(nadaptObj.find_first(1));
    EXPECT_EQ(nadaptObj.index(), 1);
    EXPECT_NE(nadaptObj.name(), "");
    EXPECT_NE(nadaptObj.bitmask(), 0);
    nadaptObj.sockaddr(saddrObj);
    EXPECT_NE(saddrObj.netaddrp(), ":0");
    nadaptObj.socknetmask(saddrObj);
    EXPECT_NE(saddrObj.netaddr(), "");

    EXPECT_TRUE(nadaptObj.find_first("loopback"));
    EXPECT_NE(nadaptObj.index(), 0);
    EXPECT_NE(nadaptObj.name(), "");
    EXPECT_NE(nadaptObj.bitmask(), 0);
    nadaptObj.sockaddr(saddrObj);
    EXPECT_THAT(saddrObj.netaddrp(), AnyOf("[::1]:0", "127.0.0.1:0"));
    nadaptObj.socknetmask(saddrObj);
    EXPECT_THAT(
        saddrObj.netaddr(),
        AnyOf("[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]", "255.0.0.0"));

    ASSERT_TRUE(nadaptObj.find_first("::1"));
    EXPECT_EQ(nadaptObj.index(), 1);
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
    EXPECT_TRUE(nadaptObj.find_first(""));
    EXPECT_NE(nadaptObj.index(), 0);
    EXPECT_NE(nadaptObj.name(), "");
    EXPECT_NE(nadaptObj.bitmask(), 0);
    nadaptObj.sockaddr(saddrObj);
    EXPECT_NE(saddrObj.netaddrp(), ":0");
    nadaptObj.socknetmask(saddrObj);
    EXPECT_NE(saddrObj.netaddr(), "");
}

TEST(NetadapterTestSuite, find_loopback_adapter_info) {
    UPnPsdk::CNetadapter nadaptObj;
    ASSERT_NO_THROW(nadaptObj.get_first());

    EXPECT_TRUE(nadaptObj.find_first("loopback"));
    EXPECT_NE(nadaptObj.index(), 0);
    EXPECT_NE(nadaptObj.name(), "");
    EXPECT_NE(nadaptObj.bitmask(), 0);
    nadaptObj.sockaddr(saddrObj);
    EXPECT_THAT(saddrObj.netaddrp(), AnyOf("[::1]:0", "127.0.0.1:0"));
    nadaptObj.socknetmask(saddrObj);
    EXPECT_THAT(
        saddrObj.netaddr(),
        AnyOf("[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]", "255.0.0.0"));

    EXPECT_TRUE(nadaptObj.find_next());
    EXPECT_NE(nadaptObj.index(), 0);
    EXPECT_NE(nadaptObj.name(), "");
    EXPECT_NE(nadaptObj.bitmask(), 0);
    nadaptObj.sockaddr(saddrObj);
    EXPECT_THAT(saddrObj.netaddrp(), AnyOf("[::1]:0", "127.0.0.1:0"));
    nadaptObj.socknetmask(saddrObj);
    EXPECT_THAT(
        saddrObj.netaddr(),
        AnyOf("[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]", "255.0.0.0"));
}

class CNetadapterMock : public UPnPsdk::INetadapter {
  public:
    CNetadapterMock() = default;
    virtual ~CNetadapterMock() override = default;
    MOCK_METHOD(void, get_first, (), (override));
    MOCK_METHOD(bool, get_next, (), (override));
    MOCK_METHOD(unsigned int, index, (), (const, override));
    MOCK_METHOD(std::string, name, (), (const, override));
    MOCK_METHOD(void, sockaddr, (UPnPsdk::SSockaddr&), (const, override));
    MOCK_METHOD(void, socknetmask, (UPnPsdk::SSockaddr&), (const, override));
    MOCK_METHOD(unsigned int, bitmask, (), (const, override));
    MOCK_METHOD(void, reset, (), (noexcept, override));
};

TEST(NetadapterTestSuite, mock_netadapter_successful) {
    // Create mocking di-service object and get the smart pointer to it.
    auto na_mockPtr = std::make_shared<CNetadapterMock>();

    EXPECT_CALL(*na_mockPtr, get_first()).Times(1);

    UPnPsdk::CNetadapter naObj(na_mockPtr);
    naObj.get_first();
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
