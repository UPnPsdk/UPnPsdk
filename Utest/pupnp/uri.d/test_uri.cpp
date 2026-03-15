// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// redistribution only with this copyright remark. last modified: 2026-03-15

// Unit Tests for code that exists only in pUPnP.

// Helpful link for ip address structures:
// https://stackoverflow.com/a/16010670/5014688

// Include source code for testing. So we have also direct access to static
// functions which need to be tested.
#include <Pupnp/upnp/src/genlib/net/uri/uri.cpp>

#include <umock/netdb_mock.hpp>
#include <utest/utest.hpp>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;


namespace utest {

// Mocking
// =======
class Mock_netv4info : public umock::NetdbMock {
    // This is a derived class from mocking netdb to provide a structure for
    // addrinfo that can be given to the mocked program.
  private:
    // Provide structures to mock system call for network address
    struct sockaddr_in m_sa{};
    struct addrinfo m_res{};

  public:
    Mock_netv4info() { m_sa.sin_family = AF_INET; }

    addrinfo* set(const char* a_ipaddr, uint16_t a_port) {
        inet_pton(m_sa.sin_family, a_ipaddr, &m_sa.sin_addr);
        m_sa.sin_port = htons(a_port);

        m_res.ai_family = m_sa.sin_family;
        m_res.ai_addrlen = sizeof(struct sockaddr);
        m_res.ai_addr = (sockaddr*)&m_sa;

        return &m_res;
    }
};


// parse_hostport() function: tests from the uri module
// ====================================================

// parse_hostport() checks that getaddrinfo() isn't called.
// --------------------------------------------------------
class HostportIp4FTestSuite : public ::testing::Test {
  protected:
    hostport_type m_out;
    sockaddr_in* m_sai4{reinterpret_cast<sockaddr_in*>(&m_out.IPaddress)};

    // Provide empty structures for mocking. Will be filled in the tests.
    sockaddr_in m_sa{};
    addrinfo m_res{};

    umock::NetdbMock netdbObj;

