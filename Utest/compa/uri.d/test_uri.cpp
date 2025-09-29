// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// redistribution only with this copyright remark. last modified: 2025-10-01

// Helpful link for ip address structures:
// https://stackoverflow.com/a/16010670/5014688

// Include source code for testing. So we have also direct access to static
// functions which need to be tested.
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
#include <Pupnp/upnp/src/genlib/net/uri/uri.cpp>
#include <Pupnp/upnp/src/gena/gena_device.cpp>
#else
#include <Compa/src/genlib/net/uri/uri.cpp>
#endif

#include <membuffer.hpp>
#include <UPnPsdk/sockaddr.hpp>
#include <utest/utest.hpp>
#include <umock/netdb_mock.hpp>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StartsWith;

using ::UPnPsdk::SSockaddr;


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


// token_cmp() functions: tests from the uri module
// ================================================
int token_string_cmp(const token* in1, const char* in2) {
    // This is a new function I use for tests. It will become part of new code.
    /*
     * \brief Compares buffer in the token object with the C string in in2 case
     * sensitive.
     *
     * \return
     *      \li < 0, if string1 is less than string2.
     *      \li == 0, if string1 is identical to string2 .
     *      \li > 0, if string1 is greater than string2.
     */
    if (in1 == nullptr && in2 == nullptr)
        return 0;
    if (in1 == nullptr && in2 != nullptr)
        return -1;
    if (in1 != nullptr && in2 == nullptr)
        return 1;
    // Here we have a valid in2 pointer to a C string, even if the string is
    // empty. If there is a nullptr in the token buffer then it is always
    // smaller.
    if (in1->buff == nullptr)
        return -1;

    const size_t in2_length = strlen(in2);
    if (in1->size != in2_length)
        return in1->size < in2_length ? -1 : 1;
    else
        return strncmp(in1->buff, in2, in1->size);
}

TEST(UriTestSuite, token_cmp) {
    // == 0, if string1 is identical to string2.
    token inull{nullptr, 0};
    EXPECT_EQ(::token_cmp(&inull, &inull), 0);

    token in0{"", 0};
    EXPECT_EQ(::token_cmp(&in0, &in0), 0);

    token in1{"some entry", 10};
    EXPECT_EQ(::token_cmp(&in1, &in1), 0);

    // < 0, if string1 is less than string2.
    token in2{"some longer entry", 17};

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": With string1 less than string2 it should return < 0 "
                     "not > 0.\n";
        EXPECT_GT(::token_cmp(&in1, &in2), 0); // Wrong!

    } else {

        EXPECT_LT(::token_cmp(&in1, &in2), 0)
            << "  # With string1 less than string2 it should return < 0 not > "
               "0.";
    }

    // > 0, if string1 is greater than string2.
    token in3{"entry", 5};
    EXPECT_GT(::token_cmp(&in1, &in3), 0);
}

TEST(UriTestSuite, token_string_cmp) {
    // == 0, if string1 is identical to string2.
    EXPECT_EQ(token_string_cmp(nullptr, nullptr), 0);

    constexpr token in0{"", 0};
    constexpr char sinempty[]{""};
    EXPECT_EQ(token_string_cmp(&in0, sinempty), 0);

    constexpr token in1{"some entry", 10};
    constexpr char sin1[]{"some entry"};
    EXPECT_EQ(token_string_cmp(&in1, sin1), 0);

    // < 0, if string1 is less than string2.
    EXPECT_LT(token_string_cmp(nullptr, sinempty), 0);

    constexpr token inull{nullptr, 0};
    EXPECT_LT(token_string_cmp(&inull, sinempty), 0);

    constexpr char sin2[]{"some longer entry"};
    EXPECT_LT(token_string_cmp(&in1, sin2), 0);

    // > 0, if string1 is greater than string2.
    EXPECT_GT(token_string_cmp(&inull, nullptr), 0);

    constexpr char sin3[]{"entry"};
    EXPECT_GT(token_string_cmp(&in1, sin3), 0);
}

