// Copyright (C) 2026+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-08-10

#include <UPnPsdk/addrinfo2.hpp>
#include <UPnPsdk/netadapter.hpp>
#include <UPnPsdk/global.hpp>

#include <utest/utest.hpp>

namespace utest {

using UPnPsdk::CAddrinfo2;
using UPnPsdk::g_dbug;
using UPnPsdk::SInaddr;
using UPnPsdk::SSockaddr;
using ADDRS = UPnPsdk::CNetadapter::ADDRS;

// Provide netadapter one time.
UPnPsdk::CNetadapter nadObj;

// Provide socket address object for general use.
SSockaddr saObj;

// Empty binary IPv6 address for comparison.
constexpr ::in6_addr sin6_addr_empty{};


#if 0
// Raw ::getaddrinfo and ::getnameinfo execution to verify its behavior. This is
// for humans only and not a real Unit Test. It should not always run.
TEST(AddrinfoTestSuite, getaddrinfo_raw) {
    ::addrinfo hints{}, *res{};

    hints.ai_flags = AI_V4MAPPED | AI_ALL;
    hints.ai_family = AF_INET6;
    // hints.ai_socktype = SOCK_STREAM;
    // hints.ai_protocol = 0;

    int ret = ::getaddrinfo(nullptr, "0", &hints, &res);
    ASSERT_EQ(ret, 0) << "Error (" << ret << "): \"" << ::gai_strerror(ret)
                      << "\".";

#if 0
    for (::addrinfo* ptr{res}; ptr != nullptr; ptr = ptr->ai_next) {
        auto sin6 = reinterpret_cast<sockaddr_in6*>(ptr->ai_addr);
        if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr))
            std::cout << "AI_V4MAPPED found\n";
        else
            std::cout << "AI_V4MAPPED not found\n";
    }

#else
    char addr_str[INET6_ADDRSTRLEN];
    char serv_str[NI_MAXSERV];
    for (::addrinfo* ptr{res}; ptr != nullptr; ptr = ptr->ai_next) {
        ::getnameinfo(ptr->ai_addr, static_cast<socklen_t>(ptr->ai_addrlen),
                      addr_str, sizeof(addr_str), serv_str, sizeof(serv_str),
                      NI_NUMERICHOST | NI_NUMERICSERV);
        std::cout << "ai_family=" << ptr->ai_family
                  << ", ai_socktype=" << ptr->ai_socktype
                  << ", ai_protocol=" << ptr->ai_protocol << ", addr=\""
                  << addr_str << "\", serv=\"" << serv_str
                  << "\", ai_flags=" << ptr->ai_flags << ".\n";
    }
#endif
    ::freeaddrinfo(res);
}
#endif


// Tests with empty CAddrinfo object
// ---------------------------------
TEST(AddrinfoTestSuite, addrinfo_empty_fails) {
    CAddrinfo2 aiObj;
    EXPECT_EQ(aiObj.get_first(SInaddr()), EAI_NONAME);
    aiObj.sockaddr(saObj);
    EXPECT_TRUE(saObj.empty());
    EXPECT_FALSE(aiObj.get_next());
}

TEST(AddrinfoTestSuite, addrinfo_empty_string_fails) {
    CAddrinfo2 aiObj;
    EXPECT_EQ(aiObj.get_first(SInaddr("")), EAI_NONAME);
    aiObj.sockaddr(saObj);
    EXPECT_TRUE(saObj.empty());
    EXPECT_FALSE(aiObj.get_next());
}

