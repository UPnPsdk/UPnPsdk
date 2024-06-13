// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-06-13

// Include source code for testing. So we have also direct access to static
// functions which need to be tested.
#include <Upnplib/src/net/addrinfo.cpp>

#include <upnplib/socket.hpp>
#include <utest/utest.hpp>
#include <umock/netdb_mock.hpp>


namespace utest {

using ::testing::_;
using ::testing::AnyOf;
using ::testing::Field;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::StrictMock;

using ::upnplib::CAddrinfo;


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


TEST(AddrinfoTestSuite, instantiate_ipv6_addrinfo_successful) {
    CAddrinfo ai1("[2001:db8::8]");

    // Check the initialized object. This is what we have given with the
    // constructor. We get just the initialized hints.
    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_NUMERICHOST);
    EXPECT_EQ(ai1->ai_addrlen, 0);
    EXPECT_EQ(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    EXPECT_EQ(ai1->ai_next, nullptr);
    // There is no address information
    EXPECT_EQ(ai1.netaddr().str(), "");
}

TEST(AddrinfoTestSuite, instantiate_ipv4_addrinfo_successful) {
    CAddrinfo ai1("192.168.200.201");

    // Check the initialized object. This is what we have given with the
    // constructor. We get just the initialized hints.
    EXPECT_EQ(ai1->ai_family, AF_INET);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_NUMERICHOST);
    EXPECT_EQ(ai1->ai_addrlen, 0);
    EXPECT_EQ(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    EXPECT_EQ(ai1->ai_next, nullptr);
    // There is no address information
    EXPECT_EQ(ai1.netaddr().str(), "");
}

TEST(AddrinfoTestSuite, init_ipv6_addrinfo_successful) {
    CAddrinfo ai1("[2001:db8::13]");

    ASSERT_NO_THROW(ai1.init());

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_NUMERICHOST);
    EXPECT_EQ(ai1->ai_addrlen, 28);
    EXPECT_NE(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    // EXPECT_NE(ai1->ai_next, nullptr); // Depends on available interfaces
    EXPECT_EQ(ai1.netaddr().str(), "[2001:db8::13]:0");
}

TEST(AddrinfoTestSuite, init_ipv4_addrinfo_successful) {
    CAddrinfo ai1("192.168.200.202");

    ASSERT_NO_THROW(ai1.init());

    EXPECT_EQ(ai1->ai_family, AF_INET);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_NUMERICHOST);
    EXPECT_EQ(ai1->ai_addrlen, 16);
    EXPECT_NE(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    // EXPECT_NE(ai1->ai_next, nullptr); // Depends on available interfaces
    EXPECT_EQ(ai1.netaddr().str(), "192.168.200.202:0");
}

TEST(AddrinfoTestSuite, double_set_addrinfo_successful) {
    // If node is not empty AI_PASSIVE is ignored.
    // Flag AI_NUMERICHOST should be set if possible to avoid expensive name
    // resolution from external DNS server.

    // Test Unit with numeric port number
    CAddrinfo ai2("[2001:db8::2]", "50048", AF_INET6, SOCK_STREAM,
                  AI_PASSIVE | AI_NUMERICHOST);
    ASSERT_NO_THROW(ai2.init());

    // Returns what ::getaddrinfo() returns.
    EXPECT_EQ(ai2->ai_family, AF_INET6);
    // Returns what ::getaddrinfo() returns.
    EXPECT_EQ(ai2->ai_socktype, SOCK_STREAM);
    // Different on platforms: Ubuntu & MacOS return 6, win32 returns 0.
    // We just return that what was requested by the user.
    EXPECT_EQ(ai2->ai_protocol, 0);
    // Different on platforms: Ubuntu returns 1025, MacOS & win32 return 0.
    // We just return that what was requested by the user.
    EXPECT_EQ(ai2->ai_flags, AI_PASSIVE | AI_NUMERICHOST);
    // Returns what ::getaddrinfo() returns.
    EXPECT_EQ(ai2.netaddr().str(), "[2001:db8::2]:50048");
    // Returns what ::getaddrinfo() returns.

    // Test Unit
    // Getting the address information again is possible but not very useful.
    // Because the same node, service and hints are used the result is exactly
    // the same as before.
    int* old_res{&ai2->ai_flags};
    ASSERT_NO_THROW(ai2.init());

    EXPECT_NE(old_res, &ai2->ai_flags);
    EXPECT_EQ(ai2->ai_family, AF_INET6);
    EXPECT_EQ(ai2->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai2->ai_protocol, 0);
    EXPECT_EQ(ai2->ai_flags, AI_PASSIVE | AI_NUMERICHOST);
    EXPECT_EQ(ai2.netaddr().str(), "[2001:db8::2]:50048");
}

TEST(AddrinfoTestSuite, instantiate_not_init_numeric_host_successful) {
    // If node is not empty AI_PASSIVE is ignored.
    // Flag AI_NUMERICHOST should be set if possible to avoid expensive name
    // resolution from external DNS server.

    // Test Unit with numeric port number
    CAddrinfo ai1("[2001:db8::1]", "50050", AF_INET6, SOCK_STREAM,
                  AI_PASSIVE | AI_NUMERICHOST);

    // Check the initialized object without address information. This is what
    // we have given with the constructor. We get just the initialized hints.
    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_PASSIVE | AI_NUMERICHOST);
    EXPECT_EQ(ai1->ai_addrlen, 0);
    EXPECT_EQ(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    EXPECT_EQ(ai1->ai_next, nullptr);
    // There is no address information
    EXPECT_EQ(ai1.netaddr().str(), "");
}

TEST(AddrinfoTestSuite, get_implicit_address_family) {
    // It is not needed to set the address family to AF_UNSPEC. That is used by
    // default.

    // Test Unit
    CAddrinfo ai1("[2001:db8::5]", "50051");
    // Same as
    // CAddrinfo ai1("[2001:db8::5]", "50051", AF_UNSPEC, SOCK_STREAM);
    ASSERT_NO_THROW(ai1.init());

    EXPECT_EQ(ai1->ai_family, AF_INET6); // set by syscal ::getaddrinfo
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_NUMERICHOST); // set by syscal ::getaddrinfo
    EXPECT_EQ(ai1.netaddr().str(), "[2001:db8::5]:50051");

