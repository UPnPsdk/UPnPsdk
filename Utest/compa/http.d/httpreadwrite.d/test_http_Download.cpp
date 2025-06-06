// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-05-30

// Include source code for testing. So we have also direct access to static
// functions which need to be tested.
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
#include <Pupnp/upnp/src/genlib/net/http/httpreadwrite.cpp>
#else
#include <Compa/src/genlib/net/http/httpreadwrite.cpp>
#endif

#include <cmake_vars.hpp>

#include <UPnPsdk/upnptools.hpp>
#include <UPnPsdk/sockaddr.hpp>
#include <UPnPsdk/socket.hpp>

#include <utest/utest.hpp>
#include <umock/sysinfo_mock.hpp>
#include <umock/sys_socket_mock.hpp>
#include <umock/stdio_mock.hpp>


namespace utest {

using ::testing::_;
using ::testing::DoAll;
using ::testing::Ge;
using ::testing::InSequence;
using ::testing::NotNull;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SetArgPointee; // <argno> is 0-based
using ::testing::SetErrnoAndReturn;
using ::testing::StrEq;
using ::testing::StrictMock;

using ::UPnPsdk::errStrEx;
using ::UPnPsdk::g_dbug;
using ::UPnPsdk::SSockaddr;

/*
clang-format off

     http_Download()
02)  |__ http_FixStrUrl()
03)  |__ get_hoststr()
04)  |__ http_MakeMessage()
05)  |__ http_RequestAndResponse() // get doc msg
     |   |__ socket()              // system call
07)  |   |__ private_connect()     // connect
08)  |   |__ http_SendMessage()    // send request
     |   |__ http_RecvMessage()    // receive response
     |
     |__ if content_type
     |      copy content type
     |__ extract doc from response msg

02) Tested with test_httpreadwrite.cpp - TEST(HttpFixUrl*)
03) Tested with test_httpreadwrite.cpp - TEST(GetHostaddr*)
04) Tested with TEST(*, make_message_*)
05) Tested with TEST(*, request_response_*)
07) Tested with test_netconnect.cpp - TEST(PrivateConnectFTestSuite, *)
08) Tested with TEST(*, http_send_message_*)

clang-format on
*/

class HttpBasicFTestSuite : public ::testing::Test {
  protected:
    // CLogging logObj; // Output only with build type DEBUG.
    membuffer m_request{};

    // Constructor
    HttpBasicFTestSuite() {
        // if (g_dbug)
        //     logObj.enable(UPNP_ALL);
        membuffer_init(&m_request);
    }

    // Destructor
    ~HttpBasicFTestSuite() override { membuffer_destroy(&m_request); }
};


class HttpMockFTestSuite : public HttpBasicFTestSuite {
  protected:
    // clang-format off
    // Instantiate mocking objects.
    StrictMock<umock::Sys_socketMock> m_sys_socketObj;
    StrictMock<umock::StdioMock> m_stdioObj;
    // Inject the mocking objects into the tested code.
    umock::Sys_socket sys_socket_injectObj = umock::Sys_socket(&m_sys_socketObj);
    umock::Stdio stdio_injectObj = umock::Stdio(&m_stdioObj);
    // clang-format on