TEST(UriDeathTest, token_string_casecmp) {
    // == 0, if string1 is identical to string2 case insensitive.
    token inull{nullptr, 0};
    constexpr char instr0[]{""};

    if (old_code) {
        std::cerr << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": A nullptr in the token structure must not segfault on "
                     "MS Windows.\n";
#ifdef _MSC_VER
        // This expects segfault.
        EXPECT_DEATH(
            { ::token_string_casecmp(&inull, instr0); },
            ".*"); // Wrong!
#endif
    } else {

        // This expects NO segfault.
        ASSERT_EXIT(
            {
                ::token_string_casecmp(&inull, instr0);
                exit(0);
            },
            ::testing::ExitedWithCode(0), ".*")
            << "  A nullptr in the token structure must not segfault.\n";

        EXPECT_EQ(::token_string_casecmp(&inull, instr0), 0);
        constexpr char instr1[]{"X"};
        EXPECT_EQ(::token_string_casecmp(&inull, instr1), -1);
    }

    token in0{"", 0};
    EXPECT_EQ(::token_string_casecmp(&in0, instr0), 0);

    token in1{"some entry", 10};
    constexpr char instr10[]{"SOME ENTRY"};
    EXPECT_EQ(::token_string_casecmp(&in1, instr10), 0);

    // < 0, if string1 is less than string2.
    constexpr char instr17[]{"some longer entry"};

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": With string1 less than string2 it should return < 0 "
                     "not > 0.\n";
        EXPECT_GT(::token_string_casecmp(&in1, instr17), 0); // Wrong!

    } else {

        EXPECT_LT(::token_string_casecmp(&in1, instr17), 0)
            << "  # With string1 less than string2 it should return < 0 not > "
               "0.";
    }

    // > 0, if string1 is greater than string2.
    constexpr char instr5[]{"entry"};
    EXPECT_GT(::token_string_casecmp(&in1, instr5), 0);
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
class RemoveEscCharsIp4PTestSuite
    : public ::testing::TestWithParam<
          //           escstr,      resultstr,   resultsize
          ::std::tuple<const char*, const char*, size_t>> {};

TEST_P(RemoveEscCharsIp4PTestSuite, remove_escaped_chars) {
    // Get parameter
    ::std::tuple params = GetParam();
    const char* escstr = ::std::get<0>(params);
    size_t size{strlen(escstr)}; // without '\0'

    // string buffer; we have to set it with a constant because we cannot get
    // the string size from pointer escstr. But there is a guard (ASSERT_GT).
    // strbuf must be one byte greater for '\0' than the strlen of escstr.
    char strbuf[32];
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
    uri, RemoveEscCharsIp4PTestSuite,
    ::testing::Values(
        //                 escstr,                   resultstr,       resultsize
        ::std::make_tuple("%3CHello%20world%21%3E", "<Hello world!>", 14),
        ::std::make_tuple("", "", 0), ::std::make_tuple("%20", " ", 1),
        ::std::make_tuple("%3C%3E", "<>", 2),
        ::std::make_tuple("hello", "hello", 5)));