    // Test Unit
    CAddrinfo ai2("192.168.9.10", "50096");
    ASSERT_NO_THROW(ai2.init());

    EXPECT_EQ(ai2->ai_family, AF_INET); // set by syscal ::getaddrinfo
    EXPECT_EQ(ai2->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai2->ai_protocol, 0);
    EXPECT_EQ(ai2->ai_flags, AI_NUMERICHOST); // set by syscal ::getaddrinfo
    EXPECT_EQ(ai2.netaddr().str(), "192.168.9.10:50096");

    // Test Unit, does not trigger a DNS query
    CAddrinfo ai3("localhost", "50049");
    ASSERT_NO_THROW(ai3.init());

    EXPECT_EQ(ai3->ai_family, AF_INET6); // set by syscal ::getaddrinfo
    EXPECT_EQ(ai3->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai3->ai_protocol, 0);
    EXPECT_EQ(ai3->ai_flags, 0);
    EXPECT_EQ(ai3.netaddr().str(), "[::1]:50049");
}

TEST(AddrinfoTestSuite, get_unknown_numeric_host_fails) {
    // With AI_NUMERICHOST "localhost" is unknown. Name resolving does not
    // trigger a DNS query.

    // Test Unit
    CAddrinfo ai1("localhost", "50031", AF_INET, SOCK_STREAM, AI_NUMERICHOST);
    EXPECT_THROW(ai1.init(), std::runtime_error);

    // Does not call ::getaddrinfo(), because invalid numeric IPv6 is detected
    // before.
    CAddrinfo ai2("localhost", "50052", AF_INET6, SOCK_DGRAM, AI_NUMERICHOST);
    EXPECT_THROW(ai2.init(), std::runtime_error);

    CAddrinfo ai3("localhost", "50053", AF_UNSPEC, SOCK_STREAM, AI_NUMERICHOST);
    EXPECT_THROW(ai3.init(), std::runtime_error);
}

