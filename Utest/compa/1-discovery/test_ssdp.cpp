// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-10-24

// Include source code for testing. So we have also direct access to static
// functions which need to be tested.
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
#include <Pupnp/upnp/src/ssdp/ssdp_server.cpp>
#else
#include <Compa/src/ssdp/ssdp_ctrlpt.cpp>
#endif

#include <UPnPsdk/global.hpp>
#include <UPnPsdk/upnptools.hpp> // for errStrEx

#include <pupnp/upnpdebug.hpp>

#include <umock/sys_socket_mock.hpp>
#include <umock/pupnp_sock_mock.hpp>
#include <umock/unistd_mock.hpp>

#include <utest/utest.hpp>


namespace utest {

using ::UPnPsdk::errStrEx;
using ::UPnPsdk::g_dbug;

using ::testing::_;
using ::testing::HasSubstr;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetErrnoAndReturn;
using ::testing::StrictMock;


// ssdp_server TestSuite
// =====================
class SsdpFTestSuite : public ::testing::Test {
  protected:
    pupnp::CLogging logObj; // Output only with build type DEBUG.

    // Constructor
    SsdpFTestSuite() {
        if (g_dbug)
            logObj.enable(UPNP_ALL);

        // Clean up needed global environment
        memset(&gIF_NAME, 0, sizeof(gIF_NAME));
        memset(&gIF_IPV4, 0, sizeof(gIF_IPV4));
        memset(&gIF_IPV4_NETMASK, 0, sizeof(gIF_IPV4_NETMASK));
        memset(&gIF_IPV6, 0, sizeof(gIF_IPV6));
        gIF_IPV6_PREFIX_LENGTH = 0;
        memset(&gIF_IPV6_ULA_GUA, 0, sizeof(gIF_IPV6_ULA_GUA));
        gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
        gIF_INDEX = unsigned(-1);
        memset(&errno, 0xAA, sizeof(errno));
    }
};

class SsdpMockFTestSuite : public SsdpFTestSuite {
  protected:
    // Instantiate mocking objects.
    StrictMock<umock::Sys_socketMock> m_sys_socketObj;
    // Inject the mocking objects into the tested code.
    umock::Sys_socket sys_socket_injectObj{&m_sys_socketObj};

    StrictMock<umock::PupnpSockMock> m_pupnpSockObj;
    umock::PupnpSock pupnp_sock_injectObj{&m_pupnpSockObj};

    StrictMock<umock::UnistdMock> m_unistdObj;
    umock::Unistd unistd_injectObj{&m_unistdObj};

