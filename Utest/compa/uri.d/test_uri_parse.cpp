// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-10-31

// Include source code for testing. So we have also direct access to static
// functions which need to be tested.
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
#include <Pupnp/upnp/src/genlib/net/uri/uri.cpp>
#else
#include <Compa/src/genlib/net/uri/uri.cpp>
#endif

#include <UPnPsdk/upnptools.hpp>
#include <UPnPsdk/sockaddr.hpp>
#include <utest/utest.hpp>

using ::testing::StartsWith;

using ::UPnPsdk::errStrEx;
using ::UPnPsdk::SSockaddr;

namespace utest {

// parse_uri() function: tests from the uri module
// ===============================================
#if 0
TEST(ParseUriIp4TestSuite, simple_call) {
    // This test is only for humans to get an idea what's going on. If you need
    // it, set '#if true' only temporary. It is not intended to be permanent
    // part of the tests. It doesn't really test things and because unmocked, it
    // queries DNS server on the internet that may have long delays.

    const char* uri_str{"scheme://example-site.de:80/uripath?uriquery#urifragment"};
    // const char* uri_str{"mailto:a@b.com"};
    uri_type out;

    // Test Unit
    EXPECT_EQ(::parse_uri(uri_str, 64, &out), HTTP_SUCCESS);
    ::std::cout << "DEBUG: out.scheme.buff = " << out.scheme.buff
                << ::std::endl;
    ::std::cout << "DEBUG: out.type Absolute(0), Relative(1) = " << out.type
                << ::std::endl;
    ::std::cout
        << "DEBUG: out.path_type ABS_PATH(0), REL_PATH(1), OPAQUE_PART(2) = "
        << out.path_type << ::std::endl;
    ::std::cout << "DEBUG: out.hostport.text.buff = " << out.hostport.text.buff
                << ::std::endl;
    ::std::cout << "DEBUG: out.pathquery.buff = " << out.pathquery.buff
                << ::std::endl;
    ::std::cout << "DEBUG: out.fragment.buff = " << out.fragment.buff
                << ::std::endl;
    ::std::cout << "DEBUG: out.fragment.size = " << (signed)out.fragment.size
                << ::std::endl;
}
#endif

TEST(ParseUriTestSuite, loopback_uri) {
    uri_type out;
    memset(&out, 0xAA, sizeof(out));

    std::string_view url_str{"https://[::1]/uri/path?uriquery#urifragment"};

    // Test Unit
    int returned{UPNP_E_INTERNAL_ERROR};
    EXPECT_EQ(returned = ::parse_uri(url_str.data(), url_str.size(), &out),
              HTTP_SUCCESS)
        << errStrEx(returned, HTTP_SUCCESS);

    // Check the uri-parts scheme, hostport, pathquery and fragment. Please
    // note that the last part of the buffer content is garbage. The valid
    // character chain is determined by its size. But it's no problem to
    // compare the whole buffer because it's defined to contain a C string. But
    // with std::string_view() I have an exact view to the components.
    EXPECT_EQ(out.type, Absolute);
    EXPECT_EQ(out.path_type, ABS_PATH);
    EXPECT_EQ(std::string_view(out.scheme.buff, out.scheme.size), "https");
    EXPECT_EQ(std::string_view(out.hostport.text.buff, out.hostport.text.size),
              "[::1]");
    EXPECT_EQ(std::string_view(out.pathquery.buff, out.pathquery.size),
              "/uri/path?uriquery");
    EXPECT_EQ(std::string_view(out.fragment.buff, out.fragment.size),
              "urifragment");
    SSockaddr saObj;
    saObj = out.hostport.IPaddress;
    EXPECT_EQ(saObj.ss.ss_family, AF_INET6);
    EXPECT_EQ(saObj.port(), 443);
#ifndef __APPLE__
    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": The resolved socket address must not have a garbage "
                     "scope id.\n";
        EXPECT_THAT(saObj.netaddrp(), StartsWith("[::1%")); // Wrong!
    } else
#endif
        EXPECT_EQ(saObj.netaddrp(), "[::1]:443");
}