TEST_F(AddrinfoMockFTestSuite, get_unknown_alphanumeric_host_fails) {
    // An alphanumeric node name with '[' is not valid.
    EXPECT_CALL(m_netdbObj,
                getaddrinfo(Pointee(*"[localhost]"), Pointee(*"50055"),
                            Field(&addrinfo::ai_flags, 0), _))
        .WillOnce(Return(EAI_NONAME));

    CAddrinfo ai("[localhost]", "50055", AF_UNSPEC, SOCK_DGRAM);
    EXPECT_THROW(ai.init(), std::runtime_error);
}

TEST_F(AddrinfoMockFTestSuite, get_addrinfo_out_of_memory) {
    EXPECT_CALL(m_netdbObj,
                getaddrinfo(Pointee(*"localhost"), Pointee(*"50118"),
                            Field(&addrinfo::ai_flags, 0), _))
        .WillOnce(Return(EAI_MEMORY));

    CAddrinfo ai("localhost", "50118", AF_UNSPEC, SOCK_STREAM);
    EXPECT_THROW(ai.init(), std::invalid_argument);
}

TEST(AddrinfoTestSuite, invalid_ipv6_node_address) {
    // Test Unit. Node address must be surounded with brackets.
    CAddrinfo ai1("::1", "", AF_INET6, 0, AI_NUMERICHOST);
    EXPECT_THROW(ai1.init(), std::runtime_error);

    CAddrinfo ai2("[::1", "", AF_INET6, 0, AI_NUMERICHOST);
    EXPECT_THROW(ai2.init(), std::runtime_error);

    CAddrinfo ai3("::1]", "", AF_INET6, 0, AI_NUMERICHOST);
    EXPECT_THROW(ai3.init(), std::runtime_error);
}

TEST(AddrinfoTestSuite, get_unknown_node_address) {
    // Test Unit with unspecified ipv6 address
    CAddrinfo ai3("[::]", "0", AF_INET6, 0, AI_NUMERICHOST | AI_NUMERICSERV);
    ASSERT_NO_THROW(ai3.init());

    EXPECT_EQ(ai3->ai_family, AF_INET6);
    EXPECT_EQ(ai3->ai_socktype, 0);
    EXPECT_EQ(ai3->ai_protocol, 0);
    EXPECT_EQ(ai3->ai_flags, AI_NUMERICHOST | AI_NUMERICSERV);
    EXPECT_EQ(ai3.netaddr().str(), "[::]:0");

    // Test Unit with unspecified ipv4 address
    CAddrinfo ai4("0.0.0.0", "0", AF_INET, 0, AI_NUMERICHOST | AI_NUMERICSERV);
    ASSERT_NO_THROW(ai4.init());

    EXPECT_EQ(ai4->ai_family, AF_INET);
    EXPECT_EQ(ai4->ai_socktype, 0);
    EXPECT_EQ(ai4->ai_protocol, 0);
    EXPECT_EQ(ai4->ai_flags, AI_NUMERICHOST | AI_NUMERICSERV);
    EXPECT_EQ(ai4.netaddr().str(), "0.0.0.0:0");
}

