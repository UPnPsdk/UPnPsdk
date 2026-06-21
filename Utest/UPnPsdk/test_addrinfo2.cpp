// Copyright (C) 2026+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-06-23

// I test different address infos that we get from system function
// ::getaddrinfo().

#include <UPnPsdk/addrinfo.hpp>
#include <UPnPsdk/addrinfo2.hpp>
#include <UPnPsdk/netadapter.hpp>
#include <utest/utest.hpp>

namespace utest {

using testing::AnyOf;

using UPnPsdk::CAddrinfo2;
using UPnPsdk::CNetadapter;
using UPnPsdk::g_dbug;
using UPnPsdk::SSockaddr;
using ADDRS = UPnPsdk::CNetadapter::ADDRS;


// General storage for temporary socket address evaluation.
SSockaddr saObj;


#if 0
// Raw ::getaddrinfo and ::getnameinfo execution to verify its behavior. This is
// for humans only and not a real Unit Test. It should not always run.
TEST(AddrinfoTestSuite, getaddrinfo_raw) {
    ::addrinfo hints{}, *res{};

    hints.ai_flags = AI_V4MAPPED | AI_ALL;
    hints.ai_family = AF_INET6;
    // hints.ai_socktype = SOCK_STREAM;
    // hints.ai_protocol = 0;

    int ret = ::getaddrinfo("localhost", nullptr, &hints, &res);
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


// Tests for IPv6 scope_id and other results
// -----------------------------------------
// Following tests are to verify the results from system function
// 'getaddrinfo()' by direct calling it. This will not change and do not need to
// be executed every time.
#if 0
// Summary:
// Using only direct system calls.
// Same result: ai_family, ai_socktype, ai_addrlen, ai_canonname, ai_addr.port.
// All platforms return the socket address that was queried as input.

class AddrinfoFTestSuite : public ::testing::Test {
  protected:
    ::addrinfo m_hints{}, *m_res{nullptr};

    static constexpr char m_lla[]{"fe80::111"};
    static constexpr char m_gua[]{"2001:db8::1"};
    static constexpr char m_lbk[]{"::1"};
    char m_addrbuf[INET6_ADDRSTRLEN];

    AddrinfoFTestSuite() {
        m_hints.ai_flags = AI_V4MAPPED | AI_NUMERICHOST;
        m_hints.ai_family = AF_INET6;
        m_hints.ai_socktype = SOCK_STREAM;
    }

    ~AddrinfoFTestSuite() {
        if (m_res != nullptr) {
            ::freeaddrinfo(m_res);
            m_res = nullptr;
        }
    }
};

TEST_F(AddrinfoFTestSuite, verify_lla_with_valid_numeric_scope_id) {
    // All platforms succeed getaddrinfo().
    // Each platform returns different ai_flags.
    // Win32 returns ai_protocol given by hint, other return specific number.
    // All platforms return ai_addr.scope_id that was given with input address.

    constexpr char llascp[]{"fe80::111%252"};

    // Test Unit
    m_hints.ai_protocol = 6;
    ASSERT_EQ(::getaddrinfo(llascp, "https", &m_hints, &m_res), 0);

    EXPECT_EQ(m_res->ai_flags,
              compiler == CO::clang
                  ? 0
                  : (compiler == CO::msc ? AI_NUMERICHOST
                                         : AI_V4MAPPED | AI_NUMERICHOST));
    EXPECT_EQ(m_res->ai_family, AF_INET6);
    EXPECT_EQ(m_res->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(m_res->ai_protocol, 6);
    EXPECT_EQ(m_res->ai_addrlen, 28);
    EXPECT_EQ(m_res->ai_canonname, nullptr);
    EXPECT_EQ(m_res->ai_next, nullptr);
    ASSERT_NE(m_res->ai_addr, nullptr);
    auto sin6 = reinterpret_cast<sockaddr_in6*>(m_res->ai_addr);
    EXPECT_EQ(sin6->sin6_scope_id, 252);
    EXPECT_EQ(sin6->sin6_port, htons(443));
    ASSERT_NE(::inet_ntop(m_res->ai_family, &sin6->sin6_addr, m_addrbuf,
                          sizeof(m_addrbuf)),
              nullptr);
    EXPECT_STREQ(m_lla, m_addrbuf);
}

TEST_F(AddrinfoFTestSuite, verify_lla_with_invalid_numeric_scope_id) {
    // Only macOS succeeds getaddrinfo() call, other fail.
    // MacOS returns ai_flags set to 0.
    // MacOS returns specific ai_protocol number.
    // MacOS returns ai_addr.scope_id set to 0.

    constexpr char llascp[]{"fe80::111%-252"};

    // Test Unit
    m_hints.ai_protocol = 6;
    auto ret = ::getaddrinfo(llascp, "https", &m_hints, &m_res);

#ifdef __APPLE__
    ASSERT_EQ(ret, 0) << gai_strerror(ret);

    EXPECT_EQ(m_res->ai_flags, 0);
    EXPECT_EQ(m_res->ai_family, AF_INET6);
    EXPECT_EQ(m_res->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(m_res->ai_protocol, 6);
    EXPECT_EQ(m_res->ai_addrlen, 28);
    EXPECT_EQ(m_res->ai_canonname, nullptr);
    EXPECT_EQ(m_res->ai_next, nullptr);
    ASSERT_NE(m_res->ai_addr, nullptr);
    auto sin6 = reinterpret_cast<sockaddr_in6*>(m_res->ai_addr);
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, htons(443));
    ASSERT_NE(::inet_ntop(m_res->ai_family, &sin6->sin6_addr, m_addrbuf,
                          sizeof(m_addrbuf)),
              nullptr);
    EXPECT_STREQ(m_lla, m_addrbuf);
#else
    EXPECT_EQ(ret, EAI_NONAME) << gai_strerror(ret);
#endif
}

TEST_F(AddrinfoFTestSuite, verify_lla_with_valid_alphanum_scope_id) {
    // Win32 fails call getaddrinfo(), other succeed.
    // Other platforms return different ai_flags.
    // Other platforms return ai_protocol other as default by hint.
    // Other platforms return ai_addr.scope_id thats given with input address.

    // Get valid alpha-numeric scope_id
    CNetadapter naObj;
    ASSERT_NO_THROW(naObj.get_first());
    ASSERT_TRUE(naObj.find_first(UPnPsdk::CNetadapter::ADDRS::lla));

    std::string llascp("fe80::111%" + naObj.name());

    // Test Unit
    m_hints.ai_protocol = 6;
    auto ret = ::getaddrinfo(llascp.c_str(), "https", &m_hints, &m_res);

#ifdef _MSC_VER
    ASSERT_EQ(ret, WSAHOST_NOT_FOUND) << gai_strerror(ret);

#else
    ASSERT_EQ(ret, 0) << gai_strerror(ret);

    EXPECT_EQ(m_res->ai_flags,
              compiler == CO::clang ? 0 : AI_V4MAPPED | AI_NUMERICHOST);
    EXPECT_EQ(m_res->ai_family, AF_INET6);
    EXPECT_EQ(m_res->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(m_res->ai_protocol, 6);
    EXPECT_EQ(m_res->ai_addrlen, 28);
    EXPECT_EQ(m_res->ai_canonname, nullptr);
    EXPECT_EQ(m_res->ai_next, nullptr);
    ASSERT_NE(m_res->ai_addr, nullptr);
    auto sin6 = reinterpret_cast<sockaddr_in6*>(m_res->ai_addr);
    EXPECT_EQ(sin6->sin6_scope_id, naObj.index());
    EXPECT_EQ(sin6->sin6_port, htons(443));
    ASSERT_NE(::inet_ntop(m_res->ai_family, &sin6->sin6_addr, m_addrbuf,
                          sizeof(m_addrbuf)),
              nullptr);
    EXPECT_STREQ(m_lla, m_addrbuf);
#endif
}

TEST_F(AddrinfoFTestSuite, verify_lla_with_invalid_alphanum_scope_id) {
    // Only macOS succeeds getaddrinfo() call, other fail.
    // MacOS returns ai_flag set to 0.
    // MacOS returns specific ai_protocol number.
    // MacOS returns ai_addr.scope_id set to 0.

    constexpr char llascp[]{"fe80::111%lozyx"};

    // Test Unit
    m_hints.ai_protocol = 6;
    auto ret = ::getaddrinfo(llascp, "https", &m_hints, &m_res);

#ifdef __APPLE__
    ASSERT_EQ(ret, 0) << gai_strerror(ret);

    EXPECT_EQ(m_res->ai_flags, 0);
    EXPECT_EQ(m_res->ai_family, AF_INET6);
    EXPECT_EQ(m_res->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(m_res->ai_protocol, 6);
    EXPECT_EQ(m_res->ai_addrlen, 28);
    EXPECT_EQ(m_res->ai_canonname, nullptr);
    EXPECT_EQ(m_res->ai_next, nullptr);
    ASSERT_NE(m_res->ai_addr, nullptr);
    auto sin6 = reinterpret_cast<sockaddr_in6*>(m_res->ai_addr);
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, htons(443));
    ASSERT_NE(::inet_ntop(m_res->ai_family, &sin6->sin6_addr, m_addrbuf,
                          sizeof(m_addrbuf)),
              nullptr);
    EXPECT_STREQ(m_lla, m_addrbuf);
#else
    EXPECT_EQ(ret, EAI_NONAME) << gai_strerror(ret);
#endif
}

TEST_F(AddrinfoFTestSuite, verify_lla_with_no_scope_id) {
    // All platforms succeed getaddrinfo().
    // Each platform returns different ai_flags.
    // Win32 returns ai_protocol given by hint, other return specific number.
    // All platforms return no ai_addr.scope_id (set to 0).

    // Test Unit
    m_hints.ai_protocol = 6;
    ASSERT_EQ(::getaddrinfo(m_lla, "https", &m_hints, &m_res), 0);

    EXPECT_EQ(m_res->ai_flags,
              compiler == CO::clang
                  ? 0
                  : (compiler == CO::msc ? AI_NUMERICHOST
                                         : AI_V4MAPPED | AI_NUMERICHOST));
    EXPECT_EQ(m_res->ai_family, AF_INET6);
    EXPECT_EQ(m_res->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(m_res->ai_protocol, 6);
    EXPECT_EQ(m_res->ai_addrlen, 28);
    EXPECT_EQ(m_res->ai_canonname, nullptr);
    EXPECT_EQ(m_res->ai_next, nullptr);
    ASSERT_NE(m_res->ai_addr, nullptr);
    auto sin6 = reinterpret_cast<sockaddr_in6*>(m_res->ai_addr);
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, htons(443));
    ASSERT_NE(::inet_ntop(m_res->ai_family, &sin6->sin6_addr, m_addrbuf,
                          sizeof(m_addrbuf)),
              nullptr);
    EXPECT_STREQ(m_lla, m_addrbuf);
}

TEST_F(AddrinfoFTestSuite, verify_lla_with_protocol_0_socktype_0) {
    // All platforms succeed getaddrinfo().
    // Each platform returns different ai_flags.
    // Win32 returns ai_protocol given by hint, other return specific number.
    // All platforms return ai_addr.scope_id that was given with input address.
    // Win32 returns only one addrinfo with socktype set to 0.
    //
    // With ai_protocol=0 and ai_socktype=0 we get all possible combinations
    // with socktype reported of the current addrinfo structure. On Microsoft
    // Windows we get only one addrinfo structure with socktype set to 0.

    constexpr char llascp[]{"fe80::111%252"};
    m_hints.ai_protocol = 0;
    m_hints.ai_socktype = 0;

    // Test Unit
    ASSERT_EQ(::getaddrinfo(llascp, "https", &m_hints, &m_res), 0);

    // Have attention to the expectations not to be equal.
#ifdef _MSC_VER
    EXPECT_EQ(m_res->ai_protocol, 0);   // Return what is given with hints.
    EXPECT_EQ(m_res->ai_socktype, 0);   // Return what is given with hints.
    EXPECT_EQ(m_res->ai_next, nullptr); // No more entries.
#else
    EXPECT_NE(m_res->ai_protocol, 0);   // Report protocol used for socktype.
    EXPECT_NE(m_res->ai_socktype, 0);   // Report what current addrinfo has.
    EXPECT_NE(m_res->ai_next, nullptr); // More entries.
#endif
    EXPECT_EQ(m_res->ai_flags,
              compiler == CO::clang
                  ? 0
                  : (compiler == CO::msc ? AI_NUMERICHOST
                                         : AI_V4MAPPED | AI_NUMERICHOST));
    EXPECT_EQ(m_res->ai_family, AF_INET6);
    EXPECT_EQ(m_res->ai_addrlen, 28);
    EXPECT_EQ(m_res->ai_canonname, nullptr);
    ASSERT_NE(m_res->ai_addr, nullptr);
    auto sin6 = reinterpret_cast<sockaddr_in6*>(m_res->ai_addr);
    EXPECT_EQ(sin6->sin6_scope_id, 252);
    EXPECT_EQ(sin6->sin6_port, htons(443));
    ASSERT_NE(::inet_ntop(m_res->ai_family, &sin6->sin6_addr, m_addrbuf,
                          sizeof(m_addrbuf)),
              nullptr);
    EXPECT_STREQ(m_lla, m_addrbuf);
}

TEST_F(AddrinfoFTestSuite, verify_lla_with_protocol_6_socktype_0) {
    // Modified previous test to show dependencies. Specifying ai_protocol
    // restrics to only one addrinfo with SOCK_STREAM.

    constexpr char llascp[]{"fe80::111%252"};
    m_hints.ai_protocol = 6; // Protocol used for TCP.
    m_hints.ai_socktype = 0;

    // Test Unit
    ASSERT_EQ(::getaddrinfo(llascp, "https", &m_hints, &m_res), 0);

    // Have attention to the expectations be equal.
    EXPECT_EQ(m_res->ai_next, nullptr); // No more entries.
    EXPECT_EQ(m_res->ai_protocol, 6);   // Report protocol used for socktype.
#ifdef _MSC_VER
    EXPECT_EQ(m_res->ai_socktype, 0);   // Return what is given with hints.
#else
    EXPECT_EQ(m_res->ai_socktype, SOCK_STREAM); // Report from current addrinfo.
#endif
}

TEST_F(AddrinfoFTestSuite, verify_gua_with_valid_numeric_scope_id) {
    // Result same as verify_lla_with_valid_numeric_id.

    // Using only direct system calls.
    constexpr char guascp[]{"2001:db8::1%252"};

    // Test Unit
    m_hints.ai_protocol = 6;
    ASSERT_EQ(::getaddrinfo(guascp, "https", &m_hints, &m_res), 0);

    EXPECT_EQ(m_res->ai_flags,
              compiler == CO::clang
                  ? 0
                  : (compiler == CO::msc ? AI_NUMERICHOST
                                         : AI_V4MAPPED | AI_NUMERICHOST));
    EXPECT_EQ(m_res->ai_family, AF_INET6);
    EXPECT_EQ(m_res->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(m_res->ai_protocol, 6);
    EXPECT_EQ(m_res->ai_addrlen, 28);
    EXPECT_EQ(m_res->ai_canonname, nullptr);
    EXPECT_EQ(m_res->ai_next, nullptr);
    ASSERT_NE(m_res->ai_addr, nullptr);
    auto sin6 = reinterpret_cast<sockaddr_in6*>(m_res->ai_addr);
    EXPECT_EQ(sin6->sin6_scope_id, 252);
    EXPECT_EQ(sin6->sin6_port, htons(443));
    ASSERT_NE(::inet_ntop(m_res->ai_family, &sin6->sin6_addr, m_addrbuf,
                          sizeof(m_addrbuf)),
              nullptr);
    EXPECT_STREQ(m_gua, m_addrbuf);
}

TEST_F(AddrinfoFTestSuite, verify_gua_with_invalid_numeric_scope_id) {
    // Result same as verify_lla_with_invalid_numeric_id.

    constexpr char guascp[]{"2001:db8::1%-252"};

    // Test Unit
    m_hints.ai_protocol = 6;
    auto ret = ::getaddrinfo(guascp, "https", &m_hints, &m_res);

#ifdef __APPLE__
    ASSERT_EQ(ret, 0) << gai_strerror(ret);

    EXPECT_EQ(m_res->ai_flags, 0);
    EXPECT_EQ(m_res->ai_family, AF_INET6);
    EXPECT_EQ(m_res->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(m_res->ai_protocol, 6);
    EXPECT_EQ(m_res->ai_addrlen, 28);
    EXPECT_EQ(m_res->ai_canonname, nullptr);
    EXPECT_EQ(m_res->ai_next, nullptr);
    ASSERT_NE(m_res->ai_addr, nullptr);
    auto sin6 = reinterpret_cast<sockaddr_in6*>(m_res->ai_addr);
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, htons(443));
    ASSERT_NE(::inet_ntop(m_res->ai_family, &sin6->sin6_addr, m_addrbuf,
                          sizeof(m_addrbuf)),
              nullptr);
    EXPECT_STREQ(m_gua, m_addrbuf);
#else
    EXPECT_EQ(ret, EAI_NONAME) << gai_strerror(ret);
#endif
}

TEST_F(AddrinfoFTestSuite, verify_gua_with_valid_alphanum_scope_id) {
    // All supported platforms fail call getaddrinfo().

    // Get valid alpha-numeric scope_id
    CNetadapter naObj;
    ASSERT_NO_THROW(naObj.get_first());
    if (!naObj.find_first(UPnPsdk::CNetadapter::ADDRS::gua))
        GTEST_SKIP() << "No usable global unicast address found on any local "
                        "network adapter.";

    std::string guascp("2001:db8::1%" + naObj.name());

    // Test Unit
    m_hints.ai_protocol = 6;
    auto ret = ::getaddrinfo(guascp.c_str(), "https", &m_hints, &m_res);

    EXPECT_EQ(ret, EAI_NONAME) << gai_strerror(ret);
}

TEST_F(AddrinfoFTestSuite, verify_gua_with_invalid_alphanum_scope_id) {
    // Result same as verify_lla_with_invalid_alphanum_id.

    constexpr char guascp[]{"2001:db8::1%lozyx"};

    // Test Unit
    m_hints.ai_protocol = 6;
    auto ret = ::getaddrinfo(guascp, "https", &m_hints, &m_res);

#ifdef __APPLE__
    ASSERT_EQ(ret, 0) << gai_strerror(ret);

    EXPECT_EQ(m_res->ai_flags, 0);
    EXPECT_EQ(m_res->ai_family, AF_INET6);
    EXPECT_EQ(m_res->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(m_res->ai_protocol, 6);
    EXPECT_EQ(m_res->ai_addrlen, 28);
    EXPECT_EQ(m_res->ai_canonname, nullptr);
    EXPECT_EQ(m_res->ai_next, nullptr);
    ASSERT_NE(m_res->ai_addr, nullptr);
    auto sin6 = reinterpret_cast<sockaddr_in6*>(m_res->ai_addr);
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, htons(443));
    ASSERT_NE(::inet_ntop(m_res->ai_family, &sin6->sin6_addr, m_addrbuf,
                          sizeof(m_addrbuf)),
              nullptr);
    EXPECT_STREQ(m_gua, m_addrbuf);
#else
    EXPECT_EQ(ret, EAI_NONAME) << gai_strerror(ret);
#endif
}

TEST_F(AddrinfoFTestSuite, verify_gua_with_no_scope_id) {
    // Result same as verify_lla_with_no_id.

    // Test Unit
    m_hints.ai_protocol = 6;
    ASSERT_EQ(::getaddrinfo(m_gua, "https", &m_hints, &m_res), 0);

    EXPECT_EQ(m_res->ai_flags,
              compiler == CO::clang
                  ? 0
                  : (compiler == CO::msc ? AI_NUMERICHOST
                                         : AI_V4MAPPED | AI_NUMERICHOST));
    EXPECT_EQ(m_res->ai_family, AF_INET6);
    EXPECT_EQ(m_res->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(m_res->ai_protocol, 6);
    EXPECT_EQ(m_res->ai_addrlen, 28);
    EXPECT_EQ(m_res->ai_canonname, nullptr);
    EXPECT_EQ(m_res->ai_next, nullptr);
    ASSERT_NE(m_res->ai_addr, nullptr);
    auto sin6 = reinterpret_cast<sockaddr_in6*>(m_res->ai_addr);
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, htons(443));
    ASSERT_NE(::inet_ntop(m_res->ai_family, &sin6->sin6_addr, m_addrbuf,
                          sizeof(m_addrbuf)),
              nullptr);
    EXPECT_STREQ(m_gua, m_addrbuf);
}

TEST_F(AddrinfoFTestSuite, verify_gua_with_socktype_0) {
    // Result same as verify_lla_with_socktype_0.

    // Using only direct system calls.
    constexpr char guascp[]{"2001:db8::1%252"};
    m_hints.ai_protocol = 6; // Used for TCP restricts to SOCK_STREAM.
    m_hints.ai_socktype = 0;

    // Test Unit
    ASSERT_EQ(::getaddrinfo(guascp, "https", &m_hints, &m_res), 0);

    EXPECT_EQ(m_res->ai_flags,
              compiler == CO::clang
                  ? 0
                  : (compiler == CO::msc ? AI_NUMERICHOST
                                         : AI_V4MAPPED | AI_NUMERICHOST));
    EXPECT_EQ(m_res->ai_family, AF_INET6);
#ifdef _MSC_VER
    EXPECT_EQ(m_res->ai_socktype, 0); // Return what is given with hints.
#else
    EXPECT_NE(m_res->ai_socktype, 0); // Report what the current addrinfo has.
#endif
    EXPECT_EQ(m_res->ai_protocol, 6);
    EXPECT_EQ(m_res->ai_addrlen, 28);
    EXPECT_EQ(m_res->ai_canonname, nullptr);
    EXPECT_EQ(m_res->ai_next, nullptr); // No more entries.
    ASSERT_NE(m_res->ai_addr, nullptr);
    auto sin6 = reinterpret_cast<sockaddr_in6*>(m_res->ai_addr);
    EXPECT_EQ(sin6->sin6_scope_id, 252);
    EXPECT_EQ(sin6->sin6_port, htons(443));
    ASSERT_NE(::inet_ntop(m_res->ai_family, &sin6->sin6_addr, m_addrbuf,
                          sizeof(m_addrbuf)),
              nullptr);
    EXPECT_STREQ(m_gua, m_addrbuf);
}

TEST_F(AddrinfoFTestSuite, verify_loopback_with_valid_numeric_scope_id) {
    // Result same as verify_lla_with_valid_numeric_id.

    // Using only direct system calls.
    constexpr char lbkscp[]{"::1%252"};

    // Test Unit
    m_hints.ai_protocol = 6;
    ASSERT_EQ(::getaddrinfo(lbkscp, "https", &m_hints, &m_res), 0);

    EXPECT_EQ(m_res->ai_flags,
              compiler == CO::clang
                  ? 0
                  : (compiler == CO::msc ? AI_NUMERICHOST
                                         : AI_V4MAPPED | AI_NUMERICHOST));
    EXPECT_EQ(m_res->ai_family, AF_INET6);
    EXPECT_EQ(m_res->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(m_res->ai_protocol, 6);
    EXPECT_EQ(m_res->ai_addrlen, 28);
    EXPECT_EQ(m_res->ai_canonname, nullptr);
    EXPECT_EQ(m_res->ai_next, nullptr);
    ASSERT_NE(m_res->ai_addr, nullptr);
    auto sin6 = reinterpret_cast<sockaddr_in6*>(m_res->ai_addr);
    EXPECT_EQ(sin6->sin6_scope_id, 252);
    EXPECT_EQ(sin6->sin6_port, htons(443));
    ASSERT_NE(::inet_ntop(m_res->ai_family, &sin6->sin6_addr, m_addrbuf,
                          sizeof(m_addrbuf)),
              nullptr);
    EXPECT_STREQ(m_lbk, m_addrbuf);
}

TEST_F(AddrinfoFTestSuite, verify_loopback_with_invalid_numeric_scope_id) {
    // Result same as verify_lla_with_invalid_numeric_id.

    constexpr char lbkscp[]{"::1%-252"};

    // Test Unit
    m_hints.ai_protocol = 6;
    auto ret = ::getaddrinfo(lbkscp, "https", &m_hints, &m_res);

#ifdef __APPLE__
    ASSERT_EQ(ret, 0) << gai_strerror(ret);

    EXPECT_EQ(m_res->ai_flags, 0);
    EXPECT_EQ(m_res->ai_family, AF_INET6);
    EXPECT_EQ(m_res->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(m_res->ai_protocol, 6);
    EXPECT_EQ(m_res->ai_addrlen, 28);
    EXPECT_EQ(m_res->ai_canonname, nullptr);
    EXPECT_EQ(m_res->ai_next, nullptr);
    ASSERT_NE(m_res->ai_addr, nullptr);
    auto sin6 = reinterpret_cast<sockaddr_in6*>(m_res->ai_addr);
    // Scope_id is 0.
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, htons(443));
    ASSERT_NE(::inet_ntop(m_res->ai_family, &sin6->sin6_addr, m_addrbuf,
                          sizeof(m_addrbuf)),
              nullptr);
    EXPECT_STREQ(m_lbk, m_addrbuf);
#else
    EXPECT_EQ(ret, EAI_NONAME) << gai_strerror(ret);
#endif
}

TEST_F(AddrinfoFTestSuite, verify_loopback_with_valid_alphanum_scope_id) {
    // Only macOS succeeds getaddrinfo() call, other fail.
    // MacOS returns ai_flags set to 0.
    // MacOS returns specific ai_protocol number.
    // MacOS returns ai_addr.scope_id thats given with input address.

    // Get valid alpha-numeric scope_id
    CNetadapter naObj;
    ASSERT_NO_THROW(naObj.get_first());
    ASSERT_TRUE(naObj.find_first(UPnPsdk::CNetadapter::ADDRS::lo));

    std::string lbkscp("::1%" + naObj.name());

    // Test Unit
    m_hints.ai_protocol = 6;
    auto ret = ::getaddrinfo(lbkscp.c_str(), "https", &m_hints, &m_res);

#ifdef __APPLE__
    ASSERT_EQ(ret, 0) << gai_strerror(ret);

    EXPECT_EQ(m_res->ai_flags, 0);
    EXPECT_EQ(m_res->ai_family, AF_INET6);
    EXPECT_EQ(m_res->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(m_res->ai_protocol, 6);
    EXPECT_EQ(m_res->ai_addrlen, 28);
    EXPECT_EQ(m_res->ai_canonname, nullptr);
    EXPECT_EQ(m_res->ai_next, nullptr);
    ASSERT_NE(m_res->ai_addr, nullptr);
    auto sin6 = reinterpret_cast<sockaddr_in6*>(m_res->ai_addr);
    EXPECT_EQ(sin6->sin6_scope_id, naObj.index());
    EXPECT_EQ(sin6->sin6_port, htons(443));
    ASSERT_NE(::inet_ntop(m_res->ai_family, &sin6->sin6_addr, m_addrbuf,
                          sizeof(m_addrbuf)),
              nullptr);
    EXPECT_STREQ(m_lbk, m_addrbuf);
#else
    EXPECT_EQ(ret, EAI_NONAME) << gai_strerror(ret);
#endif
}

TEST_F(AddrinfoFTestSuite, verify_loopback_with_invalid_alphanum_scope_id) {
    // Result same as verify_lla_with_invalid_alphanum_id.

    constexpr char lbkscp[]{"::1%lozyx"};

    // Test Unit
    m_hints.ai_protocol = 6;
    auto ret = ::getaddrinfo(lbkscp, "https", &m_hints, &m_res);

#ifdef __APPLE__
    ASSERT_EQ(ret, 0) << gai_strerror(ret);

    EXPECT_EQ(m_res->ai_flags, 0);
    EXPECT_EQ(m_res->ai_family, AF_INET6);
    EXPECT_EQ(m_res->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(m_res->ai_protocol, 6);
    EXPECT_EQ(m_res->ai_addrlen, 28);
    EXPECT_EQ(m_res->ai_canonname, nullptr);
    EXPECT_EQ(m_res->ai_next, nullptr);
    ASSERT_NE(m_res->ai_addr, nullptr);
    auto sin6 = reinterpret_cast<sockaddr_in6*>(m_res->ai_addr);
    // Scope_id is 0.
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, htons(443));
    ASSERT_NE(::inet_ntop(m_res->ai_family, &sin6->sin6_addr, m_addrbuf,
                          sizeof(m_addrbuf)),
              nullptr);
    EXPECT_STREQ(m_lbk, m_addrbuf);
#else
    EXPECT_EQ(ret, EAI_NONAME) << gai_strerror(ret);
#endif
}

TEST_F(AddrinfoFTestSuite, verify_loopback_with_no_scope_id) {
    // Result same as verify_lla_with_no_id.

    // Test Unit
    m_hints.ai_protocol = 6;
    ASSERT_EQ(::getaddrinfo(m_lbk, "https", &m_hints, &m_res), 0);

    EXPECT_EQ(m_res->ai_flags,
              compiler == CO::clang
                  ? 0
                  : (compiler == CO::msc ? AI_NUMERICHOST
                                         : AI_V4MAPPED | AI_NUMERICHOST));
    EXPECT_EQ(m_res->ai_family, AF_INET6);
    EXPECT_EQ(m_res->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(m_res->ai_protocol, 6);
    EXPECT_EQ(m_res->ai_addrlen, 28);
    EXPECT_EQ(m_res->ai_canonname, nullptr);
    EXPECT_EQ(m_res->ai_next, nullptr);
    ASSERT_NE(m_res->ai_addr, nullptr);
    auto sin6 = reinterpret_cast<sockaddr_in6*>(m_res->ai_addr);
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, htons(443));
    ASSERT_NE(::inet_ntop(m_res->ai_family, &sin6->sin6_addr, m_addrbuf,
                          sizeof(m_addrbuf)),
              nullptr);
    EXPECT_STREQ(m_lbk, m_addrbuf);
}

TEST_F(AddrinfoFTestSuite, verify_loopback_with_socktype_0) {
    // Result same as verify_lla_with_socktype_0.

    // Using only direct system calls.
    constexpr char lbkscp[]{"::1%252"};
    m_hints.ai_protocol = 6; // Used for TCP restricts to SOCK_STREAM.
    m_hints.ai_socktype = 0;

    // Test Unit
    ASSERT_EQ(::getaddrinfo(lbkscp, "https", &m_hints, &m_res), 0);

    EXPECT_EQ(m_res->ai_flags,
              compiler == CO::clang
                  ? 0
                  : (compiler == CO::msc ? AI_NUMERICHOST
                                         : AI_V4MAPPED | AI_NUMERICHOST));
    EXPECT_EQ(m_res->ai_family, AF_INET6);
#ifdef _MSC_VER
    EXPECT_EQ(m_res->ai_socktype, 0); // Return what is given with hints.
#else
    EXPECT_NE(m_res->ai_socktype, 0); // Report what the current addrinfo has.
#endif
    EXPECT_EQ(m_res->ai_protocol, 6);
    EXPECT_EQ(m_res->ai_addrlen, 28);
    EXPECT_EQ(m_res->ai_canonname, nullptr);
    EXPECT_EQ(m_res->ai_next, nullptr); // No more entries.
    ASSERT_NE(m_res->ai_addr, nullptr);
    auto sin6 = reinterpret_cast<sockaddr_in6*>(m_res->ai_addr);
    EXPECT_EQ(sin6->sin6_scope_id, 252);
    EXPECT_EQ(sin6->sin6_port, htons(443));
    ASSERT_NE(::inet_ntop(m_res->ai_family, &sin6->sin6_addr, m_addrbuf,
                          sizeof(m_addrbuf)),
              nullptr);
    EXPECT_STREQ(m_lbk, m_addrbuf);
}
#endif


// Other tests
// -----------
enum struct Entry { no, one, more };
enum struct Error { yes = true, no = false };

// clang-format off
class NetaddrAssignTest
    : public ::testing::TestWithParam<std::tuple<
          //    netaddress              result              one entry    throw error
          const std::string_view, const std::string_view, const Entry, const Error>> {};
// clang-format on

TEST_P(NetaddrAssignTest, netaddress_assign) {
    // Get parameter
    std::tuple params = GetParam();

    // Test Unit
    CAddrinfo2 aiObj(std::get<0>(params), AI_NUMERICHOST);
    if (std::get<3>(params) == Error::yes) {
        EXPECT_NE(aiObj.get_first(), 0);
        EXPECT_EQ(std::get<2>(params), Entry::no);
    } else {
        EXPECT_EQ(aiObj.get_first(), 0);
        aiObj.sockaddr(saObj);
        EXPECT_EQ(saObj.netaddrp(), std::get<1>(params));
        if (std::get<2>(params) == Entry::one)
            EXPECT_FALSE(aiObj.get_next());
        else
            EXPECT_TRUE(aiObj.get_next());
    }
}

// --gtest_filter=NetaddrAssign/NetaddrAssignTest.netaddress_assign*
// clang-format off
INSTANTIATE_TEST_SUITE_P(
    NetaddrAssign, NetaddrAssignTest,
    ::testing::Values(
        // This Test checks the netaddress with port.
        // With an invalid address part the whole netaddress is unspecified,
        // except with the first following well defined unspecified addresses.
        // A valid address with an invalid port results to port 0.
 /*00*/ std::make_tuple("", "[::1]:0", Entry::one, Error::no), // default addr=nullptr, set port=0, result=localhost
 /*00*/ std::make_tuple(":", "[::1]:0", Entry::one, Error::no), // default addr=nullptr, set port=0, result=localhost
 /*00*/ std::make_tuple(":0", "[::1]:0", Entry::one, Error::no), // default addr=nullptr, set port=0, result=localhost
        std::make_tuple("::", "[::]:0", Entry::one, Error::no),
        std::make_tuple("[::]", "[::]:0", Entry::one, Error::no),
        std::make_tuple("[::]:", "[::]:0", Entry::one, Error::no),
        std::make_tuple("[::]:0", "[::]:0", Entry::one, Error::no),
        std::make_tuple("[::]:65535", "[::]:65535", Entry::one, Error::no), // port 0 to 65535
        // Following invalid address parts will be general unspecified ("").
        // std::make_tuple("", unspec, Entry::???, Error::no), // makes passive listening, tested later
        std::make_tuple("[", "", Entry::no, Error::yes),
        std::make_tuple("]", "", Entry::no, Error::yes),
        std::make_tuple("[]", "", Entry::no, Error::yes),
        std::make_tuple(".", "", Entry::no, Error::yes),
 /*10*/ std::make_tuple(".:", "", Entry::no, Error::yes),
        std::make_tuple(":.", "", Entry::no, Error::yes),
        std::make_tuple(":::", "", Entry::no, Error::yes),
        std::make_tuple("[::", "", Entry::no, Error::yes),
        std::make_tuple("::]", "", Entry::no, Error::yes),
        // std::make_tuple("[::1", "", Entry::one, Error::no), // tested later
        // std::make_tuple("::1]", "", Entry::one, Error::no), // tested later
        // std::make_tuple("", "[::1]:0", Entry::one, Error::no), // multiple results, tested later
        // std::make_tuple(":0", "[::1]:0", Entry::one, Error::no), // multiple results, tested later
        // std::make_tuple(":50987", "[::1]:50987", Entry::one, Error::no), // multiple results, tested later
        std::make_tuple("::1", "[::1]:0", Entry::one, Error::no),
        std::make_tuple("[::1]", "[::1]:0", Entry::one, Error::no),
        std::make_tuple("[::1]:", "[::1]:0", Entry::one, Error::no),
        std::make_tuple("[::1]:0", "[::1]:0", Entry::one, Error::no),
        // std::make_tuple("[::1].4", "", Entry::one, Error::no), // dot for colon, takes long time, mocked later
        std::make_tuple("127.0.0.1", "[::ffff:127.0.0.1]:0", Entry::one, Error::no),
 /*20*/ std::make_tuple("127.0.0.1:", "[::ffff:127.0.0.1]:0", Entry::one, Error::no),
        std::make_tuple("127.0.0.1:0", "[::ffff:127.0.0.1]:0", Entry::one, Error::no),
        // std::make_tuple("127.0.0.1.5", "", Entry::one, Error::no), // dot for colon, takes long time, mocked later
        std::make_tuple("[2001:db8::43]:", "[2001:db8::43]:0", Entry::one, Error::no),
        std::make_tuple("2001:db8::41:59897", "", Entry::no, Error::yes), // no brackets and wrong quad
        std::make_tuple("[2001:db8::fg]", "", Entry::no, Error::yes),
        std::make_tuple("[2001:db8::fg]:59877", "", Entry::no, Error::yes),
        // std::make_tuple("[2001:db8::42]:65535", "[2001:db8::42]:65535", Entry::one, Error::no), // tested later
        std::make_tuple("[2001:db8::51]:65536", "", Entry::no, Error::yes), // invalid port
        std::make_tuple("[2001:db8::52]:9999999999", "", Entry::no, Error::yes), // invalid port
        std::make_tuple("[2001:db8::52::53]", "", Entry::no, Error::yes), // double double colon
        std::make_tuple("[2001:db8::52::54]:65535", "", Entry::no, Error::yes), // double double colon
 /*30*/ std::make_tuple("[12.168.88.95]", "", Entry::no, Error::yes), // IPv4 address with brackets
        std::make_tuple("[12.168.88.96]:", "", Entry::no, Error::yes),
        std::make_tuple("[12.168.88.97]:9876", "", Entry::no, Error::yes),
        // std::make_tuple("192.168.88.98:59876", "192.168.88.98:59876", Entry::one, Error::no), // tested later
        std::make_tuple("192.168.88.99:65537", "", Entry::no, Error::yes), // invalid port
        // std::make_tuple("192.168.88.256:59866", "", Entry::one, Error::no), // tested later
        // std::make_tuple("192.168.88.91", "192.168.88.91:0", Entry::one, Error::no), // tested later
        // std::make_tuple("garbage:49493", "", Entry::one, Error::no), // triggers DNS lookup
        std::make_tuple("[garbage]:49494", "", Entry::no, Error::yes),
        std::make_tuple("[2001:db8::44]:https", "[2001:db8::44]:443", Entry::one, Error::no),
        // std::make_tuple("[2001:db8::44]:httpx", "", Entry::one, Error::no), // takes long time, mocked later
        std::make_tuple("192.168.88.98:http", "[::ffff:192.168.88.98]:80", Entry::one, Error::no),
        std::make_tuple("192.168.71.73%1:44:https", "", Entry::no, Error::yes),
        std::make_tuple("192.168.71.74%lo:44:https", "", Entry::no, Error::yes)
        // std::make_tuple("192.168.88.98:httpy", "", Entry::one, Error::no), // takes long time, mocked later
        // std::make_tuple("[fe80::5054:ff:fe7f:c021]", "[fe80::5054:ff:fe7f:c021%2]:0", Entry::one, Error::no), // fails, not portable
        // std::make_tuple("[fe80::5054:ff:fe7f:c021%ens1]", "[fe80::5054:ff:fe7f:c021%2]:0", Entry::one, Error::no), // succeeds, not portable
        // std::make_tuple("[2003:d5:270b:9000:5054:ff:fe7f:c021%3]", "[2003:d5:270b:9000:5054:ff:fe7f:c021%3]:0", Entry::one, Error::no), // succeeds, not porable
        // std::make_tuple("[2003:d5:270b:9000:5054:ff:fe7f:c021%2]", "[2003:d5:270b:9000:5054:ff:fe7f:c021%2]:0", Entry::one, Error::no), // succeeds, not porable
        // std::make_tuple("[2003:d5:270b:9000:5054:ff:fe7f:c021%ens1]", "[2003:d5:270b:9000:5054:ff:fe7f:c021%2]:0", Entry::one, Error::no) // fails, not porable
    ));

INSTANTIATE_TEST_SUITE_P(
    NetaddrAssignUnspecIp4Addr, NetaddrAssignTest,
    ::testing::Values(
        std::make_tuple("0.0.0.0", "[::ffff:0.0.0.0]:0", Entry::one, Error::no),
        std::make_tuple("0.0.0.0:", "[::ffff:0.0.0.0]:0", Entry::one, Error::no),
        std::make_tuple("0.0.0.0:0", "[::ffff:0.0.0.0]:0", Entry::one, Error::no),        // port 0 ...
        std::make_tuple("0.0.0.0:65535", "[::ffff:0.0.0.0]:65535", Entry::one, Error::no) // to 65535
));
// clang-format on


#ifdef _MSC_VER
// In contrast to other platforms ::getaddrinfo() on win32 does not create
// AI_V4MAPPED addresses with AI_NUMERICHOST set. The SDK only uses IPv6
// addresses. All IPv4 addresses are mapped to IPv6. There is only one
// combination with AF_INET6 and no AI_NUMERICHOST where win32 do AI_V4MAPPED.
// All others fail.
// clang-format off
class GetaddrinfoWin32Test
    : public ::testing::TestWithParam<
    // hints.ai_flags, hints.ai_family, res.ai_flags, res.ai_family, success, v4mapped
          std::tuple<const int, const int, const int, const int, const bool, const bool>> {};
// clang-format on

TEST_P(GetaddrinfoWin32Test, how_it_works) {
    // Get parameter
    const std::tuple params = GetParam();

    addrinfo hints{};
    addrinfo* res{nullptr};

    // AI_V4MAPPED(2048) | AI_NUMERICHOST(4) | AI_NUMERICSERV(?);
    hints.ai_flags = std::get<0>(params);
    hints.ai_family = std::get<1>(params);

    hints.ai_socktype = SOCK_STREAM;
    int rc = ::getaddrinfo("192.168.24.73", "50001", &hints, &res);
    if (rc != 0) {                 // no success detected
        if (std::get<4>(params)) { // success expected
            // Trigger a message but continue.
            EXPECT_EQ(rc, 0) << ::gai_strerror(rc);
        }
    } else {                        // success detected
        if (!std::get<4>(params)) { // success not expected
            // Trigger a message but continue.
            EXPECT_NE(rc, 0) << ::gai_strerror(rc);
        }
        EXPECT_EQ(res->ai_flags, std::get<2>(params));
        EXPECT_EQ(res->ai_family, std::get<3>(params));

        EXPECT_EQ(res->ai_socktype, SOCK_STREAM);
        EXPECT_EQ(res->ai_protocol, 0);
        EXPECT_EQ(res->ai_canonname, nullptr);
        EXPECT_EQ(res->ai_next, nullptr);
        ASSERT_NE(res->ai_addr, nullptr);
        bool v4mapped{false};
        if (res->ai_family == AF_INET6)
            v4mapped = IN6_IS_ADDR_V4MAPPED(
                &reinterpret_cast<sockaddr_in6*>(res->ai_addr)->sin6_addr);
        EXPECT_EQ(v4mapped, std::get<5>(params));

        freeaddrinfo(res);
        res = nullptr;
    }
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(GetaddrinfoWin32, GetaddrinfoWin32Test, ::testing::Values(
    // All tests are done with an IPv4 address (AF_INET) that should be
    // converted to an IPv4 mapped IPv6 with using different flags.
    //                 hints.ai_flags,        lhints.ai_family, res.ai_flags, res.ai_family, success, v4mapped
/*00*/ std::make_tuple(0,                            AF_INET6,  -1,             -1,       false, false), // No such host is known.
       std::make_tuple(0,                            AF_INET,   AI_NUMERICHOST, AF_INET,  true,  false),
       std::make_tuple(0,                            AF_UNSPEC, AI_NUMERICHOST, AF_INET,  true,  false),
       std::make_tuple(AI_NUMERICHOST,               AF_INET6,  -1,             -1,       false, false),
       std::make_tuple(AI_NUMERICHOST,               AF_INET,   AI_NUMERICHOST, AF_INET,  true,  false),
       std::make_tuple(AI_NUMERICHOST,               AF_UNSPEC, AI_NUMERICHOST, AF_INET,  true,  false),
       std::make_tuple(AI_V4MAPPED,                  AF_INET6,  0,              AF_INET6, true,  true),
       std::make_tuple(AI_V4MAPPED,                  AF_INET,   AI_NUMERICHOST, AF_INET,  true,  false),
       std::make_tuple(AI_V4MAPPED,                  AF_UNSPEC, AI_NUMERICHOST, AF_INET,  true,  false),
       std::make_tuple(AI_V4MAPPED | AI_NUMERICHOST, AF_INET6,  -1,             -1,       false, false), // No such host is known.
/*10*/ std::make_tuple(AI_V4MAPPED | AI_NUMERICHOST, AF_INET,   AI_NUMERICHOST, AF_INET,  true,  false),
       std::make_tuple(AI_V4MAPPED | AI_NUMERICHOST, AF_UNSPEC, AI_NUMERICHOST, AF_INET,  true,  false)
));
// clang-format on
#endif


TEST(AddrinfoTestSuite, empty_addrinfo_object) {
    SSockaddr empty_saObj;
    CAddrinfo2 aiObj("");

    aiObj.sockaddr(saObj);
    EXPECT_EQ(empty_saObj, saObj);

    EXPECT_FALSE(aiObj.get_next());

    aiObj.set_first();
    aiObj.sockaddr(saObj);
    EXPECT_EQ(empty_saObj, saObj);
}

TEST(AddrinfoTestSuite, lla_with_valid_numeric_scope_id) {
    // All platforms succeed getaddrinfo().
    // Each platform returns different ai_flags.
    // Win32 returns ai_protocol given by hint, other return specific number.
    // All platforms return ai_addr.scope_id that was given with input address.

    const std::string llascp{"[fe80::1%252]:50001"};

    // Test Unit
    CAddrinfo2 aiObj(llascp, AI_NUMERICHOST, SOCK_DGRAM);
    ASSERT_EQ(aiObj.get_first(), 0);

    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.ss.ss_family, AF_INET6);
    EXPECT_EQ(llascp, saObj.netaddrp());
    EXPECT_EQ(saObj.sin6.sin6_scope_id, 252);
    EXPECT_EQ(saObj.sin6.sin6_port, htons(50001));
}

TEST(AddrinfoTestSuite, invalid_netaddress_fails) {
    CAddrinfo2 aiObj("[fg80::1%252]:50001", AI_NUMERICHOST);
    EXPECT_NE(aiObj.get_first(), 0);
}

TEST(AddrinfoTestSuite, lla_without_scope_id_fails) {
    CAddrinfo2 aiObj("[fe80::1]:50001", AI_NUMERICHOST);
    EXPECT_NE(aiObj.get_first(), 0);
}

TEST(AddrinfoTestSuite, lla_with_0_scope_id_fails) {
    CAddrinfo2 aiObj("[fe80::1%0]:50001", AI_NUMERICHOST);
    EXPECT_NE(aiObj.get_first(), 0);
}

TEST(AddrinfoTestSuite, lla_with_known_netinterface_name_succeeds) {
    CNetadapter naObj;
    ASSERT_NO_THROW(naObj.get_first());
    ASSERT_TRUE(naObj.find_first(ADDRS::lla));

    std::string llascp("[fe80::1%" + naObj.name() + "]:50001");

    CAddrinfo2 aiObj(llascp, AI_NUMERICHOST);
    ASSERT_EQ(aiObj.get_first(), 0);
    aiObj.sockaddr(saObj);
    EXPECT_EQ(saObj.sin6.sin6_scope_id, naObj.index());
}

TEST(AddrinfoTestSuite, lla_with_unknown_netinterface_name_fails) {
    CAddrinfo2 aiObj("[fe80::1%zyx0]:50001", AI_NUMERICHOST);
    EXPECT_NE(aiObj.get_first(), 0);
}

TEST(AddrinfoTestSuite, gua_with_scope_id_fails) {
    CAddrinfo2 aiObj("[2001:db8::1%252]:50001", AI_NUMERICHOST);
    EXPECT_NE(aiObj.get_first(), 0);
}

TEST(AddrinfoTestSuite, gua_with_socktype_0_fails) {
    CAddrinfo2 aiObj("[2001:db8::1%252]:50001", AI_NUMERICHOST, 0);
    EXPECT_NE(aiObj.get_first(), 0);
}

TEST(AddrinfoTestSuite, loopback_with_scope_id_fails) {
    CAddrinfo2 aiObj("[::1%252]:50001", AI_NUMERICHOST);
    EXPECT_NE(aiObj.get_first(), 0);
}

TEST(AddrinfoTestSuite, get_info_loopback_interface) {
    // If node is not empty AI_PASSIVE is ignored.

    CAddrinfo2 ai1("[::1]:50001", AI_PASSIVE | AI_NUMERICHOST);
    EXPECT_EQ(ai1.get_first(), 0);
    ai1.sockaddr(saObj);
    EXPECT_EQ(saObj.netaddr(), "[::1]");
    EXPECT_EQ(saObj.sin6.sin6_scope_id, 0);
    EXPECT_EQ(saObj.port(), 50001);
    EXPECT_FALSE(ai1.get_next());
#if 0
    CAddrinfo2 ai5("[::1]:50085");
    EXPECT_EQ(ai5.get_first(), 0);
    ai5.sockaddr(saObj);
    EXPECT_EQ(saddr.netaddrp(), "[::1]:50085");

    // Test Unit
    CAddrinfo2 ai3("[::1]", "50087");
    EXPECT_EQ(ai3.get_first(), 0);

    EXPECT_EQ(ai3->ai_family, AF_INET6);
    EXPECT_EQ(ai3->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai3->ai_protocol, 0);
    EXPECT_EQ(ai3->ai_flags, AI_V4MAPPED);
    ai3.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[::1]:50087");

    // Test Unit, does not trigger a DNS query
    CAddrinfo2 ai4("localhost:50088");
    EXPECT_EQ(ai4.get_first(), 0);

    EXPECT_THAT(ai4->ai_family, AnyOf(AF_INET6, AF_INET));
    EXPECT_EQ(ai4->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai4->ai_protocol, 0);
    EXPECT_EQ(ai4->ai_flags, AI_V4MAPPED);
    ai4.sockaddr(saddr);
    EXPECT_THAT(saddr.netaddrp(), AnyOf("[::1]:50088", "127.0.0.1:50088"));

    // Test Unit
    CAddrinfo2 ai2("127.0.0.1", "50086", 0, SOCK_DGRAM);
    ASSERT_EQ(ai2.get_first(), 0);

    EXPECT_EQ(ai2->ai_socktype, SOCK_DGRAM);
    EXPECT_EQ(ai2->ai_protocol, 0);
    EXPECT_EQ(ai2->ai_flags, AI_V4MAPPED);
    ai2.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[::ffff:127.0.0.1]:50086");
    ASSERT_EQ(ai2->ai_family, AF_INET6);
    ASSERT_NE(ai2->ai_addr, nullptr);
    EXPECT_EQ(reinterpret_cast<sockaddr_in6*>(ai2->ai_addr)->sin6_scope_id, 0);
#endif
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