TEST(ParseUriTestSuite, absolute_uri_successful) {
    uri_type out;
    memset(&out, 0xAA, sizeof(out));

    char url_str[]{
        "https://[::ffff:192.168.234.132]:443/uri/path?uriquery#urifragment"};

    // Test Unit
    int returned{UPNP_E_INTERNAL_ERROR};
    EXPECT_EQ(returned = ::parse_uri(url_str, strlen(url_str), &out),
              HTTP_SUCCESS)
        << errStrEx(returned, HTTP_SUCCESS);

    // Check the uri-parts scheme, hostport, pathquery and fragment. Please note
    // that the last part of the buffer content is garbage. The valid character
    // chain is determined by its size. But it's no problem to compare the whole
    // buffer because it's defined to contain a C string.
    // The token.buff pointer just only point into the original url_str:
    url_str[37] = 'X'; // offset is zero based.
    EXPECT_STREQ(
        out.scheme.buff,
        "https://[::ffff:192.168.234.132]:443/Xri/path?uriquery#urifragment");
    //                                        ^ there is an 'X' now
    // but out.scheme.buff is delimted by its size
    EXPECT_EQ(std::string_view(out.scheme.buff, out.scheme.size), "https");

    EXPECT_EQ(std::string_view(out.hostport.text.buff, out.hostport.text.size),
              "[::ffff:192.168.234.132]:443");
    EXPECT_EQ(std::string_view(out.pathquery.buff, out.pathquery.size),
              "/Xri/path?uriquery");
    EXPECT_EQ(std::string_view(out.fragment.buff, out.fragment.size),
              "urifragment");
    EXPECT_EQ(out.type, Absolute);
    EXPECT_EQ(out.path_type, ABS_PATH);
    SSockaddr saObj;
    saObj = out.hostport.IPaddress;
    EXPECT_EQ(saObj.port(), 443);
    EXPECT_EQ(saObj.ss.ss_family, AF_INET6);
#ifndef __APPLE__
    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": The resolved socket address must not have a garbage "
                     "scope id.\n";
        EXPECT_THAT(saObj.netaddrp(),
                    StartsWith("[::ffff:192.168.234.132%")); // Wrong!
    } else
#endif
        EXPECT_EQ(saObj.netaddrp(), "[::ffff:192.168.234.132]:443");
}

TEST(ParseUriTestSuite, absolute_uri_with_shorter_max_size) {
    uri_type out;
    memset(&out, 0xAA, sizeof(out));

    constexpr char url_str[65]{
        "https://[::ffff:192.168.88.77]:443/uri/path?uriquery#urifragment"};
    // This is by 21 chars too short for the whole url (without '\0'). It will
    // split pathquery.
    constexpr size_t max_size = 65 - 1 - 21;

    // Test Unit
    int returned{UPNP_E_INTERNAL_ERROR};
    EXPECT_EQ(returned = ::parse_uri(url_str, max_size, &out), HTTP_SUCCESS)
        << errStrEx(returned, HTTP_SUCCESS);

    // Check the uri-parts scheme, hostport, pathquery and fragment. Please note
    // that the last part of the buffer content is garbage. The valid character
    // chain is determined by its size. But it's no problem to compare the whole
    // buffer because it's defined to contain a C string.
    EXPECT_EQ(std::string_view(out.scheme.buff, out.scheme.size), "https");
    EXPECT_EQ(std::string_view(out.hostport.text.buff, out.hostport.text.size),
              "[::ffff:192.168.88.77]:443");
    // Here we see that query is stripped from path due to shorten input string.
    EXPECT_EQ(std::string_view(out.pathquery.buff, out.pathquery.size),
              "/uri/path"); // Would be regular "/uri/path?uriquery"
    EXPECT_STREQ(out.fragment.buff, nullptr);
    EXPECT_EQ(out.fragment.size, 0u);
    EXPECT_EQ(out.type, Absolute);
    EXPECT_EQ(out.path_type, ABS_PATH);
    SSockaddr saObj;
    saObj = out.hostport.IPaddress;
    EXPECT_EQ(saObj.port(), 443);
    EXPECT_EQ(saObj.ss.ss_family, AF_INET6);
#ifndef __APPLE__
    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": The resolved socket address must not have a garbage "
                     "scope id.\n";
        EXPECT_THAT(saObj.netaddrp(),
                    StartsWith("[::ffff:192.168.88.77%")); // Wrong!
    } else
#endif
        EXPECT_EQ(saObj.netaddrp(), "[::ffff:192.168.88.77]:443");
}

TEST(ParseUriTestSuite, sized_url_string_not_null_terminated) {
    // Max size is the strlen without '\0' terminator as specified with the
    // documentation.
    uri_type out;
    memset(&out, 0xAA, sizeof(out));

    // string_view does not terminate its data with '\0'
    std::string_view url_str{
        "https://[2001:db8::b050]:443/uri/path?uriquery#urifragment"};

    // Test Unit
    int returned{UPNP_E_INTERNAL_ERROR};
    EXPECT_EQ(returned = ::parse_uri(url_str.data(), url_str.size(), &out),
              HTTP_SUCCESS)
        << errStrEx(returned, HTTP_SUCCESS);
}

