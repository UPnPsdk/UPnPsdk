// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-08-14

#include <UPnPsdk/sockaddr.hpp>
#include <UPnPsdk/netadapter.hpp>

#include <utest/utest.hpp>

namespace utest {

using testing::EndsWith;

using ADDRS = UPnPsdk::CNetadapter::ADDRS;
using UPnPsdk::CNetadapter;
using UPnPsdk::is_unum_str;
using UPnPsdk::SInaddr;
using UPnPsdk::sockaddr_t;
using UPnPsdk::SSockaddr;
using UPnPsdk::to_port;

// General storage for temporary socket address evaluation.
SSockaddr saObj;

// To compare if socket address is empty.
constexpr in6_addr sin6_addr_empty{};

// Unspecified socket address for comparing.
::sockaddr_storage ss_unspec{};

#if 0
TEST(SockaddrTestSuite, pointer_cast_to_binary_ipv6_address) {
    // Just to learn how it works.
    sockaddr_in6 sin6{};
    EXPECT_EQ(sin6.sin6_addr.s6_addr[0], 0);
    EXPECT_EQ(sin6.sin6_addr.s6_addr[8], 0);

    uint64_t* s6addr = reinterpret_cast<uint64_t*>(sin6.sin6_addr.s6_addr);
    // same with & ...................................V
    // uint64_t* s6addr = reinterpret_cast<uint64_t*>(&sin6.sin6_addr.s6_addr);
    s6addr[0] = 1;
    s6addr[1] = 2;
    EXPECT_EQ(sin6.sin6_addr.s6_addr[0], 1); // set
    EXPECT_EQ(sin6.sin6_addr.s6_addr[1], 0);
    EXPECT_EQ(sin6.sin6_addr.s6_addr[2], 0);
    EXPECT_EQ(sin6.sin6_addr.s6_addr[3], 0);
    EXPECT_EQ(sin6.sin6_addr.s6_addr[4], 0);
    EXPECT_EQ(sin6.sin6_addr.s6_addr[5], 0);
    EXPECT_EQ(sin6.sin6_addr.s6_addr[6], 0);
    EXPECT_EQ(sin6.sin6_addr.s6_addr[7], 0);
    EXPECT_EQ(sin6.sin6_addr.s6_addr[8], 2); // set
    EXPECT_EQ(sin6.sin6_addr.s6_addr[9], 0);
}
#endif

#if 0
// Test what is accepted by inet_pton() for information. I do not need to always
// test it.
TEST(SockaddrTestSuite, test_pton) {
    in_addr buf[sizeof(in_addr)];
    in6_addr buf6[sizeof(in6_addr)];

    EXPECT_EQ(inet_pton(AF_INET, "192.168.1.2", &buf), 1);
    // Following are invalid
    EXPECT_EQ(inet_pton(AF_INET, "192.168.1.2:50002", &buf), 0);
    EXPECT_EQ(inet_pton(AF_INET, "192.168.1.2/24", &buf), 0);
    EXPECT_EQ(inet_pton(AF_INET, "192.168.1.2/24:50002", &buf), 0);

    EXPECT_EQ(inet_pton(AF_INET6, "2001:db8::1", &buf6), 1);
    // Following may be invalid
    EXPECT_EQ(inet_pton(AF_INET6, "2001:db8::1/64", &buf6), 0);
#ifdef __APPLE__ // valid
    EXPECT_EQ(inet_pton(AF_INET6, "2001:db8::1%eth0", &buf6), 1);
#else
    EXPECT_EQ(inet_pton(AF_INET6, "2001:db8::1%eth0", &buf6), 0);
#endif
    EXPECT_EQ(inet_pton(AF_INET6, "[2001:db8::1]", &buf6), 0);
    EXPECT_EQ(inet_pton(AF_INET6, "[2001:db8::1]/64", &buf6), 0);
    EXPECT_EQ(inet_pton(AF_INET6, "[2001:db8::1]%eth0", &buf6), 0);
    EXPECT_EQ(inet_pton(AF_INET6, "[2001:db8::1]:50010", &buf6), 0);

    EXPECT_EQ(inet_pton(AF_INET6, "fe80::1", &buf6), 1);
    // Following may be invalid
#ifdef __APPLE__ // valid
    EXPECT_EQ(inet_pton(AF_INET6, "fe80::1%252", &buf6), 1);
#else
    EXPECT_EQ(inet_pton(AF_INET6, "fe80::1%252", &buf6), 0);
#endif
}
#endif

// Veify macro/function IN6_IS_ADDR_GLOBAL
// ---------------------------------------
// For GCC compiler the macros can be found in 'netinet/in.h'.
// For MSVC compiler the inline functions can be found in 'ws2ipdef.h'.
//
// Global addressing is all in the [2000::]/3 range
//     current range is [2000::] to [3fff:ffff:ffff:ffff:ffff:ffff:ffff:ffff],
//     that could be expanded in the future
// link-local addressing is all in the [fe80::]/64 range from [fe80::]/10.
// (Deprecated ULA is in the [fc00::]/7 range)
// Multicast is in the [ff00::]/8 range
// REF:_[How_to_detect_global_vs._link_local_IPv6_address](https://stackoverflow.com/questions/66324779/how-to-detect-global-vs-link-local-ipv6-address#comment117257243_66324779)
//
// Microsoft specifies it more relax with function (not macro) in 'ws2ipdef.h'
// as follows:
#if 0
IN6_IS_ADDR_GLOBAL(CONST IN6_ADDR* a) {
    // Check the format prefix and exclude addresses whose high 4 bits are all
    // zero or all one. This is a cheap way of excluding v4-compatible,
    // v4-mapped, loopback, multicast, link-local, site-local.
    ULONG High = (a->s6_bytes[0] & 0xf0);
    return (BOOLEAN)((High != 0) && (High != 0xf0));
}
#endif
// If IN6_IS_ADDR_GLOBAL is not defined I use the more restricted macro.
TEST(SockaddrTestSuite, verify_in6_is_addr_global) {
    in6_addr sin6_addr;

    // clang-format off
    // No Global Unicast Address (different on win32)
    ASSERT_EQ(inet_pton(AF_INET6, "1fff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",
                        &sin6_addr), 1);
#ifdef _MSC_VER
    EXPECT_TRUE(::IN6_IS_ADDR_GLOBAL(&sin6_addr)); // System function
    EXPECT_FALSE(UPnPsdk::IN6_IS_ADDR_GLOBAL2(&sin6_addr)); // Fixed version
#else
    // System function not available.
    EXPECT_FALSE(UPnPsdk::IN6_IS_ADDR_GLOBAL2(&sin6_addr)); // Fixed version
#endif

    // First Global Unicast Address (not first on win32)
    ASSERT_EQ(inet_pton(AF_INET6, "2000::", //
                        &sin6_addr), 1);
#ifdef _MSC_VER
    EXPECT_TRUE(::IN6_IS_ADDR_GLOBAL(&sin6_addr)); // System function
    EXPECT_TRUE(UPnPsdk::IN6_IS_ADDR_GLOBAL2(&sin6_addr)); // Fixed version
#else
    // System function not available.
    EXPECT_TRUE(UPnPsdk::IN6_IS_ADDR_GLOBAL2(&sin6_addr)); // Fixed version
#endif

    // Documentation- and test-address
    ASSERT_EQ(inet_pton(AF_INET6, "2001:db8::1", //
                        &sin6_addr), 1);
#ifdef _MSC_VER
    EXPECT_TRUE(::IN6_IS_ADDR_GLOBAL(&sin6_addr)); // System function
    EXPECT_TRUE(UPnPsdk::IN6_IS_ADDR_GLOBAL2(&sin6_addr)); // Fixed version
#else
    // System function not available.
    EXPECT_TRUE(UPnPsdk::IN6_IS_ADDR_GLOBAL2(&sin6_addr)); // Fixed version
#endif

    // Last Global Unicast Address (not last on win32)
    ASSERT_EQ(inet_pton(AF_INET6, "3fff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", //
                        &sin6_addr), 1);
#ifdef _MSC_VER
    EXPECT_TRUE(::IN6_IS_ADDR_GLOBAL(&sin6_addr)); // System function
    EXPECT_TRUE(UPnPsdk::IN6_IS_ADDR_GLOBAL2(&sin6_addr)); // Fixed version
#else
    // System function not available.
    EXPECT_TRUE(UPnPsdk::IN6_IS_ADDR_GLOBAL2(&sin6_addr)); // Fixed version
#endif

    // No Global Unicast Address (different on win32)
    ASSERT_EQ(inet_pton(AF_INET6, "4000::", //
                        &sin6_addr), 1);
#ifdef _MSC_VER
    EXPECT_TRUE(::IN6_IS_ADDR_GLOBAL(&sin6_addr)); // System function
    EXPECT_FALSE(UPnPsdk::IN6_IS_ADDR_GLOBAL2(&sin6_addr)); // Fixed version
#else
    // System function not available.
    EXPECT_FALSE(UPnPsdk::IN6_IS_ADDR_GLOBAL2(&sin6_addr)); // Fixed version
#endif
}

TEST(SockaddrTestSuite, verify_in6_is_addr_other_addresses) {
    in6_addr sin6_addr;

    // clang-format off
    // starting with 00 is the reserved address block

    // Unspecified Address, belongs to the reserved address block
    ASSERT_EQ(inet_pton(AF_INET6, "::", //
                        &sin6_addr), 1);
    EXPECT_TRUE(IN6_IS_ADDR_UNSPECIFIED(&sin6_addr));

    // loopback address, belongs to the reserved address block
    ASSERT_EQ(inet_pton(AF_INET6, "::1", //
                        &sin6_addr), 1);
    EXPECT_TRUE(IN6_IS_ADDR_LOOPBACK(&sin6_addr));

    // v4-mapped address, does it belong to the reserved address block?
    // v4-mapped address min
    ASSERT_EQ(inet_pton(AF_INET6, "::ffff:0.0.0.0", //
                        &sin6_addr), 1);
    EXPECT_TRUE(IN6_IS_ADDR_V4MAPPED(&sin6_addr));

    // Not v4-mapped address
    ASSERT_EQ(inet_pton(AF_INET6, "::1:ffff:0.0.0.0", //
                        &sin6_addr), 1);
    EXPECT_FALSE(IN6_IS_ADDR_V4MAPPED(&sin6_addr));

    // v4-mapped address max
    ASSERT_EQ(inet_pton(AF_INET6, "::ffff:255.255.255.255", //
                        &sin6_addr), 1);
    EXPECT_TRUE(IN6_IS_ADDR_V4MAPPED(&sin6_addr));

    // IPv4-compatible embedded IPv6 address (IN6_IS_ADDR_V4COMPAT), belongs to
    // the reserved address block. Deprecated since february 2006 and not
    // supported by this SDK.
    ASSERT_EQ(inet_pton(AF_INET6, "::101.45.75.219", //
                        &sin6_addr), 1);
    // clang-format on
}

TEST(SockaddrTestSuite, verify_in6_is_addr_linklocal) {
    in6_addr sin6_addr;

    // clang-format off
    // Link-local address with subnet is not valid, but accepted by default
    // system macro on linux platforms.
    ASSERT_EQ(inet_pton(AF_INET6, "fe80:1::", //
                        &sin6_addr), 1);
    EXPECT_TRUE(IN6_IS_ADDR_LINKLOCAL(&sin6_addr)); // System function Wrong!
    EXPECT_FALSE(UPnPsdk::IN6_IS_ADDR_LINKLOCAL2(&sin6_addr)); // Fixed version

    // Only left most bit of network prefix set.
    ASSERT_EQ(inet_pton(AF_INET6, "fea0::", //
                        &sin6_addr), 1);
    EXPECT_TRUE(IN6_IS_ADDR_LINKLOCAL(&sin6_addr)); // System function Wrong!
    EXPECT_FALSE(UPnPsdk::IN6_IS_ADDR_LINKLOCAL2(&sin6_addr)); // Fixed version

    // Only right most bit of network prefix set.
    ASSERT_EQ(inet_pton(AF_INET6, "fe80:0:0:1::", //
                        &sin6_addr), 1);
    EXPECT_TRUE(IN6_IS_ADDR_LINKLOCAL(&sin6_addr)); // System function Wrong!
    EXPECT_FALSE(UPnPsdk::IN6_IS_ADDR_LINKLOCAL2(&sin6_addr)); // Fixed version

    // First link-local address
    ASSERT_EQ(inet_pton(AF_INET6, "fe80::", //
                        &sin6_addr), 1);
    EXPECT_TRUE(IN6_IS_ADDR_LINKLOCAL(&sin6_addr));
    EXPECT_TRUE(UPnPsdk::IN6_IS_ADDR_LINKLOCAL2(&sin6_addr));

    // Last link-local address
    ASSERT_EQ(inet_pton(AF_INET6, "fe80::ffff:ffff:ffff:ffff", //
                        &sin6_addr), 1);
    EXPECT_TRUE(IN6_IS_ADDR_LINKLOCAL(&sin6_addr));
    EXPECT_TRUE(UPnPsdk::IN6_IS_ADDR_LINKLOCAL2(&sin6_addr));
    // clang-format on
}

TEST(SockaddrTestSuite, is_unum_str) {
    EXPECT_TRUE(is_unum_str("65535", 5));
    EXPECT_TRUE(is_unum_str("65535", 6));
    EXPECT_FALSE(is_unum_str("65535", 4));
    EXPECT_FALSE(is_unum_str("65535", 0));
    EXPECT_FALSE(is_unum_str("0x65535", 7));
    EXPECT_FALSE(is_unum_str("65535x", 5));
    EXPECT_FALSE(is_unum_str("65535x", 10));
    EXPECT_FALSE(is_unum_str("-65535", 6));
    EXPECT_TRUE(is_unum_str("0", 1));
    EXPECT_TRUE(is_unum_str("0", 2));
    EXPECT_FALSE(is_unum_str("0", 0));
    EXPECT_FALSE(is_unum_str("", 0));
    EXPECT_FALSE(is_unum_str("", 1));
}

TEST(SockaddrTestSuite, string_to_port) {
    in_port_t port{50000};

    EXPECT_EQ(to_port("123", nullptr), 0);
    EXPECT_EQ(port, 50000);
    EXPECT_EQ(to_port("123", &port), 0);
    EXPECT_EQ(port, 123);
    EXPECT_EQ(to_port("00000", &port), 0);
    EXPECT_EQ(port, 0);
    EXPECT_EQ(to_port("00456", &port), 0);
    EXPECT_EQ(port, 456);
    EXPECT_EQ(to_port("0", &port), 0);
    EXPECT_EQ(port, 0);
    EXPECT_EQ(to_port("65535", &port), 0);
    EXPECT_EQ(port, 65535);
    EXPECT_EQ(to_port("", &port), 0);
    EXPECT_EQ(port, 0);
    EXPECT_EQ(to_port("9", &port), 0);
    EXPECT_EQ(port, 9);
    EXPECT_EQ(to_port("000000", &port), -1);
    EXPECT_EQ(port, 9);
    EXPECT_EQ(to_port("65536", &port), 1);
    EXPECT_EQ(port, 9);
    EXPECT_EQ(to_port("-1", &port), -1);
    EXPECT_EQ(port, 9);
    EXPECT_EQ(to_port("123456", &port), 1);
    EXPECT_EQ(port, 9);
    EXPECT_EQ(to_port(" ", &port), -1);
    EXPECT_EQ(port, 9);
    EXPECT_EQ(to_port(" 123", &port), -1);
    EXPECT_EQ(port, 9);
    EXPECT_EQ(to_port("123 ", &port), -1);
    EXPECT_EQ(port, 9);
    EXPECT_EQ(to_port("123.4", &port), -1);
    EXPECT_EQ(port, 9);
    EXPECT_EQ(to_port(":1234", &port), -1);
    EXPECT_EQ(port, 9);
    EXPECT_EQ(to_port("12x34", &port), -1);
    EXPECT_EQ(port, 9);
}

TEST(SockaddrTestSuite, string_to_port_test_only) {
    EXPECT_EQ(to_port("X"), -1);
    EXPECT_EQ(to_port("-0"), -1);
    EXPECT_EQ(to_port("0"), 0);
    EXPECT_EQ(to_port("65535"), 0);
    EXPECT_EQ(to_port("5535"), 0);
    EXPECT_EQ(to_port("-5535"), -1);
    EXPECT_EQ(to_port("65536"), 1);
    EXPECT_EQ(to_port("99999"), 1);
    EXPECT_EQ(to_port("6553X"), -1);
    EXPECT_EQ(to_port("65535Y"), -1);
    EXPECT_EQ(to_port("http"), -1);
}

// clang-format off
class SplitAddrPortTest
    : public ::testing::TestWithParam<std::tuple<
    // IP address to split  IP address only   scope_id          port
       std::string_view,    std::string_view, std::string_view, std::string_view>> {};

TEST_P(SplitAddrPortTest, split_address_and_port) {
    // Get parameter
    const std::tuple params = GetParam();

    // Test Unit
    SInaddr iaObj(std::get<0>(params));
    EXPECT_EQ(iaObj.node, std::get<1>(params));
    EXPECT_EQ(iaObj.scope, std::get<2>(params));
    EXPECT_EQ(iaObj.service, std::get<3>(params));
}

INSTANTIATE_TEST_SUITE_P(SplitAddrPort, SplitAddrPortTest, ::testing::Values(
 /*00*/ std::make_tuple("[2001:db8::1]:50001", "2001:db8::1", "", "50001"),
        std::make_tuple("[2001:dB8::2]:", "2001:dB8::2", "", "0"),
        std::make_tuple("[2001:db8::2]", "2001:db8::2", "", ""),
        std::make_tuple(":0", "", "", "0"),
        std::make_tuple(":50002", "", "", "50002"),
        std::make_tuple("127.0.0.4:50003", "127.0.0.4", "", "50003"),
        std::make_tuple("127.0.0.5:", "127.0.0.5", "", "0"),
        std::make_tuple("127.0.0.6", "127.0.0.6", "", ""),
        std::make_tuple("0", "", "", "0"),
        std::make_tuple("50004", "", "", "50004"),
 /*10*/ std::make_tuple("500044", "500044", "", ""),
        std::make_tuple("2001:db8::7", "2001:db8::7", "", ""),
        std::make_tuple("example.COM:50005", "example.COM", "", "50005"),
        std::make_tuple("example.com:httPS", "example.com", "", "httPS"),
        std::make_tuple("example.com:", "example.com", "", "0"),
        std::make_tuple("example.com", "example.com", "", ""),
        std::make_tuple("example.com%123", "example.com", "123", ""),
        std::make_tuple("localhost:50006", "localhost", "", "50006"),
        std::make_tuple("localhost:https", "localhost", "", "https"),
        std::make_tuple("localhost:", "localhost", "", "0"),
 /*20*/ std::make_tuple("localhost", "localhost", "", ""),
        std::make_tuple("[localhost]", "[localhost]", "", ""),
        std::make_tuple("[localhost]:", "[localhost]", "", "0"),
        std::make_tuple("[localhost]:50007", "[localhost]", "", "50007"),
        std::make_tuple(":https", "", "", "https"),
        std::make_tuple("https", "https", "", ""),
        std::make_tuple("", "", "", ""),
        std::make_tuple("   ", "", "", ""),
        std::make_tuple("::", "::", "", ""),
        std::make_tuple("[::]", "::", "", ""),
 /*30*/ std::make_tuple("[::]:", "::", "", "0"),
        std::make_tuple("[::]:0", "::", "", "0"),
        std::make_tuple("::1", "::1", "", ""),
        std::make_tuple("[::1]", "::1", "", ""),
        std::make_tuple("[::1]:", "::1", "", "0"),
        std::make_tuple("[::1]:0", "::1", "", "0"),
        std::make_tuple("[::1].4", "[::1].4", "", ""),
        std::make_tuple("[::127.0.0.9]:50009", "::127.0.0.9", "", "50009"), // deprecated, not supported
        std::make_tuple("[::127.0.0.10]:", "::127.0.0.10", "", "0"), // deprecated, not supported
        std::make_tuple("[::127.0.0.11%47]", "::127.0.0.11", "47", ""), // deprecated, not supported
        std::make_tuple("[127.0.0.1]", "[127.0.0.1]", "", ""), // invalid ip-addr, interpreted as alpha-num
        std::make_tuple("[127.0.0.1%2]", "[127.0.0.1", "2]", ""), // invalid ip-addr, interpreted as alpha-num with scope_id
        std::make_tuple("[127.0.0.1%2]:50015", "[127.0.0.1", "2]", "50015"), // invalid ip-addr, interpreted as alpha-num with scope_id
 /*40*/ std::make_tuple("[::FFff:142.250.185.99]:50008", "::FFff:142.250.185.99", "", "50008"),
        std::make_tuple("[fe80::5053%]:50010", "fe80::5053", "", "50010"),
        std::make_tuple("[2001:db8::5054%513]:50011", "2001:db8::5054", "513", "50011"),
        std::make_tuple("[fe80::5055%2]:50012", "fe80::5055", "2", "50012"),
        std::make_tuple("[fe80::5056%scope]:50013", "fe80::5056", "", "50013"), // scope_id cannot be resolved to number
        std::make_tuple("example.com", "example.com", "", ""),
        std::make_tuple("example.com%", "example.com", "", ""),
        std::make_tuple("example.com%:", "example.com", "", "0"),
        std::make_tuple("example.com%382:", "example.com", "382", "0"),
        std::make_tuple("example.com%:https", "example.com", "", "https"),
        std::make_tuple("example.com:50014", "example.com", "", "50014")
));
// clang-format on


TEST(SockaddrTestSuite, inaddr_with_valid_scope_id) {
    CNetadapter nadObj;
    ASSERT_NO_THROW(nadObj.get_first());
    ASSERT_TRUE(nadObj.find_first(ADDRS::lla));

    SInaddr inaddr("[fe80::1%" + nadObj.name() + "]:50007");
    EXPECT_EQ(inaddr.node, "fe80::1");
    EXPECT_EQ(inaddr.scope, std::to_string(nadObj.index()));
    EXPECT_EQ(inaddr.service, "50007");
}


TEST(SockaddrTestSuite, sockaddr_empty) {
    saObj.clear();
    EXPECT_TRUE(saObj.empty());
    saObj.family = AF_UNSPEC;
    EXPECT_FALSE(saObj.empty());
    saObj.clear();
    saObj.sin6.sin6_addr.s6_addr[0] = 1;
    EXPECT_FALSE(saObj.empty());
    saObj.clear();
    saObj.sin6.sin6_addr.s6_addr[15] = 1;
    EXPECT_FALSE(saObj.empty());
    saObj.clear();
    saObj.sin6.sin6_scope_id = 1;
    EXPECT_FALSE(saObj.empty());
    saObj.clear();
    saObj.sin6.sin6_port = 1;
    EXPECT_FALSE(saObj.empty());
}

// Set sockaddr with trivial sockaddr_storage
// ------------------------------------------
TEST(SockaddrTestSuite, sockaddr_set_empty) {
    // Setting socket address to AF_UNSPEC, clears it.
    ::sockaddr_storage ss;
    ::memset(&ss, 0xAA, sizeof(ss));
    ss.ss_family = AF_UNSPEC;

    saObj = ss;
    EXPECT_EQ(saObj.ss.ss_family, AF_UNSPEC);
    EXPECT_EQ(::memcmp(&saObj.ss, &ss_unspec, sizeof(ss_unspec)), 0);
}

TEST(SockaddrTestSuite, sockaddr_unspec_ipv6) {
    sockaddr_t saddr{};
    saddr.ss.ss_family = AF_INET6;

    saObj = saddr.ss;
    EXPECT_EQ(saObj.ss.ss_family, AF_INET6);
    EXPECT_TRUE(IN6_IS_ADDR_UNSPECIFIED(&saObj.sin6.sin6_addr));
    EXPECT_EQ(saObj.sin6.sin6_scope_id, 0);
    EXPECT_EQ(saObj.sin6.sin6_port, 0);
    EXPECT_EQ(saObj.netaddr(), "[::]");
    EXPECT_EQ(saObj.netaddrp(), "[::]:0");
}

TEST(SockaddrTestSuite, sockaddr_unspec_ipv4) {
    sockaddr_t saddr{};
    saddr.ss.ss_family = AF_INET;

    saObj = saddr.ss;
    EXPECT_EQ(saObj.ss.ss_family, AF_INET);
    EXPECT_EQ(saObj.sin.sin_addr.s_addr, 0);
    EXPECT_EQ(saObj.sin.sin_port, 0);
    EXPECT_EQ(saObj.netaddr(), "0.0.0.0");
    EXPECT_EQ(saObj.netaddrp(), "0.0.0.0:0");
}

TEST(SockaddrTestSuite, sockaddr_unspec_ipv6_with_port) {
    sockaddr_t saddr{};
    saddr.ss.ss_family = AF_INET6;
    saddr.sin6.sin6_port = htons(UINT16_MAX);

    saObj = saddr.ss;
    EXPECT_EQ(saObj.ss.ss_family, AF_INET6);
    EXPECT_TRUE(IN6_IS_ADDR_UNSPECIFIED(&saObj.sin6.sin6_addr));
    EXPECT_EQ(saObj.sin6.sin6_scope_id, 0);
    EXPECT_EQ(saObj.sin6.sin6_port, UINT16_MAX);
    EXPECT_EQ(saObj.netaddr(), "[::]");
    EXPECT_EQ(saObj.netaddrp(), "[::]:65535");
}

TEST(SockaddrTestSuite, sockaddr_unspec_ipv4_with_port) {
    sockaddr_t saddr{};
    saddr.ss.ss_family = AF_INET;
    saddr.sin6.sin6_port = htons(UINT16_MAX);

    saObj = saddr.ss;
    EXPECT_EQ(saObj.ss.ss_family, AF_INET);
    EXPECT_EQ(saObj.sin.sin_addr.s_addr, 0);
    EXPECT_EQ(saObj.sin6.sin6_port, UINT16_MAX);
    EXPECT_EQ(saObj.netaddr(), "0.0.0.0");
    EXPECT_EQ(saObj.netaddrp(), "0.0.0.0:65535");
}

TEST(SockaddrTestSuite, sockaddr_loopback_ipv6) {
    sockaddr_t saddr{};
    saddr.ss.ss_family = AF_INET6;
    ASSERT_EQ(::inet_pton(saddr.ss.ss_family, "::1", &saddr.sin6.sin6_addr), 1);
    saddr.sin6.sin6_port = htons(UINT16_MAX);

    saObj = saddr.ss;
    EXPECT_EQ(saObj.ss.ss_family, AF_INET6);
    EXPECT_TRUE(IN6_IS_ADDR_LOOPBACK(&saObj.sin6.sin6_addr));
    EXPECT_EQ(saObj.sin6.sin6_scope_id, 0);
    EXPECT_EQ(saObj.sin6.sin6_port, UINT16_MAX);
    EXPECT_EQ(saObj.netaddr(), "[::1]");
    EXPECT_EQ(saObj.netaddrp(), "[::1]:65535");
}

TEST(SockaddrTestSuite, sockaddr_loopback_ipv4) {
    sockaddr_t saddr{};
    saddr.ss.ss_family = AF_INET;
    ASSERT_EQ(::inet_pton(saddr.ss.ss_family, "127.0.0.1", &saddr.sin.sin_addr),
              1);
    saddr.sin6.sin6_port = htons(UINT16_MAX);

    saObj = saddr.ss;
    EXPECT_EQ(saObj.ss.ss_family, AF_INET);
    // binary compare with "127.0.0.1"
    EXPECT_EQ(saObj.sin.sin_addr.s_addr, htonl(2130706433));
    EXPECT_EQ(saObj.sin.sin_port, UINT16_MAX);
    EXPECT_EQ(saObj.netaddr(), "127.0.0.1");
    EXPECT_EQ(saObj.netaddrp(), "127.0.0.1:65535");

    ASSERT_EQ(
        ::inet_pton(saddr.ss.ss_family, "127.255.255.255", &saddr.sin.sin_addr),
        1);
    saObj = saddr.ss;
    EXPECT_EQ(saObj.ss.ss_family, AF_INET);
    // binary compare with "127.255.255.255"
    EXPECT_EQ(saObj.sin.sin_addr.s_addr, htonl(2147483647));
    EXPECT_EQ(saObj.sin.sin_port, UINT16_MAX);
    EXPECT_EQ(saObj.netaddr(), "127.255.255.255");
    EXPECT_EQ(saObj.netaddrp(), "127.255.255.255:65535");
}

TEST(SockaddrTestSuite, sockaddr_lla_successful) {
    sockaddr_t saddr;
    ::memset(&saddr, 0xAA, sizeof(saddr));
    ASSERT_EQ(::inet_pton(AF_INET6, "fe80::db8:1", &saddr.sin6.sin6_addr), 1);
    saddr.ss.ss_family = AF_INET6;
    saddr.sin6.sin6_scope_id = 252;
    saddr.sin6.sin6_port = htons(51234);

    saObj = saddr.ss;
    EXPECT_EQ(saObj.family, AF_INET6);
    EXPECT_EQ(saObj.sin6.sin6_scope_id, 252);
    EXPECT_EQ(saObj.sin6.sin6_port, ntohs(51234));
    EXPECT_EQ(saObj.netaddr(), "[fe80::db8:1%252]");
    EXPECT_EQ(saObj.netaddrp(), "[fe80::db8:1%252]:51234");
}

TEST(SockaddrTestSuite, sockaddr_lla_with_subnet_fails) {
    sockaddr_t saddr;
    ::memset(&saddr, 0xAA, sizeof(saddr));
    ASSERT_EQ(::inet_pton(AF_INET6, "fe80:db8::1", &saddr.sin6.sin6_addr), 1);
    saddr.ss.ss_family = AF_INET6;
    saddr.sin6.sin6_scope_id = 252;
    saddr.sin6.sin6_port = htons(51234);

    saObj = saddr.ss;
    EXPECT_EQ(saObj.family, AF_UNSPEC);
    EXPECT_EQ(::memcmp(&saObj.ss, &ss_unspec, sizeof(ss_unspec)), 0);
}

TEST(SockaddrTestSuite, sockaddr_lla_without_scope_id_fails) {
    sockaddr_t saddr;
    ::memset(&saddr, 0xAA, sizeof(saddr));
    ASSERT_EQ(::inet_pton(AF_INET6, "fe80::1", &saddr.sin6.sin6_addr), 1);
    saddr.ss.ss_family = AF_INET6;
    saddr.sin6.sin6_scope_id = 0;
    saddr.sin6.sin6_port = htons(51234);

    saObj = saddr.ss;
    EXPECT_EQ(saObj.family, AF_UNSPEC);
    EXPECT_EQ(::memcmp(&saObj.ss, &ss_unspec, sizeof(ss_unspec)), 0);
}

TEST(SockaddrTestSuite, sockaddr_mcast_successful) {
    sockaddr_t saddr;
    ::memset(&saddr, 0xAA, sizeof(saddr));
    ASSERT_EQ(::inet_pton(AF_INET6, "ff00::1", &saddr.sin6.sin6_addr), 1);
    saddr.ss.ss_family = AF_INET6;
    saddr.sin6.sin6_scope_id = 252;
    saddr.sin6.sin6_port = htons(1900);

    saObj = saddr.ss;
    EXPECT_EQ(saObj.family, AF_INET6);
    EXPECT_EQ(saObj.sin6.sin6_scope_id, 252);
    EXPECT_EQ(saObj.sin6.sin6_port, ntohs(1900));
    EXPECT_EQ(saObj.netaddr(), "[ff00::1%252]");
    EXPECT_EQ(saObj.netaddrp(), "[ff00::1%252]:1900");
}

TEST(SockaddrTestSuite, sockaddr_mcast_without_scope_id_fails) {
    sockaddr_t saddr;
    ::memset(&saddr, 0xAA, sizeof(saddr));
    ASSERT_EQ(::inet_pton(AF_INET6, "ff00::1", &saddr.sin6.sin6_addr), 1);
    saddr.ss.ss_family = AF_INET6;
    saddr.sin6.sin6_scope_id = 0;
    saddr.sin6.sin6_port = htons(1900);

    saObj = saddr.ss;
    EXPECT_EQ(saObj.family, AF_UNSPEC);
    EXPECT_EQ(::memcmp(&saObj.ss, &ss_unspec, sizeof(ss_unspec)), 0);
}

TEST(SockaddrTestSuite, sockaddr_gua_successful) {
    sockaddr_t saddr;
    ::memset(&saddr, 0xAA, sizeof(saddr));
    ASSERT_EQ(::inet_pton(AF_INET6, "2001:db8::1", &saddr.sin6.sin6_addr), 1);
    saddr.ss.ss_family = AF_INET6;
    saddr.sin6.sin6_port = htons(65535);

    saObj = saddr.ss;
    EXPECT_EQ(saObj.ss.ss_family, AF_INET6);
    EXPECT_EQ(saObj.sin6.sin6_scope_id, 0); // Silently reset for gua.
    EXPECT_EQ(saObj.sin6.sin6_port, ntohs(65535));
    EXPECT_EQ(saObj.netaddr(), "[2001:db8::1]");
    EXPECT_EQ(saObj.netaddrp(), "[2001:db8::1]:65535");
}

TEST(SockaddrTestSuite, sockaddr_ipv4_successful) {
    sockaddr_t saddr;
    ::memset(&saddr, 0xAA, sizeof(saddr));
    ASSERT_EQ(::inet_pton(AF_INET, "192.168.5.6", &saddr.sin.sin_addr), 1);
    saddr.ss.ss_family = AF_INET;
    saddr.sin.sin_port = htons(65535);

    saObj = saddr.ss;
    EXPECT_EQ(saObj.ss.ss_family, AF_INET);
    EXPECT_EQ(saObj.sin.sin_port, ntohs(65535));
    EXPECT_EQ(saObj.netaddr(), "192.168.5.6");
    EXPECT_EQ(saObj.netaddrp(), "192.168.5.6:65535");
}

TEST(SockaddrTestSuite, sockaddr_unsupported_address_family) {
    sockaddr_t saddr;
    ::memset(&saddr, 0xAA, sizeof(saddr));
    saddr.ss.ss_family = AF_UNIX;

    saObj = saddr.ss;
    EXPECT_EQ(saObj.ss.ss_family, AF_UNSPEC);
    EXPECT_EQ(::memcmp(&saObj.ss, &ss_unspec, sizeof(ss_unspec)), 0);
}

TEST(SockaddrTestSuite, sockaddr_copy_and_assign) {
    SSockaddr saddr1;
    // saddr1 = "[2001:db8::1]:50001";
    saddr1.ss.ss_family = AF_INET6;
    ASSERT_EQ(
        ::inet_pton(saddr1.ss.ss_family, "2001:db8::1", &saddr1.sin6.sin6_addr),
        1);
    saddr1.sin6.sin6_port = 50001;

    // Test Unit copy and binary compare
    SSockaddr saddr2 = saddr1;
    EXPECT_TRUE(saddr1 == saddr2);

    saddr1.clear();
    // saddr1 = "192.168.251.252:50002";
    saddr1.ss.ss_family = AF_INET;
    ASSERT_EQ(::inet_pton(saddr1.ss.ss_family, "192.168.251.252",
                          &saddr1.sin.sin_addr),
              1);
    saddr1.sin.sin_port = 50002;

    // Test Unit assign and binary compare
    SSockaddr saddr3;
    saddr3 = saddr1;
    EXPECT_TRUE(saddr3 == saddr1);
}

TEST(SockaddrTestSuite, sockaddr_get_sizeof_current) {
    saObj.clear(); // sets default to AF_INET6
    EXPECT_EQ(saObj.sizeof_saddr(), sizeof(sockaddr_in6));
    saObj.ss.ss_family = AF_UNSPEC;
    EXPECT_EQ(saObj.sizeof_saddr(), sizeof(sockaddr_storage));
    saObj.ss.ss_family = AF_INET6;
    EXPECT_EQ(saObj.sizeof_saddr(), sizeof(sockaddr_in6));
    saObj.ss.ss_family = AF_INET;
    EXPECT_EQ(saObj.sizeof_saddr(), sizeof(sockaddr_in));
}


// Using SInaddr: how it works.
// ----------------------------
TEST(SockaddrTestSuite, inaddr_lla_successful) {
    SInaddr iaObj("[fe80::2%2]:50002");
    SSockaddr saddrObj;
    saddrObj = iaObj;
    ASSERT_EQ(saddrObj.family, AF_INET6);
    EXPECT_EQ(saddrObj.netaddrp(), "[fe80::2%2]:50002");

    saddrObj = SInaddr("[fe80::3%333]:50003");
    ASSERT_EQ(saddrObj.family, AF_INET6);
    EXPECT_EQ(saddrObj.netaddrp(), "[fe80::3%333]:50003");
}

TEST(SockaddrTestSuite, inaddr_gua_successful) {
    SSockaddr saddrObj;
    saddrObj = SInaddr("[2001:db8::4]:50004");
    ASSERT_EQ(saddrObj.family, AF_INET6);
    EXPECT_EQ(saddrObj.netaddrp(), "[2001:db8::4]:50004");
}

TEST(SockaddrTestSuite, inaddr_ip4_successful) {
    SSockaddr saddrObj;
    saddrObj = SInaddr("192.168.169.4:50004");
    ASSERT_EQ(saddrObj.family, AF_INET6);
    EXPECT_EQ(saddrObj.netaddrp(), "[::ffff:192.168.169.4]:50004");
}

TEST(SockaddrTestSuite, modify_ipv6_address_and_port_special_cases) {
    SSockaddr sa1Obj;

    // Setting nothing without internet address.
    sa1Obj = SInaddr("50010");
    EXPECT_EQ(sa1Obj.family, AF_UNSPEC);
    EXPECT_EQ(::memcmp(&sa1Obj.ss, &ss_unspec, sizeof(ss_unspec)), 0);
    EXPECT_EQ(sa1Obj.netaddrp(), ":0");

    // Set ip address and port.
    sa1Obj = SInaddr("[fe80::aaa%432]:51234");
    EXPECT_EQ(sa1Obj.family, AF_INET6);
    EXPECT_EQ(sa1Obj.netaddr(), "[fe80::aaa%432]");
    EXPECT_EQ(ntohs(sa1Obj.sin6.sin6_port), 51234);

    // Modify only ip address.
    sa1Obj = SInaddr("[fe80::bbb%252]");
    EXPECT_EQ(sa1Obj.family, AF_INET6);
    EXPECT_EQ(sa1Obj.netaddr(), "[fe80::bbb%252]");
    EXPECT_EQ(ntohs(sa1Obj.sin6.sin6_port), 0);

    // Modify only port.
    sa1Obj = SInaddr("61666");
    EXPECT_EQ(sa1Obj.family, AF_UNSPEC);
    EXPECT_EQ(::memcmp(&sa1Obj.ss, &ss_unspec, sizeof(ss_unspec)), 0);
    EXPECT_EQ(sa1Obj.netaddrp(), ":0");

    // Reset only port.
    sa1Obj = SInaddr(":");
    EXPECT_EQ(sa1Obj.family, AF_UNSPEC);
    EXPECT_EQ(::memcmp(&sa1Obj.ss, &ss_unspec, sizeof(ss_unspec)), 0);
    EXPECT_EQ(sa1Obj.netaddrp(), ":0");

    // Modify only port also with port separator.
    sa1Obj = SInaddr(":49999");
    EXPECT_EQ(sa1Obj.family, AF_UNSPEC);
    EXPECT_EQ(::memcmp(&sa1Obj.ss, &ss_unspec, sizeof(ss_unspec)), 0);
    EXPECT_EQ(sa1Obj.netaddrp(), ":0");

    // and reset only port with port separator.
    sa1Obj = SInaddr(":0");
    EXPECT_EQ(sa1Obj.family, AF_UNSPEC);
    EXPECT_EQ(::memcmp(&sa1Obj.ss, &ss_unspec, sizeof(ss_unspec)), 0);
    EXPECT_EQ(sa1Obj.netaddrp(), ":0");
}


// Test setting socket address with different internet address input
// -----------------------------------------------------------------
// clang-format off
class SetAddrPortTest : public ::testing::TestWithParam<
    //               inaddr,           addr_family, empty,       netaddr,                port
    std::tuple<const std::string_view, const int, const bool, const std::string_view, const in_port_t>> {};

TEST_P(SetAddrPortTest, set_address_and_port) {
    // Get parameter
    const std::tuple params = GetParam();

    const std::string_view inaddr_sv = std::get<0>(params);
    const int family = std::get<1>(params);
    const bool empty = std::get<2>(params);
    const std::string_view netaddr_sv = std::get<3>(params);
    const in_port_t port = std::get<4>(params);

    saObj = SInaddr(inaddr_sv);
    EXPECT_EQ(saObj.family, family);
    EXPECT_EQ(saObj.empty(), empty);
    EXPECT_EQ(saObj.netaddr(), netaddr_sv);
    EXPECT_EQ(ntohs(saObj.sin6.sin6_port), port);
}

INSTANTIATE_TEST_SUITE_P(SetAddrPort, SetAddrPortTest, ::testing::Values(
        // --- Essential for checking bind, see note next test
 /*00*/ std::make_tuple("", AF_UNSPEC, false, "", 0),
        std::make_tuple("::", AF_INET6, true, "[::]", 0),
        std::make_tuple("[::]", AF_INET6, true, "[::]", 0),
        std::make_tuple("[::]:", AF_INET6, true, "[::]", 0),
        std::make_tuple("[::]:0", AF_INET6, true, "[::]", 0),
        // ---
        std::make_tuple("[::]:50064", AF_INET6, false, "[::]", 50064),
        std::make_tuple("::1", AF_INET6, false, "[::1]", 0),
        std::make_tuple("[::1]", AF_INET6, false, "[::1]", 0),
        std::make_tuple("[::1]:", AF_INET6, false, "[::1]", 0),
        std::make_tuple("[::1]:0", AF_INET6, false, "[::1]", 0),
 /*10*/ std::make_tuple("[::1]:50065", AF_INET6, false, "[::1]", 50065),
        std::make_tuple("2001:db8::70", AF_INET6, false, "[2001:db8::70]", 0),
        std::make_tuple("[2001:db8::68]", AF_INET6, false, "[2001:db8::68]", 0),
        std::make_tuple("[2001:db8::67]:", AF_INET6, false, "[2001:db8::67]", 0),
        // --- Essential for checking bind, see note next test
        std::make_tuple("0.0.0.0", AF_INET6, false, "[::ffff:0.0.0.0]", 0),
        std::make_tuple("0.0.0.0:", AF_INET6, false, "[::ffff:0.0.0.0]", 0),
        std::make_tuple("0.0.0.0:0", AF_INET6, false, "[::ffff:0.0.0.0]", 0),
        // ---
        std::make_tuple("0.0.0.0:50068", AF_INET6, false, "[::ffff:0.0.0.0]", 50068),
        std::make_tuple("127.0.0.1", AF_INET6, false, "[::ffff:127.0.0.1]", 0),
        std::make_tuple("127.0.0.1:", AF_INET6, false, "[::ffff:127.0.0.1]", 0),
 /*20*/ std::make_tuple("127.0.0.1:50069", AF_INET6, false, "[::ffff:127.0.0.1]", 50069),
        std::make_tuple("[127.0.0.1]", AF_UNSPEC, false, "", 0),
        std::make_tuple("[127.0.0.1%2]", AF_UNSPEC, false, "", 0),
        std::make_tuple("[127.0.0.1%252]:50016", AF_UNSPEC, false, "", 0),
        std::make_tuple("192.168.33.34", AF_INET6, false, "[::ffff:192.168.33.34]", 0),
        std::make_tuple("192.168.33.35:", AF_INET6, false, "[::ffff:192.168.33.35]", 0),
        std::make_tuple("192.168.33.35:0", AF_INET6, false, "[::ffff:192.168.33.35]", 0),
        std::make_tuple("192.168.33.36:50067", AF_INET6, false, "[::ffff:192.168.33.36]", 50067),
        // --- Failing calls
        std::make_tuple(":65536", AF_UNSPEC, false, "", 0), // Number string with colon not in range 0..65535.
        std::make_tuple("65536", AF_UNSPEC, false, "", 0), // Number string not in range 0..65535.
 /*30*/ std::make_tuple("127.0.0.1:65536", AF_UNSPEC, false, "", 0), // Number string with port not in range 0..65535.
        std::make_tuple("[2001::db8::1]", AF_UNSPEC, false, "", 0), // Invalid address pattern.
        std::make_tuple("2001:db8::2]", AF_UNSPEC, false, "", 0), // Invalid address pattern.
        std::make_tuple("[2001:db8::3", AF_UNSPEC, false, "", 0), // Invalid address pattern.
        std::make_tuple("[2001:db8::5]50003", AF_UNSPEC, false, "", 0), // Missing port separator ':'.
        std::make_tuple("[2001:db8::35003]", AF_UNSPEC, false, "", 0), // Invalid netaddress.
        std::make_tuple("192.168.66.67.", AF_INET6, true, "[::]", 0), // Invalid netaddress, but ambiguous with DNS host name paatern.
        std::make_tuple("192.168.66.67z", AF_INET6, true, "[::]", 0), // Invalid netaddress, but ambiguous with DNS host name paatern.
        std::make_tuple("[192.168.66.68]", AF_UNSPEC, false, "", 0), // Invalid IPv4 address with brackets.
        std::make_tuple("[192.168.66.68]:50044", AF_UNSPEC, false, "", 0), // Invalid IPv4 address with brackets.
 /*40*/ std::make_tuple("garbage", AF_INET6, true, "[::]", 0), // Invalid netaddress but may be a DNS host name.
        std::make_tuple("example.com", AF_INET6, true, "[::]", 0), // Invalid netaddress but may be a DNS host name.
        std::make_tuple("other_example.com", AF_UNSPEC, false, "", 0), // Invalid underscore in DNS host name.
        std::make_tuple("example(invalid).com", AF_UNSPEC, false, "", 0), // Invalid char in DNS host name.
        // -- LLA addresses
        std::make_tuple("[fe80::2%2]:50045", AF_INET6, false, "[fe80::2%2]", 50045),
        std::make_tuple("[fe80::3%253]", AF_INET6, false, "[fe80::3%253]", 0),
        std::make_tuple("[fe80::4]", AF_UNSPEC, false, "", 0), // LLA without scope_id is invalid.
        std::make_tuple("[fe80:2::5%5]", AF_UNSPEC, false, "", 0), // LLA with subnet is invalid.
        std::make_tuple("[2001:db8::6%6]", AF_INET6, false, "[2001:db8::6]", 0), // Scope_id silently removed.
        // -- Some alphanumeric pattern
        std::make_tuple("example.com", AF_INET6, true, "[::]", 0), // Empty sockaddr to indicate name resolution.
 /*50*/ std::make_tuple("[2001:db8::2]:https", AF_INET6, true, "[::]", 0) // Empty sockaddr to indicate name resolution.
));
// clang-format on
TEST(SockaddrTestSuite, pattern_for_checking_bind) {
    // In addition to the marked pattern above these pattern are also essential
    // for portable checking if a socket is bound to a local interface with an
    // ip address by calling the system function '::bind()'.
    SSockaddr saddrObj;

    EXPECT_EQ(saddrObj.family, AF_INET6);
    EXPECT_EQ(saddrObj.netaddr(), "[::]");
    EXPECT_EQ(saddrObj.netaddrp(), "[::]:0");
    EXPECT_EQ(::memcmp(&saddrObj.sin6.sin6_addr, &sin6_addr_empty,
                       sizeof(sin6_addr_empty)),
              0);
    EXPECT_EQ(saddrObj.sin6.sin6_scope_id, 0);
    EXPECT_EQ(saddrObj.sin6.sin6_port, 0);

    saddrObj.family = AF_UNSPEC;
    EXPECT_EQ(saddrObj.netaddr(), "");
    EXPECT_EQ(saddrObj.netaddrp(), ":0");
    EXPECT_EQ(::memcmp(&saddrObj.ss, &ss_unspec, sizeof(ss_unspec)), 0);

    saddrObj.family = AF_INET;
    EXPECT_EQ(saddrObj.netaddr(), "0.0.0.0");
    EXPECT_EQ(saddrObj.netaddrp(), "0.0.0.0:0");
    EXPECT_EQ(saddrObj.sin.sin_addr.s_addr, 0);
    EXPECT_EQ(saddrObj.sin.sin_port, 0);
}

TEST(SockaddrTestSuite, output_netaddr_to_ostream) {
    SSockaddr sa1Obj;
    CaptureStdOutErr captoutObj(STDOUT_FILENO);

    // Test Unit output stream
    sa1Obj = SInaddr("[2001:db8::1]:61234");
    captoutObj.start();
    std::cout << std::flush << sa1Obj << std::endl;
    EXPECT_THAT(captoutObj.str(), EndsWith("[2001:db8::1]:61234\n"));

    CaptureStdOutErr capterrObj(STDERR_FILENO);

    sa1Obj = SInaddr("[fe80::1:2%2]:49999");
    capterrObj.start();
    std::cerr << sa1Obj << std::endl;
    EXPECT_THAT(capterrObj.str(), EndsWith("[fe80::1:2%2]:49999\n"));
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