// Tests with empty node
// ---------------------
TEST(AddrinfoTestSuite, addrinfo_node_empty_with_port_active_successful) {
    // This result is specified in 'man getaddrinfo'.
    CAddrinfo2 aiObj;
    EXPECT_EQ(aiObj.get_first(SInaddr("0")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.family, AF_INET6);
    EXPECT_EQ(saObj.netaddrp(), "[::1]:0");
    EXPECT_FALSE(aiObj.get_next());
    aiObj.sockaddr(saObj);                  // Important test against segfault.
    EXPECT_EQ(saObj.netaddrp(), "[::1]:0"); // Nothing has changed.
}

TEST(AddrinfoTestSuite, addrinfo_node_empty_scope_0_port_active_successful) {
    // This result is specified in 'man getaddrinfo'.
    CAddrinfo2 aiObj(0, SOCK_DGRAM);
    EXPECT_EQ(aiObj.get_first(SInaddr("%0:0")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.family, AF_INET6);
    EXPECT_EQ(saObj.netaddrp(), "[::1]:0");
    EXPECT_FALSE(aiObj.get_next());
}

TEST(AddrinfoTestSuite, addrinfo_node_empty_with_port_passive_successful) {
    // This result is specified in 'man getaddrinfo'.
    CAddrinfo2 aiObj(AI_PASSIVE, SOCK_DGRAM);
    EXPECT_EQ(aiObj.get_first(SInaddr(":0")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.family, AF_INET6);
    EXPECT_EQ(saObj.netaddrp(), "[::]:0");
    EXPECT_FALSE(aiObj.get_next());
}

TEST(AddrinfoTestSuite, addrinfo_node_empty_scope_0_port_passive_successful) {
    // This result is specified in 'man getaddrinfo'.
    CAddrinfo2 aiObj(AI_PASSIVE);
    EXPECT_EQ(aiObj.get_first(SInaddr("%0:0")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.family, AF_INET6);
    EXPECT_EQ(saObj.netaddrp(), "[::]:0");
    EXPECT_FALSE(aiObj.get_next());
}

TEST(AddrinfoTestSuite, addrinfo_node_empty_with_scope_wrong_and_port) {
    CAddrinfo2 aiObj;
    EXPECT_EQ(aiObj.get_first(SInaddr("%1:50001")), EAI_NONAME);
    aiObj.sockaddr(saObj);
    EXPECT_TRUE(saObj.empty());
    EXPECT_FALSE(aiObj.get_next());
}

TEST(AddrinfoTestSuite, addrinfo_node_empty_with_scope_valid_and_port) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("[%1]:50001")), EAI_NONAME);
}

// Tests with the unspecified address
// ----------------------------------
TEST(AddrinfoTestSuite, addrinfo_unspec_no_brackets_successful) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("::")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_TRUE(saObj.empty());
    EXPECT_FALSE(aiObj.get_next());
}

TEST(AddrinfoTestSuite, addrinfo_unspec_with_brackets_successful) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("[::]")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_TRUE(saObj.empty());
    EXPECT_FALSE(aiObj.get_next());
}

TEST(AddrinfoTestSuite, addrinfo_unspec_addr_with_scope_and_port_successful) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("[::%1]:50001")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.family, AF_INET6);
    // scope_id is corrected by saObj, not by aiObj.
    EXPECT_EQ(saObj.sin6.sin6_scope_id, 0);
    EXPECT_EQ(saObj.sin6.sin6_port, htons(50001));
    EXPECT_EQ(
        ::memcmp(&saObj.sin6.sin6_addr, &sin6_addr_empty, sizeof(in6_addr)), 0);
    EXPECT_FALSE(aiObj.get_next());
}

TEST(AddrinfoTestSuite, addrinfo_unspec_addr_with_port_successful) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("[::]:50001")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.family, AF_INET6);
    EXPECT_FALSE(saObj.empty());
    EXPECT_EQ(saObj.sin6.sin6_scope_id, 0);
    EXPECT_EQ(saObj.sin6.sin6_port, htons(50001));
    EXPECT_EQ(
        ::memcmp(&saObj.sin6.sin6_addr, &sin6_addr_empty, sizeof(in6_addr)), 0);
    EXPECT_FALSE(aiObj.get_next());
}

// Tests with numeric loopback interface
// -------------------------------------
TEST(AddrinfoTestSuite, addrinfo_loopback_successful) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("[::1]")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.family, AF_INET6);
    EXPECT_EQ(saObj.netaddrp(), "[::1]:0");
    EXPECT_FALSE(aiObj.get_next());
}

TEST(AddrinfoTestSuite, addrinfo_loopback_ipv4_with_port_successful) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("127.0.0.1:50001")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.family, AF_INET6);
    EXPECT_EQ(saObj.netaddrp(), "[::ffff:127.0.0.1]:50001");
    EXPECT_FALSE(aiObj.get_next());
}

TEST(AddrinfoTestSuite, addrinfo_loopback_with_scope_and_port_successful) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("[::1%1]:50001")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.family, AF_INET6);
    // scope_id is corrected by saObj, not by aiObj.
    EXPECT_EQ(saObj.netaddrp(), "[::1]:50001");
    EXPECT_FALSE(aiObj.get_next());
}

TEST(AddrinfoTestSuite, addrinfo_loopback_ipv4_with_scope_fails) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("127.0.0.1%252")), EAI_NONAME);
}

TEST(AddrinfoTestSuite, addrinfo_loopback_scope_no_brackets_and_port_fails) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("::1%1:50001")), EAI_NONAME);
}

TEST(AddrinfoTestSuite, addrinfo_loopback_with_port_successful) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("[::1]:50001")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.family, AF_INET6);
    EXPECT_EQ(saObj.netaddrp(), "[::1]:50001");
    EXPECT_FALSE(aiObj.get_next());
}

TEST(AddrinfoTestSuite, addrinfo_loopback_with_wrong_port_fails) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("::1:50001")), EAI_NONAME);
}