TEST(ParseUriTestSuite, uri_with_invalid_netaddress) {
    // This test will not segfault with a null initialized 'url' structure for
    // the output of 'parse_uri()'.
    constexpr char url_str[] = "http://[z80::28]:80/path/?key=value#fragment";
    uri_type url;
    memset(&url, 0xAA, sizeof(uri_type));

    // Test Unit
    // Get a uri structure with parse_uri(). It fails with NONAME to get a valid
    // host & port.
    int returned{UPNP_E_INTERNAL_ERROR};
    EXPECT_EQ(returned = ::parse_uri(url_str, strlen(url_str), &url),
              UPNP_E_INVALID_URL)
        << errStrEx(returned, UPNP_E_INVALID_URL);

    // Some components of the url are valid.
    EXPECT_EQ(url.type, Absolute);
    EXPECT_EQ(url.path_type, OPAQUE_PART);
    EXPECT_EQ(std::string_view(url.scheme.buff, url.scheme.size), "http");

    // This indicates the error...
    EXPECT_EQ(url.hostport.text.buff, nullptr);
    EXPECT_EQ(url.hostport.text.size, 0u);
#if 0 // Undefined behavior, does not match. Will mostly find untouched 0xAA.
    // ...and also following components.
    EXPECT_EQ(url.pathquery.buff, nullptr);
    EXPECT_EQ(url.pathquery.size, 0u);
    EXPECT_EQ(url.fragment.buff, nullptr);
    EXPECT_EQ(url.fragment.size, 0u);
    SSockaddr saObj;
    saObj = url.hostport.IPaddress;
    EXPECT_EQ(saObj.ss.ss_family, AF_UNSPEC);
    EXPECT_EQ(saObj.netaddrp(), ":0");
#endif
}

TEST(ParseUriTestSuite, ip_address_without_pathquery) {
    uri_type out;
    memset(&out, 0xAA, sizeof(out));

    // Test Unit
    constexpr char url_str[]{"http://[fe80::7df]#urifragment"};

    int returned{UPNP_E_INTERNAL_ERROR};
    EXPECT_EQ(returned = ::parse_uri(url_str, strlen(url_str), &out),
              HTTP_SUCCESS)
        << errStrEx(returned, HTTP_SUCCESS);

    EXPECT_EQ(out.type, Absolute);
    EXPECT_EQ(out.path_type, OPAQUE_PART);
    EXPECT_EQ(std::string_view(out.scheme.buff, out.scheme.size), "http");
    EXPECT_EQ(std::string_view(out.hostport.text.buff, out.hostport.text.size),
              "[fe80::7df]");
    EXPECT_STREQ(out.pathquery.buff, "#urifragment");
    EXPECT_EQ(out.pathquery.size, 0u);
    EXPECT_EQ(std::string_view(out.fragment.buff, out.fragment.size),
              "urifragment");
    SSockaddr saObj;
    saObj = out.hostport.IPaddress;
    EXPECT_EQ(saObj.port(), 80);
    EXPECT_EQ(saObj.ss.ss_family, AF_INET6);
#ifndef __APPLE__
    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": The resolved socket address must not have a garbage "
                     "scope id.\n";
        EXPECT_THAT(saObj.netaddrp(), StartsWith("[fe80::7df%")); // Wrong!
    } else
        // lla not from a local netadapter does not has a scope id.
        EXPECT_EQ(saObj.netaddrp(), "[fe80::7df]:80");
#endif
}

TEST(ParseUriTestSuite, ip_address_without_fragment) {
    uri_type out;
    memset(&out, 0xAA, sizeof(out));

    constexpr char url_str[] =
        "http://[::ffff:192.168.167.166]/path/?key=value";

    // Test Unit
    int returned{UPNP_E_INTERNAL_ERROR};
    EXPECT_EQ(returned = ::parse_uri(url_str, strlen(url_str), &out),
              HTTP_SUCCESS)
        << errStrEx(returned, HTTP_SUCCESS);

    EXPECT_EQ(out.type, Absolute);
    EXPECT_EQ(out.path_type, ABS_PATH);
    EXPECT_EQ(std::string_view(out.scheme.buff, out.scheme.size), "http");
    EXPECT_EQ(std::string_view(out.hostport.text.buff, out.hostport.text.size),
              "[::ffff:192.168.167.166]");
    EXPECT_EQ(std::string_view(out.pathquery.buff, out.pathquery.size),
              "/path/?key=value");
    EXPECT_STREQ(out.fragment.buff, nullptr);
    EXPECT_EQ(out.fragment.size, 0u);
    SSockaddr saObj;
    saObj = out.hostport.IPaddress;
    EXPECT_EQ(saObj.port(), 80);
    EXPECT_EQ(saObj.ss.ss_family, AF_INET6);
#ifndef __APPLE__
    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": The resolved socket address must not have a garbage "
                     "scope id.\n";
        EXPECT_THAT(saObj.netaddrp(),
                    StartsWith("[::ffff:192.168.167.166%")); // Wrong!
    } else