    // Constructor
    HttpMockFTestSuite() {
        // Set default socket object values
        ON_CALL(m_sys_socketObj, socket(_, _, _))
            .WillByDefault(SetErrnoAndReturn(EACCESP, SOCKET_ERROR));
        ON_CALL(m_sys_socketObj, bind(_, _, _))
            .WillByDefault(SetErrnoAndReturn(EBADFP, SOCKET_ERROR));
        ON_CALL(m_sys_socketObj, listen(_, _))
            .WillByDefault(SetErrnoAndReturn(EBADFP, SOCKET_ERROR));
        ON_CALL(m_sys_socketObj, select(_, _, _, _, _))
            .WillByDefault(SetErrnoAndReturn(EBADFP, SOCKET_ERROR));
        ON_CALL(m_sys_socketObj, getsockopt(_, _, _, _, _))
            .WillByDefault(SetErrnoAndReturn(EBADFP, SOCKET_ERROR));
        ON_CALL(m_sys_socketObj, setsockopt(_, _, _, _, _))
            .WillByDefault(SetErrnoAndReturn(EBADFP, SOCKET_ERROR));
        ON_CALL(m_sys_socketObj, getsockname(_, _, _))
            .WillByDefault(SetErrnoAndReturn(EBADFP, SOCKET_ERROR));
        ON_CALL(m_stdioObj, fopen(_, _))
            .WillByDefault(SetErrnoAndReturn(EINVALP, nullptr));
        // stdio fread, fwrite, does not return errors but default 0.
        ON_CALL(m_stdioObj, fclose(_))
            .WillByDefault(SetErrnoAndReturn(EBADFP, EOF));
        ON_CALL(m_stdioObj, fflush(_))
            .WillByDefault(SetErrnoAndReturn(EBADFP, EOF));
        ON_CALL(m_stdioObj, feof(_)).WillByDefault(Return(1));
        ON_CALL(m_stdioObj, ferror(_)).WillByDefault(Return(1));
        // void stdio clearerr() does nothing return.
    }
};


// Testsuite for http_MakeMessage()
// ================================
// For format types look at the declaration of http_MakeMessage() in the header
// file httpreadwrite.hpp. All available format types are tested.


TEST_F(HttpBasicFTestSuite, make_message_format_B_successful) {
    // format type 'B':  arg = int status_code  -- appends content-length,
    //                                             content-type and HTML body
    //                                             for given code.
    // status_code as given in .*/statcodes.cpp in ranges beginning with 100,
    // 200, 300, 400 and 500.

    // Test Unit with status_code 200 -> "OK"
    EXPECT_EQ(http_MakeMessage(&m_request, 1, 1, "B", 200), 0);
    EXPECT_STREQ(m_request.buf,
                 "CONTENT-LENGTH: 41\r\nAccept-Ranges: bytes\r\nCONTENT-TYPE: "
                 "text/html\r\n\r\n<html><body><h1>200 OK</h1></body></html>");
}

TEST_F(HttpBasicFTestSuite, make_message_format_b_successful) {
    // format type 'b':  arg1 = const char* buf;  -- mem buffer
    //                   arg2 = size_t buf_length memory ptr

    constexpr char hoststr[]{"upnplib.net:443/tvdevicedesc.xml"};
    size_t hostlen = 15; // hoststr[14] is '/' but not '\0'!

    // Test Unit
    EXPECT_EQ(http_MakeMessage(&m_request, 1, 1, "b", hoststr, hostlen), 0);
    EXPECT_STREQ(m_request.buf, "upnplib.net:443");
}

TEST_F(HttpBasicFTestSuite, make_message_format_C_successful) {
    // format type 'C':  (no args)  -- appends a HTTP CONNECTION: close header
    //                                 depending on major, minor version.

    // Test Unit
    EXPECT_EQ(http_MakeMessage(&m_request, 1, 1, "C"), 0);
    EXPECT_STREQ(m_request.buf, "CONNECTION: close\r\n");
}

TEST_F(HttpBasicFTestSuite, make_message_format_c_successful) {
    // format type 'c':  (no args)  -- appends CRLF "\r\n"

    // Test Unit
    EXPECT_EQ(http_MakeMessage(&m_request, 1, 1, "c"), 0);
    EXPECT_STREQ(m_request.buf, "\r\n");
}

TEST_F(HttpBasicFTestSuite, make_message_format_D_successful) {
    // format type 'D':  (no args)  -- appends HTTP DATE: header

    // Mock "current" time to have a constant value.
    umock::SysinfoMock sysinfoObj;
    umock::Sysinfo sysinfo_injectObj(&sysinfoObj);
    // Return Unix Epoch
    EXPECT_CALL(sysinfoObj, time(nullptr)).WillOnce(Return(1675628181));

    // Test Unit
    EXPECT_EQ(http_MakeMessage(&m_request, 1, 1, "D"), 0);
    EXPECT_STREQ(m_request.buf, "DATE: Sun, 05 Feb 2023 20:16:21 GMT\r\n");
}

TEST_F(HttpBasicFTestSuite, make_message_format_d_successful) {
    // format type 'd':  arg = int number  -- appends decimal number

    // Test Unit
    EXPECT_EQ(http_MakeMessage(&m_request, 1, 1, "d", 123456), 0);
    EXPECT_STREQ(m_request.buf, "123456");
}

TEST_F(HttpBasicFTestSuite, make_message_format_G_successful) {
    // format type 'G':  arg = range information  -- add range header

    // From */upnp.hpp to function 'UpnpOpenHttpGetEx()':
    // Gets specified number of bytes from a file specified in the URL.
    // The number of bytes is specified through a low count and a high count
    // which are passed as a range of bytes for the request.

    SendInstruction rangeBuf;
    constexpr int lowRange{2}; // Maybe an empty file with "\r\n"
    constexpr int highRange{32768};

    // Provide a rangeBuf (what ever it is ?-: )
    // Taken from */httpreadwrite.cpp function 'http_OpenHttpGetEx()'
    memset(&rangeBuf, 0, sizeof(rangeBuf));
    int rc = snprintf(rangeBuf.RangeHeader, sizeof(rangeBuf.RangeHeader),
                      "Range: bytes=%d-%d\r\n", lowRange, highRange);
    if (rc < 0 || (unsigned int)rc >= sizeof(rangeBuf.RangeHeader))
        GTEST_FAIL();

    // Test Unit
    EXPECT_EQ(http_MakeMessage(&m_request, 1, 1, "G", &rangeBuf), 0);
    EXPECT_STREQ(m_request.buf, "Range: bytes=2-32768\r\n");
}

TEST_F(HttpBasicFTestSuite, make_message_format_h_successful) {
    // format type 'h':  arg = off_t number  -- appends off_t number

    // One more will compile to -Werror=overflow
    constexpr off_t bignum{1073741824 + 1073741823};

    // Test Unit
    EXPECT_EQ(http_MakeMessage(&m_request, 1, 1, "h", bignum), 0);
    EXPECT_STREQ(m_request.buf, "2147483647");
}

TEST_F(HttpBasicFTestSuite, make_message_format_K_successful) {
    // format type 'K':  (no args)  -- add chunky header

    // Test Unit
    EXPECT_EQ(http_MakeMessage(&m_request, 1, 1, "K"), 0);
    EXPECT_STREQ(m_request.buf, "TRANSFER-ENCODING: chunked\r\n");
}

TEST_F(HttpBasicFTestSuite, make_message_format_L_successful) {
    // format type 'L':  arg = language information
    //                   -- add Content-Language header only
    //                      if Accept-Language header is not empty and
    //                      if WEB_SERVER_CONTENT_LANGUAGE is not empty
    SendInstruction RespInstr;
    memset(&RespInstr, 0, sizeof(RespInstr));

    // Test Unit with default setting.
    int ret_http_MakeMessage =
        http_MakeMessage(&m_request, 1, 1, "L", &RespInstr);
    EXPECT_EQ(ret_http_MakeMessage, 0) << errStrEx(ret_http_MakeMessage, 0);
    // This does not return a UPnP header entry.
    EXPECT_EQ(m_request.buf, nullptr);

    // Test Unit with only content language set.
    web_server_content_language = "en";

    ret_http_MakeMessage = http_MakeMessage(&m_request, 1, 1, "L", &RespInstr);
    EXPECT_EQ(ret_http_MakeMessage, 0) << errStrEx(ret_http_MakeMessage, 0);
    // This does not return a UPnP header entry.
    EXPECT_EQ(m_request.buf, nullptr);

    // RFC 9110: 12.5.4. Accept-Language
    strcpy(RespInstr.AcceptLanguageHeader, // buffer size is 200
           "en, de-DE;q=0.8, de;q=0.7");

    // Test Unit
    web_server_content_language.clear();

    ret_http_MakeMessage = http_MakeMessage(&m_request, 1, 1, "L", &RespInstr);
    EXPECT_EQ(ret_http_MakeMessage, 0) << errStrEx(ret_http_MakeMessage, 0);
    // This does not return a UPnP header entry.
    EXPECT_EQ(m_request.buf, nullptr);

    // Test Unit with all needed information set.
    web_server_content_language = "en";

    ret_http_MakeMessage = http_MakeMessage(&m_request, 1, 1, "L", &RespInstr);
    EXPECT_EQ(ret_http_MakeMessage, 0) << errStrEx(ret_http_MakeMessage, 0);
    EXPECT_STREQ(m_request.buf, "CONTENT-LANGUAGE: en\r\n");
}

TEST_F(HttpBasicFTestSuite, make_message_format_N_successful) {
    // format type 'N':  arg1 = off_t content_length  -- content-length header

    // One more will compile to -Werror=overflow
    constexpr off_t bignum{1073741824 + 1073741823};

    // Test Unit
    EXPECT_EQ(http_MakeMessage(&m_request, 1, 1, "N", bignum), 0);
    EXPECT_STREQ(m_request.buf,
                 "CONTENT-LENGTH: 2147483647\r\nAccept-Ranges: bytes\r\n");
}

TEST_F(HttpBasicFTestSuite, make_message_format_Q_successful) {
    // format type 'Q':  arg1 = http_method_t;  -- start line of request
    //                   arg2 = char* urlpath; (not complete url)
    //                   arg3 = size_t urlpath_length

    // Provide only path from e.g.
    // "https://user.name@www.upnplib.net:443/path/dest/?query=value#fragment";
    constexpr char urlpath[]{"/path/dest/?query=value#fragment"};

    // Test Unit
    EXPECT_EQ(http_MakeMessage(&m_request, 1, 1, "Q", HTTPMETHOD_GET, urlpath,
                               sizeof(urlpath)),
              0);
    EXPECT_STREQ(m_request.buf, "GET /path/dest/?query=value#fragment");
}

TEST_F(HttpBasicFTestSuite, make_message_format_q_successful) {
    // format type 'q':  arg1 = http_method_t  -- request start line
    //                                            and HOST header
    //                   arg2 = (uri_type *)

    // Provide a url structure
    uri_type url;
    // I use an ip address. With a name a DNS server would be asked and that
    // fails with test conditions.
#ifdef UPnPsdk_HAVE_OPENSSL
    constexpr char url_str[]{"https://192.168.192.170:443/path/dest/"
                             "?query=value#fragment"};
#else
    constexpr char url_str[]{"http://192.168.192.171:80/path/dest/"
                             "?query=value#fragment"};
#endif
    ASSERT_EQ(http_FixStrUrl(url_str, strlen(url_str), &url), 0);

    // Test Unit
    int ret_http_MakeMessage{UPNP_E_INTERNAL_ERROR};
    ret_http_MakeMessage =
        http_MakeMessage(&m_request, 1, 1, "q", HTTPMETHOD_GET, &url);
    EXPECT_EQ(ret_http_MakeMessage, 0) << errStrEx(ret_http_MakeMessage, 0);
#ifdef UPnPsdk_HAVE_OPENSSL
    EXPECT_STREQ(m_request.buf, "GET /path/dest/?query=value HTTP/1.1\r\nHOST: "
                                "192.168.192.170:443\r\n");
#else
    EXPECT_STREQ(
        m_request.buf,
        "GET /path/dest/?query=value HTTP/1.1\r\nHOST: 192.168.192.171:80\r\n");
#endif
}

TEST_F(HttpBasicFTestSuite, make_message_format_R_successful) {
    // format type 'R':  arg = int status_code  -- adds a response start line

    // Test Unit
    EXPECT_EQ(http_MakeMessage(&m_request, 1, 1, "R", HTTP_OK), 0);
    EXPECT_STREQ(m_request.buf, "HTTP/1.1 200 OK\r\n");
}

TEST_F(HttpBasicFTestSuite, make_message_format_S_successful) {
    // format type 'S':  (no args)  -- appends HTTP SERVER: header

#ifdef _WIN32
    // Test Unit
    EXPECT_EQ(http_MakeMessage(&m_request, 1, 1, "S"), 0);

    EXPECT_THAT(m_request.buf,
                MatchesStdRegex("SERVER: UPnP/1.0, Portable SDK for UPnP "
                                "devices/" PUPNP_VERSION " ?on windows\r\n"));

#else

    // Provide structure for 'uname()' with system info
    utsname sysinf{};
    strncpy(sysinf.sysname, "Linux", sizeof(sysinf.sysname) - 1);
    strncpy(sysinf.release, "5.10.0-20-amd64", sizeof(sysinf.release) - 1);

    // Mock uname system info
    umock::SysinfoMock sysinfoObj;
    umock::Sysinfo sysinfo_injectObj(&sysinfoObj);
    EXPECT_CALL(sysinfoObj, uname(_))
        .WillOnce(DoAll(StructCpyToArg<0>(&sysinf, sizeof(sysinf)), Return(0)));

    // Test Unit
    EXPECT_EQ(http_MakeMessage(&m_request, 1, 1, "S"), 0);

    std::string result = "SERVER: Linux/5.10.0-20-amd64, UPnP/1.0, Portable "
                         "SDK for UPnP devices/" PUPNP_VERSION "\r\n";
    EXPECT_EQ(std::string(m_request.buf), result);
#endif
}

TEST_F(HttpBasicFTestSuite, make_message_format_T_successful) {
    // format type 'T':  arg = char* content_type;  -- add content-type header,
    //                                                 format e.g: "text/html";

    // Test Unit
    EXPECT_EQ(http_MakeMessage(&m_request, 1, 1, "T", "text/html"), 0);
    EXPECT_STREQ(m_request.buf, "CONTENT-TYPE: text/html\r\n");
}

TEST_F(HttpBasicFTestSuite, make_message_format_U_successful) {
    // This is the same as with format type 'S' except that the HTTP header
    // field starts with "USER-AGENT: " instead of "SERVER: ". Just tested only
    // this difference.

    // Test Unit
    EXPECT_EQ(http_MakeMessage(&m_request, 1, 1, "U"), 0);

#ifdef _WIN32
    EXPECT_THAT(m_request.buf,
                MatchesStdRegex("USER-AGENT: UPnP/1.0, Portable SDK for UPnP "
                                "devices/" PUPNP_VERSION " ?on windows\r\n"));
#else
    EXPECT_THAT(
        m_request.buf,
        MatchesStdRegex("USER-AGENT: .*/.*, UPnP/1.0, Portable SDK for UPnP "
                        "devices/" PUPNP_VERSION "\r\n"));
#endif
}


// Tests for the HTTP version number
// ---------------------------------

TEST_F(HttpBasicFTestSuite, make_message_format_C_invalid_version) {
    // format type 'C':  (no args)  -- appends a HTTP CONNECTION: close header
    //                                 depending on major, minor version.

    // Test Unit
    EXPECT_EQ(http_MakeMessage(&m_request, 0, 0, "C"), 0);
    EXPECT_EQ(m_request.buf, nullptr);
}

TEST_F(HttpBasicFTestSuite, make_message_format_Q_invalid_version) {
    // format type 'Q':  arg1 = http_method_t;  -- start line of request
    //                   arg2 = char* urlpath; (not complete url)
    //                   arg3 = size_t urlpath_length

    // Provide only path from e.g.
    // "https://user.name@www.upnplib.net:443/path/dest/?query=value#fragment"};
    constexpr char urlpath[]{"/path/dest/?query=value#fragment"};

    // Test Unit
    EXPECT_EQ(http_MakeMessage(&m_request, 0, 0, "Q", HTTPMETHOD_GET, urlpath,
                               sizeof(urlpath)),
              0);
    EXPECT_STREQ(m_request.buf, "GET /path/dest/?query=value#fragment");
}

TEST_F(HttpBasicFTestSuite, make_message_get_sdk_info_successful) {
    char info[128];

    // Test Unit
    get_sdk_info(info, sizeof(info));

    if (old_code) {
        std::cout
            << CYEL "[ BUGFIX   ] " CRES << __LINE__
            << ": UPnP header string should not miss a spaceon windows.\n";
    }

#ifdef _WIN32
    if (old_code) {
        EXPECT_STREQ((char*)info,
                     "UPnP/1.0, Portable SDK for UPnP devices/" PUPNP_VERSION
                     "on windows\r\n"); // Wrong! There is a space missing

    } else {

        EXPECT_STREQ((char*)info,
                     "UPnP/1.0, Portable SDK for UPnP devices/" PUPNP_VERSION
                     " on windows\r\n");
    }
#else
    EXPECT_THAT((char*)info,
                MatchesStdRegex(".*/.*, UPnP/1.0, Portable SDK for UPnP "
                                "devices/" PUPNP_VERSION "\r\n"));
#endif
}

#ifndef _WIN32
TEST_F(HttpBasicFTestSuite, make_message_get_sdk_info_system_info_fails) {
    char info[128];

    // Provide invalid structure for 'uname()'
    utsname sysinf;
    constexpr size_t sysinf_size{sizeof(sysinf)};
    memset(&sysinf, 0xAA, sysinf_size);
    // To be on the safe side we will have a defined end for strings.
    memset((unsigned char*)&sysinf + sysinf_size - 1, '\0', 1);

    // Mock 'uname()' system info to fail with invalid utsname structure. errno
    // returns 'EFAULT buf is not valid'.
    umock::SysinfoMock sysinfoObj;
    umock::Sysinfo sysinfo_injectObj(&sysinfoObj);
    EXPECT_CALL(sysinfoObj, uname(_))
        .WillOnce(DoAll(StructCpyToArg<0>(&sysinf, sizeof(sysinf)),
                        SetErrnoAndReturn(EFAULT, -1)));

    // Test Unit
    get_sdk_info(info, sizeof(info));

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Failing system info must not return memory garbage.\n";
        EXPECT_EQ(strncmp(info,
                          "\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA"
                          "\xAA\xAA\xAA",
                          16),
                  0); // Wrong!
    } else {

        EXPECT_STREQ((char*)info,
                     "Unspecified, UPnP/1.0, Portable SDK for UPnP "
                     "devices/" PUPNP_VERSION "\r\n");
    }
}
#endif


