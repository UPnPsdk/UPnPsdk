// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-04-16

// I test different address infos that we get from system function
// ::getaddrinfo().

// Include source code for testing. So we have also direct access to static
// functions which need to be tested.
#include <UPnPsdk/src/net/addrinfo.cpp>

#include <UPnPsdk/socket.hpp>
#include <utest/utest.hpp>
#include <umock/netdb_mock.hpp>


namespace utest {

using ::testing::_;
using ::testing::AnyOf;
using ::testing::Conditional;
using ::testing::DoAll;
using ::testing::Field;
using ::testing::HasSubstr;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;
// using ::UPnPsdk::is_netaddr;

using ::UPnPsdk::CAddrinfo;
using ::UPnPsdk::SSockaddr;


namespace {

// General storage for temporary socket address evaluation
SSockaddr saddr;

constexpr int suppress_dns_lookup{AI_NUMERICHOST};

// Alternative proof of runtime select of the platform instead of conditional
// compiling.
enum class Co { // Possible compiler
    unknown,
    gnuc,
    clang,
    msc
};
// Current used compiler
#if defined(__GNUC__) && !defined(__clang__)
Co co = Co::gnuc;
#elif defined(__clang__)
Co co = Co::clang;
#elif defined(_MSC_VER)
Co co = Co::msc;
#else
Co co = Co::unknown;
#endif

} // anonymous namespace

#if 0
// Raw ::getaddrinfo and ::getnameinfo execution to verify its behavior. This is
// for humans only and not a real Unit Test. It should not always run.
TEST(AddrinfoTestSuite, getaddrinfo_raw) {
    ::addrinfo hints{}, *res{};
    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_INET6;

    int ret = ::getaddrinfo(nullptr, "50100", &hints, &res);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(res->ai_next, nullptr);

    char addr_str[INET6_ADDRSTRLEN];
    char serv_str[NI_MAXSERV];
    for (::addrinfo* ptr{res}; ptr != nullptr; ptr = ptr->ai_next) {
        ::getnameinfo(ptr->ai_addr, ptr->ai_addrlen, addr_str, sizeof(addr_str),
                      serv_str, sizeof(serv_str),
                      NI_NUMERICHOST | NI_NUMERICSERV);
        std::cout << "ai_family=" << ptr->ai_family
                  << ", ai_socktype=" << ptr->ai_socktype
                  << ", ai_protocol=" << ptr->ai_protocol << ", addr=\""
                  << addr_str << "\", serv=\"" << serv_str
                  << "\", ai_flags=" << ptr->ai_flags << ".\n";
    }
}
#endif


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


enum struct Entry { no, one, more };
enum struct Error { yes = true, no = false };

class NetaddrAssignTest
    : public ::testing::TestWithParam<std::tuple<
          //    netaddress         result       one entry   throw error
          const std::string, const std::string, const Entry, const Error>> {};

TEST_P(NetaddrAssignTest, netaddress_assign) {
    // Get parameter
    std::tuple params = GetParam();

    // Test Unit
    CAddrinfo aiObj(std::get<0>(params), suppress_dns_lookup);
    if (std::get<3>(params) == Error::yes) {
        ASSERT_FALSE(aiObj.get_first());
        ASSERT_EQ(std::get<2>(params), Entry::no);
    } else {
        ASSERT_TRUE(aiObj.get_first());
        if (std::get<2>(params) == Entry::one)
            EXPECT_EQ(aiObj->ai_next, nullptr);
        else
            EXPECT_NE(aiObj->ai_next, nullptr);
        aiObj.sockaddr(saddr);
        EXPECT_EQ(saddr.netaddrp(), std::get<1>(params));
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
        // A valid addreess with an invalid port results to port 0.
        /*0*/ std::make_tuple("[::]", "[::]:0", Entry::one, Error::no),
        std::make_tuple("[::]:", "[::]:0", Entry::one, Error::no),
        std::make_tuple("[::]:0", "[::]:0", Entry::one, Error::no),
        std::make_tuple("[::]:65535", "[::]:65535", Entry::one, Error::no), // port 0 to 65535
        std::make_tuple("0.0.0.0", "0.0.0.0:0", Entry::one, Error::no),
        std::make_tuple("0.0.0.0:", "0.0.0.0:0", Entry::one, Error::no),
        std::make_tuple("0.0.0.0:0", "0.0.0.0:0", Entry::one, Error::no),
        std::make_tuple("0.0.0.0:65535", "0.0.0.0:65535", Entry::one, Error::no), // port 0 to 65535
        // Following invalid address parts will be general unspecified ("").
        // std::make_tuple("", unspec, Entry::???, Error::no), // makes passive listening, tested later
        std::make_tuple("[", "", Entry::no, Error::yes),
        std::make_tuple("]", "", Entry::no, Error::yes),
        /*10*/ std::make_tuple("[]", "", Entry::no, Error::yes),
        std::make_tuple(":", "", Entry::no, Error::yes),
        std::make_tuple(".", "", Entry::no, Error::yes),
        std::make_tuple(".:", "", Entry::no, Error::yes),
        std::make_tuple(":.", "", Entry::no, Error::yes),
        std::make_tuple("::", "[::]:0", Entry::one, Error::no),
        std::make_tuple(":::", "", Entry::no, Error::yes),
        std::make_tuple("[::", "", Entry::no, Error::yes),
        std::make_tuple("::]", "", Entry::no, Error::yes),
        // std::make_tuple("[::1", "", Entry::one, Error::no), // tested later
        // std::make_tuple("::1]", "", Entry::one, Error::no), // tested later
        // std::make_tuple("", "[::1]:0", Entry::one, Error::no), // multiple results, tested later
        // std::make_tuple(":0", "[::1]:0", Entry::one, Error::no), // multiple results, tested later
        // std::make_tuple(":50987", "[::1]:50987", Entry::one, Error::no), // multiple results, tested later
        std::make_tuple("::1", "[::1]:0", Entry::one, Error::no),
        /*20*/ std::make_tuple("[::1]", "[::1]:0", Entry::one, Error::no),
        std::make_tuple("[::1]:", "[::1]:0", Entry::one, Error::no),
        std::make_tuple("[::1]:0", "[::1]:0", Entry::one, Error::no),
        // std::make_tuple("[::1].4", "", Entry::one, Error::no), // dot for colon, takes long time, mocked later
        std::make_tuple("127.0.0.1", "127.0.0.1:0", Entry::one, Error::no),
        std::make_tuple("127.0.0.1:", "127.0.0.1:0", Entry::one, Error::no),
        std::make_tuple("127.0.0.1:0", "127.0.0.1:0", Entry::one, Error::no),
        // std::make_tuple("127.0.0.1.5", "", Entry::one, Error::no), // dot for colon, takes long time, mocked later
        std::make_tuple("[2001:db8::43]:", "[2001:db8::43]:0", Entry::one, Error::no),
        std::make_tuple("2001:db8::41:59897", "", Entry::no, Error::yes), // no brackets and wrong quad
        std::make_tuple("[2001:db8::fg]", "", Entry::no, Error::yes),
        std::make_tuple("[2001:db8::fg]:59877", "", Entry::no, Error::yes),
        // std::make_tuple("[2001:db8::42]:65535", "[2001:db8::42]:65535", Entry::one, Error::no), // tested later
        /*30*/ std::make_tuple("[2001:db8::51]:65536", "", Entry::no, Error::yes), // invalid port
        std::make_tuple("[2001:db8::52]:9999999999", "", Entry::no, Error::yes), // invalid port
        std::make_tuple("[2001:db8::52::53]", "", Entry::no, Error::yes), // double double colon
        std::make_tuple("[2001:db8::52::54]:65535", "", Entry::no, Error::yes), // double double colon
        std::make_tuple("[12.168.88.95]", "", Entry::no, Error::yes), // IPv4 address with brackets
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
        /*40*/ std::make_tuple("192.168.88.98:http", "192.168.88.98:80", Entry::one, Error::no),
        std::make_tuple("192.168.71.73%1:44:https", "", Entry::no, Error::yes),
        std::make_tuple("192.168.71.74%lo:44:https", "", Entry::no, Error::yes)
        // std::make_tuple("192.168.88.98:httpy", "", Entry::one, Error::no), // takes long time, mocked later
        // std::make_tuple("[fe80::5054:ff:fe7f:c021]", "[fe80::5054:ff:fe7f:c021%2]:0", Entry::one, Error::no), // fails, not portable
        // std::make_tuple("[fe80::5054:ff:fe7f:c021%ens1]", "[fe80::5054:ff:fe7f:c021%2]:0", Entry::one, Error::no), // succeeds, not portable
        // std::make_tuple("[2003:d5:270b:9000:5054:ff:fe7f:c021%3]", "[2003:d5:270b:9000:5054:ff:fe7f:c021%3]:0", Entry::one, Error::no), // succeeds, not porable
        // std::make_tuple("[2003:d5:270b:9000:5054:ff:fe7f:c021%2]", "[2003:d5:270b:9000:5054:ff:fe7f:c021%2]:0", Entry::one, Error::no), // succeeds, not porable
        // std::make_tuple("[2003:d5:270b:9000:5054:ff:fe7f:c021%ens1]", "[2003:d5:270b:9000:5054:ff:fe7f:c021%2]:0", Entry::one, Error::no) // fails, not porable
    ));
// clang-format on


// Test getting info for "localhost", or "loopback" device
// -------------------------------------------------------
TEST(AddrinfoTestSuite, get_active_empty_node_address_with_port) {
    // An empty node name will return information about the loopback interface.
    // This is specified by syscall ::getaddrinfo().

    // Test Unit for default AF_UNSPEC.
    CAddrinfo ai1("", "0");
    ASSERT_TRUE(ai1.get_first());

    // Address family set by syscal ::getaddrinfo().
    // There should be two results
    bool double_res1{false};
    bool double_res2{false};
    do {
        EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
        EXPECT_EQ(ai1->ai_protocol, 0);
        EXPECT_EQ(ai1->ai_flags, 0);
        EXPECT_NE(ai1->ai_addr, nullptr); // not equal nullptr
        EXPECT_EQ(ai1->ai_canonname, nullptr);
        if (ai1->ai_family == AF_INET6) {
            ASSERT_FALSE(double_res1);
            double_res1 = true;
            EXPECT_EQ(ai1->ai_addrlen, 28);
            ai1.sockaddr(saddr);
            EXPECT_EQ(saddr.netaddrp(), "[::1]:0");
        } else if (ai1->ai_family == AF_INET) {
            ASSERT_FALSE(double_res2);
            double_res2 = true;
            EXPECT_EQ(ai1->ai_addrlen, 16);
            ai1.sockaddr(saddr);
            EXPECT_EQ(saddr.netaddrp(), "127.0.0.1:0");
        } else {
            GTEST_FAIL()
                << "  Unexpected address information: address family = "
                << ai1->ai_family << ", socket type = " << ai1->ai_socktype
                << "\n";
        }
    } while (ai1.get_next());

    // Test Unit for AF_UNSPEC, important test of special case.
    CAddrinfo ai2("[]", "50101", AI_NUMERICHOST, 0);
    EXPECT_FALSE(ai2.get_first());
}

TEST(AddrinfoTestSuite, get_active_empty_node_address_without_port) {
    // An empty node name will return information about the loopback interface.
    // This is specified by syscall ::getaddrinfo().

    // Test Unit for default AF_UNSPEC.
    CAddrinfo ai1("");
    ASSERT_TRUE(ai1.get_first());

    // Address family set by syscal ::getaddrinfo().
    // There should be two results
    bool double_res1{false};
    bool double_res2{false};
    do {
        EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
        EXPECT_EQ(ai1->ai_protocol, 0);
        EXPECT_EQ(ai1->ai_flags, 0);
        EXPECT_NE(ai1->ai_addr, nullptr); // not equal nullptr
        EXPECT_EQ(ai1->ai_canonname, nullptr);
        if (ai1->ai_family == AF_INET6) {
            ASSERT_FALSE(double_res1);
            double_res1 = true;
            EXPECT_EQ(ai1->ai_addrlen, 28);
            ai1.sockaddr(saddr);
            EXPECT_EQ(saddr.netaddrp(), "[::1]:0");
        } else if (ai1->ai_family == AF_INET) {
            ASSERT_FALSE(double_res2);
            double_res2 = true;
            EXPECT_EQ(ai1->ai_addrlen, 16);
            ai1.sockaddr(saddr);
            EXPECT_EQ(saddr.netaddrp(), "127.0.0.1:0");
        } else {
            GTEST_FAIL()
                << "  Unexpected address information: address family = "
                << ai1->ai_family << ", socket type = " << ai1->ai_socktype
                << "\n";
        }
    } while (ai1.get_next());
}

TEST(AddrinfoTestSuite, get_active_empty_node_netaddrp_with_unspec_port) {
    // An empty node name will return information about the loopback interface.
    // This is specified by syscall ::getaddrinfo().

    // Test Unit for default AF_UNSPEC.
    CAddrinfo ai1(":0");
    ASSERT_TRUE(ai1.get_first());

    // Address family set by syscal ::getaddrinfo().
    // There should be two results
    bool double_res1{false};
    bool double_res2{false};
    do {
        EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
        EXPECT_EQ(ai1->ai_protocol, 0);
        EXPECT_EQ(ai1->ai_flags, 0);
        EXPECT_NE(ai1->ai_addr, nullptr); // not equal nullptr
        EXPECT_EQ(ai1->ai_canonname, nullptr);
        if (ai1->ai_family == AF_INET6) {
            ASSERT_FALSE(double_res1);
            double_res1 = true;
            EXPECT_EQ(ai1->ai_addrlen, 28);
            ai1.sockaddr(saddr);
            EXPECT_EQ(saddr.netaddrp(), "[::1]:0");
        } else if (ai1->ai_family == AF_INET) {
            ASSERT_FALSE(double_res2);
            double_res2 = true;
            EXPECT_EQ(ai1->ai_addrlen, 16);
            ai1.sockaddr(saddr);
            EXPECT_EQ(saddr.netaddrp(), "127.0.0.1:0");
        } else {
            GTEST_FAIL()
                << "  Unexpected address information: address family = "
                << ai1->ai_family << ", socket type = " << ai1->ai_socktype
                << "\n";
        }
    } while (ai1.get_next());
}

TEST(AddrinfoTestSuite, get_active_empty_node_socktype_0) {
    // Due to "man getaddrinfo" an empty node returns information of the
    // loopback interface, but either node or service, but not both, may be
    // empty. With setting everything unspecified, except service, we get all
    // available combinations with loopback interfaces but different on
    // platforms. 'ai_socktype' specifies the preferred socket type, for
    // example SOCK_STREAM or SOCK_DGRAM. Specifying 0 in this field indicates
    // that socket addresses of any type can be returned by getaddrinfo().

    // Test Unit for default AF_UNSPEC.
    CAddrinfo ai6("", "0", 0 /*flags*/, 0 /*ai_socktype*/);
    ASSERT_TRUE(ai6.get_first());

    int number_of_entries{};
    do {
        // The combination of the next two assertions should give number of
        // entries.
        ASSERT_THAT(ai6->ai_family, AnyOf(AF_INET6, AF_INET));
#ifdef __APPLE__
        ASSERT_THAT(ai6->ai_socktype, AnyOf(SOCK_STREAM, SOCK_DGRAM));
#elif defined(_MSC_VER)
        ASSERT_EQ(ai6->ai_socktype, 0);
#else
        ASSERT_THAT(ai6->ai_socktype, AnyOf(SOCK_STREAM, SOCK_DGRAM, SOCK_RAW));
#endif
        ASSERT_EQ(ai6->ai_protocol, 0);
        ASSERT_EQ(ai6->ai_flags, 0);
        ai6.sockaddr(saddr);
        ASSERT_THAT(saddr.netaddrp(), AnyOf("[::1]:0", "127.0.0.1:0"));
        number_of_entries++;
    } while (ai6.get_next());

#ifdef __APPLE__
    EXPECT_EQ(number_of_entries, 4);
#elif defined(_MSC_VER)
    EXPECT_EQ(number_of_entries, 2);
#else
    EXPECT_EQ(number_of_entries, 6);
#endif
}

TEST(AddrinfoTestSuite, get_multiple_address_infos) {
    CAddrinfo ai1("localhost:50686");
    ASSERT_TRUE(ai1.get_first());

    constexpr int expected_addrinfos{2}; // Value of expected address infos
    int found_addrinfos{};
    do {
        EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
        EXPECT_EQ(ai1->ai_protocol, 0);
        EXPECT_EQ(ai1->ai_flags, 0);
        EXPECT_NE(ai1->ai_addr, nullptr);
        EXPECT_EQ(ai1->ai_canonname, nullptr);

        switch (ai1->ai_family) {
        case AF_INET6:
            EXPECT_EQ(ai1->ai_addrlen, 28);
            ai1.sockaddr(saddr);
            EXPECT_EQ(saddr.netaddrp(), "[::1]:50686");
            break;
        case AF_INET:
            EXPECT_EQ(ai1->ai_addrlen, 16);
            ai1.sockaddr(saddr);
            EXPECT_EQ(saddr.netaddrp(), "127.0.0.1:50686");
            break;
        default:
            GTEST_FAIL() << "  Unexpected address family = " << ai1->ai_family
                         << "\n";
        }
        found_addrinfos++;
    } while (ai1.get_next());

    ASSERT_EQ(found_addrinfos, expected_addrinfos);
}

TEST(AddrinfoTestSuite, get_info_loopback_interface) {
    // If node is not empty AI_PASSIVE is ignored.

    // Test Unit with string port number
    CAddrinfo ai1("[::1]", "50001", AI_PASSIVE | AI_NUMERICHOST);
    ASSERT_TRUE(ai1.get_first());

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_PASSIVE | AI_NUMERICHOST);
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[::1]:50001");