    // Constructor
    SsdpMockFTestSuite() {
        // Set default socket object values
        ON_CALL(m_sys_socketObj, socket(_, _, _))
            .WillByDefault(SetErrnoAndReturn(EACCES, SOCKET_ERROR));
        ON_CALL(m_sys_socketObj, bind(_, _, _))
            .WillByDefault(SetErrnoAndReturn(EBADF, SOCKET_ERROR));
        ON_CALL(m_sys_socketObj, listen(_, _))
            .WillByDefault(SetErrnoAndReturn(EBADF, SOCKET_ERROR));
        ON_CALL(m_sys_socketObj, select(_, _, _, _, _))
            .WillByDefault(SetErrnoAndReturn(EBADF, SOCKET_ERROR));
        ON_CALL(m_sys_socketObj, getsockopt(_, _, _, _, _))
            .WillByDefault(SetErrnoAndReturn(EBADF, SOCKET_ERROR));
        ON_CALL(m_sys_socketObj, setsockopt(_, _, _, _, _))
            .WillByDefault(SetErrnoAndReturn(EBADF, SOCKET_ERROR));
        ON_CALL(m_sys_socketObj, getsockname(_, _, _))
            .WillByDefault(SetErrnoAndReturn(EBADF, SOCKET_ERROR));

        ON_CALL(m_pupnpSockObj, sock_make_no_blocking(_))
            .WillByDefault(SetErrnoAndReturn(EBADF, -1));
        ON_CALL(m_pupnpSockObj, sock_make_blocking(_))
            .WillByDefault(SetErrnoAndReturn(EBADF, -1));

        ON_CALL(m_unistdObj, CLOSE_SOCKET_P(_))
            .WillByDefault(SetErrnoAndReturn(EBADF, -1));
    }
};

// Because we have a void pointer (const void* optval) we cannot direct use it
// in a matcher to get the pointed value. We must cast the type with this custom
// matcher to use the pointer.
MATCHER_P(IsIpMulticastTtl, value, "") {
    uint8_t ip_ttl = *reinterpret_cast<const uint8_t*>(arg);
    if (ip_ttl == value)
        return true;

    *result_listener << "pointing to " << std::to_string(ip_ttl);
    return false;
}


TEST_F(SsdpMockFTestSuite, create_sock_reqv4_successful) {
    // Steps as given by the Unit and expected results:
    // 1. get a socket succeeds
    // 2. set socket option IP_MULTICAST_TTL succeeds
    // 3. set socket no blocking succeeds
    // 4. close socket on error not called

    // Due to mocking network connections we don't need real values.
    strcpy(gIF_IPV4, "192.168.192.168");
    SOCKET sockfd{1001};  // mocked socket file descriptor
    const uint8_t ttl{4}; // The ip multicast ttl that is expected to set.

    SOCKET ssdpSock{INVALID_SOCKET}; // buffer to get the socket fd

    // Provide a socket id to the Unit
    EXPECT_CALL(m_sys_socketObj, socket(AF_INET, SOCK_DGRAM, 0))
        .WillOnce(Return(sockfd));

    // Expect socket option
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL,
                           IsIpMulticastTtl(ttl), sizeof(ttl)))
        .WillOnce(Return(0));

    // Unblock connection, means don't wait on connect and return
    // immediately.
    EXPECT_CALL(m_pupnpSockObj, sock_make_no_blocking(sockfd))
        .WillOnce(Return(0));

    // Mock close socket
    EXPECT_CALL(m_unistdObj, CLOSE_SOCKET_P(_)).Times(0);

    // Test Unit
    int ret_create_ssdp_sock_reqv4{UPNP_E_INTERNAL_ERROR};
    EXPECT_EQ(ret_create_ssdp_sock_reqv4 = create_ssdp_sock_reqv4(&ssdpSock),
              UPNP_E_SUCCESS)
        << errStrEx(ret_create_ssdp_sock_reqv4, UPNP_E_SUCCESS);
    EXPECT_EQ(ssdpSock, sockfd);
}

TEST_F(SsdpMockFTestSuite, set_socket_no_blocking_fails) {
    // Steps as given by the Unit and expected results:
    // 1. get a socket succeeds
    // 2. set socket option IP_MULTICAST_TTL succeeds
    // 3. set socket no blocking fails
    // 4. close socket on error succeeds

    // Due to mocking network connections we don't need real values.
    strcpy(gIF_IPV4, "192.168.192.169");
    SOCKET sockfd{1002};  // mocked socket file descriptor
    const uint8_t ttl{4}; // The ip multicast ttl that is expected to set.

    SOCKET ssdpSock{INVALID_SOCKET}; // buffer to get the socket fd

    // Provide a socket id to the Unit
    EXPECT_CALL(m_sys_socketObj, socket(AF_INET, SOCK_DGRAM, 0))
        .WillOnce(Return(sockfd));
    // Expect socket option
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL,
                           IsIpMulticastTtl(ttl), sizeof(ttl)))
        .WillOnce(Return(0));
    // Unblock connection, means don't wait on connect and return immediately.
    // sock_make_no_blocking() returns on fail with SOCKET_ERROR.
    EXPECT_CALL(m_pupnpSockObj, sock_make_no_blocking(sockfd))
        .WillOnce(Return(SOCKET_ERROR));

    // Test Unit
    std::cout << CYEL "[ FIXIT    ] " CRES << __LINE__
              << ": create_ssdp_sock_reqv4() must not ignore failed setting of "
                 "\"no blocking\" mode.\n";
    int ret_create_ssdp_sock_reqv4{UPNP_E_INTERNAL_ERROR};

    if (old_code) {
        ret_create_ssdp_sock_reqv4 = create_ssdp_sock_reqv4(&ssdpSock);

        EXPECT_EQ(ret_create_ssdp_sock_reqv4, UPNP_E_SUCCESS) // Wrong!
            << errStrEx(ret_create_ssdp_sock_reqv4, UPNP_E_SUCCESS);
        EXPECT_EQ(ssdpSock, 1002);                            // Wrong!

    } else {

        CaptureStdOutErr captureObj(STDERR_FILENO); // or STDOUT_FILENO
        captureObj.start();
        ret_create_ssdp_sock_reqv4 = create_ssdp_sock_reqv4(&ssdpSock);
        std::cout << captureObj.str();
        EXPECT_THAT(captureObj.str(), HasSubstr("] CRITICAL MSG1029: "));

        // This should be ...
        // EXPECT_EQ(ret_create_ssdp_sock_reqv4, UPNP_E_SOCKET_ERROR)
        //     << errStrEx(ret_create_ssdp_sock_reqv4, UPNP_E_SOCKET_ERROR);
        // EXPECT_EQ(ssdpSock, INVALID_SOCKET);

        // ... but this is due to compatibility.
        EXPECT_EQ(ret_create_ssdp_sock_reqv4, UPNP_E_SUCCESS)
            << errStrEx(ret_create_ssdp_sock_reqv4, UPNP_E_SUCCESS);
        EXPECT_EQ(ssdpSock, 1002);
    }
}

