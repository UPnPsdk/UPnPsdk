// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-09-19

// All functions of the miniserver module have been covered by a gtest. Some
// tests are skipped and must be completed when missed information is
// available.

// Include source code for testing. So we have also direct access to static
// functions which need to be tested.
#ifdef UPNPLIB_WITH_NATIVE_PUPNP
#include <pupnp/upnp/src/genlib/miniserver/miniserver.cpp>
#else
#include <compa/src/genlib/miniserver/miniserver.cpp>
#endif

#include <webserver.hpp>

#include <pupnp/upnpdebug.hpp>

#include <upnplib/upnptools.hpp> // for errStrEx
#include <upnplib/port.hpp>
#include <upnplib/sockaddr.hpp>
#include <upnplib/socket.hpp>
#include <upnplib/gtest.hpp>
#include <upnplib/addrinfo.hpp>

// #include <umock/pthread_mock.hpp>
#include <umock/sys_socket_mock.hpp>


namespace compa {
bool old_code{false}; // Managed in compa/gtest_main.inc
bool github_actions = std::getenv("GITHUB_ACTIONS");

using ::testing::_;
using ::testing::AnyOf;
using ::testing::DoAll;
using ::testing::Ge;
using ::testing::HasSubstr;
using ::testing::NotNull;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetErrnoAndReturn;
using ::testing::StartsWith;
using ::testing::StrictMock;

using ::pupnp::CLogging;

using ::upnplib::CAddrinfo;
using ::upnplib::CSocket_basic;
using ::upnplib::errStrEx;
using ::upnplib::SSockaddr_storage;

using ::upnplib::testing::CaptureStdOutErr;
using ::upnplib::testing::ContainsStdRegex;
using ::upnplib::testing::MatchesStdRegex;
using ::upnplib::testing::StrCpyToArg;
using ::upnplib::testing::StrnCpyToArg;


// The miniserver call stack to get a server socket
//=================================================
// clang-format off
/* This test suite follows the successful calls to get a server socket:
   StartMiniServer()
   |
   |__ InitMiniServerSockArray()                   ]
   |__ get_miniserver_sockets()                    ]
   |   |__ init_socket_suff()                      ] create sockets
   |   |   |__ get a socket()                      ]
   |   |   |__ setsockopt() - MINISERVER_REUSEADDR |
   |   |                                           V
   |   |__ do_bind_listen()
   |       |
   |       |__ do_bind()              ] bind socket to ip address
   |       |   |__ bind()             ]
   |       |
   |       |__ do_listen()            ]
   |       |   |__ listen()           ] listen on a port,
   |       |                          ] and wait for a connection
   |       |__ if EADDRINUSE          ]
   |       |      init_socket_suff()  ]
   |       |
   |       |__ get_port()             ] get the current port
   |           |__ getsockname()      ]
   |                                               A
   |__ get_miniserver_stopsock()                   | create sockets
   |__ get_ssdp_sockets()                          ]
   |
   |__ TPJobInit() to RunMiniServer()              ]
   |__ TPJobSetPriority()                          ] Add MiniServer
   |__ TPJobSetFreeFunction()                      ] to ThreadPool
   |__ ThreadPoolAddPersistent()                   ]
   |__ while ("wait for miniserver to start")

StartMiniServer() has started RunMiniServer() as thread. It then waits blocked
with select() undefinetly (timeout set to NULL) until it receive from any
selected socket. StartMiniServer() also has started a miniserver stopsock() on
another thread. This thread polls receive_from_stopSock() for an incomming
SOCK_DGRAM message "ShutDown" send to stopsock bound to "127.0.0.1". This will
be recieved by select() so it will always enable a blocking (waiting) select().

   RunMiniServer() as thread, started by StartMiniServer()
   |__ while(receive_from_stopSock() not "ShutDown")
   |   |__ select()
   |   |__ web_server_accept()
   |   |   |__ accept()
   |   |   |__ schedule_request_job()
   |   |       |__ TPJobInit() to handle_request()
   |   |
   |   |__ ssdp_read()
   |   |__ block until receive_from_stopSock()
   |
   |__ sock_close()
   |__ free()

   handle_request() as thread, started by schedule_request_job()
   |__ http_RecvMessage()
*/
// clang-format on

// Miniserver TestSuite
// ====================
class MiniServerFTestSuite : public ::testing::Test {
  protected:
    // clang-format off
    // Instantiate mocking objects.
    StrictMock<umock::Sys_socketMock> m_sys_socketObj;
    // Inject the mocking objects into the tested code.
    umock::Sys_socket sys_socket_injectObj = umock::Sys_socket(&m_sys_socketObj);
    // clang-format on
};

typedef MiniServerFTestSuite StartMiniServerFTestSuite;
typedef MiniServerFTestSuite DoBindFTestSuite;
typedef MiniServerFTestSuite StopMiniServerFTestSuite;
typedef MiniServerFTestSuite RunMiniServerFTestSuite;


// This test uses real connections and isn't portable. It is only for humans to
// see how it works and should not always enabled.
#if 0
TEST(StartMiniServerTestSuite, StartMiniServer_in_context) {
    // MINISERVER_REUSEADDR = false;
    // gIF_IPV4 = "";
    // LOCAL_PORT_V4 = 0;
    // LOCAL_PORT_V6 = 0;
    // LOCAL_PORT_V6_ULA_GUA = 0;

    // umock::Sys_socketMock sys_socketObj;
    // umock::Sys_socket sys_socket_injectObj(&sys_socketObj);
    // class Mock_sys_socket mocked_sys_socketObj;
    // class Mock_stdlib  mocked_stdlibObj;
    // class Mock_unistd mocked_unistdObj;

    // Perform initialization preamble.
    int ret_UpnpInitPreamble = UpnpInitPreamble();
    ASSERT_EQ(ret_UpnpInitPreamble, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInitPreamble, UPNP_E_SUCCESS);

    // Retrieve interface information (Addresses, index, etc).
    int ret_UpnpGetIfInfo = UpnpGetIfInfo(nullptr);
    ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

    // Due to initialization by components it should not have flagged to be
    // initialized. That will we do now.
    ASSERT_FALSE(MINISERVER_REUSEADDR);
    ASSERT_EQ(UpnpSdkInit, 0);
    ASSERT_STRNE(gIF_IPV4, "");
    UpnpSdkInit = 1;

    // Test Unit
    int ret_StartMiniServer =
        StartMiniServer(&LOCAL_PORT_V4, &LOCAL_PORT_V6, &LOCAL_PORT_V6_ULA_GUA);
    EXPECT_EQ(ret_StartMiniServer, UPNP_E_SUCCESS)
        << errStrEx(ret_StartMiniServer, UPNP_E_SUCCESS);

    ASSERT_FALSE(MINISERVER_REUSEADDR);
    EXPECT_EQ(UpnpSdkInit, 1);
    EXPECT_EQ(LOCAL_PORT_V4, APPLICATION_LISTENING_PORT);
    EXPECT_EQ(LOCAL_PORT_V6, 0);
    EXPECT_EQ(LOCAL_PORT_V6_ULA_GUA, 0);

    EXPECT_EQ(StopMiniServer(), 0); // Always returns 0

    int ret_UpnpFinish = UpnpFinish();
    EXPECT_EQ(ret_UpnpFinish, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpFinish, UPNP_E_SUCCESS);

    EXPECT_EQ(UpnpSdkInit, 0);
}
#endif


TEST(StartMiniServerTestSuite, start_miniserver_already_running) {
    MINISERVER_REUSEADDR = false;
    gMServState = MSERV_RUNNING;

    // Test Unit
    int ret_StartMiniServer =
        StartMiniServer(&LOCAL_PORT_V4, &LOCAL_PORT_V6, &LOCAL_PORT_V6_ULA_GUA);
    EXPECT_EQ(ret_StartMiniServer, UPNP_E_INTERNAL_ERROR)
        << errStrEx(ret_StartMiniServer, UPNP_E_INTERNAL_ERROR);
}

// Subroutine for multiple check of extended expectations.
void chk_minisocket(MiniServerSockArray& minisocket) {
    // EXPECT_EQ(miniSocket.miniServerSock4, sockfd);
    // EXPECT_EQ(miniSocket.miniServerPort4, APPLICATION_LISTENING_PORT);
    EXPECT_EQ(minisocket.miniServerSock6, INVALID_SOCKET);
    EXPECT_EQ(minisocket.miniServerSock6UlaGua, INVALID_SOCKET);
    EXPECT_EQ(minisocket.miniServerStopSock, INVALID_SOCKET);
    EXPECT_EQ(minisocket.ssdpSock4, INVALID_SOCKET);
    EXPECT_EQ(minisocket.ssdpSock6, INVALID_SOCKET);
    EXPECT_EQ(minisocket.ssdpSock6UlaGua, INVALID_SOCKET);
    EXPECT_EQ(minisocket.stopPort, 0u);
    EXPECT_EQ(minisocket.miniServerPort6, 0u);
    EXPECT_EQ(minisocket.miniServerPort6UlaGua, 0u);
    EXPECT_EQ(minisocket.ssdpReqSock4, INVALID_SOCKET);
    EXPECT_EQ(minisocket.ssdpReqSock6, INVALID_SOCKET);
}

TEST_F(StartMiniServerFTestSuite, get_miniserver_sockets) {
    WINSOCK_INIT

    // Initialize needed structure
    MINISERVER_REUSEADDR = false;
    strcpy(gIF_IPV4, "192.168.245.254");
    constexpr SOCKET sockfd{FD_SETSIZE - 10};
    const CAddrinfo ai(std::string(gIF_IPV4),
                       std::to_string(APPLICATION_LISTENING_PORT), AF_INET,
                       SOCK_STREAM, AI_NUMERICHOST | AI_NUMERICSERV);
    MiniServerSockArray miniSocket{};
    InitMiniServerSockArray(&miniSocket);

    // Provide a socket id
    EXPECT_CALL(m_sys_socketObj, socket(AF_INET, SOCK_STREAM, 0))
        .WillOnce(Return(sockfd));
    // Bind socket to an ip address (gIF_IPV4)
    EXPECT_CALL(m_sys_socketObj, bind(sockfd, _, _)).WillOnce(Return(0));
    // Query port from socket
    EXPECT_CALL(m_sys_socketObj,
                getsockname(sockfd, _, Pointee(Ge((socklen_t)ai->ai_addrlen))))
        .WillOnce(DoAll(SetArgPointee<1>(*ai->ai_addr), Return(0)));
    // Listen on the socket
    EXPECT_CALL(m_sys_socketObj, listen(sockfd, _)).WillOnce(Return(0));

    // Test Unit, needs initialized sockets on MS Windows
    int ret_get_miniserver_sockets =
        get_miniserver_sockets(&miniSocket, 0, 0, 0);
    EXPECT_EQ(ret_get_miniserver_sockets, UPNP_E_SUCCESS)
        << errStrEx(ret_get_miniserver_sockets, UPNP_E_SUCCESS);

    EXPECT_EQ(miniSocket.miniServerSock4, sockfd);
    EXPECT_EQ(miniSocket.miniServerPort4, APPLICATION_LISTENING_PORT);
    {
        SCOPED_TRACE("");
        chk_minisocket(miniSocket);
    }
    // Close socket
    // No need to close a socket because we haven't got a real socket. It was
    // mocked.
}

TEST(StartMiniServerTestSuite, get_miniserver_sockets_with_empty_ip_address) {
    // Using an empty text ip address gIF_IPV4 == "" will not bind to a valid
    // socket. TODO: With address 0, it would successful bind to all local ip
    // addresses. If this is intended then gIF_IPV4 should be set to "0.0.0.0".
    gIF_IPV4[0] = '\0'; // Empty ip address
    MINISERVER_REUSEADDR = false;

    // Initialize needed structure
    MiniServerSockArray miniSocket{};
    InitMiniServerSockArray(&miniSocket);

    // Due to unmocked bind() it returns successful with a valid ip address
    // instead of failing. Getting a valid ip address is possible because of
    // side effects from previous tests on the global variable gIF_IPV4. It
    // results in an endless loop with this test fixture. So we must have an
    // empty ip address.
    ASSERT_STREQ(gIF_IPV4, "");

    // Test Unit, needs initialized sockets on MS Windows (look at the fixture).
    int ret_get_miniserver_sockets =
        get_miniserver_sockets(&miniSocket, 0, 0, 0);

    if (old_code) {
        std::cout << CRED "[ BUG      ] " CRES << __LINE__
                  << ": Using empty IPv4 address with disabled IPv6 stack must "
                     "not succeed.\n";
#ifndef UPNP_ENABLE_IPV6
        // This isn't relevant for new code because there is IPv6 always
        // available.
        EXPECT_EQ(ret_get_miniserver_sockets, UPNP_E_SUCCESS)
            << errStrEx(ret_get_miniserver_sockets, UPNP_E_SUCCESS); // Wrong!
#else
        EXPECT_EQ(ret_get_miniserver_sockets, UPNP_E_OUTOF_SOCKET)
            << errStrEx(ret_get_miniserver_sockets, UPNP_E_OUTOF_SOCKET);
#endif
    } else {

        EXPECT_EQ(ret_get_miniserver_sockets, UPNP_E_OUTOF_SOCKET)
            << errStrEx(ret_get_miniserver_sockets, UPNP_E_OUTOF_SOCKET);
    }

    // We do not get a valid socket with an empty text ip address.
    EXPECT_EQ(miniSocket.miniServerSock4, INVALID_SOCKET);
    // It should return the 0 port.
    EXPECT_EQ(miniSocket.miniServerPort4, 0);
    {
        SCOPED_TRACE("");
        chk_minisocket(miniSocket);
    }
    // Close socket should fail, there is no valid socket.
    EXPECT_EQ(CLOSE_SOCKET_P(miniSocket.miniServerSock4), -1);
}

TEST(StartMiniServerTestSuite, get_miniserver_sockets_with_invalid_ip_address) {
    WINSOCK_INIT
    strcpy(gIF_IPV4, "192.168.129.XXX"); // Invalid ip address
    MINISERVER_REUSEADDR = false;

    // Initialize needed structure
    MiniServerSockArray miniSocket{};
    InitMiniServerSockArray(&miniSocket);

    // Test Unit, needs initialized sockets on MS Windows (look at the fixture).
    int ret_get_miniserver_sockets =
        get_miniserver_sockets(&miniSocket, 0, 0, 0);

    if (old_code) {
        std::cout
            << CRED "[ BUG      ] " CRES << __LINE__
            << ": Using invalid IPv4 address with disabled IPv6 stack must "
               "not succeed.\n";
#ifndef UPNP_ENABLE_IPV6
        // This isn't relevant for new code because there is IPv6 always
        // available.
        EXPECT_EQ(ret_get_miniserver_sockets, UPNP_E_SUCCESS)
            << errStrEx(ret_get_miniserver_sockets, UPNP_E_SUCCESS); // Wrong!
#else
        EXPECT_EQ(ret_get_miniserver_sockets, UPNP_E_OUTOF_SOCKET)
            << errStrEx(ret_get_miniserver_sockets, UPNP_E_OUTOF_SOCKET);
#endif
    } else {

        EXPECT_EQ(ret_get_miniserver_sockets, UPNP_E_OUTOF_SOCKET)
            << errStrEx(ret_get_miniserver_sockets, UPNP_E_OUTOF_SOCKET);
    }

    EXPECT_EQ(miniSocket.miniServerSock4, INVALID_SOCKET);
    EXPECT_EQ(miniSocket.miniServerPort4, 0);
    {
        SCOPED_TRACE("");
        chk_minisocket(miniSocket);
    }
    // Close socket should fail because there is no socket to close.
    EXPECT_EQ(CLOSE_SOCKET_P(miniSocket.miniServerSock4), -1);
}

TEST(StartMiniServerTestSuite, get_miniserver_sockets_uninitialized) {
    // MS Windows sockets are not initialized. It should never return a valid
    // socket and/or port.

    if (github_actions && !old_code)
        GTEST_SKIP() << "             known failing test on Github Actions";

    if (old_code) {
        std::cout
            << CYEL "[ BUGFIX   ] " CRES << __LINE__
            << ": Function should fail with win32 uninitialized sockets.\n";
    }

#ifdef _WIN32
    MINISERVER_REUSEADDR = false;
    strcpy(gIF_IPV4, "192.168.200.199");

    // Initialize needed structure
    MiniServerSockArray miniSocket{};
    InitMiniServerSockArray(&miniSocket);

    // Test Unit, needs initialized sockets on MS Windows
    int ret_get_miniserver_sockets =
        get_miniserver_sockets(&miniSocket, 0, 0, 0);

    if (old_code) {
        EXPECT_EQ(ret_get_miniserver_sockets, UPNP_E_OUTOF_SOCKET)
            << errStrEx(ret_get_miniserver_sockets, UPNP_E_OUTOF_SOCKET);

    } else {

        EXPECT_EQ(ret_get_miniserver_sockets, UPNP_E_INIT_FAILED)
            << errStrEx(ret_get_miniserver_sockets, UPNP_E_INIT_FAILED);
    }

    EXPECT_EQ(miniSocket.miniServerSock4, INVALID_SOCKET);
    EXPECT_EQ(miniSocket.miniServerPort4, 0);
    {
        SCOPED_TRACE("");
        chk_minisocket(miniSocket);
    }
    // Close socket
    EXPECT_EQ(CLOSE_SOCKET_P(miniSocket.miniServerSock4), -1);
#endif // _WIN32
}

TEST_F(StartMiniServerFTestSuite, get_miniserver_sockets_with_invalid_socket) {
    WINSOCK_INIT
    MINISERVER_REUSEADDR = false;
    strcpy(gIF_IPV4, "192.168.12.9");

    // Initialize needed structure
    MiniServerSockArray miniSocket{};
    InitMiniServerSockArray(&miniSocket);

    // Mock to get an invalid socket id
    EXPECT_CALL(m_sys_socketObj, socket(AF_INET, SOCK_STREAM, 0))
        .WillOnce(Return(INVALID_SOCKET));

    // Test Unit, needs initialized sockets on MS Windows
    int ret_get_miniserver_sockets =
        get_miniserver_sockets(&miniSocket, 0, 0, 0);

    if (old_code) {
        std::cout << CRED "[ BUG      ] " CRES << __LINE__
                  << ": Getting an invalid socket for IPv4 address with "
                     "disabled IPv6 stack must not succeed.\n";
#ifndef UPNP_ENABLE_IPV6
        // This isn't relevant for new code because there is IPv6 always
        // available.
        EXPECT_EQ(ret_get_miniserver_sockets, UPNP_E_SUCCESS)
            << errStrEx(ret_get_miniserver_sockets, UPNP_E_SUCCESS); // Wrong!
#else
        EXPECT_EQ(ret_get_miniserver_sockets, UPNP_E_OUTOF_SOCKET)
            << errStrEx(ret_get_miniserver_sockets, UPNP_E_OUTOF_SOCKET);
#endif
    } else {

        EXPECT_EQ(ret_get_miniserver_sockets, UPNP_E_OUTOF_SOCKET)
            << errStrEx(ret_get_miniserver_sockets, UPNP_E_OUTOF_SOCKET);
    }

    EXPECT_EQ(miniSocket.miniServerSock4, INVALID_SOCKET);
    EXPECT_EQ(miniSocket.miniServerPort4, 0);
    {
        SCOPED_TRACE("");
        chk_minisocket(miniSocket);
    }
}

TEST(StartMiniServerTestSuite, init_socket_suff) {
    WINSOCK_INIT
    MINISERVER_REUSEADDR = false;

    // Set ip address and needed structure
    constexpr char text_addr[]{"192.168.54.85"};
    char addrbuf[16];
    s_SocketStuff ss4;
    memset(&ss4, 0xAA, sizeof(ss4));

    // Test Unit, needs initialized sockets on MS Windows
    EXPECT_EQ(init_socket_suff(&ss4, text_addr, 4), 0);

    EXPECT_EQ(ss4.ip_version, 4);
    EXPECT_STREQ(ss4.text_addr, text_addr);
    EXPECT_EQ(ss4.serverAddr4->sin_family, AF_INET);
    EXPECT_STREQ(inet_ntop(AF_INET, &ss4.serverAddr4->sin_addr, addrbuf,
                           sizeof(addrbuf)),
                 text_addr);
    // Valid real socket
    EXPECT_NE(ss4.fd, INVALID_SOCKET);
    EXPECT_EQ(ss4.try_port, 0);
    EXPECT_EQ(ss4.actual_port, 0);
    EXPECT_EQ(ss4.address_len, (socklen_t)sizeof(*ss4.serverAddr4));

    char reuseaddr;
    socklen_t optlen{sizeof(reuseaddr)};
    EXPECT_EQ(getsockopt(ss4.fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, &optlen),
              0);
    EXPECT_FALSE(reuseaddr);

    // Close real socket
    EXPECT_EQ(CLOSE_SOCKET_P(ss4.fd), 0);
}

TEST(StartMiniServerTestSuite, init_socket_suff_reuseaddr) {
    WINSOCK_INIT
    MINISERVER_REUSEADDR = true;

    // Set ip address and needed structure
    constexpr char text_addr[]{"192.168.24.85"};
    s_SocketStuff ss4;

    // Test Unit, needs initialized sockets on MS Windows
    EXPECT_EQ(init_socket_suff(&ss4, text_addr, 4), 0);

    char reuseaddr;
    socklen_t optlen{sizeof(reuseaddr)};
    EXPECT_EQ(
        ::getsockopt(ss4.fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, &optlen), 0);
    EXPECT_TRUE(reuseaddr);

    // Important! Otherwise repeated tests will fail later because all file
    // descriptors for the process are consumed.
    EXPECT_EQ(CLOSE_SOCKET_P(ss4.fd), 0) << std::strerror(errno);
}

TEST_F(StartMiniServerFTestSuite, init_socket_suff_with_invalid_socket) {
    MINISERVER_REUSEADDR = false;

    // Set ip address and needed structure
    constexpr char text_addr[]{"192.168.99.85"};
    char addrbuf[16];
    s_SocketStuff ss4;

    // Mock to get an invalid socket id
    EXPECT_CALL(m_sys_socketObj, socket(AF_INET, SOCK_STREAM, 0))
        .WillOnce(SetErrnoAndReturn(EINVAL, INVALID_SOCKET));

    // Test Unit
    // On MS Windows we get error WSANOTINITIALISED = 10093 because
    // WSAGetLastError() is not mocked.
    EXPECT_THAT(init_socket_suff(&ss4, text_addr, 4), AnyOf(1, 10093));

    EXPECT_EQ(ss4.fd, INVALID_SOCKET);
    EXPECT_EQ(ss4.ip_version, 4);
    EXPECT_STREQ(ss4.text_addr, text_addr);
    EXPECT_EQ(ss4.serverAddr4->sin_family, AF_INET);
    EXPECT_EQ(ss4.address_len, 16);
    EXPECT_STREQ(inet_ntop(AF_INET, &ss4.serverAddr4->sin_addr, addrbuf,
                           sizeof(addrbuf)),
                 "192.168.99.85");
}

TEST(StartMiniServerTestSuite, init_socket_suff_with_invalid_ip_address) {
    MINISERVER_REUSEADDR = false;

    // Set ip address and needed structure
    constexpr char text_addr[]{
        "192.168.255.256"}; // invalid ip address with .256
    char addrbuf[16];
    s_SocketStuff ss4;

    // Test Unit, needs initialized sockets on MS Windows
    EXPECT_EQ(init_socket_suff(&ss4, text_addr, 4), 1);
    EXPECT_STREQ(ss4.text_addr, text_addr);
    EXPECT_EQ(ss4.fd, INVALID_SOCKET);
    EXPECT_EQ(ss4.ip_version, 4);
    EXPECT_EQ(ss4.serverAddr4->sin_family, AF_INET);
    EXPECT_EQ(ss4.address_len, 16);
    EXPECT_STREQ(inet_ntop(AF_INET, &ss4.serverAddr4->sin_addr, addrbuf,
                           sizeof(addrbuf)),
                 "0.0.0.0");
}

TEST(StartMiniServerTestSuite, init_socket_suff_with_invalid_ip_version) {
    // Set ip address and needed structure. There is no real network adapter on
    // this host with this ip address.
    constexpr char text_addr[]{"192.168.24.85"};
    char addrbuf[16];
    s_SocketStuff ss4;

    // Test Unit
    EXPECT_EQ(init_socket_suff(&ss4, text_addr, 0), 1);

    EXPECT_EQ(ss4.fd, INVALID_SOCKET);
    EXPECT_EQ(ss4.ip_version, 0);
    EXPECT_STREQ(ss4.text_addr, text_addr);
    EXPECT_EQ(ss4.serverAddr4->sin_family, 0);
    EXPECT_EQ(ss4.address_len, 0);
    EXPECT_STREQ(inet_ntop(AF_INET, &ss4.serverAddr4->sin_addr, addrbuf,
                           sizeof(addrbuf)),
                 "0.0.0.0");
}

TEST_F(StartMiniServerFTestSuite, do_bind_listen_successful) {
    // Configure expected system calls:
    // * Use fictive socket file descriptor 600
    // * Mocked bind() returns successful
    // * Mocked listen() returns successful
    // * Mocked getsockname() returns a sockaddr with current ip address and
    //   port
    WINSOCK_INIT

    MINISERVER_REUSEADDR = false;
    constexpr char text_addr[]{"192.168.54.188"};
    char addrbuf[16];
    constexpr SOCKET sockfd{FD_SETSIZE - 11};

    s_SocketStuff s;
    // Fill all fields of struct s_SocketStuff
    s.serverAddr = (struct sockaddr*)&s.ss;
    s.ip_version = 4;
    s.text_addr = text_addr;
    s.serverAddr4->sin_family = AF_INET;
    s.actual_port = 56789;
    s.serverAddr4->sin_port = htons(s.actual_port);
    inet_pton(AF_INET, text_addr, &s.serverAddr4->sin_addr);
    s.fd = sockfd;
    s.try_port = APPLICATION_LISTENING_PORT;
    s.address_len = sizeof(*s.serverAddr4);

    // If not mocked bind does not know the given ip address and fails.
    // The Unit will loop through all port numbers to find a free port
    // but will never find one. The program hungs in an endless loop.

    // Provide a sockaddr structure that will be returned by mocked
    // getsockname().
    const CAddrinfo ai(std::string(text_addr),
                       std::to_string(APPLICATION_LISTENING_PORT), AF_INET,
                       SOCK_STREAM, AI_NUMERICHOST | AI_NUMERICSERV);

    // Mock system functions
    EXPECT_CALL(m_sys_socketObj, bind(sockfd, _, _)).Times(1);
    EXPECT_CALL(m_sys_socketObj, listen(sockfd, SOMAXCONN)).Times(1);
    EXPECT_CALL(m_sys_socketObj,
                getsockname(sockfd, _, Pointee(Ge((socklen_t)ai->ai_addrlen))))
        .WillOnce(DoAll(SetArgPointee<1>(*ai->ai_addr), Return(0)));

    // Test Unit
    int ret_get_do_bind_listen = do_bind_listen(&s);
    EXPECT_EQ(ret_get_do_bind_listen, UPNP_E_SUCCESS)
        << errStrEx(ret_get_do_bind_listen, UPNP_E_SUCCESS);

    // Check all fields of struct s_SocketStuff
    EXPECT_EQ(s.ip_version, 4);
    EXPECT_STREQ(s.text_addr, text_addr);
    EXPECT_EQ(s.serverAddr4->sin_family, AF_INET);
    EXPECT_STREQ(
        inet_ntop(AF_INET, &s.serverAddr4->sin_addr, addrbuf, sizeof(addrbuf)),
        text_addr);
    EXPECT_EQ(ntohs(s.serverAddr4->sin_port), APPLICATION_LISTENING_PORT);
    EXPECT_EQ(s.fd, sockfd);
    EXPECT_EQ(s.try_port, APPLICATION_LISTENING_PORT + 1);
    EXPECT_EQ(s.actual_port, APPLICATION_LISTENING_PORT);
    EXPECT_EQ(s.address_len, (socklen_t)sizeof(*s.serverAddr4));
}

TEST(StartMiniServerTestSuite, do_bind_listen_with_wrong_socket) {
    WINSOCK_INIT

    MINISERVER_REUSEADDR = false;
    constexpr char text_addr[]{"0.0.0.0"};

    s_SocketStuff s;
    EXPECT_EQ(init_socket_suff(&s, text_addr, 4), 0);
    EXPECT_EQ(CLOSE_SOCKET_P(s.fd), 0) << std::strerror(errno);
    // The socket id wasn't got from a socket() call now and should trigger an
    // error.
    s.fd = FD_SETSIZE - 39;
    s.try_port = 65534;

    // Test Unit
    int ret_get_do_bind_listen = do_bind_listen(&s);
    EXPECT_EQ(ret_get_do_bind_listen, UPNP_E_SOCKET_BIND)
        << errStrEx(ret_get_do_bind_listen, UPNP_E_SOCKET_BIND);
}

TEST_F(StartMiniServerFTestSuite, do_bind_listen_with_failed_listen) {
    // Configure expected system calls:
    // * Use fictive socket file descriptor 600
    // * Mocked bind() returns successful
    // * Mocked listen() returns error

    MINISERVER_REUSEADDR = false;
    constexpr char text_addr[]{"192.168.54.188"};
    constexpr int actual_port{0};
    constexpr SOCKET sockfd{FD_SETSIZE - 12};

    s_SocketStuff s;
    // Fill all fields of struct s_SocketStuff
    s.serverAddr = (struct sockaddr*)&s.ss;
    s.ip_version = 4;
    s.text_addr = text_addr;
    s.serverAddr4->sin_family = AF_INET;
    s.actual_port = actual_port;
    s.serverAddr4->sin_port = htons(s.actual_port);
    inet_pton(AF_INET, text_addr, &s.serverAddr4->sin_addr);
    s.fd = sockfd;
    s.try_port = APPLICATION_LISTENING_PORT;
    s.address_len = sizeof(*s.serverAddr4);

    // Mock system functions
    EXPECT_CALL(m_sys_socketObj, socket(AF_INET, SOCK_STREAM, 0)).Times(0);
    EXPECT_CALL(m_sys_socketObj, bind(s.fd, _, _)).Times(1);
    EXPECT_CALL(m_sys_socketObj, listen(s.fd, SOMAXCONN))
        .WillOnce(SetErrnoAndReturn(EOPNOTSUPP, -1));

    // Test Unit
    int ret_get_do_bind_listen = do_bind_listen(&s);
    EXPECT_EQ(ret_get_do_bind_listen, UPNP_E_LISTEN)
        << errStrEx(ret_get_do_bind_listen, UPNP_E_LISTEN);

    // sock_close() is not needed because there is no socket called.
}

TEST_F(StartMiniServerFTestSuite, do_bind_listen_address_in_use) {
    // Configure expected system calls:
    // * Use fictive socket file descriptors with port 50024
    // * Mocked bind() returns successful
    // * Mocked listen() returns error with errno EADDRINUSE
    // * Mocked getsockname() returns a sockaddr with current ip address and
    //   port

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Unit should not loop through about 50000 ports to "
                     "find one free port.\n";
        // This very expensive behavior is skipped here and fixed in function
        // do_bind() of compa code.

    } else {

        WINSOCK_INIT

        MINISERVER_REUSEADDR = false;
        constexpr char text_addr[]{"192.168.54.188"};
        char addrbuf[16];
        constexpr int actual_port{50024};
        constexpr SOCKET sockfd_inuse{FD_SETSIZE - 13};
        constexpr SOCKET sockfd_free{FD_SETSIZE - 14};

        s_SocketStuff s;
        // Fill all fields of struct s_SocketStuff
        s.serverAddr = (struct sockaddr*)&s.ss;
        s.ip_version = 4;
        s.text_addr = text_addr;
        s.serverAddr4->sin_family = AF_INET;
        s.actual_port = actual_port;
        s.serverAddr4->sin_port = htons(s.actual_port);
        inet_pton(AF_INET, text_addr, &s.serverAddr4->sin_addr);
        s.fd = sockfd_inuse;
        s.try_port = actual_port + 1;
        s.address_len = sizeof(*s.serverAddr4);

        // Provide a sockaddr structure that will be returned by mocked
        // getsockname().
        const CAddrinfo ai(std::string(text_addr),
                           std::to_string(actual_port + 1), AF_INET,
                           SOCK_STREAM, AI_NUMERICHOST | AI_NUMERICSERV);

        // Mock system functions
        // A successful bind is expected but listen should fail with "address in
        // use"
        EXPECT_CALL(m_sys_socketObj, bind(sockfd_inuse, _, _)).Times(1);
        EXPECT_CALL(m_sys_socketObj, listen(sockfd_inuse, SOMAXCONN))
            .WillOnce(SetErrnoAndReturn(EADDRINUSE, SOCKET_ERROR));
        // A second attempt will call init_socket_suff() to get a new socket.
        EXPECT_CALL(m_sys_socketObj, socket(AF_INET, SOCK_STREAM, 0))
            .WillOnce(Return(sockfd_free));
        EXPECT_CALL(m_sys_socketObj, bind(sockfd_free, _, _)).Times(1);
        EXPECT_CALL(m_sys_socketObj, listen(sockfd_free, SOMAXCONN)).Times(1);
        EXPECT_CALL(
            m_sys_socketObj,
            getsockname(sockfd_free, _, Pointee(Ge((socklen_t)ai->ai_addrlen))))
            .WillOnce(DoAll(SetArgPointee<1>(*ai->ai_addr), Return(0)));

        // Test Unit
        int ret_get_do_bind_listen = do_bind_listen(&s);
        EXPECT_EQ(ret_get_do_bind_listen, UPNP_E_SUCCESS)
            << errStrEx(ret_get_do_bind_listen, UPNP_E_SUCCESS);

        // Check all fields of struct s_SocketStuff
        EXPECT_EQ(s.ip_version, 4);
        EXPECT_STREQ(s.text_addr, text_addr);
        EXPECT_EQ(s.serverAddr4->sin_family, AF_INET);
        EXPECT_STREQ(inet_ntop(AF_INET, &s.serverAddr4->sin_addr, addrbuf,
                               sizeof(addrbuf)),
                     text_addr);
        EXPECT_EQ(ntohs(s.serverAddr4->sin_port), actual_port + 1);
        EXPECT_EQ(s.fd, sockfd_free);
        EXPECT_EQ(s.try_port, actual_port + 2);
        EXPECT_EQ(s.actual_port, actual_port + 1);
        EXPECT_EQ(s.address_len, (socklen_t)sizeof(*s.serverAddr4));
    }
}