    // Test Unit
    CAddrinfo ai5("[::1]:50085");
    ASSERT_TRUE(ai5.get_first());

    EXPECT_EQ(ai5->ai_family, AF_INET6);
    EXPECT_EQ(ai5->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai5->ai_protocol, 0);
    EXPECT_EQ(ai5->ai_flags, 0);
    ai5.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[::1]:50085");

    // Test Unit
    CAddrinfo ai2("127.0.0.1", "50086", 0, SOCK_DGRAM);
    ASSERT_TRUE(ai2.get_first());

    EXPECT_EQ(ai2->ai_family, AF_INET);
    EXPECT_EQ(ai2->ai_socktype, SOCK_DGRAM);
    EXPECT_EQ(ai2->ai_protocol, 0);
    EXPECT_EQ(ai2->ai_flags, 0);
    ai2.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "127.0.0.1:50086");

    // Test Unit
    CAddrinfo ai3("[::1]", "50087");
    ASSERT_TRUE(ai3.get_first());

    EXPECT_EQ(ai3->ai_family, AF_INET6);
    EXPECT_EQ(ai3->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai3->ai_protocol, 0);
    EXPECT_EQ(ai3->ai_flags, 0);
    ai3.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[::1]:50087");

    // Test Unit, does not trigger a DNS query
    CAddrinfo ai4("localhost:50088");
    ASSERT_TRUE(ai4.get_first());

    EXPECT_THAT(ai4->ai_family, AnyOf(AF_INET6, AF_INET));
    EXPECT_EQ(ai4->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai4->ai_protocol, 0);
    EXPECT_EQ(ai4->ai_flags, 0);
    ai4.sockaddr(saddr);
    EXPECT_THAT(saddr.netaddrp(), AnyOf("[::1]:50088", "127.0.0.1:50088"));
}


