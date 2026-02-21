// Copyright (C) 2025+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-02-21

#include <UPnPsdk/src/net/http/uri.cpp>
#include <utest/utest.hpp>

namespace utest {

using STATE = UPnPsdk::CComponent::STATE;
using UPnPsdk::CFragment;
using UPnPsdk::CHost;
using UPnPsdk::CPath;
using UPnPsdk::CPort;
using UPnPsdk::CQuery;
using UPnPsdk::CScheme;
using UPnPsdk::CUri;
using UPnPsdk::CUriRef;
using UPnPsdk::CUserinfo;
using UPnPsdk::get_authority;
using UPnPsdk::get_scheme;


// CUri Unit Tests generelly
// =========================
TEST(CUriTestSuite, uri_normalize_0) {
    CUri uriObj("HttPs://[::1]");
    ASSERT_EQ(uriObj.base.scheme.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.scheme.str(), "https");
    EXPECT_EQ(uriObj.base.authority.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.str(), "[::1]");
    EXPECT_EQ(uriObj.base.authority.userinfo.state(), STATE::undef);
    EXPECT_THROW(
        { uriObj.base.authority.userinfo.str(); }, std::invalid_argument);
    EXPECT_EQ(uriObj.base.authority.host.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.host.str(), "[::1]");
    EXPECT_EQ(uriObj.base.authority.port.state(), STATE::undef);
    EXPECT_THROW({ uriObj.base.authority.port.str(); }, std::invalid_argument);
    EXPECT_EQ(uriObj.base.path.state(), STATE::empty);
    EXPECT_EQ(uriObj.base.path.str(), "");
    EXPECT_EQ(uriObj.base.query.state(), STATE::undef);
    EXPECT_THROW({ uriObj.base.query.str(); }, std::invalid_argument);
    EXPECT_EQ(uriObj.base.fragment.state(), STATE::undef);
    EXPECT_THROW({ uriObj.base.fragment.str(); }, std::invalid_argument);

    EXPECT_EQ(uriObj.base.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.str(), "https://[::1]/");

    ASSERT_EQ(uriObj.target.scheme.state(), STATE::undef);
    EXPECT_EQ(uriObj.target.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uriObj.target.authority.host.state(), STATE::undef);
    EXPECT_EQ(uriObj.target.authority.port.state(), STATE::undef);
    EXPECT_EQ(uriObj.target.authority.state(), STATE::undef);
    EXPECT_EQ(uriObj.target.path.state(), STATE::empty);
    EXPECT_EQ(uriObj.target.path.str(), "");
    EXPECT_EQ(uriObj.target.query.state(), STATE::undef);
    EXPECT_EQ(uriObj.target.fragment.state(), STATE::undef);

    EXPECT_EQ(uriObj.target.state(), STATE::empty);
    EXPECT_EQ(uriObj.target.str(), "");

    EXPECT_EQ(uriObj.state(), STATE::avail);
    EXPECT_EQ(uriObj.str(), "https://[::1]/");
}

TEST(CUriTestSuite, uri_normalize_1) {
    CUri uriObj("https://A.Aa/");
    ASSERT_EQ(uriObj.base.scheme.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.scheme.str(), "https");
    EXPECT_EQ(uriObj.base.authority.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.str(), "a.aa");
    EXPECT_EQ(uriObj.base.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.authority.host.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.host.str(), "a.aa");
    EXPECT_EQ(uriObj.base.authority.port.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.path.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.path.str(), "/");
    EXPECT_EQ(uriObj.base.query.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.fragment.state(), STATE::undef);

    EXPECT_EQ(uriObj.base.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.str(), "https://a.aa/");

    EXPECT_EQ(uriObj.state(), STATE::avail);
    EXPECT_EQ(uriObj.str(), "https://a.aa/");
}

TEST(CUriTestSuite, uri_normalize_2) {
    CUri uriObj("https://192.168.1.2:/");
    ASSERT_EQ(uriObj.base.scheme.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.scheme.str(), "https");
    EXPECT_EQ(uriObj.base.authority.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.str(), "192.168.1.2");
    EXPECT_EQ(uriObj.base.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.authority.host.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.host.str(), "192.168.1.2");
    EXPECT_EQ(uriObj.base.authority.port.state(), STATE::empty);
    EXPECT_EQ(uriObj.base.authority.port.str(), "");
    EXPECT_EQ(uriObj.base.path.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.path.str(), "/");
    EXPECT_EQ(uriObj.base.query.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.fragment.state(), STATE::undef);

    EXPECT_EQ(uriObj.base.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.str(), "https://192.168.1.2/");

    EXPECT_EQ(uriObj.state(), STATE::avail);
    EXPECT_EQ(uriObj.str(), "https://192.168.1.2/");
}

TEST(CUriTestSuite, uri_normalize_3) {
    // A "http" scheme normalizes port 80 to default undefined.
    CUri uriObj("http://a.aa:80/");
    ASSERT_EQ(uriObj.base.scheme.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.scheme.str(), "http");
    EXPECT_EQ(uriObj.base.authority.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.str(), "a.aa");
    EXPECT_EQ(uriObj.base.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.authority.host.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.host.str(), "a.aa");
    EXPECT_EQ(uriObj.base.authority.port.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.path.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.path.str(), "/");
    EXPECT_EQ(uriObj.base.query.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.fragment.state(), STATE::undef);

    EXPECT_EQ(uriObj.base.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.str(), "http://a.aa/");

    EXPECT_EQ(uriObj.state(), STATE::avail);
    EXPECT_EQ(uriObj.str(), "http://a.aa/");
}

TEST(CUriTestSuite, uri_normalize_4) {
    // A "http" scheme normalizes port 80 but not port 443.
    CUri uriObj("http://192.168.167.166:443/");
    ASSERT_EQ(uriObj.base.scheme.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.scheme.str(), "http");
    EXPECT_EQ(uriObj.base.authority.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.str(), "192.168.167.166:443");
    EXPECT_EQ(uriObj.base.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.authority.host.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.host.str(), "192.168.167.166");
    EXPECT_EQ(uriObj.base.authority.port.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.port.str(), "443");
    EXPECT_EQ(uriObj.base.path.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.path.str(), "/");
    EXPECT_EQ(uriObj.base.query.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.fragment.state(), STATE::undef);

    EXPECT_EQ(uriObj.base.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.str(), "http://192.168.167.166:443/");

    EXPECT_EQ(uriObj.state(), STATE::avail);
    EXPECT_EQ(uriObj.str(), "http://192.168.167.166:443/");
}

TEST(CUriTestSuite, uri_normalize_5) {
    // A "https" scheme normalizes port 443 to default undefined.
    CUri uriObj("https://a.aa:443/");
    ASSERT_EQ(uriObj.base.scheme.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.scheme.str(), "https");
    EXPECT_EQ(uriObj.base.authority.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.str(), "a.aa");
    EXPECT_EQ(uriObj.base.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.authority.host.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.host.str(), "a.aa");
    EXPECT_EQ(uriObj.base.authority.port.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.path.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.path.str(), "/");
    EXPECT_EQ(uriObj.base.query.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.fragment.state(), STATE::undef);

    EXPECT_EQ(uriObj.base.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.str(), "https://a.aa/");

    EXPECT_EQ(uriObj.state(), STATE::avail);
    EXPECT_EQ(uriObj.str(), "https://a.aa/");
}

TEST(CUriTestSuite, uri_normalize_6) {
    // A "https" scheme normalizes port 443 but not port 80.
    CUri uriObj("https://a.aa:80/");
    ASSERT_EQ(uriObj.base.scheme.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.scheme.str(), "https");
    EXPECT_EQ(uriObj.base.authority.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.str(), "a.aa:80");
    EXPECT_EQ(uriObj.base.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.authority.host.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.host.str(), "a.aa");
    EXPECT_EQ(uriObj.base.authority.port.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.port.str(), "80");
    EXPECT_EQ(uriObj.base.path.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.path.str(), "/");
    EXPECT_EQ(uriObj.base.query.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.fragment.state(), STATE::undef);

    EXPECT_EQ(uriObj.base.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.str(), "https://a.aa:80/");

    EXPECT_EQ(uriObj.state(), STATE::avail);
    EXPECT_EQ(uriObj.str(), "https://a.aa:80/");
}

TEST(CUriTestSuite, uri_normalize_7) {
    CUri uriObj("https://:@[::1]:/?#");
    ASSERT_EQ(uriObj.base.scheme.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.scheme.str(), "https");
    EXPECT_EQ(uriObj.base.authority.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.str(), "[::1]");
    EXPECT_EQ(uriObj.base.authority.userinfo.state(), STATE::empty);
    EXPECT_EQ(uriObj.base.authority.userinfo.str(), "");
    EXPECT_EQ(uriObj.base.authority.host.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.host.str(), "[::1]");
    EXPECT_EQ(uriObj.base.authority.port.state(), STATE::empty);
    EXPECT_EQ(uriObj.base.authority.port.str(), "");
    EXPECT_EQ(uriObj.base.path.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.path.str(), "/");
    EXPECT_EQ(uriObj.base.query.state(), STATE::empty);
    EXPECT_EQ(uriObj.base.query.str(), "");
    EXPECT_EQ(uriObj.base.fragment.state(), STATE::empty);
    EXPECT_EQ(uriObj.base.fragment.str(), "");

    EXPECT_EQ(uriObj.base.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.str(), "https://[::1]/?#");

    EXPECT_EQ(uriObj.state(), STATE::avail);
    EXPECT_EQ(uriObj.str(), "https://[::1]/?#");
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
TEST(CUriTestSuite, uri_scheme_fail) {
    // Scheme "http[s]" must always have a host.
    EXPECT_THROW({ CUri uriObj("https://"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("http://"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("https:/"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("https:"); }, std::invalid_argument);

    // Scheme "http[s]" does not allow URI without host.
    EXPECT_THROW({ CUri uriObj("https:///"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("hTTP:/"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("https://@:/"); }, std::invalid_argument);
}

TEST(CUriTestSuite, uri_get_scheme) {
    EXPECT_EQ(get_scheme("UnKnown:"), "UnKnown:");
    EXPECT_EQ(get_scheme(":"), ":");
    EXPECT_EQ(get_scheme("?"), "");
    EXPECT_EQ(get_scheme(""), "");
    EXPECT_EQ(get_scheme(" "), "");
    EXPECT_EQ(get_scheme("http"), "");
    EXPECT_EQ(get_scheme("0http://a.aa"), "");
    EXPECT_EQ(get_scheme("https#://a.aa"), "");
    // Next truncates the possible scheme to "http" that has no separator ':'.
    EXPECT_EQ(get_scheme("http\0://[::1]"), "");
}

TEST(CUriTestSuite, uri_scheme) {
    // Any syntactic valid scheme pattern is accepted. If it is supported
    // should be tested on a higher level.
    CScheme scheme1("UnKnown:");
    EXPECT_EQ(scheme1.state(), STATE::avail);
    EXPECT_EQ(scheme1.str(), "unknown");

    CScheme scheme2("http");
    EXPECT_EQ(scheme2.state(), STATE::undef);
    EXPECT_THROW(scheme2.str(), std::invalid_argument);

    CScheme scheme3(":");
    EXPECT_EQ(scheme3.state(), STATE::empty);
    EXPECT_EQ(scheme3.str(), "");

    CScheme scheme4("?");
    EXPECT_EQ(scheme4.state(), STATE::undef);

    CScheme scheme5("");
    EXPECT_EQ(scheme5.state(), STATE::undef);

    CScheme scheme6(" ");
    EXPECT_EQ(scheme6.state(), STATE::undef);

    CScheme scheme7("0http://a.aa");
    EXPECT_EQ(scheme7.state(), STATE::undef);

    CScheme scheme8("https#://a.aa");
    EXPECT_EQ(scheme8.state(), STATE::undef);

    CScheme scheme9("http\0://a.aa");
    EXPECT_EQ(scheme9.state(), STATE::undef);
}


// Scheme file component Unit Tests
// ================================
// Due to RFC8089 2. Syntax:
// 0. file       relative URI reference, not a file URI
// 1. file:      invalid
// 1. file://    invalid
// 2. file:///
// 3. file:///path/to/file
// 4. file://localhost/
// 5. file://localhost/path/to/file
// 6. file://a.aa/
// 7. file://a.aa/path/to/file
// 8. file:/
// 9. file:/path/to/file

TEST(CUriTestSuite, uri_scheme_file_1) {
    EXPECT_THROW({ CUri uriObj("file:"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("file://"); }, std::invalid_argument);
}

TEST(CUriTestSuite, uri_scheme_file_2) {
    // Scheme "file" does allow URI without host because it has defined a
    // default host "localhost". The "file" scheme must have a "path-absolute",
    // maybe empty, so the path can only have STATE::avail (RFC8089_2.).
    CUri uriObj("file:///");
    EXPECT_EQ(uriObj.base.scheme.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.scheme.str(), "file");
    EXPECT_EQ(uriObj.base.authority.state(), STATE::empty);
    EXPECT_EQ(uriObj.base.authority.str(), "");
    EXPECT_EQ(uriObj.base.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.authority.host.state(), STATE::empty);
    EXPECT_EQ(uriObj.base.authority.host.str(), "");
    EXPECT_EQ(uriObj.base.authority.port.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.path.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.path.str(), "/");
    EXPECT_EQ(uriObj.base.query.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.fragment.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.str(), "file:///");
    EXPECT_EQ(uriObj.str(), "file:///");
}

TEST(CUriTestSuite, uri_scheme_file_3) {
    // Due to RFC8089 Appendix B, example URIs, local files: a traditional file
    // URI for a local file with an empty authority. This is the most common
    // format in use today.
    CUri uriObj("file:///path/to/file");
    EXPECT_EQ(uriObj.base.scheme.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.scheme.str(), "file");
    EXPECT_EQ(uriObj.base.authority.state(), STATE::empty);
    EXPECT_EQ(uriObj.base.authority.str(), "");
    EXPECT_EQ(uriObj.base.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.authority.host.state(), STATE::empty);
    EXPECT_EQ(uriObj.base.authority.host.str(), "");
    EXPECT_EQ(uriObj.base.authority.port.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.path.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.path.str(), "/path/to/file");
    EXPECT_EQ(uriObj.base.query.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.fragment.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.str(), "file:///path/to/file");
    EXPECT_EQ(uriObj.str(), "file:///path/to/file");
}

TEST(CUriTestSuite, uri_scheme_file_4) {
    CUri uriObj("file://localhost/");
    EXPECT_EQ(uriObj.base.scheme.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.scheme.str(), "file");
    EXPECT_EQ(uriObj.base.authority.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.str(), "localhost");
    EXPECT_EQ(uriObj.base.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.authority.host.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.host.str(), "localhost");
    EXPECT_EQ(uriObj.base.authority.port.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.path.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.path.str(), "/");
    EXPECT_EQ(uriObj.base.query.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.fragment.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.str(), "file://localhost/");
    EXPECT_EQ(uriObj.str(), "file://localhost/");
}

TEST(CUriTestSuite, uri_scheme_file_5) {
    CUri uriObj("file://localhost/path/to/file");
    EXPECT_EQ(uriObj.base.scheme.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.scheme.str(), "file");
    EXPECT_EQ(uriObj.base.authority.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.str(), "localhost");
    EXPECT_EQ(uriObj.base.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.authority.host.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.host.str(), "localhost");
    EXPECT_EQ(uriObj.base.authority.port.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.path.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.path.str(), "/path/to/file");
    EXPECT_EQ(uriObj.base.query.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.fragment.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.str(), "file://localhost/path/to/file");
    EXPECT_EQ(uriObj.str(), "file://localhost/path/to/file");
}

TEST(CUriTestSuite, uri_scheme_file_6) {
    CUri uriObj("file://a.aa/");
    EXPECT_EQ(uriObj.base.scheme.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.scheme.str(), "file");
    EXPECT_EQ(uriObj.base.authority.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.str(), "a.aa");
    EXPECT_EQ(uriObj.base.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.authority.host.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.host.str(), "a.aa");
    EXPECT_EQ(uriObj.base.authority.port.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.path.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.path.str(), "/");
    EXPECT_EQ(uriObj.base.query.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.fragment.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.str(), "file://a.aa/");
    EXPECT_EQ(uriObj.str(), "file://a.aa/");
}

TEST(CUriTestSuite, uri_scheme_file_7) {
    // Due to RFC8089 Appendix B, example URIs, non-local files: a non-local
    // file with an explicit authority.
    CUri uriObj("file://a.aa/path/to/file");
    EXPECT_EQ(uriObj.base.scheme.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.scheme.str(), "file");
    EXPECT_EQ(uriObj.base.authority.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.str(), "a.aa");
    EXPECT_EQ(uriObj.base.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.authority.host.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.authority.host.str(), "a.aa");
    EXPECT_EQ(uriObj.base.authority.port.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.path.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.path.str(), "/path/to/file");
    EXPECT_EQ(uriObj.base.query.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.fragment.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.str(), "file://a.aa/path/to/file");
    EXPECT_EQ(uriObj.str(), "file://a.aa/path/to/file");
}

TEST(CUriTestSuite, uri_scheme_file_8) {
    CUri uriObj("file:/");
    EXPECT_EQ(uriObj.base.scheme.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.scheme.str(), "file");
    EXPECT_EQ(uriObj.base.authority.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.path.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.path.str(), "/");
    EXPECT_EQ(uriObj.base.query.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.fragment.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.str(), "file:/");
    EXPECT_EQ(uriObj.str(), "file:/");
}

TEST(CUriTestSuite, uri_scheme_file_9) {
    // Due to RFC8089 Appendix B, example URIs, local files: the minimal
    // representation of a local file with no authority field and an absolute
    // path that begins with a slash "/".
    CUri uriObj("file:/path/to/file");
    EXPECT_EQ(uriObj.base.scheme.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.scheme.str(), "file");
    EXPECT_EQ(uriObj.base.authority.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.path.state(), STATE::avail);
    EXPECT_EQ(uriObj.base.path.str(), "/path/to/file");
    EXPECT_EQ(uriObj.base.query.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.fragment.state(), STATE::undef);
    EXPECT_EQ(uriObj.base.str(), "file:/path/to/file");
    EXPECT_EQ(uriObj.str(), "file:/path/to/file");
}

TEST(CUriTestSuite, uri_scheme_file_invalid) {
    EXPECT_THROW({ CUri uriObj("file://a.aa"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("file://@:/"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("file://?"); }, std::invalid_argument);
    EXPECT_THROW({ CUri uriObj("file://#"); }, std::invalid_argument);
}


// Authority component Unit Tests
// ==============================
TEST(CUriTestSuite, uri_get_authority) {
    EXPECT_EQ(get_authority("scheme://userinfo@host:port/path"),
              "//userinfo@host:port");
    EXPECT_EQ(get_authority("scheme://userinfo@host:port/"),
              "//userinfo@host:port");
    EXPECT_EQ(get_authority("scheme://userinfo@host:port/?"),
              "//userinfo@host:port");
    EXPECT_EQ(get_authority("scheme://userinfo@host:port?/"),
              "//userinfo@host:port");
    EXPECT_EQ(get_authority("scheme://userinfo@host:port"),
              "//userinfo@host:port");
    EXPECT_EQ(get_authority("scheme://userinfo@host:"), "//userinfo@host:");
    EXPECT_EQ(get_authority("scheme://userinfo@host"), "//userinfo@host");
    EXPECT_EQ(get_authority("scheme://userinfo@"), "//userinfo@");
    EXPECT_EQ(get_authority("scheme://userinfo"), "//userinfo");
    EXPECT_EQ(get_authority("scheme://a.aa"), "//a.aa");
    EXPECT_EQ(get_authority("scheme://"), "//");
    EXPECT_EQ(get_authority("scheme:///"), "//");
    EXPECT_EQ(get_authority("scheme:/path"), "");
    EXPECT_EQ(get_authority("scheme:/"), "");
    EXPECT_EQ(get_authority("scheme:?query"), "");
    EXPECT_EQ(get_authority("scheme:?"), "");
    EXPECT_EQ(get_authority("scheme:#fragment"), "");
    EXPECT_EQ(get_authority("scheme:#"), "");
    EXPECT_EQ(get_authority("scheme:x"), "");
    EXPECT_EQ(get_authority("scheme:"), "");
}

TEST(CUriTestSuite, uri_userinfo) {
    CUserinfo userinfo1("//userinfo@");
    EXPECT_EQ(userinfo1.state(), STATE::avail);
    EXPECT_EQ(userinfo1.str(), "userinfo");

    CUserinfo userinfo2("//@");
    EXPECT_EQ(userinfo2.state(), STATE::empty);
    EXPECT_EQ(userinfo2.str(), "");

    CUserinfo userinfo8("//:@");
    EXPECT_EQ(userinfo8.state(), STATE::empty);
    EXPECT_EQ(userinfo8.str(), "");

    CUserinfo userinfo10("//::@");
    EXPECT_EQ(userinfo10.state(), STATE::empty);
    EXPECT_EQ(userinfo10.str(), "");

    CUserinfo userinfo11("///");
    EXPECT_EQ(userinfo11.state(), STATE::undef);

    CUserinfo userinfo9("//:password@");
    EXPECT_EQ(userinfo9.state(), STATE::empty);
    EXPECT_EQ(userinfo9.str(), "");

    CUserinfo userinfo3("//userinfo");
    EXPECT_EQ(userinfo3.state(), STATE::undef);

    CUserinfo userinfo4("userinfo@");
    EXPECT_EQ(userinfo4.state(), STATE::undef);

    CUserinfo userinfo5("/userinfo@");
    EXPECT_EQ(userinfo5.state(), STATE::undef);

    CUserinfo userinfo6("//userinfo:@");
    EXPECT_EQ(userinfo6.state(), STATE::avail);
    EXPECT_EQ(userinfo6.str(), "userinfo");

    CUserinfo userinfo7("//userinfo:password@");
    EXPECT_EQ(userinfo7.state(), STATE::avail);
    EXPECT_EQ(userinfo7.str(), "userinfo");
}

TEST(CUriTestSuite, uri_host) {
    CHost host1("//[2001:DB8::1]");
    EXPECT_EQ(host1.state(), STATE::avail);
    EXPECT_EQ(host1.str(), "[2001:db8::1]");

    // Invalid IPv6 address (two double colon).
    EXPECT_THROW(CHost host("//[2001::db8::1]"), std::invalid_argument);

    CHost host2("//192.168.200.255");
    EXPECT_EQ(host2.state(), STATE::avail);
    EXPECT_EQ(host2.str(), "192.168.200.255");

    // Invalid last octet ".256".
    EXPECT_THROW(CHost host("//192.168.200.256"), std::invalid_argument);

    CHost host3("//A.AA/");
    EXPECT_EQ(host3.state(), STATE::avail);
    EXPECT_EQ(host3.str(), "a.aa");

    // DNS root domain must have at least two characters.
    EXPECT_THROW(CHost host("//a.a/"), std::invalid_argument);

    CHost host4("//");
    EXPECT_EQ(host4.state(), STATE::empty);
    EXPECT_EQ(host4.str(), "");

    CHost host5("///");
    EXPECT_EQ(host5.state(), STATE::empty);
    EXPECT_EQ(host5.str(), "");

    CHost host6("//@/");
    EXPECT_EQ(host6.state(), STATE::empty);
    EXPECT_EQ(host6.str(), "");

    CHost host7("//:");
    EXPECT_EQ(host7.state(), STATE::empty);
    EXPECT_EQ(host7.str(), "");

    CHost host8("//?");
    EXPECT_EQ(host8.state(), STATE::empty);
    EXPECT_EQ(host8.str(), "");

    CHost host9("//#");
    EXPECT_EQ(host9.state(), STATE::empty);
    EXPECT_EQ(host9.str(), "");

    CHost host10("//@:");
    EXPECT_EQ(host10.state(), STATE::empty);
    EXPECT_EQ(host10.str(), "");

    CHost host11("//@a.aa");
    EXPECT_EQ(host11.state(), STATE::avail);
    EXPECT_EQ(host11.str(), "a.aa");
}

TEST(CUriTestSuite, uri_port) {
    CPort port1("//:65535");
    EXPECT_EQ(port1.state(), STATE::avail);
    EXPECT_EQ(port1.str(), "65535");

    CPort port11("//:65535/");
    EXPECT_EQ(port11.state(), STATE::avail);
    EXPECT_EQ(port11.str(), "65535");

    // Port is out of range 0..65535.
    EXPECT_THROW(CPort port("//:65536"), std::invalid_argument);

    CPort port2("//:");
    EXPECT_EQ(port2.state(), STATE::empty);
    EXPECT_EQ(port2.str(), "");

    CPort port21("///");
    EXPECT_EQ(port21.state(), STATE::undef);

    CPort port3("//");
    EXPECT_EQ(port3.state(), STATE::undef);

    CPort port4("/");
    EXPECT_EQ(port4.state(), STATE::undef);

    CPort port5(":");
    EXPECT_EQ(port5.state(), STATE::undef);
}

TEST(CUriTestSuite, uri_path) {
    CPath path00("scheme:");
    EXPECT_EQ(path00.state(), STATE::empty);
    EXPECT_EQ(path00.str(), "");

    CPath path0("scheme:///");
    EXPECT_EQ(path0.state(), STATE::avail);
    EXPECT_EQ(path0.str(), "/");

    CPath path1("scheme://a.aa/path/part?query#frag");
    EXPECT_EQ(path1.state(), STATE::avail);
    EXPECT_EQ(path1.str(), "/path/part");

    CPath path2("scheme://a.aa");
    EXPECT_EQ(path2.state(), STATE::empty);
    EXPECT_EQ(path2.str(), "");

    CPath path3("scheme://a.aa/");
    EXPECT_EQ(path3.state(), STATE::avail);
    EXPECT_EQ(path3.str(), "/");

    CPath path4("scheme://a.aa:/");
    EXPECT_EQ(path4.state(), STATE::avail);
    EXPECT_EQ(path4.str(), "/");

    CPath path5("https://a.aa:443/");
    EXPECT_EQ(path5.state(), STATE::avail);
    EXPECT_EQ(path5.str(), "/");

    CPath path6("scheme://a.aa/?query");
    EXPECT_EQ(path6.state(), STATE::avail);
    EXPECT_EQ(path6.str(), "/");

    CPath path7("foo://info.example.com?fred");
    EXPECT_EQ(path7.state(), STATE::empty);
    EXPECT_EQ(path7.str(), "");

    CPath path8("mailto:fred@example.com");
    EXPECT_EQ(path8.state(), STATE::avail);
    EXPECT_EQ(path8.str(), "fred@example.com");
}

TEST(CUriTestSuite, uri_query) {
    CQuery query1("scheme://a.aa/path/part?query#frag");
    EXPECT_EQ(query1.state(), STATE::avail);
    EXPECT_EQ(query1.str(), "query");

    CQuery query2("scheme://a.aa/path/part?#");
    EXPECT_EQ(query2.state(), STATE::empty);
    EXPECT_EQ(query2.str(), "");

    CQuery query3("scheme://a.aa/path/part#?frag");
    EXPECT_EQ(query3.state(), STATE::undef);

    CQuery query4("?query");
    EXPECT_EQ(query4.state(), STATE::avail);
    EXPECT_EQ(query4.str(), "query");
}

TEST(CUriTestSuite, uri_fragment) {
    CFragment query1("scheme://a.aa/path/part?query#frag");
    EXPECT_EQ(query1.state(), STATE::avail);
    EXPECT_EQ(query1.str(), "frag");

    CFragment query2("scheme://a.aa/path/part?#frag");
    EXPECT_EQ(query2.state(), STATE::avail);
    EXPECT_EQ(query2.str(), "frag");

    CFragment query3("scheme://a.aa/path/part?#");
    EXPECT_EQ(query3.state(), STATE::empty);
    EXPECT_EQ(query3.str(), "");

    CFragment query4("scheme://a.aa/path/part#?");
    EXPECT_EQ(query4.state(), STATE::avail);
    EXPECT_EQ(query4.str(), "?");

    CFragment query5("#frag");
    EXPECT_EQ(query5.state(), STATE::avail);
    EXPECT_EQ(query5.str(), "frag");
}

TEST(CUriTestSuite, uri_base) {
    CUriRef uri_ref("HttPs://userinfo:@a.Aa:56789/path/sub?query#frag");

    EXPECT_EQ(uri_ref.state(), STATE::avail);
    EXPECT_EQ(uri_ref.str(), "https://userinfo@a.aa:56789/path/sub?query#frag");
    EXPECT_EQ(uri_ref.scheme.state(), STATE::avail);
    EXPECT_EQ(uri_ref.scheme.str(), "https");
    EXPECT_EQ(uri_ref.authority.state(), STATE::avail);
    EXPECT_EQ(uri_ref.authority.str(), "userinfo@a.aa:56789");
    EXPECT_EQ(uri_ref.authority.userinfo.state(), STATE::avail);
    EXPECT_EQ(uri_ref.authority.userinfo.str(), "userinfo");
    EXPECT_EQ(uri_ref.authority.host.state(), STATE::avail);
    EXPECT_EQ(uri_ref.authority.host.str(), "a.aa");
    EXPECT_EQ(uri_ref.authority.port.state(), STATE::avail);
    EXPECT_EQ(uri_ref.authority.port.str(), "56789");
    EXPECT_EQ(uri_ref.path.state(), STATE::avail);
    EXPECT_EQ(uri_ref.path.str(), "/path/sub");
    EXPECT_EQ(uri_ref.query.state(), STATE::avail);
    EXPECT_EQ(uri_ref.query.str(), "query");
    EXPECT_EQ(uri_ref.fragment.state(), STATE::avail);
    EXPECT_EQ(uri_ref.fragment.str(), "frag");
}

TEST(CUriTestSuite, uri_rel) {
    CUriRef uri_ref("//userinfo:@A.aa:56789/path/sub?query#frag");

    EXPECT_EQ(uri_ref.state(), STATE::avail);
    EXPECT_EQ(uri_ref.str(), "//userinfo@a.aa:56789/path/sub?query#frag");
    EXPECT_EQ(uri_ref.scheme.state(), STATE::undef);
    EXPECT_EQ(uri_ref.authority.state(), STATE::avail);
    EXPECT_EQ(uri_ref.authority.str(), "userinfo@a.aa:56789");
    EXPECT_EQ(uri_ref.authority.userinfo.state(), STATE::avail);
    EXPECT_EQ(uri_ref.authority.userinfo.str(), "userinfo");
    EXPECT_EQ(uri_ref.authority.host.state(), STATE::avail);
    EXPECT_EQ(uri_ref.authority.host.str(), "a.aa");
    EXPECT_EQ(uri_ref.authority.port.state(), STATE::avail);
    EXPECT_EQ(uri_ref.authority.port.str(), "56789");
    EXPECT_EQ(uri_ref.path.state(), STATE::avail);
    EXPECT_EQ(uri_ref.path.str(), "/path/sub");
    EXPECT_EQ(uri_ref.query.state(), STATE::avail);
    EXPECT_EQ(uri_ref.query.str(), "query");
    EXPECT_EQ(uri_ref.fragment.state(), STATE::avail);
    EXPECT_EQ(uri_ref.fragment.str(), "frag");
}

TEST(CUriTestSuite, uri) {
    // UPnPsdk::CUri uri("HttPs://userinfo:@a.aa:56789/path/sub?query#frag");
    CUri uri("HttPs://a.aa/b/c/d;p?q");

    EXPECT_EQ(uri.base.state(), STATE::avail);
    EXPECT_EQ(uri.base.str(), "https://a.aa/b/c/d;p?q");
    EXPECT_EQ(uri.base.scheme.state(), STATE::avail);
    EXPECT_EQ(uri.base.scheme.str(), "https");
    EXPECT_EQ(uri.base.authority.state(), STATE::avail);
    EXPECT_EQ(uri.base.authority.str(), "a.aa");
    EXPECT_EQ(uri.base.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uri.base.authority.host.state(), STATE::avail);
    EXPECT_EQ(uri.base.authority.host.str(), "a.aa");
    EXPECT_EQ(uri.base.authority.port.state(), STATE::undef);
    EXPECT_EQ(uri.base.path.state(), STATE::avail);
    EXPECT_EQ(uri.base.path.str(), "/b/c/d;p");
    EXPECT_EQ(uri.base.query.state(), STATE::avail);
    EXPECT_EQ(uri.base.query.str(), "q");
    EXPECT_EQ(uri.base.fragment.state(), STATE::undef);

    uri = ("g;x?y#s");

    EXPECT_EQ(uri.state(), STATE::avail);
    EXPECT_EQ(uri.str(), "https://a.aa/b/c/g;x?y#s");
    EXPECT_EQ(uri.target.state(), STATE::avail);
    EXPECT_EQ(uri.target.str(), "https://a.aa/b/c/g;x?y#s");
    EXPECT_EQ(uri.target.scheme.state(), STATE::avail);
    EXPECT_EQ(uri.target.scheme.str(), "https");
    EXPECT_EQ(uri.target.authority.state(), STATE::avail);
    EXPECT_EQ(uri.target.authority.str(), "a.aa");
    EXPECT_EQ(uri.target.authority.userinfo.state(), STATE::undef);
    EXPECT_EQ(uri.target.authority.host.state(), STATE::avail);
    EXPECT_EQ(uri.target.authority.host.str(), "a.aa");
    EXPECT_EQ(uri.target.authority.port.state(), STATE::undef);
    EXPECT_EQ(uri.target.path.state(), STATE::avail);
    EXPECT_EQ(uri.target.path.str(), "/b/c/g;x");
    EXPECT_EQ(uri.target.query.state(), STATE::avail);
    EXPECT_EQ(uri.target.query.str(), "y");
    EXPECT_EQ(uri.target.fragment.state(), STATE::avail);
    EXPECT_EQ(uri.target.fragment.str(), "s");
}

TEST(CUriTestSuite, uri_invalid) {
    // Not a base URI with valid scheme.
    EXPECT_THROW(CUri uri("/a/c/d"), std::invalid_argument);
    EXPECT_THROW(CUri uri(":/a/c/d"), std::invalid_argument);

    CUri uri("HttPs://a.aa/b/c/d;p?q");
    // Not a relative reference without scheme.
    EXPECT_THROW(uri = "https://b.bb/a/c/d", std::invalid_argument);
}


// Relative reference
// ==================
// Test relative references
CUri uriObj("HttPS://a.Aa/b/c/d;p?q");

class RelativeReferencesPTestSuite
    : public ::testing::TestWithParam<
          //           rel_ref,          target_uri
          ::std::tuple<std::string_view, std::string_view>> {};

TEST_P(RelativeReferencesPTestSuite, relative_reference) {
    // Get parameter
    std::tuple params = GetParam();
    std::string_view rel_ref = std::get<0>(params);
    std::string_view target_uri = std::get<1>(params);

    uriObj = std::string(rel_ref);
    EXPECT_EQ(uriObj.str(), target_uri);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    uri, RelativeReferencesPTestSuite,
    ::testing::Values(
        //               relref, target_uri
        std::make_tuple("g", "https://a.aa/b/c/g"),
        std::make_tuple("./g", "https://a.aa/b/c/g"),
        std::make_tuple("g/", "https://a.aa/b/c/g/"),
        std::make_tuple("/g", "https://a.aa/g"),
        // std::make_tuple("//g.gg", "https://g.gg"), // TODO
        std::make_tuple("?y", "https://a.aa/b/c/d;p?y"),
        std::make_tuple("g?y", "https://a.aa/b/c/g?y"),
        std::make_tuple("#s", "https://a.aa/b/c/d;p?q#s"),
        std::make_tuple("g#s", "https://a.aa/b/c/g#s"),
        std::make_tuple("g?y#s", "https://a.aa/b/c/g?y#s"),
        std::make_tuple(";x", "https://a.aa/b/c/;x"),
        std::make_tuple("g;x", "https://a.aa/b/c/g;x"),
        // std::make_tuple("g;x?y#s", "https://a.aa/b/c/g;x?y#s"), // already tested above.
        std::make_tuple("", "https://a.aa/b/c/d;p?q"),
        std::make_tuple(".", "https://a.aa/b/c/"),
        std::make_tuple("./", "https://a.aa/b/c/"),
        std::make_tuple("..", "https://a.aa/b/"),
        std::make_tuple("../", "https://a.aa/b/"),
        std::make_tuple("../g", "https://a.aa/b/g"),
        std::make_tuple("../..", "https://a.aa/"),
        std::make_tuple("../../", "https://a.aa/"),
        std::make_tuple("../../g", "https://a.aa/g")
    ));
// clang-format on

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv); // preferred
#include <utest/utest_main.inc>
    return gtest_return_code;               // managed in utest/utest_main.inc
}
