// Copyright (C) 2026+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// redistribution only with this copyright remark. last modified: 2026-03-15

// This Unit Tests are used to verify pUPnP software with new compatible code.
// These tests compile with pUPnP code and with compatible code. Unit Tests for
// code that only exists in pUPnP are available at Utest/pupnp/*.

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

#include <UPnPsdk/sockaddr.hpp>
#include <UPnPsdk/socket.hpp>
#include <umock/netdb_mock.hpp>
#include <utest/utest.hpp>


using ::testing::_;
using ::testing::DoAll;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetErrnoAndReturn;
using ::testing::StartsWith;
using ::testing::StrictMock;

using ::UPnPsdk::SSockaddr;


namespace utest {

// token_cmp() functions: tests from the uri module
// ================================================
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
            { ::token_string_casecmp(&inull, instr0); }, ".*"); // Wrong!
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


// resolve_rel_url() function: tests from the uri module
// =====================================================

TEST(UriTestSuite, resolve_rel_url_successful) {
    SSockaddr saObj;
    saObj = "[::ffff:192.168.186.186]:443";

    ::addrinfo res{};
    res.ai_family = saObj.ss.ss_family;
    res.ai_socktype = SOCK_STREAM;
    res.ai_addrlen = saObj.sizeof_saddr();
    res.ai_addr = &saObj.sa;

    // Instantiate mocking object.
    StrictMock<umock::NetdbMock> netdbObj;
    // Set default object values
    ON_CALL(netdbObj, getaddrinfo(_, _, _, _))
        .WillByDefault(SetErrnoAndReturn(EACCESP, EAI_NONAME));
    // Inject the mocking object into the tested code.
    umock::Netdb netdb_injectObj = umock::Netdb(&netdbObj);

    // Mock for network address system call
    EXPECT_CALL(netdbObj, getaddrinfo(Pointee(*"example.com"), _, _, _))
        .WillOnce(DoAll(SetArgPointee<3>(&res), Return(0)));
    EXPECT_CALL(netdbObj, freeaddrinfo(_)).Times(1);

    // Provide arguments to execute the unit
    char base_url[]{"https://example.com:443"};
    char rel_url[]{"homepage#this-fragment"};

    // Test Unit
    char* abs_url = ::resolve_rel_url(*&base_url, *&rel_url);
    EXPECT_STREQ(abs_url, old_code
                              ? "https://example.com:443/homepage#this-fragment"
                              : "https://example.com/homepage#this-fragment");

    free(abs_url);
}

TEST(UriTestSuite, resolve_rel_url_arg1_nullptr_arg2_rel_url) {
    // If the base_url is a nullptr, then a copy of the rel_url is passed back.

    // Instantiate mocking object.
    StrictMock<umock::NetdbMock> netdbObj;
    // Set default object values
    ON_CALL(netdbObj, getaddrinfo(_, _, _, _))
        .WillByDefault(SetErrnoAndReturn(EACCESP, EAI_NONAME));

    // Inject the mocking object into the tested code.
    umock::Netdb netdb_injectObj = umock::Netdb(&netdbObj);

    // Mock for network address system call
    EXPECT_CALL(netdbObj, getaddrinfo(_, _, _, _)).Times(0);
    EXPECT_CALL(netdbObj, freeaddrinfo(_)).Times(0);

    // Provide arguments to execute the unit
    char rel_url[]{"homepage#this-fragment"};

    // Test Unit
    char* abs_url = ::resolve_rel_url(nullptr, rel_url);
    EXPECT_STREQ(abs_url, "homepage#this-fragment");

    free(abs_url);
}

TEST(UriDeathTest, resolve_rel_url_arg1_base_url_arg2_nullptr) {
    // nullptr_rel_url_returns_base_url.

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

        SSockaddr saObj;
        saObj = "[2001:db8::9474]";

        ::addrinfo res{};
        res.ai_family = saObj.ss.ss_family;
        res.ai_socktype = SOCK_STREAM;
        res.ai_addrlen = saObj.sizeof_saddr();
        res.ai_addr = &saObj.sa;

        // Instantiate mocking object.
        StrictMock<umock::NetdbMock> netdbObj;
        // Set default object values
        ON_CALL(netdbObj, getaddrinfo(_, _, _, _))
            .WillByDefault(SetErrnoAndReturn(EACCESP, EAI_NONAME));
        // Inject the mocking object into the tested code.
        umock::Netdb netdb_injectObj = umock::Netdb(&netdbObj);

        // Mock for network address system call
        EXPECT_CALL(netdbObj, getaddrinfo(_, _, _, _))
            .WillOnce(DoAll(SetArgPointee<3>(&res), Return(0)));
        EXPECT_CALL(netdbObj, freeaddrinfo(_)).Times(1);

        // Test Unit
        char* abs_url = ::resolve_rel_url(base_url, nullptr);

        // The URI "http://example.com/" is the normal form for the "http"
        // scheme, as specified by RFC3986 6.2.3.
        EXPECT_STREQ(abs_url, "http://example.com/");

        free(abs_url);
    }
}