// Other tests
// -----------
TEST(AddrinfoTestSuite, query_ipv6_addrinfo_successful) {
    CAddrinfo ai1("[fe80:db8::8%1]:50001");

    // Check the initialized object. This is what we have given with the
    // constructor. We get just the initialized hints.
    EXPECT_EQ(ai1->ai_family, AF_UNSPEC);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, 0);
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
    EXPECT_EQ(ai1->ai_flags, 0);
    EXPECT_EQ(ai1->ai_addrlen, 28);
    EXPECT_NE(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    // I don't know why macOS cripples address to "[fe80::8%lo0]:50001"
    ai1.sockaddr(saddr);
    EXPECT_THAT(saddr.netaddrp(),
                AnyOf("[fe80:db8::8%lo]:50001", "[fe80::8%lo0]:50001",
                      "[fe80:db8::8%1]:50001"));
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
    EXPECT_CALL(m_netdbObj, getaddrinfo(Pointee(*"2001:db8::9"), nullptr,
                                        Field(&addrinfo::ai_flags, 0), _))
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
                            AllOf(Field(&addrinfo::ai_family, AF_UNSPEC),
                                  Field(&addrinfo::ai_flags, 0)),
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
    EXPECT_EQ(ai->ai_flags, 0);
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
    EXPECT_EQ(ai1->ai_flags, 0);
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
    EXPECT_EQ(ai1->ai_flags, 0);
    EXPECT_EQ(ai1->ai_addrlen, 28);
    EXPECT_NE(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    EXPECT_EQ(ai1->ai_next, nullptr);
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[2001:db8::15]:59877");
}

TEST(AddrinfoTestSuite, query_ipv4_addrinfo_successful) {
    CAddrinfo ai1("192.168.200.201");

    // Check the initialized object. This is what we have given with the
    // constructor. We get just the initialized hints.
    EXPECT_EQ(ai1->ai_family, AF_UNSPEC);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, 0);
    EXPECT_EQ(ai1->ai_addrlen, 0);
    EXPECT_EQ(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    EXPECT_EQ(ai1->ai_next, nullptr);
    // There is no address information
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), ":0");

    ASSERT_TRUE(ai1.get_first());

    EXPECT_EQ(ai1->ai_family, AF_INET);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    // EXPECT_EQ(ai1->ai_flags, AI_NUMERICHOST | AI_NUMERICSERV);
    EXPECT_EQ(ai1->ai_flags, 0);
    EXPECT_EQ(ai1->ai_addrlen, 16);
    EXPECT_NE(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    EXPECT_EQ(ai1->ai_next, nullptr);
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "192.168.200.201:0");
}