TEST_F(DoBindFTestSuite, bind_successful) {
    // Configure expected system calls:
    // * Use fictive socket file descriptor 511
    // * Actual used port is 56789
    // * Next port to try is 56790
    // * Mocked bind() returns successful

    // Provide needed data for the Unit
    constexpr SOCKET sockfd{FD_SETSIZE - 15};
    constexpr char text_addr[]{"192.168.101.233"};
    char addrbuf[16];
    constexpr uint16_t actual_port{56789};
    constexpr uint16_t try_port{56790};

    s_SocketStuff s;
    // Fill all fields of struct s_SocketStuff
    s.serverAddr = (struct sockaddr*)&s.ss;
    s.ip_version = 4;
    s.text_addr = text_addr;
    s.serverAddr4->sin_family = AF_INET;
    s.serverAddr4->sin_port = htons(actual_port);
    inet_pton(AF_INET, text_addr, &s.serverAddr4->sin_addr);
    s.fd = sockfd;
    s.try_port = try_port;
    s.actual_port = actual_port;
    s.address_len = sizeof(*s.serverAddr4);

    // Mock system functions
    EXPECT_CALL(m_sys_socketObj, socket(AF_INET, SOCK_STREAM, 0)).Times(0);
    EXPECT_CALL(m_sys_socketObj, bind(sockfd, _, _)).WillOnce(Return(0));

    // Test Unit
    errno = EINVAL; // Check if this has an impact.
    int ret_do_bind = do_bind(&s);
    EXPECT_EQ(ret_do_bind, UPNP_E_SUCCESS)
        << errStrEx(ret_do_bind, UPNP_E_SUCCESS);

    // Check all fields of struct s_SocketStuff
    EXPECT_EQ(s.ip_version, 4);
    EXPECT_STREQ(s.text_addr, text_addr);
    EXPECT_EQ(s.serverAddr4->sin_family, AF_INET);
    EXPECT_EQ(ntohs(s.serverAddr4->sin_port), actual_port + 1);
    EXPECT_STREQ(
        inet_ntop(AF_INET, &s.serverAddr4->sin_addr, addrbuf, sizeof(addrbuf)),
        text_addr);
    EXPECT_EQ(s.fd, sockfd);
    EXPECT_EQ(s.try_port, try_port + 1);
    EXPECT_EQ(s.address_len, (socklen_t)sizeof(*s.serverAddr4));

    if (old_code) {
        std::cout
            << CYEL "[ BUGFIX   ] " CRES << __LINE__
            << ": The actual_port number should be set to the new number.\n";
        EXPECT_EQ(s.actual_port, actual_port);

    } else {

        EXPECT_EQ(s.actual_port, actual_port + 1);
    }

    // sock_close() is not needed because there is no socket called.
}

