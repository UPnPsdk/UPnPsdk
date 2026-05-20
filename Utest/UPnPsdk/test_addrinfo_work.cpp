// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-05-29

// I test different address infos that we get from system function
// ::getaddrinfo().

// Include source code for testing. So we have also direct access to static
// functions which need to be tested.
#include <UPnPsdk/src/net/addrinfo.cpp>

#include <UPnPsdk/socket.hpp>
#include <UPnPsdk/netadapter.hpp>

#include <utest/utest.hpp>
#include <umock/netdb_mock.hpp>


namespace utest {

using ::testing::_;
using ::testing::AnyOf;
using ::testing::Conditional;
using ::testing::DoAll;
using ::testing::ExitedWithCode;
using ::testing::Field;
using ::testing::HasSubstr;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;

using ::UPnPsdk::CAddrinfo;
using ::UPnPsdk::CNetadapter;
using ::UPnPsdk::g_dbug;
using ::UPnPsdk::SSockaddr;


namespace {

// General storage for temporary socket address evaluation
SSockaddr saddr;

constexpr int suppress_dns_lookup{AI_NUMERICHOST};

} // anonymous namespace


class AddrinfoMockFTestSuite : public ::testing::Test {
  protected:
    // Instantiate mocking object.
    StrictMock<umock::NetdbMock> m_netdbObj;
    // Inject the mocking object into the tested code.
    umock::Netdb netdb_injectObj = umock::Netdb(&m_netdbObj);

    // Constructor
    AddrinfoMockFTestSuite() {
        ON_CALL(m_netdbObj, getaddrinfo(_, _, _, _))
            .WillByDefault(Return(EAI_FAMILY));
    }
};


// Other tests
// -----------
TEST(AddrinfoTestSuite, lla_with_valid_numeric_id_successful) {
    CAddrinfo ai("[fe80::acd%252]:https");
    ASSERT_TRUE(ai.get_first());

    EXPECT_EQ(ai->ai_protocol, 0);
    EXPECT_EQ(ai->ai_flags, AI_V4MAPPED);
    EXPECT_EQ(ai->ai_addrlen, 28);
    ASSERT_NE(ai->ai_addr, nullptr);
    ai.sockaddr(saddr);
    EXPECT_EQ(saddr.sin6.sin6_scope_id, 252);
    EXPECT_EQ(saddr.netaddrp(), "[fe80::acd%252]:443");
    EXPECT_EQ(ai->ai_canonname, nullptr);
    EXPECT_EQ(ai->ai_next, nullptr);
}

TEST(AddrinfoTestSuite, query_ipv6_addrinfo_successful) {
    CAddrinfo ai1("[2001:db8::8%1]:50001");

    // Check the initialized object. This is what we have given with the
    // constructor. We get just the initialized hints.
    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_V4MAPPED);
    EXPECT_EQ(ai1->ai_addrlen, 0);
    EXPECT_EQ(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    // There is no address information
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), ":0");
    EXPECT_EQ(ai1->ai_next, nullptr);

    ASSERT_TRUE(ai1.get_first());

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_V4MAPPED);
    EXPECT_EQ(ai1->ai_addrlen, 28);
    EXPECT_NE(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[2001:db8::8]:50001");
    EXPECT_EQ(ai1->ai_next, nullptr);
    EXPECT_EQ(ai1.what(), "Success.");
}

TEST_F(AddrinfoMockFTestSuite, query_ipv6_addrinfo_successful) {
    UPnPsdk::sockaddr_t sockaddr{};
    sockaddr.ss.ss_family = AF_INET6;

    ::addrinfo res;

    res.ai_flags = 0;
    res.ai_family = AF_INET6;
    res.ai_socktype = SOCK_STREAM;
    res.ai_protocol = 0;
    res.ai_addrlen = sizeof(sockaddr.sin6);
    res.ai_addr = &sockaddr.sa;
    res.ai_canonname = nullptr;
    res.ai_next = nullptr;

    // Mock 'CAddrinfo::get_first()'
    EXPECT_CALL(m_netdbObj,
                getaddrinfo(Pointee(*"2001:db8::9"), nullptr,
                            Field(&addrinfo::ai_flags, AI_V4MAPPED), _))
        .WillOnce(DoAll(SetArgPointee<3>(&res), Return(0)));
    // Mock 'freeaddrinfo()'
    EXPECT_CALL(m_netdbObj, freeaddrinfo(&res)).Times(1);

    // Test Unit
    CAddrinfo ai("[2001:db8::9]");
    ASSERT_TRUE(ai.get_first());
    EXPECT_EQ(ai.what(), "Success.");
}