// Tests with alpha-numeric localhost interface
// --------------------------------------------
TEST(AddrinfoTestSuite, addrinfo_localhost_successful) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("localhost")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.family, AF_INET6);
    EXPECT_EQ(saObj.netaddrp(), "[::1]:0");
    EXPECT_FALSE(aiObj.get_next());
}

TEST(AddrinfoTestSuite, addrinfo_localhost_with_service_successful) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("localhost:https")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.family, AF_INET6);
    EXPECT_EQ(saObj.netaddrp(), "[::1]:443");
    EXPECT_FALSE(aiObj.get_next());
}

TEST(AddrinfoTestSuite, addrinfo_localhost_with_scope_fails) {
    if (!g_dbug)
        GTEST_SKIP()
            << "due to trigger DNS lookup. Enable with '--UPnPsdk_debug'.";

    // Get valid local adapter name that has a localhost address.
    ASSERT_TRUE(nadObj.find_first(ADDRS::lo));
    {
        SInaddr inaddr("localhost%" + nadObj.name());
        CAddrinfo2 aiObj;
        ASSERT_EQ(aiObj.get_first(inaddr), EAI_NONAME);
    }
    {
        SInaddr inaddr("localhost%" + nadObj.name() + ":http");
        CAddrinfo2 aiObj;
        ASSERT_EQ(aiObj.get_first(inaddr), EAI_NONAME);
    }
}

// Tests with link-local address
// -----------------------------
TEST(AddrinfoTestSuite, addrinfo_lla_without_scope_fails) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("fe80::1")), EAI_NONAME);
}

TEST(AddrinfoTestSuite, addrinfo_lla_with_subnet_fails) {
    CAddrinfo2 aiObj;
#if 0
    if (compiler == CO::clang) {
        // macOS accepts lla with subnet.
        ASSERT_EQ(aiObj.get_first(SInaddr("fe80:1::2")), 0);
        aiObj.sockaddr(saObj);
        EXPECT_EQ(saObj.family, AF_INET6);
        // The result is corrected by the SDK.
        EXPECT_EQ(saObj.netaddrp(), "[fe80::2%1]:0");
        EXPECT_FALSE(aiObj.get_next());
    } else {
        // Other platforms reject this.
        ASSERT_EQ(aiObj.get_first(SInaddr("fe80:1::2")), EAI_NONAME);
    }
#endif
    ASSERT_EQ(aiObj.get_first(SInaddr("fe80:1::2")), EAI_NONAME);
}

TEST(AddrinfoTestSuite, addrinfo_lla_with_scope_num_and_port_num_successful) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("[fe80:0::1%252]:50001")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.family, AF_INET6);
    EXPECT_EQ(saObj.netaddrp(), "[fe80::1%252]:50001");
    EXPECT_FALSE(aiObj.get_next());
}

TEST(AddrinfoTestSuite, addrinfo_lla_scope_valid_alpha_successful) {
    // Get valid local adapter name that has an lla.
    ASSERT_TRUE(nadObj.find_first(ADDRS::lla));

    std::string inaddr("fe80::1%" + nadObj.name());
    CAddrinfo2 aiObj;
#if 0
    if (compiler == CO::msc) {
        // Microsoft Windows accepts only a numeric scope_id.
        ASSERT_EQ(aiObj.get_first(SInaddr(inaddr)), 11001); // WSAHOST_NOT_FOUND
    } else
#endif
    {
        ASSERT_EQ(aiObj.get_first(SInaddr(inaddr)), 0);
        aiObj.sockaddr(saObj);
        EXPECT_EQ(saObj.family, AF_INET6);
        EXPECT_EQ(saObj.netaddrp(),
                  "[fe80::1%" + std::to_string(nadObj.index()) + "]:0");
        EXPECT_FALSE(aiObj.get_next());
    }
}

TEST(AddrinfoTestSuite, addrinfo_lla_scope_invalid_alpha_fails) {
    SInaddr inaObj("[fe80::1%zagl9]:0"); // Hope this is never valid.
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(inaObj), EAI_NONAME);
}


// Tests with unique local address
// -------------------------------
// Has to be done.


// Tests with global unicast address
// ---------------------------------
TEST(AddrinfoTestSuite, addrinfo_gua_successful) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("[2001:db8::1]:50001")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.family, AF_INET6);
    EXPECT_EQ(saObj.netaddrp(), "[2001:db8::1]:50001");
    EXPECT_FALSE(aiObj.get_next());
}

TEST(AddrinfoTestSuite, addrinfo_gua_scope_id_num_removed_successful) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("2001:db8::1%123")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.family, AF_INET6);
    EXPECT_EQ(saObj.netaddrp(), "[2001:db8::1]:0");
    EXPECT_FALSE(aiObj.get_next());
}