TEST(UriDeathTest, remove_escaped_chars_ip4_edge_conditions) {
    char strbuf[32]{};
    size_t size{strlen(strbuf)};

#if defined(__APPLE__) && !defined(DEBUG)
    // With this "Release" build there is a curious situation on MacOS. The
    // normal test with EXPECT_EQ(), as shown below, fails with a
    // "SIGTRAP***Exception". The EXPECT_DEATH() test exits with "failed to
    // die." The EXPECT_EXIT() test fails with "Signal(11)". Huh? Seems we have
    // a Schroedinger thing: the test itself modifies the test. Maybe it has
    // something to do with the 'assert()' debug function that is disabled
    // here? Anyway, the new code is fixed.
    if (old_code) {
        //     EXPECT_EQ(::remove_escaped_chars(nullptr, nullptr),
        //               UPNP_E_SUCCESS);
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": Calling Unit with nullptr should not segfault.\n";
    }

#elif defined(__APPLE__) && defined(DEBUG)
    if (old_code) {
        EXPECT_DEATH(
            { ::remove_escaped_chars(nullptr, nullptr); },
            ".*"); // Wrong!
    }

#elif defined(__linux__) || defined(_MSC_VER)
    if (old_code) {
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": Calling Unit with nullptr should not segfault.\n";
        EXPECT_DEATH(
            { ::remove_escaped_chars(nullptr, nullptr); },
            ".*"); // Wrong!
    }
#endif

    if (!old_code) {
        ASSERT_EXIT(
            {
                ::remove_escaped_chars(nullptr, nullptr);
                exit(0);
            },
            ::testing::ExitedWithCode(0), ".*")
            << "  Calling Unit with nullptr should not segfault.";
        EXPECT_EQ(::remove_escaped_chars(nullptr, nullptr), UPNP_E_SUCCESS);
    }

    strcpy(strbuf, "hello"); // with '\0'
    size = strlen(strbuf);

    if (old_code) {
#ifndef __APPLE__
        EXPECT_DEATH(
            { ::remove_escaped_chars(nullptr, &size); },
            ".*"); // Wrong!
#endif
    } else {
        ASSERT_EXIT((::remove_escaped_chars(nullptr, &size), exit(0)),
                    ::testing::ExitedWithCode(0), ".*")
            << "  Calling Unit with nullptr should not segfault.";
        EXPECT_EQ(::remove_escaped_chars(nullptr, &size), UPNP_E_SUCCESS);
        EXPECT_EQ(size, 5u);
    }

    if (old_code) {
#ifndef __APPLE__
        EXPECT_DEATH(
            { ::remove_escaped_chars(strbuf, nullptr); },
            ".*"); // Wrong!
#endif
    } else {
        ASSERT_EXIT((::remove_escaped_chars(strbuf, nullptr), exit(0)),
                    ::testing::ExitedWithCode(0), ".*")
            << "  Calling Unit with nullptr should not segfault.";
        EXPECT_EQ(::remove_escaped_chars(strbuf, nullptr), UPNP_E_SUCCESS);
        EXPECT_STREQ(strbuf, "hello");
    }
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


// resolve_rel_url() function: tests from the uri module
// =====================================================

TEST(UriTestSuite, resolve_rel_url_ip4_arg1_nullptr_base_url) {
    // nullptr_base_url_returns_rel_url
    Mock_netv4info netv4inf;
    addrinfo* res = netv4inf.set("0.0.0.0", 0);

    // Mock for network address system call
    umock::Netdb netdb_injectObj(&netv4inf);
    ON_CALL(netv4inf, getaddrinfo(_, _, _, _))
        .WillByDefault(DoAll(SetArgPointee<3>(res), Return(EAI_NONAME)));
    EXPECT_CALL(netv4inf, getaddrinfo(_, _, _, _)).Times(0);
    EXPECT_CALL(netv4inf, freeaddrinfo(_)).Times(0);

    // Provide arguments to execute the unit
    char rel_url[]{"homepage#this-fragment"};

    // Test Unit
    char* abs_url = ::resolve_rel_url(nullptr, *&rel_url);
    EXPECT_STREQ(abs_url, "homepage#this-fragment");
    free(abs_url);
}

TEST(UriDeathTest, resolve_rel_url_ip4_arg2_nullptr_rel_url) {
    // nullptr_rel_url_returns_base_url
    // Provide arguments to execute the unit
    char base_url[]{"http://example.com"};

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": nullptr rel_url must not segfault.\n";
        EXPECT_DEATH(
            { free(::resolve_rel_url(base_url, nullptr)); },
            ".*"); // Wrong!

    } else {

        ASSERT_EXIT(
            {
                free(::resolve_rel_url(base_url, nullptr));
                exit(0);
            },
            ::testing::ExitedWithCode(0), ".*")
            << "  nullptr rel_url must not segfault.";

        Mock_netv4info netv4inf;
        addrinfo* res = netv4inf.set("0.0.0.0", 0);

        // Mock for network address system call
        umock::Netdb netdb_injectObj(&netv4inf);
        ON_CALL(netv4inf, getaddrinfo(_, _, _, _))
            .WillByDefault(DoAll(SetArgPointee<3>(res), Return(EAI_NONAME)));
        EXPECT_CALL(netv4inf, getaddrinfo(_, _, _, _)).Times(0);
        EXPECT_CALL(netv4inf, freeaddrinfo(_)).Times(0);

        // Test Unit
        char* abs_url = ::resolve_rel_url(base_url, nullptr);
        EXPECT_STREQ(abs_url, "http://example.com");
        free(abs_url);
    }
}

TEST(UriTestSuite, resolve_rel_url_ip4_arg1_arg2_nullptr_base_and_rel_url) {
    // nullptr_base_and_rel_url_returns_nullptr
    Mock_netv4info netv4inf;
    addrinfo* res = netv4inf.set("0.0.0.0", 0);

    // Mock for network address system call
    umock::Netdb netdb_injectObj(&netv4inf);
    ON_CALL(netv4inf, getaddrinfo(_, _, _, _))
        .WillByDefault(DoAll(SetArgPointee<3>(res), Return(EAI_NONAME)));
    EXPECT_CALL(netv4inf, getaddrinfo(_, _, _, _)).Times(0);
    EXPECT_CALL(netv4inf, freeaddrinfo(_)).Times(0);

    // Test Unit
    char* abs_url = ::resolve_rel_url(nullptr, nullptr);
    EXPECT_EQ(abs_url, nullptr);
    free(abs_url);
}

