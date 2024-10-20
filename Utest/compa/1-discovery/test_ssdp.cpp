// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-08-18

// Include source code for testing. So we have also direct access to static
// functions which need to be tested.

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


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