TEST(AddrinfoTestSuite, addrinfo_gua_scope_id_alpha_removed_successful) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("2001:db8::1%zagl9")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.family, AF_INET6);
    EXPECT_EQ(saObj.netaddrp(), "[2001:db8::1]:0");
    EXPECT_FALSE(aiObj.get_next());
}

TEST(AddrinfoTestSuite, addrinfo_node_alpha_successful) {
    if (!g_dbug)
        GTEST_SKIP()
            << "due to trigger DNS lookup. Enable with '--UPnPsdk_debug'.";

    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("example.com:50002")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.family, AF_INET6);
    EXPECT_FALSE(saObj.empty()); // Not empty only with valid sockaddr.
    EXPECT_TRUE(aiObj.get_next());
    EXPECT_FALSE(saObj.empty());
}

TEST(AddrinfoTestSuite, addrinfo_node_alpha_with_scope_num_fails) {
    if (!g_dbug)
        GTEST_SKIP()
            << "due to trigger DNS lookup. Enable with '--UPnPsdk_debug'.";

    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("example.com%2")), EAI_NONAME);
}

// Tests with IPv4 address
// -----------------------
TEST(AddrinfoTestSuite, addrinfo_ipv4_successful) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("192.168.88.98")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.family, AF_INET6);
    EXPECT_EQ(saObj.netaddrp(), "[::ffff:192.168.88.98]:0");
    EXPECT_FALSE(aiObj.get_next());
}

TEST(AddrinfoTestSuite, addrinfo_ipv4_scope_0_port_num_successful) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("192.168.88.98%0:50011")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.family, AF_INET6);
    EXPECT_EQ(saObj.netaddrp(), "[::ffff:192.168.88.98]:50011");
    EXPECT_FALSE(aiObj.get_next());
}

TEST(AddrinfoTestSuite, addrinfo_ipv4_with_port_alpha_successful) {
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("192.168.88.98:https")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.family, AF_INET6);
    EXPECT_EQ(saObj.netaddrp(), "[::ffff:192.168.88.98]:443");
    EXPECT_FALSE(aiObj.get_next());
}

TEST(AddrinfoTestSuite, addrinfo_ipv4_with_scope_id_num_fails) {
    if (!g_dbug)
        GTEST_SKIP()
            << "due to trigger DNS lookup. Enable with '--UPnPsdk_debug'.";

    // An unusable scope_id "%1:44".
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("192.168.88.98%1:https")), EAI_NONAME);
}

TEST(AddrinfoTestSuite, addrinfo_ipv4_with_wrong_scope_id_num_fails) {
    if (!g_dbug)
        GTEST_SKIP()
            << "due to trigger DNS lookup. Enable with '--UPnPsdk_debug'.";

    // An unusable scope_id "%1:44".
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("192.168.88.98%1:44:https")), EAI_NONAME);
}

TEST(AddrinfoTestSuite, addrinfo_ipv4_with_wrong_scope_id_alpha_fails) {
    if (!g_dbug)
        GTEST_SKIP()
            << "due to trigger DNS lookup. Enable with '--UPnPsdk_debug'.";

    // An unusable scope_id "%lo:44".
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("192.168.88.98%lo:44:http")), EAI_NONAME);
}

TEST(AddrinfoTestSuite, addrinfo_ipv4_with_wrong_node_fails) {
    if (!g_dbug)
        GTEST_SKIP()
            << "due to trigger DNS lookup. Enable with '--UPnPsdk_debug'.";

    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("192.168.88.98.44")), EAI_NONAME);
}

TEST(AddrinfoTestSuite, addrinfo_ipv4_with_node_and_port_alpha_successful) {
    if (!g_dbug)
        GTEST_SKIP()
            << "due to trigger DNS lookup. Enable with '--UPnPsdk_debug'.";

    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(SInaddr("example.com:https")), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.family, AF_INET6);
    EXPECT_FALSE(saObj.empty());
    EXPECT_EQ(saObj.sin6.sin6_scope_id, 0);
    EXPECT_EQ(saObj.sin6.sin6_port, htons(443));
    EXPECT_NE(
        ::memcmp(&saObj.sin6.sin6_addr, &sin6_addr_empty, sizeof(in6_addr)), 0);
    EXPECT_TRUE(aiObj.get_next());
}

// Other edge conditions
// ---------------------
TEST(AddrinfoTestSuite, addrinfo_get_first_addr_two_times_fails) {
    SInaddr inaddr("[::1]");
    CAddrinfo2 aiObj;
    ASSERT_EQ(aiObj.get_first(inaddr), 0);
    ASSERT_THROW(aiObj.get_first(inaddr), std::runtime_error);
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
    utest::nadObj.get_first();
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