TEST(UriTestSuite, resolve_rel_url_ip4_arg1_empty_base_url) {
    // empty_base_url_returns_nullptr
    Mock_netv4info netv4inf;
    addrinfo* res = netv4inf.set("0.0.0.0", 0);

    // Mock for network address system call
    umock::Netdb netdb_injectObj(&netv4inf);
    ON_CALL(netv4inf, getaddrinfo(_, _, _, _))
        .WillByDefault(DoAll(SetArgPointee<3>(res), Return(EAI_NONAME)));
    EXPECT_CALL(netv4inf, getaddrinfo(_, _, _, _)).Times(0);
    EXPECT_CALL(netv4inf, freeaddrinfo(_)).Times(0);

    // Provide arguments to execute the unit.
    // No url is absolute, so a nullptr is returned as specified.
    char base_url[]{""};
    char rel_url[]{"homepage#this-fragment"};

    // Test Unit
    char* abs_url = ::resolve_rel_url(*&base_url, *&rel_url);
    EXPECT_EQ(abs_url, nullptr);
    free(abs_url);
}

TEST(UriTestSuite, resolve_rel_url_ip4_arg2_empty_rel_url) {
    // empty_rel_url_returns_base_url
    Mock_netv4info netv4inf;
    addrinfo* res = netv4inf.set("192.168.168.168", 80);

    // Mock for network address system call
    umock::Netdb netdb_injectObj(&netv4inf);
    EXPECT_CALL(netv4inf, getaddrinfo(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<3>(res), Return(0)));
    EXPECT_CALL(netv4inf, freeaddrinfo(_)).Times(1);

    // Provide arguments to execute the unit
    char base_url[]{"http://example.com"};
    char rel_url[]{""};

    // Test Unit
    char* abs_url = ::resolve_rel_url(*&base_url, *&rel_url);
    EXPECT_STREQ(abs_url, "http://example.com");
    free(abs_url);
}

TEST(UriTestSuite, resolve_rel_url_ip4_arg1_arg2_empty_base_and_rel_url) {
    // empty_base_and_rel_url_returns_nullptr
    Mock_netv4info netv4inf;
    addrinfo* res = netv4inf.set("0.0.0.0", 0);

    // Mock for network address system call
    umock::Netdb netdb_injectObj(&netv4inf);
    ON_CALL(netv4inf, getaddrinfo(_, _, _, _))
        .WillByDefault(DoAll(SetArgPointee<3>(res), Return(EAI_NONAME)));
    EXPECT_CALL(netv4inf, getaddrinfo(_, _, _, _)).Times(0);
    EXPECT_CALL(netv4inf, freeaddrinfo(_)).Times(0);

    // Provide arguments to execute the unit
    char base_url[]{""};
    char rel_url[]{""};

    // Test Unit
    char* abs_url = ::resolve_rel_url(*&base_url, *&rel_url);
    EXPECT_EQ(abs_url, nullptr);
    free(abs_url);
}

TEST(UriTestSuite, resolve_rel_url_ip4_arg2_absolute_rel_url) {
    // absolute_rel_url_returns_a_copy_of_it
    Mock_netv4info netv4inf;
    addrinfo* res = netv4inf.set("192.168.168.168", 443);

    // Mock for network address system call
    umock::Netdb netdb_injectObj(&netv4inf);
    EXPECT_CALL(netv4inf, getaddrinfo(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<3>(res), Return(0)));
    EXPECT_CALL(netv4inf, freeaddrinfo(_)).Times(1);

    // Provide arguments to execute the unit
    char base_url[]{"http://example.com"};
    char rel_url[]{"https://absolute.net:443/home-page#fragment"};

    // Test Unit
    char* abs_url = ::resolve_rel_url(*&base_url, *&rel_url);
    EXPECT_STREQ(abs_url, rel_url);
    free(abs_url);
}

TEST(UriTestSuite,
     resolve_rel_url_ip4_arg1_arg2_base_and_rel_url_not_absolute) {
    // base and rel_url not absolute returns nullptr
    Mock_netv4info netv4inf;
    addrinfo* res = netv4inf.set("0.0.0.0", 0);

    // Mock for network address system call
    umock::Netdb netdb_injectObj(&netv4inf);
    ON_CALL(netv4inf, getaddrinfo(_, _, _, _))
        .WillByDefault(DoAll(SetArgPointee<3>(res), Return(EAI_NONAME)));
    EXPECT_CALL(netv4inf, getaddrinfo(_, _, _, _)).Times(0);
    EXPECT_CALL(netv4inf, freeaddrinfo(_)).Times(0);

    // Provide arguments to execute the unit
    char base_url[]{"/example.com"};
    char rel_url[]{"home-page#fragment"};

    // Test Unit
    char* abs_url = ::resolve_rel_url(*&base_url, *&rel_url);
    EXPECT_EQ(abs_url, nullptr);
    free(abs_url);
}