TEST_F(DoBindFTestSuite, bind_with_invalid_argument) {
    // Configure expected system calls:
    // * Use fictive socket file descriptor 512
    // * Actual used port is 56890
    // * Next port to try is 56891
    // * Mocked bind() returns EINVAL

    // Provide needed data for the Unit
    constexpr SOCKET sockfd{FD_SETSIZE - 16};
    constexpr char text_addr[]{"192.168.202.233"};
    char addrbuf[16];
    constexpr uint16_t actual_port{56890};
    constexpr uint16_t try_port{56891};

    s_SocketStuff s;
    // Fill all fields of struct s_SocketStuff
    s.serverAddr = (struct sockaddr*)&s.ss;
    s.ip_version = 4;
    s.text_addr = text_addr;
    s.serverAddr4->sin_family = AF_INET;
    s.serverAddr4->sin_port = htons(actual_port);
    inet_pton(AF_INET, text_addr, &s.serverAddr4->sin_addr);
    s.fd = sockfd;
    s.try_port = try_port;
    s.actual_port = actual_port;
    s.address_len = sizeof(*s.serverAddr4);

    // Mock system functions
#ifdef _WIN32
    WSASetLastError(WSAEINVAL);
#endif

    if (old_code) {
        // If bind() always returns failure due to unchanged invalid argument
        // the Unit will hung in an endless loop. There is no exit for this
        // condition. Here it will only stop after three loops because bind()
        // returns successful at last. This is fixed in new code.
        EXPECT_CALL(m_sys_socketObj, bind(sockfd, _, _))
            .WillOnce(SetErrnoAndReturn(EINVAL, -1))
            .WillOnce(SetErrnoAndReturn(EINVAL, -1))
            .WillOnce(Return(0));

        // Test Unit
        // This wrong condition is expected if the code hasn't changed.
        int ret_do_bind = do_bind(&s);
        EXPECT_EQ(ret_do_bind, UPNP_E_SUCCESS)
            << errStrEx(ret_do_bind, UPNP_E_SUCCESS);

    } else {

        EXPECT_CALL(m_sys_socketObj, bind(sockfd, _, _))
            .WillOnce(SetErrnoAndReturn(EINVAL, -1));

        // Test Unit
        int ret_do_bind = do_bind(&s);
        EXPECT_EQ(ret_do_bind, UPNP_E_SOCKET_BIND)
            << errStrEx(ret_do_bind, UPNP_E_SOCKET_BIND);
    }

    // Check all fields of struct s_SocketStuff
    EXPECT_EQ(s.ip_version, 4);
    EXPECT_STREQ(s.text_addr, text_addr);
    EXPECT_EQ(s.serverAddr4->sin_family, AF_INET);
    EXPECT_STREQ(
        inet_ntop(AF_INET, &s.serverAddr4->sin_addr, addrbuf, sizeof(addrbuf)),
        text_addr);
    EXPECT_EQ(s.fd, sockfd);
    EXPECT_EQ(s.address_len, (socklen_t)sizeof(*s.serverAddr4));

    if (old_code) {
        // See notes above about the endless loop. Expected values here are
        // meaningless and only tested to watch code changes.
        std::cout
            << CYEL "[ BUGFIX   ] " CRES << __LINE__
            << ": do_bind() should never hung with testing all free ports "
               "before failing.\n";
        EXPECT_EQ(s.actual_port, actual_port);
        EXPECT_EQ(ntohs(s.serverAddr4->sin_port), actual_port + 3);
        EXPECT_EQ(s.try_port, try_port + 3);

    } else {

        EXPECT_EQ(s.actual_port, actual_port + 1);
        EXPECT_EQ(ntohs(s.serverAddr4->sin_port), actual_port + 1);
        EXPECT_EQ(s.try_port, try_port + 1);
    }
}