// Testsuite for http_SendMessage()
// ================================

#ifdef SO_NOSIGPIPE // this seems to be defined on MacOS
// Subroutine for multiple check of expectations.
void chk_sigpipe(umock::Sys_socketMock& m_sys_socketObj, SOCKINFO& info) {
    if (old_code) { // Scope InSequence
        InSequence seq;
        // Mock getsockopt(), get old option SIGPIPE
        EXPECT_CALL(m_sys_socketObj,
                    getsockopt(info.socket, SOL_SOCKET, SO_NOSIGPIPE, _, _))
            .WillOnce(DoAll(SetArgPtrIntValue<3>(0xAA55), Return(0)));
        // Mock setsockopt(), set new option SIGPIPE
        EXPECT_CALL(m_sys_socketObj,
                    setsockopt(info.socket, SOL_SOCKET, SO_NOSIGPIPE,
                               PointeeVoidToConstInt(1), _));
        // Mock setsockopt(), restore old option SIGPIPE
        EXPECT_CALL(m_sys_socketObj,
                    setsockopt(info.socket, SOL_SOCKET, SO_NOSIGPIPE,
                               PointeeVoidToConstInt(0xAA55), _));
    } // End scope InSequence
}
#endif

TEST_F(HttpMockFTestSuite, http_send_message_from_buffer_successful) {
    SOCKINFO info{};
    info.socket = umock::sfd_base + 51;
    int timeout_secs{HTTP_DEFAULT_TIMEOUT};
    constexpr char request_out[]{"Test content from buffer."};
    constexpr size_t request_length{sizeof(request_out) - 1};
    SSockaddr saObj;
    saObj = "[::1]:50077";

    // Mock select()
    EXPECT_CALL(m_sys_socketObj,
                select(info.socket + 1, _, NotNull(), NULL, NotNull()))
        .WillOnce(Return(1)); // send from buffer successful

    if (!old_code) {
        // Mock getsockopt()
        EXPECT_CALL(m_sys_socketObj, getsockopt(_, SOL_SOCKET, SO_ERROR, _, _))
            .Times(3)
            .WillRepeatedly(Return(0));
        // Mock getsockname()
        if (g_dbug)
            EXPECT_CALL(m_sys_socketObj,
                        getsockname(info.socket, _,
                                    Pointee(Ge(static_cast<socklen_t>(
                                        sizeof(saObj.ss))))))
                .Times(3)
                .WillRepeatedly(
                    DoAll(StructCpyToArg<1>(&saObj.ss, sizeof(saObj.ss)),
                          SetArgPointee<2>(saObj.sizeof_saddr()), Return(0)));
    }
    // Mock send()
    EXPECT_CALL(m_sys_socketObj,
                send(info.socket, request_out, request_length, _))
        .WillOnce(Return((SSIZEP_T)request_length));

#ifdef SO_NOSIGPIPE // this seems to be defined on MacOS
    {
        SCOPED_TRACE("");
        chk_sigpipe(m_sys_socketObj, info);
    }
#endif
    // Test Unit, send from buffer successful
    int ret_http_SendMessage = http_SendMessage(&info, &timeout_secs, "b",
                                                request_out, request_length);
    EXPECT_EQ(ret_http_SendMessage, UPNP_E_SUCCESS)
        << errStrEx(ret_http_SendMessage, UPNP_E_SUCCESS);

    // Test Unit, send from buffer with buffer length = 0
    ret_http_SendMessage =
        http_SendMessage(&info, &timeout_secs, "b", request_out, 0);
    EXPECT_EQ(ret_http_SendMessage, UPNP_E_SUCCESS)
        << errStrEx(ret_http_SendMessage, UPNP_E_SUCCESS);

    // Test Unit, only giving send instruction
    SendInstruction instr{};

    ret_http_SendMessage = http_SendMessage(&info, &timeout_secs, "I", &instr);
    EXPECT_EQ(ret_http_SendMessage, UPNP_E_SUCCESS)
        << errStrEx(ret_http_SendMessage, UPNP_E_SUCCESS);
}