TEST(AddrinfoTestSuite, get_active_empty_node_address) {
    // An empty node name will return information about the loopback interface.
    // This is specified by syscall ::getaddrinfo().

    { // Scoped to reduce memory usage for testing with node "".
        // Test Unit for AF_UNSPEC
        CAddrinfo ai1("", "50007");
        ASSERT_NO_THROW(ai1.init());

        // Address family set by syscal ::getaddrinfo(). Maybe interface with
        // index number 1 is used and its address_family is returned?
        EXPECT_EQ(ai1->ai_family, AF_INET6);
        EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
        EXPECT_EQ(ai1->ai_protocol, 0);
        EXPECT_EQ(ai1->ai_flags, 0);
        EXPECT_THAT(ai1.netaddr().str(), "[::1]:50007");

        // Test Unit for AF_UNSPEC
        CAddrinfo ai2("", "50099", AF_UNSPEC, 0, AI_NUMERICHOST);
        ASSERT_NO_THROW(ai2.init());

        EXPECT_EQ(ai2->ai_family, AF_INET6);
        EXPECT_EQ(ai2->ai_socktype, 0);
        EXPECT_EQ(ai2->ai_protocol, 0);
        EXPECT_EQ(ai2->ai_flags, AI_NUMERICHOST);
        EXPECT_THAT(ai2.netaddr().str(), "[::1]:50099");

        // Test Unit for AF_INET6
        CAddrinfo ai3("", "50084", AF_INET6, SOCK_STREAM);
        ASSERT_NO_THROW(ai3.init());

        EXPECT_EQ(ai3->ai_family, AF_INET6);
        EXPECT_EQ(ai3->ai_socktype, SOCK_STREAM);
        EXPECT_EQ(ai3->ai_protocol, 0);
        EXPECT_EQ(ai3->ai_flags, 0);
        EXPECT_EQ(ai3.netaddr().str(), "[::1]:50084");

        // Test Unit for AF_INET6
        CAddrinfo ai4("", "50058", AF_INET6, SOCK_STREAM,
                      AI_NUMERICHOST | AI_NUMERICSERV);
        ASSERT_NO_THROW(ai4.init());

        EXPECT_EQ(ai4->ai_family, AF_INET6);
        EXPECT_EQ(ai4->ai_socktype, SOCK_STREAM);
        EXPECT_EQ(ai4->ai_protocol, 0);
        EXPECT_EQ(ai4->ai_flags, AI_NUMERICHOST | AI_NUMERICSERV);
        EXPECT_EQ(ai4.netaddr().str(), "[::1]:50058");

        // Test Unit for AF_INET
        CAddrinfo ai5("", "50057", AF_INET);
        ASSERT_NO_THROW(ai5.init());

        EXPECT_EQ(ai5->ai_family, AF_INET);
        EXPECT_EQ(ai5->ai_socktype, SOCK_STREAM);
        EXPECT_EQ(ai5->ai_protocol, 0);
        EXPECT_EQ(ai5->ai_flags, 0);
        EXPECT_EQ(ai5.netaddr().str(), "127.0.0.1:50057");

        // Test Unit for AF_INET
        CAddrinfo ai6("", "50100", AF_INET, 0, AI_NUMERICHOST);
        ASSERT_NO_THROW(ai6.init());

        EXPECT_EQ(ai6->ai_family, AF_INET);
        EXPECT_EQ(ai6->ai_socktype, 0);
        EXPECT_EQ(ai6->ai_protocol, 0);
        EXPECT_EQ(ai6->ai_flags, AI_NUMERICHOST);
        EXPECT_EQ(ai6.netaddr().str(), "127.0.0.1:50100");
    }

    { // Scoped to reduce memory usage for testing with node "[]".
        // Test Unit for AF_UNSPEC
        CAddrinfo ai2("[]", "50101", AF_UNSPEC, 0, AI_NUMERICHOST);
        EXPECT_THROW(ai2.init(), std::runtime_error);

        // Test Unit AF_INET6 to be numeric, important test of special case.
        CAddrinfo ai4("[]", "50103", AF_INET6, 0, AI_NUMERICHOST);
        EXPECT_THROW(ai4.init(), std::runtime_error);

        // Test Unit AF_INET
        CAddrinfo ai6("[]", "50105", AF_INET, 0, AI_NUMERICHOST);
        EXPECT_THROW(ai6.init(), std::runtime_error);
    }
}