TEST_F(DoBindFTestSuite, bind_with_try_port_overrun) {
    // This setup will 'try_port' overrun after 65535 to 0. The overrun should
    // finish the search for a free port to bind.
    //
    // Configure expected system calls:
    // * Use fictive socket file descriptor 511
    // * Actual used port is 56789
    // * Next port to try is 65533
    // * Mocked bind() returns always failure with errno EINVAL

    // Provide needed data for the Unit
    constexpr SOCKET sockfd{FD_SETSIZE - 17};
    constexpr char text_addr[]{"192.168.101.233"};
    char addrbuf[16];

    s_SocketStuff s;
    // Fill all fields of struct s_SocketStuff
    s.serverAddr = (struct sockaddr*)&s.ss;
    s.ip_version = 4;
    s.text_addr = text_addr;
    s.serverAddr4->sin_family = AF_INET;
    s.serverAddr4->sin_port = htons(56789);
    inet_pton(AF_INET, text_addr, &s.serverAddr4->sin_addr);
    s.fd = sockfd;
    s.try_port = 65533;
    s.actual_port = 56789;
    s.address_len = sizeof(*s.serverAddr4);

    // Mock socket
    EXPECT_CALL(m_sys_socketObj, socket(AF_INET, SOCK_STREAM, 0)).Times(0);
    // Mock system function, must also set errno
    EXPECT_CALL(m_sys_socketObj, bind(sockfd, _, _))
        .Times(3)
        .WillRepeatedly(SetErrnoAndReturn(EADDRINUSE, -1));

    // Test Unit
    int ret_do_bind = do_bind(&s);
    EXPECT_EQ(ret_do_bind, UPNP_E_SOCKET_BIND)
        << errStrEx(ret_do_bind, UPNP_E_SOCKET_BIND);

    // Check all fields of struct s_SocketStuff
    EXPECT_EQ(s.ip_version, 4);
    EXPECT_STREQ(s.text_addr, text_addr);
    EXPECT_EQ(s.serverAddr4->sin_family, AF_INET);
    EXPECT_STREQ(
        inet_ntop(AF_INET, &s.serverAddr4->sin_addr, addrbuf, sizeof(addrbuf)),
        text_addr);
    EXPECT_EQ(ntohs(s.serverAddr4->sin_port), 65535);
    EXPECT_EQ(s.fd, sockfd);
    EXPECT_EQ(s.address_len, (socklen_t)sizeof(*s.serverAddr4));

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Next try to bind a port should start with "
                     "APPLICATION_LISTENING_PORT but not with port 0.\n";
        EXPECT_EQ(s.try_port, 0);
        std::cout
            << CYEL "[ BUGFIX   ] " CRES << __LINE__
            << ": The actual_port number should be set to the new number.\n";
        EXPECT_EQ(s.actual_port, 56789);

    } else {

        EXPECT_EQ(s.try_port, APPLICATION_LISTENING_PORT);
        EXPECT_EQ(s.actual_port, 65535);
    }
}

TEST_F(DoBindFTestSuite, bind_successful_with_two_tries) {
    // Configure expected system calls:
    // * Use fictive socket file descriptor 511
    // * Actual used port is 56789
    // * Next port to try is 65533
    // * Mocked bind() fails with two tries errno EADDRINUSE, then successful.

    // Provide needed data for the Unit
    constexpr SOCKET sockfd{FD_SETSIZE - 18};
    constexpr char text_addr[]{"192.168.101.233"};
    char addrbuf[16];
    s_SocketStuff s;

    // Fill all fields of struct s_SocketStuff
    s.serverAddr = (struct sockaddr*)&s.ss;
    s.ip_version = 4;
    s.text_addr = text_addr;
    s.serverAddr4->sin_family = AF_INET;
    s.serverAddr4->sin_port = htons(56789);
    inet_pton(AF_INET, text_addr, &s.serverAddr4->sin_addr);
    s.fd = sockfd;
    s.try_port = 65533;
    s.actual_port = 56789;
    s.address_len = sizeof(*s.serverAddr4);

    // Mock system functions
    EXPECT_CALL(m_sys_socketObj, socket(AF_INET, SOCK_STREAM, 0)).Times(0);
    EXPECT_CALL(m_sys_socketObj, bind(sockfd, _, _))
        .WillOnce(SetErrnoAndReturn(EADDRINUSE, -1))
        .WillOnce(SetErrnoAndReturn(EADDRINUSE, -1))
        // The system library never reset errno so don't do it here.
        // .WillOnce(SetErrnoAndReturn(0, 0));
        .WillOnce(Return(0));

    // Test Unit
    int ret_do_bind = do_bind(&s);

    if (old_code) {
        EXPECT_EQ(s.try_port, 0);

    } else {

        EXPECT_EQ(s.try_port, APPLICATION_LISTENING_PORT);
    }

    EXPECT_EQ(ret_do_bind, UPNP_E_SUCCESS)
        << errStrEx(ret_do_bind, UPNP_E_SUCCESS);

    // Check all fields of struct s_SocketStuff
    EXPECT_EQ(s.ip_version, 4);
    EXPECT_STREQ(s.text_addr, text_addr);
    EXPECT_EQ(s.serverAddr4->sin_family, AF_INET);
    EXPECT_STREQ(
        inet_ntop(AF_INET, &s.serverAddr4->sin_addr, addrbuf, sizeof(addrbuf)),
        text_addr);
    EXPECT_EQ(ntohs(s.serverAddr4->sin_port), 65535);
    EXPECT_EQ(s.fd, sockfd);
    EXPECT_EQ(s.address_len, (socklen_t)sizeof(*s.serverAddr4));

    if (old_code) {
        std::cout
            << CYEL "[ FIX      ] " CRES << __LINE__
            << ": The actual_port number should be set to the new number.\n";
        EXPECT_EQ(s.actual_port, 56789);

    } else {

        EXPECT_EQ(s.actual_port, 65535);
    }
}

TEST(DoBindTestSuite, bind_with_empty_parameter) {
    // With this test we have an initialized ip_version = 0, instead of valid 4
    // or 6. Switching for this value will never find an end.
    if (old_code) {
        std::cout << CYEL "[ FIX      ] " CRES << __LINE__
                  << ": This test stuck the program in an endless loop.\n";

    } else {

        s_SocketStuff s{};
        s.serverAddr = (sockaddr*)&s.ss;

        // Test Unit
        int ret_do_bind = do_bind(&s);

        EXPECT_EQ(ret_do_bind, UPNP_E_INVALID_PARAM)
            << errStrEx(ret_do_bind, UPNP_E_INVALID_PARAM);
    }
}

TEST(DoBindTestSuite, bind_with_wrong_ip_version_assignment) {
    // Setting ip_version = 6 and sin_family = AF_INET and vise versa does not
    // fit. Provide needed data for the Unit.
    constexpr SOCKET sockfd{FD_SETSIZE - 19};
    constexpr char text_addr[]{"192.168.101.233"};
    constexpr uint16_t try_port{65533};

    s_SocketStuff s;
    // Fill all fields of struct s_SocketStuff.
    // Set ip_version = 6 and sin_family = AF_INET
    s.serverAddr = (struct sockaddr*)&s.ss;
    s.ip_version = 6;
    s.text_addr = text_addr;
    s.serverAddr4->sin_family = AF_INET;
    s.actual_port = 56789;
    s.serverAddr4->sin_port = htons(s.actual_port);
    inet_pton(AF_INET, text_addr, &s.serverAddr4->sin_addr);
    s.fd = sockfd;
    s.try_port = try_port;
    s.address_len = sizeof(*s.serverAddr4);

    if (old_code) {
        // Starting from try_port this will loop until 65535 with always the
        // same error. The short running test here is only given because we
        // start with try_port = 65533 so we have only three attempts here.
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": This should not loop through all free port numbers. It "
                     "will always fail.\n";
    }

    // Test Unit
    int ret_do_bind = do_bind(&s);
    EXPECT_EQ(ret_do_bind, UPNP_E_SOCKET_BIND)
        << errStrEx(ret_do_bind, UPNP_E_SOCKET_BIND);

    // Set ip_version = 4 and sin_family = AF_INET6
    s.ip_version = 4;
    s.serverAddr4->sin_family = AF_INET6;
    s.try_port = try_port;

    // Test Unit
    ret_do_bind = do_bind(&s);
    EXPECT_EQ(ret_do_bind, UPNP_E_SOCKET_BIND)
        << errStrEx(ret_do_bind, UPNP_E_SOCKET_BIND);
}

TEST_F(StartMiniServerFTestSuite, do_listen_successful) {
    // Configure expected system calls:
    // * Use fictive socket file descriptor 512
    // * Actual used port 60000 will be set
    // * Next port to try is 0 because not used here
    // * Mocked listen() returns successful
    // * Mocked getsockname() returns successful
    WINSOCK_INIT

    // Provide needed data for the Unit
    constexpr SOCKET sockfd{FD_SETSIZE - 20};
    constexpr char text_addr[] = "192.168.202.233";
    char addrbuf[16];
    constexpr uint16_t actual_port{60000};
    constexpr uint16_t try_port{0}; // not used

    s_SocketStuff s;
    // Fill all fields of struct s_SocketStuff
    s.serverAddr = (sockaddr*)&s.ss;
    s.ip_version = 4;
    s.text_addr = text_addr;
    s.serverAddr4->sin_family = AF_INET;
    s.serverAddr4->sin_port = 0; // not used
    inet_pton(AF_INET, text_addr, &s.serverAddr4->sin_addr);
    s.fd = sockfd;
    s.try_port = try_port; // not used
    s.actual_port = 0;
    s.address_len = sizeof(*s.serverAddr4);

    // Provide a sockaddr structure that will be returned by mocked
    // getsockname().
    const CAddrinfo ai(std::string(text_addr), std::to_string(actual_port),
                       AF_INET, SOCK_STREAM, AI_NUMERICHOST | AI_NUMERICSERV);

    // Mock system functions
    EXPECT_CALL(m_sys_socketObj, socket(AF_INET, SOCK_STREAM, 0)).Times(0);
    EXPECT_CALL(m_sys_socketObj, listen(sockfd, SOMAXCONN)).Times(1);
    EXPECT_CALL(m_sys_socketObj,
                getsockname(sockfd, _, Pointee(Ge((socklen_t)ai->ai_addrlen))))
        .WillOnce(DoAll(SetArgPointee<1>(*ai->ai_addr), Return(0)));

    // Test Unit
    int ret_do_listen = do_listen(&s);
    EXPECT_EQ(ret_do_listen, UPNP_E_SUCCESS)
        << errStrEx(ret_do_listen, UPNP_E_SUCCESS);

    // Check all fields of struct s_SocketStuff
    EXPECT_EQ(s.ip_version, 4);
    EXPECT_STREQ(s.text_addr, text_addr);
    EXPECT_EQ(s.serverAddr4->sin_family, AF_INET);
    EXPECT_EQ(ntohs(s.serverAddr4->sin_port), 0); // not used
    EXPECT_STREQ(
        inet_ntop(AF_INET, &s.serverAddr4->sin_addr, addrbuf, sizeof(addrbuf)),
        text_addr);
    EXPECT_EQ(s.fd, sockfd);
    EXPECT_EQ(s.actual_port, actual_port);
    EXPECT_EQ(s.try_port, try_port); // not used
    EXPECT_EQ(s.address_len, (socklen_t)sizeof(*s.serverAddr4));
}

TEST_F(StartMiniServerFTestSuite, do_listen_not_supported) {
    // Configure expected system calls:
    // * Use fictive socket file descriptor 612
    // * Actual used port will not be set
    // * Next port to try is 0 because not used here
    // * Mocked listen() returns with EOPNOTSUPP
    // * Mocked getsockname() is not called

    // Provide needed data for the Unit
    constexpr SOCKET sockfd{FD_SETSIZE - 21};
    constexpr char text_addr[] = "192.168.101.203";
    char addrbuf[16];

    s_SocketStuff s;
    // Fill all fields of struct s_SocketStuff
    s.serverAddr = (struct sockaddr*)&s.ss;
    s.ip_version = 4;
    s.text_addr = text_addr;
    s.serverAddr4->sin_family = AF_INET;
    s.serverAddr4->sin_port = 0; // not used
    inet_pton(AF_INET, text_addr, &s.serverAddr4->sin_addr);
    s.fd = sockfd;
    s.try_port = 0; // not used
    s.actual_port = 0;
    s.address_len = sizeof(*s.serverAddr4);

    // Mock system functions
    EXPECT_CALL(m_sys_socketObj, socket(AF_INET, SOCK_STREAM, 0)).Times(0);
    EXPECT_CALL(m_sys_socketObj, listen(sockfd, SOMAXCONN))
        .WillOnce(SetErrnoAndReturn(EOPNOTSUPP, -1));
    EXPECT_CALL(m_sys_socketObj, getsockname(sockfd, _, _)).Times(0);

    // Test Unit
    int ret_do_listen = do_listen(&s);
    EXPECT_EQ(ret_do_listen, UPNP_E_LISTEN)
        << errStrEx(ret_do_listen, UPNP_E_LISTEN);

    // Check all fields of struct s_SocketStuff
    EXPECT_EQ(s.ip_version, 4);
    EXPECT_STREQ(s.text_addr, text_addr);
    EXPECT_EQ(s.serverAddr4->sin_family, AF_INET);
    EXPECT_EQ(ntohs(s.serverAddr4->sin_port), 0); // not used
    EXPECT_STREQ(
        inet_ntop(AF_INET, &s.serverAddr4->sin_addr, addrbuf, sizeof(addrbuf)),
        text_addr);
    EXPECT_EQ(s.fd, sockfd);
    EXPECT_EQ(s.actual_port, 0);
    EXPECT_EQ(s.try_port, 0); // not used
    EXPECT_EQ(s.address_len, (socklen_t)sizeof(*s.serverAddr4));
}