TEST_F(SsdpMockFTestSuite, get_ssdp_sockets) {
    SOCKET sock6_reqest{1003}; // mocked ssdp request socket
    SOCKET sock6_bind{1004};   // mocked ssdp socket

    // Manage IPv6 ssdp request socket 'sock6_reqest'
    // ----------------------------------------------
    EXPECT_CALL(m_sys_socketObj, socket(AF_INET6, SOCK_DGRAM, 0))
        .WillOnce(Return(sock6_reqest))
        .WillOnce(Return(sock6_bind));
    EXPECT_CALL(m_sys_socketObj, setsockopt(sock6_reqest, IPPROTO_IPV6,
                                            IPV6_MULTICAST_HOPS, _, _))
        .WillOnce(Return(0));
#ifdef UPNP_MINISERVER_REUSEADDR
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sock6_reqest, SOL_SOCKET, SO_REUSEADDR, _, _))
        .WillOnce(Return(0));
#endif
#if (defined(BSD) && !defined(__GNU__)) || defined(__APPLE__)
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sock6_reqest, SOL_SOCKET, SO_REUSEPORT, _, _))
        .WillOnce(Return(0));
#endif /* BSD, __APPLE__ */
    EXPECT_CALL(m_pupnpSockObj, sock_make_no_blocking(sock6_reqest))
        .WillOnce(Return(0));

    // Manage IPv6 ssdp socket 'sock6_bind'
    // ------------------------------------
#if !defined(__APPLE__)
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sock6_bind, SOL_SOCKET, SO_REUSEADDR, _, _))
        .WillOnce(Return(0));
#endif
#if (defined(BSD) && !defined(__GNU__)) || defined(__APPLE__)
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sock6_bind, SOL_SOCKET, SO_REUSEPORT, _, _))
        // setsockopt(1004, 65535, 512, 0x7ff7b81f9ebc, 4)
        .WillOnce(Return(0));
#endif /* BSD, __APPLE__ */
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sock6_bind, IPPROTO_IPV6, IPV6_V6ONLY, _, _))
        // setsockopt(sock6_bind, 41, 27, _, _))
        .WillOnce(Return(0));
    EXPECT_CALL(m_sys_socketObj, bind(sock6_bind, _, _)).WillOnce(Return(0));
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sock6_bind, IPPROTO_IPV6, IPV6_JOIN_GROUP, _, _))
        // setsockopt(1004, 41, 12, 0x7ff7b9629ec8, 20)
        .WillOnce(Return(0));
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sock6_bind, SOL_SOCKET, SO_BROADCAST, _, _))
        // setsockopt(1004, 65535, 32, 0x7ff7b1ab4ebc, 4)
        .WillOnce(Return(0));

    // Provide needed data.
    MiniServerSockArray mini_sock{};
    std::strcpy(gIF_IPV6, "2001:db8::55");
    // std::strcpy(gIF_IPV6_ULA_GUA, "fe80::55");

    // Test Unit
    int ret_get_ssdp_socket = get_ssdp_sockets(&mini_sock);
    EXPECT_EQ(ret_get_ssdp_socket, UPNP_E_SUCCESS)
        << errStrEx(ret_get_ssdp_socket, UPNP_E_SUCCESS);
}

} // namespace utest