TEST(AddrinfoTestSuite, load_ipv4_addrinfo_and_port_successful) {
    CAddrinfo ai1("192.168.200.202", "54544");

    ASSERT_TRUE(ai1.get_first());

    EXPECT_EQ(ai1->ai_family, AF_INET);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    // EXPECT_EQ(ai1->ai_flags, AI_NUMERICHOST | AI_NUMERICSERV);
    EXPECT_EQ(ai1->ai_flags, 0);
    EXPECT_EQ(ai1->ai_addrlen, 16);
    EXPECT_NE(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    EXPECT_EQ(ai1->ai_next, nullptr);
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "192.168.200.202:54544");
}

TEST(AddrinfoTestSuite, load_ipv4_addrinfo_with_port_successful) {
    CAddrinfo ai1("192.168.200.203:http");

    ASSERT_TRUE(ai1.get_first());

    EXPECT_EQ(ai1->ai_family, AF_INET);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    // EXPECT_EQ(ai1->ai_flags, AI_NUMERICHOST);
    EXPECT_EQ(ai1->ai_flags, 0);
    EXPECT_EQ(ai1->ai_addrlen, 16);
    EXPECT_NE(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    EXPECT_EQ(ai1->ai_next, nullptr);
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "192.168.200.203:80");
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
    EXPECT_EQ(ai2->ai_flags, AI_PASSIVE | AI_NUMERICHOST);
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
    EXPECT_EQ(ai2->ai_flags, AI_PASSIVE | AI_NUMERICHOST);
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
    EXPECT_EQ(ai1->ai_family, AF_UNSPEC);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_PASSIVE | AI_NUMERICHOST);
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
    // EXPECT_EQ(ai1->ai_flags, AI_NUMERICHOST | AI_NUMERICSERV);
    EXPECT_EQ(ai1->ai_flags, 0);
    EXPECT_EQ(ai1->ai_next, nullptr);
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[2001:db8::5]:50051");

    // Test Unit
    CAddrinfo ai2("192.168.9.10", "50096");
    ASSERT_TRUE(ai2.get_first());

    EXPECT_EQ(ai2->ai_family, AF_INET); // set by syscal ::getaddrinfo
    EXPECT_EQ(ai2->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai2->ai_protocol, 0);
    // EXPECT_EQ(ai2->ai_flags, AI_NUMERICHOST | AI_NUMERICSERV);
    EXPECT_EQ(ai2->ai_flags, 0);
    EXPECT_EQ(ai2->ai_next, nullptr);
    ai2.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "192.168.9.10:50096");
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
                            Field(&addrinfo::ai_flags, 0), _))
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
                            Field(&addrinfo::ai_flags, 0), _))
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
    EXPECT_CALL(m_netdbObj, getaddrinfo(Pointee(*"[::1].4"), nullptr,
                                        Field(&addrinfo::ai_flags, 0), _))
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
    EXPECT_CALL(m_netdbObj, getaddrinfo(Pointee(*"127.0.0.1.5"), nullptr,
                                        Field(&addrinfo::ai_flags, 0), _))
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
                            Field(&addrinfo::ai_flags, 0), _))
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
                            Field(&addrinfo::ai_flags, 0), _))
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

    EXPECT_EQ(ai3->ai_family, AF_UNSPEC);
    EXPECT_EQ(ai3->ai_socktype, 0);
    EXPECT_EQ(ai3->ai_protocol, 0);
    EXPECT_EQ(ai3->ai_flags, AI_NUMERICHOST | AI_NUMERICSERV);
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
        EXPECT_EQ(ai3->ai_flags, AI_NUMERICHOST | AI_NUMERICSERV);
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

    EXPECT_EQ(ai4->ai_family, AF_UNSPEC);
    EXPECT_EQ(ai4->ai_socktype, 0);
    EXPECT_EQ(ai4->ai_protocol, 0);
    EXPECT_EQ(ai4->ai_flags, AI_NUMERICHOST | AI_NUMERICSERV);
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
        EXPECT_EQ(ai4->ai_flags, AI_NUMERICHOST | AI_NUMERICSERV);
        EXPECT_NE(ai4->ai_addr, nullptr); // not equal nullptr
        EXPECT_EQ(ai4->ai_canonname, nullptr);