#endif
        EXPECT_EQ(saObj.netaddrp(), "[::ffff:192.168.167.166]:80");
}

TEST(ParseUriTestSuite, parse_scheme_of_uri) {
    ::token out;

    EXPECT_EQ(::parse_scheme("https://dummy.net:80/page", 6, &out), 5u);
    EXPECT_EQ(out.size, 5u);
    EXPECT_STREQ(out.buff, "https://dummy.net:80/page");
    EXPECT_EQ(::parse_scheme("https://dummy.net:80/page", 5, &out), 0u);
    EXPECT_EQ(out.size, 0u);
    EXPECT_STREQ(out.buff, nullptr);
    EXPECT_EQ(::parse_scheme("h:tps://dummy.net:80/page", 32, &out), 1u);
    EXPECT_EQ(out.size, 1u);
    EXPECT_STREQ(out.buff, "h:tps://dummy.net:80/page");
    EXPECT_EQ(::parse_scheme("1ttps://dummy.net:80/page", 32, &out), 0u);
    EXPECT_EQ(out.size, 0u);
    EXPECT_STREQ(out.buff, nullptr);
    EXPECT_EQ(::parse_scheme("h§tps://dummy.net:80/page", 32, &out), 0u);
    EXPECT_EQ(out.size, 0u);
    EXPECT_STREQ(out.buff, nullptr);
    EXPECT_EQ(::parse_scheme(":ttps://dummy.net:80/page", 32, &out), 0u);
    EXPECT_EQ(out.size, 0u);
    EXPECT_STREQ(out.buff, nullptr);
    EXPECT_EQ(::parse_scheme("h*tps://dummy.net:80/page", 32, &out), 0u);
    EXPECT_EQ(out.size, 0u);
    EXPECT_STREQ(out.buff, nullptr);
    EXPECT_EQ(::parse_scheme("mailto:a@b.com", 7, &out), 6u);
    EXPECT_EQ(out.size, 6u);
    EXPECT_STREQ(out.buff, "mailto:a@b.com");
    EXPECT_EQ(::parse_scheme("mailto:a@b.com", 6, &out), 0u);
    EXPECT_EQ(out.size, 0u);
    EXPECT_STREQ(out.buff, nullptr);
}

TEST(ParseUriTestSuite, relative_uri_with_authority_and_absolute_path) {
    uri_type out;
    memset(&out, 0xAA, sizeof(out));

    constexpr char url_str[]{
        "//[2001:db8::40ec]:80/uri/path?uriquery#urifragment"};

    // Test Unit
    EXPECT_EQ(::parse_uri(url_str, strlen(url_str), &out), HTTP_SUCCESS);

    // Check the uri-parts scheme, hostport, pathquery and fragment
    EXPECT_EQ(out.type, Relative);
    EXPECT_EQ(out.path_type, ABS_PATH);
    EXPECT_EQ(out.scheme.buff, nullptr);
    EXPECT_EQ(out.scheme.size, 0u);
    EXPECT_EQ(std::string_view(out.hostport.text.buff, out.hostport.text.size),
              "[2001:db8::40ec]:80");
    EXPECT_EQ(std::string_view(out.pathquery.buff, out.pathquery.size),
              "/uri/path?uriquery");
    EXPECT_EQ(std::string_view(out.fragment.buff, out.fragment.size),
              "urifragment");
    SSockaddr saObj;
    saObj = out.hostport.IPaddress;
    EXPECT_EQ(saObj.port(), 80);
    EXPECT_EQ(saObj.ss.ss_family, AF_INET6);
#ifndef __APPLE__
    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": The resolved socket address must not have a garbage "
                     "scope id.\n";
        EXPECT_THAT(saObj.netaddrp(), StartsWith("[2001:db8::40ec%")); // Wrong!
    } else
#endif
        EXPECT_EQ(saObj.netaddrp(), "[2001:db8::40ec]:80");
}

