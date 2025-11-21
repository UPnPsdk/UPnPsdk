// Copyright (C) 2025+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-11-21

#include <UPnPsdk/uri.hpp>
#include <utest/utest.hpp>

namespace utest {

using ::UPnPsdk::CUri;
using ::UPnPsdk::remove_dot_segments;
using STATE = UPnPsdk::CUri::CUriRef::STATE;


// CUriRef Unit Tests generelly
// ============================
TEST(CUriTestSuite, uri_reference) {
    CUri uriObj("https://[::1]");
    ASSERT_NE(uriObj.base.scheme.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.scheme.str(), "https");
    EXPECT_EQ(uriObj.base.path.state(), STATE::empty);

    uriObj = "/path";
    EXPECT_EQ(uriObj.rel.scheme.state(), STATE::undef);
    EXPECT_EQ(uriObj.rel.path.state(), STATE::avail);
    EXPECT_EQ(uriObj.rel.path.str(), "/path");
}

TEST(CUriTestSuite, uri_read_undefined_component) {
    // Read undifined component.
    CUri uriObj("https://[::1]/");
    EXPECT_EQ(uriObj.base.path.state(), STATE::empty);
    EXPECT_EQ(uriObj.base.path.str(), "");
    EXPECT_EQ(uriObj.base.authority.port.state(), STATE::undef);
    EXPECT_THROW({ uriObj.base.authority.port.str(); }, std::invalid_argument);
}

#if false
TEST(CUriRefTestSuite, uri_normalize_percent_encoding) {
    uriObj = "https://[::1]/%3ab%3CA%2f";
    EXPECT_EQ(uriObj.str(), "https://[::1]/%3Ab%3CA%2F");
    uriObj = "https://[::1]/%3ab%3CA%2fX";
    EXPECT_EQ(uriObj.str(), "https://[::1]/%3Ab%3CA%2FX");

    // Invalid hexdigit.
    EXPECT_THROW({ uriObj = "https://[::1]/%xAb%3CA"; }, std::invalid_argument);
    EXPECT_THROW(
        { uriObj = "https://[::1]/%3Ab%3CA%2g"; }, std::invalid_argument);
    // First octets are incomplete.
    EXPECT_THROW({ uriObj = "%"; }, std::invalid_argument);
    EXPECT_THROW({ uriObj = "%1"; }, std::invalid_argument);
    // Last octets are incomplete.
    EXPECT_THROW({ uriObj = "ht%"; }, std::invalid_argument);
    EXPECT_THROW({ uriObj = "ht%1"; }, std::invalid_argument);
    EXPECT_THROW(
        { uriObj = "https://[::1]/%3ab%3C%2f%b"; }, std::invalid_argument);
    EXPECT_THROW(
        { uriObj = "https://[::1]/%3ab%3C%2fb%"; }, std::invalid_argument);
}

// Scheme component Unit Tests
// ===========================

TEST(CUriRefTestSuite, uri_scheme) {
    // Scheme "http[s]" must always have a host.
    uriObj = "HttPs://[::1]";
    EXPECT_EQ(uriObj.scheme.state(), STATE::avail);
    EXPECT_EQ(uriObj.scheme.str(), "https");
    EXPECT_EQ(uriObj.str(), "https://[::1]/");
    EXPECT_THROW({ uriObj = "https:/"; }, std::invalid_argument);
    EXPECT_THROW({ uriObj = "https:"; }, std::invalid_argument);
    EXPECT_THROW({ uriObj = "https"; }, std::invalid_argument);

    // For scheme "file" host may be empty.
    uriObj = "FILE://[::1]";
    EXPECT_EQ(uriObj.scheme.state(), STATE::avail);
    EXPECT_EQ(uriObj.scheme.str(), "file");
    EXPECT_EQ(uriObj.str(), "file:///");
    EXPECT_THROW({ uriObj = "file:/"; }, std::invalid_argument);
    EXPECT_THROW({ uriObj = "file:"; }, std::invalid_argument);
    EXPECT_THROW({ uriObj = "file"; }, std::invalid_argument);

    // Fails, unsupported scheme.
    EXPECT_THROW({ uriObj = "unknown://[::1]"; }, std::invalid_argument);

    EXPECT_THROW({ uriObj = ""; }, std::invalid_argument);
    EXPECT_THROW({ uriObj = " "; }, std::invalid_argument);
    EXPECT_THROW({ uriObj = ":"; }, std::invalid_argument);
    EXPECT_THROW({ uriObj = "?"; }, std::invalid_argument);
    EXPECT_THROW({ uriObj = "0https://[::1]"; }, std::invalid_argument);
    EXPECT_THROW({ uriObj = "https#://[::1]"; }, std::invalid_argument);
    EXPECT_THROW({ uriObj = "hTTP:/"; }, std::invalid_argument);
    // Next truncates the possible scheme to "http" that has no separator ':'.
    EXPECT_THROW({ uriObj = "http\0://[::1]"; }, std::invalid_argument);
    // Valid but unsupported so far.
    EXPECT_THROW({ uriObj = "mailto://[::1]"; }, std::invalid_argument);
}


// Authority component Unit Tests
// ==============================

TEST(CUriRefTestSuite, uri_authority) {
    uriObj = "https://@[::1]:"; // Empty userinfo and port
    EXPECT_EQ(uriObj.authority.state(), STATE::avail);
    EXPECT_EQ(uriObj.authority.str(), "[::1]");
    EXPECT_EQ(uriObj.authority.userinfo.state(), STATE::empty);
    EXPECT_EQ(uriObj.authority.userinfo.str(), "");
    EXPECT_EQ(uriObj.authority.host.state(), STATE::avail);
    EXPECT_EQ(uriObj.authority.host.str(), "[::1]");
    EXPECT_EQ(uriObj.str(), "https://[::1]/");

    uriObj = "https://@[::1]?"; // Empty userinfo and port
    EXPECT_EQ(uriObj.authority.state(), STATE::avail);
    EXPECT_EQ(uriObj.authority.str(), "[::1]");

    // Scheme "http[s]" does not allow URI without host.
    EXPECT_THROW({ uriObj = "https:///"; }, std::invalid_argument);
    // Empty userinfo, host, and port.
    EXPECT_THROW({ uriObj = "https://@:/"; }, std::invalid_argument);

    // Scheme "file" does allow URI without hoat because it has defined a
    // default host "localhost".
    uriObj = "file:///";
    EXPECT_EQ(uriObj.scheme.state(), STATE::avail);
    EXPECT_EQ(uriObj.scheme.str(), "file");
    EXPECT_EQ(uriObj.authority.state(), STATE::empty);
    EXPECT_EQ(uriObj.authority.str(), "");
    EXPECT_EQ(uriObj.str(), "file:///");

    uriObj = "file://@:/";
    EXPECT_EQ(uriObj.scheme.state(), STATE::avail);
    EXPECT_EQ(uriObj.scheme.str(), "file");
    EXPECT_EQ(uriObj.authority.state(), STATE::empty);
    EXPECT_EQ(uriObj.authority.str(), "");
    EXPECT_EQ(uriObj.str(), "file:///");

    EXPECT_THROW({ uriObj = "http://"; }, std::invalid_argument);
}


// Authority userinfo component Unit Tests
// =======================================

TEST(CUriRefTestSuite, uri_authority_userinfo) {
    uriObj = "https://J%6fn.Doe@[::1]";
    EXPECT_EQ(uriObj.authority.userinfo.state(), STATE::avail);
    EXPECT_EQ(uriObj.authority.userinfo.str(), "J%6Fn.Doe");
    EXPECT_EQ(uriObj.str(), "https://J%6Fn.Doe@[::1]/");

    uriObj = "https://Jon.Doe:@[::1]/";
    EXPECT_EQ(uriObj.authority.userinfo.state(), STATE::avail);
    EXPECT_EQ(uriObj.authority.userinfo.str(), "Jon.Doe");
    EXPECT_EQ(uriObj.str(), "https://Jon.Doe@[::1]/");

    uriObj = "https://Jon.Doe:@[::1]?";
    EXPECT_EQ(uriObj.authority.userinfo.state(), STATE::avail);
    EXPECT_EQ(uriObj.authority.userinfo.str(), "Jon.Doe");
    EXPECT_EQ(uriObj.str(), "https://Jon.Doe@[::1]/?");

    uriObj = "https://Jon.Doe:paasword@[::1]#";
    EXPECT_EQ(uriObj.authority.userinfo.state(), STATE::avail);
    EXPECT_EQ(uriObj.authority.userinfo.str(), "Jon.Doe");
    EXPECT_EQ(uriObj.str(), "https://Jon.Doe@[::1]/#");
}


// Authority host component Unit Tests
// ===================================

TEST(CUriRefTestSuite, uri_authority_host) {
    uriObj = "https://[2001:DB8::F9E8]";
    EXPECT_EQ(uriObj.authority.host.state(), STATE::avail);
    EXPECT_EQ(uriObj.authority.host.str(), "[2001:db8::f9e8]");
    EXPECT_EQ(uriObj.str(), "https://[2001:db8::f9e8]/");

    uriObj = "https://192.168.66.77";
    EXPECT_EQ(uriObj.authority.host.state(), STATE::avail);
    EXPECT_EQ(uriObj.authority.host.str(), "192.168.66.77");
    EXPECT_EQ(uriObj.str(), "https://192.168.66.77/");

    uriObj = "http://Example.COM";
    EXPECT_EQ(uriObj.authority.host.state(), STATE::avail);
    EXPECT_EQ(uriObj.authority.host.str(), "example.com");
    EXPECT_EQ(uriObj.str(), "http://example.com/");
}

TEST(CUriRefTestSuite, uri_authority_host_scheme_file) {
    // This behavior is taken from web browser firefox.

    uriObj = "file:///";
    EXPECT_EQ(uriObj.authority.host.state(), STATE::empty);
    EXPECT_EQ(uriObj.authority.host.str(), "");
    EXPECT_EQ(uriObj.str(), "file:///");

    // A host is silently ignored.
    uriObj = "file://[2001:db8::2]";
    EXPECT_EQ(uriObj.authority.host.state(), STATE::empty);
    EXPECT_EQ(uriObj.authority.host.str(), "");
    EXPECT_EQ(uriObj.str(), "file:///");

    // Any additional character defines an empty host. Even a single space is
    // sufficient.
    EXPECT_THROW({ uriObj = "file://"; }, std::invalid_argument);
    uriObj = "file:// ";
    EXPECT_EQ(uriObj.authority.host.state(), STATE::empty);
    EXPECT_EQ(uriObj.authority.host.str(), "");
    EXPECT_EQ(uriObj.str(), "file:///");

    uriObj = "file://?";
    EXPECT_EQ(uriObj.authority.host.state(), STATE::empty);
    EXPECT_EQ(uriObj.authority.host.str(), "");
    EXPECT_EQ(uriObj.str(), "file:///?");

    uriObj = "file://#";
    EXPECT_EQ(uriObj.authority.host.state(), STATE::empty);
    EXPECT_EQ(uriObj.authority.host.str(), "");
    EXPECT_EQ(uriObj.str(), "file:///#");
}


// Authority port component Unit Tests
// ===================================

TEST(CUriRefTestSuite, uri_authority_port) {
    uriObj = "https://domain.label:54321";
    EXPECT_EQ(uriObj.authority.port.state(), STATE::avail);
    EXPECT_EQ(uriObj.authority.port.str(), "54321");
    EXPECT_EQ(uriObj.str(), "https://domain.label:54321/");

    uriObj = "http://domain.label:";
    EXPECT_EQ(uriObj.authority.port.state(), STATE::empty);
    EXPECT_EQ(uriObj.authority.port.str(), "");
    EXPECT_EQ(uriObj.str(), "http://domain.label/");

    uriObj = "https://domain.label";
    EXPECT_EQ(uriObj.authority.port.state(), STATE::undef);
    EXPECT_EQ(uriObj.str(), "https://domain.label/");

    uriObj = "http://[::1]:54324";
    EXPECT_EQ(uriObj.authority.port.state(), STATE::avail);
    EXPECT_EQ(uriObj.authority.port.str(), "54324");
    EXPECT_EQ(uriObj.str(), "http://[::1]:54324/");

    uriObj = "https://Mueller:@domain.label:54325";
    EXPECT_EQ(uriObj.authority.port.state(), STATE::avail);
    EXPECT_EQ(uriObj.authority.port.str(), "54325");
    EXPECT_EQ(uriObj.str(), "https://Mueller@domain.label:54325/");

    // Same as webbrowser firefox is doing.
    uriObj = "file://Mueller:@domain.label:55555";
    EXPECT_EQ(uriObj.authority.port.state(), STATE::undef);
    EXPECT_EQ(uriObj.str(), "file:///");

    uriObj = "http://Mueller:@domain.label";
    EXPECT_EQ(uriObj.authority.port.state(), STATE::undef);
    EXPECT_EQ(uriObj.str(), "http://Mueller@domain.label/");

    // Port number > 65535
    EXPECT_THROW(
        { uriObj = "https://domain.label:65536"; }, std::invalid_argument);
}


// Path component Unit Tests
// =========================

// remove_dot_segments() free function Unit Tests
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class RemoveDotSegmentsPTestSuite
    : public ::testing::TestWithParam<
          //           uri_path,         result
          ::std::tuple<std::string_view, std::string_view>> {};

TEST_P(RemoveDotSegmentsPTestSuite, remove_dot_segments) {
    // Get parameter
    std::tuple params = GetParam();
    std::string uri_path = std::string(std::get<0>(params));
    std::string_view result_sv = std::get<1>(params);

    remove_dot_segments(uri_path);
    EXPECT_EQ(uri_path, result_sv);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    uri, RemoveDotSegmentsPTestSuite,
    ::testing::Values(
        //               uri_path, result
        // A.
        std::make_tuple("../", ""),
        std::make_tuple("./", ""),
        std::make_tuple("../bar", "bar"),
        std::make_tuple("./bar", "bar"),
        // B.
        std::make_tuple("/./", "/"),
        std::make_tuple("/.", "/"),
        std::make_tuple("/./bar", "/bar"),
        std::make_tuple("/.bar", "/.bar"),
        // C.
        std::make_tuple("/../", "/"),
        std::make_tuple("/..", "/"),
        std::make_tuple("/../bar", "/bar"),
        std::make_tuple("/..bar", "/..bar"),
        // D.
        std::make_tuple(".", ""),
        std::make_tuple("..", ""),
        //
        std::make_tuple("/", "/"),
        std::make_tuple(".././bar", "bar"),
        std::make_tuple("/foo/./bar", "/foo/bar"),
        std::make_tuple("/bar/./", "/bar/"),
        std::make_tuple("/bar/.", "/bar/"),
        std::make_tuple("/foo/../bar", "/bar"),
        std::make_tuple("/bar/../", "/"),
        std::make_tuple("/..", "/"),
        std::make_tuple("bar/..", "/"),
        std::make_tuple("bar/.", "bar/"),
        std::make_tuple("/foo/bar/..", "/foo/"),
        std::make_tuple(".../", ".../"),
        std::make_tuple(".../bar", ".../bar"),
        std::make_tuple("/./hello/foo/../bar", "/hello/bar")
    ));
// clang-format on

TEST(CUriRefTestSuite, uri_remove_dot_segments_copy_to_c_str) {
    char uri1[]{"/foo-path/../bar"};
    std::string uri_str{uri1, strlen(uri1)};
    remove_dot_segments(uri_str);
    uri_str.copy(uri1, uri_str.size());
    // No Null delimiter set. Shows until old '\0'.
    EXPECT_STREQ(uri1, "/bar-path/../bar");
    uri1[uri_str.size()] = '\0';
    // This is correct.
    EXPECT_STREQ(uri1, "/bar");

    char uri2[]{"/foo/bar"};
    uri_str = std::string(uri2, strlen(uri2));
    remove_dot_segments(uri_str);
    uri_str.copy(uri2, uri_str.size());
    uri2[uri_str.size()] = '\0';
    EXPECT_STREQ(uri2, "/foo/bar");
}

TEST(CUriRefTestSuite, uri_path) {
    uriObj = "https://[::1]/path";
    EXPECT_EQ(uriObj.path.state(), STATE::avail);
    EXPECT_EQ(uriObj.path.str(), "/path");
    EXPECT_EQ(uriObj.str(), "https://[::1]/path");

    uriObj = "https://[::1]/path/part1";
    EXPECT_EQ(uriObj.path.state(), STATE::avail);
    EXPECT_EQ(uriObj.path.str(), "/path/part1");
    EXPECT_EQ(uriObj.str(), "https://[::1]/path/part1");

    // The trailing slash belongs to the path.
    uriObj = "https://[::1]/path/";
    EXPECT_EQ(uriObj.path.state(), STATE::avail);
    EXPECT_EQ(uriObj.path.str(), "/path/");
    EXPECT_EQ(uriObj.str(), "https://[::1]/path/");

    uriObj = "https://[::1]/";
    EXPECT_EQ(uriObj.path.state(), STATE::empty);
    EXPECT_EQ(uriObj.path.str(), "");
    EXPECT_EQ(uriObj.str(), "https://[::1]/");

    uriObj = "https://[::1]";
    EXPECT_EQ(uriObj.path.state(), STATE::empty);
    EXPECT_EQ(uriObj.path.str(), "");
    EXPECT_EQ(uriObj.str(), "https://[::1]/");

    // Complete path segments "." and ".." should be removed (RFC3986_6.2.2.3.).
    uriObj = "https://[::1]/./a/./b/../c";
    EXPECT_EQ(uriObj.path.state(), STATE::avail);
    EXPECT_EQ(uriObj.path.str(), "/a/c");
    EXPECT_EQ(uriObj.str(), "https://[::1]/a/c");

    // Hostname "[::1]." is invalid. Path starts with first single '/'.
    EXPECT_THROW(
        { uriObj = "https://[::1]./a/./b/../c"; }, std::invalid_argument);
}

TEST(CUriRefTestSuite, uri_query) {
    uriObj = "https://[::1]?query";
    EXPECT_EQ(uriObj.query.state(), STATE::avail);
    EXPECT_EQ(uriObj.query.str(), "query");
    EXPECT_EQ(uriObj.str(), "https://[::1]/?query");

    uriObj = "https://[::1]?";
    EXPECT_EQ(uriObj.query.state(), STATE::empty);
    EXPECT_EQ(uriObj.query.str(), "");
    EXPECT_EQ(uriObj.str(), "https://[::1]/?");

    uriObj = "https://[::1]??";
    EXPECT_EQ(uriObj.query.state(), STATE::avail);
    EXPECT_EQ(uriObj.query.str(), "?");
    EXPECT_EQ(uriObj.str(), "https://[::1]/??");

    uriObj = "https://[::1]?#";
    EXPECT_EQ(uriObj.query.state(), STATE::empty);
    EXPECT_EQ(uriObj.query.str(), "");
    EXPECT_EQ(uriObj.str(), "https://[::1]/?#");

    uriObj = "https://[::1]?query#";
    EXPECT_EQ(uriObj.query.state(), STATE::avail);
    EXPECT_EQ(uriObj.query.str(), "query");
    EXPECT_EQ(uriObj.str(), "https://[::1]/?query#");

    uriObj = "https://[::1]?rel/key=?1234";
    EXPECT_EQ(uriObj.query.state(), STATE::avail);
    EXPECT_EQ(uriObj.query.str(), "rel/key=?1234");
    EXPECT_EQ(uriObj.str(), "https://[::1]/?rel/key=?1234");
}

TEST(CUriRefTestSuite, uri_fragment) {
    uriObj = "https://[::1]#fragment";
    EXPECT_EQ(uriObj.fragment.state(), STATE::avail);
    EXPECT_EQ(uriObj.fragment.str(), "fragment");
    EXPECT_EQ(uriObj.str(), "https://[::1]/#fragment");

    uriObj = "https://[::1]#";
    EXPECT_EQ(uriObj.fragment.state(), STATE::empty);
    EXPECT_EQ(uriObj.fragment.str(), "");
    EXPECT_EQ(uriObj.str(), "https://[::1]/#");

    uriObj = "https://[::1]##";
    EXPECT_EQ(uriObj.fragment.state(), STATE::avail);
    EXPECT_EQ(uriObj.fragment.str(), "#");
    EXPECT_EQ(uriObj.str(), "https://[::1]/##");

    uriObj = "https://[::1]#fragment#";
    EXPECT_EQ(uriObj.fragment.state(), STATE::avail);
    EXPECT_EQ(uriObj.fragment.str(), "fragment#");
    EXPECT_EQ(uriObj.str(), "https://[::1]/#fragment#");
}
#endif

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv); // preferred
#include <utest/utest_main.inc>
    return gtest_return_code;               // managed in utest/utest_main.inc
}
