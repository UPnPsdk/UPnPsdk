// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-04-07

// Include source code for testing. So we have also direct access to static
// functions which need to be tested.
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
#include <Pupnp/upnp/src/ssdp/ssdp_server.cpp>
#else
#include <Compa/src/ssdp/ssdp_ctrlpt.cpp>
#endif

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

        // initialize needed global variables
        memset(&gIF_NAME, 0, sizeof(gIF_NAME));
        memset(&gIF_IPV4, 0, sizeof(gIF_IPV4));
        memset(&gIF_IPV4_NETMASK, 0, sizeof(gIF_IPV4_NETMASK));
        memset(&gIF_IPV6, 0, sizeof(gIF_IPV6));
        gIF_IPV6_PREFIX_LENGTH = 0;
        memset(&gIF_IPV6_ULA_GUA, 0, sizeof(gIF_IPV6_ULA_GUA));
        gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
        gIF_INDEX = unsigned(-1);

        // Destroy global variables to avoid side effects.
        // memset(&UpnpSdkInit, 0xAA, sizeof(UpnpSdkInit));
        memset(&errno, 0xAA, sizeof(errno));
        memset(&GlobalHndRWLock, 0xAA, sizeof(GlobalHndRWLock));
        // memset(&gWebMutex, 0xAA, sizeof(gWebMutex));
        // memset(&gUUIDMutex, 0xAA, sizeof(gUUIDMutex));
        // memset(&GlobalClientSubscribeMutex, 0xAA,
        //        sizeof(GlobalClientSubscribeMutex));
        memset(&gUpnpSdkNLSuuid, 0, sizeof(gUpnpSdkNLSuuid));
        // memset(&HandleTable, 0xAA, sizeof(HandleTable));
        memset(&gSendThreadPool, 0xAA, sizeof(gSendThreadPool));
        memset(&gRecvThreadPool, 0xAA, sizeof(gRecvThreadPool));
        memset(&gMiniServerThreadPool, 0xAA, sizeof(gMiniServerThreadPool));
        memset(&gTimerThread, 0xAA, sizeof(gTimerThread));
        memset(&bWebServerState, 0xAA, sizeof(bWebServerState));
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