TEST_F(StartMiniServerFTestSuite, do_listen_insufficient_resources) {
    // Configure expected system calls:
    // * Use fictive socket file descriptor 512
    // * Actual used port will not be set
    // * Next port to try is 0 because not used here
    // * Mocked listen() returns successful
    // * Mocked getsockname() returns with ENOBUFS

    // Provide needed data for the Unit
    constexpr SOCKET sockfd{FD_SETSIZE - 22};
    constexpr char text_addr[] = "192.168.101.203";
    char addrbuf[16];

    s_SocketStuff s;
    // Fill all fields of struct s_SocketStuff
    s.serverAddr = (struct sockaddr*)&s.ss;
    s.ip_version = 4;
    s.text_addr = text_addr;
    s.serverAddr4->sin_family = AF_INET;
    s.serverAddr4->sin_port = 0; // not used
    inet_pton(AF_INET, text_addr, &s.serverAddr4->sin_addr);
    s.fd = sockfd;
    s.try_port = 0; // not used
    s.actual_port = 0;
    s.address_len = sizeof(*s.serverAddr4);

    // Mock system functions
    EXPECT_CALL(m_sys_socketObj, socket(AF_INET, SOCK_STREAM, 0)).Times(0);
    EXPECT_CALL(m_sys_socketObj, listen(sockfd, SOMAXCONN)).Times(1);
    EXPECT_CALL(m_sys_socketObj, getsockname(sockfd, _, _))
        .WillOnce(SetErrnoAndReturn(ENOBUFS, -1));

    // Test Unit
    int ret_do_listen = do_listen(&s);
    EXPECT_EQ(ret_do_listen, UPNP_E_INTERNAL_ERROR)
        << errStrEx(ret_do_listen, UPNP_E_INTERNAL_ERROR);

    // Check all fields of struct s_SocketStuff
    EXPECT_EQ(s.ip_version, 4);
    EXPECT_STREQ(s.text_addr, text_addr);
    EXPECT_EQ(s.serverAddr4->sin_family, AF_INET);
    EXPECT_EQ(ntohs(s.serverAddr4->sin_port), 0); // not used
    EXPECT_STREQ(
        inet_ntop(AF_INET, &s.serverAddr4->sin_addr, addrbuf, sizeof(addrbuf)),
        text_addr);
    EXPECT_EQ(s.fd, sockfd);
    EXPECT_EQ(s.actual_port, 0);
    EXPECT_EQ(s.try_port, 0); // not used
    EXPECT_EQ(s.address_len, (socklen_t)sizeof(*s.serverAddr4));
}

TEST_F(StartMiniServerFTestSuite, get_port_successful) {
    // Configure expected system calls:
    // * Use fictive socket file descriptor 1000
    // * Actual socket used port is 55555
    // * Mocked getsockname() returns successful
    WINSOCK_INIT

    // Provide needed data for the Unit
    constexpr SOCKET sockfd{FD_SETSIZE - 23};
    constexpr char text_addr[] = "192.168.154.188";
    constexpr uint16_t actual_port{55555};
    // This is for the returned port number
    uint16_t port{0xAAAA};

    // Provide a sockaddr structure that will be returned by mocked
    // getsockname().
    const CAddrinfo ai(std::string(text_addr), std::to_string(actual_port),
                       AF_INET, SOCK_STREAM, AI_NUMERICHOST | AI_NUMERICSERV);

    // Mock system functions
    EXPECT_CALL(m_sys_socketObj,
                getsockname(sockfd, _, Pointee(Ge((socklen_t)ai->ai_addrlen))))
        .WillOnce(DoAll(SetArgPointee<1>(*ai->ai_addr),
                        SetArgPointee<2>((socklen_t)ai->ai_addrlen),
                        Return(0)));

    // Test Unit
    EXPECT_EQ(get_port(sockfd, &port), 0);

    EXPECT_EQ(port, actual_port);
}

TEST_F(StartMiniServerFTestSuite, get_port_wrong_sockaddr_family) {
    // Configure expected system calls:
    // * Use fictive socket file descriptor 998
    // * Mocked getsockname() returns successful unusable sockaddr family 0.

    // Provide needed data for the Unit
    constexpr SOCKET sockfd{FD_SETSIZE - 24};
    // This is for the returned port number
    uint16_t port{0xAAAA};

    // Provide a sockaddr structure that will be returned by mocked
    // getsockname(). It is empty.
    const sockaddr sa{};

    // Mock system function
    EXPECT_CALL(m_sys_socketObj, getsockname(sockfd, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(sa),
                        SetArgPointee<2>((socklen_t)sizeof(sa)), Return(0)));

    // Test Unit
    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Getting port number with unusable sockaddr family must "
                     "fail.\n";
        EXPECT_EQ(get_port(sockfd, &port), 0); // Wrong!

    } else {

        EXPECT_EQ(get_port(sockfd, &port), -1);
    }

    // The port variable should not be modified.
    EXPECT_EQ(port, 0xAAAA);
}

TEST_F(StartMiniServerFTestSuite, get_port_fails) {
    // Configure expected system calls:
    // * Use fictive socket file descriptor 900
    // * Mocked getsockname() fails with insufficient resources (ENOBUFS).

    // Provide needed data for the Unit
    constexpr SOCKET sockfd{FD_SETSIZE - 25};
    // This is for the returned port number
    uint16_t port{0xAAAA};

    // Provide a sockaddr structure that will be returned by mocked
    // getsockname(). It will be empty.
    const sockaddr sa{};

    // Mock system functions
    EXPECT_CALL(m_sys_socketObj, getsockname(sockfd, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(sa), SetErrnoAndReturn(ENOBUFS, -1)));

    // Test Unit
    EXPECT_EQ(get_port(sockfd, &port), -1);

    EXPECT_EQ(errno, ENOBUFS);
    EXPECT_EQ(port, 0xAAAA);
}

TEST(StartMiniServerTestSuite, get_miniserver_stopsock) {
    // Here we test a real connection to the loopback device. This needs
    // initialization of sockets on MS Windows which is done with the fixture.
    // We also have to close the socket.
    WINSOCK_INIT
    MiniServerSockArray out;
    InitMiniServerSockArray(&out);

    // Test Unit, needs initialized sockets on MS Windows
    int ret_get_miniserver_stopsock = get_miniserver_stopsock(&out);
    EXPECT_EQ(ret_get_miniserver_stopsock, UPNP_E_SUCCESS)
        << errStrEx(ret_get_miniserver_stopsock, UPNP_E_SUCCESS);

    EXPECT_NE(out.miniServerStopSock, (SOCKET)0);
    EXPECT_NE(out.stopPort, 0);
    EXPECT_EQ(out.stopPort, miniStopSockPort);

    // Get socket object from the bound socket
    CSocket_basic sockObj(out.miniServerStopSock);

    // and verify its settings
    EXPECT_EQ(sockObj.get_family(), AF_INET);
    EXPECT_EQ(sockObj.get_port(), miniStopSockPort);
    EXPECT_EQ(sockObj.get_addr_str(), "127.0.0.1");

    // Close socket
    EXPECT_EQ(sock_close(out.miniServerStopSock), 0);
}

TEST_F(StartMiniServerFTestSuite, get_miniserver_stopsock_fails) {
    // Configure expected system calls:
    // * Get a socket() fails with EACCES (Permission denied).
    // * bind() is not called.
    // * getsockname() is not called.

    // Provide needed data for the Unit
    MiniServerSockArray out;
    InitMiniServerSockArray(&out);

    // Mock system functions
    EXPECT_CALL(m_sys_socketObj, socket(AF_INET, SOCK_DGRAM, 0))
        .WillOnce(SetErrnoAndReturn(EACCES, -1));
    EXPECT_CALL(m_sys_socketObj, bind(_, _, _)).Times(0);
    EXPECT_CALL(m_sys_socketObj, getsockname(_, _, _)).Times(0);

    // Test Unit
    int ret_get_miniserver_stopsock = get_miniserver_stopsock(&out);
    EXPECT_EQ(ret_get_miniserver_stopsock, UPNP_E_OUTOF_SOCKET)
        << errStrEx(ret_get_miniserver_stopsock, UPNP_E_OUTOF_SOCKET);

    // Close socket; we don't need to close a mocked socket
}

TEST_F(StartMiniServerFTestSuite, get_miniserver_stopsock_bind_fails) {
    // Configure expected system calls:
    // * socket() returns file descriptor 890.
    // * bind() fails with ENOMEM.
    // * getsockname() is not called.

    // Provide needed data for the Unit
    MiniServerSockArray out;
    InitMiniServerSockArray(&out);
    const SOCKET sockfd{FD_SETSIZE - 26};

    // Mock system functions
    EXPECT_CALL(m_sys_socketObj, socket(AF_INET, SOCK_DGRAM, 0))
        .WillOnce(Return(sockfd));
    EXPECT_CALL(m_sys_socketObj, bind(sockfd, _, _))
        .WillOnce(SetErrnoAndReturn(ENOMEM, -1));
    EXPECT_CALL(m_sys_socketObj, getsockname(_, _, _)).Times(0);

    // Test Unit
    int ret_get_miniserver_stopsock = get_miniserver_stopsock(&out);
    EXPECT_EQ(ret_get_miniserver_stopsock, UPNP_E_SOCKET_BIND)
        << errStrEx(ret_get_miniserver_stopsock, UPNP_E_SOCKET_BIND);

    // Close socket; we don't need to close a mocked socket
}

TEST_F(StartMiniServerFTestSuite, get_miniserver_stopsock_getsockname_fails) {
    // Configure expected system calls:
    // * socket() returns file descriptor 888.
    // * bind() returns successful.
    // * getsockname() fails with ENOBUFS (Cannot allocate memory).

    // Provide needed data for the Unit
    MiniServerSockArray out;
    InitMiniServerSockArray(&out);
    const SOCKET sockfd{FD_SETSIZE - 27};

    // Mock system functions
    EXPECT_CALL(m_sys_socketObj, socket(AF_INET, SOCK_DGRAM, 0))
        .WillOnce(Return(sockfd));
    EXPECT_CALL(m_sys_socketObj, bind(sockfd, _, _)).WillOnce(Return(0));
    EXPECT_CALL(m_sys_socketObj, getsockname(sockfd, _, _))
        .WillOnce(SetErrnoAndReturn(ENOBUFS, -1));

    // Test Unit
    int ret_get_miniserver_stopsock = get_miniserver_stopsock(&out);
    EXPECT_EQ(ret_get_miniserver_stopsock, UPNP_E_INTERNAL_ERROR)
        << errStrEx(ret_get_miniserver_stopsock, UPNP_E_INTERNAL_ERROR);

    // Close socket; we don't need to close a mocked socket
}

TEST_F(RunMiniServerFTestSuite, receive_from_stopsock_successful) {
    WINSOCK_INIT
    // CLogging loggingObj; // Output only with build type DEBUG.

    // The stop socket is got with 'get_miniserver_stopsock()' and uses a
    // datagram with exactly "ShutDown" on AF_INET to 127.0.0.1.
    constexpr char shutdown_str[]{"ShutDown"};
    // This should be the buffer size in the tested code. The test will fail if
    // it does not match so there is no danger to break destination buffer size.
    constexpr int expected_destbuflen{9};

    constexpr SOCKET sockfd{FD_SETSIZE - 5};
    const CAddrinfo ai("127.0.0.1", "50015", AF_INET, SOCK_DGRAM,
                       AI_NUMERICHOST | AI_NUMERICSERV);
    fd_set rdSet;
    FD_ZERO(&rdSet);
    FD_SET(sockfd, &rdSet);

    // Mock system functions
    // expected_destbuflen is important here to avoid buffer limit overwrite.
    EXPECT_CALL(m_sys_socketObj,
                recvfrom(sockfd, _, AnyOf(25, expected_destbuflen), 0, _,
                         Pointee((socklen_t)sizeof(sockaddr_storage))))
        .WillOnce(DoAll(StrnCpyToArg<1>(shutdown_str, expected_destbuflen),
                        SetArgPointee<4>(*ai->ai_addr),
                        Return(expected_destbuflen)));

    // Test Unit
    // Returns 1 (true) if successfully received "ShutDown" from stopSock
    EXPECT_EQ(receive_from_stopSock(sockfd, &rdSet), 1);
}

TEST_F(RunMiniServerFTestSuite, receive_from_stopsock_not_selected) {
    // CLogging loggingObj; // Output only with build type DEBUG.

    constexpr SOCKET sockfd{FD_SETSIZE - 29};

    fd_set rdSet;
    FD_ZERO(&rdSet);
    // Socket not selected to be received
    // FD_SET(sockfd, &rdSet);
    // This should not call recvfrom() and is guarded by StrictMock<>.

    // Test Unit
    EXPECT_EQ(receive_from_stopSock(sockfd, &rdSet), 0);
}

TEST_F(RunMiniServerFTestSuite, receive_from_stopsock_receiving_fails) {
    // CLogging loggingObj; // Output only with build type DEBUG.

    constexpr SOCKET sockfd{FD_SETSIZE - 28};
    fd_set rdSet;
    FD_ZERO(&rdSet);
    FD_SET(sockfd, &rdSet);

    // Mock system functions
    EXPECT_CALL(m_sys_socketObj, recvfrom(sockfd, _, _, _, _, _))
        .WillOnce(SetErrnoAndReturn(EINVAL, SOCKET_ERROR));

    // Test Unit
    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Failing to receive data from the socket must also fail "
                     "the Unit.\n";
        EXPECT_EQ(receive_from_stopSock(sockfd, &rdSet), 1); // Wrong!

    } else {

        EXPECT_EQ(receive_from_stopSock(sockfd, &rdSet), 0);
    }
}