TEST_F(AddrinfoMockFTestSuite, query_addrinfo_url_with_service_successful) {
    ::addrinfo res3;
    UPnPsdk::sockaddr_t ss{};
    res3.ai_flags = 0;
    res3.ai_family = AF_INET6;
    res3.ai_socktype = SOCK_STREAM;
    res3.ai_protocol = 0;
    res3.ai_addrlen = sizeof(ss.sin6);
    res3.ai_addr = &ss.sa;
    res3.ai_canonname = nullptr;
    res3.ai_next = nullptr;

    // Mock 'CAddrinfo::get_first()'
    EXPECT_CALL(m_netdbObj,
                getaddrinfo(Pointee(*"www.excample.com"), Pointee(*"https"),
                            AllOf(Field(&addrinfo::ai_family, AF_INET6),
                                  Field(&addrinfo::ai_flags, AI_V4MAPPED)),
                            _))
        .WillOnce(DoAll(SetArgPointee<3>(&res3), Return(0)));
    // Mock 'freeaddrinfo()'
    EXPECT_CALL(m_netdbObj, freeaddrinfo(_)).Times(1);

    // Test Unit
    CAddrinfo ai("www.example.com:https");
    ASSERT_TRUE(ai.get_first());

    EXPECT_EQ(ai->ai_family, AF_INET6);
    EXPECT_EQ(ai->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai->ai_protocol, 0);
    EXPECT_EQ(ai->ai_flags, AI_V4MAPPED);
    EXPECT_EQ(ai->ai_addrlen, 28);
    EXPECT_EQ(ai->ai_addr, &ss.sa);
    EXPECT_EQ(ai->ai_canonname, nullptr);
    EXPECT_EQ(ai->ai_next, nullptr);
    ai.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), ":0");
    EXPECT_EQ(ai.what(), "Success.");
}

TEST(AddrinfoTestSuite, load_ipv6_addrinfo_and_port_successful) {
    CAddrinfo ai1("[2001:db8::14]", "https");

    ASSERT_TRUE(ai1.get_first());

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    // EXPECT_EQ(ai1->ai_flags, AI_NUMERICHOST);
    EXPECT_EQ(ai1->ai_flags, AI_V4MAPPED);
    EXPECT_EQ(ai1->ai_addrlen, 28);
    EXPECT_NE(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    EXPECT_EQ(ai1->ai_next, nullptr);
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[2001:db8::14]:443");
}

TEST(AddrinfoTestSuite, load_ipv6_addrinfo_with_port_successful) {
    CAddrinfo ai1("[2001:db8::15]:59877");

    ASSERT_TRUE(ai1.get_first());

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    // EXPECT_EQ(ai1->ai_flags, AI_NUMERICHOST | AI_NUMERICSERV);
    EXPECT_EQ(ai1->ai_flags, AI_V4MAPPED);
    EXPECT_EQ(ai1->ai_addrlen, 28);
    EXPECT_NE(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    EXPECT_EQ(ai1->ai_next, nullptr);
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[2001:db8::15]:59877");
}

TEST(AddrinfoTestSuite, query_ipv4_addrinfo_fails) {
    CAddrinfo ai1("192.168.200.201");

    // Check the initialized object. This is what we have given with the
    // constructor. We get just the initialized hints.
    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_V4MAPPED);
    EXPECT_EQ(ai1->ai_addrlen, 0);
    EXPECT_EQ(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    EXPECT_EQ(ai1->ai_next, nullptr);
    // There is no address information
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), ":0");

    ASSERT_TRUE(ai1.get_first());

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_V4MAPPED);
    EXPECT_EQ(ai1->ai_addrlen, 28);
    EXPECT_NE(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    EXPECT_EQ(ai1->ai_next, nullptr);
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[::ffff:192.168.200.201]:0");
}

TEST(AddrinfoTestSuite, load_ipv4_addrinfo_and_port_fails) {
    CAddrinfo ai1("192.168.200.202", "54544");

    EXPECT_TRUE(ai1.get_first());

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_V4MAPPED);
    EXPECT_EQ(ai1->ai_addrlen, 28);
    EXPECT_NE(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    EXPECT_EQ(ai1->ai_next, nullptr);
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[::ffff:192.168.200.202]:54544");
}

TEST(AddrinfoTestSuite, load_ipv4_addrinfo_with_port_fails) {
    CAddrinfo ai1("192.168.200.203:http");

    ASSERT_TRUE(ai1.get_first());

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_V4MAPPED);
    EXPECT_EQ(ai1->ai_addrlen, 28);
    EXPECT_NE(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    EXPECT_EQ(ai1->ai_next, nullptr);
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[::ffff:192.168.200.203]:80");
}