TEST(UriTestSuite, resolve_rel_url_ip4_successful) {
    Mock_netv4info netv4inf;
    addrinfo* res = netv4inf.set("192.168.186.186", 443);

    // Mock for network address system call
    umock::Netdb netdb_injectObj(&netv4inf);
    EXPECT_CALL(netv4inf, getaddrinfo(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<3>(res), Return(0)));
    EXPECT_CALL(netv4inf, freeaddrinfo(_)).Times(1);

    // Provide arguments to execute the unit
    char base_url[]{"https://example.com:443"};
    char rel_url[]{"homepage#this-fragment"};

    // Test Unit
    char* abs_url = ::resolve_rel_url(*&base_url, *&rel_url);
    EXPECT_STREQ(abs_url, "https://example.com:443/homepage#this-fragment");
    free(abs_url);
}

TEST(UriDeathTest, create_url_list_empty) {
    memptr urls_ser{}; // "<url><url>"

    // Test Unit
    if (old_code) {
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": nullptr arg1 (urls_ser) must not segfault.\n";
        EXPECT_DEATH((::create_url_list(&urls_ser, nullptr)),
                     ".*"); // Wrong!
    } else {
        EXPECT_EQ(::create_url_list(&urls_ser, nullptr), 0);
    }

    URL_list url_list;
    memset(&url_list, 0xAA, sizeof(url_list));

    // Test Unit
    if (old_code) {
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": nullptr arg2 (url_list) must not segfault.\n";
#if !defined(__APPLE__) || defined(DEBUG)
        EXPECT_DEATH((::create_url_list(nullptr, &url_list)),
                     ".*"); // Wrong!
#endif
    } else {
        EXPECT_EQ(::create_url_list(nullptr, &url_list), 0);
        EXPECT_EQ(url_list.size, 0);
        EXPECT_EQ(url_list.URLs, nullptr);
        EXPECT_EQ(url_list.parsedURLs, nullptr);
        free_URL_list(&url_list);
    }
    memset(&url_list, 0xAA, sizeof(url_list));

    // Test Unit
    EXPECT_EQ(::create_url_list(&urls_ser, &url_list), 0);
    EXPECT_EQ(url_list.size, 0);
    EXPECT_EQ(url_list.URLs, nullptr);
    EXPECT_EQ(url_list.parsedURLs, nullptr);
    free_URL_list(&url_list);
}

TEST(UriTestSuite, create_url_list_from_loopback_if) {
    URL_list url_list;
    memset(&url_list, 0xAA, sizeof(url_list));

    std::string_view urls{"<https://[::1]>"};
    memptr base_urls; // "<url><url>"
    base_urls.buf = const_cast<char*>(urls.data());
    base_urls.length = urls.size();

    // Test Unit
    ASSERT_EQ(::create_url_list(&base_urls, &url_list), 1);

    EXPECT_EQ(url_list.size, 1);
    EXPECT_STREQ(url_list.URLs, urls.data());
    ASSERT_NE(url_list.parsedURLs, nullptr);
    EXPECT_EQ(url_list.parsedURLs[0].type, Absolute);
    EXPECT_EQ(url_list.parsedURLs[0].path_type, OPAQUE_PART);
    EXPECT_EQ(std::string_view(url_list.parsedURLs[0].scheme.buff,
                               url_list.parsedURLs[0].scheme.size),
              "https");
    EXPECT_EQ(std::string_view(url_list.parsedURLs[0].hostport.text.buff,
                               url_list.parsedURLs[0].hostport.text.size),
              "[::1]");
    // EXPECT_STREQ(url_list.parsedURLs[0].pathquery.buff, ">"); // undefined..
    EXPECT_EQ(url_list.parsedURLs[0].pathquery.size, 0); // ...no pathquery
    EXPECT_EQ(url_list.parsedURLs[0].fragment.buff, nullptr);
    EXPECT_EQ(url_list.parsedURLs[0].fragment.size, 0);
    SSockaddr saObj;
    saObj = url_list.parsedURLs[0].hostport.IPaddress;
#ifndef __APPLE__
    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": The resolved socket address must not have a garbage "
                     "scope id.\n";
        EXPECT_THAT(saObj.netaddrp(), StartsWith("[::1%")); // Wrong!
    } else
#else
    EXPECT_EQ(saObj.netaddrp(), "[::1]:443");
#endif
        free_URL_list(&url_list);
}

