// Copyright (C) 2025+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-11-22

#include <UPnPsdk/uri.hpp>
#include <utest/utest.hpp>

namespace utest {

using ::UPnPsdk::CUri;
using ::UPnPsdk::remove_dot_segments;
using STATE = UPnPsdk::CUri::STATE;


// CUriRef Unit Tests generelly
// ============================
TEST(CUriTestSuite, uri_reference) {
    CUri uriObj("https://[::1]");
    ASSERT_NE(uriObj.base.scheme.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.scheme.str(), "https");
    EXPECT_EQ(uriObj.base.path.state(), STATE::empty);

    // uriObj = "/path";
    // EXPECT_EQ(uriObj.rel.scheme.state(), STATE::undef);
    // EXPECT_EQ(uriObj.rel.path.state(), STATE::avail);
    // EXPECT_EQ(uriObj.rel.path.str(), "/path");
}

TEST(CUriTestSuite, uri_read_undefined_component) {
    // Read undifined component.
    CUri uriObj("https://[::1]/");
    EXPECT_EQ(uriObj.base.path.state(), STATE::empty);
    EXPECT_EQ(uriObj.base.path.str(), "");
    EXPECT_EQ(uriObj.base.authority.port.state(), STATE::undef);
    EXPECT_THROW({ uriObj.base.authority.port.str(); }, std::invalid_argument);
}

TEST(CUriTestSuite, uri_normalize_percent_encoding) {
    CUri uri1Obj("https://[::1]/%3ab%3CA%2f");
    EXPECT_EQ(uri1Obj.base.str(), "https://[::1]/%3Ab%3CA%2F");
    CUri uri2Obj("https://[::1]/%3ab%3CA%2fX");
    EXPECT_EQ(uri2Obj.base.str(), "https://[::1]/%3Ab%3CA%2FX");

    // Invalid hexdigit.
    EXPECT_THROW(
        { CUri uriObj("https://[::1]/%xAb%3CA"); }, std::invalid_argument);
    EXPECT_THROW(
        { CUri uriObj("https://[::1]/%3Ab%3CA%2g"); }, std::invalid_argument);
    // First octets are incomplete.
    EXPECT_THROW({ CUri uriObj("%"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("%1"); }, std::invalid_argument);
    // Last octets are incomplete.
    EXPECT_THROW({ CUri uriObj("ht%"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("ht%1"); }, std::invalid_argument);
    EXPECT_THROW(
        { CUri uriObj("https://[::1]/%3ab%3C%2f%b"); }, std::invalid_argument);
    EXPECT_THROW(
        { CUri uriObj("https://[::1]/%3ab%3C%2fb%"); }, std::invalid_argument);
}

// Scheme component Unit Tests
// ===========================

TEST(CUriTestSuite, uri_scheme) {
    // Scheme "http[s]" must always have a host.
    CUri uri1Obj("HttPs://[::1]");
    EXPECT_EQ(uri1Obj.base.scheme.state(), STATE::avail);
    EXPECT_EQ(uri1Obj.base.scheme.str(), "https");
    EXPECT_EQ(uri1Obj.base.str(), "https://[::1]/");
    EXPECT_THROW({ CUri uriObj("https:/"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("https:"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("https"); }, std::invalid_argument);

    // For scheme "file" host may be empty.
    CUri uri2Obj("FILE://[::1]");
    EXPECT_EQ(uri2Obj.base.scheme.state(), STATE::avail);
    EXPECT_EQ(uri2Obj.base.scheme.str(), "file");
    EXPECT_EQ(uri2Obj.base.str(), "file:///");
    EXPECT_THROW({ CUri uriObj("file:/"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("file:"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("file"); }, std::invalid_argument);

    // Fails, unsupported scheme.
    EXPECT_THROW({ CUri uriObj("unknown://[::1]"); }, std::invalid_argument);

    EXPECT_THROW({ CUri uriObj(""); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj(" "); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj(":"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("?"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("0https://[::1]"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("https#://[::1]"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("hTTP:/"); }, std::invalid_argument);
    // Next truncates the possible scheme to "http" that has no separator ':'.
    EXPECT_THROW({ CUri uriObj("http\0://[::1]"); }, std::invalid_argument);
    // Valid but unsupported so far.
    EXPECT_THROW({ CUri uriObj("mailto://[::1]"); }, std::invalid_argument);
}


// Authority component Unit Tests
// ==============================

TEST(CUriTestSuite, uri_authority) {
    CUri uri1Obj("https://@[::1]:"); // Empty userinfo and port
    EXPECT_EQ(uri1Obj.base.authority.state(), STATE::avail);
    EXPECT_EQ(uri1Obj.base.authority.str(), "[::1]");
    EXPECT_EQ(uri1Obj.base.authority.userinfo.state(), STATE::empty);
    EXPECT_EQ(uri1Obj.base.authority.userinfo.str(), "");
    EXPECT_EQ(uri1Obj.base.authority.host.state(), STATE::avail);
    EXPECT_EQ(uri1Obj.base.authority.host.str(), "[::1]");
    EXPECT_EQ(uri1Obj.base.str(), "https://[::1]/");

    CUri uri2Obj("https://@[::1]?"); // Empty userinfo and port
    EXPECT_EQ(uri2Obj.base.authority.state(), STATE::avail);
    EXPECT_EQ(uri2Obj.base.authority.str(), "[::1]");

    // Scheme "http[s]" does not allow URI without host.
    EXPECT_THROW({ CUri uriObj("https:///"); }, std::invalid_argument);
    // Empty userinfo, host, and port.
    EXPECT_THROW({ CUri uriObj("https://@:/"); }, std::invalid_argument);

    // Scheme "file" does allow URI without hoat because it has defined a
    // default host "localhost".
    CUri uri3Obj("file:///");
    EXPECT_EQ(uri3Obj.base.scheme.state(), STATE::avail);
    EXPECT_EQ(uri3Obj.base.scheme.str(), "file");
    EXPECT_EQ(uri3Obj.base.authority.state(), STATE::empty);
    EXPECT_EQ(uri3Obj.base.authority.str(), "");
    EXPECT_EQ(uri3Obj.base.str(), "file:///");

    CUri uri4Obj("file://@:/");
    EXPECT_EQ(uri4Obj.base.scheme.state(), STATE::avail);
    EXPECT_EQ(uri4Obj.base.scheme.str(), "file");
    EXPECT_EQ(uri4Obj.base.authority.state(), STATE::empty);
    EXPECT_EQ(uri4Obj.base.authority.str(), "");
    EXPECT_EQ(uri4Obj.base.str(), "file:///");

    EXPECT_THROW({ CUri uriObj("http://"); }, std::invalid_argument);
}


// Authority userinfo component Unit Tests
// =======================================

TEST(CUriTestSuite, uri_authority_userinfo) {
    CUri uri1Obj("https://J%6fn.Doe@[::1]");
    EXPECT_EQ(uri1Obj.base.authority.userinfo.state(), STATE::avail);
    EXPECT_EQ(uri1Obj.base.authority.userinfo.str(), "J%6Fn.Doe");
    EXPECT_EQ(uri1Obj.base.str(), "https://J%6Fn.Doe@[::1]/");

    CUri uri2Obj("https://Jon.Doe:@[::1]/");
    EXPECT_EQ(uri2Obj.base.authority.userinfo.state(), STATE::avail);
    EXPECT_EQ(uri2Obj.base.authority.userinfo.str(), "Jon.Doe");
    EXPECT_EQ(uri2Obj.base.str(), "https://Jon.Doe@[::1]/");

    CUri uri3Obj("https://Jon.Doe:@[::1]?");
    EXPECT_EQ(uri3Obj.base.authority.userinfo.state(), STATE::avail);
    EXPECT_EQ(uri3Obj.base.authority.userinfo.str(), "Jon.Doe");
    EXPECT_EQ(uri3Obj.base.str(), "https://Jon.Doe@[::1]/?");

    CUri uri4Obj("https://Jon.Doe:paasword@[::1]#");
    EXPECT_EQ(uri4Obj.base.authority.userinfo.state(), STATE::avail);
    EXPECT_EQ(uri4Obj.base.authority.userinfo.str(), "Jon.Doe");
    EXPECT_EQ(uri4Obj.base.str(), "https://Jon.Doe@[::1]/#");
}


// Authority host component Unit Tests
// ===================================

TEST(CUriTestSuite, uri_authority_host) {
    CUri uri1Obj("https://[2001:DB8::F9E8]");
    EXPECT_EQ(uri1Obj.base.authority.host.state(), STATE::avail);
    EXPECT_EQ(uri1Obj.base.authority.host.str(), "[2001:db8::f9e8]");
    EXPECT_EQ(uri1Obj.base.str(), "https://[2001:db8::f9e8]/");

    CUri uri2Obj("https://192.168.66.77");
    EXPECT_EQ(uri2Obj.base.authority.host.state(), STATE::avail);
    EXPECT_EQ(uri2Obj.base.authority.host.str(), "192.168.66.77");
    EXPECT_EQ(uri2Obj.base.str(), "https://192.168.66.77/");

    CUri uri3Obj("http://Example.COM");
    EXPECT_EQ(uri3Obj.base.authority.host.state(), STATE::avail);
    EXPECT_EQ(uri3Obj.base.authority.host.str(), "example.com");
    EXPECT_EQ(uri3Obj.base.str(), "http://example.com/");
}

TEST(CUriTestSuite, uri_authority_host_scheme_file) {
    // This behavior is taken from web browser firefox.

    CUri uri1Obj("file:///");
    EXPECT_EQ(uri1Obj.base.authority.host.state(), STATE::empty);
    EXPECT_EQ(uri1Obj.base.authority.host.str(), "");
    EXPECT_EQ(uri1Obj.base.str(), "file:///");

    // A host is silently ignored.
    CUri uri2Obj("file://[2001:db8::2]");
    EXPECT_EQ(uri2Obj.base.authority.host.state(), STATE::empty);
    EXPECT_EQ(uri2Obj.base.authority.host.str(), "");
    EXPECT_EQ(uri2Obj.base.str(), "file:///");

    // Any additional character defines an empty host. Even a single space is
    // sufficient.
    EXPECT_THROW({ CUri uriObj("file://"); }, std::invalid_argument);
    CUri uri3Obj("file:// ");
    EXPECT_EQ(uri3Obj.base.authority.host.state(), STATE::empty);
    EXPECT_EQ(uri3Obj.base.authority.host.str(), "");
    EXPECT_EQ(uri3Obj.base.str(), "file:///");

    CUri uri4Obj("file://?");
    EXPECT_EQ(uri4Obj.base.authority.host.state(), STATE::empty);
    EXPECT_EQ(uri4Obj.base.authority.host.str(), "");
    EXPECT_EQ(uri4Obj.base.str(), "file:///?");

    CUri uri5Obj("file://#");
    EXPECT_EQ(uri5Obj.base.authority.host.state(), STATE::empty);
    EXPECT_EQ(uri5Obj.base.authority.host.str(), "");
    EXPECT_EQ(uri5Obj.base.str(), "file:///#");
}


// Authority port component Unit Tests
// ===================================

TEST(CUriTestSuite, uri_authority_port) {
    CUri uri1Obj("https://domain.label:54321");
    EXPECT_EQ(uri1Obj.base.authority.port.state(), STATE::avail);
    EXPECT_EQ(uri1Obj.base.authority.port.str(), "54321");
    EXPECT_EQ(uri1Obj.base.str(), "https://domain.label:54321/");

    CUri uri2Obj("http://domain.label:");
    EXPECT_EQ(uri2Obj.base.authority.port.state(), STATE::empty);
    EXPECT_EQ(uri2Obj.base.authority.port.str(), "");
    EXPECT_EQ(uri2Obj.base.str(), "http://domain.label/");

    CUri uri3Obj("https://domain.label");
    EXPECT_EQ(uri3Obj.base.authority.port.state(), STATE::undef);
    EXPECT_EQ(uri3Obj.base.str(), "https://domain.label/");

    CUri uri4Obj("http://[::1]:54324");
    EXPECT_EQ(uri4Obj.base.authority.port.state(), STATE::avail);
    EXPECT_EQ(uri4Obj.base.authority.port.str(), "54324");
    EXPECT_EQ(uri4Obj.base.str(), "http://[::1]:54324/");

    CUri uri5Obj("https://Mueller:@domain.label:54325");
    EXPECT_EQ(uri5Obj.base.authority.port.state(), STATE::avail);
    EXPECT_EQ(uri5Obj.base.authority.port.str(), "54325");
    EXPECT_EQ(uri5Obj.base.str(), "https://Mueller@domain.label:54325/");

    // Same as webbrowser firefox is doing.
    CUri uri6Obj("file://Mueller:@domain.label:55555");
    EXPECT_EQ(uri6Obj.base.authority.port.state(), STATE::undef);
    EXPECT_EQ(uri6Obj.base.str(), "file:///");

    CUri uri7Obj("http://Mueller:@domain.label");
    EXPECT_EQ(uri7Obj.base.authority.port.state(), STATE::undef);
    EXPECT_EQ(uri7Obj.base.str(), "http://Mueller@domain.label/");

    // Port number > 65535
    EXPECT_THROW(
        { CUri uriObj("https://domain.label:65536"); }, std::invalid_argument);
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


TEST(CUriTestSuite, uri_remove_dot_segments_copy_to_c_str) {
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

TEST(CUriTestSuite, uri_path) {
    CUri uri1Obj("https://[::1]/path");
    EXPECT_EQ(uri1Obj.base.path.state(), STATE::avail);
    EXPECT_EQ(uri1Obj.base.path.str(), "/path");
    EXPECT_EQ(uri1Obj.base.str(), "https://[::1]/path");

    CUri uri2Obj("https://[::1]/path/part1");
    EXPECT_EQ(uri2Obj.base.path.state(), STATE::avail);
    EXPECT_EQ(uri2Obj.base.path.str(), "/path/part1");
    EXPECT_EQ(uri2Obj.base.str(), "https://[::1]/path/part1");

    // The trailing slash belongs to the path.
    CUri uri3Obj("https://[::1]/path/");
    EXPECT_EQ(uri3Obj.base.path.state(), STATE::avail);
    EXPECT_EQ(uri3Obj.base.path.str(), "/path/");
    EXPECT_EQ(uri3Obj.base.str(), "https://[::1]/path/");

    CUri uri4Obj("https://[::1]/");
    EXPECT_EQ(uri4Obj.base.path.state(), STATE::empty);
    EXPECT_EQ(uri4Obj.base.path.str(), "");
    EXPECT_EQ(uri4Obj.base.str(), "https://[::1]/");

    CUri uri5Obj("https://[::1]");
    EXPECT_EQ(uri5Obj.base.path.state(), STATE::empty);
    EXPECT_EQ(uri5Obj.base.path.str(), "");
    EXPECT_EQ(uri5Obj.base.str(), "https://[::1]/");

    // Complete path segments "." and ".." should be removed (RFC3986_6.2.2.3.).
    CUri uri6Obj("https://[::1]/./a/./b/../c");
    EXPECT_EQ(uri6Obj.base.path.state(), STATE::avail);
    EXPECT_EQ(uri6Obj.base.path.str(), "/a/c");
    EXPECT_EQ(uri6Obj.base.str(), "https://[::1]/a/c");

    // Hostname "[::1]." is invalid. Path starts with first single '/'.
    EXPECT_THROW(
        { CUri uriObj("https://[::1]./a/./b/../c"); }, std::invalid_argument);
}

TEST(CUriTestSuite, uri_query) {
    CUri uri1Obj("https://[::1]?query");
    EXPECT_EQ(uri1Obj.base.query.state(), STATE::avail);
    EXPECT_EQ(uri1Obj.base.query.str(), "query");
    EXPECT_EQ(uri1Obj.base.str(), "https://[::1]/?query");

    CUri uri2Obj("https://[::1]?");
    EXPECT_EQ(uri2Obj.base.query.state(), STATE::empty);
    EXPECT_EQ(uri2Obj.base.query.str(), "");
    EXPECT_EQ(uri2Obj.base.str(), "https://[::1]/?");

    CUri uri3Obj("https://[::1]??");
    EXPECT_EQ(uri3Obj.base.query.state(), STATE::avail);
    EXPECT_EQ(uri3Obj.base.query.str(), "?");
    EXPECT_EQ(uri3Obj.base.str(), "https://[::1]/??");

    CUri uri4Obj("https://[::1]?#");
    EXPECT_EQ(uri4Obj.base.query.state(), STATE::empty);
    EXPECT_EQ(uri4Obj.base.query.str(), "");
    EXPECT_EQ(uri4Obj.base.str(), "https://[::1]/?#");

    CUri uri5Obj("https://[::1]?query#");
    EXPECT_EQ(uri5Obj.base.query.state(), STATE::avail);
    EXPECT_EQ(uri5Obj.base.query.str(), "query");
    EXPECT_EQ(uri5Obj.base.str(), "https://[::1]/?query#");

    CUri uri6Obj("https://[::1]?rel/key=?1234");
    EXPECT_EQ(uri6Obj.base.query.state(), STATE::avail);
    EXPECT_EQ(uri6Obj.base.query.str(), "rel/key=?1234");
    EXPECT_EQ(uri6Obj.base.str(), "https://[::1]/?rel/key=?1234");
}

TEST(CUriTestSuite, uri_fragment) {
    CUri uri1Obj("https://[::1]#fragment");
    EXPECT_EQ(uri1Obj.base.fragment.state(), STATE::avail);
    EXPECT_EQ(uri1Obj.base.fragment.str(), "fragment");
    EXPECT_EQ(uri1Obj.base.str(), "https://[::1]/#fragment");

    CUri uri2Obj("https://[::1]#");
    EXPECT_EQ(uri2Obj.base.fragment.state(), STATE::empty);
    EXPECT_EQ(uri2Obj.base.fragment.str(), "");
    EXPECT_EQ(uri2Obj.base.str(), "https://[::1]/#");

    CUri uri3Obj("https://[::1]##");
    EXPECT_EQ(uri3Obj.base.fragment.state(), STATE::avail);
    EXPECT_EQ(uri3Obj.base.fragment.str(), "#");
    EXPECT_EQ(uri3Obj.base.str(), "https://[::1]/##");

    CUri uri4Obj("https://[::1]#fragment#");
    EXPECT_EQ(uri4Obj.base.fragment.state(), STATE::avail);
    EXPECT_EQ(uri4Obj.base.fragment.str(), "fragment#");
    EXPECT_EQ(uri4Obj.base.str(), "https://[::1]/#fragment#");
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv); // preferred
#include <utest/utest_main.inc>
    return gtest_return_code;               // managed in utest/utest_main.inc
}