TEST(AddrinfoTestSuite, double_set_addrinfo_successful) {
    // If node is not empty AI_PASSIVE is ignored.
    // Flag AI_NUMERICHOST should be set if possible to avoid expensive name
    // resolution from external DNS server.

    // Test Unit with numeric port number
    CAddrinfo ai2("[2001:db8::2]", "50048", AI_PASSIVE | AI_NUMERICHOST);
    ASSERT_TRUE(ai2.get_first());

    // Returns what ::getaddrinfo() returns.
    EXPECT_EQ(ai2->ai_family, AF_INET6);
    // Returns what ::getaddrinfo() returns.
    EXPECT_EQ(ai2->ai_socktype, SOCK_STREAM);
    // Different on platforms: Ubuntu & MacOS return 6, win32 returns 0.
    // We just return that what was requested by the user.
    EXPECT_EQ(ai2->ai_protocol, 0);
    // Different on platforms: Ubuntu returns 1025, MacOS & win32 return 0.
    // We just return that what was requested by the user.
    // EXPECT_EQ(ai2->ai_flags, AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV);
    EXPECT_EQ(ai2->ai_flags, AI_V4MAPPED | AI_PASSIVE | AI_NUMERICHOST);
    // Returns what ::getaddrinfo() returns.
    ai2.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[2001:db8::2]:50048");
    // Returns what ::getaddrinfo() returns.
    EXPECT_EQ(ai2->ai_next, nullptr);

    // Test Unit
    // Getting the address information again is possible but not very useful.
    // Because the same node, service and hints are used the result is exactly
    // the same as before.
    const int* old_res{&ai2->ai_flags};
    ASSERT_TRUE(ai2.get_first());

    EXPECT_NE(old_res, &ai2->ai_flags);
    EXPECT_EQ(ai2->ai_family, AF_INET6);
    EXPECT_EQ(ai2->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai2->ai_protocol, 0);
    // EXPECT_EQ(ai2->ai_flags, AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV);
    EXPECT_EQ(ai2->ai_flags, AI_V4MAPPED | AI_PASSIVE | AI_NUMERICHOST);
    ai2.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[2001:db8::2]:50048");
    EXPECT_EQ(ai2->ai_next, nullptr);
    EXPECT_EQ(ai2.what(), "Success.");
}

TEST(AddrinfoTestSuite, instantiate_not_load_numeric_host_successful) {
    // If node is not empty AI_PASSIVE is ignored.
    // Flag AI_NUMERICHOST should be set if possible to avoid expensive name
    // resolution from external DNS server.

    // Test Unit with numeric port number
    CAddrinfo ai1("[2001:db8::1]", "50050", AI_PASSIVE | AI_NUMERICHOST);

    // Check the initialized object without address information. This is what
    // we have given with the constructor. We get just the initialized hints.
    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_V4MAPPED | AI_PASSIVE | AI_NUMERICHOST);
    EXPECT_EQ(ai1->ai_addrlen, 0);
    EXPECT_EQ(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    EXPECT_EQ(ai1->ai_next, nullptr);
    // There is no address information
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), ":0");
    EXPECT_EQ(ai1.what(), "Success.");
}

TEST(AddrinfoTestSuite, get_implicit_address_family) {
    // It is not needed to set the address family to AF_UNSPEC. That is used by
    // default.

    // Test Unit
    CAddrinfo ai1("[2001:db8::5]", "50051");
    ASSERT_TRUE(ai1.get_first());

    EXPECT_EQ(ai1->ai_family, AF_INET6); // set by syscal ::getaddrinfo
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_V4MAPPED);
    EXPECT_EQ(ai1->ai_next, nullptr);
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[2001:db8::5]:50051");

    // Test Unit
    CAddrinfo ai2("192.168.9.10", "50096");
    ASSERT_TRUE(ai2.get_first());

    EXPECT_EQ(ai2->ai_family, AF_INET6); // IPv4 mapped IPv6
    EXPECT_EQ(ai2->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai2->ai_protocol, 0);
    EXPECT_EQ(ai2->ai_flags, AI_V4MAPPED);
    EXPECT_EQ(ai2->ai_next, nullptr);
    ai2.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[::ffff:192.168.9.10]:50096");
}

TEST(AddrinfoTestSuite, get_unknown_numeric_host_fails) {
    // With AI_NUMERICHOST "localhost" is unknown. Name resolving does not
    // trigger a DNS query.

    // Test Unit
    CAddrinfo ai1("localhost", "50031", AI_NUMERICHOST);
    EXPECT_FALSE(ai1.get_first());
    EXPECT_THAT(ai1.what(), HasSubstr("] WHAT MSG1112: errid("));

    // Does not call ::getaddrinfo(), because invalid numeric IPv6 is detected
    // before.
    CAddrinfo ai2("localhost:50052", AI_NUMERICHOST, SOCK_DGRAM);
    EXPECT_FALSE(ai2.get_first());
    EXPECT_THAT(ai2.what(), HasSubstr("] WHAT MSG1112: errid("));

    CAddrinfo ai3("localhost", "50053", AI_NUMERICHOST);
    EXPECT_FALSE(ai3.get_first());
    EXPECT_THAT(ai3.what(), HasSubstr("] WHAT MSG1112: errid("));
}