TEST(UriTestSuite, create_url_list_from_lla) {
    URL_list url_list;
    memset(&url_list, 0xAA, sizeof(url_list));

    char urls[]{"<https://[fe80::5054]:58138/path/query#fragment>"};
    memptr base_urls; // "<url><url>"
    base_urls.buf = urls;
    base_urls.length = strlen(urls);

    // Test Unit
    EXPECT_EQ(::create_url_list(&base_urls, &url_list), 1);
    // Destroy the input string to detect wrong pointer. It should be coppied to
    // url_list.
    ASSERT_STREQ(base_urls.buf, url_list.URLs);
    memset(urls, 0xAA, sizeof(urls));

    EXPECT_EQ(url_list.size, 1); // Number of URLs in URL list.
    EXPECT_EQ(url_list.parsedURLs[0].type, Absolute);
    EXPECT_EQ(url_list.parsedURLs[0].path_type, ABS_PATH);
    EXPECT_EQ(std::string_view(url_list.parsedURLs[0].scheme.buff,
                               url_list.parsedURLs[0].scheme.size),
              "https");
    EXPECT_THAT(url_list.parsedURLs[0].pathquery.buff,
                StartsWith("/path/query"));
    EXPECT_EQ(url_list.parsedURLs[0].pathquery.size, 11);
    EXPECT_THAT(url_list.parsedURLs[0].fragment.buff, StartsWith("fragment"));
    EXPECT_EQ(url_list.parsedURLs[0].fragment.size, 8);
    EXPECT_THAT(url_list.parsedURLs[0].hostport.text.buff,
                StartsWith("[fe80::5054]:58138"));
    EXPECT_EQ(url_list.parsedURLs[0].hostport.text.size, 18);
    SSockaddr saObj;
    saObj = url_list.parsedURLs[0].hostport.IPaddress;
    if (old_code) {
        std::cout
            << CYEL "[ BUGFIX   ] " CRES << __LINE__
            << ": IPv6 link-local address must not have a garbage scope id.\n";
#ifdef __APPLE__
        EXPECT_EQ(saObj.netaddrp(), ":58138"); // Wrong!
#else
        EXPECT_THAT(saObj.netaddrp(), StartsWith("[fe80::5054%")); // Wrong!
#endif
    } else {
        EXPECT_EQ(saObj.netaddrp(), "[fe80::5054]:58138");
    }
    free_URL_list(&url_list);
}

TEST(UriTestSuite, create_url_list_from_three_ip_addresses) {
    // Create with two valid and one invalid ip address with wrong scheme
    // separator "http//:".
    URL_list url_list;
    memset(&url_list, 0xAA, sizeof(url_list));

    char urls[]{"<https://[::ffff:192.168.1.2]/path/query#fragment><http//"
                ":example.com><http://[2001:db8::e]/path>"};
    memptr base_urls; // "<url><url>"
    base_urls.buf = urls;
    base_urls.length = strlen(urls);

    // Test Unit
    EXPECT_EQ(::create_url_list(&base_urls, &url_list), 2);

    EXPECT_EQ(url_list.size, 2);
    EXPECT_NE(url_list.URLs, nullptr);
    ASSERT_NE(url_list.parsedURLs, nullptr);
    EXPECT_EQ(url_list.parsedURLs[1].type, Absolute);
    EXPECT_THAT(url_list.parsedURLs[1].scheme.buff, StartsWith("http"));
    EXPECT_EQ(url_list.parsedURLs[1].scheme.size, 4);
    EXPECT_EQ(url_list.parsedURLs[1].path_type, ABS_PATH);
    EXPECT_THAT(url_list.parsedURLs[1].pathquery.buff, StartsWith("/path"));
    EXPECT_EQ(url_list.parsedURLs[1].pathquery.size, 5);
    EXPECT_EQ(url_list.parsedURLs[1].fragment.buff, nullptr);
    EXPECT_EQ(url_list.parsedURLs[1].fragment.size, 0);
    EXPECT_THAT(url_list.parsedURLs[1].hostport.text.buff,
                StartsWith("[2001:db8::e]"));
    EXPECT_EQ(url_list.parsedURLs[1].hostport.text.size, 13);
    SSockaddr saObj;
#ifndef __APPLE__
    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": IPv6 addresses must not have garbage scope_ids.\n";
        saObj = url_list.parsedURLs[0].hostport.IPaddress;
        EXPECT_THAT(saObj.netaddrp(),
                    StartsWith("[::ffff:192.168.1.2%"));            // Wrong!
        saObj = url_list.parsedURLs[1].hostport.IPaddress;
        EXPECT_THAT(saObj.netaddrp(), StartsWith("[2001:db8::e%")); // Wrong!
    } else
#endif
    {
        saObj = url_list.parsedURLs[0].hostport.IPaddress;
        EXPECT_EQ(saObj.netaddrp(), "[::ffff:192.168.1.2]:443");
        saObj = url_list.parsedURLs[1].hostport.IPaddress;
        EXPECT_EQ(saObj.netaddrp(), "[2001:db8::e]:80");
    }

    free_URL_list(&url_list);
}

