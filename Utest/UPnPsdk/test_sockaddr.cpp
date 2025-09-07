// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-09-12

#include <UPnPsdk/src/net/sockaddr.cpp>
#include <utest/utest.hpp>

namespace utest {

using ::testing::AnyOf;
using ::testing::EndsWith;
using ::testing::HasSubstr;
using ::testing::ThrowsMessage;

using ::UPnPsdk::g_dbug;
using ::UPnPsdk::sockaddrcmp;
using ::UPnPsdk::split_addr_port;
using ::UPnPsdk::SSockaddr;
using ::UPnPsdk::to_port;


// SSockaddr TestSuite
// ===========================
class SetAddrPortTest
    : public ::testing::TestWithParam<std::tuple<
          const std::string, const int, const std::string, const int>> {};

TEST_P(SetAddrPortTest, set_address_and_port) {
    // Get parameter
    const std::tuple params = GetParam();
    const std::string netaddr = std::get<0>(params);
    const int family = std::get<1>(params);
    const std::string addr_str = std::get<2>(params);
    const int port = std::get<3>(params);

    SSockaddr saddr;
    saddr = netaddr;
    EXPECT_EQ(saddr.ss.ss_family, family);
    EXPECT_EQ(saddr.netaddr(), addr_str);
    EXPECT_EQ(saddr.netaddrp(), addr_str + ":" + std::to_string(port));
    EXPECT_EQ(saddr.port(), port);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(SetAddrPort, SetAddrPortTest, ::testing::Values(
    // std::make_tuple("", AF_UNSPEC, "", 0), // special, single tested
    // --- Essential for checking bind, see note next test
    std::make_tuple("", AF_UNSPEC, "", 0),
    std::make_tuple(":0", AF_UNSPEC, "", 0),
    std::make_tuple("::", AF_INET6, "[::]", 0),
    std::make_tuple("[::]", AF_INET6, "[::]", 0),
    std::make_tuple("[::]:", AF_INET6, "[::]", 0),
    std::make_tuple("[::]:0", AF_INET6, "[::]", 0),
    // ---
    std::make_tuple("[::]:50064", AF_INET6, "[::]", 50064),
    std::make_tuple("::1", AF_INET6, "[::1]", 0),
    std::make_tuple("[::1]", AF_INET6, "[::1]", 0),
    std::make_tuple("[::1]:", AF_INET6, "[::1]", 0),
    std::make_tuple("[::1]:0", AF_INET6, "[::1]", 0),
    std::make_tuple("[::1]:50065", AF_INET6, "[::1]", 50065),
    std::make_tuple("2001:db8::70", AF_INET6, "[2001:db8::70]", 0),
    std::make_tuple("[2001:db8::68]", AF_INET6, "[2001:db8::68]", 0),
    std::make_tuple("[2001:db8::67]:", AF_INET6, "[2001:db8::67]", 0),
    // --- Essential for checking bind, see note next test
    std::make_tuple("0.0.0.0", AF_INET, "0.0.0.0", 0),
    std::make_tuple("0.0.0.0:", AF_INET, "0.0.0.0", 0),
    std::make_tuple("0.0.0.0:0", AF_INET, "0.0.0.0", 0),
    // ---
    std::make_tuple("0.0.0.0:50068", AF_INET, "0.0.0.0", 50068),
    std::make_tuple("127.0.0.1", AF_INET, "127.0.0.1", 0),
    std::make_tuple("127.0.0.1:", AF_INET, "127.0.0.1", 0),
    std::make_tuple("127.0.0.1:50069", AF_INET, "127.0.0.1", 50069),
    std::make_tuple("192.168.33.34", AF_INET, "192.168.33.34", 0),
    std::make_tuple("192.168.33.35:", AF_INET, "192.168.33.35", 0),
    std::make_tuple("192.168.33.35:0", AF_INET, "192.168.33.35", 0),
    std::make_tuple("192.168.33.36:50067", AF_INET, "192.168.33.36", 50067)
));
// clang-format on
//
TEST(SockaddrStorageTestSuite, pattern_for_checking_bind) {
    // In addition to the marked pattern above these pattern are also essential
    // for portable checking if a socket is bound to a local interface with an
    // ip address by calling the system function '::bind()'.
    SSockaddr saddr;

    EXPECT_EQ(saddr.ss.ss_family, AF_UNSPEC);
    EXPECT_EQ(saddr.netaddr(), "");
    EXPECT_EQ(saddr.netaddrp(), ":0");
    EXPECT_EQ(saddr.port(), 0);

    saddr.ss.ss_family = AF_INET6;
    EXPECT_EQ(saddr.netaddr(), "[::]");
    EXPECT_EQ(saddr.netaddrp(), "[::]:0");
    EXPECT_EQ(saddr.port(), 0);

    saddr.ss.ss_family = AF_INET;
    EXPECT_EQ(saddr.netaddr(), "0.0.0.0");
    EXPECT_EQ(saddr.netaddrp(), "0.0.0.0:0");
    EXPECT_EQ(saddr.port(), 0);
}

TEST(SockaddrStorageTestSuite, modify_address_and_port_successful) {
    SSockaddr saddr;

    saddr = 50010;
    EXPECT_EQ(saddr.ss.ss_family, AF_UNSPEC);
    EXPECT_EQ(saddr.netaddr(), "");
    EXPECT_EQ(saddr.netaddrp(), ":50010");
    EXPECT_EQ(saddr.port(), 50010);

    // Setting address and port in two steps
    saddr.ss.ss_family = AF_INET;
    saddr = "[2001:db8::3]:";
    EXPECT_EQ(saddr.ss.ss_family, AF_INET6);
    EXPECT_EQ(saddr.netaddr(), "[2001:db8::3]");
    EXPECT_EQ(saddr.netaddrp(), "[2001:db8::3]:0");
    EXPECT_EQ(saddr.port(), 0);
    EXPECT_EQ(saddr.port(), 0);

    saddr = "[2001:db8::1]:50001";
    EXPECT_EQ(saddr.ss.ss_family, AF_INET6);
    EXPECT_EQ(saddr.netaddr(), "[2001:db8::1]");
    EXPECT_EQ(saddr.netaddrp(), "[2001:db8::1]:50001");
    EXPECT_EQ(saddr.port(), 50001);

    saddr = 0;
    EXPECT_EQ(saddr.ss.ss_family, AF_INET6);
    EXPECT_EQ(saddr.netaddr(), "[2001:db8::1]");
    EXPECT_EQ(saddr.netaddrp(), "[2001:db8::1]:0");
    EXPECT_EQ(saddr.port(), 0);

    saddr = 65535;
    EXPECT_EQ(saddr.ss.ss_family, AF_INET6);
    EXPECT_EQ(saddr.netaddr(), "[2001:db8::1]");
    EXPECT_EQ(saddr.netaddrp(), "[2001:db8::1]:65535");
    EXPECT_EQ(saddr.port(), 65535);

    saddr = ":65535";
    EXPECT_EQ(saddr.ss.ss_family, AF_INET6);
    EXPECT_EQ(saddr.netaddr(), "[2001:db8::1]");
    EXPECT_EQ(saddr.netaddrp(), "[2001:db8::1]:65535");
    EXPECT_EQ(saddr.port(), 65535);

    // This will not modify the port
    saddr = "192.168.47.48";
    EXPECT_EQ(saddr.ss.ss_family, AF_INET);
    EXPECT_EQ(saddr.netaddr(), "192.168.47.48");
    EXPECT_EQ(saddr.netaddrp(), "192.168.47.48:65535");
    EXPECT_EQ(saddr.port(), 65535);

    // Check that a failing call does not modify old settings.
    EXPECT_THAT(
        [&saddr]() { saddr = "65536"; },
        ThrowsMessage<std::range_error>(HasSubstr("UPnPsdk MSG1127 EXCEPT[")));
    EXPECT_EQ(saddr.ss.ss_family, AF_INET);
    EXPECT_EQ(saddr.netaddr(), "192.168.47.48");
    EXPECT_EQ(saddr.netaddrp(), "192.168.47.48:65535");
    EXPECT_EQ(saddr.port(), 65535);

    saddr = "[2001:db8::2]";
    EXPECT_EQ(saddr.ss.ss_family, AF_INET6);
    EXPECT_EQ(saddr.netaddr(), "[2001:db8::2]");
    EXPECT_EQ(saddr.netaddrp(), "[2001:db8::2]:65535");
    EXPECT_EQ(saddr.port(), 65535);

    saddr = "127.0.0.4:50004";
    EXPECT_EQ(saddr.ss.ss_family, AF_INET);
    EXPECT_EQ(saddr.netaddr(), "127.0.0.4");
    EXPECT_EQ(saddr.netaddrp(), "127.0.0.4:50004");
    EXPECT_EQ(saddr.port(), 50004);

    saddr = "2001:db8::7";
    EXPECT_EQ(saddr.ss.ss_family, AF_INET6);
    EXPECT_EQ(saddr.netaddr(), "[2001:db8::7]");
    EXPECT_EQ(saddr.netaddrp(), "[2001:db8::7]:50004");
    EXPECT_EQ(saddr.port(), 50004);

    saddr = "127.0.0.6:";
    EXPECT_EQ(saddr.ss.ss_family, AF_INET);
    EXPECT_EQ(saddr.netaddr(), "127.0.0.6");
    EXPECT_EQ(saddr.netaddrp(), "127.0.0.6:0");
    EXPECT_EQ(saddr.port(), 0);
}

TEST(SockaddrStorageTestSuite, set_link_local_address) {
    // IPv6 link-local unicast address structure is defined as:
    // ----------+------------+-----------+--------------
    //           |  10 bits   |  54 bits  |   64 bits
    // fe80::/10 | 1111111010 | 000...000 | Interface ID
    // ----------+------------+-----------+--------------
    // Different platforms behave different for accepting a valid LLA.
    SSockaddr saddr;

    saddr = "[fe80::]";
    EXPECT_EQ(saddr.netaddrp(), "[fe80::]:0");

    saddr = "[fe80::1]:50001";
    EXPECT_EQ(saddr.netaddrp(), "[fe80::1]:50001");

    saddr = "[fe80::1111:2222:3333:4444]:50002";
    EXPECT_EQ(saddr.netaddrp(), "[fe80::1111:2222:3333:4444]:50002");

    // Invalid LLA?
    saddr = "[fe80::1:2222:3333:4444:5555]:50003";
    EXPECT_EQ(saddr.netaddrp(), "[fe80::1:2222:3333:4444:5555]:50003");

    // Invalid LLA?
    saddr = "[fe80:1::2222:3333:4444:5555]:50004";
#ifdef __APPLE__
    // Huu? Seems macOS modifies to a different ip address.
    EXPECT_EQ(saddr.netaddrp(), "[fe80::2222:3333:4444:5555%lo0]:50004");
#else
    EXPECT_EQ(saddr.netaddrp(), "[fe80:1::2222:3333:4444:5555]:50004");
#endif

    // Invalid LLA?
    saddr = "[fe80::5678:9abc:def1:2345:6789:abcd]:50005";
    EXPECT_EQ(saddr.netaddrp(), "[fe80:0:5678:9abc:def1:2345:6789:abcd]:50005");

    // Invalid LLA?
    saddr = "[fe80:1234:5678:9abc:def1:2345:6789:abcd]:50006";
#ifdef __APPLE__
    EXPECT_EQ(saddr.netaddrp(), ":50006");
#else
    EXPECT_EQ(saddr.netaddrp(),
              "[fe80:1234:5678:9abc:def1:2345:6789:abcd]:50006");
#endif

    // If the scope id on Unix like platforms matches a real interface then its
    // name is returned instead of the number, e.g. "%lo" instead of "%1".
    // Microsoft Windows always returns the number.
    saddr = "[fe80::8%1]:50008";
    EXPECT_THAT(saddr.netaddrp(),
                AnyOf("[fe80::8%1]:50008", "[fe80::8%lo]:50008",
                      "[fe80::8%lo0]:50008"));
    saddr = "[fe80::a%1]";
    EXPECT_THAT(saddr.netaddrp(),
                AnyOf("[fe80::a%1]:50008", "[fe80::a%lo]:50008",
                      "[fe80::a%lo0]:50008"));
    saddr = "fe80::b%1";
    EXPECT_THAT(saddr.netaddrp(),
                AnyOf("[fe80::b%1]:50008", "[fe80::b%lo]:50008",
                      "[fe80::b%lo0]:50008"));
    saddr = "[fe80::9%1]:";
    EXPECT_THAT(saddr.netaddrp(),
                AnyOf("[fe80::9%1]:0", "[fe80::9%lo]:0", "[fe80::9%lo0]:0"));
}

TEST(SockaddrStorageTestSuite, set_address_and_port_fail) {
    SSockaddr saddr;

    EXPECT_THAT([&saddr]() { saddr = ":65536"; },
                ThrowsMessage<std::range_error>(
                    EndsWith("] Number string from "
                             "\":65536\" for port is out of range 0..65535.")));

    EXPECT_THAT([&saddr]() { saddr = "65536"; },
                ThrowsMessage<std::range_error>(
                    EndsWith("] Number string from "
                             "\"65536\" for port is out of range 0..65535.")));

    EXPECT_THAT([&saddr]() { saddr = "127.0.0.1:65536"; },
                ThrowsMessage<std::range_error>(EndsWith(
                    "] Number string from "
                    "\"127.0.0.1:65536\" for port is out of range 0..65535.")));

    EXPECT_THAT([&saddr]() { saddr = ":"; },
                ThrowsMessage<std::invalid_argument>(
                    EndsWith("] Invalid netaddress \":\".\n")));

    EXPECT_THAT([&saddr]() { saddr = "garbage"; },
                ThrowsMessage<std::invalid_argument>(
                    EndsWith("] Invalid netaddress \"garbage\".\n")));

    EXPECT_THAT([&saddr]() { saddr = "[2001::db8::1]"; },
                ThrowsMessage<std::invalid_argument>(
                    EndsWith("] Invalid netaddress \"[2001::db8::1]\".\n")));

    EXPECT_THAT([&saddr]() { saddr = "2001:db8::2]"; },
                ThrowsMessage<std::invalid_argument>(
                    EndsWith("] Invalid netaddress \"2001:db8::2]\".\n")));

    EXPECT_THAT([&saddr]() { saddr = "[2001:db8::3"; },
                ThrowsMessage<std::invalid_argument>(
                    EndsWith("] Invalid netaddress \"[2001:db8::3\".\n")));

    EXPECT_THAT([&saddr]() { saddr = "[2001:db8::5]50003"; },
                ThrowsMessage<std::invalid_argument>(
                    EndsWith("] Invalid netaddress "
                             "\"[2001:db8::5]50003\".\n")));

    EXPECT_THAT([&saddr]() { saddr = "[2001:db8::35003]"; },
                ThrowsMessage<std::invalid_argument>(
                    EndsWith("] Invalid netaddress "
                             "\"[2001:db8::35003]\".\n")));

    EXPECT_THAT([&saddr]() { saddr = "192.168.66.67."; },
                ThrowsMessage<std::invalid_argument>(
                    EndsWith("] Invalid netaddress \"192.168.66.67.\".\n")));

    EXPECT_THAT([&saddr]() { saddr = "192.168.66.67z"; },
                ThrowsMessage<std::invalid_argument>(
                    EndsWith("] Invalid netaddress \"192.168.66.67z\".\n")));

    EXPECT_THAT([&saddr]() { saddr = "[192.168.66.67]"; },
                ThrowsMessage<std::invalid_argument>(
                    EndsWith("] Invalid netaddress \"[192.168.66.67]\".\n")));

    EXPECT_THAT([&saddr]() { saddr = "[192.168.66.68]:"; },
                ThrowsMessage<std::invalid_argument>(
                    EndsWith("] Invalid netaddress "
                             "\"[192.168.66.68]:\".\n")));

    EXPECT_THAT([&saddr]() { saddr = "[192.168.66.68]:50044"; },
                ThrowsMessage<std::invalid_argument>(
                    EndsWith("] Invalid netaddress "
                             "\"[192.168.66.68]:50044\".\n")));
}

TEST(SockaddrCmpTestSuite, compare_equal_ipv6_sockaddrs_successful) {
    SSockaddr saddr1;
    saddr1 = "[2001:db8::33]:50033";
    SSockaddr saddr2;
    saddr2 = "[2001:db8::33]:50033";

    // Test Unit
    EXPECT_TRUE(sockaddrcmp(&saddr1.ss, &saddr2.ss));
}

TEST(SockaddrCmpTestSuite, compare_equal_ipv4_sockaddrs_successful) {
    SSockaddr saddr1;
    saddr1 = "192.168.167.166:50037";
    SSockaddr saddr2;
    saddr2 = "192.168.167.166:50037";

    // Test Unit
    EXPECT_TRUE(sockaddrcmp(&saddr1.ss, &saddr2.ss));
}

TEST(SockaddrCmpTestSuite, compare_different_ipv6_sockaddrs) {
    // Test Unit, compare with different ports
    SSockaddr saddr1;
    saddr1 = "[2001:db8::35]:50035";
    SSockaddr saddr2;
    saddr2 = "[2001:db8::35]:50036";
    EXPECT_FALSE(sockaddrcmp(&saddr1.ss, &saddr2.ss));

    // Test Unit, compare with different address
    saddr2 = "[2001:db8::36]:50035";
    EXPECT_FALSE(sockaddrcmp(&saddr1.ss, &saddr2.ss));

    // Test Unit, compare with different address family
    saddr2 = "192.168.171.172:50035";
    EXPECT_FALSE(sockaddrcmp(&saddr1.ss, &saddr2.ss));
}

TEST(SockaddrCmpTestSuite, compare_different_ipv4_sockaddrs) {
    // Test Unit, compare with different ports
    SSockaddr saddr1;
    saddr1 = "192.168.13.14:50038";
    SSockaddr saddr2;
    saddr2 = "192.168.13.14:50039";
    EXPECT_FALSE(sockaddrcmp(&saddr1.ss, &saddr2.ss));

    // Test Unit, compare with different address
    saddr2 = "192.168.13.15:50038";
    EXPECT_FALSE(sockaddrcmp(&saddr1.ss, &saddr2.ss));

    // Test Unit, compare with different address family already done with
    // compare_different_ipv6_sockaddrs.
}

TEST(SockaddrCmpTestSuite, compare_unspecified_sockaddrs) {
    // Test Unit with unspecified address family will always return true if
    // equal, independent of address and port.
    SSockaddr saddr1;
    saddr1.ss.ss_family = AF_UNSPEC;
    SSockaddr saddr2;
    saddr2.ss.ss_family = AF_UNSPEC;
    EXPECT_TRUE(sockaddrcmp(&saddr1.ss, &saddr2.ss));

    saddr2 = "[2001:db8::40]:50040";
    EXPECT_FALSE(sockaddrcmp(&saddr1.ss, &saddr2.ss));
    EXPECT_FALSE(sockaddrcmp(&saddr2.ss, &saddr1.ss));
}

TEST(SockaddrCmpTestSuite, compare_nullptr_to_sockaddrs) {
    SSockaddr saddr1;
    saddr1 = "[2001:db8::34]:50034";

    // Test Unit
    EXPECT_TRUE(sockaddrcmp(nullptr, nullptr));
    EXPECT_FALSE(sockaddrcmp(nullptr, &saddr1.ss));
    EXPECT_FALSE(sockaddrcmp(&saddr1.ss, nullptr));
}

TEST(SockaddrStorageTestSuite, copy_and_assign_structure) {
    SSockaddr saddr1;
    saddr1 = "[2001:db8::1]:50001";

    // Test Unit copy
    // With UPnPsdk_WITH_TRACE we do not see "Construct ..." because the
    // default copy constructor is used here.
    SSockaddr saddr2 = saddr1;
    EXPECT_EQ(saddr2.ss.ss_family, AF_INET6);
    EXPECT_EQ(saddr2.netaddr(), "[2001:db8::1]");
    EXPECT_EQ(saddr2.port(), 50001);

    // Test Unit assign
    saddr1 = "192.168.251.252:50002";
    SSockaddr saddr3;
    saddr3 = saddr1;
    EXPECT_EQ(saddr3.ss.ss_family, AF_INET);
    EXPECT_EQ(saddr3.netaddr(), "192.168.251.252");
    EXPECT_EQ(saddr3.port(), 50002);
}

TEST(SockaddrStorageTestSuite, compare_ipv6_address) {
    SSockaddr saddr1;
    saddr1 = "[2001:db8::27]:50027";
    SSockaddr saddr2;
    saddr2 = "[2001:db8::27]:50027";

    // Show how to address and compare ipv6 address. It is stored with
    // unsigned char s6_addr[16]; so we have to use memcmp().
    const unsigned char* const s6_addr1 =
        reinterpret_cast<const sockaddr_in6*>(&saddr1.ss)->sin6_addr.s6_addr;
    const unsigned char* const s6_addr2 =
        reinterpret_cast<const sockaddr_in6*>(&saddr2.ss)->sin6_addr.s6_addr;
    EXPECT_EQ(memcmp(s6_addr1, s6_addr2, sizeof(*s6_addr1)), 0);

    // Test Unit compare successful
    EXPECT_TRUE(saddr1 == saddr2);

    // Test Unit different address family
    saddr2 = "192.168.0.27:50027";
    EXPECT_FALSE(saddr1 == saddr2);

    // Test Unit different address
    saddr2 = "[2001:db8::28]:50027";
    EXPECT_FALSE(saddr1 == saddr2);

    // Test Unit different port
    saddr2 = "[2001:db8::27]:50028";
    EXPECT_FALSE(saddr1 == saddr2);

    // Test Unit with unsupported address family
    saddr2 = "[2001:db8::27]:50027";
    saddr1.ss.ss_family = AF_UNIX;
    EXPECT_FALSE(saddr1 == saddr2);
    saddr1.ss.ss_family = AF_INET6;
    saddr2.ss.ss_family = AF_UNIX;
    EXPECT_FALSE(saddr1 == saddr2);
    saddr2.ss.ss_family = AF_INET6;
    EXPECT_TRUE(saddr1 == saddr2);
}

TEST(SockaddrStorageTestSuite, compare_ipv4_address) {
    SSockaddr saddr1;
    saddr1 = "192.168.0.1:50029";
    SSockaddr saddr2;
    saddr2 = "192.168.0.1:50029";

    // Test Unit compare successful
    EXPECT_TRUE(saddr1 == saddr2);

    // Test Unit different address family
    saddr2 = "[2001:db8::27]:50027";
    EXPECT_FALSE(saddr1 == saddr2);

    // Test Unit different address
    saddr2 = "192.168.0.2:50029";
    EXPECT_FALSE(saddr1 == saddr2);

    // Test Unit different port
    saddr2 = "192.168.0.1:50030";
    EXPECT_FALSE(saddr1 == saddr2);

    // Test Unit with unsupported address family
    saddr2 = "192.168.0.1:50029";
    saddr1.ss.ss_family = AF_UNIX;
    EXPECT_FALSE(saddr1 == saddr2);
    saddr1.ss.ss_family = AF_INET;
    saddr2.ss.ss_family = AF_UNIX;
    EXPECT_FALSE(saddr1 == saddr2);
    saddr2.ss.ss_family = AF_INET;
    EXPECT_TRUE(saddr1 == saddr2);
}

TEST(SockaddrStorageTestSuite, sizeof_saddr_get_successful) {
    SSockaddr saddr;
    saddr = "[::]";
    EXPECT_EQ(saddr.sizeof_saddr(),
              static_cast<socklen_t>(sizeof(::sockaddr_in6)));
    saddr = "0.0.0.0";
    EXPECT_EQ(saddr.sizeof_saddr(),
              static_cast<socklen_t>(sizeof(::sockaddr_in)));
    saddr = ":50001"; // Does not modify previous address setting
    EXPECT_EQ(saddr.sizeof_saddr(),
              static_cast<socklen_t>(sizeof(::sockaddr_in)));
    EXPECT_EQ(saddr.netaddrp(), "0.0.0.0:50001");
    SSockaddr saddr2; // An empty socket address structure provides full storage
    saddr2 = "";      // this clears the complete socket address.
    EXPECT_EQ(saddr2.sizeof_saddr(),
              static_cast<socklen_t>(sizeof(::sockaddr_storage)));
    saddr2.ss.ss_family = AF_UNIX; // Inval addr family doesn't provide storage
    EXPECT_EQ(saddr2.sizeof_saddr(), static_cast<socklen_t>(0));
}

TEST(SockaddrStorageTestSuite, is_loopback) {
    SSockaddr saObj;
    saObj = "[::]";
    EXPECT_FALSE(saObj.is_loopback());
    saObj = "[::1]";
    EXPECT_TRUE(saObj.is_loopback());
    saObj = "[::2]";
    EXPECT_FALSE(saObj.is_loopback());
    saObj = "126.255.255.255";
    EXPECT_FALSE(saObj.is_loopback());
    saObj = "127.0.0.0"; // IPv4 never supported.
    EXPECT_FALSE(saObj.is_loopback());
    saObj = "[::ffff:127.0.0.0]";
    EXPECT_TRUE(saObj.is_loopback());
    saObj = "127.0.0.1"; // IPv4 never supported.
    EXPECT_FALSE(saObj.is_loopback());
    saObj = "[::ffff:127.0.0.1]";
    EXPECT_TRUE(saObj.is_loopback());
    saObj = "127.255.255.255"; // IPv4 never supported.
    EXPECT_FALSE(saObj.is_loopback());
    saObj = "[::ffff:127.255.255.255]";
    EXPECT_TRUE(saObj.is_loopback());
    saObj = "128.0.0.0";
    EXPECT_FALSE(saObj.is_loopback());
}

TEST(SockaddrStorageTestSuite, output_netaddr_to_ostream) {
    SSockaddr saObj;
    CaptureStdOutErr captoutObj(STDOUT_FILENO);

    // Test Unit output stream
    saObj = "[2001:db8::1]:61234";
    captoutObj.start();
    std::cout << std::flush << saObj << std::endl;
    EXPECT_THAT(captoutObj.str(), EndsWith("[2001:db8::1]:61234\n"));

    saObj = "[2001:db8::2]";
    captoutObj.start();
    std::cout << std::flush << saObj << std::endl;
    EXPECT_THAT(captoutObj.str(), EndsWith("[2001:db8::2]:61234\n"));

    CaptureStdOutErr capterrObj(STDERR_FILENO);

    saObj = "[2001:db8::3]:49999";
    capterrObj.start();
    std::cerr << saObj << std::endl;
    EXPECT_THAT(capterrObj.str(), EndsWith("[2001:db8::3]:49999\n"));

    saObj = "[2001:db8::4]";
    capterrObj.start();
    std::cerr << saObj << std::endl;
    EXPECT_THAT(capterrObj.str(), EndsWith("[2001:db8::4]:49999\n"));
}

TEST(ToAddrStrTestSuite, sockaddr_to_address_string) {
    SSockaddr saddr;

    EXPECT_EQ(saddr.netaddr(), "");

    saddr.ss.ss_family = AF_UNSPEC;
    EXPECT_EQ(saddr.netaddr(), "");

    saddr.ss.ss_family = AF_INET6;
    EXPECT_EQ(saddr.netaddr(), "[::]");

    saddr.ss.ss_family = AF_INET;
    EXPECT_EQ(saddr.netaddr(), "0.0.0.0");

    saddr = "[fe80:db8::5%21]";
#ifdef __APPLE__
    EXPECT_EQ(saddr.netaddr(), "");
#else
    EXPECT_EQ(saddr.netaddr(), "[fe80:db8::5%21]");
#endif

    saddr = "[2001:db8::4]";
    EXPECT_EQ(saddr.netaddr(), "[2001:db8::4]");

    saddr = "192.168.88.99";
    EXPECT_EQ(saddr.netaddr(), "192.168.88.99");

    bool g_dbug_old = g_dbug;
    CaptureStdOutErr captureObj(UPnPsdk::log_fileno);
    saddr.ss.ss_family = AF_UNIX;
    g_dbug = false;
    captureObj.start();
    EXPECT_EQ(saddr.netaddr(), "");
    EXPECT_EQ(captureObj.str(), "");
    g_dbug = true;
    captureObj.start();
    EXPECT_EQ(saddr.netaddr(), "");
    EXPECT_THAT(
        captureObj.str(),
        ContainsStdRegex(
            "UPnPsdk MSG1129 ERROR \\[.* Unsupported address family 1"));

    saddr.ss.ss_family = 255;
    g_dbug = false;
    captureObj.start();
    EXPECT_EQ(saddr.netaddr(), "");
    EXPECT_EQ(captureObj.str(), "");
    g_dbug = true;
    captureObj.start();
    EXPECT_EQ(saddr.netaddr(), "");
    EXPECT_THAT(
        captureObj.str(),
        ContainsStdRegex(
            "UPnPsdk MSG1129 ERROR \\[.* Unsupported address family 255"));
    g_dbug = g_dbug_old;
}

TEST(ToAddrStrTestSuite, sockaddr_to_address_port_string) {
    SSockaddr saddr;

    EXPECT_EQ(saddr.netaddrp(), ":0");

    saddr.ss.ss_family = AF_UNSPEC;
    EXPECT_EQ(saddr.netaddrp(), ":0");

    saddr.ss.ss_family = AF_INET6;
    EXPECT_EQ(saddr.netaddrp(), "[::]:0");

    saddr.ss.ss_family = AF_INET;
    EXPECT_EQ(saddr.netaddrp(), "0.0.0.0:0");

    saddr = "[2001:db8::4]";
    EXPECT_EQ(saddr.netaddrp(), "[2001:db8::4]:0");

    saddr = "192.168.88.100";
    EXPECT_EQ(saddr.netaddrp(), "192.168.88.100:0");

    saddr = "[2001:db8::5]:56789";
    EXPECT_EQ(saddr.netaddrp(), "[2001:db8::5]:56789");

    saddr = "192.168.88.101:54321";
    EXPECT_EQ(saddr.netaddrp(), "192.168.88.101:54321");

    saddr.ss.ss_family = AF_UNIX;
    bool g_dbug_old = g_dbug;
    CaptureStdOutErr captureObj(UPnPsdk::log_fileno);
    g_dbug = false;
    captureObj.start();
    EXPECT_EQ(saddr.netaddrp(), ":0");
    EXPECT_EQ(captureObj.str(), "");
    g_dbug = true;
    captureObj.start();
    EXPECT_EQ(saddr.netaddrp(), ":0");
    EXPECT_THAT(
        captureObj.str(),
        ContainsStdRegex(
            "UPnPsdk MSG1129 ERROR \\[.* Unsupported address family 1"));
    g_dbug = g_dbug_old;
}

#if 0
// Test what is accepted by inet_pton() for information. I do not need to always
// test it.
TEST(SockaddrStorageTestSuite, test_pton) {
    in_addr buf[sizeof(in_addr)];
    in6_addr buf6[sizeof(in6_addr)];

    EXPECT_EQ(inet_pton(AF_INET, "192.168.1.2", &buf), 1);
    // Following are invalid
    EXPECT_EQ(inet_pton(AF_INET, "192.168.1.2:50002", &buf), 0);
    EXPECT_EQ(inet_pton(AF_INET, "192.168.1.2/24", &buf), 0);
    EXPECT_EQ(inet_pton(AF_INET, "192.168.1.2/24:50002", &buf), 0);

    EXPECT_EQ(inet_pton(AF_INET6, "2001:db8::1", &buf6), 1);
    // Following are invalid
    EXPECT_EQ(inet_pton(AF_INET6, "2001:db8::1/64", &buf6), 0);
    EXPECT_EQ(inet_pton(AF_INET6, "2001:db8::1%eth0", &buf6), 0);
    EXPECT_EQ(inet_pton(AF_INET6, "[2001:db8::1]", &buf6), 0);
    EXPECT_EQ(inet_pton(AF_INET6, "[2001:db8::1]/64", &buf6), 0);
    EXPECT_EQ(inet_pton(AF_INET6, "[2001:db8::1]%eth0", &buf6), 0);
    EXPECT_EQ(inet_pton(AF_INET6, "[2001:db8::1]:50010", &buf6), 0);
}
#endif

#if 0
// This is to get the binary netorder ipv4 value from the IPv4 mapped IPv6
// address to be used for inexpensive comparison. It's not a real test so I do
// not need to always run it.
//
TEST(UpnpapiTestSuite, get_binary_ip) {
    UPnPsdk::SSockaddr saObj;
    in_addr* sin_addr =
        reinterpret_cast<in_addr*>(&saObj.sin6.sin6_addr.s6_addr[12]);

    saObj = "[::ffff:10.0.0.0]";
    std::cout << "10.0.0.0        = " << ntohl(sin_addr->s_addr) << '\n';
    saObj = "[::ffff:10.255.255.255]";
    std::cout << "10.255.255.255  = " << ntohl(sin_addr->s_addr) << '\n';

    saObj = "[::ffff:127.0.0.0]";
    std::cout << "127.0.0.0       = " << ntohl(sin_addr->s_addr) << '\n';
    saObj = "[::ffff:127.255.255.255]";
    std::cout << "127.255.255.255 = " << ntohl(sin_addr->s_addr) << '\n';

    saObj = "[::ffff:172.16.0.0]";
    std::cout << "172.16.0.0      = " << ntohl(sin_addr->s_addr) << '\n';
    saObj = "[::ffff:172.31.255.255]";
    std::cout << "172.31.255.255  = " << ntohl(sin_addr->s_addr) << '\n';

    saObj = "[::ffff:192.168.0.0]";
    std::cout << "192.168.0.0     = " << ntohl(sin_addr->s_addr) << '\n';
    saObj = "[::ffff:192.168.255.255]";
    std::cout << "192.168.255.255 = " << ntohl(sin_addr->s_addr) << '\n';

    char addrbuf[20]{};
    inet_ntop(AF_INET, sin_addr, addrbuf, sizeof(addrbuf));
    std::cout << "addrbuf = " << addrbuf << '\n';
}
#endif

TEST(SockaddrStorageTestSuite, string_to_port) {
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

TEST(SockaddrStorageTestSuite, string_to_port_test_only) {
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


std::string m_addr_str;
std::string m_port_str;

class SplitAddrPortTest
    : public ::testing::TestWithParam<std::tuple<
          // IP address to split   IP address only    port
          const std::string, const std::string, const std::string>> {};

TEST_P(SplitAddrPortTest, split_address_and_port) {
    // Get parameter
    const std::tuple params = GetParam();

    split_addr_port(std::get<0>(params), m_addr_str, m_port_str);
    EXPECT_EQ(m_addr_str, std::get<1>(params));
    EXPECT_EQ(m_port_str, std::get<2>(params));
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(SplitAddrPort, SplitAddrPortTest, ::testing::Values(
    std::make_tuple("[2001:db8::1]:50001", "2001:db8::1", "50001"),
    std::make_tuple("[2001:dB8::2]:", "2001:dB8::2", "0"),
    std::make_tuple("[2001:db8::2]", "2001:db8::2", ""),
    std::make_tuple(":50002", "", "50002"),
    std::make_tuple("127.0.0.4:50003", "127.0.0.4", "50003"),
    std::make_tuple("127.0.0.5:", "127.0.0.5", "0"),
    std::make_tuple("127.0.0.6", "127.0.0.6", ""),
    std::make_tuple("50004", "", "50004"),
    std::make_tuple("2001:db8::7", "2001:db8::7", ""),
    std::make_tuple("example.COM:50005", "example.COM", "50005"),
    std::make_tuple("example.com:httPS", "example.com", "httPS"),
    std::make_tuple("example.com:", "example.com", "0"),
    std::make_tuple("example.com", "example.com", ""),
    std::make_tuple("localhost:50006", "localhost", "50006"),
    std::make_tuple("localhost:https", "localhost", "https"),
    std::make_tuple("localhost:", "localhost", "0"),
    std::make_tuple("localhost", "localhost", ""),
    std::make_tuple("[localhost]", "[localhost]", ""),
    std::make_tuple("[localhost]:", "[localhost]", "0"),
    std::make_tuple("[localhost]:50007", "[localhost]", "50007"),
    std::make_tuple(":https", "", "https"),
    std::make_tuple("https", "https", ""),
    std::make_tuple("", "", ""),
    std::make_tuple("   ", "   ", ""),
    std::make_tuple("::", "::", ""),
    std::make_tuple("[::]", "::", ""),
    std::make_tuple("[::]:", "::", "0"),
    std::make_tuple("[::]:0", "::", "0"),
    std::make_tuple("::1", "::1", ""),
    std::make_tuple("[::1]", "::1", ""),
    std::make_tuple("[::1]:", "::1", "0"),
    std::make_tuple("[::1]:0", "::1", "0"),
    std::make_tuple("[::1].4", "[::1].4", ""),
    std::make_tuple("[::FFff:142.250.185.99]:50008", "::FFff:142.250.185.99", "50008")
));
// clang-format on

TEST(SockaddrStorageTestSuite, sockaddr_clear) {
    SSockaddr saObj;
    saObj = "[2001:db8::1]:50001";
    EXPECT_EQ(saObj.netaddrp(), "[2001:db8::1]:50001");
    saObj = "";
    EXPECT_EQ(saObj.netaddr(), "");
    EXPECT_EQ(saObj.netaddrp(), ":0");
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