TEST(AddrinfoTestSuite, get_unknown_alphanumeric_host_fails) {
    CAddrinfo ai1("[localhost]", "50055", suppress_dns_lookup, SOCK_DGRAM);
    EXPECT_FALSE(ai1.get_first());
    EXPECT_THAT(ai1.what(), HasSubstr("] WHAT MSG1112: errid("));

    CAddrinfo ai2("[localhost]:50005", suppress_dns_lookup, SOCK_DGRAM);
    EXPECT_FALSE(ai2.get_first());
    EXPECT_THAT(ai2.what(), HasSubstr("] WHAT MSG1112: errid("));
}

TEST_F(AddrinfoMockFTestSuite, get_addrinfo_out_of_memory) {
    // Mock CAddrinfo::get_first()
    EXPECT_CALL(m_netdbObj,
                getaddrinfo(Pointee(*"localhost"), Pointee(*"50118"),
                            Field(&addrinfo::ai_flags, AI_V4MAPPED), _))
        .WillOnce(Return(EAI_MEMORY));
    // Mock 'freeaddrinfo()'
    EXPECT_CALL(m_netdbObj, freeaddrinfo(_)).Times(0);

    // Test Unit
    CAddrinfo ai("localhost", "50118");
    EXPECT_FALSE(ai.get_first());
    EXPECT_THAT(ai.what(), HasSubstr("] WHAT MSG1112: errid("));
}

TEST_F(AddrinfoMockFTestSuite, get_addrinfo_invalid_ipv4_address) {
    // This test triggers a DNS lookup, so I mock it.
    // Mock 'CAddrinfo::get_first()'
    EXPECT_CALL(m_netdbObj,
                getaddrinfo(Pointee(*"192.168.88.256"), Pointee(*"59866"),
                            Field(&addrinfo::ai_flags, AI_V4MAPPED), _))
        .WillOnce(Return(EAI_NONAME));
    // Mock 'freeaddrinfo()'
    EXPECT_CALL(m_netdbObj, freeaddrinfo(_)).Times(0);

    // Test Unit
    CAddrinfo ai("192.168.88.256:59866");
    EXPECT_FALSE(ai.get_first());
    EXPECT_THAT(ai.what(), HasSubstr("] WHAT MSG1112: errid("));
}

TEST_F(AddrinfoMockFTestSuite, get_addrinfo_ipv6_service_dot_instead_colon) {
    // Looking for mistaken service tooks long time, so I mock it.
    ::addrinfo res{};
    res.ai_family = AF_INET6;

    // Mock 'CAddrinfo::get_first()'
    EXPECT_CALL(m_netdbObj,
                getaddrinfo(Pointee(*"[::1].4"), nullptr,
                            Field(&addrinfo::ai_flags, AI_V4MAPPED), _))
        .WillOnce(Return(EAI_NONAME));
    // Mock 'freeaddrinfo()'
    EXPECT_CALL(m_netdbObj, freeaddrinfo(&res)).Times(0);

    // Test Unit
    CAddrinfo ai("[::1].4");
    EXPECT_FALSE(ai.get_first());
}

TEST_F(AddrinfoMockFTestSuite, get_addrinfo_ipv4_service_dot_for_colon) {
    // Looking for mistaken service tooks long time, so I mock it.
    ::addrinfo res{};

    // Mock 'CAddrinfo::get_first()'
    EXPECT_CALL(m_netdbObj,
                getaddrinfo(Pointee(*"127.0.0.1.5"), nullptr,
                            Field(&addrinfo::ai_flags, AI_V4MAPPED), _))
        .WillOnce(Return(EAI_NONAME));
    // Mock 'freeaddrinfo()'
    EXPECT_CALL(m_netdbObj, freeaddrinfo(&res)).Times(0);

    // Test Unit
    CAddrinfo ai("127.0.0.1.5");
    EXPECT_FALSE(ai.get_first());
}

TEST_F(AddrinfoMockFTestSuite, get_addrinfo_ipv6_service_undefined_alpha_name) {
    // Looking for undefined service tooks long time, so I mock it.
    ::addrinfo res{};
    res.ai_family = AF_INET6;

    // Mock 'CAddrinfo::get_first()'
    EXPECT_CALL(m_netdbObj,
                getaddrinfo(Pointee(*"2001:db8::44"), Pointee(*"httpx"),
                            Field(&addrinfo::ai_flags, AI_V4MAPPED), _))
        .WillOnce(Return(EAI_SERVICE));
    // Mock 'freeaddrinfo()'
    EXPECT_CALL(m_netdbObj, freeaddrinfo(&res)).Times(0);

    // Test Unit
    CAddrinfo ai("[2001:db8::44]:httpx");
    EXPECT_FALSE(ai.get_first());
}