TEST(AddrinfoTestSuite, get_passive_node_address) {
    // To get a passive address info, node must be empty ("") or an empty
    // IPv6 address ("[]") otherwise flag AI_PASSIVE is ignored.

    { // Scoped to reduce memory usage for testing with node "".
        // Test Unit for AF_UNSPEC
        CAddrinfo ai1("", "50106", AF_UNSPEC, SOCK_STREAM, AI_PASSIVE);
        ASSERT_NO_THROW(ai1.init());

        // Ubuntu returns AF_INET, MacOS and Microsoft Windows return AF_INET6
        EXPECT_THAT(ai1->ai_family, AnyOf(AF_INET6, AF_INET));
        EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
        EXPECT_EQ(ai1->ai_protocol, 0);
        EXPECT_EQ(ai1->ai_flags, AI_PASSIVE);
        EXPECT_THAT(ai1.netaddr().str(), AnyOf("[::]:50106", "0.0.0.0:50106"));

        // Test Unit for AF_UNSPEC
        CAddrinfo ai2("", "50107", AF_UNSPEC, 0, AI_PASSIVE | AI_NUMERICHOST);
        ASSERT_NO_THROW(ai2.init());

        // Ubuntu returns AF_INET, MacOS and Microsoft Windows return AF_INET6
        EXPECT_THAT(ai2->ai_family, AnyOf(AF_INET6, AF_INET));
        EXPECT_EQ(ai2->ai_socktype, 0);
        EXPECT_EQ(ai2->ai_protocol, 0);
        EXPECT_EQ(ai2->ai_flags, AI_PASSIVE | AI_NUMERICHOST);
        EXPECT_THAT(ai2.netaddr().str(), AnyOf("[::]:50107", "0.0.0.0:50107"));

        // Test Unit for AF_INET6
        CAddrinfo ai3("", "50108", AF_INET6, 0, AI_PASSIVE);
        ASSERT_NO_THROW(ai3.init());

        EXPECT_EQ(ai3->ai_family, AF_INET6);
        EXPECT_EQ(ai3->ai_socktype, 0);
        EXPECT_EQ(ai3->ai_protocol, 0);
        EXPECT_EQ(ai3->ai_flags, AI_PASSIVE);
        EXPECT_EQ(ai3.netaddr().str(), "[::]:50108");

        // Test Unit for AF_INET6
        CAddrinfo ai4("", "50109", AF_INET6, 0, AI_PASSIVE | AI_NUMERICHOST);
        ASSERT_NO_THROW(ai4.init());

        EXPECT_EQ(ai4->ai_family, AF_INET6);
        EXPECT_EQ(ai4->ai_socktype, 0);
        EXPECT_EQ(ai4->ai_protocol, 0);
        EXPECT_EQ(ai4->ai_flags, AI_PASSIVE | AI_NUMERICHOST);
        EXPECT_EQ(ai4.netaddr().str(), "[::]:50109");

        // Test Unit for AF_INET
        CAddrinfo ai5("", "50110", AF_INET, 0, AI_PASSIVE);
        ASSERT_NO_THROW(ai5.init());

        EXPECT_EQ(ai5->ai_family, AF_INET);
        EXPECT_EQ(ai5->ai_socktype, 0);
        EXPECT_EQ(ai5->ai_protocol, 0);
        EXPECT_EQ(ai5->ai_flags, AI_PASSIVE);
        EXPECT_EQ(ai5.netaddr().str(), "0.0.0.0:50110");

        // Test Unit for AF_INET
        CAddrinfo ai6("", "50111", AF_INET, 0, AI_PASSIVE | AI_NUMERICHOST);
        ASSERT_NO_THROW(ai6.init());

        EXPECT_EQ(ai6->ai_family, AF_INET);
        EXPECT_EQ(ai6->ai_socktype, 0);
        EXPECT_EQ(ai6->ai_protocol, 0);
        EXPECT_EQ(ai6->ai_flags, AI_PASSIVE | AI_NUMERICHOST);
        // This will listen on all local network interfaces.
        EXPECT_EQ(reinterpret_cast<sockaddr_in*>(ai6->ai_addr)->sin_addr.s_addr,
                  INADDR_ANY); // or
        EXPECT_EQ(ai6.netaddr().str(), "0.0.0.0:50111");
    }

    { // Scoped to reduce memory usage for testing with node "[]".
      // "[]" is undefined and if specified as numeric address
      // (AI_NUMERICHOST flag set) it is treated as invalid. As alphanumeric
      // node name it is given to syscall ::getaddrinfo() that triggers a DNS
      // name resolution. I think that will not find it.

        // Test Unit for AF_UNSPEC
        CAddrinfo ai2("[]", "50113", AF_UNSPEC, 0, AI_PASSIVE | AI_NUMERICHOST);
        EXPECT_THROW(ai2.init(), std::runtime_error);

        // Test Unit for AF_INET6
        CAddrinfo ai4("[]", "50115", AF_INET6, 0, AI_PASSIVE | AI_NUMERICHOST);
        EXPECT_THROW(ai4.init(), std::runtime_error);

        // Test Unit for AF_INET
        // Wrong address family
        CAddrinfo ai6("[]", "50117", AF_INET, 0, AI_PASSIVE | AI_NUMERICHOST);
        EXPECT_THROW(ai6.init(), std::runtime_error);
    }

    // Test Unit
    // Using explicit the unknown netaddress should definetly return the
    // in6addr_any passive listening socket info.
    CAddrinfo ai1("[::]", "50006", AF_INET6, SOCK_DGRAM,
                  AI_PASSIVE | AI_NUMERICHOST);
    ASSERT_NO_THROW(ai1.init());

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_DGRAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_PASSIVE | AI_NUMERICHOST);
    // This will listen on all local network interfaces.
    EXPECT_EQ(ai1.netaddr().str(), "[::]:50006");

    // Test Unit
    // Using explicit the unknown netaddress should definetly return the
    // INADDR_ANY passive listening socket info.
    CAddrinfo ai2("0.0.0.0", "50032", AF_INET, SOCK_STREAM,
                  AI_PASSIVE | AI_NUMERICHOST);
    ASSERT_NO_THROW(ai2.init());

    EXPECT_EQ(ai2->ai_family, AF_INET);
    EXPECT_EQ(ai2->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai2->ai_protocol, 0);
    EXPECT_EQ(ai2->ai_flags, AI_PASSIVE | AI_NUMERICHOST);
    // This will listen on all local network interfaces.
    EXPECT_EQ(ai2.netaddr().str(), "0.0.0.0:50032");
}