#if !defined(_MSC_VER)
        if (ai4->ai_family == AF_INET && ai4->ai_socktype == SOCK_STREAM) {
            ASSERT_FALSE(double_res1);
            double_res1 = true;
            EXPECT_EQ(ai4->ai_addrlen, 16);
            ai4.sockaddr(saddr);
            EXPECT_EQ(saddr.netaddrp(), "0.0.0.0:0");
        } else if (ai4->ai_family == AF_INET &&
                   ai4->ai_socktype == SOCK_DGRAM) {
            ASSERT_FALSE(double_res2);
            double_res2 = true;
            EXPECT_EQ(ai4->ai_addrlen, 16);
            ai4.sockaddr(saddr);
            EXPECT_EQ(saddr.netaddrp(), "0.0.0.0:0");
#if !defined(__APPLE__)
        } else if (ai4->ai_family == AF_INET && ai4->ai_socktype == SOCK_RAW) {
            ASSERT_FALSE(double_res3);
            double_res3 = true;
            EXPECT_EQ(ai4->ai_addrlen, 16);
            ai4.sockaddr(saddr);
            EXPECT_EQ(saddr.netaddrp(), "0.0.0.0:0");
#endif
#else // _MSC_VER
        if (ai4->ai_family == AF_INET && ai4->ai_socktype == 0) {
            ASSERT_FALSE(double_res1);
            double_res1 = true;
            EXPECT_EQ(ai4->ai_addrlen, 16);
            ai4.sockaddr(saddr);
            EXPECT_EQ(saddr.netaddrp(), "0.0.0.0:0");
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
        EXPECT_EQ(ai0->ai_flags, AI_PASSIVE);
        ai0.sockaddr(saddr);
        EXPECT_THAT(saddr.netaddrp(), AnyOf("[::]:0", "0.0.0.0:0"));

        // Test Unit for default AF_UNSPEC
        CAddrinfo ai1("", "0", AI_PASSIVE);
        ASSERT_TRUE(ai1.get_first());

        // Ubuntu returns AF_INET, MacOS and Microsoft Windows return AF_INET6
        EXPECT_THAT(ai1->ai_family, AnyOf(AF_INET6, AF_INET));
        EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
        EXPECT_EQ(ai1->ai_protocol, 0);
        EXPECT_EQ(ai1->ai_flags, AI_PASSIVE);
        ai1.sockaddr(saddr);
        EXPECT_THAT(saddr.netaddrp(), AnyOf("[::]:0", "0.0.0.0:0"));

        // Test Unit for default AF_UNSPEC
        CAddrinfo ai2("", "50106", AI_PASSIVE);
        ASSERT_TRUE(ai2.get_first());

        // Ubuntu returns AF_INET, MacOS and Microsoft Windows return AF_INET6
        EXPECT_THAT(ai2->ai_family, AnyOf(AF_INET6, AF_INET));
        EXPECT_EQ(ai2->ai_socktype, SOCK_STREAM);
        EXPECT_EQ(ai2->ai_protocol, 0);
        EXPECT_EQ(ai2->ai_flags, AI_PASSIVE);
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
        EXPECT_EQ(ai3->ai_flags, AI_PASSIVE | AI_NUMERICHOST);
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
        EXPECT_EQ(ai1->ai_flags, AI_PASSIVE | AI_NUMERICHOST);
        // This will listen on all local network interfaces.
        ai1.sockaddr(saddr);
        EXPECT_EQ(saddr.netaddrp(), "[::]:50006");

        // Test Unit
        // Using explicit the unknown netaddress should definetly return the
        // INADDR_ANY passive listening socket info.
        CAddrinfo ai2("0.0.0.0", "50032", AI_PASSIVE | AI_NUMERICHOST);
        ASSERT_TRUE(ai2.get_first());

        EXPECT_EQ(ai2->ai_family, AF_INET);
        EXPECT_EQ(ai2->ai_socktype, SOCK_STREAM);
        EXPECT_EQ(ai2->ai_protocol, 0);
        EXPECT_EQ(ai2->ai_flags, AI_PASSIVE | AI_NUMERICHOST);
        // This will listen on all local network interfaces.
        ai2.sockaddr(saddr);
        EXPECT_EQ(saddr.netaddrp(), "0.0.0.0:50032");

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
    EXPECT_EQ(ai1->ai_flags, AI_NUMERICHOST);
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "[2001:db8::8]:0");
}