TEST_F(AddrinfoMockFTestSuite, get_addrinfo_ipv4_service_undefined_alpha_name) {
    // Looking for undefined service tooks long time, so I mock it.
    ::addrinfo res{};
    res.ai_family = AF_INET;

    // Mock 'CAddrinfo::get_first()'
    EXPECT_CALL(m_netdbObj,
                getaddrinfo(Pointee(*"192.168.88.98"), Pointee(*"httpy"),
                            Field(&addrinfo::ai_flags, AI_V4MAPPED), _))
        .WillOnce(Return(EAI_SERVICE));
    // Mock 'freeaddrinfo()'
    EXPECT_CALL(m_netdbObj, freeaddrinfo(&res)).Times(0);

    // Test Unit
    CAddrinfo ai("192.168.88.98:httpy");
    EXPECT_FALSE(ai.get_first());
}

TEST(AddrinfoTestSuite, invalid_ipv6_node_address) {
    // Test Unit.
    CAddrinfo ai2("[::1", "", AI_NUMERICHOST | AI_NUMERICSERV, 0);
    EXPECT_FALSE(ai2.get_first());

    CAddrinfo ai3("::1]", "", AI_NUMERICHOST | AI_NUMERICSERV, 0);
    EXPECT_FALSE(ai3.get_first());
}

TEST(AddrinfoTestSuite, get_unknown_ipv6_node_address) {
    CAddrinfo ai3("[::]", "0", AI_NUMERICHOST | AI_NUMERICSERV, 0);

    EXPECT_EQ(ai3->ai_family, AF_INET6); // Fix given by specification
    EXPECT_EQ(ai3->ai_socktype, 0);
    EXPECT_EQ(ai3->ai_protocol, 0);
    EXPECT_EQ(ai3->ai_flags, AI_V4MAPPED | AI_NUMERICHOST | AI_NUMERICSERV);
    EXPECT_EQ(ai3->ai_addrlen, 0);
    EXPECT_EQ(ai3->ai_addr, nullptr);
    EXPECT_EQ(ai3->ai_canonname, nullptr);
    EXPECT_EQ(ai3->ai_next, nullptr);
    ai3.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), ":0");

    ASSERT_TRUE(ai3.get_first());

    bool double_res1{false};
    [[maybe_unused]] bool double_res2{false};
    [[maybe_unused]] bool double_res3{false};
    do {
        EXPECT_EQ(ai3->ai_protocol, 0);
        EXPECT_EQ(ai3->ai_flags, AI_V4MAPPED | AI_NUMERICHOST | AI_NUMERICSERV);
        EXPECT_NE(ai3->ai_addr, nullptr); // not equal nullptr
        EXPECT_EQ(ai3->ai_canonname, nullptr);
#if !defined(_MSC_VER)
        if (ai3->ai_family == AF_INET6 && ai3->ai_socktype == SOCK_STREAM) {
            ASSERT_FALSE(double_res1);
            double_res1 = true;
            EXPECT_EQ(ai3->ai_addrlen, 28);
            ai3.sockaddr(saddr);
            EXPECT_EQ(saddr.netaddrp(), "[::]:0");
        } else if (ai3->ai_family == AF_INET6 &&
                   ai3->ai_socktype == SOCK_DGRAM) {
            ASSERT_FALSE(double_res2);
            double_res2 = true;
            EXPECT_EQ(ai3->ai_addrlen, 28);
            ai3.sockaddr(saddr);
            EXPECT_EQ(saddr.netaddrp(), "[::]:0");
#if !defined(__APPLE__)
        } else if (ai3->ai_family == AF_INET6 && ai3->ai_socktype == SOCK_RAW) {
            ASSERT_FALSE(double_res3);
            double_res3 = true;
            EXPECT_EQ(ai3->ai_addrlen, 28);
            ai3.sockaddr(saddr);
            EXPECT_EQ(saddr.netaddrp(), "[::]:0");
#endif
#else // _MSC_VER
        if (ai3->ai_family == AF_INET6 && ai3->ai_socktype == 0) {
            ASSERT_FALSE(double_res1);
            double_res1 = true;
            EXPECT_EQ(ai3->ai_addrlen, 28);
            ai3.sockaddr(saddr);
            EXPECT_EQ(saddr.netaddrp(), "[::]:0");
#endif
        } else {
            GTEST_FAIL()
                << "  Unexpected address information: address family = "
                << ai3->ai_family << ", socket type = " << ai3->ai_socktype
                << "\n";
        }
    } while (ai3.get_next());

    // Call another one for testing.
    EXPECT_FALSE(ai3.get_next());
}

