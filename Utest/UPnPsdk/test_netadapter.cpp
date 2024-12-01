// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-11-23

#ifdef _WIN32
#include <UPnPsdk/src/net/netadapter_win32.cpp>
#include <UPnPsdk/sockaddr.hpp>
#endif

#include <UPnPsdk/global.hpp>
#include <UPnPsdk/netadapter.hpp>
#include <utest/utest.hpp>


namespace utest {

using ::testing::AnyOf;
using ::testing::HasSubstr;


UPnPsdk::SSockaddr saddrObj;
UPnPsdk::SSockaddr snmskObj;

TEST(NetadapterTestSuite, get_adapters_info_successful) {
    char addrStr[INET6_ADDRSTRLEN];
    char nmskStr[INET6_ADDRSTRLEN];
    char servStr[NI_MAXSERV];

    UPnPsdk::CNetadapter netadapterObj;
    UPnPsdk::INetadapter& nadObj{netadapterObj};

    ASSERT_NO_THROW(nadObj.load());

    do {
        ASSERT_FALSE(nadObj.name().empty());
        saddrObj = nadObj.sockaddr();
        ASSERT_THAT(saddrObj.ss.ss_family, AnyOf(AF_INET6, AF_INET));
        // Check valid ip address
        ASSERT_EQ(::getnameinfo(&saddrObj.sa, sizeof(saddrObj.ss), addrStr,
                                sizeof(addrStr), servStr, sizeof(servStr),
                                NI_NUMERICHOST),
                  0);
        snmskObj = nadObj.socknetmask();
        // Check valid netmask
        ASSERT_EQ(::getnameinfo(&snmskObj.sa, sizeof(snmskObj.ss), nmskStr,
                                sizeof(nmskStr), nullptr, 0, NI_NUMERICHOST),
                  0);
#if 0
        // To show resolved iface names set first NI_NUMERICHOST above to 0.
        std::cout << "DEBUG: \"" << nadObj.name() << "\" address = " << addrStr
                  << "(" << saddrObj.netaddr()
                  << "), netmask = " << snmskObj.netaddr()
                  << ", service = " << servStr << '\n';
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

#ifdef _WIN32
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
    UPnPsdk::bitnum_to_netmask(AF_INET6, 128, saddrObj);
    EXPECT_EQ(captureObj.str(), "");

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

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