TEST(UriTestSuite, resolve_rel_url_arg1_nullptr_arg2_nullptr) {
    // Instantiate mocking object.
    StrictMock<umock::NetdbMock> netdbObj;
    // Set default object values
    ON_CALL(netdbObj, getaddrinfo(_, _, _, _))
        .WillByDefault(SetErrnoAndReturn(EACCESP, EAI_NONAME));
    // Inject the mocking object into the tested code.
    umock::Netdb netdb_injectObj = umock::Netdb(&netdbObj);

    // Mock for network address system call
    EXPECT_CALL(netdbObj, getaddrinfo(_, _, _, _)).Times(0);
    EXPECT_CALL(netdbObj, freeaddrinfo(_)).Times(0);

    // Test Unit
    char* abs_url = ::resolve_rel_url(nullptr, nullptr);
    EXPECT_EQ(abs_url, nullptr);

    free(abs_url);
}

TEST(UriTestSuite, resolve_rel_url_arg1_empty_arg2_rel_url) {
    // Instantiate mocking object.
    StrictMock<umock::NetdbMock> netdbObj;
    // Set default object values
    ON_CALL(netdbObj, getaddrinfo(_, _, _, _))
        .WillByDefault(SetErrnoAndReturn(EACCESP, EAI_NONAME));

    // Inject the mocking object into the tested code.
    umock::Netdb netdb_injectObj = umock::Netdb(&netdbObj);

    // Mock for network address system call
    EXPECT_CALL(netdbObj, getaddrinfo(_, _, _, _)).Times(0);
    EXPECT_CALL(netdbObj, freeaddrinfo(_)).Times(0);

    // Provide arguments to execute the unit.
    // No url is absolute, so a nullptr is returned as specified.
    char base_url[]{""};
    char rel_url[]{"homepage#this-fragment"};

    // Test Unit
    char* abs_url = ::resolve_rel_url(base_url, rel_url);
    EXPECT_EQ(abs_url, nullptr);

    free(abs_url);
}

TEST(UriTestSuite, resolve_rel_url_arg1_abs_url_arg2_empty) {
    // If the rel_url is empty (""), then a copy of the base_url is passed back.

    SSockaddr saObj;
    saObj = "[::ffff:192.168.168.168]:8080";

    ::addrinfo res{};
    res.ai_family = saObj.ss.ss_family;
    res.ai_socktype = SOCK_STREAM;
    res.ai_addrlen = saObj.sizeof_saddr();
    res.ai_addr = &saObj.sa;

    // Instantiate mocking object.
    StrictMock<umock::NetdbMock> netdbObj;
    // Set default object values
    ON_CALL(netdbObj, getaddrinfo(_, _, _, _))
        .WillByDefault(SetErrnoAndReturn(EACCESP, EAI_NONAME));

    // Inject the mocking object into the tested code.
    umock::Netdb netdb_injectObj = umock::Netdb(&netdbObj);

    EXPECT_CALL(netdbObj, getaddrinfo(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<3>(&res), Return(0)));
    EXPECT_CALL(netdbObj, freeaddrinfo(_)).Times(1);

    // Provide arguments to execute the unit
    char base_url[]{"http://example.com"};
    char rel_url[]{""};

    // Test Unit
    char* abs_url = ::resolve_rel_url(base_url, rel_url);

    // The URI "http://example.com/" is the normal form for the "http"
    // scheme, as specified by RFC3986 6.2.3.
    EXPECT_STREQ(abs_url, old_code ? base_url : "http://example.com/");

    free(abs_url);
}