TEST_F(HttpMockFTestSuite, http_send_message_from_file_successful) {
    if (old_code) {
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": Reading a file should be error checked.\n";
    }
    constexpr char file_name[]{"./mocked/message.txt"};
    constexpr char file_content[]{"Test content from file."};
    FILE fd{};

    // Mock fopen()
#ifdef _WIN32
    EXPECT_CALL(m_stdioObj, fopen_s(_, StrEq(file_name), StrEq("rb")))
        // Open with Instruction ("If").
        .WillOnce(DoAll(SetArgPointee<0>(&fd), Return(0)))
        // Open with Instruction ("If"), but ReadSendSize = 0.
        .WillOnce(DoAll(SetArgPointee<0>(&fd), Return(0)))
        // Open without Instruction ("f").
        .WillOnce(DoAll(SetArgPointee<0>(nullptr), Return(EINVALP)))
        // Open with Instruction ("If") but fails with fopen_s() on old_code and
        // with read() on new code.
        .WillOnce(DoAll(SetArgPointee<0>(old_code ? nullptr : &fd),
                        Return(old_code ? EINVALP : 0)));
#else
    EXPECT_CALL(m_stdioObj, fopen(StrEq(file_name), StrEq("rb")))
        // Open with Instruction ("If").
        .WillOnce(Return(&fd))
        // Open with Instruction ("If"), but ReadSendSize = 0.
        .WillOnce(Return(&fd))
        // Open without Instruction ("f").
        .WillOnce(Return(nullptr))
        // Open with Instruction ("If") but fails with fopen() on old_code and
        // with read() on new code.
        .WillOnce(Return(old_code ? nullptr : &fd));
#endif
    // Mock fread()
    EXPECT_CALL(m_stdioObj, fread(_, 1, _, _))
        .Times(old_code ? 1 : 2)
        .WillRepeatedly(
            DoAll(StrCpyToArg<0>(file_content), Return(sizeof(file_content))));
    // Mock fclose()
    EXPECT_CALL(m_stdioObj, fclose(&fd)).Times(old_code ? 2 : 3);

    // Provide needed data.
    SOCKINFO info{};
    info.socket = umock::sfd_base + 53;
    int timeout_secs{HTTP_DEFAULT_TIMEOUT};
    SendInstruction instr{};
    instr.ReadSendSize = sizeof(file_content);
    SSockaddr saObj;
    saObj = "[::1]:50079";

    // Mock select()
    EXPECT_CALL(m_sys_socketObj,
                select(info.socket + 1, NotNull(), NotNull(), NULL, NotNull()))
        .WillOnce(Return(1)); // send from buffer successful

    if (!old_code) {
        // Mock getsockopt()
        EXPECT_CALL(m_sys_socketObj, getsockopt(_, SOL_SOCKET, SO_ERROR, _, _))
            .Times(4)
            .WillRepeatedly(Return(0));
        // Mock getsockname()
        if (g_dbug)
            EXPECT_CALL(m_sys_socketObj,
                        getsockname(info.socket, _,
                                    Pointee(Ge(static_cast<socklen_t>(
                                        sizeof(saObj.ss))))))
                .Times(4)
                .WillRepeatedly(
                    DoAll(StructCpyToArg<1>(&saObj.ss, sizeof(saObj.ss)),
                          SetArgPointee<2>(saObj.sizeof_saddr()), Return(0)));
        // Mock ferror()
        EXPECT_CALL(m_stdioObj, ferror(&fd))
            .WillOnce(Return(0))
            .WillOnce(Return(1));
        // Mock clearerr()
        EXPECT_CALL(m_stdioObj, clearerr(&fd)).Times(old_code ? 0 : 1);
    }
    // Mock send()
    EXPECT_CALL(m_sys_socketObj, send(info.socket, _, _, _))
        .WillOnce(Return((SSIZEP_T)sizeof(file_content)));

#ifdef SO_NOSIGPIPE // this seems to be defined on MacOS
    {
        SCOPED_TRACE("");
        chk_sigpipe(m_sys_socketObj, info);
    }
#endif
    // Test Unit, send from file successful.
    int ret_http_SendMessage =
        http_SendMessage(&info, &timeout_secs, "If", &instr, file_name);
    EXPECT_EQ(ret_http_SendMessage, UPNP_E_SUCCESS)
        << errStrEx(ret_http_SendMessage, UPNP_E_SUCCESS);

    // Test Unit, send from file with file length = 0.
    instr.ReadSendSize = 0;
    ret_http_SendMessage =
        http_SendMessage(&info, &timeout_secs, "If", &instr, file_name);
    EXPECT_EQ(ret_http_SendMessage, UPNP_E_SUCCESS)
        << errStrEx(ret_http_SendMessage, UPNP_E_SUCCESS);

    // Test Unit, send from file without instruction.
    ret_http_SendMessage =
        http_SendMessage(&info, &timeout_secs, "f", file_name);
    EXPECT_EQ(ret_http_SendMessage, UPNP_E_FILE_READ_ERROR)
        << errStrEx(ret_http_SendMessage, UPNP_E_FILE_READ_ERROR);

    // Test Unit, try to send from file but mocked file read error.
    instr.ReadSendSize = sizeof(file_content);
    ret_http_SendMessage =
        http_SendMessage(&info, &timeout_secs, "If", &instr, file_name);
    EXPECT_EQ(ret_http_SendMessage, UPNP_E_FILE_READ_ERROR)
        << errStrEx(ret_http_SendMessage, UPNP_E_FILE_READ_ERROR);
}