TEST(UriTestSuite, create_url_list_from_dns_name) {
    std::cout << "Triggers DNS lookup with different IP addresses. Has to be "
                 "mocked.\n";
    if (!github_actions && !old_code)
        GTEST_FAIL();
    else
        GTEST_SKIP();

    URL_list url_list;
    memset(&url_list, 0xAA, sizeof(url_list));

    char urls[]{"<https://example.com>"};
    memptr base_urls; // "<url><url>"
    base_urls.buf = urls;
    base_urls.length = strlen(urls);

    // Test Unit
    EXPECT_EQ(::create_url_list(&base_urls, &url_list), 1);

    EXPECT_EQ(url_list.size, 1);
    EXPECT_NE(url_list.URLs, nullptr);
    ASSERT_NE(url_list.parsedURLs, nullptr);
    EXPECT_EQ(url_list.parsedURLs[0].type, Absolute);
    EXPECT_THAT(url_list.parsedURLs[0].scheme.buff, StartsWith("https"));
    EXPECT_EQ(url_list.parsedURLs[0].scheme.size, 5);
    EXPECT_EQ(url_list.parsedURLs[0].path_type, OPAQUE_PART);
    EXPECT_STREQ(url_list.parsedURLs[0].pathquery.buff, ">"); // Delimiter.
    EXPECT_EQ(url_list.parsedURLs[0].pathquery.size, 0);
    EXPECT_STREQ(url_list.parsedURLs[0].fragment.buff, nullptr);
    EXPECT_EQ(url_list.parsedURLs[0].fragment.size, 0);
    EXPECT_THAT(url_list.parsedURLs[0].hostport.text.buff,
                StartsWith("example.com"));
    EXPECT_EQ(url_list.parsedURLs[0].hostport.text.size, 11);
    SSockaddr saObj;
    saObj = url_list.parsedURLs[0].hostport.IPaddress;
    EXPECT_EQ(saObj.netaddrp(), "[2600:1406:5e00:6::17ce:bc1b]:443");

    free_URL_list(&url_list);
}

TEST(UriTestSuite, create_url_list_fails) {
    memptr url_chain{nullptr, 0};
    URL_list url_list;
    memset(&url_list, 0xAA, sizeof(url_list));

    char urls1[]{"<0 = no valid url>"};
    // --------------------------------
    url_chain.buf = urls1;
    url_chain.length = strlen(urls1);

    // Test Unit
    EXPECT_EQ(::create_url_list(&url_chain, &url_list), 0);
    free_URL_list(&url_list);

    char urls2[]{"http://example.com"}; // no '<', '>' delimiter
    // --------------------------------
    url_chain.buf = urls2;
    url_chain.length = strlen(urls2);

    // Test Unit
    EXPECT_EQ(::create_url_list(&url_chain, &url_list), 0);
    free_URL_list(&url_list);

    char urls3[]{
        "<http://example.com:49486><example.com><https://[2001:db8::1]:49487>"};
    // --------------------------------
    url_chain.buf = urls3;
    url_chain.length = strlen(urls3);

    // Test Unit
    ASSERT_EQ(::create_url_list(&url_chain, &url_list), 2);
    EXPECT_EQ(std::string(url_list.parsedURLs[1].hostport.text.buff,
                          url_list.parsedURLs[1].hostport.text.size),
              "[2001:db8::1]:49487");
    free_URL_list(&url_list);
}