TEST_F(AddrinfoMockFTestSuite, get_two_brackets_alphanum_node_address) {
    // "[]" is undefined and if specified as numeric address (AI_NUMERICHOST
    // flag set) it is treated as invalid. As alphanumeric node name it is
    // given to syscall ::getaddrinfo() that triggers a DNS name resolution. I
    // think that will not find it.
    // To avoid real DNS server call I mock it for testing.

    EXPECT_CALL(m_netdbObj,
                getaddrinfo(Pointee(*"[]"), _,
                            Field(&addrinfo::ai_flags, AnyOf(0, 1)), _))
        .WillRepeatedly(Return(EAI_NONAME));

    // Test Unit, there is no hint AI_NUMERICHOST, so it is assumed that the
    // node is alphanumeric.
    CAddrinfo ai7("[]", "50098");
    EXPECT_THROW(ai7.init(), std::runtime_error);

    // Test Unit for AF_UNSPEC
    CAddrinfo ai1("[]", "50112", AF_UNSPEC, SOCK_STREAM, AI_PASSIVE);
    EXPECT_THROW(ai1.init(), std::runtime_error);
    CAddrinfo ai2("[]", "50112", AF_UNSPEC, SOCK_STREAM);
    EXPECT_THROW(ai2.init(), std::runtime_error);

    // Test Unit for AF_INET6
    CAddrinfo ai3("[]", "50114", AF_INET6, 0, AI_PASSIVE);
    EXPECT_THROW(ai3.init(), std::runtime_error);
    CAddrinfo ai4("[]", "50114", AF_INET6);
    EXPECT_THROW(ai4.init(), std::runtime_error);

    // Test Unit for AF_INET
    // This alphanumeric address is never a valid IPv4 address but it
    // invokes an expensive DNS lookup.
    CAddrinfo ai5("[]", "50116", AF_INET, 0, AI_PASSIVE);
    EXPECT_THROW(ai5.init(), std::runtime_error);
    CAddrinfo ai6("[]", "50116", AF_INET);
    EXPECT_THROW(ai6.init(), std::runtime_error);
}

