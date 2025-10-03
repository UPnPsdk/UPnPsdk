// Copyright (C) 2025+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-10-26

#include <UPnPsdk/uri.hpp>
#include <utest/utest.hpp>

namespace utest {

using ::UPnPsdk::CUri;
using STATE = CUri::STATE;

TEST(CUriTestSuite, uri_empty) {
    CUri uri1Obj;
    EXPECT_EQ(uri1Obj.scheme.state(), STATE::undef);
    EXPECT_EQ(uri1Obj.scheme.str(), "");
    EXPECT_EQ(uri1Obj.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uri1Obj.authority.userinfo.str(), "");
    EXPECT_EQ(uri1Obj.authority.host.state(), STATE::undef);
    EXPECT_EQ(uri1Obj.authority.host.str(), "");
    EXPECT_EQ(uri1Obj.authority.port.state(), STATE::undef);
    EXPECT_EQ(uri1Obj.authority.port.str(), "");
    EXPECT_EQ(uri1Obj.path.state(), STATE::undef);
    EXPECT_EQ(uri1Obj.path.str(), "");
    EXPECT_EQ(uri1Obj.query.state(), STATE::undef);
    EXPECT_EQ(uri1Obj.query.str(), "");
    EXPECT_EQ(uri1Obj.fragment.state(), STATE::undef);
    EXPECT_EQ(uri1Obj.fragment.str(), "");
}

TEST(CUriTestSuite, uri_scheme) {
    CUri uri1Obj("HTTPs://[::1]"); // Scheme http[s] must have a host.
    EXPECT_EQ(uri1Obj.scheme.state(), STATE::avail);
    EXPECT_EQ(uri1Obj.scheme.str(), "https");
    EXPECT_EQ(uri1Obj.authority.state(), STATE::avail);
    EXPECT_EQ(uri1Obj.authority.str(), "[::1]");
    EXPECT_EQ(uri1Obj.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uri1Obj.authority.host.state(), STATE::avail);
    EXPECT_EQ(uri1Obj.authority.host.str(), "[::1]");
    EXPECT_EQ(uri1Obj.authority.port.state(), STATE::undef);
    EXPECT_EQ(uri1Obj.path.state(), STATE::empty);
    EXPECT_EQ(uri1Obj.path.str(), "");
    EXPECT_EQ(uri1Obj.query.state(), STATE::undef);
    EXPECT_EQ(uri1Obj.fragment.state(), STATE::undef);
    EXPECT_EQ(uri1Obj.str(), "https://[::1]/");

    EXPECT_THROW({ CUri uriObj("https"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("https:"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("file:"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj(""); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj(" "); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj(":"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("?"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("0http:"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("https#:"); }, std::invalid_argument);
    // Scheme http[s] must always have a host.
    EXPECT_THROW({ CUri uriObj("hTTP:/"); }, std::invalid_argument);
    // Next truncates the possible scheme to "http" that has no separator ':'.
    EXPECT_THROW({ CUri uriObj("http\0:"); }, std::invalid_argument);
    // Valid but unsupported so far.
    EXPECT_THROW({ CUri uriObj("mailto://[::1]"); }, std::invalid_argument);
}

TEST(CUriTestSuite, uri_http_https_minimal) {
    // All of the requirements for the "http" scheme are also
    // requirements for the "https" scheme, except that TCP port 443 is the
    // default (RFC7230 2.7.2.).

    CUri uri2Obj("https://@[::1]:"); // Empty userinfo and port
    EXPECT_EQ(uri2Obj.scheme.state(), STATE::avail);
    EXPECT_EQ(uri2Obj.scheme.str(), "https");
    EXPECT_EQ(uri2Obj.authority.state(), STATE::avail);
    EXPECT_EQ(uri2Obj.authority.str(), "[::1]");
    EXPECT_EQ(uri2Obj.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uri2Obj.authority.host.state(), STATE::avail);
    EXPECT_EQ(uri2Obj.authority.host.str(), "[::1]");
    EXPECT_EQ(uri2Obj.authority.port.state(), STATE::undef);
    EXPECT_EQ(uri2Obj.str(), "https://[::1]/");
}

TEST(CUriTestSuite, uri_authority_empty) {
    // A sender MUST NOT generate an "http" URI with an empty host identifier.
    // A recipient that processes such a URI reference MUST reject it as
    // invalid (RFC7230 2.7.1.).

    EXPECT_THROW({ CUri uriObj("https:path"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("http://"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("https://"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("http:///"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("https:///"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("http:///path"); }, std::invalid_argument);

    // Unspecified due to RFC3986_3.3.
    EXPECT_THROW({ CUri uriObj("file://"); }, std::invalid_argument);
    // For scheme "file" an empty authority is accepted because it has implicit
    // a default authority.host "localhost" defined (RFC3986_3.2.2.). When a
    // scheme defines a default for authority and a URI reference to that
    // default is desired, the reference should be normalized to an empty
    // authority (RFC3986_6.2.3.).
    CUri uri1Obj("file:///");
    EXPECT_EQ(uri1Obj.scheme.str(), "file");
    EXPECT_EQ(uri1Obj.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uri1Obj.authority.host.state(), STATE::empty);
    EXPECT_EQ(uri1Obj.authority.host.str(), "");
    EXPECT_EQ(uri1Obj.authority.port.state(), STATE::undef);
    EXPECT_EQ(uri1Obj.str(), "file:///");
}

TEST(CUriTestSuite, uri_authority) {
    CUri uri3Obj("Https://Mueller@example.com:57391");
    EXPECT_EQ(uri3Obj.scheme.str(), "https");
    EXPECT_EQ(uri3Obj.authority.state(), STATE::avail);
    EXPECT_EQ(uri3Obj.authority.str(), "Mueller@example.com:57391");
    EXPECT_EQ(uri3Obj.authority.userinfo.state(), STATE::avail);
    EXPECT_EQ(uri3Obj.authority.host.state(), STATE::avail);
    EXPECT_EQ(uri3Obj.authority.port.state(), STATE::avail);
}

TEST(CUriTestSuite, uri_authority_userinfo) {
    CUri uri1Obj;
    EXPECT_EQ(uri1Obj.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uri1Obj.authority.userinfo.str(), "");

    CUri uri2Obj("https://Fritz.Mueller@example.com:54321/path");
    EXPECT_EQ(uri2Obj.authority.userinfo.state(), STATE::avail);
    EXPECT_EQ(uri2Obj.authority.userinfo.str(), "Fritz.Mueller");
    EXPECT_EQ(uri2Obj.str(), "https://Fritz.Mueller@example.com:54321/path");

    CUri uri3Obj("http://fritz.mueller@example.com:54321");
    EXPECT_EQ(uri3Obj.authority.userinfo.state(), STATE::avail);
    EXPECT_EQ(uri3Obj.authority.userinfo.str(), "fritz.mueller");
    EXPECT_EQ(uri3Obj.str(), "http://fritz.mueller@example.com:54321/");

    // An optional user information, if present, is followed by a commercial
    // at-sign ('@') that delimits it from the host (RFC3986 3.2.1.). The host
    // is mandatory (RFC3986 3.2.) within an available authority.
    EXPECT_THROW(
        { CUri uri4Obj("HTTPS://FRITZ.MUELLER@"); }, std::invalid_argument);

    // So the following is interpreted as host name that fails because its TLD
    // (top level domain) "mueller" is to long.
    EXPECT_THROW(
        { CUri uriObj("https://fritz.mueller"); }, std::invalid_argument);

    CUri uri6Obj("http://@[::1]"); // empty authority.userinfo.
    EXPECT_EQ(uri6Obj.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uri6Obj.str(), "http://[::1]/");

    // empty authority.userinfo. The second '@' will be the hostname that
    // obviously fails.
    EXPECT_THROW({ CUri uriObj("http://@@"); }, std::invalid_argument);

    CUri uri8Obj("https://Mueller:@[::1]"); // No password used.
    EXPECT_EQ(uri8Obj.authority.userinfo.state(), STATE::avail);
    EXPECT_EQ(uri8Obj.authority.userinfo.str(), "Mueller");
    EXPECT_EQ(uri8Obj.str(), "https://Mueller@[::1]/");

    CUri uri9Obj("https://Mueller:password@[::1]"); // Password suppressed.
    EXPECT_EQ(uri9Obj.authority.userinfo.state(), STATE::avail);
    EXPECT_EQ(uri9Obj.authority.userinfo.str(), "Mueller");
    EXPECT_EQ(uri9Obj.str(), "https://Mueller@[::1]/");

    // Empty authority.userinfo and empty password.
    CUri uri10Obj("https://:@[::1]");
    EXPECT_EQ(uri10Obj.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uri10Obj.str(), "https://[::1]/");

    // Empty userinfo and suppressed password ":".
    CUri uri11Obj("https://::@[::1]");
    EXPECT_EQ(uri11Obj.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uri11Obj.str(), "https://[::1]/");
}

TEST(CUriTestSuite, uri_authority_host) {
    CUri uri0Obj("https://Mueller@[::1]:54321/");
    EXPECT_EQ(uri0Obj.authority.host.state(), STATE::avail);
    EXPECT_EQ(uri0Obj.authority.host.str(), "[::1]");

    CUri uri1Obj("https://[2001:db8::403a]");
    EXPECT_EQ(uri1Obj.authority.host.state(), STATE::avail);
    EXPECT_EQ(uri1Obj.authority.host.str(), "[2001:db8::403a]");

    CUri uri2Obj("https://192.168.47.11");
    EXPECT_EQ(uri2Obj.authority.host.state(), STATE::avail);
    EXPECT_EQ(uri2Obj.authority.host.str(), "192.168.47.11");

    CUri uri3Obj("https://localhost");
    EXPECT_EQ(uri3Obj.authority.host.state(), STATE::avail);
    EXPECT_EQ(uri3Obj.authority.host.str(), "localhost");

    CUri uri7Obj("https://Example.COM");
    EXPECT_EQ(uri7Obj.authority.host.state(), STATE::avail);
    EXPECT_EQ(uri7Obj.authority.host.str(), "example.com");
}

TEST(CUriTestSuite, uri_authority_host_exception) {
    EXPECT_THROW(
        { CUri uriObj("https://[2001:db8::defg]"); }, std::invalid_argument);
    // Wrong IPv4 address. 256 exeeds octet limit of 255.
    EXPECT_THROW(
        { CUri uriObj("https://192.168.255.256"); }, std::invalid_argument);
    EXPECT_THROW(
        { CUri uriObj("https://-example.com"); }, std::invalid_argument);
    EXPECT_THROW(
        { CUri uriObj("https://example-.com"); }, std::invalid_argument);
    EXPECT_THROW(
        { CUri uriObj("https://example.-com"); }, std::invalid_argument);
    EXPECT_THROW(
        { CUri uriObj("https://example.com-"); }, std::invalid_argument);
    EXPECT_THROW(
        { CUri uriObj("https://not--valid.com"); }, std::invalid_argument);
    // No host specified.
    EXPECT_THROW({ CUri uriObj("https://@"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("https://:"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("https://:@:"); }, std::invalid_argument);
}

TEST(CUriTestSuite, uri_http_https_authority_port) {
    CUri uri1Obj("https://domain.label:54321");
    EXPECT_EQ(uri1Obj.authority.port.state(), STATE::avail);
    EXPECT_EQ(uri1Obj.authority.port.str(), "54321");
    EXPECT_EQ(uri1Obj.str(), "https://domain.label:54321/");

    CUri uri2Obj("http://domain.label:");
    EXPECT_EQ(uri2Obj.authority.port.state(), STATE::undef);
    EXPECT_EQ(uri2Obj.authority.port.str(), "");
    EXPECT_EQ(uri2Obj.str(), "http://domain.label/");

    CUri uri3Obj("https://domain.label");
    EXPECT_EQ(uri3Obj.authority.port.state(), STATE::undef);
    EXPECT_EQ(uri3Obj.str(), "https://domain.label/");

    CUri uri4Obj("http://[::1]:54324");
    EXPECT_EQ(uri4Obj.authority.port.state(), STATE::avail);
    EXPECT_EQ(uri4Obj.authority.port.str(), "54324");
    EXPECT_EQ(uri4Obj.str(), "http://[::1]:54324/");

    CUri uri5Obj("https://Mueller:@domain.label:54325");
    EXPECT_EQ(uri5Obj.authority.port.state(), STATE::avail);
    EXPECT_EQ(uri5Obj.authority.port.str(), "54325");
    EXPECT_EQ(uri5Obj.str(), "https://Mueller@domain.label:54325/");

    CUri uri6Obj("http://Mueller:@domain.label");
    EXPECT_EQ(uri6Obj.authority.port.state(), STATE::undef);
    EXPECT_EQ(uri6Obj.str(), "http://Mueller@domain.label/");

    // Port number > 65535
    EXPECT_THROW(
        { CUri uriObj("https://domain.label:65536"); }, std::invalid_argument);
}

TEST(CUriTestSuite, uri_path) {
    CUri uri1Obj("https://[::1]/path");
    EXPECT_EQ(uri1Obj.path.state(), STATE::avail);
    EXPECT_EQ(uri1Obj.path.str(), "path");
    EXPECT_EQ(uri1Obj.str(), "https://[::1]/path");

    CUri uri3Obj("https://[::1]/path/part1");
    EXPECT_EQ(uri3Obj.path.state(), STATE::avail);
    EXPECT_EQ(uri3Obj.path.str(), "path/part1");
    EXPECT_EQ(uri3Obj.str(), "https://[::1]/path/part1");

    // The trailing slash belongs to the path.
    CUri uri4Obj("https://[::1]/path/");
    EXPECT_EQ(uri4Obj.path.state(), STATE::avail);
    EXPECT_EQ(uri4Obj.path.str(), "path/");
    EXPECT_EQ(uri4Obj.str(), "https://[::1]/path/");

    CUri uri5Obj("https://[::1]/");
    EXPECT_EQ(uri5Obj.path.state(), STATE::empty);
    EXPECT_EQ(uri5Obj.path.str(), "");
    EXPECT_EQ(uri5Obj.str(), "https://[::1]/");
}

TEST(CUriTestSuite, uri_query) {
    CUri uri1Obj("https://[::1]?query");
    EXPECT_EQ(uri1Obj.query.state(), STATE::avail);
    EXPECT_EQ(uri1Obj.query.str(), "query");
    EXPECT_EQ(uri1Obj.str(), "https://[::1]/?query");

    CUri uri2Obj("https://[::1]?");
    EXPECT_EQ(uri2Obj.query.state(), STATE::empty);
    EXPECT_EQ(uri2Obj.query.str(), "");
    EXPECT_EQ(uri2Obj.str(), "https://[::1]/?");

    CUri uri7Obj("https://[::1]??");
    EXPECT_EQ(uri7Obj.query.state(), STATE::avail);
    EXPECT_EQ(uri7Obj.query.str(), "?");
    EXPECT_EQ(uri7Obj.str(), "https://[::1]/??");

    CUri uri3Obj("https://[::1]?#");
    EXPECT_EQ(uri3Obj.query.state(), STATE::empty);
    EXPECT_EQ(uri3Obj.query.str(), "");
    EXPECT_EQ(uri3Obj.str(), "https://[::1]/?#");

    CUri uri5Obj("https://[::1]?query#");
    EXPECT_EQ(uri5Obj.query.state(), STATE::avail);
    EXPECT_EQ(uri5Obj.query.str(), "query");
    EXPECT_EQ(uri5Obj.str(), "https://[::1]/?query#");

    CUri uri6Obj("https://[::1]?rel/key=?1234");
    EXPECT_EQ(uri6Obj.query.state(), STATE::avail);
    EXPECT_EQ(uri6Obj.query.str(), "rel/key=?1234");
    EXPECT_EQ(uri6Obj.str(), "https://[::1]/?rel/key=?1234");
}

TEST(CUriTestSuite, uri_fragment) {
    CUri uri1Obj("https://[::1]#fragment");
    EXPECT_EQ(uri1Obj.fragment.state(), STATE::avail);
    EXPECT_EQ(uri1Obj.fragment.str(), "fragment");
    EXPECT_EQ(uri1Obj.str(), "https://[::1]/#fragment");

    CUri uri2Obj("https://[::1]#");
    EXPECT_EQ(uri2Obj.fragment.state(), STATE::empty);
    EXPECT_EQ(uri2Obj.fragment.str(), "");
    EXPECT_EQ(uri2Obj.str(), "https://[::1]/#");

    CUri uri3Obj("https://[::1]##");
    EXPECT_EQ(uri3Obj.fragment.state(), STATE::avail);
    EXPECT_EQ(uri3Obj.fragment.str(), "#");
    EXPECT_EQ(uri3Obj.str(), "https://[::1]/##");

    CUri uri4Obj("https://[::1]#fragment#");
    EXPECT_EQ(uri4Obj.fragment.state(), STATE::avail);
    EXPECT_EQ(uri4Obj.fragment.str(), "fragment#");
    EXPECT_EQ(uri4Obj.str(), "https://[::1]/#fragment#");
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv); // preferred
#include <utest/utest_main.inc>
    return gtest_return_code;               // managed in utest/utest_main.inc
}