TEST(UriTestSuite, create_url_list_with_wrong_url_str_fails) {
    if (old_code) {
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": Unit should not ignore wrong setting of delimiter '<' "
                     "and '>'.\n";
        GTEST_SKIP() << '\x8'; // backspace, remove output empty line
    }

    memptr base_urls; // "<url><url>"
    URL_list url_list;

    char urls1[]{"https://[::1]><http://[2001:db8::1]>"};
    base_urls.buf = urls1;
    base_urls.length = strlen(urls1);

    // Test Unit
    EXPECT_EQ(::create_url_list(&base_urls, &url_list), UPNP_E_INVALID_URL);

    char urls2[]{"<https://[::1]<http://[2001:db8::2]>"};
    base_urls.buf = urls2;
    base_urls.length = strlen(urls2);

    // Test Unit
    EXPECT_EQ(::create_url_list(&base_urls, &url_list), UPNP_E_INVALID_URL);

    char urls3[]{"<https://[::1]>http://[2001:db8::3]>"};
    base_urls.buf = urls3;
    base_urls.length = strlen(urls3);

    // Test Unit
    EXPECT_EQ(::create_url_list(&base_urls, &url_list), UPNP_E_INVALID_URL);

    char urls4[]{"<https://[::1]><http://[2001:db8::4]"};
    base_urls.buf = urls4;
    base_urls.length = strlen(urls4);

    // Test Unit
    EXPECT_EQ(::create_url_list(&base_urls, &url_list), UPNP_E_INVALID_URL);

    char urls5[]{"<https://[::1]><http://[2001:db8::5]>"};
    base_urls.buf = urls5;
    base_urls.length = strlen(urls5);

    // Test Unit
    EXPECT_EQ(::create_url_list(&base_urls, &url_list), 2);
    free_URL_list(&url_list);

    char urls6[]{"<https://[::1]"};
    base_urls.buf = urls6;
    base_urls.length = strlen(urls6);

    // Test Unit
    EXPECT_EQ(::create_url_list(&base_urls, &url_list), UPNP_E_INVALID_URL);

    char urls7[]{"https://[::1]>"};
    base_urls.buf = urls7;
    base_urls.length = strlen(urls7);

    // Test Unit
    EXPECT_EQ(::create_url_list(&base_urls, &url_list), UPNP_E_INVALID_URL);

    char urls8[]{"<>"};
    base_urls.buf = urls8;
    base_urls.length = strlen(urls8);

    // Test Unit
    EXPECT_EQ(::create_url_list(&base_urls, &url_list), 0);
}

TEST(UriTestSuite, copy_url_list_successful) {
    // Provide needed values.
    char urls[]{"<https://[2001:db8::ac15]:58140/path/query#fragment>"};
    memptr base_urls; // "<url><url>"
    base_urls.buf = urls;
    base_urls.length = strlen(urls);

    URL_list src_urlist{}, dst_urlist{};
    EXPECT_EQ(::create_url_list(&base_urls, &src_urlist), 1);
    ASSERT_EQ(src_urlist.size, 1);
    memset(urls, 0xAA, sizeof(urls));
    EXPECT_THAT(src_urlist.parsedURLs[0].scheme.buff, StartsWith("https"));

    if (old_code)
        // For details look for symbol UPNPLIB_PUPNP_BUG at the source code in
        // Pupnp/upnp/src/genlib/net/uri/uri.cpp: copy_url_list().
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Possible leak with allocating memory for base_url and "
                     "url_list.\n";
    // Test Unit
    EXPECT_EQ(::copy_URL_list(&src_urlist, &dst_urlist), HTTP_SUCCESS);
    // Destroy source list to ensure there is a real copy with all its dynamic
    // allocations.
    memset(src_urlist.URLs, 0xAA, strlen(src_urlist.URLs + 1));
    memset(src_urlist.parsedURLs, 0xAA, sizeof(uri_type) * src_urlist.size);
    // I leak the allocated source list because I cannot be sure that freeing
    // it succeeds on a destroyed list. Leaking is only for the short time the
    // test executable runs.
    // free_URL_list(&src_urlist);

    EXPECT_EQ(dst_urlist.size, 1);
    // EXPECT_STREQ(urls, dst_urlist.URLs);
    EXPECT_EQ(dst_urlist.parsedURLs[0].type, Absolute);
    EXPECT_THAT(dst_urlist.parsedURLs[0].scheme.buff, StartsWith("https"));
    EXPECT_EQ(dst_urlist.parsedURLs[0].scheme.size, 5);
    EXPECT_EQ(dst_urlist.parsedURLs[0].path_type, ABS_PATH);
    EXPECT_THAT(dst_urlist.parsedURLs[0].pathquery.buff,
                StartsWith("/path/query"));
    EXPECT_EQ(dst_urlist.parsedURLs[0].pathquery.size, 11);
    EXPECT_THAT(dst_urlist.parsedURLs[0].fragment.buff, StartsWith("fragment"));
    EXPECT_EQ(dst_urlist.parsedURLs[0].fragment.size, 8);
    EXPECT_THAT(dst_urlist.parsedURLs[0].hostport.text.buff,
                StartsWith("[2001:db8::ac15]:58140"));
    EXPECT_EQ(dst_urlist.parsedURLs[0].hostport.text.size, 22);
    SSockaddr saObj;
    saObj = dst_urlist.parsedURLs[0].hostport.IPaddress;
#ifndef __APPLE__
    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": The resolved socket address must not have a garbage "
                     "scope id.\n";
        EXPECT_THAT(saObj.netaddrp(), StartsWith("[2001:db8::ac15%")); // Wrong!
    } else
#endif
        EXPECT_EQ(saObj.netaddrp(), "[2001:db8::ac15]:58140");

    free_URL_list(&dst_urlist);
}

} // namespace utest

int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // Managed in gtest_main.inc
}