TEST(AddrinfoTestSuite, get_unknown_ipv4_node_address) {
    CAddrinfo ai4("0.0.0.0", "0", AI_NUMERICHOST | AI_NUMERICSERV, 0);

    EXPECT_EQ(ai4->ai_family, AF_INET6); // Fix given by specification
    EXPECT_EQ(ai4->ai_socktype, 0);
    EXPECT_EQ(ai4->ai_protocol, 0);
    EXPECT_EQ(ai4->ai_flags, AI_V4MAPPED | AI_NUMERICHOST | AI_NUMERICSERV);
    EXPECT_EQ(ai4->ai_addrlen, 0);
    EXPECT_EQ(ai4->ai_addr, nullptr);
    EXPECT_EQ(ai4->ai_canonname, nullptr);
    EXPECT_EQ(ai4->ai_next, nullptr);
    ai4.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), ":0");

    ASSERT_TRUE(ai4.get_first());

    bool double_res1{false};
    [[maybe_unused]] bool double_res2{false};
    [[maybe_unused]] bool double_res3{false};

    do {
        EXPECT_EQ(ai4->ai_protocol, 0);
        EXPECT_EQ(ai4->ai_flags, AI_V4MAPPED | AI_NUMERICHOST | AI_NUMERICSERV);
        EXPECT_NE(ai4->ai_addr, nullptr); // not equal nullptr
        EXPECT_EQ(ai4->ai_canonname, nullptr);
#if !defined(_MSC_VER)
        if (ai4->ai_family == AF_INET6 && ai4->ai_socktype == SOCK_STREAM) {
            ASSERT_FALSE(double_res1);
            double_res1 = true;
            EXPECT_EQ(ai4->ai_addrlen, 28);
            ai4.sockaddr(saddr);
            EXPECT_EQ(saddr.netaddrp(), "[::ffff:0.0.0.0]:0");
        } else if (ai4->ai_family == AF_INET6 &&
                   ai4->ai_socktype == SOCK_DGRAM) {
            ASSERT_FALSE(double_res2);
            double_res2 = true;
            EXPECT_EQ(ai4->ai_addrlen, 28);
            ai4.sockaddr(saddr);
            EXPECT_EQ(saddr.netaddrp(), "[::ffff:0.0.0.0]:0");
#if !defined(__APPLE__)
        } else if (ai4->ai_family == AF_INET6 && ai4->ai_socktype == SOCK_RAW) {
            ASSERT_FALSE(double_res3);
            double_res3 = true;
            EXPECT_EQ(ai4->ai_addrlen, 28);
            ai4.sockaddr(saddr);
            EXPECT_EQ(saddr.netaddrp(), "[::ffff:0.0.0.0]:0");
#endif
#else // _MSC_VER
        if (ai4->ai_family == AF_INET6 && ai4->ai_socktype == 0) {
            ASSERT_FALSE(double_res1);
            double_res1 = true;
            EXPECT_EQ(ai4->ai_addrlen, 28);
            ai4.sockaddr(saddr);
            // Shows mapped part in hex, not in num base 10.
            EXPECT_EQ(saddr.netaddrp(), "[::ffff:0:0]:0");
#endif
        } else {
            GTEST_FAIL()
                << "  Unexpected address information: address family = "
                << ai4->ai_family << ", socket type = " << ai4->ai_socktype
                << "\n";
        }
    } while (ai4.get_next());

    // Call another one for testing.
    EXPECT_FALSE(ai4.get_next());
}