TEST_F(RunMiniServerFTestSuite, receive_from_stopsock_no_bytes_received) {
    WINSOCK_INIT
    // CLogging loggingObj; // Output only with build type DEBUG.

    constexpr char shutdown_str[]{"ShutDown"};
    constexpr SOCKET sockfd{FD_SETSIZE - 30};
    const CAddrinfo ai("127.0.0.1", "50016", AF_INET, SOCK_DGRAM,
                       AI_NUMERICHOST | AI_NUMERICSERV);
    fd_set rdSet;
    FD_ZERO(&rdSet);
    FD_SET(sockfd, &rdSet);

    // Mock system functions
    EXPECT_CALL(m_sys_socketObj,
                recvfrom(sockfd, _, AnyOf(25, (SIZEP_T)sizeof(shutdown_str)), 0,
                         _, Pointee((socklen_t)sizeof(sockaddr_storage))))
        .WillOnce(DoAll(StrCpyToArg<1>(""), SetArgPointee<4>(*ai->ai_addr),
                        Return(0)));

    // Test Unit
    // Returns 1 (true) if successfully received "ShutDown" from stopSock
    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Receiving a datagram with 0 bytes length must fail.\n";
        EXPECT_EQ(receive_from_stopSock(sockfd, &rdSet), 1); // Wrong!

    } else {

        EXPECT_EQ(receive_from_stopSock(sockfd, &rdSet), 0);
    }
}

TEST_F(RunMiniServerFTestSuite, receive_from_stopsock_wrong_stop_message) {
    WINSOCK_INIT
    // CLogging loggingObj; // Output only with build type DEBUG.

    constexpr char shutdown_str[]{"Nothings"};
    // This should be the buffer size in the tested code. The test will fail if
    // it does not match so there is no danger to break buffer size.
    constexpr int expected_destbuflen{9};

    constexpr SOCKET sockfd{FD_SETSIZE - 31};
    const CAddrinfo ai("127.0.0.1", "50017", AF_INET, SOCK_DGRAM,
                       AI_NUMERICHOST | AI_NUMERICSERV);
    fd_set rdSet;
    FD_ZERO(&rdSet);
    FD_SET(sockfd, &rdSet);

    // Mock system functions
    // expected_destbuflen is important here to avoid buffer limit overwrite.
    EXPECT_CALL(m_sys_socketObj,
                recvfrom(sockfd, _, AnyOf(25, expected_destbuflen), 0, _,
                         Pointee((socklen_t)sizeof(sockaddr_storage))))
        .WillOnce(DoAll(StrnCpyToArg<1>(shutdown_str, expected_destbuflen),
                        SetArgPointee<4>(*ai->ai_addr),
                        Return(expected_destbuflen)));

    // Test Unit
    // Returns 1 (true) if successfully received "ShutDown" from stopSock
    EXPECT_EQ(receive_from_stopSock(sockfd, &rdSet), 0);
}

TEST_F(RunMiniServerFTestSuite, receive_from_stopsock_from_wrong_address) {
    WINSOCK_INIT
    // CLogging loggingObj; // Output only with build type DEBUG.

    constexpr char shutdown_str[]{"ShutDown"};
    // This should be the buffer size in the tested code. The test will fail if
    // it does not match so there is no danger to break buffer size.
    constexpr int expected_destbuflen{9};

    constexpr SOCKET sockfd{FD_SETSIZE - 48};
    const CAddrinfo ai("192.168.150.151", "50018", AF_INET, SOCK_DGRAM,
                       AI_NUMERICHOST | AI_NUMERICSERV);
    fd_set rdSet;
    FD_ZERO(&rdSet);
    FD_SET(sockfd, &rdSet);

    // Mock system functions
    // expected_destbuflen is important here to avoid buffer limit overwrite.
    EXPECT_CALL(m_sys_socketObj,
                recvfrom(sockfd, _, AnyOf(25, expected_destbuflen), 0, _,
                         Pointee((socklen_t)sizeof(sockaddr_storage))))
        .WillOnce(DoAll(StrnCpyToArg<1>(shutdown_str, expected_destbuflen),
                        SetArgPointee<4>(*ai->ai_addr),
                        Return(expected_destbuflen)));

    // Test Unit
    // Returns 1 (true) if successfully received "ShutDown" from stopSock
    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Receiving ShutDown not from 127.0.0.1 must fail.\n";
        EXPECT_EQ(receive_from_stopSock(sockfd, &rdSet), 1); // Wrong!

    } else {

        EXPECT_EQ(receive_from_stopSock(sockfd, &rdSet), 0);
    }
}

TEST_F(RunMiniServerFTestSuite, receive_from_stopsock_without_0_termbyte) {
    WINSOCK_INIT
    // CLogging loggingObj; // Output only with build type DEBUG.

    constexpr char shutdown_str[]{"ShutDown "};
    // This should be the buffer size in the tested code. The test will fail if
    // it does not match so there is no danger to break buffer size.
    constexpr int expected_destbuflen{9};

    constexpr SOCKET sockfd{FD_SETSIZE - 49};
    const CAddrinfo ai("127.0.0.1", "50019", AF_INET, SOCK_DGRAM,
                       AI_NUMERICHOST | AI_NUMERICSERV);
    fd_set rdSet;
    FD_ZERO(&rdSet);
    FD_SET(sockfd, &rdSet);

    // Mock system functions
    // expected_destbuflen is important here to avoid buffer limit overwrite.
    EXPECT_CALL(m_sys_socketObj,
                recvfrom(sockfd, _, AnyOf(25, expected_destbuflen), 0, _,
                         Pointee((socklen_t)sizeof(sockaddr_storage))))
        .WillOnce(DoAll(StrnCpyToArg<1>(shutdown_str, expected_destbuflen),
                        SetArgPointee<4>(*ai->ai_addr),
                        Return(expected_destbuflen)));

    // Test Unit
    // Returns 1 (true) if successfully received "ShutDown" from stopSock
    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Receiving ShutDown not from 127.0.0.1 must fail.\n";
        EXPECT_EQ(receive_from_stopSock(sockfd, &rdSet), 1); // Wrong!

    } else {

        EXPECT_EQ(receive_from_stopSock(sockfd, &rdSet), 0);
    }
}

TEST_F(RunMiniServerFTestSuite, RunMiniServer) {
    // IMPORTANT! There is a limit FD_SETSIZE = 1024 for socket file
    // descriptors that can be used with 'select()'. This limit is not given
    // when using 'poll()' or 'epoll' instead. Old_code uses 'select()' so we
    // must test with this limited socket file descriptors. Otherwise we may
    // get segfaults with 'FD_SET()'. For details have a look at 'man select'.
    //
    // This would start some other threads. We run into dynamic problems with
    // parallel running threads here. For example running the miniserver with
    // schedule_request_job() in a new thread cannot be finished before the
    // mocked miniserver shutdown in the calling thread has been executed at
    // Unit end. This is why I prevent starting other threads. We only test
    // initialize running the miniserver and stopping it.
    //
    // We have 7 socket file descriptors and additional 2 with client APIs,
    // that are used to listen to the different IPv4 and IPv6 protocols for the
    // miniserver (4 fds), the ssdp service (3 fds) and the ssdp request
    // service (2 fds). These are file descriptors summarized in the structure
    // MiniServerSockArray. For details look there.
    //
    // For this test we use only socket file descriptor miniServerSock4 that is
    // listening on IPv4 for the miniserver. --Ingo
    std::cout
        << CYEL "[ TODO     ] " CRES << __LINE__
        << ": Test must be extended for IPv6 sockets and other sockets.\n";

    // CLogging loggingObj; // Output only with build type DEBUG.

    // Initialize the threadpool. Don't forget to shutdown the threadpool at the
    // end. nullptr means to use default attributes.
    ASSERT_EQ(ThreadPoolInit(&gMiniServerThreadPool, nullptr), 0);
    // Prevent to add jobs, we test jobs isolated.
    gMiniServerThreadPool.shutdown = 1;
    // EXPECT_EQ(TPAttrSetMaxJobsTotal(&gMiniServerThreadPool.attr, 0), 0);

    // We need this on the heap because it is freed by 'RunMiniServer()'.
    MiniServerSockArray* minisock =
        (MiniServerSockArray*)malloc(sizeof(MiniServerSockArray));
    ASSERT_NE(minisock, nullptr);
    InitMiniServerSockArray(minisock);

    // Set needed data, listen miniserver only on IPv4 that will connect to a
    // remote client which has done e rquest.
    minisock->miniServerSock4 = FD_SETSIZE - 6;
    minisock->miniServerPort4 = 50012;
    // minisock->miniServerSock6 = FD_SETSIZE - n;
    // minisock->miniServerPort6 = 5xxxx;
    // minisock->ssdpSock4 = FD_SETSIZE - 7;

    minisock->miniServerStopSock = FD_SETSIZE - 8;
    minisock->stopPort = 50013;
    constexpr SOCKET remote_connect_sockfd = FD_SETSIZE - 9;
    const std::string remote_connect_port = "50014";

    // Get highest used socket file descriptor
    SOCKET select_nfds{};
    select_nfds = std::max(select_nfds, minisock->miniServerSock4);
    // select_nfds = std::max(select_nfds, minisock->miniServerSock6);
    // select_nfds = std::max(select_nfds, minisock->ssdpSock4);
    select_nfds = std::max(select_nfds, minisock->miniServerStopSock);
    select_nfds = std::max(select_nfds, remote_connect_sockfd);
    ++select_nfds; // Must be highest used fd + 1

    { // Scope of mocking only within this block

        if (old_code) {
            std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                      << ": Max socket fd for select() not setting to 0 if "
                         "INVALID_SOCKET in MiniServerSockArray on WIN32.\n";
#ifdef _WIN32
            EXPECT_CALL(m_sys_socketObj, //
                        select(0, _, nullptr, _, nullptr))
                .WillOnce(Return(1)); // Wrong !
#else
            EXPECT_CALL(m_sys_socketObj,
                        select(select_nfds, _, nullptr, _, nullptr))
                .WillOnce(Return(1));
#endif
        } else {

            EXPECT_CALL(m_sys_socketObj,
                        select(select_nfds, _, nullptr, _, nullptr))
                .WillOnce(Return(1));
        }

        SSockaddr_storage ss_connected;
        ss_connected = "192.168.200.201:" + remote_connect_port;
        EXPECT_CALL(m_sys_socketObj,
                    accept(minisock->miniServerSock4, NotNull(),
                           Pointee((socklen_t)sizeof(sockaddr_storage))))
            .WillOnce(DoAll(SetArgPointee<1>(*(sockaddr*)&ss_connected.ss),
                            Return(remote_connect_sockfd)));

        SSockaddr_storage ss_localhost;
        ss_localhost = "127.0.0.1:" + std::to_string(minisock->stopPort);

        if (old_code) {
            EXPECT_CALL(m_sys_socketObj,
                        recvfrom(minisock->miniServerStopSock, _, 25, 0, _,
                                 Pointee((socklen_t)sizeof(sockaddr_storage))))
                .WillOnce(DoAll(StrCpyToArg<1>("ShutDown"),
                                SetArgPointee<4>(*(sockaddr*)&ss_localhost.ss),
                                Return((SSIZEP_T)sizeof("ShutDown"))));

        } else {

            constexpr char shutdown_str[]("ShutDown");
            constexpr size_t shutdown_strlen{sizeof(shutdown_str)};
            // It is important to expect shutdown_strlen.
            EXPECT_CALL(m_sys_socketObj,
                        recvfrom(minisock->miniServerStopSock, _,
                                 shutdown_strlen, 0, _,
                                 Pointee((socklen_t)sizeof(sockaddr_storage))))
                .WillOnce(DoAll(StrCpyToArg<1>(shutdown_str),
                                SetArgPointee<4>(*(sockaddr*)&ss_localhost.ss),
                                Return((SSIZEP_T)shutdown_strlen)));
        }

        std::cout << CRED "[ BUG      ] " CRES << __LINE__
                  << ": Unit must not expect its argument MiniServerSockArray* "
                     "to be on the heap and free it.\n";

        // Test Unit
        RunMiniServer(minisock);

    } // End scope of mocking, objects within the block will be destructed.

    // Shutdown the threadpool.
    EXPECT_EQ(ThreadPoolShutdown(&gMiniServerThreadPool), 0);
}

TEST_F(RunMiniServerFTestSuite, ssdp_read_successful) {
    WINSOCK_INIT
    // CLogging loggingObj; // Output only with build type DEBUG.

    // Initialize the threadpool. Don't forget to shutdown the threadpool at the
    // end. nullptr means to use default attributes.
    ASSERT_EQ(ThreadPoolInit(&gRecvThreadPool, nullptr), 0);
    // Prevent to add jobs, we test jobs isolated.
    gRecvThreadPool.shutdown = 1;

    constexpr char ssdpdata_str[]{
        "Some SSDP test data for a request of a remote client."};
    ASSERT_LE(sizeof(ssdpdata_str), BUFSIZE - 1);

    SOCKET ssdp_sockfd{FD_SETSIZE - 32};
    const CAddrinfo ai("192.168.71.82", "50023", AF_INET, SOCK_DGRAM,
                       AI_NUMERICHOST | AI_NUMERICSERV);
    fd_set rdSet;
    FD_ZERO(&rdSet);
    FD_SET(ssdp_sockfd, &rdSet);

    // Instantiate mocking objects.
    // umock::PthreadMock pthreadObj;
    // Inject mocking objects into the production code.
    // umock::Pthread pthread_injectObj(&pthreadObj);

    // EXPECT_CALL(pthreadObj, pthread_mutex_lock(_)).Times(1);
    // EXPECT_CALL(pthreadObj, pthread_mutex_unlock(_)).Times(1);
    EXPECT_CALL(m_sys_socketObj,
                recvfrom(ssdp_sockfd, _, BUFSIZE - 1, 0, _,
                         Pointee((socklen_t)sizeof(sockaddr_storage))))
        .WillOnce(DoAll(StrCpyToArg<1>(ssdpdata_str),
                        SetArgPointee<4>(*ai->ai_addr),
                        Return((SSIZEP_T)sizeof(ssdpdata_str))));

    // Test Unit
    ssdp_read(&ssdp_sockfd, &rdSet);

    EXPECT_NE(ssdp_sockfd, INVALID_SOCKET);

    // Shutdown the threadpool.
    EXPECT_EQ(ThreadPoolShutdown(&gRecvThreadPool), 0);
}