TEST(UriTestSuite, resolve_rel_url_arg1_empty_arg2_empty) {
    // Instantiate mocking object.
    StrictMock<umock::NetdbMock> netdbObj;
    // Set default object values
    ON_CALL(netdbObj, getaddrinfo(_, _, _, _))
        .WillByDefault(SetErrnoAndReturn(EACCESP, EAI_NONAME));
    // Inject the mocking object into the tested code.
    umock::Netdb netdb_injectObj = umock::Netdb(&netdbObj);

    // Mock for network address system call
    EXPECT_CALL(netdbObj, getaddrinfo(_, _, _, _)).Times(0);
    EXPECT_CALL(netdbObj, freeaddrinfo(_)).Times(0);

    // Provide arguments to execute the unit
    char base_url[]{""};
    char rel_url[]{""};

    // Test Unit
    char* abs_url = ::resolve_rel_url(base_url, rel_url);
    EXPECT_EQ(abs_url, nullptr);

    free(abs_url);
}

TEST(UriTestSuite, resolve_rel_url_arg1_abs_url_arg2_abs_url) {
    // If the rel_url is absolute (with a valid base_url), then a copy of the
    // rel_url is passed back.

    SSockaddr saObj;
    saObj = "[fe80:db8::8%1]:50001";

    ::addrinfo res{};
    res.ai_family = saObj.ss.ss_family;
    res.ai_socktype = SOCK_STREAM;
    res.ai_addrlen = saObj.sizeof_saddr();
    res.ai_addr = &saObj.sa;

    // Instantiate mocking object.
    StrictMock<umock::NetdbMock> netdbObj;
    // Set default object values
    ON_CALL(netdbObj, getaddrinfo(_, _, _, _))
        .WillByDefault(SetErrnoAndReturn(EACCESP, EAI_NONAME));
    // Inject the mocking object into the tested code.
    umock::Netdb netdb_injectObj = umock::Netdb(&netdbObj);

    // Mock for network address system call
    EXPECT_CALL(netdbObj, getaddrinfo(Pointee(*"absolute.net"), _, _, _))
        .WillOnce(DoAll(SetArgPointee<3>(&res), Return(0)));
    EXPECT_CALL(netdbObj, freeaddrinfo(_)).Times(1);

    // Provide arguments to execute the unit
    char base_url[]{"http://example.com"};
    char rel_url[]{"https://absolute.net:443/home-page#fragment"};

    // Test Unit
    // absolute arg2 rel_url_returns_a_copy_of_it
    char* abs_url = ::resolve_rel_url(base_url, rel_url);
    EXPECT_STREQ(abs_url, old_code ? rel_url
                                   : "https://absolute.net/home-page#fragment");

    free(abs_url);
}

TEST(UriTestSuite, resolve_rel_url_arg1_rel_url_arg2_rel_url) {
    // Instantiate mocking object.
    StrictMock<umock::NetdbMock> netdbObj;
    // Set default object values
    ON_CALL(netdbObj, getaddrinfo(_, _, _, _))
        .WillByDefault(SetErrnoAndReturn(EACCESP, EAI_NONAME));
    // Inject the mocking object into the tested code.
    umock::Netdb netdb_injectObj = umock::Netdb(&netdbObj);

    // Mock for network address system call
    EXPECT_CALL(netdbObj, getaddrinfo(_, _, _, _)).Times(0);
    EXPECT_CALL(netdbObj, freeaddrinfo(_)).Times(0);

    // Provide arguments to execute the unit
    char base_url[]{"/example.com"};
    char rel_url[]{"home-page#fragment"};

    // Test Unit
    // base and rel_url not absolute returns nullptr
    char* abs_url = ::resolve_rel_url(base_url, rel_url);
    EXPECT_EQ(abs_url, nullptr);

    free(abs_url);
}


// create_url_list() function: tests from the uri module
// =====================================================
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
    memset(&url_list, 0xAA, sizeof(url_list)); // Fill with garbage

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
    memset(&url_list, 0xAA, sizeof(url_list)); // Fill with garbage

    // Test Unit
    EXPECT_EQ(::create_url_list(&urls_ser, &url_list), 0);
    EXPECT_EQ(url_list.size, 0);
    EXPECT_EQ(url_list.URLs, nullptr);
    EXPECT_EQ(url_list.parsedURLs, nullptr);
    free_URL_list(&url_list);
}