    HostportIp4FTestSuite() {
        // Complete the addrinfo structure
        m_res.ai_addrlen = sizeof(sockaddr);
        m_res.ai_addr = reinterpret_cast<sockaddr*>(&m_sa);

        // Set default return values for getaddrinfo in case we get an
        // unexpected call but it should not occur. I only use an ip address
        // here so name resolution should be called.
        ON_CALL(netdbObj, getaddrinfo(_, _, _, _))
            .WillByDefault(DoAll(SetArgPointee<3>(&m_res), Return(EAI_NONAME)));
        EXPECT_CALL(netdbObj, getaddrinfo(_, _, _, _)).Times(0);
        EXPECT_CALL(netdbObj, freeaddrinfo(_)).Times(0);
    }
};

TEST_F(HostportIp4FTestSuite, parse_ip_address_without_port) {
    // ip address without port
    EXPECT_EQ(parse_hostport("192.168.1.1", 80, &m_out), 11);
    EXPECT_STREQ(m_out.text.buff, "192.168.1.1");
    EXPECT_EQ(m_out.text.size, 11);
    EXPECT_EQ(m_sai4->sin_family, AF_INET);
    EXPECT_EQ(m_sai4->sin_port, htons(80));
    EXPECT_STREQ(inet_ntoa(m_sai4->sin_addr), "192.168.1.1");
}

TEST_F(HostportIp4FTestSuite, parse_ip_address_with_port) {
    // Ip address with port
    EXPECT_EQ(parse_hostport("192.168.1.2:443", 80, &m_out), 15);
    // DefaultPort 80 is ignored.
    EXPECT_STREQ(m_out.text.buff, "192.168.1.2:443");
    EXPECT_EQ(m_out.text.size, 15);
    EXPECT_EQ(m_sai4->sin_family, AF_INET);
    EXPECT_EQ(m_sai4->sin_port, htons(443));
    EXPECT_STREQ(inet_ntoa(m_sai4->sin_addr), "192.168.1.2");
}


// parse_hostport() calls should fail
// ----------------------------------
class HostportFailIp4PTestSuite
    : public ::testing::TestWithParam<
          //           uristr,      ipaddr,      port
          ::std::tuple<const char*, const char*, uint16_t>> {};

TEST_P(HostportFailIp4PTestSuite, parse_name_with_scheme) {
    // Get parameter
    ::std::tuple params = GetParam();
    const char* uristr = ::std::get<0>(params);
    const char* ipaddr = ::std::get<1>(params);
    const uint16_t port = ::std::get<2>(params);

    Mock_netv4info netv4inf;
    addrinfo* res = netv4inf.set(ipaddr, port);

    // Mock for network address system call
    umock::Netdb netdb_injectObj(&netv4inf);
    EXPECT_CALL(netv4inf, getaddrinfo(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<3>(res), Return(EAI_NONAME)));
    EXPECT_CALL(netv4inf, freeaddrinfo(_)).Times(0);

    // Provide arguments to execute the unit
    hostport_type out{};
    sockaddr_in* sai4 = reinterpret_cast<sockaddr_in*>(&out.IPaddress);

    // Execute the unit
    EXPECT_EQ(parse_hostport(uristr, 80, &out), UPNP_E_INVALID_URL);
    EXPECT_STREQ(out.text.buff, nullptr);
    EXPECT_EQ(out.text.size, 0);
    EXPECT_EQ(sai4->sin_family, AF_UNSPEC);
    EXPECT_EQ(sai4->sin_port, 0);
    EXPECT_STREQ(inet_ntoa(sai4->sin_addr), "0.0.0.0");
}

INSTANTIATE_TEST_SUITE_P(
    uri, HostportFailIp4PTestSuite,
    ::testing::Values(
        // clang-format off
        //                 uristr,              ipaddr,        port
        ::std::make_tuple("http://192.168.1.3", "192.168.1.3", 80),
        ::std::make_tuple("/192.168.1.4:433", "192.168.1.4", 433),
        ::std::make_tuple("http://example.com/site", "192.168.1.5", 80),
        ::std::make_tuple(":example.com:443/site", "192.168.1.6", 433)));
// clang-format on


// parse_hostport() calls should be successful
// -------------------------------------------
class HostportIp4PTestSuite
    : public ::testing::TestWithParam<
          //           uristr,      ipaddr,      port
          ::std::tuple<const char*, const char*, uint16_t>> {};

TEST_P(HostportIp4PTestSuite, parse_hostport_successful) {
    // Get parameter
    ::std::tuple params = GetParam();
    const char* uristr = ::std::get<0>(params);
    const char* ipaddr = ::std::get<1>(params);
    const uint16_t port = ::std::get<2>(params);
    const size_t size = ::strcspn(uristr, "/");

    Mock_netv4info netv4inf;
    addrinfo* res = netv4inf.set(ipaddr, port);

    // Mock for network address system call
    umock::Netdb netdb_injectObj(&netv4inf);
    EXPECT_CALL(netv4inf, getaddrinfo(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<3>(res), Return(0)));
    EXPECT_CALL(netv4inf, freeaddrinfo(_)).Times(1);

    // Provide arguments to execute the unit
    hostport_type out{};
    sockaddr_in* sai4 = reinterpret_cast<sockaddr_in*>(&out.IPaddress);

    // Execute the unit
    EXPECT_EQ(parse_hostport(uristr, 80, &out), size);
    EXPECT_STREQ(out.text.buff, uristr);
    EXPECT_EQ(out.text.size, size);
    EXPECT_EQ(sai4->sin_family, AF_INET);
    EXPECT_EQ(sai4->sin_port, htons(port));
    EXPECT_STREQ(inet_ntoa(sai4->sin_addr), ipaddr);
}

INSTANTIATE_TEST_SUITE_P(
    uri, HostportIp4PTestSuite,
    ::testing::Values(
        //                 uristr,      ipaddr,     port
        ::std::make_tuple("localhost", "127.0.0.1", 80),
        ::std::make_tuple("example.com", "192.168.1.3", 80),
        ::std::make_tuple("example.com:433", "192.168.1.4", 433),
        ::std::make_tuple("example-site.de/path?query#fragment", "192.168.1.5",
                          80),
        ::std::make_tuple("example-site.de:433/path?query#fragment",
                          "192.168.1.5", 433)));


TEST(UriTestSuite, parse_hostport_ip6_without_name_resolution) {
    hostport_type out;

    // Check IPv6 addresses
    sockaddr_in6* sai6 = reinterpret_cast<sockaddr_in6*>(&out.IPaddress);
    char dst[128];

    EXPECT_EQ(
        parse_hostport("[2001:db8:85a3:8d3:1319:8a2e:370:7348]:443", 80, &out),
        42);
    EXPECT_STREQ(out.text.buff, "[2001:db8:85a3:8d3:1319:8a2e:370:7348]:443");
    EXPECT_EQ(out.text.size, 42);
    EXPECT_EQ(sai6->sin6_family, AF_INET6);
    EXPECT_EQ(sai6->sin6_port, htons(443));
    inet_ntop(AF_INET6, sai6->sin6_addr.s6_addr, dst, sizeof(dst));
    EXPECT_STREQ(dst, "2001:db8:85a3:8d3:1319:8a2e:370:7348");
}


// replace_escaped() function: tests from the uri module
// =====================================================
TEST(UriTestSuite, replace_escaped_ip4_check_buffer) {
    const char escstr[]{"%20"};
    size_t max = sizeof(escstr);
    char strbuf[sizeof(escstr)];
    memset(strbuf, 0xFF, max);
    EXPECT_EQ(strbuf[3], '\xFF');
    // Copies escstr with null terminator
    strcpy(strbuf, escstr);

    EXPECT_EQ(strbuf[0], '%');
    EXPECT_EQ(strbuf[1], '2');
    EXPECT_EQ(strbuf[2], '0');
    EXPECT_EQ(strbuf[3], '\0');
    EXPECT_EQ(strbuf[max - 1], '\0');

    // Test Unit; will fill trailing bytes with null
    ASSERT_EQ(::replace_escaped(strbuf, 0, &max), 1);
    EXPECT_EQ(strbuf[0], ' ');
    EXPECT_EQ(strbuf[1], '\0');
    EXPECT_EQ(strbuf[2], '\0');
    EXPECT_EQ(strbuf[3], '\0');
    EXPECT_EQ(max, sizeof(escstr) - 2);
}

TEST(UriTestSuite, replace_escaped_ip4) {
    const char escstr[]{"Hello%20%0AWorld%G0!%0x"};
    size_t max = sizeof(escstr);
    char strbuf[sizeof(escstr)];
    memset(strbuf, 0xFF, max);
    strcpy(strbuf, escstr);
    EXPECT_EQ(strbuf[max - 1], '\0');

    // Test Unit
    // The function converts only one escaped character and the index must
    // exactly point to its '%'.
    ASSERT_EQ(::replace_escaped(strbuf, 5, &max), 1);
    EXPECT_STREQ(strbuf, "Hello %0AWorld%G0!%0x");
    // max buffer length is reduced by two characters (redudce '%xx' to ' ').
    EXPECT_EQ(max, sizeof(escstr) - 2);
    EXPECT_EQ(max, strlen(strbuf) + 1);
    // The two trailing free characters are filled with '\0'.
    EXPECT_EQ(strbuf[max - 2], 'x');
    EXPECT_EQ(strbuf[max - 1], '\0'); // regular new delimiter
    EXPECT_EQ(strbuf[max], '\0');     // filled
    EXPECT_EQ(strbuf[max + 1], '\0'); // filled
    EXPECT_EQ(max + 2, sizeof(strbuf));

    // Not pointing to an escaped character
    EXPECT_EQ(::replace_escaped(strbuf, 0, &max), 0);
    // No hex values after escape character
    EXPECT_EQ(::replace_escaped(strbuf, 16, &max), 0);
    EXPECT_EQ(::replace_escaped(strbuf, 20, &max), 0);
    // Failures should not modify output.
    EXPECT_STREQ(strbuf, "Hello %0AWorld%G0!%0x");
    EXPECT_EQ(max, sizeof(escstr) - 2);
}


// remove_escaped_chars() function: tests from the uri module
// ==========================================================
class RemoveEscCharsPTestSuite
    : public ::testing::TestWithParam<
          //           escstr,      resultstr,   resultsize
          ::std::tuple<const char*, const char*, size_t>> {};

TEST_P(RemoveEscCharsPTestSuite, remove_escaped_chars) {
    // Get parameter
    ::std::tuple params = GetParam();
    const char* escstr = ::std::get<0>(params);
    size_t size{strlen(escstr)}; // without '\0'

    // string buffer; we have to set it with a constant because we cannot get
    // the string size from pointer escstr. But there is a guard (ASSERT_GT).
    // strbuf must be one byte greater for '\0' than the strlen of escstr.
    char strbuf[32];
    memset(strbuf, 0xAA, sizeof(strbuf)); // Fill with garbage.
    ASSERT_GT(sizeof(strbuf), size)
        << "Error: string buffer too small for testing. You must increase it "
           "in this test.\n";
    strcpy(strbuf, escstr); // with '\0'

    // Test Unit.
    EXPECT_EQ(::remove_escaped_chars(strbuf, &size), UPNP_E_SUCCESS);
    EXPECT_STREQ(strbuf, ::std::get<1>(params)); // new result string
    EXPECT_EQ(size, ::std::get<2>(params));      // new result size
}

INSTANTIATE_TEST_SUITE_P(
    uri, RemoveEscCharsPTestSuite,
    ::testing::Values(
        // clang-format off
        // Using reserved generic delimiters (RFC3986 2.2.), execpt %20.
        //                 escstr,                   resultstr,       resultsize
        ::std::make_tuple("%5BHello%20world%21%5D", "[Hello world!]", 14),
        ::std::make_tuple("", "", 0),
        ::std::make_tuple("%20", " ", 1),
        ::std::make_tuple("%3A%2F", ":/", 2),
        ::std::make_tuple("hello", "hello", 5)
        // clang-format on
        ));


TEST(UriDeathTest, remove_escaped_chars_edge_conditions) {
#if defined(__APPLE__) && !defined(DEBUG)
    // With this "Release" build there is a curious situation on MacOS. The
    // normal test with EXPECT_EQ(), as shown below, fails with a
    // "SIGTRAP***Exception". The EXPECT_DEATH() test exits with "failed to
    // die." The EXPECT_EXIT() test fails with "Signal(11)". Huh? Seems we have
    // a Schroedinger thing: the test itself modifies the test. Maybe it has
    // something to do with the 'assert()' debug function that is disabled
    // here? Anyway, the new code is fixed.

    //     EXPECT_EQ(::remove_escaped_chars(nullptr, nullptr),
    //               UPNP_E_SUCCESS);
    std::cout << CYEL "[    FIX   ] " CRES << __LINE__
              << ": Calling Unit with nullptr should not segfault.\n";

#elif defined(__APPLE__) && defined(DEBUG)
    EXPECT_DEATH({ ::remove_escaped_chars(nullptr, nullptr); }, ".*"); // Wrong!

#elif defined(__linux__) || defined(_MSC_VER)
    std::cout << CYEL "[    FIX   ] " CRES << __LINE__
              << ": Calling Unit with nullptr should not segfault.\n";
    EXPECT_DEATH({ ::remove_escaped_chars(nullptr, nullptr); }, ".*"); // Wrong!
#endif

#ifndef __APPLE__
    char strbuf[]{"hello"};
    size_t size{sizeof(strbuf) - 1};
    EXPECT_DEATH({ ::remove_escaped_chars(nullptr, &size); }, ".*");  // Wrong!
    EXPECT_DEATH({ ::remove_escaped_chars(strbuf, nullptr); }, ".*"); // Wrong!
#endif
}


// remove_dots() function: tests from the uri module
// =================================================
class RemoveDotsIp4PTestSuite
    : public ::testing::TestWithParam<
          //           path,        size,   result,      retval
          ::std::tuple<const char*, size_t, const char*, int>> {};

TEST_P(RemoveDotsIp4PTestSuite, remove_dots) {
    // Get parameter
    ::std::tuple params = GetParam();
    const char* path = ::std::get<0>(params);
    size_t size = ::std::get<1>(params);
    const char* result = ::std::get<2>(params);
    int retval = ::std::get<3>(params);

    // string buffer; we have to set it with a constant because we cannot get
    // the string size from pointer path. But there is a guard (ASSERT_GT).
    // strbuf must be one byte greater for '\0' than the strlen of path.
    char strbuf[32];
    ASSERT_GT(sizeof(strbuf), strlen(path))
        << "Error: string buffer too small for testing. You must increase it "
           "in this test.\n";
    strcpy(strbuf, path); // with '\0'

    // Prozess the unit.
    EXPECT_EQ(::remove_dots(strbuf, size), retval);
    EXPECT_STREQ(strbuf, result);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    uri, RemoveDotsIp4PTestSuite,
    ::testing::Values(
        //                 path,    size, result,  retval
/*00*/  ::std::make_tuple("../bar", 6, "bar", UPNP_E_SUCCESS),
        ::std::make_tuple("/../bar", 7, "/bar", UPNP_E_SUCCESS),
        ::std::make_tuple("./bar", 5, "bar", UPNP_E_SUCCESS),
        ::std::make_tuple(".././bar", 8, "bar", UPNP_E_SUCCESS),
        ::std::make_tuple("/foo/./bar", 10, "/foo/bar", UPNP_E_SUCCESS),
        ::std::make_tuple("/bar/./", 7, "/bar/", UPNP_E_SUCCESS),
        ::std::make_tuple("/.", 2, "/", UPNP_E_SUCCESS),
        ::std::make_tuple("/bar/.", 6, "/bar/", UPNP_E_SUCCESS),
        ::std::make_tuple("/foo/../bar", 11, "/bar", UPNP_E_SUCCESS),
        ::std::make_tuple("/bar/../", 8, "/", UPNP_E_SUCCESS),
/*10*/  ::std::make_tuple("/..", 3, "/", UPNP_E_SUCCESS),
        ::std::make_tuple("bar/..", 6, "/", UPNP_E_SUCCESS),
        ::std::make_tuple("/foo/bar/..", 11, "/foo/", UPNP_E_SUCCESS),
        ::std::make_tuple(".", 1, "", UPNP_E_SUCCESS),
        ::std::make_tuple("..", 2, "", UPNP_E_SUCCESS),
        ::std::make_tuple("../", 3, "", UPNP_E_SUCCESS),
        ::std::make_tuple("./", 2, "", UPNP_E_SUCCESS),
        ::std::make_tuple("/./", 3, "/", UPNP_E_SUCCESS),
        ::std::make_tuple("/./bar", 6, "/bar", UPNP_E_SUCCESS),
        ::std::make_tuple("/../", 4, "/", UPNP_E_SUCCESS),
        // Seems that invalid URLs are just returned.
        ::std::make_tuple(".../", 4, ".../", UPNP_E_SUCCESS),
        ::std::make_tuple(".../bar", 7, ".../bar", UPNP_E_SUCCESS),
        ::std::make_tuple("/./hello/foo/../bar", 19, "/hello/bar", UPNP_E_SUCCESS)
    ));
// clang-format on

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // Managed in gtest_main.inc
}