#if 0
TEST_F(HttpMockFTestSuite, http_send_message_fails) {
    SOCKINFO info{};
    info.socket = umock::sfd_base + n;
    int timeout_secs{HTTP_DEFAULT_TIMEOUT};
    constexpr char request[]{"This is a UPnP Request."};
    constexpr size_t request_length{sizeof(request) - 1};

    // Mock select()
    EXPECT_CALL(m_sys_socketObj,
                select(info.socket + 1, NotNull(), NotNull(), NULL, NotNull()))
        .WillOnce(Return(1))  // 1) successful
        .WillOnce(Return(0)); // 2) timeout
    // Mock send()
    EXPECT_CALL(m_sys_socketObj, send(info.socket, request, request_length, _))
        .WillOnce(Return((SSIZEP_T)request_length));

    // 1) Test Unit, http_send_message_successful
    int ret_http_SendMessage =
        http_SendMessage(&info, &timeout_secs, "b", request, request_length);
    ASSERT_EQ(ret_http_SendMessage, UPNP_E_SUCCESS)
        << errStrEx(ret_http_SendMessage, UPNP_E_SUCCESS);

    // 2) Test Unit, send message with timeout of select
    ret_http_SendMessage =
        http_SendMessage(&info, &timeout_secs, "b", request, request_length);
    ASSERT_EQ(ret_http_SendMessage, UPNP_E_SUCCESS)
        << errStrEx(ret_http_SendMessage, UPNP_E_SUCCESS);
}
#endif