TEST(AddrinfoTestSuite, get_passive_node_address) {
    // To get a passive address info, node must be empty ("") otherwise flag
    // AI_PASSIVE is ignored.

    { // Scoped to reduce memory usage for testing with node "".
        // Test Unit for default AF_UNSPEC. This is equal to next test with
        // CAddrinfo ai1("", "0", AI_PASSIVE);
        CAddrinfo ai0("", AI_PASSIVE);
        ASSERT_TRUE(ai0.get_first());

        // Ubuntu returns AF_INET, MacOS and Microsoft Windows return AF_INET6
        EXPECT_THAT(ai0->ai_family, AnyOf(AF_INET6, AF_INET));
        EXPECT_EQ(ai0->ai_socktype, SOCK_STREAM);
        EXPECT_EQ(ai0->ai_protocol, 0);
        EXPECT_EQ(ai0->ai_flags, AI_V4MAPPED | AI_PASSIVE);
        ai0.sockaddr(saddr);
        EXPECT_THAT(saddr.netaddrp(), AnyOf("[::]:0", "0.0.0.0:0"));

        // Test Unit for default AF_UNSPEC
        CAddrinfo ai1("", "0", AI_PASSIVE);
        ASSERT_TRUE(ai1.get_first());

        // Ubuntu returns AF_INET, MacOS and Microsoft Windows return AF_INET6
        EXPECT_THAT(ai1->ai_family, AnyOf(AF_INET6, AF_INET));
        EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
        EXPECT_EQ(ai1->ai_protocol, 0);
        EXPECT_EQ(ai1->ai_flags, AI_V4MAPPED | AI_PASSIVE);
        ai1.sockaddr(saddr);
        EXPECT_THAT(saddr.netaddrp(), AnyOf("[::]:0", "0.0.0.0:0"));

        // Test Unit for default AF_UNSPEC
        CAddrinfo ai2("", "50106", AI_PASSIVE);
        ASSERT_TRUE(ai2.get_first());

        // Ubuntu returns AF_INET, MacOS and Microsoft Windows return AF_INET6
        EXPECT_THAT(ai2->ai_family, AnyOf(AF_INET6, AF_INET));
        EXPECT_EQ(ai2->ai_socktype, SOCK_STREAM);
        EXPECT_EQ(ai2->ai_protocol, 0);
        EXPECT_EQ(ai2->ai_flags, AI_V4MAPPED | AI_PASSIVE);
        ai2.sockaddr(saddr);
        EXPECT_THAT(saddr.netaddrp(), AnyOf("[::]:50106", "0.0.0.0:50106"));

        // Test Unit for default AF_UNSPEC
        CAddrinfo ai3("", "50107", AI_PASSIVE | AI_NUMERICHOST,
                      0 /*ai_socktype*/);
        ASSERT_TRUE(ai3.get_first());

        // Ubuntu returns AF_INET, MacOS and Microsoft Windows return AF_INET6
        EXPECT_THAT(ai3->ai_family, AnyOf(AF_INET6, AF_INET));
#ifdef __APPLE__
        EXPECT_EQ(ai3->ai_socktype, SOCK_DGRAM);
#elif defined(_MSC_VER)
        EXPECT_EQ(ai3->ai_socktype, 0);
#else
        EXPECT_EQ(ai3->ai_socktype, SOCK_STREAM);
#endif
        EXPECT_EQ(ai3->ai_protocol, 0);
        EXPECT_EQ(ai3->ai_flags, AI_V4MAPPED | AI_PASSIVE | AI_NUMERICHOST);
        ai3.sockaddr(saddr);
        EXPECT_THAT(saddr.netaddrp(), AnyOf("[::]:50107", "0.0.0.0:50107"));
    }

    { // Scoped to reduce memory usage for testing.
        // Test Unit
        // Using explicit the unknown netaddress should definetly return the
        // in6addr_any passive listening socket info.
        CAddrinfo ai1("[::]", "50006", AI_PASSIVE | AI_NUMERICHOST, SOCK_DGRAM);
        ASSERT_TRUE(ai1.get_first());

        EXPECT_EQ(ai1->ai_family, AF_INET6);
        EXPECT_EQ(ai1->ai_socktype, SOCK_DGRAM);
        EXPECT_EQ(ai1->ai_protocol, 0);
        EXPECT_EQ(ai1->ai_flags, AI_V4MAPPED | AI_PASSIVE | AI_NUMERICHOST);
        // This will listen on all local network interfaces.
        ai1.sockaddr(saddr);
        EXPECT_EQ(saddr.netaddrp(), "[::]:50006");

        // Test Unit
        // Using explicit the unknown IPv4 netaddress should return the IPv4
        // mapped IPv6 address.
        CAddrinfo ai2("0.0.0.0", "50032", AI_PASSIVE | AI_NUMERICHOST);
        ASSERT_TRUE(ai2.get_first());

        EXPECT_EQ(ai2->ai_family, AF_INET6);
        EXPECT_EQ(ai2->ai_socktype, SOCK_STREAM);
        EXPECT_EQ(ai2->ai_protocol, 0);
        EXPECT_EQ(ai2->ai_flags, AI_V4MAPPED | AI_PASSIVE | AI_NUMERICHOST);
        // This will listen on all local network interfaces.
        ai2.sockaddr(saddr);
#ifdef _MSC_VER
        // Shows mapped part in hex, not in num base 10.
        EXPECT_EQ(saddr.netaddrp(), "[::ffff:0:0]:50032");
#else
        EXPECT_EQ(saddr.netaddrp(), "[::ffff:0.0.0.0]:50032");
#endif

        // Test Unit for AF_UNSPEC
        // "[]" is undefined and if specified as numeric address
        // (AI_NUMERICHOST flag set) it is treated as invalid. As alphanumeric
        // node name it is given to syscall ::getaddrinfo() that triggers a DNS
        // name resolution. I think that will not find it.
        CAddrinfo ai3("[]", "50113", AI_PASSIVE | AI_NUMERICHOST, 0);
        EXPECT_FALSE(ai3.get_first());
    }
}

TEST(AddrinfoTestSuite, get_two_brackets_alphanum_node_address) {
    // "[]" is undefined so get it as invalid.

    // Test Unit
    CAddrinfo ai7("[]", "50098", suppress_dns_lookup, 0);
    EXPECT_FALSE(ai7.get_first());

    // Test Unit for AF_UNSPEC
    CAddrinfo ai1("[]", "50112", AI_PASSIVE | suppress_dns_lookup);
    EXPECT_FALSE(ai1.get_first());
    CAddrinfo ai2("[]", "50112", suppress_dns_lookup);
    EXPECT_FALSE(ai2.get_first());

    // Test Unit for AF_INET6
    CAddrinfo ai3("[]", "50114", AI_PASSIVE | suppress_dns_lookup, 0);
    EXPECT_FALSE(ai3.get_first());
    CAddrinfo ai4("[]", "50114", suppress_dns_lookup, 0);
    EXPECT_FALSE(ai4.get_first());

    // Test Unit for AF_INET
    // This alphanumeric address is never a valid IPv4 address but it
    // invokes an expensive DNS lookup.
    CAddrinfo ai5("[]", "50116", AI_PASSIVE | suppress_dns_lookup, 0);
    EXPECT_FALSE(ai5.get_first());
    CAddrinfo ai6("[]", "50116", suppress_dns_lookup, 0);
    EXPECT_FALSE(ai6.get_first());
}