TEST_F(RunMiniServerFTestSuite, ssdp_read_fails) {
    WINSOCK_INIT
    // CLogging loggingObj; // Output only with build type DEBUG.

    constexpr char ssdpdata_str[]{
        "Some SSDP test data for a request of a remote client."};
    ASSERT_LE(sizeof(ssdpdata_str), BUFSIZE - 1);

    constexpr SOCKET ssdp_sockfd_valid{FD_SETSIZE - 46};
    const CAddrinfo ai("192.168.71.82", "50023", AF_INET, SOCK_DGRAM,
                       AI_NUMERICHOST | AI_NUMERICSERV);

    // Instantiate mocking objects.
    EXPECT_CALL(m_sys_socketObj, recvfrom(_, _, _, _, _, _)).Times(0);

    // Socket is set but not in the select set.
    SOCKET ssdp_sockfd{ssdp_sockfd_valid};
    fd_set rdSet;
    FD_ZERO(&rdSet);

    // Test Unit, socket should be untouched.
    ssdp_read(&ssdp_sockfd, &rdSet);
    EXPECT_EQ(ssdp_sockfd, ssdp_sockfd_valid);

    // Invalid socket must not be set in the select set.
    ssdp_sockfd = INVALID_SOCKET;

    // Test Unit
    ssdp_read(&ssdp_sockfd, &rdSet);
    EXPECT_EQ(ssdp_sockfd, INVALID_SOCKET);

    // Socket is set, also in the select set, but reading from socket fails.
    ssdp_sockfd = ssdp_sockfd_valid;
    FD_SET(ssdp_sockfd, &rdSet);

    EXPECT_CALL(m_sys_socketObj, recvfrom(ssdp_sockfd, _, _, _, _, _))
        .WillOnce(SetErrnoAndReturn(EINVAL, SOCKET_ERROR));

    // Test Unit
    ssdp_read(&ssdp_sockfd, &rdSet);
    EXPECT_EQ(ssdp_sockfd, INVALID_SOCKET);
}

TEST_F(RunMiniServerFTestSuite, web_server_accept) {
    constexpr SOCKET listen_sockfd{FD_SETSIZE - 33};
    constexpr SOCKET connected_sockfd{FD_SETSIZE - 34};
    const std::string connected_port = "306";
    fd_set set;
    FD_ZERO(&set);
    FD_SET(listen_sockfd, &set);

    // Initialize the threadpool. Don't forget to shutdown the threadpool at the
    // end. nullptr means to use default attributes.
    ASSERT_EQ(ThreadPoolInit(&gMiniServerThreadPool, nullptr), 0);
    // Prevent to add jobs, we test jobs isolated. See note at
    // TEST(RunMiniServerTestSuite, RunMiniServer).
    // gMiniServerThreadPool.shutdown = 1;
    EXPECT_EQ(TPAttrSetMaxJobsTotal(&gMiniServerThreadPool.attr, 0), 0);

    { // Scope of mocking only within this block

        SSockaddr_storage ssObj;
        ssObj = "192.168.201.202:" + connected_port;
        EXPECT_CALL(m_sys_socketObj,
                    accept(listen_sockfd, NotNull(),
                           Pointee((socklen_t)sizeof(::sockaddr_storage))))
            .WillOnce(DoAll(SetArgPointee<1>(*(sockaddr*)&ssObj.ss),
                            Return(connected_sockfd)));

        // Capture output to stderr
        CLogging loggingObj; // Output only with build type DEBUG.
        CaptureStdOutErr captureObj(STDERR_FILENO); // or STDOUT_FILENO
        captureObj.start();

        // Test Unit
        web_server_accept(listen_sockfd, &set);

        // Get captured output
        // '\n' is not matched by regex '.'-wildcard so we just replace it.
        std::replace(captureObj.str().begin(), captureObj.str().end(), '\n',
                     '@');
#ifdef DEBUG
        if (old_code)
            EXPECT_THAT(captureObj.str(),
                        ContainsStdRegex(" UPNP-MSER-2: .* mserv " +
                                         std::to_string(connected_sockfd) +
                                         ": cannot schedule request"));
        else
            EXPECT_THAT(
                captureObj.str(),
                ContainsStdRegex(" connected to host 192\\.168\\.201\\.202:306 "
                                 "with socket " +
                                 std::to_string(connected_sockfd)));
            // ContainsStdRegex(" connected to host 192\\.168\\.201\\.202:306 "
            //                  "with socket 206.* UPNP-MSER-1: .* mserv 206: "
            //                  "cannot schedule request"));
#else
        EXPECT_THAT(captureObj.str(),
                    ContainsStdRegex("libupnp ThreadPoolAdd too many jobs: 0"));
#endif
    } // End scope of mocking, objects within the block will be destructed.

    // Shutdown the threadpool.
    EXPECT_EQ(ThreadPoolShutdown(&gMiniServerThreadPool), 0);
}

TEST_F(RunMiniServerFTestSuite, web_server_accept_with_invalid_socket) {
    constexpr SOCKET listen_sockfd = INVALID_SOCKET;

    EXPECT_CALL(m_sys_socketObj, accept(_, _, _)).Times(0);

    // Capture output to stderr
    CLogging loggingObj; // Output only with build type DEBUG.
    CaptureStdOutErr captureObj(STDERR_FILENO); // or STDOUT_FILENO
    captureObj.start();

    // Test Unit
    web_server_accept(listen_sockfd, nullptr);

    // Get captured output
    if (old_code)
        EXPECT_TRUE(captureObj.str().empty());
    else
#ifdef DEBUG
        EXPECT_THAT(captureObj.str(),
                    ContainsStdRegex(" UPNP-MSER-1: .* invalid socket\\(-1\\) "
                                     "or set\\(.*\\)\\.\n"));
#else
        EXPECT_TRUE(captureObj.str().empty());
#endif
}

TEST_F(RunMiniServerFTestSuite, web_server_accept_with_empty_set) {
    constexpr SOCKET listen_sockfd{FD_SETSIZE - 35};
    fd_set set;
    FD_ZERO(&set);

    EXPECT_CALL(m_sys_socketObj, accept(_, _, _)).Times(0);

    // Capture output to stderr
    CLogging loggingObj; // Output only with build type DEBUG.
    class CaptureStdOutErr captureObj(STDERR_FILENO); // or STDOUT_FILENO
    captureObj.start();

    // Test Unit
    web_server_accept(listen_sockfd, &set);

    // Get captured output
    if (old_code)
        EXPECT_TRUE(captureObj.str().empty());
    else
#ifdef DEBUG
        EXPECT_THAT(captureObj.str(),
                    ContainsStdRegex(" UPNP-MSER-1: .* invalid socket\\(" +
                                     std::to_string(listen_sockfd) +
                                     "\\) or set\\(.*\\)\\.\n"));
#else
        EXPECT_TRUE(captureObj.str().empty());
#endif
}

TEST_F(RunMiniServerFTestSuite, web_server_accept_fails) {
    constexpr SOCKET listen_sockfd{FD_SETSIZE - 50};
    fd_set set;
    FD_ZERO(&set);
    FD_SET(listen_sockfd, &set);

    EXPECT_CALL(m_sys_socketObj,
                accept(listen_sockfd, NotNull(),
                       Pointee((socklen_t)sizeof(::sockaddr_storage))))
        .WillOnce(SetErrnoAndReturn(EINVAL, INVALID_SOCKET));

    // Capture output to stderr
    CLogging loggingObj; // Output only with build type DEBUG.
    class CaptureStdOutErr captureObj(STDERR_FILENO); // or STDOUT_FILENO
    captureObj.start();

    // Test Unit
    web_server_accept(listen_sockfd, &set);

    // Get captured output
    std::cout << captureObj.str();
#ifdef DEBUG
    if (old_code)
        EXPECT_THAT(
            captureObj.str(),
            ContainsStdRegex(
                " UPNP-MSER-2: .* miniserver: Error in accept\\(\\): "));
    else
        EXPECT_THAT(
            captureObj.str(),
            ContainsStdRegex(" UPNP-MSER-1: .* web_server_accept\\(\\): Error "
                             "in accept\\(\\): "));
#else
    EXPECT_TRUE(captureObj.str().empty());
#endif
}

TEST(RunMiniServerTestSuite, fdset_if_valid) {
    fd_set rdSet;
    FD_ZERO(&rdSet);

    // Testing for negative or >= FD_SETSIZE socket file descriptors with
    // FD_ISSET fails on Github Integration tests with **exception or with
    // "buffer overflow" using Ubuntu Release. So we cannot check if negative
    // file descriptors are not added to the FD list for select().
    //
    // EXPECT_EQ(FD_ISSET(INVALID_SOCKET, &rdSet), 0)
    //     << "Socket file descriptor INVALID_SOCKET should not fail with "
    //        "**exeption or buffer overflow";
    //
    // EXPECT_EQ(FD_ISSET(FD_SETSIZE, &rdSet), 0)
    //     << "Socket file descriptor FD_SETSIZE should not fail with "
    //        "**exeption or buffer overflow";

    // Valid socket file descriptor will be added to the set.
    constexpr SOCKET sockfd1{FD_SETSIZE - 47};
    fdset_if_valid(sockfd1, &rdSet);
    EXPECT_NE(FD_ISSET(sockfd1, &rdSet), 0)
        << "Socket file descriptor " << sockfd1
        << " should be added to the FD SET for select().";

    std::cout << CYEL "[ TODO     ] " CRES << __LINE__
              << ": Check if \"buffer overflow\" still exists with FD_ISSET "
                 "using invalid socket file descriptor.\n";

    if (old_code)
        // These expectations are labeled below with 'Wrong!'.
        std::cout << CYEL "[ FIX      ] " CRES << __LINE__
                  << ": invalid socket file descriptors should not be added to "
                     "the FD SET for select().\n";

    // This isn't added to the set.
    constexpr SOCKET sockfd2 = INVALID_SOCKET;
    fdset_if_valid(sockfd2, &rdSet);
    EXPECT_NE(FD_ISSET(sockfd1, &rdSet), 0)
        << "Socket file descriptor " << sockfd1
        << " should be added to the FD SET for select().";
    // We cannot check it because of possible negative socket fd.
    // EXPECT_EQ(FD_ISSET(sockfd2, &rdSet), 0)
    //     << "Socket file descriptor " << sockfd2
    //     << " should NOT be added to the FD SET for select().";

    // File descriptor for stderr must not be added to the set.
    constexpr SOCKET sockfd3 = 2;
    fdset_if_valid(sockfd3, &rdSet);
    EXPECT_NE(FD_ISSET(sockfd1, &rdSet), 0)
        << "Socket file descriptor " << sockfd1
        << " should be added to the FD SET for select().";
    if (old_code)
        EXPECT_NE(FD_ISSET(sockfd3, &rdSet), 0); // Wrong!
    else
        EXPECT_EQ(FD_ISSET(sockfd3, &rdSet), 0)
            << "Socket file descriptor " << sockfd3
            << " should NOT be added to the FD SET for select().";

    // File descriptor 3 could be usable if not already used by another service.
    constexpr SOCKET sockfd4 = 3;
    fdset_if_valid(sockfd4, &rdSet);
    EXPECT_NE(FD_ISSET(sockfd1, &rdSet), 0)
        << "Socket file descriptor " << sockfd1
        << " should be added to the FD SET for select().";
    EXPECT_NE(FD_ISSET(sockfd4, &rdSet), 0)
        << "Socket file descriptor " << sockfd4
        << " should be added to the FD SET for select().";

    // File descriptor >= FD_SETSIZE must not be added to the set. It is beyond
    // the limit for connect().
    if (!old_code) {
        // Old code tries to add this to the FD set and fails with buffer
        // overflow on Github Integration test with Ubuntu Release.
        constexpr SOCKET sockfd5 = FD_SETSIZE;
        fdset_if_valid(sockfd5, &rdSet);
        EXPECT_NE(FD_ISSET(sockfd4, &rdSet), 0)
            << "Socket file descriptor " << sockfd4
            << " should be added to the FD SET for select().";
    }
    // We cannot check it because of buffer overflow with FD_ISSET using
    // FD_SETSIZE.
    // EXPECT_EQ(FD_ISSET(sockfd5, &rdSet), 0)
    //     << "Socket file descriptor " << sockfd5
    //     << " should NOT be added to the FD SET for select().";
}