TEST(UriTestSuite, create_url_list_with_empty_url) {
    // To be compatible with pUPnP this results in a URL list with 0 URLs.
    URL_list url_list;
    memset(&url_list, 0xAA, sizeof(url_list)); // Fill with garbage

    std::string_view urls{"<>"};
    memptr base_urls; // "<url><url>"
    base_urls.buf = const_cast<char*>(urls.data());
    base_urls.length = urls.size();

    // Test Unit
    ASSERT_EQ(::create_url_list(&base_urls, &url_list), 0);

    EXPECT_EQ(url_list.size, 0);
    EXPECT_EQ(url_list.URLs, nullptr);
    EXPECT_EQ(url_list.parsedURLs, nullptr);
}

TEST(UriTestSuite, create_url_list_from_loopback_if) {
    URL_list url_list;
    memset(&url_list, 0xAA, sizeof(url_list)); // Fill with garbage

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
    ASSERT_EQ(::create_url_list(&base_urls, &url_list), 1);
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

    char urls[]{"<https://[::ffff:192.168.1.2]/path?query#fragment><http//"
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
    URL_list url_list;
    memset(&url_list, 0xAA, sizeof(url_list));

    char urls[]{"<https://localhost>"};
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
    // EXPECT_STREQ(url_list.parsedURLs[0].pathquery.buff, ">"); // Undefined.
    EXPECT_EQ(url_list.parsedURLs[0].pathquery.size, 0);
    EXPECT_STREQ(url_list.parsedURLs[0].fragment.buff, nullptr);
    EXPECT_EQ(url_list.parsedURLs[0].fragment.size, 0);
    EXPECT_THAT(url_list.parsedURLs[0].hostport.text.buff,
                StartsWith("localhost"));
    EXPECT_EQ(url_list.parsedURLs[0].hostport.text.size, 9);
    SSockaddr saObj;
    saObj = url_list.parsedURLs[0].hostport.IPaddress;
    EXPECT_EQ(saObj.netaddrp(), "[::1]:443");

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
    base_urls.length = sizeof(urls1) - 1;

    // Test Unit
    EXPECT_EQ(::create_url_list(&base_urls, &url_list), UPNP_E_INVALID_URL);

    char urls2[]{"<https://[::1]<http://[2001:db8::2]>"};
    base_urls.buf = urls2;
    base_urls.length = sizeof(urls2) - 1;

    // Test Unit
    EXPECT_EQ(::create_url_list(&base_urls, &url_list), UPNP_E_INVALID_URL);

    char urls3[]{"<https://[::1]>http://[2001:db8::3]>"};
    base_urls.buf = urls3;
    base_urls.length = sizeof(urls3) - 1;

    // Test Unit
    EXPECT_EQ(::create_url_list(&base_urls, &url_list), UPNP_E_INVALID_URL);

    char urls4[]{"<https://[::1]><http://[2001:db8::4]"};
    base_urls.buf = urls4;
    base_urls.length = sizeof(urls4) - 1;

    // Test Unit
    EXPECT_EQ(::create_url_list(&base_urls, &url_list), UPNP_E_INVALID_URL);

    char urls5[]{"<https://[::1]><http://[2001:db8::5]>"};
    base_urls.buf = urls5;
    base_urls.length = sizeof(urls5) - 1;

    // Test Unit
    EXPECT_EQ(::create_url_list(&base_urls, &url_list), 2);
    free_URL_list(&url_list);

    char urls6[]{"<https://[::1]"};
    base_urls.buf = urls6;
    base_urls.length = sizeof(urls6) - 1;

    // Test Unit
    EXPECT_EQ(::create_url_list(&base_urls, &url_list), UPNP_E_INVALID_URL);

    char urls7[]{"https://[::1]>"};
    base_urls.buf = urls7;
    base_urls.length = sizeof(urls7) - 1;

    // Test Unit
    EXPECT_EQ(::create_url_list(&base_urls, &url_list), UPNP_E_INVALID_URL);

    char urls8[]{"<>"};
    base_urls.buf = urls8;
    base_urls.length = sizeof(urls8) - 1;

    // Test Unit
    EXPECT_EQ(::create_url_list(&base_urls, &url_list), 0);
}

TEST(UriTestSuite, copy_url_list_successful) {
    // Provide needed values.
    char urls[]{"<https://[2001:db8::ac15]:58140/path/query#fragment>"};
    memptr base_urls; // "<url><url>"
    base_urls.buf = urls;
    base_urls.length = sizeof(urls) - 1;

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
