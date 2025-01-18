// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-01-29

#ifdef _MSC_VER
#include <UPnPsdk/src/net/netadapter_win32.cpp>
#include <UPnPsdk/sockaddr.hpp>
#endif

#include <UPnPsdk/netadapter.hpp>
#include <utest/utest.hpp>


namespace utest {

using ::testing::AnyOf;
using ::testing::HasSubstr;


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
        if (saddrObj.sin6.sin6_family == AF_INET6 &&
            saddrObj.sin6.sin6_addr.s6_addr[15] == 1) {
            ASSERT_GT(nadObj.index(), 0)
                << "index should be greater 0 for \"[::1]\"";
        }
        // TODO: Loopback addresses are not limited to the 127.0.0.0/8 block.
        if (saddrObj.sin.sin_family == AF_INET &&
            ntohl(saddrObj.sin.sin_addr.s_addr) == 2130706433) { // "127.0.0.1"
            ASSERT_GT(nadObj.index(), 0)
                << "index should be greater 0 for \"127.0.0.1\"";
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
// Get Subnet mask from prefix bit number, first version with two nested loops
// working on 128 bits. I have made a more performant version working on bytes
// but won't discard this version.
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

#ifdef MSC_VER
class BitnumToNetmaskTest
    : public ::testing::TestWithParam<std::tuple<
          //    family,      bitnum,        netmask
          const sa_family_t, const uint8_t, std::string>> {};

TEST_P(BitnumToNetmaskTest, set_family_and_bitnum) {
    // Get parameter
    const std::tuple params = GetParam();
    const sa_family_t family = std::get<0>(params);
    const uint8_t bitnum = std::get<1>(params);
    const std::string netmask = std::get<2>(params);

    UPnPsdk::bitnum_to_netmask(family, bitnum, saddrObj);
    EXPECT_EQ(saddrObj.netaddr(), netmask);
    if (family == AF_INET6 || family == AF_INET || family == AF_UNSPEC)
        EXPECT_EQ(saddrObj.ss.ss_family, family);
    else
        EXPECT_EQ(saddrObj.ss.ss_family, AF_UNSPEC);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(BitnumToNetmask, BitnumToNetmaskTest, ::testing::Values(
    std::make_tuple(AF_INET6, 64, "[ffff:ffff:ffff:ffff::]"),
    std::make_tuple(AF_INET6, -1, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]"),
    std::make_tuple(AF_INET6, 0, "[::]"),
    std::make_tuple(AF_INET6, 1, "[8000::]"),
    std::make_tuple(AF_INET6, 2, "[c000::]"),
    std::make_tuple(AF_INET6, 126, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffc]"),
    std::make_tuple(AF_INET6, 127, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffe]"),
    std::make_tuple(AF_INET6, 128, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]"),
    std::make_tuple(AF_INET6, 129, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]"),
    std::make_tuple(AF_INET6, 255, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]"),
    // Here we have uint8_t overrun.
    std::make_tuple(AF_INET6, 256, "[::]"),
    std::make_tuple(AF_INET6, 257, "[8000::]"),
    std::make_tuple(AF_INET6, 511, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]"),
    std::make_tuple(AF_INET6, 512, "[::]"),
    std::make_tuple(AF_INET6, 513, "[8000::]"),
    std::make_tuple(AF_INET, 24, "255.255.255.0"),
    std::make_tuple(AF_INET, -1, "255.255.255.255"), // 16
    std::make_tuple(AF_INET, 0, "255.255.255.255"),
    std::make_tuple(AF_INET, 1, "128.0.0.0"),
    std::make_tuple(AF_INET, 2, "192.0.0.0"),
    std::make_tuple(AF_INET, 30, "255.255.255.252"),
    std::make_tuple(AF_INET, 31, "255.255.255.254"),
    std::make_tuple(AF_INET, 32, "255.255.255.255"),
    std::make_tuple(AF_INET, 33, "255.255.255.255"),
    std::make_tuple(AF_INET, 64, "255.255.255.255"),
    std::make_tuple(AF_INET, 65, "255.255.255.255"),
    // Here we have uint8_t overrun.
    std::make_tuple(AF_INET, 256, "255.255.255.255"),
    std::make_tuple(AF_INET, 257, "128.0.0.0"),
    std::make_tuple(AF_UNSPEC, 64, ""),
    std::make_tuple(4711, 64, ""),
    std::make_tuple(AF_UNSPEC, 24, ""),
    std::make_tuple(4712, 24, "")
));
// clang-format on

TEST(NetadapterTestSuite, check_bitnum_to_netmask_error_messages) {
    // Capture output to stderr
    CaptureStdOutErr captureObj(UPnPsdk::log_fileno);

    // Test Unit AF_INET6
    captureObj.start();
    UPnPsdk::bitnum_to_netmask(AF_INET6, 129, saddrObj);
    std::cout << captureObj.str();
    EXPECT_THAT(captureObj.str(), HasSubstr("] CRITICAL MSG1124: "));

    captureObj.start();
    UPnPsdk::bitnum_to_netmask(AF_INET6, 255, saddrObj);
    std::cout << captureObj.str();
    EXPECT_THAT(captureObj.str(), HasSubstr("] CRITICAL MSG1124: "));

    // Test Unit AF_INET
    captureObj.start();
    UPnPsdk::bitnum_to_netmask(AF_INET, 32, saddrObj);
    EXPECT_EQ(captureObj.str(), "");

    captureObj.start();
    UPnPsdk::bitnum_to_netmask(AF_INET, 33, saddrObj);
    std::cout << captureObj.str();
    EXPECT_THAT(captureObj.str(), HasSubstr("] CRITICAL MSG1125: "));

    captureObj.start();
    UPnPsdk::bitnum_to_netmask(AF_INET, 255, saddrObj);
    std::cout << captureObj.str();
    EXPECT_THAT(captureObj.str(), HasSubstr("] CRITICAL MSG1125: "));

    captureObj.start();
    UPnPsdk::bitnum_to_netmask(AF_UNIX, 64, saddrObj);
    std::cout << captureObj.str();
    EXPECT_THAT(captureObj.str(), HasSubstr("] CRITICAL MSG1126: "));
}
#endif

TEST(NetadapterTestSuite, find_adapters_info) {
    UPnPsdk::CNetadapter nadaptObj;
    ASSERT_NO_THROW(nadaptObj.get_first());

    // Index 1 is always the loopback device on all platforms but it has
    // different names.
    EXPECT_TRUE(nadaptObj.find_first(1));
    EXPECT_EQ(nadaptObj.index(), 1);
    std::string nad_name = nadaptObj.name();

    EXPECT_TRUE(nadaptObj.find_first(nad_name));
    nadaptObj.sockaddr(saddrObj);
    EXPECT_THAT(saddrObj.netaddrp(), AnyOf("[::1]:0", "127.0.0.1:0"));

    ASSERT_TRUE(nadaptObj.find_first("::1"));
    EXPECT_EQ(nadaptObj.index(), 1);
    nadaptObj.socknetmask(saddrObj);
    EXPECT_EQ(saddrObj.netaddr(), "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]");

    ASSERT_TRUE(nadaptObj.find_first("127.0.0.1"));
    EXPECT_EQ(nadaptObj.index(), 1);
    nadaptObj.socknetmask(saddrObj);
    EXPECT_EQ(saddrObj.netaddr(), "255.0.0.0");

    EXPECT_FALSE(nadaptObj.find_first(0));
    EXPECT_EQ(nadaptObj.index(), 0);
    EXPECT_EQ(nadaptObj.name(), "");
    nadaptObj.sockaddr(saddrObj);
    EXPECT_EQ(saddrObj.netaddrp(), ":0");
    nadaptObj.socknetmask(saddrObj);
    EXPECT_EQ(saddrObj.netaddr(), "");

    EXPECT_FALSE(nadaptObj.find_first(~0u - 2));
    EXPECT_EQ(nadaptObj.index(), 0);
    EXPECT_EQ(nadaptObj.name(), "");
    nadaptObj.sockaddr(saddrObj);
    EXPECT_EQ(saddrObj.netaddrp(), ":0");
    nadaptObj.socknetmask(saddrObj);
    EXPECT_EQ(saddrObj.netaddr(), "");

    EXPECT_FALSE(nadaptObj.find_first(""));
    EXPECT_EQ(nadaptObj.index(), 0);
    EXPECT_EQ(nadaptObj.name(), "");
    nadaptObj.sockaddr(saddrObj);
    EXPECT_EQ(saddrObj.netaddrp(), ":0");
    nadaptObj.socknetmask(saddrObj);
    EXPECT_EQ(saddrObj.netaddr(), "");
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