TEST(AddrinfoTestSuite, service_out_of_range) {
    // Test Unit
    CAddrinfo ai1("[2001:db8::c9]", "65536", AI_NUMERICHOST);
    ASSERT_FALSE(ai1.get_first());

    EXPECT_EQ(ai1->ai_family, AF_UNSPEC);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_NUMERICHOST);
    ai1.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), ":0");
    EXPECT_THAT(ai1.what(), HasSubstr("] WHAT MSG1128: catched next line"));
    // std::cout << ai1.what() << '\n';
}

TEST(AddrinfoTestSuite, load_loopback_addr_with_scope_id) {
    CAddrinfo ai1("[::1%1]");
    ASSERT_TRUE(ai1.get_first());

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, 0);
    EXPECT_EQ(ai1->ai_addrlen, 28);
    EXPECT_NE(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    ai1.sockaddr(saddr);
    EXPECT_THAT(saddr.netaddrp(),
                Conditional(co == Co::clang, "[::1]:0", "[::1%1]:0"));
    EXPECT_EQ(ai1->ai_next, nullptr);

    CAddrinfo ai2("[::1%lo]");
#ifdef __APPLE__
    ASSERT_TRUE(ai2.get_first());

    EXPECT_EQ(ai2->ai_family, AF_INET6);
    EXPECT_EQ(ai2->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai2->ai_protocol, 0);
    EXPECT_EQ(ai2->ai_flags, 0);
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

TEST(AddrinfoTestSuite, load_lla_with_scope_id) {
    CAddrinfo ai1("[fe80::acd%1]:ssh");
    ASSERT_TRUE(ai1.get_first());

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, 0);
    EXPECT_EQ(ai1->ai_addrlen, 28);
    EXPECT_NE(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    ai1.sockaddr(saddr);
    EXPECT_THAT(
        saddr.netaddrp(),
        AnyOf("[fe80::acd%lo]:22", "[fe80::acd%lo0]:22", "[fe80::acd%1]:22"));
    EXPECT_EQ(ai1->ai_next, nullptr);

    CAddrinfo ai2("[fe80::acd%lo]:ssh");
#ifdef _MSC_VER
    EXPECT_FALSE(ai2.get_first());
#else
    ASSERT_TRUE(ai2.get_first());

    EXPECT_EQ(ai2->ai_family, AF_INET6);
    EXPECT_EQ(ai2->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai2->ai_protocol, 0);
    EXPECT_EQ(ai2->ai_flags, 0);
    EXPECT_EQ(ai2->ai_addrlen, 28);
    EXPECT_NE(ai2->ai_addr, nullptr);
    EXPECT_EQ(ai2->ai_canonname, nullptr);
    ai2.sockaddr(saddr);
    EXPECT_THAT(saddr.netaddrp(), Conditional(co == Co::clang, "[fe80::acd]:22",
                                              "[fe80::acd%lo]:22"));
    EXPECT_EQ(ai2->ai_next, nullptr);
#endif // _MSC_VER
}

TEST(AddrinfoTestSuite, load_gua_with_scope_id) {
    // Unicast addresses are unique worldwide so using a scope id makes no much
    // sense but it is partly possible, except for AppleClang.
    CAddrinfo ai1("[2001:db8::55%1]:https");
    ASSERT_TRUE(ai1.get_first());

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, 0);
    EXPECT_EQ(ai1->ai_addrlen, 28);
    EXPECT_NE(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    // AppleClang accepts scope id (%1) only from link local addresses [fe80::]
    ai1.sockaddr(saddr);
    EXPECT_THAT(saddr.netaddrp(),
                Conditional(co == Co::clang, "[2001:db8::55]:443",
                            "[2001:db8::55%1]:443"));
    EXPECT_EQ(ai1->ai_next, nullptr);

    CAddrinfo ai2("[2001:db8::55%lo]:https");
#if __APPLE__
    ASSERT_TRUE(ai1.get_first());

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, 0);
    EXPECT_EQ(ai1->ai_addrlen, 28);
    EXPECT_NE(ai1->ai_addr, nullptr);
    EXPECT_EQ(ai1->ai_canonname, nullptr);
    // AppleClang accepts scope id (%1) only from link local addresses [fe80::]
    ai1.sockaddr(saddr);
    EXPECT_THAT(saddr.netaddrp(),
                Conditional(co == Co::clang, "[2001:db8::55]:443",
                            "[2001:db8::55%1]:433"));
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
    ASSERT_TRUE(ai2.get_first());
    // Test Unit
    ai2.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), "127.0.0.1:50002");
}