TEST(AddrinfoTestSuite, empty_service) {
    // With a node but an empty service the returned port number in the address
    // structure is set to 0.

    // Test Unit
    CAddrinfo ai1("[2001:db8::8]", "", AI_NUMERICHOST);
    ASSERT_TRUE(ai1.get_first());

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_V4MAPPED | AI_NUMERICHOST);
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[2001:db8::8]:0");
}

TEST(AddrinfoTestSuite, service_out_of_range) {
    // Test Unit
    CAddrinfo ai1("[2001:db8::c9]", "65536", AI_NUMERICHOST);
    ASSERT_FALSE(ai1.get_first());

    EXPECT_EQ(ai1->ai_family, AF_INET6); // Fix given by specification;
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_V4MAPPED | AI_NUMERICHOST);
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), ":0");
    EXPECT_THAT(ai1.what(),
                HasSubstr("] WHAT MSG1128: Port number 65536 out of range"));
    // std::cout << ai1.what() << '\n';
}

TEST(AddrinfoTestSuite, load_loopback_addr_with_scope_id) {
    CAddrinfo ai1("[::1%1]");
    ASSERT_TRUE(ai1.get_first());

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_V4MAPPED);
    EXPECT_EQ(ai1->ai_addrlen, 28);
    EXPECT_NE(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[::1]:0");
    EXPECT_EQ(ai1->ai_next, nullptr);

    CAddrinfo ai2("[::1%lo]");
#ifdef __APPLE__
    ASSERT_TRUE(ai2.get_first());

    EXPECT_EQ(ai2->ai_family, AF_INET6);
    EXPECT_EQ(ai2->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai2->ai_protocol, 0);
    EXPECT_EQ(ai2->ai_flags, AI_V4MAPPED);
    EXPECT_EQ(ai2->ai_addrlen, 28);
    EXPECT_NE(ai2->ai_addr, nullptr);
    EXPECT_EQ(ai2->ai_canonname, nullptr);
    ai2.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[::1]:0");
    EXPECT_EQ(ai2->ai_next, nullptr);
#else
    EXPECT_FALSE(ai2.get_first());
#endif
}

TEST(AddrinfoTestSuite, load_gua_with_scope_id) {
    // Unicast addresses are unique worldwide so using a scope id makes no much
    // sense but it is partly possible, except for AppleClang.
    CAddrinfo ai1("[2001:db8::55%1]:https");
    ASSERT_TRUE(ai1.get_first());

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_V4MAPPED);
    EXPECT_EQ(ai1->ai_addrlen, 28);
    EXPECT_NE(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    // AppleClang accepts scope id (%1) only from link local addresses [fe80::]
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[2001:db8::55]:443");
    EXPECT_EQ(ai1->ai_next, nullptr);

    CAddrinfo ai2("[2001:db8::55%lo]:https");
#if __APPLE__
    ASSERT_TRUE(ai1.get_first());

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_V4MAPPED);
    EXPECT_EQ(ai1->ai_addrlen, 28);
    EXPECT_NE(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    // AppleClang accepts scope id (%1) only from link local addresses [fe80::]
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[2001:db8::55]:443");
    EXPECT_EQ(ai1->ai_next, nullptr);
#else
    EXPECT_FALSE(ai2.get_first());
#endif
}

TEST(AddrinfoTestSuite, check_netaddrp) {
    CAddrinfo ai1("[fe80::1%1]:50001");
    ASSERT_TRUE(ai1.get_first());
    // Test Unit
    ai1.sockaddr(saddr);
    EXPECT_THAT(saddr.netaddrp(),
                AnyOf("[fe80::1%lo]:50001", "[fe80::1%lo0]:50001",
                      "[fe80::1%1]:50001"));

    CAddrinfo ai2("127.0.0.1:50002");
    EXPECT_TRUE(ai2.get_first());
    // Test Unit
    ai2.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[::ffff:127.0.0.1]:50002");
}

TEST(AddrinfoTestSuite, get_sockaddr) {
    CAddrinfo aiObj("[2001:db8::2%1]:47111");
    ASSERT_TRUE(aiObj.get_first());

    aiObj.sockaddr(saddr);
    EXPECT_EQ(saddr.ss.ss_family, AF_INET6);
    EXPECT_EQ(saddr.sin6.sin6_port, htons(47111));
    // AppleClang accepts scope id (%1) only from link local addresses [fe80::]
    EXPECT_THAT(saddr.netaddrp(),
                AnyOf("[2001:db8::2%lo]:47111", "[2001:db8::2]:47111",
                      "[2001:db8::2%1]:47111"));
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