TEST_F(HttpBasicFTestSuite, http_send_message_without_socket_file_descriptor) {
    if (github_actions)
        GTEST_SKIP()
            << "Test needs to be completed after revision of test_sock.cpp.";

    SOCKINFO info{};
    int timeout_secs{HTTP_DEFAULT_TIMEOUT};
    constexpr char request[]{
        "Try to send message without socket file descriptor."};
    constexpr size_t request_length{sizeof(request) - 1};

    // Mock select()
    // EXPECT_CALL(m_sys_socketObj,
    //            select(info.socket + 1, NotNull(), NotNull(), NULL,
    //            NotNull()))
    // .WillOnce(SetErrnoAndReturn(EBADF, -1)); // Bad file descriptor
    // Mock send()
    // EXPECT_CALL(m_sys_socketObj, send(_, _, _, _)).Times(0);

    // Test Unit
    int ret_http_SendMessage =
        http_SendMessage(&info, &timeout_secs, "b", request, request_length);
    EXPECT_EQ(ret_http_SendMessage, UPNP_E_SOCKET_ERROR)
        << errStrEx(ret_http_SendMessage, UPNP_E_SOCKET_ERROR);
}

TEST_F(HttpMockFTestSuite, request_and_response_successful) {
    if (github_actions)
        GTEST_SKIP() << "Test needs to be completed after testing subroutines.";

    uri_type url;
    http_parser_t response;

    // Test Unit
    int ret_http_RequestAndResponse = http_RequestAndResponse(
        &url, m_request.buf, m_request.length, HTTPMETHOD_GET,
        HTTP_DEFAULT_TIMEOUT, &response);
    EXPECT_EQ(ret_http_RequestAndResponse, UPNP_E_TIMEDOUT)
        << errStrEx(ret_http_RequestAndResponse, UPNP_E_SUCCESS);

    httpmsg_destroy(&response.msg);
}

TEST_F(HttpMockFTestSuite, http_Download_successful) {
    if (github_actions)
        GTEST_SKIP() << "Test needs to be completed after testing subroutines.";

    const char url[]{"http://127.0.0.1:50001/tvdevicedesc.xml"};
    char* outBuf;
    size_t doc_length;
    char contentType[LINE_SIZE];

    // Test Unit
    int ret_http_Download = http_Download(url, HTTP_DEFAULT_TIMEOUT, &outBuf,
                                          &doc_length, contentType);
    EXPECT_EQ(ret_http_Download, UPNP_E_TIMEDOUT)
        << errStrEx(ret_http_Download, UPNP_E_SUCCESS);
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