#if false
#include <Pupnp/upnp/src/ssdp/ssdp_server.cpp>
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
#define NS
#else
#define NS ::compa
#include <compa/src/ssdp/ssdp_server.cpp>
#endif

#include <upnp.hpp> // for UPNP_E_* constants

#include <UPnPsdk/global.hpp>
#ifdef _MSC_VER
#include <UPnPsdk/synclog.hpp>
#endif
#include <UPnPsdk/upnptools.hpp> // for errStrEx

#include <utest/utest.hpp>
#include <umock/sys_socket_mock.hpp>
#include <umock/pupnp_sock_mock.hpp>
#include <umock/unistd_mock.hpp>

#include <string>


namespace utest {

using ::UPnPsdk::errStrEx;

using ::testing::_;
using ::testing::InSequence;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SetErrnoAndReturn;


// The ssdp_server call stack
//===========================
// This is a simpliefied pseudo call stack for overview:
/* clang-format off

01)  get_ssdp_sockets()
     |
#ifdef COMPA_HAVE_CTRLPT_SSDP
     |__ create_ssdp_sock_reqv4() // for SSDP REQUESTS
     |   |__ socket()                  // get a socket
     |   |__ setsockopt(.* IP_MULTICAST_TTL .*)
     |   |__ sock_make_no_blocking()
     |
     |__ create_ssdp_sock_reqv6() // for SSDP REQUESTS
     |   |__ socket()                  // get a socket
     |   |__ setsockopt(.* IPV6_MULTICAST_HOPS .*)
     |   |__ sock_make_no_blocking()
#endif
     |__ create_ssdp_sock_v4()         // for SSDP
     |   |__ socket()                  // get a socket
     |   |__ setsockopt(.* SO_REUSEADDR .*)
     | #if (defined(BSD) && !defined(__GNU__)) || defined(__APPLE__)
     |   |__ setsockopt(.* SO_REUSEPORT .*)
     | #endif
     |   |__ bind(.* INADDR_ANY + SSDP_PORT .*)
     |   |__ setsockopt(.* IP_ADD_MEMBERSHIP .*) // join multicast group
     |   |__ setsockopt(.* IP_MULTICAST_IF .*)
     |   |__ setsockopt(.* IP_MULTICAST_TTL .*)
     |   |__ setsockopt(.* SO_BROADCAST .*)
     |
     |__ create_ssdp_sock_v6()         // for SSDP
     |__ create_ssdp_sock_v6_ula_gua() // for SSDP

01) Creates the IPv4 and IPv6 ssdp sockets required by the control point and
    device operation.
clang-format on */


// ssdp_server TestSuite
// =====================
class SSDPserverFTestSuite : public ::testing::Test {
  protected:
    SSDPserverFTestSuite() {
#ifdef _MSC_VER
        // Initialize Windows sockets
        TRACE("SSDPserverFTestSuite: initialize Windows sockets")
        WSADATA wsaData;
        int rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (rc != NO_ERROR) {
            throw std::runtime_error(
                std::string("Failed to start Windows sockets (WSAStartup)."));
        }
#endif
        // Reset global variables
        memset(&gIF_NAME, 0, sizeof(gIF_NAME));
        memset(&gIF_IPV4, 0, sizeof(gIF_IPV4));
        memset(&gIF_IPV4_NETMASK, 0, sizeof(gIF_IPV4_NETMASK));
        memset(&gIF_IPV6, 0, sizeof(gIF_IPV6));
        gIF_IPV6_PREFIX_LENGTH = 0;
        memset(&gIF_IPV6_ULA_GUA, 0, sizeof(gIF_IPV6_ULA_GUA));
        gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
        gIF_INDEX = (unsigned)-1;
        memset(&errno, 0xAA, sizeof(errno));
    }