TEST(RunMiniServerTestSuite, schedule_request_job) {
    WINSOCK_INIT
    constexpr SOCKET connected_sockfd{FD_SETSIZE - 36};
    constexpr uint16_t connected_port = 302;
    const CAddrinfo ai("192.168.1.1", std::to_string(connected_port), AF_INET,
                       SOCK_STREAM, AI_NUMERICHOST | AI_NUMERICSERV);

    // Initialize the threadpool. Don't forget to shutdown the threadpool at the
    // end. nullptr means to use default attributes.
    ASSERT_EQ(ThreadPoolInit(&gMiniServerThreadPool, nullptr), 0);
    // Prevent to add jobs, we test jobs isolated. See note at
    // TEST(RunMiniServerTestSuite, RunMiniServer).
    // gMiniServerThreadPool.shutdown = 1;
    EXPECT_EQ(TPAttrSetMaxJobsTotal(&gMiniServerThreadPool.attr, 0), 0);

    // Capture output to stderr
    CLogging loggingObj; // Output only with build type DEBUG.
    CaptureStdOutErr captureObj(STDERR_FILENO); // or STDOUT_FILENO
    captureObj.start();

    // Test Unit
    schedule_request_job(connected_sockfd, ai->ai_addr);

    // Get captured output
#ifdef DEBUG
    EXPECT_THAT(captureObj.str(),
                ContainsStdRegex(" UPNP-MSER-\\d: .* " +
                                 std::to_string(connected_sockfd) +
                                 ": cannot schedule request\n"));
#else
    EXPECT_THAT(captureObj.str(),
                ContainsStdRegex("libupnp ThreadPoolAdd too many jobs: 0\n"));
#endif
    // Shutdown the threadpool.
    EXPECT_EQ(ThreadPoolShutdown(&gMiniServerThreadPool), 0);
}

TEST(RunMiniServerDeathTest, free_handle_request_arg_with_nullptr_to_struct) {
    // Provide request structure
    constexpr mserv_request_t* const request{nullptr};

    // Test Unit
    if (old_code) {
#if defined __APPLE__ && !DEBUG
#else
        std::cout << CYEL "[ FIX      ] " CRES << __LINE__
                  << ": free_handle_re4quest with nullptr must not segfault.\n";
        // There may be a multithreading test running before with
        // TEST(RunMiniServerTestSuite, schedule_request_job). Due to problems
        // running this together with death tests as noted in the gtest docu the
        // GTEST_FLAG has to be set. If having more death tests it should be
        // considered to run multithreading tests with an own test file without
        // death tests.
        GTEST_FLAG_SET(death_test_style, "threadsafe");
        EXPECT_DEATH(free_handle_request_arg(request), "");
#endif
    } else {

        free_handle_request_arg(request);
    }
}

TEST(RunMiniServerTestSuite, handle_request) {
    GTEST_SKIP()
        << "Still needs to be done when I have understood http_RecvMessage().";
}

TEST(RunMiniServerTestSuite, handle_request_with_invalid_socket) {
    GTEST_SKIP()
        << "Still needs to be done when I have understood http_RecvMessage().";

    mserv_request_t request{};
    request.connfd = 999;

    // Test Unit
    handle_request(&request);
}

TEST(RunMiniServerTestSuite, free_handle_request_arg) {
    // Provide request structure
    mserv_request_t* const request = (mserv_request_t*)malloc(sizeof(*request));
    memset(request, 0, sizeof *request);
    // and set a socket
    request->connfd = socket(AF_INET, SOCK_STREAM, 0);

    // Test Unit
    free_handle_request_arg(request);
}

TEST(RunMiniServerTestSuite, free_handle_request_arg_with_invalid_socket) {
    // Provide request structure
    mserv_request_t* const request = (mserv_request_t*)malloc(sizeof(*request));
    memset(request, 0, sizeof *request);
    // and set an invalid socket
    request->connfd = INVALID_SOCKET;

    // Test Unit
    free_handle_request_arg(request);
}

TEST(RunMiniServerTestSuite, handle_error) {
    GTEST_SKIP() << "Still needs to be done when I have made the test to "
                    "http_SendStatusResponse.";
}

TEST(RunMiniServerTestSuite, dispatch_request) {
    GTEST_SKIP() << "Still needs to be done when we have complete tests for "
                    "httpreadwrite.";
}

TEST_F(RunMiniServerFTestSuite, get_numeric_host_redirection) {
    WINSOCK_INIT
    // getNumericHostRedirection() returns the ip address with port as text
    // (e.g. "192.168.1.2:54321") that is bound to a socket.

    constexpr SOCKET sockfd{FD_SETSIZE - 37};
    char host_port[INET6_ADDRSTRLEN + 1 + 5]{"<no message>"};

    // Provide a sockaddr structure that will be returned by mocked
    // getsockname().
    const CAddrinfo ai1("192.168.123.122", "54321", AF_INET, SOCK_STREAM,
                        AI_NUMERICHOST | AI_NUMERICSERV);

    // Test Unit
    if (old_code) {
        EXPECT_CALL(m_sys_socketObj, getsockname(sockfd, _, _))
            .WillOnce(DoAll(SetArgPointee<1>(*ai1->ai_addr), Return(0)));
        EXPECT_TRUE(getNumericHostRedirection((int)sockfd, host_port,
                                              sizeof(host_port)));
    } else {

        EXPECT_CALL(m_sys_socketObj,
                    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, _, _))
            .WillOnce(Return(0));
        EXPECT_CALL(m_sys_socketObj, getsockname(sockfd, _, _))
            .Times(2)
            .WillRepeatedly(DoAll(SetArgPointee<1>(*ai1->ai_addr), Return(0)));
        EXPECT_TRUE(
            getNumericHostRedirection(sockfd, host_port, sizeof(host_port)));
    }

    EXPECT_STREQ(host_port, "192.168.123.122:54321");
}

TEST_F(RunMiniServerFTestSuite,
       get_numeric_host_redirection_with_insufficient_resources) {
    constexpr SOCKET sockfd{FD_SETSIZE - 38};
    char host_port[INET6_ADDRSTRLEN + 1 + 5]{"<no message>"};

    // Mock system function getsockname()
    // to fail with insufficient resources.
    EXPECT_CALL(m_sys_socketObj, getsockname(sockfd, _, _))
        .WillOnce(SetErrnoAndReturn(ENOBUFS, -1));

    // Capture output to stderr
    CaptureStdOutErr captureObj(STDERR_FILENO); // or STDOUT_FILENO
    captureObj.start();

    // Test Unit
    if (old_code) {
        EXPECT_FALSE(getNumericHostRedirection((int)sockfd, host_port,
                                               sizeof(host_port)));
        // Get captured output. This doesn't give any error messages.
        EXPECT_TRUE(captureObj.str().empty());

    } else {

        EXPECT_CALL(m_sys_socketObj,
                    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, _, _))
            .WillOnce(Return(0));

        EXPECT_FALSE(
            getNumericHostRedirection(sockfd, host_port, sizeof(host_port)));
        // Get captured output
        EXPECT_THAT(captureObj.str(), HasSubstr("UPnPlib ERROR 1001!"));
    }

    EXPECT_STREQ(host_port, "<no message>");
}

TEST_F(RunMiniServerFTestSuite,
       get_numeric_host_redirection_with_wrong_address_family) {
    constexpr SOCKET sockfd{407};
    char host_port[INET6_ADDRSTRLEN + 1 + 5]{"<no message>"};

    // Provide a sockaddr structure that will be returned by mocked
    // getsockname().
    SSockaddr_storage ssObj;
    ssObj.ss.ss_family = AF_UNIX;

    if (old_code) {
        EXPECT_CALL(m_sys_socketObj, getsockname(sockfd, _, _))
            .WillOnce(
                DoAll(SetArgPointee<1>(*(sockaddr*)&ssObj.ss), Return(0)));

    } else {

        EXPECT_CALL(m_sys_socketObj,
                    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, _, _))
            .WillOnce(Return(0));
        EXPECT_CALL(m_sys_socketObj, getsockname(sockfd, _, _))
            .WillOnce(
                DoAll(SetArgPointee<1>(*(sockaddr*)&ssObj.ss), Return(0)));
    }

    // Capture output to stderr
    CaptureStdOutErr captureObj(STDERR_FILENO); // or STDOUT_FILENO
    captureObj.start();

    // Test Unit
    if (old_code) {
        bool ret_getNumericHostRedirection = getNumericHostRedirection(
            (int)sockfd, host_port, sizeof(host_port));

        std::cout << CYEL "[ FIX      ] " CRES << __LINE__
                  << ": A wrong but accepted address family AF_UNIX should "
                     "return an error.\n";
        EXPECT_TRUE(ret_getNumericHostRedirection); // wrong
        // Get captured output
        EXPECT_TRUE(captureObj.str().empty());
        EXPECT_STREQ(host_port, "0.0.0.0:0"); // wrong

    } else {

        bool ret_getNumericHostRedirection =
            getNumericHostRedirection(sockfd, host_port, sizeof(host_port));

        EXPECT_FALSE(ret_getNumericHostRedirection);
        // Get captured output
        EXPECT_THAT(captureObj.str(), HasSubstr("UPnPlib ERROR 1024!"));
        EXPECT_STREQ(host_port, "<no message>");
    }
}

TEST(RunMiniServerTestSuite, host_header_is_numeric) {
    char host_port[INET_ADDRSTRLEN + 1 + 5]{"192.168.88.99:59876"};

    EXPECT_TRUE(host_header_is_numeric(host_port, sizeof(host_port)));
}

TEST(RunMiniServerTestSuite, host_header_is_numeric_with_invalid_ip_address) {
    char host_port[INET_ADDRSTRLEN + 1 + 5]{"192.168.88.256:59877"};

    EXPECT_FALSE(host_header_is_numeric(host_port, sizeof(host_port)));
}

TEST(RunMiniServerTestSuite, host_header_is_numeric_with_empty_port) {
    char host_port[INET_ADDRSTRLEN + 1 + 5]{"192.168.88.99:"};

    EXPECT_TRUE(host_header_is_numeric(host_port, sizeof(host_port)));
}

TEST(RunMiniServerTestSuite, host_header_is_numeric_without_port) {
    char host_port[INET_ADDRSTRLEN + 1 + 5]{"192.168.88.99"};

    EXPECT_TRUE(host_header_is_numeric(host_port, sizeof(host_port)));
}

TEST(RunMiniServerTestSuite, set_http_get_callback) {
    memset(&gGetCallback, 0xAA, sizeof(gGetCallback));
    SetHTTPGetCallback(web_server_callback);
    EXPECT_EQ(gGetCallback, (MiniServerCallback)web_server_callback);
}

TEST(RunMiniServerTestSuite, set_soap_callback) {
    memset(&gSoapCallback, 0xAA, sizeof(gSoapCallback));
    SetSoapCallback(nullptr);
    EXPECT_EQ(gSoapCallback, (MiniServerCallback) nullptr);
}

TEST(RunMiniServerTestSuite, set_gena_callback) {
    memset(&gGenaCallback, 0xAA, sizeof(gGenaCallback));
    SetGenaCallback(nullptr);
    EXPECT_EQ(gGenaCallback, (MiniServerCallback) nullptr);
}

TEST(RunMiniServerTestSuite, do_reinit) {
    WINSOCK_INIT
    // CLogging loggingObj; // Output only with build type DEBUG.

    // On reinit the socket file descriptor will be closed and a new file
    // descriptor is requested. Mostly it is the same but it is possible that
    // it changes when other socket fds are requested.

    MINISERVER_REUSEADDR = false;
    constexpr char text_addr[] = "192.168.202.244";
    char addrbuf[16];

    // Get a valid socket, needs initialized sockets on MS Windows with fixture.
    const SOCKET sockfd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_NE(sockfd, (SOCKET)-1);

    s_SocketStuff s;
    // Fill all fields of struct s_SocketStuff
    s.ss.ss_family = AF_INET;
    s.serverAddr = (sockaddr*)&s.ss;
    s.ip_version = 4;
    s.text_addr = text_addr;
    s.serverAddr4->sin_port = 0; // not used
    inet_pton(AF_INET, text_addr, &s.serverAddr4->sin_addr);
    s.fd = sockfd;
    s.try_port = 0; // not used
    s.actual_port = 0;
    s.address_len = sizeof(*s.serverAddr4);

    // Test Unit
    EXPECT_EQ(do_reinit(&s), 0);

    EXPECT_STREQ(s.text_addr, text_addr);
    EXPECT_STREQ(
        inet_ntop(AF_INET, &s.serverAddr4->sin_addr, addrbuf, sizeof(addrbuf)),
        text_addr);
    // Valid real socket
    EXPECT_NE(s.fd, INVALID_SOCKET);
    // EXPECT_EQ(s.fd, sockfd); This is an invalid condition. The fd may change.
    EXPECT_EQ(s.try_port, 0);
    EXPECT_EQ(s.actual_port, 0);
    EXPECT_EQ(s.address_len, (socklen_t)sizeof(*s.serverAddr4));

    // Close real socket
    EXPECT_EQ(CLOSE_SOCKET_P(s.fd), 0);
}

TEST(StopMiniServerTestSuite, sock_close) {
    WINSOCK_INIT
    // Close invalid sockets
    EXPECT_EQ(sock_close(INVALID_SOCKET), -1);
    EXPECT_EQ(sock_close(1234), -1);

    // Get a valid socket, needs initialized sockets on MS Windows with fixture.
    const SOCKET sockfd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_NE(sockfd, (SOCKET)-1);
    // Close a valid socket.
    EXPECT_EQ(sock_close(sockfd), 0);
}

} // namespace compa

//
int main(int argc, char** argv) {
#ifdef _MSC_VER
    // Uninitialize Windows sockets because it is global initialized with using
    // the upnplib library. We need this for testing uninitialized sockets.
    ::WSACleanup();
#endif
    ::testing::InitGoogleMock(&argc, argv);
#include "compa/gtest_main.inc"
    return gtest_return_code; // managed in compa/gtest_main.inc
}