TEST(AddrinfoTestSuite, get_info_loopback_interface) {
    // If node is not empty AI_PASSIVE is ignored.

    // Test Unit with string port number
    CAddrinfo ai1("[::1]", "50001", AF_INET6, SOCK_STREAM,
                  AI_PASSIVE | AI_NUMERICHOST);
    ASSERT_NO_THROW(ai1.init());

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_PASSIVE | AI_NUMERICHOST);
    EXPECT_EQ(ai1.netaddr().str(), "[::1]:50001");

    // Test Unit
    CAddrinfo ai5("[::1]", "50085", AF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(ai5.init());

    EXPECT_EQ(ai5->ai_family, AF_INET6);
    EXPECT_EQ(ai5->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai5->ai_protocol, 0);
    EXPECT_EQ(ai5->ai_flags, AI_NUMERICHOST);
    EXPECT_EQ(ai5.netaddr().str(), "[::1]:50085");

    // Test Unit
    CAddrinfo ai2("127.0.0.1", "50086", AF_INET, SOCK_DGRAM);
    ASSERT_NO_THROW(ai2.init());

    EXPECT_EQ(ai2->ai_family, AF_INET);
    EXPECT_EQ(ai2->ai_socktype, SOCK_DGRAM);
    EXPECT_EQ(ai2->ai_protocol, 0);
    EXPECT_EQ(ai2->ai_flags, AI_NUMERICHOST);
    EXPECT_EQ(ai2.netaddr().str(), "127.0.0.1:50086");

    // Test Unit
    CAddrinfo ai3("[::1]", "50087", AF_INET6);
    ASSERT_NO_THROW(ai3.init());

    EXPECT_EQ(ai3->ai_family, AF_INET6);
    EXPECT_EQ(ai3->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai3->ai_protocol, 0);
    EXPECT_EQ(ai3->ai_flags, AI_NUMERICHOST);
    EXPECT_EQ(ai3.netaddr().str(), "[::1]:50087");

    // Test Unit, does not trigger a DNS query
    CAddrinfo ai4("localhost", "50088");
    ASSERT_NO_THROW(ai4.init());

    EXPECT_EQ(ai4->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai4->ai_protocol, 0);
    EXPECT_EQ(ai4->ai_flags, 0);
    EXPECT_THAT(ai4.netaddr().str(), AnyOf("[::1]:50088", "127.0.0.1:50088"));
}

TEST(AddrinfoTestSuite, empty_service) {
    // With a node but an empty service the returned port number in the address
    // structure is set to 0.

    // Test Unit
    CAddrinfo ai1("[2001:db8::8]", "", AF_INET6, SOCK_STREAM, AI_NUMERICHOST);
    ASSERT_NO_THROW(ai1.init());

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_NUMERICHOST);
    EXPECT_EQ(ai1.netaddr().str(), "[2001:db8::8]:0");
}

TEST(AddrinfoTestSuite, get_fails) {
    // Test Unit. Address family does not match the numeric host address.
    // There are different error messages on different platforms.
    CAddrinfo ai1("192.168.155.15", "50003", AF_INET6, SOCK_STREAM,
                  AI_NUMERICHOST);
#if defined(__GNUC__) && !defined(__clang__)
    EXPECT_THROW(ai1.init(), std::invalid_argument);
#else
    EXPECT_THROW(ai1.init(), std::runtime_error);
#endif
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
    WINSOCK_INIT
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