void InitMiniServerSockArray(MiniServerSockArray* miniSocket) {
    miniSocket->miniServerSock4 = INVALID_SOCKET;
    miniSocket->miniServerSock6 = INVALID_SOCKET;
    miniSocket->miniServerSock6UlaGua = INVALID_SOCKET;
    miniSocket->miniServerStopSock = INVALID_SOCKET;
    miniSocket->ssdpSock4 = INVALID_SOCKET;
    miniSocket->ssdpSock6 = INVALID_SOCKET;
    miniSocket->ssdpSock6UlaGua = INVALID_SOCKET;
    miniSocket->stopPort = 0u;
    miniSocket->miniServerPort4 = 0u;
    miniSocket->miniServerPort6 = 0u;
    miniSocket->miniServerPort6UlaGua = 0u;
#ifdef INCLUDE_CLIENT_APIS
    miniSocket->ssdpReqSock4 = INVALID_SOCKET;
    miniSocket->ssdpReqSock6 = INVALID_SOCKET;
#endif /* INCLUDE_CLIENT_APIS */
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

TEST_F(SsdpMockFTestSuite, set_ipv4_socket_no_blocking_fails) {
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
    std::cout << CRED "[ FIXIT    ] " CRES << __LINE__
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
        EXPECT_THAT(captureObj.str(), HasSubstr("UPnPsdk MSG1090 CRIT  ["));

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

TEST_F(SsdpFTestSuite, get_ssdp_sockets_for_ipv4_local) {
    // Provide needed data.
    MiniServerSockArray mini_sock;
    InitMiniServerSockArray(&mini_sock);
    std::strcpy(gIF_IPV4, "127.0.0.1");

    // Test Unit
    int ret_get_ssdp_socket = get_ssdp_sockets(&mini_sock);
    EXPECT_EQ(ret_get_ssdp_socket, UPNP_E_SUCCESS)
        << errStrEx(ret_get_ssdp_socket, UPNP_E_SUCCESS);
}

TEST_F(SsdpFTestSuite, get_ssdp_sockets_for_ipv6_lla_local) {
    GTEST_SKIP()
        << CRED "[ FIXIT    ] " CRES << __LINE__
        << ": Unit uses multicast group FF02::C (Link-Local (2)). Must be "
           "FF01::C (Interface-Local (1)) for localhost (::1)";

    // Provide needed data.
    MiniServerSockArray mini_sock;
    InitMiniServerSockArray(&mini_sock);
    std::strcpy(gIF_IPV6, "::1");

    // Test Unit
    int ret_get_ssdp_socket = get_ssdp_sockets(&mini_sock);
    EXPECT_EQ(ret_get_ssdp_socket, UPNP_E_SUCCESS)
        << errStrEx(ret_get_ssdp_socket, UPNP_E_SUCCESS);
}

TEST_F(SsdpFTestSuite, get_ssdp_sockets_for_ipv6_gua_local) {
    GTEST_SKIP() << CRED "[ FIXIT    ] " CRES << __LINE__
                 << ": Unit fails with valid IPv6 ULA address by selecting a "
                    "\"garbage\" multicast group.";

    // Provide needed data.
    MiniServerSockArray mini_sock;
    InitMiniServerSockArray(&mini_sock);
    // Locally generated ULA ('fd') with '9e:21a7:a92c' 40-bit Site-ID, and
    // '2323' 16-bit Subnet-ID.
    std::strcpy(gIF_IPV6_ULA_GUA, "fd9e:21a7:a92c:2323::1");

    // Test Unit
    int ret_get_ssdp_socket = get_ssdp_sockets(&mini_sock);
    EXPECT_EQ(ret_get_ssdp_socket, UPNP_E_SUCCESS)
        << errStrEx(ret_get_ssdp_socket, UPNP_E_SUCCESS);
}

TEST_F(SsdpMockFTestSuite, get_ssdp_sockets_for_ipv6_lla) {
    std::cout << CRED "[ FIXIT    ] " CRES << __LINE__
              << ": mocking setsockopt() fails on macOS.\n";
#ifdef __APPLE__
    GTEST_SKIP();
#endif

    SOCKET sock6_reqest{1003}; // mocked ssdp request socket
    SOCKET sock6_bind{1004};   // mocked ssdp socket

    EXPECT_CALL(m_sys_socketObj, socket(AF_INET6, SOCK_DGRAM, 0))
        .WillOnce(Return(sock6_reqest))
        .WillOnce(Return(sock6_bind));

    // Manage IPv6 ssdp request socket 'sock6_reqest'
    // ----------------------------------------------
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
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sock6_bind, SOL_SOCKET, SO_REUSEADDR, _, _))
        .WillOnce(Return(0));
#if (defined(BSD) && !defined(__GNU__)) || defined(__APPLE__)
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sock6_bind, SOL_SOCKET, SO_REUSEPORT, _, _))
        .WillOnce(Return(0));
#endif /* BSD, __APPLE__ */
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sock6_bind, IPPROTO_IPV6, IPV6_V6ONLY, _, _))
        .WillOnce(Return(0));
    EXPECT_CALL(m_sys_socketObj, bind(sock6_bind, _, _)).WillOnce(Return(0));
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sock6_bind, IPPROTO_IPV6, IPV6_JOIN_GROUP, _, _))
        .WillOnce(Return(0));
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sock6_bind, SOL_SOCKET, SO_BROADCAST, _, _))
        .WillOnce(Return(0));

    // Provide needed data.
    MiniServerSockArray mini_sock;
    InitMiniServerSockArray(&mini_sock);
    std::strcpy(gIF_IPV6, "fe80::55");
    // std::strcpy(gIF_IPV6_ULA_GUA, "2001:db8::55");

    // Test Unit
    int ret_get_ssdp_socket = get_ssdp_sockets(&mini_sock);
    EXPECT_EQ(ret_get_ssdp_socket, UPNP_E_SUCCESS)
        << errStrEx(ret_get_ssdp_socket, UPNP_E_SUCCESS);
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