TEST(ParseUriTestSuite, relative_uri_with_absolute_path) {
    uri_type out;
    memset(&out, 0xAA, sizeof(out));

    constexpr char url_str[]{"/uri/path?uriquery#urifragment"};

    // Test Unit
    EXPECT_EQ(::parse_uri(url_str, strlen(url_str), &out), HTTP_SUCCESS);
    // Check the uri-parts scheme, hostport, pathquery and fragment
    EXPECT_EQ(out.type, Relative);
    EXPECT_EQ(out.path_type, ABS_PATH);
    EXPECT_EQ(out.scheme.buff, nullptr);
    EXPECT_EQ(out.scheme.size, 0u);
    EXPECT_EQ(out.hostport.text.buff, nullptr);
    EXPECT_EQ(out.hostport.text.size, 0u);
    EXPECT_EQ(std::string_view(out.pathquery.buff, out.pathquery.size),
              "/uri/path?uriquery");
    EXPECT_EQ(std::string_view(out.fragment.buff, out.fragment.size),
              "urifragment");
    SSockaddr saObj;
    saObj = out.hostport.IPaddress;
    EXPECT_EQ(saObj.ss.ss_family, AF_UNSPEC);
    EXPECT_EQ(saObj.port(), 0);
    EXPECT_EQ(saObj.netaddrp(), ":0");
}

TEST(ParseUriTestSuite, relative_uri_with_relative_path) {
    uri_type out;

    // The relative path does not have a leading '/'
    constexpr char url_str[]{"uri/path?uriquery#urifragment"};

    // Test Unit
    EXPECT_EQ(::parse_uri(url_str, strlen(url_str), &out), HTTP_SUCCESS);
    // Check the uri-parts scheme, hostport, pathquery and fragment
    EXPECT_EQ(out.type, Relative);
    EXPECT_EQ(out.path_type, REL_PATH);
    EXPECT_EQ(out.scheme.buff, nullptr);
    EXPECT_EQ(out.scheme.size, 0u);
    EXPECT_EQ(out.hostport.text.buff, nullptr);
    EXPECT_EQ(out.hostport.text.size, 0u);
    EXPECT_EQ(std::string_view(out.pathquery.buff, out.pathquery.size),
              "uri/path?uriquery");
    EXPECT_EQ(std::string_view(out.fragment.buff, out.fragment.size),
              "urifragment");
    SSockaddr saObj;
    saObj = out.hostport.IPaddress;
    EXPECT_EQ(saObj.ss.ss_family, AF_UNSPEC);
    EXPECT_EQ(saObj.port(), 0);
    EXPECT_EQ(saObj.netaddrp(), ":0");
}

TEST(ParseUriTestSuite, uri_with_opaque_part) {
    uri_type out;
    memset(&out, 0xAA, sizeof(out));

    // The relative path does not have a leading '/'
    constexpr char url_str[]{"mailto:a@b.com"};

    // Test Unit
    // The relative path does not have a leading '/'
    int returned{UPNP_E_INTERNAL_ERROR};
    EXPECT_EQ(returned = ::parse_uri(url_str, strlen(url_str), &out),
              HTTP_SUCCESS)
        << errStrEx(returned, HTTP_SUCCESS);
    // Check the uri-parts scheme, hostport, pathquery and fragment
    EXPECT_EQ(out.type, Absolute);
    EXPECT_EQ(out.path_type, OPAQUE_PART);
    EXPECT_EQ(std::string_view(out.scheme.buff, out.scheme.size), "mailto");
    EXPECT_EQ(out.hostport.text.buff, nullptr);
    EXPECT_EQ(out.hostport.text.size, 0u);
    EXPECT_EQ(std::string_view(out.pathquery.buff, out.pathquery.size),
              "a@b.com");
    EXPECT_EQ(out.fragment.buff, nullptr);
    EXPECT_EQ(out.fragment.size, 0u);
    SSockaddr saObj;
    saObj = out.hostport.IPaddress;
    EXPECT_EQ(saObj.ss.ss_family, AF_UNSPEC);
    EXPECT_EQ(saObj.port(), 0);
    EXPECT_EQ(saObj.netaddrp(), ":0");
}

TEST(ParseUriTestSuite, parse_uric) {
    token out;

    // There is an invalid URI character '^' as separator for a fragment. This
    // should not truncate the URI string. It should not accept the whole URI.
    constexpr char url_str2[]{"https://192.168.192.170/path/dest/"
                              "?query=value^fragment"};
    EXPECT_EQ(::parse_uric(url_str2, strlen(url_str2), &out), 46);

    // The valid separator '#' is also not accepted as valid character and also
    // trunkcate the URI string. That's a bug and should be fixed.
    constexpr char url_str1[]{"https://192.168.192.170/path/dest/"
                              "?query=value#fragment"};
    EXPECT_EQ(::parse_uric(url_str1, strlen(url_str1), &out), 46);
}

} // namespace utest

int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