// Veify macro/function IN6_IS_ADDR_GLOBAL
// ---------------------------------------
// Global addressing is all in the 2000::/3 range
//     current range is 2000:: to 3fff:ffff:ffff:ffff:ffff:ffff:ffff:ffff,
//     that could be expanded into the future
// link-local addressing is all in the fe80::/10 range
// (Deprecated ULA is in the fc00::/7 range)
// Multicast is in the ff00::/8 range
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
TEST(AddrinfoTestSuite, check_in6_is_addr_global) {
    {
        // No Global Unicast Address (different on win32)
        CAddrinfo aiObj("[1fff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]");
        aiObj.get_first();
        in6_addr* sa6 =
            &reinterpret_cast<sockaddr_in6*>(aiObj->ai_addr)->sin6_addr;
#ifdef _MSC_VER
        EXPECT_TRUE(IN6_IS_ADDR_GLOBAL(sa6));
#else
        EXPECT_FALSE(IN6_IS_ADDR_GLOBAL(sa6));
#endif
    }
    {
        // First Global Unicast Address (not first on win32)
        CAddrinfo aiObj("[2000::]");
        aiObj.get_first();
        in6_addr* sa6 =
            &reinterpret_cast<sockaddr_in6*>(aiObj->ai_addr)->sin6_addr;
        EXPECT_TRUE(IN6_IS_ADDR_GLOBAL(sa6));
    }
    {
        // Documentation- and test-address
        CAddrinfo aiObj("[2001:db8::1]");
        aiObj.get_first();
        in6_addr* sa6 =
            &reinterpret_cast<sockaddr_in6*>(aiObj->ai_addr)->sin6_addr;
        EXPECT_TRUE(IN6_IS_ADDR_GLOBAL(sa6));
    }
    {
        // Last Global Unicast Address (not last on win32)
        CAddrinfo aiObj("[3fff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]");
        aiObj.get_first();
        in6_addr* sa6 =
            &reinterpret_cast<sockaddr_in6*>(aiObj->ai_addr)->sin6_addr;
        EXPECT_TRUE(IN6_IS_ADDR_GLOBAL(sa6));
    }
    {
        // No Global Unicast Address (different on win32)
        CAddrinfo aiObj("[4000::]");
        aiObj.get_first();
        in6_addr* sa6 =
            &reinterpret_cast<sockaddr_in6*>(aiObj->ai_addr)->sin6_addr;
#ifdef _MSC_VER
        EXPECT_TRUE(IN6_IS_ADDR_GLOBAL(sa6));
#else
        EXPECT_FALSE(IN6_IS_ADDR_GLOBAL(sa6));
#endif
    }
    {
        // Link-local address
        CAddrinfo aiObj("[fe80::1]");
        aiObj.get_first();
        in6_addr* sa6 =
            &reinterpret_cast<sockaddr_in6*>(aiObj->ai_addr)->sin6_addr;
        EXPECT_FALSE(IN6_IS_ADDR_GLOBAL(sa6));
    }

    // starting with 00 is the reserved address block
    {
        // Address belongs to the reserved address block
        CAddrinfo aiObj("[00ab::1]");
        aiObj.get_first();
        in6_addr* sa6 =
            &reinterpret_cast<sockaddr_in6*>(aiObj->ai_addr)->sin6_addr;
        EXPECT_FALSE(IN6_IS_ADDR_GLOBAL(sa6));
    }
    {
        // Unspecified Address, belongs to the reserved address block
        CAddrinfo aiObj("[::]");
        aiObj.get_first();
        in6_addr* sa6 =
            &reinterpret_cast<sockaddr_in6*>(aiObj->ai_addr)->sin6_addr;
        EXPECT_FALSE(IN6_IS_ADDR_GLOBAL(sa6));
        EXPECT_TRUE(IN6_IS_ADDR_UNSPECIFIED(sa6));
    }
    {
        // loopback address, belongs to the reserved address block
        CAddrinfo aiObj("[::1]");
        aiObj.get_first();
        in6_addr* sa6 =
            &reinterpret_cast<sockaddr_in6*>(aiObj->ai_addr)->sin6_addr;
        EXPECT_FALSE(IN6_IS_ADDR_GLOBAL(sa6));
        EXPECT_TRUE(IN6_IS_ADDR_LOOPBACK(sa6));
    }
    {
        // v4-mapped address, belongs to the reserved address block
        CAddrinfo aiObj("[::ffff:142.250.185.99]");
        aiObj.get_first();
        in6_addr* sa6 =
            &reinterpret_cast<sockaddr_in6*>(aiObj->ai_addr)->sin6_addr;
        EXPECT_FALSE(IN6_IS_ADDR_GLOBAL(sa6));
        EXPECT_TRUE(IN6_IS_ADDR_V4MAPPED(sa6));
    }
    {
        // IPv4-compatible embedded IPv6 address, belongs to the reserved
        // address block
        CAddrinfo aiObj("[::101.45.75.219]");
        aiObj.get_first();
        in6_addr* sa6 =
            &reinterpret_cast<sockaddr_in6*>(aiObj->ai_addr)->sin6_addr;
        EXPECT_FALSE(IN6_IS_ADDR_GLOBAL(sa6));
        EXPECT_TRUE(IN6_IS_ADDR_V4COMPAT(sa6));
    }
}

TEST(AddrinfoTestSuite, get_sockaddr) {
    CAddrinfo aiObj("[2001:db8::2%1]:47111");
    aiObj.get_first();

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