    ~SSDPserverFTestSuite() override {
#ifdef _WIN32
        // Cleanup Windows sochets
        WSACleanup();
#endif
    }
};
typedef SSDPserverFTestSuite CreateSSDPsockReqV4FTestSuite;
typedef SSDPserverFTestSuite CreateSSDPsockV4FTestSuite;


// Because we have a void pointer (const void* optval) we cannot direct use it
// in a matcher to get the pointed value. We must cast the type with this custom
// matcher to use the pointer.
MATCHER_P(IsIpMulticastTtl, value, "") {
    uint8_t ip_ttl = *reinterpret_cast<const uint8_t*>(arg);
    if (ip_ttl == value)
        return true;

    *result_listener << "pointing to " << std::to_string(ip_ttl);
    return false;
}


TEST_F(CreateSSDPsockReqV4FTestSuite, create_successful) {
    // Steps as given by the Unit and expected results:
    // 1. get a socket succeeds
    // 2. set socket option IP_MULTICAST_TTL succeeds
    // 3. set socket no blocking succeeds
    // 4. close socket on error not called

    // Due to mocking network connections we don't need real values.
    strcpy(gIF_IPV4, "192.168.192.168");
    SOCKET sockfd{1001};  // mocked socket file descriptor
    const uint8_t ttl{4}; // The ip multicast ttl that is expected to set.

    SOCKET ssdpSock;      // buffer to get the socket fd

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Debug UpnpPrintf() must use correct strerror_r() "
                     "function for error message.\n";
    }

    // Mock system socket functions
    umock::Sys_socketMock sys_socketObj;
    umock::Sys_socket sys_socket_injectObj(&sys_socketObj);
    // Provide a socket id to the Unit
    EXPECT_CALL(sys_socketObj, socket(AF_INET, SOCK_DGRAM, 0))
        .WillOnce(Return(sockfd));

    // Expect socket option
    EXPECT_CALL(sys_socketObj, setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL,
                                          IsIpMulticastTtl(ttl), sizeof(ttl)))
        .WillOnce(Return(0));

    // Unblock connection, means don't wait on connect and return
    // immediately.
    umock::PupnpSockMock pupnpSockObj;
    umock::PupnpSock pupnp_sock_injectObj(&pupnpSockObj);
    EXPECT_CALL(pupnpSockObj, sock_make_no_blocking(sockfd))
        .WillOnce(Return(0));

    // Mock close socket
    umock::UnistdMock unistdObj;
    umock::Unistd unistd_injectObj(&unistdObj);
    EXPECT_CALL(unistdObj, CLOSE_SOCKET_P(_)).Times(0);

    // Test Unit
    int ret_create_ssdp_sock_reqv4{UPNP_E_INTERNAL_ERROR};
    EXPECT_EQ(ret_create_ssdp_sock_reqv4 = create_ssdp_sock_reqv4(&ssdpSock),
              UPNP_E_SUCCESS)
        << errStrEx(ret_create_ssdp_sock_reqv4, UPNP_E_SUCCESS);
    EXPECT_EQ(ssdpSock, sockfd);
}

#if 0
TEST_F(CreateSSDPsockReqV4FTestSuite, set_socket_no_blocking_fails) {
    // Steps as given by the Unit and expected results:
    // 1. get a socket succeeds
    // 2. set socket option IP_MULTICAST_TTL succeeds
    // 3. set socket no blocking fails
    // 4. close socket on error succeeds

    // Due to mocking network connections we don't need real values.
    strcpy(gIF_IPV4, "192.168.192.169");
    SOCKET sockfd{1002};  // mocked socket file descriptor
    const uint8_t ttl{4}; // The ip multicast ttl that is expected to set.

    SOCKET ssdpSock{0xAAAA}; // buffer to get the socket fd

    // Mock system socket functions
    umock::Sys_socketMock sys_socketObj;
    umock::Sys_socket sys_socket_injectObj(&sys_socketObj);
    // Provide a socket id to the Unit
    EXPECT_CALL(sys_socketObj, socket(AF_INET, SOCK_DGRAM, 0))
        .WillOnce(Return(sockfd));
    // Expect socket option
    EXPECT_CALL(sys_socketObj,
                setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL,
                           IsIpMulticastTtl(ttl), sizeof(ttl)))
        .WillOnce(Return(0));
    // Unblock connection, means don't wait on connect and return
    // immediately.
    // sock_make_no_blocking() returns on fail with SOCKET_ERROR(-1) on WIN32
    // and with -1 otherwise.
    umock::PupnpSockMock pupnpSockObj;
    umock::PupnpSock pupnp_sock_injectObj(&pupnpSockObj);
    EXPECT_CALL(pupnpSockObj, sock_make_no_blocking(sockfd))
        .WillOnce(Return(-1));

    // Test Unit
    int ret_create_ssdp_sock_reqv4{UPNP_E_INTERNAL_ERROR};
    ret_create_ssdp_sock_reqv4 = create_ssdp_sock_reqv4(&ssdpSock);

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": create_ssdp_sock_reqv4() must not ignore failing set "
                     "of no blocking mode.\n";
        EXPECT_EQ(ret_create_ssdp_sock_reqv4, UPNP_E_SUCCESS); // Wrong!
        EXPECT_EQ(ssdpSock, 1002);                             // Wrong!

    } else {

        EXPECT_EQ(ret_create_ssdp_sock_reqv4, UPNP_E_SOCKET_ERROR)
            << errStrEx(ret_create_ssdp_sock_reqv4, UPNP_E_SOCKET_ERROR);
        EXPECT_EQ(ssdpSock, 0xAAAA);
    }
}
#endif

TEST(SsdpTestSuite, get_ssdp_sockets) {
    GTEST_SKIP() << "TODO: Test must be completed.";
#if 0
    { // begin scope InSequence
        InSequence seq;

        // Manage create_ssdp_sock_v6_ula_gua().
        EXPECT_CALL(m_sys_socketObj, socket(AF_INET6, SOCK_DGRAM, 0))
            .WillOnce(Return(ssdp_sockfd));
        EXPECT_CALL(m_sys_socketObj,
                    setsockopt(ssdp_sockfd, SOL_SOCKET, SO_REUSEADDR, _, _))
            .WillOnce(Return(0));
#if (defined(BSD) && !defined(__GNU__)) || defined(__APPLE__)
        EXPECT_CALL(m_sys_socketObj,
                    setsockopt(ssdp_sockfd, SOL_SOCKET, SO_REUSEPORT, _, _))
            .WillOnce(Return(0));
#endif /* BSD, __APPLE__ */
        EXPECT_CALL(m_sys_socketObj,
                    setsockopt(ssdp_sockfd, IPPROTO_IPV6, IPV6_V6ONLY, _, _))
            .WillOnce(Return(0));
        EXPECT_CALL(m_sys_socketObj, bind(ssdp_sockfd, _, _))
            .WillOnce(Return(0));
        EXPECT_CALL(m_sys_socketObj, setsockopt(ssdp_sockfd, IPPROTO_IPV6,
                                                IPV6_JOIN_GROUP, _, _))
            .WillOnce(Return(0));
        EXPECT_CALL(m_sys_socketObj,
                    setsockopt(ssdp_sockfd, SOL_SOCKET, SO_BROADCAST, _, _))
            .WillOnce(Return(0));

    } // end scope InSequence
#endif
    // Provide needed data.
    MiniServerSockArray mini_sock{};

    // Test Unit
    int ret_get_ssdp_socket = get_ssdp_sockets(&mini_sock);
    EXPECT_EQ(ret_get_ssdp_socket, UPNP_E_SUCCESS)
        << errStrEx(ret_get_ssdp_socket, UPNP_E_SUCCESS);
}

} // namespace utest
#endif


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
