// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-12-19

#include <UPnPsdk/socket.hpp>
#include <UPnPsdk/addrinfo.hpp>

#include <umock/sys_socket_mock.hpp>

#include <random>

#include <utest/utest.hpp>


namespace utest {

using ::testing::_;
using ::testing::AnyOf;
using ::testing::Between;
using ::testing::DoAll;
using ::testing::EndsWith;
using ::testing::Ge;
using ::testing::HasSubstr;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetErrnoAndReturn;
using ::testing::StrictMock;
using ::testing::ThrowsMessage;

using ::UPnPsdk::CSocket;
using ::UPnPsdk::CSocket_basic;
using ::UPnPsdk::CSocketErr;
using ::UPnPsdk::g_dbug;
using ::UPnPsdk::SSockaddr;


class SocketMockFTestSuite : public ::testing::Test {
  protected:
    // Instantiate mocking objects.
    StrictMock<umock::Sys_socketMock> m_sys_socketObj;
    // Inject the mocking objects into the tested code.
    // umock::Sys_socket sys_socket_injectObj{&m_sys_socketObj};

    // Constructor
    SocketMockFTestSuite() {
        // Set default socket object values
        ON_CALL(m_sys_socketObj, socket(_, _, _))
            .WillByDefault(SetErrnoAndReturn(EACCES, SOCKET_ERROR));
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
    }
};


// Create a simple random number generator for port numbers.
// We could need this because we do not reuse addresses before TIME_WAIT has
// expired (socket option SO_REUSEADDR = false). We may want to use different
// socket addresses and that is already given with different port numbers.
std::random_device rd;         // obtain a random number from hardware
std::minstd_rand random(rd()); // seed the generator
std::uniform_int_distribution<in_port_t> portno(49152, 65535); // define range


#if 0
// This is for humans only to check how 'connect()' to a remote host exactly
// works so we can mock it the right way. Don't enable this test permanently
// because it connects to the real internet for DNS name resolution and may
// slow down this gtest dramatically or block the test until timeout expires if
// no connection is possible.
#include <syncstream>

TEST(SockTestSuite, sock_connect_to_host) {
    // Get the remote host socket address
    UPnPsdk::CAddrinfo ai("example.com", "http");
    ASSERT_NO_THROW(ai.init());
    std::osyncstream(std::cout)
        << "Remote IP addrees to connect = \"" << ai.netaddr().str() << "\".\n";

    // Get a socket
    UPnPsdk::CSocket sockObj(ai->ai_addr->sa_family, ai->ai_socktype);
    ASSERT_NO_THROW(sockObj.load());

    // Connect to the remote host
    UPnPsdk::CSocketErr sockerrObj;
    if (::connect(sockObj, ai->ai_addr, ai->ai_addrlen) == SOCKET_ERROR) {
        sockerrObj.catch_error();
        std::cerr << "Error on connect socket: " << sockerrObj.error_str()
                  << ".\n";
        GTEST_FAIL();
    }

    // The unbound socket has bin bound to a local address by ::connect().
    const std::string_view locnetaddr = sockObj.netaddrp();
    std::osyncstream(std::cout)
        << "local IP address of the socket = \"" << locnetaddr << "\".\n";
}
#endif


#ifdef _MSC_VER
TEST(SocketErrorTestSuite, check_WSA_errorcode_compatibillity) {
    // The result of this test with the real system functions is, that there is
    // no clear assignment between BSD socket error constant numbers (e.g.
    // ENOTSOCK) and its counterparts on win32 (e.g. WSAENOTSOCK). I have to
    // map them with a new macro e.g. EBADFP.
    //
    // For details look at
    // REF:_[Error_Codes_-_errno,_h_errno_and_WSAGetLastError]_(https://learn.microsoft.com/en-us/windows/win32/winsock/error-codes-errno-h-errno-and-wsagetlasterror-2)
    WSASetLastError(EWOULDBLOCK);
    EXPECT_EQ(WSAGetLastError(), EWOULDBLOCK);
    EXPECT_EQ(EWOULDBLOCK, 140);
    EXPECT_EQ(WSAEWOULDBLOCK, 10035);
    // std::cout << "EWOULDBLOCK = " << EWOULDBLOCK << ", WSAEWOULDBLOCK = " <<
    // WSAEWOULDBLOCK << "\n";

    WSASetLastError(EINVAL);
    EXPECT_EQ(WSAGetLastError(), EINVAL);
    EXPECT_EQ(EINVAL, 22);
    EXPECT_EQ(WSAEINVAL, 10022);
    // std::cout << "EINVAL = " << EINVAL << ", WSAEINVAL = " << WSAEINVAL <<
    // "\n";

    // This returns a real error of an invalid socket file descriptor number.
    char so_opt;
    socklen_t optlen{sizeof(so_opt)};
    EXPECT_NE(::getsockopt(55555, SOL_SOCKET, SO_ERROR, &so_opt, &optlen), 0);
    EXPECT_EQ(WSAGetLastError(), WSAENOTSOCK); // WSAENOTSOCK = 10038
    EXPECT_NE(WSAGetLastError(), ENOTSOCK);    // ENOTSOCK = 128
}
#endif

TEST(SocketErrorTestSuite, get_socket_error_successful) {
    CSocketErr sockerrObj;

    // Test Unit
    // This returns a real error of an invalid socket file descriptor number.
    char so_opt;
    socklen_t optlen{sizeof(so_opt)};
    int ret_getsockopt =
        ::getsockopt(55555, SOL_SOCKET, SO_ERROR, &so_opt, &optlen);
    sockerrObj.catch_error();
    EXPECT_NE(ret_getsockopt, 0);

    EXPECT_EQ(static_cast<int>(sockerrObj), EBADFP);
    // Don't know what exact message is given. It depends on the platform.
    EXPECT_GE(sockerrObj.error_str().size(), 10);
    std::cout << sockerrObj.error_str() << "\n";
}

TEST(SocketBasicTestSuite, instantiate_socket_successful) {
    SOCKET sfd = ::socket(PF_INET6, SOCK_STREAM, 0);
    ASSERT_NE(sfd, INVALID_SOCKET);

    // Test Unit
    CSocket_basic sockObj(sfd);
    sockObj.load();

    EXPECT_EQ(static_cast<SOCKET>(sockObj), sfd);
    EXPECT_EQ(sockObj.get_family(), PF_INET6);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_STREAM);
    EXPECT_EQ(sockObj.netaddr(), "[::]");
    EXPECT_EQ(sockObj.netaddrp(), "[::]:0");
    EXPECT_EQ(sockObj.get_port(), 0);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());

    CLOSE_SOCKET_P(sfd);
}

TEST(SocketBasicTestSuite, instantiate_empty_socket) {
    // Test Unit
    CSocket_basic sockObj;
    EXPECT_EQ(static_cast<SOCKET>(sockObj), INVALID_SOCKET);
    // All getter from an INVALID_SOCKET throw an exception.
    EXPECT_THAT(
        [&sockObj]() { sockObj.netaddr(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1001: ")));
    EXPECT_THAT([&sockObj]() { sockObj.netaddrp(); },
                ThrowsMessage<std::runtime_error>(
                    // Different on MacOS with MSG1001.
                    AnyOf(HasSubstr("] EXCEPTION MSG1057: "),
                          HasSubstr("] EXCEPTION MSG1001: "))));
    EXPECT_THAT(
        [&sockObj]() { sockObj.get_port(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1001: ")));
    EXPECT_THAT(
        [&sockObj]() { sockObj.get_family(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1001: ")));
    EXPECT_THAT(
        [&sockObj]() { sockObj.get_socktype(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1030: ")));
    EXPECT_THAT(
        [&sockObj]() { sockObj.get_sockerr(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1011: ")));
    EXPECT_THAT(
        [&sockObj]() { sockObj.is_reuse_addr(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1013: ")));
    EXPECT_THAT(
        [&sockObj]() { sockObj.is_bound(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1001: ")));
}

TEST(SocketBasicTestSuite, instantiate_with_bound_socket_fd) {
    // and bind it to a socket.
    CSocket bound_sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(bound_sockObj.load());
    // Default IPV6_V6ONLY setting is different on different platforms but
    // doesn't matter; will be set before binding.
    // EXPECT_FALSE(bound_sockObj.is_v6only());
    ASSERT_NO_THROW(bound_sockObj.bind("", "8080"));
    EXPECT_TRUE(bound_sockObj.is_v6only());
    SOCKET bound_sock = bound_sockObj;

    // Test Unit with a bound socket.
    CSocket_basic sockObj(bound_sock);
    sockObj.load();

    EXPECT_EQ((SOCKET)sockObj, bound_sock);
    EXPECT_EQ(sockObj.get_family(), PF_INET6);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_STREAM);
    EXPECT_EQ(sockObj.netaddr(), "[::1]");
    EXPECT_EQ(sockObj.netaddrp(), "[::1]:8080");
    EXPECT_EQ(sockObj.get_port(), 8080);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
}

TEST(SocketBasicTestSuite, instantiate_socket_af_unix_sock_stream) {
    SOCKET sfd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_NE(sfd, INVALID_SOCKET);

    CSocket_basic sockObj(sfd);
    sockObj.load();
    EXPECT_NE((SOCKET)sockObj, INVALID_SOCKET);
    EXPECT_EQ(sockObj.get_family(), AF_UNIX);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_STREAM);
    EXPECT_EQ(sockObj.get_port(), 0);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());

    bool g_dbug_old = g_dbug;
    CaptureStdOutErr captureObj(UPnPsdk::log_fileno);
    g_dbug = false;
    captureObj.start();
    EXPECT_EQ(sockObj.netaddr(), "");
    EXPECT_THAT(
        captureObj.str(),
        AnyOf("",
              ContainsStdRegex(
                  "^TRACE\\[.*\\].* Executing CSocket_basic::netaddr\\(\\)")));
    g_dbug = true;
    captureObj.start();
    EXPECT_EQ(sockObj.netaddr(), "");
    EXPECT_THAT(captureObj.str(),
                HasSubstr("] ERROR MSG1129: Unsupported address family 1"));
    g_dbug = g_dbug_old;

    CLOSE_SOCKET_P(sfd);
}

TEST(SocketBasicTestSuite, instantiate_socket_af_unix_sock_dgram) {
    SOCKET sfd = ::socket(AF_UNIX, SOCK_DGRAM, 0);

// Seems this isn't supported on Microsoft Windows
#ifndef _MSC_VER
    ASSERT_NE(sfd, INVALID_SOCKET);
    CSocket_basic sockObj(sfd);
    sockObj.load();
    EXPECT_NE((SOCKET)sockObj, INVALID_SOCKET);
    EXPECT_EQ(sockObj.get_family(), AF_UNIX);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_DGRAM);
    EXPECT_EQ(sockObj.get_port(), 0);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
#else
    EXPECT_EQ(sfd, INVALID_SOCKET);
#endif

    CLOSE_SOCKET_P(sfd);
}

TEST(SocketBasicTestSuite, instantiate_socket_af_unix_sock_raw) {
    // The GCC compiler accepts with family AF_UNIX the types SOCK_STREAM,
    // SOCK_DGRAM, SOCK_RAW (changes silently to SOCK_DGRAM) and
    // SOCK_SEQPACKET.
    SOCKET sfd = ::socket(AF_UNIX, SOCK_RAW, 0);

// Seems this is only supported by the GCC compiler
#if defined(__GNUC__) && !defined(__clang__)
    ASSERT_NE(sfd, INVALID_SOCKET);
    CSocket_basic sockObj(sfd);
    sockObj.load();
    EXPECT_NE((SOCKET)sockObj, INVALID_SOCKET);
    EXPECT_EQ(sockObj.get_family(), AF_UNIX);
    // Silently changed
    EXPECT_EQ(sockObj.get_socktype(), SOCK_DGRAM);
    EXPECT_EQ(sockObj.get_port(), 0);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
#else
    EXPECT_EQ(sfd, INVALID_SOCKET);
#endif

    CLOSE_SOCKET_P(sfd);
}

TEST(SocketBasicTestSuite, instantiate_socket_af_unix_sock_seqpacket) {
    // The GCC compiler accepts with family AF_UNIX the types SOCK_STREAM,
    // SOCK_DGRAM, SOCK_RAW (changes silently to SOCK_DGRAM) and
    // SOCK_SEQPACKET.
    SOCKET sfd = ::socket(AF_UNIX, SOCK_SEQPACKET, 0);

// Seems this is only supported by the GCC compiler
#if defined(__GNUC__) && !defined(__clang__)
    ASSERT_NE(sfd, INVALID_SOCKET);
    CSocket_basic sockObj(sfd);
    sockObj.load();
    EXPECT_NE((SOCKET)sockObj, INVALID_SOCKET);
    EXPECT_EQ(sockObj.get_port(), 0);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
#else
    EXPECT_EQ(sfd, INVALID_SOCKET);
#endif

    CLOSE_SOCKET_P(sfd);
}

TEST(SocketBasicTestSuite, set_invalid_socket_fd) {
    CSocket_basic sockObj(INVALID_SOCKET);

    // Test Unit
    EXPECT_THAT(
        [&sockObj]() { sockObj.load(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1014: ")));
}

TEST(SocketBasicTestSuite, set_object_two_times) {
    SOCKET sfd = ::socket(PF_INET6, SOCK_STREAM, 0);
    ASSERT_NE(sfd, INVALID_SOCKET);

    // Test Unit
    CSocket_basic sockObj(sfd);
    sockObj.load();
    sockObj.load();

    EXPECT_EQ(static_cast<SOCKET>(sockObj), sfd);
    EXPECT_EQ(sockObj.get_family(), PF_INET6);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_STREAM);
    EXPECT_EQ(sockObj.netaddrp(), "[::]:0");
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());

    CLOSE_SOCKET_P(sfd);
}

TEST(SocketTestSuite, verify_system_socket_function) {
    // This verfies different behavior on different platforms.
    EXPECT_EQ(::socket(0, 0, 0), INVALID_SOCKET);
    EXPECT_EQ(::socket(0, SOCK_STREAM, 0), INVALID_SOCKET);
    EXPECT_EQ(::socket(0, SOCK_DGRAM, 0), INVALID_SOCKET);
    EXPECT_EQ(::socket(AF_UNSPEC, 0, 0), INVALID_SOCKET);
    EXPECT_EQ(::socket(AF_UNSPEC, SOCK_STREAM, 0), INVALID_SOCKET);
    EXPECT_EQ(::socket(AF_UNSPEC, SOCK_DGRAM, 0), INVALID_SOCKET);
    EXPECT_EQ(::socket(AF_UNIX, SOCK_RDM, 0), INVALID_SOCKET);

    SOCKET sfd{INVALID_SOCKET};
#ifdef _MSC_VER
    sfd = ::socket(PF_INET, 0, 0);
    EXPECT_NE(sfd, INVALID_SOCKET);
    CLOSE_SOCKET_P(sfd);
    sfd = ::socket(PF_INET6, 0, 0);
    EXPECT_NE(sfd, INVALID_SOCKET);
    CLOSE_SOCKET_P(sfd);
#else
    EXPECT_EQ(::socket(PF_INET, 0, 0), INVALID_SOCKET);
    EXPECT_EQ(::socket(PF_INET6, 0, 0), INVALID_SOCKET);
#endif

    sfd = ::socket(PF_INET, SOCK_STREAM, 0);
    EXPECT_NE(sfd, INVALID_SOCKET);
    CLOSE_SOCKET_P(sfd);
    sfd = ::socket(PF_INET, SOCK_DGRAM, 0);
    EXPECT_NE(sfd, INVALID_SOCKET);
    CLOSE_SOCKET_P(sfd);
    sfd = ::socket(PF_INET6, SOCK_STREAM, 0);
    EXPECT_NE(sfd, INVALID_SOCKET);
    CLOSE_SOCKET_P(sfd);
    sfd = ::socket(PF_INET6, SOCK_DGRAM, 0);
    EXPECT_NE(sfd, INVALID_SOCKET);
    CLOSE_SOCKET_P(sfd);
}

TEST(SocketTestSuite, verify_system_getsockname_function) {
    // int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
    SOCKET sfd = ::socket(PF_INET6, SOCK_STREAM, 0);
    ASSERT_NE(sfd, INVALID_SOCKET);

    ::sockaddr_storage ss{};
    socklen_t len = sizeof(ss); // May be modified

#ifndef _MSC_VER
    EXPECT_EQ(::getsockname(sfd, (sockaddr*)&ss, &len), 0);
    EXPECT_EQ(ss.ss_family, AF_INET6);
#else
    ss.ss_family = 0xAAAA;
    EXPECT_NE(::getsockname(sfd, (sockaddr*)&ss, &len), 0);
    EXPECT_EQ(ss.ss_family, 0xAAAA);
#endif
    CLOSE_SOCKET_P(sfd);
}

TEST(SocketTestSuite, get_unbound_ipv6_stream_socket_successful) {
    // Test Unit
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());

    // An unbound socket returns the unknown ip address and port 0
    EXPECT_NE((SOCKET)sockObj, INVALID_SOCKET);
    EXPECT_EQ(sockObj.get_family(), PF_INET6);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_STREAM);
    EXPECT_EQ(sockObj.netaddr(), "[::]");
    EXPECT_EQ(sockObj.netaddrp(), "[::]:0");
    EXPECT_EQ(sockObj.get_port(), 0);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    // Default IPV6_V6ONLY setting is different on different platforms but
    // doesn't matter; will be set before binding.
    // EXPECT_FALSE(sockObj.is_v6only());
    EXPECT_FALSE(sockObj.is_bound());
    EXPECT_FALSE(sockObj.is_listen());
}

TEST(SocketTestSuite, get_socket_two_times_init_successful) {
    // Test Unit
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    ASSERT_NO_THROW(sockObj.load());

    // An unbound socket returns the unknown ip address and port 0
    EXPECT_NE((SOCKET)sockObj, INVALID_SOCKET);
    EXPECT_EQ(sockObj.get_family(), PF_INET6);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_STREAM);
    EXPECT_EQ(sockObj.netaddr(), "[::]");
    EXPECT_EQ(sockObj.netaddrp(), "[::]:0");
    EXPECT_EQ(sockObj.get_port(), 0);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    // Default IPV6_V6ONLY setting is different on different platforms but
    // doesn't matter; will be set before binding.
    // EXPECT_FALSE(sockObj.is_v6only());
    EXPECT_FALSE(sockObj.is_bound());
    EXPECT_FALSE(sockObj.is_listen());
}

TEST(SocketTestSuite, get_unbound_ipv6_dgram_socket_successful) {
    // Test Unit
    CSocket sockObj(PF_INET6, SOCK_DGRAM);
    ASSERT_NO_THROW(sockObj.load());

    // An unbound socket returns the unknown ip address and port 0
    EXPECT_NE((SOCKET)sockObj, INVALID_SOCKET);
    EXPECT_EQ(sockObj.get_family(), PF_INET6);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_DGRAM);
    EXPECT_EQ(sockObj.netaddr(), "[::]");
    EXPECT_EQ(sockObj.netaddrp(), "[::]:0");
    EXPECT_EQ(sockObj.get_port(), 0);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    // Default IPV6_V6ONLY setting is different on different platforms but
    // doesn't matter; will be set before binding.
    // EXPECT_FALSE(sockObj.is_v6only());
    EXPECT_FALSE(sockObj.is_bound());
    EXPECT_FALSE(sockObj.is_listen());
}

TEST(SocketTestSuite, get_unbound_ipv4_stream_socket_successful) {
    // Test Unit
    CSocket sockObj(PF_INET, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());

    // An unbound socket returns the unknown ip address and port 0
    EXPECT_NE((SOCKET)sockObj, INVALID_SOCKET);
    EXPECT_EQ(sockObj.get_family(), PF_INET);
    EXPECT_EQ(sockObj.netaddr(), "0.0.0.0");
    EXPECT_EQ(sockObj.netaddrp(), "0.0.0.0:0");
    EXPECT_EQ(sockObj.get_port(), 0);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_STREAM);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    // Default IPV6_V6ONLY setting is different on different platforms but
    // doesn't matter; will be set before binding.
#ifdef _MSC_VER
    EXPECT_TRUE(sockObj.is_v6only());
#else
    EXPECT_FALSE(sockObj.is_v6only());
#endif
    EXPECT_FALSE(sockObj.is_bound());
    EXPECT_FALSE(sockObj.is_listen());
}

TEST(SocketTestSuite, get_unbound_ipv4_dgram_socket_successful) {
    // Test Unit
    CSocket sockObj(PF_INET, SOCK_DGRAM);
    ASSERT_NO_THROW(sockObj.load());

    // An unbound socket returns the unknown ip address and port 0
    EXPECT_NE((SOCKET)sockObj, INVALID_SOCKET);
    EXPECT_EQ(sockObj.netaddr(), "0.0.0.0");
    EXPECT_EQ(sockObj.netaddrp(), "0.0.0.0:0");
    EXPECT_EQ(sockObj.get_port(), 0);
    EXPECT_EQ(sockObj.get_family(), PF_INET);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_DGRAM);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    // Default IPV6_V6ONLY setting is different on different platforms but
    // doesn't matter; will be set before binding.
#ifdef _MSC_VER
    EXPECT_TRUE(sockObj.is_v6only());
#else
    EXPECT_FALSE(sockObj.is_v6only());
#endif
    EXPECT_FALSE(sockObj.is_bound());
    EXPECT_FALSE(sockObj.is_listen());
}

TEST(SocketTestSuite, try_to_instantiate_invalid_sockets) {
    EXPECT_THAT(
        []() {
            CSocket sockObj(AF_UNSPEC, SOCK_STREAM);
            sockObj.load();
        },
        ThrowsMessage<std::invalid_argument>(
            HasSubstr("] EXCEPTION MSG1015: ")));

    EXPECT_THAT(
        []() {
            CSocket sockObj(AF_UNIX, SOCK_STREAM);
            sockObj.load();
        },
        ThrowsMessage<std::invalid_argument>(
            HasSubstr("] EXCEPTION MSG1015: ")));

    EXPECT_THAT(
        []() {
            CSocket sockObj(0, SOCK_STREAM);
            sockObj.load();
        },
        ThrowsMessage<std::invalid_argument>(
            HasSubstr("] EXCEPTION MSG1015: ")));

    EXPECT_THAT(
        []() {
            CSocket sockObj(PF_INET6, SOCK_RAW);
            sockObj.load();
        },
        ThrowsMessage<std::invalid_argument>(
            HasSubstr("] EXCEPTION MSG1016: ")));

    EXPECT_THAT(
        []() {
            CSocket sockObj(PF_INET, 0);
            sockObj.load();
        },
        ThrowsMessage<std::invalid_argument>(
            HasSubstr("] EXCEPTION MSG1016: ")));
}

TEST(SocketTestSuite, instantiate_empty_socket) {
    // Test Unit
    CSocket sockObj;
    EXPECT_EQ(static_cast<SOCKET>(sockObj), INVALID_SOCKET);
    // All getter from an INVALID_SOCKET throw an exception.
    EXPECT_THAT(
        [&sockObj]() { sockObj.netaddr(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1001: ")));
    EXPECT_THAT([&sockObj]() { sockObj.netaddrp(); },
                ThrowsMessage<std::runtime_error>(
                    // Different on MacOS with MSG1001.
                    AnyOf(HasSubstr("] EXCEPTION MSG1057: "),
                          HasSubstr("] EXCEPTION MSG1001: "))));
    EXPECT_THAT(
        [&sockObj]() { sockObj.get_port(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1001: ")));
    EXPECT_THAT(
        [&sockObj]() { sockObj.get_family(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1001: ")));
    EXPECT_THAT(
        [&sockObj]() { sockObj.get_socktype(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1030: ")));
    EXPECT_THAT(
        [&sockObj]() { sockObj.get_sockerr(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1011: ")));
    EXPECT_THAT(
        [&sockObj]() { sockObj.is_reuse_addr(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1013: ")));
    EXPECT_THAT(
        [&sockObj]() { sockObj.is_v6only(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1028: ")));
    EXPECT_THAT(
        [&sockObj]() { sockObj.is_bound(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1001: ")));
    EXPECT_THAT(
        [&sockObj]() { sockObj.is_listen(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1035: ")));
}

TEST(SocketTestSuite, move_socket_successful) {
    // Provide a socket object
    CSocket sock1(PF_INET, SOCK_STREAM);
    ASSERT_NO_THROW(sock1.load());
    SOCKET old_fd_sock1 = sock1;

    // Get local interface address when node is empty with flag AI_PASSIVE.
    ASSERT_NO_THROW(sock1.bind("", "8080", AI_PASSIVE));

    // Configure socket to listen so it will accept connections.
    ASSERT_NO_THROW(sock1.listen());

    // Test Unit, move sock1 to a new sock2
    // CSocket sock2 = sock1; // This does not compile because it needs a copy
    //                           constructor that isn't available. We restrict
    //                           to move only.
    // This moves the socket file descriptor.
    CSocket sock2{std::move(sock1)};

    // The socket file descriptor has been moved to the new object.
    EXPECT_EQ(static_cast<SOCKET>(sock2), old_fd_sock1);
    // The old object has now an invalid socket file descriptor.
    EXPECT_EQ(static_cast<SOCKET>(sock1), INVALID_SOCKET);

    // Tests of socket object (sock1) with INVALID_SOCKET see later test.
    // Check if new socket is valid.
    EXPECT_EQ(sock2.get_family(), PF_INET);
    EXPECT_EQ(sock2.get_socktype(), SOCK_STREAM);
    EXPECT_EQ(sock2.netaddr(), "0.0.0.0");
    EXPECT_EQ(sock2.netaddrp(), "0.0.0.0:8080");
    EXPECT_EQ(sock2.get_port(), 8080);
    EXPECT_EQ(sock2.get_sockerr(), 0);
    EXPECT_FALSE(sock2.is_reuse_addr());
    // Default IPV6_V6ONLY setting is different on different platforms.
#ifdef _MSC_VER
    EXPECT_TRUE(sock2.is_v6only());
#else
    EXPECT_FALSE(sock2.is_v6only());
#endif
    EXPECT_TRUE(sock2.is_bound());
    EXPECT_TRUE(sock2.is_listen());
}

TEST(SocketTestSuite, assign_socket_successful) {
    // Provide first of two socket objects.
    CSocket sock1(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sock1.load());
    SOCKET old_fd_sock1 = sock1;

    // Get local interface address when node is empty with flag AI_PASSIVE.
    ASSERT_NO_THROW(sock1.bind("", "8080", AI_PASSIVE));

    // Configure socket to listen so it will accept connections.
    ASSERT_NO_THROW(sock1.listen());

    // Provide second empty socket.
    CSocket sock2;

    // Test Unit. We can only move. Copy a socket resource is not useful.
    sock2 = std::move(sock1);

    // The socket file descriptor has been moved to the destination object.
    EXPECT_EQ((SOCKET)sock2, old_fd_sock1);
    // The old object has now an invalid socket file descriptor.
    EXPECT_EQ((SOCKET)sock1, INVALID_SOCKET);

    // Tests of socket object (sock1) with INVALID_SOCKET see later test.
    // Check if new socket is valid.
    EXPECT_EQ(sock2.get_family(), PF_INET6);
    EXPECT_EQ(sock2.get_socktype(), SOCK_STREAM);
    EXPECT_EQ(sock2.netaddr(), "[::]");
    EXPECT_EQ(sock2.netaddrp(), "[::]:8080");
    EXPECT_EQ(sock2.get_port(), 8080);
    EXPECT_EQ(sock2.get_sockerr(), 0);
    EXPECT_FALSE(sock2.is_reuse_addr());
    EXPECT_TRUE(sock2.is_v6only());
    EXPECT_TRUE(sock2.is_bound());
    EXPECT_TRUE(sock2.is_listen());
}

TEST(SocketTestSuite, set_wrong_arguments) {
    // Test Unit. Set wrong address family.
    EXPECT_THAT(
        []() {
            CSocket sockObj((sa_family_t)-1, SOCK_STREAM);
            sockObj.load();
        },
        ThrowsMessage<std::invalid_argument>(
            HasSubstr("] EXCEPTION MSG1015: ")));

    // Test Unit. Set wrong socket type.
    EXPECT_THAT(
        []() {
            CSocket sockObj(PF_INET6, -1);
            sockObj.load();
        },
        ThrowsMessage<std::invalid_argument>(
            HasSubstr("] EXCEPTION MSG1016: ")));
}

TEST_F(SocketMockFTestSuite, get_socket_fails) {
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    // Inject the mocking objects into the tested code.
    umock::Sys_socket sys_socket_injectObj(&m_sys_socketObj);

    // Mock to get an invalid socket id
    EXPECT_CALL(m_sys_socketObj, socket(PF_INET6, SOCK_STREAM, 0))
        .Times(Between(1, 2)) // EXPECT_THAT calls second time if fails
        .WillRepeatedly(SetErrnoAndReturn(EINVALP, INVALID_SOCKET));

    // Test Unit
    ASSERT_THAT(
        [&sockObj]() { sockObj.load(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1017: ")));
}

TEST_F(SocketMockFTestSuite, set_option_reuseaddr_fails) {
    // sfd is given to the Unit and it will close it.
    SOCKET sfd = socket(PF_INET6, SOCK_STREAM, 0);
    ASSERT_NE(sfd, INVALID_SOCKET);

    CSocket sockObj(PF_INET6, SOCK_STREAM);
    umock::Sys_socket sys_socket_injectObj(&m_sys_socketObj);

    // Mock system function.
    EXPECT_CALL(m_sys_socketObj, socket(PF_INET6, SOCK_STREAM, 0))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(Return(sfd));
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, _, _))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(SetErrnoAndReturn(EBADFP, SOCKET_ERROR));

    // Test Unit
    EXPECT_THAT(
        [&sockObj]() { sockObj.load(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1018: ")));
}

#ifdef _MSC_VER
TEST_F(SocketMockFTestSuite, set_win32_option_exl_addr_use_fails) {
    // sfd is given to the Unit and it will close it.
    SOCKET sfd = socket(PF_INET6, SOCK_STREAM, 0);
    ASSERT_NE(sfd, INVALID_SOCKET);

    CSocket sockObj(PF_INET6, SOCK_STREAM);
    umock::Sys_socket sys_socket_injectObj(&m_sys_socketObj);

    // Mock system function.
    EXPECT_CALL(m_sys_socketObj, socket(PF_INET6, SOCK_STREAM, 0))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(Return(sfd));
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, _, _))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(Return(0));
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sfd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, _, _))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(SetErrnoAndReturn(EBADFP, SOCKET_ERROR));

    // Test Unit
    EXPECT_THAT(
        [&sockObj]() { sockObj.load(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1019: ")));
}
#endif

TEST(SocketTestSuite, get_addr_str_ipv6_successful) {
    // Get a socket and bind it to the local address.
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    ASSERT_NO_THROW(sockObj.bind("", "8080", AI_PASSIVE));

    // Test Unit
    EXPECT_EQ(sockObj.netaddr(), "[::]");
    EXPECT_EQ(sockObj.netaddrp(), "[::]:8080");
}

TEST(SocketTestSuite, get_addr_str_ipv4_successful) {
    // Get a socket and bind it to the local address.
    CSocket sockObj(PF_INET, SOCK_DGRAM);
    ASSERT_NO_THROW(sockObj.load());
    ASSERT_NO_THROW(sockObj.bind("", "8080", AI_PASSIVE));

    // Test Unit
    EXPECT_EQ(sockObj.netaddr(), "0.0.0.0");
    EXPECT_EQ(sockObj.netaddrp(), "0.0.0.0:8080");
}

TEST(SocketTestSuite, get_addr_str_from_invalid_socket) {
    // Test Unit wit empty socket.
    CSocket sockObj;
    EXPECT_THAT(
        [&sockObj]() { sockObj.netaddr(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1001: ")));
    EXPECT_THAT([&sockObj]() { sockObj.netaddrp(); },
                ThrowsMessage<std::runtime_error>(
                    // Different on MacOS with MSG1001.
                    AnyOf(HasSubstr("] EXCEPTION MSG1057: "),
                          HasSubstr("] EXCEPTION MSG1001: "))));
}

TEST(SocketTestSuite, get_addr_str_from_unbound_socket) {
    // Get a valid socket but do not bind it to an address.
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    EXPECT_EQ(sockObj.netaddr(), "[::]");
    EXPECT_EQ(sockObj.netaddrp(), "[::]:0");
    EXPECT_EQ(sockObj.get_port(), 0);
}

TEST(SocketTestSuite, get_addr_str_syscall_fail) {
    // Get a socket and bind it to the local address.
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    ASSERT_NO_THROW(sockObj.bind("", "8080", AI_PASSIVE));

    // Mock system function getsockname().
    umock::Sys_socketMock sys_socketObj;
    umock::Sys_socket sys_socket_injectObj(&sys_socketObj);
    EXPECT_CALL(
        sys_socketObj,
        getsockname((SOCKET)sockObj, _, Pointee((int)sizeof(sockaddr_storage))))
        // Different on MacOS with 3 times.
        .Times(Between(2, 3))
        .WillRepeatedly(SetErrnoAndReturn(ENOBUFS, SOCKET_ERROR));

    // Test Unit
    EXPECT_THAT(
        [&sockObj]() { sockObj.netaddr(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1001: ")));
    EXPECT_THAT([&sockObj]() { sockObj.netaddrp(); },
                ThrowsMessage<std::runtime_error>(
                    // Different on MacOS with MSG1001.
                    AnyOf(HasSubstr("] EXCEPTION MSG1057: "),
                          HasSubstr("] EXCEPTION MSG1001: "))));
}

TEST(SocketTestSuite, get_addr_str_invalid_address_family) {
    // Get a socket and bind it to the local address.
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    ASSERT_NO_THROW(sockObj.bind("", "8080", AI_PASSIVE));

    // Provide invalid address family.
    ::sockaddr_storage ss{};
    ss.ss_family = static_cast<sa_family_t>(255);

    // Mock system function
    umock::Sys_socketMock sys_socketObj;
    umock::Sys_socket sys_socket_injectObj(&sys_socketObj);
    EXPECT_CALL(
        sys_socketObj,
        getsockname(static_cast<SOCKET>(sockObj), _,
                    Pointee(static_cast<int>(sizeof(::sockaddr_storage)))))
        .Times(4)
        .WillRepeatedly(DoAll(
            SetArgPointee<1>(*reinterpret_cast<sockaddr*>(&ss)), Return(0)));

    // Test Unit
    bool g_dbug_old = g_dbug;
    CaptureStdOutErr captureObj(UPnPsdk::log_fileno);
    g_dbug = false;
    captureObj.start();
    EXPECT_EQ(sockObj.netaddr(), "");
    EXPECT_THAT(
        captureObj.str(),
        AnyOf("",
              ContainsStdRegex(
                  "^TRACE\\[.*\\].* Executing CSocket_basic::netaddr\\(\\)")));
    g_dbug = true;
    captureObj.start();
    EXPECT_EQ(sockObj.netaddr(), "");
    EXPECT_THAT(captureObj.str(),
                HasSubstr("] ERROR MSG1129: Unsupported address family 255"));

    g_dbug = false;
    captureObj.start();
    EXPECT_EQ(sockObj.netaddrp(), "");
    EXPECT_THAT(captureObj.str(),
                AnyOf("", ContainsStdRegex("^TRACE\\[.*\\].* Executing "
                                           "CSocket_basic::netaddrp\\(\\)")));
    g_dbug = true;
    captureObj.start();
    EXPECT_EQ(sockObj.netaddrp(), "");
    EXPECT_THAT(captureObj.str(),
                HasSubstr("] ERROR MSG1129: Unsupported address family 255"));
    g_dbug = g_dbug_old;
}

TEST(SocketBindTestSuite, bind_ipv6_successful) {
    // Create an unbound socket object
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    // Default IPV6_V6ONLY setting is different on different platforms but
    // doesn't matter; will be set before binding.
#ifdef _MSC_VER
    EXPECT_TRUE(sockObj.is_v6only());
    // Can be modified with PF_INET6 and before binding
    EXPECT_NO_THROW(sockObj.set_v6only(false));
    EXPECT_FALSE(sockObj.is_v6only());
#else
    EXPECT_FALSE(sockObj.is_v6only());
    // Can be modified with PF_INET6 and before binding
    EXPECT_NO_THROW(sockObj.set_v6only(true));
    EXPECT_TRUE(sockObj.is_v6only());
    EXPECT_NO_THROW(sockObj.set_v6only(false));
    EXPECT_FALSE(sockObj.is_v6only());
#endif

    // Test Unit.
    // This binds the local address to the socket and sets IPV6_V6ONLY always
    // to true which cannot be modified afterwards.
    ASSERT_NO_THROW(sockObj.bind("[::1]", "8080"));
    EXPECT_TRUE(sockObj.is_v6only());
    EXPECT_NO_THROW(sockObj.set_v6only(false));
    // v6only is true because it is a socket property that's of domain PF_INET6.
    EXPECT_TRUE(sockObj.is_v6only());

    EXPECT_EQ(sockObj.get_family(), PF_INET6);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_STREAM);
    EXPECT_EQ(sockObj.netaddr(), "[::1]");
    EXPECT_EQ(sockObj.netaddrp(), "[::1]:8080");
    EXPECT_EQ(sockObj.get_port(), 8080);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    EXPECT_TRUE(sockObj.is_bound());
    EXPECT_FALSE(sockObj.is_listen());
}

#ifndef _MSC_VER
TEST(SocketBindTestSuite, bind_ipv4_successful) {
    // Create an unbound socket object
    CSocket sockObj(PF_INET, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    // Default setting
    EXPECT_FALSE(sockObj.is_v6only());
    // Cannot be set on PF_INET
    EXPECT_NO_THROW(sockObj.set_v6only(true));
    EXPECT_FALSE(sockObj.is_v6only());

    // Test Unit.
    // This binds the local address to the socket.
    ASSERT_NO_THROW(sockObj.bind("127.0.0.1", "8080"));
    EXPECT_FALSE(sockObj.is_v6only());
    // Can never be set after binding
    EXPECT_NO_THROW(sockObj.set_v6only(true));
    EXPECT_FALSE(sockObj.is_v6only());

    EXPECT_EQ(sockObj.get_family(), PF_INET);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_STREAM);
    EXPECT_EQ(sockObj.netaddr(), "127.0.0.1");
    EXPECT_EQ(sockObj.netaddrp(), "127.0.0.1:8080");
    EXPECT_EQ(sockObj.get_port(), 8080);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    EXPECT_TRUE(sockObj.is_bound());
    EXPECT_FALSE(sockObj.is_listen());
}

#else // bind on win32
TEST(SocketBindTestSuite, bind_ipv4_successful) {
    // On win32 option IPV6_V6ONLY is set by default and can never be modified
    // when using PF_INET. This is also accepted by bind and means that you
    // find an IPv4 socket with IPV6_V6ONLY option set, that doesn't make
    // sense. I think win32 ignores the option in this case.

    // Create an unbound socket object
    CSocket sockObj(PF_INET, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    // Default setting
    EXPECT_TRUE(sockObj.is_v6only());
    // Option cannot be modified
    EXPECT_NO_THROW(sockObj.set_v6only(false));
    EXPECT_TRUE(sockObj.is_v6only());

    // Test Unit.
    // This binds the local address to the socket.
    ASSERT_NO_THROW(sockObj.bind("127.0.0.1", "8080"));
    EXPECT_TRUE(sockObj.is_v6only()); // Doesn't make sense
    // Can never be mondified after binding
    EXPECT_NO_THROW(sockObj.set_v6only(false));
    EXPECT_TRUE(sockObj.is_v6only());

    EXPECT_EQ(sockObj.get_family(), PF_INET);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_STREAM);
    EXPECT_EQ(sockObj.netaddr(), "127.0.0.1");
    EXPECT_EQ(sockObj.netaddrp(), "127.0.0.1:8080");
    EXPECT_EQ(sockObj.get_port(), 8080);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    EXPECT_TRUE(sockObj.is_bound());
    EXPECT_FALSE(sockObj.is_listen());
}
#endif

TEST_F(SocketMockFTestSuite, v6only_set_with_bind_fails) {
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    SOCKET sfd = sockObj;

    SSockaddr ss6Obj; // Use a socket address
    ss6Obj = "[::1]:8080";

    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sfd, IPPROTO_IPV6, IPV6_V6ONLY, _, _))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(SetErrnoAndReturn(EBADFP, SOCKET_ERROR));

    EXPECT_CALL(
        m_sys_socketObj,
        getsockname(sfd, _,
                    Pointee(Ge(static_cast<socklen_t>(sizeof(sockaddr_in6))))))
        .Times(Between(1, 2)) // EXPECT_THAT() calls second time if it fails.
        .WillRepeatedly(
            DoAll(SetArgPointee<1>(*reinterpret_cast<sockaddr*>(&ss6Obj.ss)),
                  Return(0)));

    // Inject mocking functions
    umock::Sys_socket sys_socket_injectObj(&m_sys_socketObj);

    // Test Unit
    EXPECT_THAT(
        ([&sockObj, &ss6Obj]() {
            sockObj.bind(ss6Obj.netaddr(), std::to_string(ss6Obj.get_port()));
        }),
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1007: ")));
}

TEST(SocketBindTestSuite, bind_to_next_free_port_successful) {
    // With empty service the operating system returns next free port number.
    // Create an unbound socket object
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());

    // Test Unit.
    ASSERT_NO_THROW(sockObj.bind("[::1]", ""));

    EXPECT_EQ(sockObj.get_family(), PF_INET6);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_STREAM);
    EXPECT_EQ(sockObj.netaddr(), "[::1]");
    // Next free port number but never 0.
    EXPECT_GT(sockObj.get_port(), 0);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    // v6only is true because it is a socket property that's of domain PF_INET6.
    EXPECT_TRUE(sockObj.is_v6only());
    EXPECT_TRUE(sockObj.is_bound());
    EXPECT_FALSE(sockObj.is_listen());
}

TEST(SocketBindTestSuite, bind_only_service_passive_successful) {
    // Create an unbind socket object.
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    sockObj.set_v6only(false);

    // Test Unit.
    ASSERT_NO_THROW(sockObj.bind("", "8080", AI_PASSIVE));

    EXPECT_EQ(sockObj.get_family(), PF_INET6);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_STREAM);
    // With AI_PASSIVE setting (for listening) the presented address is the
    // unknown address. When using this to listen, it will listen on all local
    // network interfaces.
    EXPECT_EQ(sockObj.netaddr(), "[::]");
    EXPECT_EQ(sockObj.netaddrp(), "[::]:8080");
    EXPECT_EQ(sockObj.get_port(), 8080);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    // With passive mode the preset IPV6_V6ONLY value isn't modified by the
    // operating system.
    EXPECT_TRUE(sockObj.is_v6only());
    EXPECT_TRUE(sockObj.is_bound());
    EXPECT_FALSE(sockObj.is_listen());
}

TEST(SocketBindTestSuite, bind_only_service_not_passive_successful) {
    // Create an unbind socket object.
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    sockObj.set_v6only(false);

    // Test Unit.
    ASSERT_NO_THROW(sockObj.bind("", "8080"));

    EXPECT_EQ(sockObj.get_family(), PF_INET6);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_STREAM);
    // With empty node the loopback address is selected.
    EXPECT_EQ(sockObj.netaddr(), "[::1]");
    EXPECT_EQ(sockObj.netaddrp(), "[::1]:8080");
    EXPECT_EQ(sockObj.get_port(), 8080);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    // v6only is always set true with PF_INET6 and active mode (AI_PASSIVE not
    // set).
    EXPECT_TRUE(sockObj.is_v6only());
    EXPECT_TRUE(sockObj.is_bound());
    EXPECT_FALSE(sockObj.is_listen());
}

TEST(SocketBindTestSuite, bind_with_empty_socket_fails) {
    // Test Unit. Binding an empty socket object will fail.
    EXPECT_THAT(
        []() {
            CSocket sockObj;
            sockObj.bind("", "8080", AI_PASSIVE);
        },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1001: ")));
}

TEST(SocketBindTestSuite, bind_two_times_different_addresses_fail) {
    // Binding a socket two times isn't possible. The socket must be
    // shutdown/closed before bind it again.
    // Provide a socket object
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());

    // Test Unit.
    ASSERT_NO_THROW(sockObj.bind("", "8080", AI_PASSIVE));

    // Try to bind the socket a second time to another address.
    EXPECT_THAT(
        ([&sockObj]() { sockObj.bind("", "8081", AI_PASSIVE); }),
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1007: ")));
}

TEST_F(SocketMockFTestSuite, bind_to_address_fails) {
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    SOCKET sfd = sockObj;

    SSockaddr ss6Obj; // Local socket address to bind to
    ss6Obj = "[::1]:8080";

    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sfd, IPPROTO_IPV6, IPV6_V6ONLY, _, _))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(Return(0));

    EXPECT_CALL(m_sys_socketObj, getsockopt(sfd, SOL_SOCKET, SO_TYPE, _, _))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(
            DoAll(SetArgPtrIntValue<3>(sockObj.get_socktype()), Return(0)));

    EXPECT_CALL(
        m_sys_socketObj,
        getsockname(sfd, _,
                    Pointee(Ge(static_cast<socklen_t>(sizeof(sockaddr_in6))))))
        .Times(Between(1, 2)) // EXPECT_THAT() calls second time if it fails.
        .WillRepeatedly(
            DoAll(SetArgPointee<1>(*reinterpret_cast<sockaddr*>(&ss6Obj.ss)),
                  Return(0)));

    EXPECT_CALL(m_sys_socketObj, bind(sfd, _, _))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(SetErrnoAndReturn(EBADFP, SOCKET_ERROR));

    // Inject mocking functions
    umock::Sys_socket sys_socket_injectObj(&m_sys_socketObj);

    // Test Unit
    EXPECT_THAT(
        ([&sockObj, &ss6Obj]() {
            sockObj.bind(ss6Obj.netaddr(), std::to_string(ss6Obj.get_port()));
        }),
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1008: ")));
}

#if 0  // Don't enable next test permanent!
       // It is very expensive and do not really test a Unit. It's only for
       // humans to show whats going on.
// On Microsoft Windows there is an issue with binding an address to a socket
// in conjunction with the socket option SO_EXCLUSIVEADDRUSE that I use for
// security reasons. Even on successful new opened socket file descriptors it
// may be possible that we cannot bind an unused socket address to it. For
// details of this have a look at: [SO_EXCLUSIVEADDRUSE socket option]
// (https://learn.microsoft.com/en-us/windows/win32/winsock/so-exclusiveaddruse)
// The following test examins the situation. It tries to bind all free user
// application socket numbers from 49152 to 65635 and shows with which port
// number it fails. It seems the problem exists only for this port range.
// For example Reusing port 8080 or 8081 works as expected.
#ifdef _MSC_VER
TEST(SocketBindTestSuite, check_binding_passive_all_free_ports) {
    // Binding a socket again is possible after destruction of the socket that
    // shutdown/close it.
    in_port_t port{49152};

    std::cout << "DEBUG\! start port = " << port << "\n";
    for (; port < 65535; port++) {
        CSocket sockObj;
        sockObj.load(PF_INET6, SOCK_STREAM);
        try {
            sockObj.bind("", std::to_string(port), AI_PASSIVE);
        } catch (const std::runtime_error& e) {
            std::cout << "DEBUG\! port " << port << ": ";
            std::cout << e.what() << "\n";
        }
    }

    std::cout << "DEBUG\! finished port = " << port << "\n";
}
#endif // MSC_VER
#endif // if 0

#ifdef _MSC_VER
TEST(SocketBindTestSuite, set_unset_bind_win32_same_address_multiple_times) {
    // Binding a socket again is possible after destruction of the socket that
    // shutdown/close it. But we have to take attention about the issue on
    // win32 with the SO_EXCLUSIVEADDRUSE socket option. For details look at
    // the previous test.
    // Get local interface address.
    in_port_t port = portno(random);

    // Test Unit
    for (int i{0}; i < 2; i++) {
        CSocket sockObj(PF_INET6, SOCK_STREAM);
        ASSERT_NO_THROW(sockObj.load());
        try {
            sockObj.bind("", std::to_string(port), AI_PASSIVE);
            // std::cout << "DEBUG\! Success port " << port << "\n";
        } catch ([[maybe_unused]] const std::runtime_error& e) {
            // std::cout << "DEBUG\! port " << port << ": " << e.what() << "\n";
            // Continue with a new port number.
            port = portno(random);
            // std::this_thread::sleep_for(std::chrono::seconds(5)); // DEBUG\!
        }
    }

    // std::cout << "DEBUG\! Success port " << port << "\n";
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    ASSERT_NO_THROW(sockObj.bind("", std::to_string(port), AI_PASSIVE));
}

#else  // _MSC_VER

TEST(SocketBindTestSuite, set_unset_bind_unix_same_address_multiple_times) {
    // Binding a socket again is possible after destruction of the socket that
    // shutdown/close it.

    // Test Unit
    {
        CSocket sockObj(PF_INET6, SOCK_STREAM);
        ASSERT_NO_THROW(sockObj.load());
        ASSERT_NO_THROW(sockObj.bind("", "8080", AI_PASSIVE));
    }
    {
        CSocket sockObj(PF_INET6, SOCK_STREAM);
        ASSERT_NO_THROW(sockObj.load());
        ASSERT_NO_THROW(sockObj.bind("", "8080", AI_PASSIVE));
    }
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    ASSERT_NO_THROW(sockObj.bind("", "8080", AI_PASSIVE));
}
#endif // _MSC_VER

TEST(SocketBindTestSuite, bind_same_address_fails) {
    // Test Unit
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    ASSERT_NO_THROW(sockObj.bind("", "8080", AI_PASSIVE));

    // Doing the same again will fail.
    EXPECT_THAT(
        [&sockObj]() { sockObj.bind("", "8080", AI_PASSIVE); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1007: ")));
}

TEST(SocketBindTestSuite, listen_to_same_address_multiple_times) {
    // Listen on the same address again of a valid socket is possible and should
    // do nothing.

    // Test Unit
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    ASSERT_NO_THROW(sockObj.bind("", "8080", AI_PASSIVE));
    ASSERT_NO_THROW(sockObj.listen());

    EXPECT_NO_THROW(sockObj.listen());
}

TEST_F(SocketMockFTestSuite, listen_fails) {
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    SOCKET sfd = sockObj;

    EXPECT_CALL(m_sys_socketObj, listen(sfd, 1))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(SetErrnoAndReturn(EBADFP, SOCKET_ERROR));

    // Inject mocking functions
    umock::Sys_socket sys_socket_injectObj(&m_sys_socketObj);

    // Test Unit
    EXPECT_THAT(
        [&sockObj]() { sockObj.listen(); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1034: ")));
}

TEST(SocketV6onlyTestSuite, v6only_modify_pf_inet6_stream_successful) {
    // Get socket
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());

    // Set option IPV6_V6ONLY. Of course this can only be done with PF_INET6 on
    // an unbound socket.
    // Test Unit
    ASSERT_NO_THROW(sockObj.set_v6only(true));
    EXPECT_TRUE(sockObj.is_v6only());
    ASSERT_NO_THROW(sockObj.set_v6only(false));
    EXPECT_FALSE(sockObj.is_v6only());
}

TEST_F(SocketMockFTestSuite, v6only_modify_fails) {
    // Try to set option IPV6_V6ONLY. Of course this can only be done with
    // PF_INET6 on an unbound socket.

    // Get socket
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    SOCKET sfd = sockObj;

    SSockaddr ss6Obj; // for getsockname() IPv6 return sockaddr & port
    ss6Obj = "[::1]:0";

    EXPECT_CALL(
        m_sys_socketObj,
        getsockname(sfd, _,
                    Pointee(Ge(static_cast<socklen_t>(sizeof(sockaddr_in6))))))
        .Times(Between(2, 4)) // EXPECT_THAT() calls second time if it fails.
        .WillRepeatedly(
            DoAll(SetArgPointee<1>(*reinterpret_cast<sockaddr*>(&ss6Obj.ss)),
                  Return(0)));

    EXPECT_CALL(m_sys_socketObj, setsockopt(_, IPPROTO_IPV6, IPV6_V6ONLY, _, _))
        .Times(Between(1, 2)) // EXPECT_THAT() calls second time if it fails.
        .WillRepeatedly(SetErrnoAndReturn(EBADFP, SOCKET_ERROR));

    // Inject mocking functions
    umock::Sys_socket sys_socket_injectObj(&m_sys_socketObj);

    // Test Unit
    EXPECT_THAT(
        [&sockObj]() { sockObj.set_v6only(true); },
        ThrowsMessage<std::runtime_error>(HasSubstr("] EXCEPTION MSG1006: ")));
}

TEST(SocketV6onlyTestSuite, v6only_af_inet6_dgram) {
    // With PF_INET6 and SOCK_DGRAM the flag IP6_V6ONLY can be modified.

    // Get socket
    CSocket sockObj(PF_INET6, SOCK_DGRAM);
    ASSERT_NO_THROW(sockObj.load());
    // Default IPV6_V6ONLY setting is different on different platforms but
    // doesn't matter; will be set before binding.
#ifdef _MSC_VER
    EXPECT_TRUE(sockObj.is_v6only());
    ASSERT_NO_THROW(sockObj.set_v6only(false));
#else
    EXPECT_FALSE(sockObj.is_v6only());
#endif

    // Set option IPV6_V6ONLY. This is always ignored.
    // Test Unit
    ASSERT_NO_THROW(sockObj.set_v6only(true));
    EXPECT_TRUE(sockObj.is_v6only());
}

TEST(SocketV6onlyTestSuite, v6only_af_inet_stream) {
    // With PF_INET the default setting of flag IP6_V6ONLY can not be modified.

    // Get socket
    CSocket sockObj(PF_INET, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());

#ifdef _MSC_VER
    // Default setting
    EXPECT_TRUE(sockObj.is_v6only());
    EXPECT_NO_THROW(sockObj.set_v6only(false));
    // But cannot be modified with PF_INET
    EXPECT_TRUE(sockObj.is_v6only());
#else
    // Default setting
    EXPECT_FALSE(sockObj.is_v6only());
    EXPECT_NO_THROW(sockObj.set_v6only(true));
    // But cannot be modified with PF_INET
    EXPECT_FALSE(sockObj.is_v6only());
#endif
}

TEST(SocketV6onlyTestSuite, v6only_af_inet_dgram) {
    // With PF_INET the default setting of flag IP6_V6ONLY can not be modified.

    // Get socket
    CSocket sockObj(PF_INET, SOCK_DGRAM);
    ASSERT_NO_THROW(sockObj.load());

#ifdef _MSC_VER
    // Default setting
    EXPECT_TRUE(sockObj.is_v6only());
    EXPECT_NO_THROW(sockObj.set_v6only(false));
    // But cannot be modified with PF_INET
    EXPECT_TRUE(sockObj.is_v6only());
#else
    // Default setting
    EXPECT_FALSE(sockObj.is_v6only());
    EXPECT_NO_THROW(sockObj.set_v6only(true));
    // But cannot be modified with PF_INET
    EXPECT_FALSE(sockObj.is_v6only());
#endif
}

TEST(SocketV6onlyTestSuite, v6only_on_bound_af_inet6_stream_socket) {
    // Create an unbound socket object
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    // Default IPV6_V6ONLY setting is different on different platforms but
    // doesn't matter; will be set before binding.
#ifdef _MSC_VER
    EXPECT_TRUE(sockObj.is_v6only());
#else
    EXPECT_FALSE(sockObj.is_v6only());
#endif

    // This binds the local address to the socket and always sets ipv6_v6only
    // because we bind to an PF_INET6 protocol/socket.
    ASSERT_NO_THROW(sockObj.bind("[::1]", "8080"));
    EXPECT_TRUE(sockObj.is_v6only());

    // Test Unit
    EXPECT_NO_THROW(sockObj.set_v6only(false));

    // Nothing has changed, the socket is valid.
    EXPECT_EQ(sockObj.get_family(), PF_INET6);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_STREAM);
    EXPECT_EQ(sockObj.netaddr(), "[::1]");
    EXPECT_EQ(sockObj.netaddrp(), "[::1]:8080");
    EXPECT_EQ(sockObj.get_port(), 8080);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    // Not set to false because the socket is already bound to an PF_INET6.
    EXPECT_TRUE(sockObj.is_v6only());
    EXPECT_TRUE(sockObj.is_bound());
    EXPECT_FALSE(sockObj.is_listen());
}

TEST(SocketV6onlyTestSuite, v6only_on_bound_af_inet6_dgram_socket) {
    // Create an unbound socket object
    CSocket sockObj(PF_INET6, SOCK_DGRAM);
    ASSERT_NO_THROW(sockObj.load());
    // Default IPV6_V6ONLY setting is different on different platforms.
#ifdef _MSC_VER
    EXPECT_TRUE(sockObj.is_v6only());
#else
    EXPECT_FALSE(sockObj.is_v6only());
#endif

    // This binds the local address to the socket and always sets ipv6_v6only
    // because we bind to an PF_INET6 protocol/socket.
    ASSERT_NO_THROW(sockObj.bind("[::1]", "8080"));
    EXPECT_TRUE(sockObj.is_v6only());

    // Test Unit
    EXPECT_NO_THROW(sockObj.set_v6only(false));

    // Nothing has changed, the socket is valid.
    EXPECT_EQ(sockObj.get_family(), PF_INET6);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_DGRAM);
    EXPECT_EQ(sockObj.netaddr(), "[::1]");
    EXPECT_EQ(sockObj.netaddrp(), "[::1]:8080");
    EXPECT_EQ(sockObj.get_port(), 8080);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    // Not set to false because the socket is already bound to an PF_INET6.
    EXPECT_TRUE(sockObj.is_v6only());
    EXPECT_TRUE(sockObj.is_bound());
    EXPECT_FALSE(sockObj.is_listen());
}

TEST(SocketV6onlyTestSuite, v6only_on_bound_pf_inet_stream_socket) {
    // The IPV6_V6ONLY option isn't touched with binding to PF_INET. The
    // default setting of the platform was retained.

    // Create an unbound socket object
    CSocket sockObj(PF_INET, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    // Default IPV6_V6ONLY setting is different on different platforms.
#ifdef _MSC_VER
    EXPECT_TRUE(sockObj.is_v6only());
#else
    EXPECT_FALSE(sockObj.is_v6only());
#endif

    // This binds the local address to the socket and never modify ipv6_v6only
    // because we bind to an PF_INET protocol/socket.
    ASSERT_NO_THROW(sockObj.bind("127.0.0.1", "8080"));
#ifdef _MSC_VER
    // The default setting hasn't changed but should be ignored by the
    // operating system with PF_INET.
    EXPECT_TRUE(sockObj.is_v6only());
#else
    EXPECT_FALSE(sockObj.is_v6only());
#endif

    // Test Unit
    // This is silently ignored.
    EXPECT_NO_THROW(sockObj.set_v6only(true));

    // Nothing has changed, the socket is valid.
    EXPECT_EQ(sockObj.get_family(), PF_INET);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_STREAM);
    EXPECT_EQ(sockObj.netaddr(), "127.0.0.1");
    EXPECT_EQ(sockObj.netaddrp(), "127.0.0.1:8080");
    EXPECT_EQ(sockObj.get_port(), 8080);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    EXPECT_TRUE(sockObj.is_bound());
#ifdef _MSC_VER
    // The default setting hasn't changed but should be ignored by the
    // operating system with PF_INET.
    EXPECT_TRUE(sockObj.is_v6only());
#else
    EXPECT_FALSE(sockObj.is_v6only());
#endif
}

TEST(SocketV6onlyTestSuite, v6only_on_bound_af_inet_dgram_socket) {
    // Create an unbound socket object
    CSocket sockObj(PF_INET, SOCK_DGRAM);
    ASSERT_NO_THROW(sockObj.load());
    // Default IPV6_V6ONLY setting is different on different platforms but
    // doesn't matter; will be set before binding.
#ifdef _MSC_VER
    EXPECT_TRUE(sockObj.is_v6only());
#else
    EXPECT_FALSE(sockObj.is_v6only());
#endif

    // This binds the local address to the socket but doesn't touch default
    // ipv6_v6only because we bind to an PF_INET protocol/socket.
    ASSERT_NO_THROW(sockObj.bind("127.0.0.1", "8080"));
#ifdef _MSC_VER
    EXPECT_TRUE(sockObj.is_v6only());
#else
    EXPECT_FALSE(sockObj.is_v6only());
#endif

    // Test Unit
    // This is silently ignored.
    EXPECT_NO_THROW(sockObj.set_v6only(true));

    // Nothing has changed, the socket is valid.
    EXPECT_EQ(sockObj.get_family(), PF_INET);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_DGRAM);
    EXPECT_EQ(sockObj.netaddr(), "127.0.0.1");
    EXPECT_EQ(sockObj.netaddrp(), "127.0.0.1:8080");
    EXPECT_EQ(sockObj.get_port(), 8080);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    // Setting should be ignored by the operating system with PF_INET.
#ifdef _MSC_VER
    EXPECT_TRUE(sockObj.is_v6only());
#else
    EXPECT_FALSE(sockObj.is_v6only());
#endif
    EXPECT_TRUE(sockObj.is_bound());
    EXPECT_FALSE(sockObj.is_listen());
}

TEST(SocketV6onlyTestSuite, v6only_default_on_passive_af_inet6_stream_socket) {
    // Create an unbound socket object
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    // Default IPV6_V6ONLY setting is different on different platforms but
    // doesn't matter; will be set before binding.
#ifdef _MSC_VER
    EXPECT_TRUE(sockObj.is_v6only());
#else
    EXPECT_FALSE(sockObj.is_v6only());
#endif

    // Test Unit
    // This binds the local address to the socket. It sets IPV6_V6ONLY by
    // default with PF_INET6 before binding.
    ASSERT_NO_THROW(sockObj.bind("", "8080", AI_PASSIVE));
    EXPECT_TRUE(sockObj.is_v6only());

    // It cannot be modified after binding.
    EXPECT_NO_THROW(sockObj.set_v6only(false));
    EXPECT_TRUE(sockObj.is_v6only());

    // Nothing has changed, the socket is valid.
    EXPECT_EQ(sockObj.get_family(), PF_INET6);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_STREAM);
    EXPECT_EQ(sockObj.netaddr(), "[::]");
    EXPECT_EQ(sockObj.netaddrp(), "[::]:8080");
    EXPECT_EQ(sockObj.get_port(), 8080);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    EXPECT_TRUE(sockObj.is_bound());
    EXPECT_FALSE(sockObj.is_listen());
}

TEST(SocketV6onlyTestSuite, v6only_unset_on_passive_af_inet6_stream_socket) {
    // Create an unbound socket object
    CSocket sockObj(PF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sockObj.load());
    // Default IPV6_V6ONLY setting is different on different platforms but
    // doesn't matter; will be set before binding.
#ifdef _MSC_VER
    EXPECT_TRUE(sockObj.is_v6only());
    EXPECT_NO_THROW(sockObj.set_v6only(false));
#endif
    EXPECT_FALSE(sockObj.is_v6only());

    // Test Unit
    // This binds the local address to the socket. It sets IPV6_V6ONLY by
    // default with PF_INET6 before binding.
    ASSERT_NO_THROW(sockObj.bind("", "8080", AI_PASSIVE));
    EXPECT_TRUE(sockObj.is_v6only());

    // It cannot be modified afer binding.
    EXPECT_NO_THROW(sockObj.set_v6only(false));
    EXPECT_TRUE(sockObj.is_v6only());

    // Nothing has changed, the socket is valid.
    EXPECT_EQ(sockObj.get_family(), PF_INET6);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_STREAM);
    EXPECT_EQ(sockObj.netaddr(), "[::]");
    EXPECT_EQ(sockObj.netaddrp(), "[::]:8080");
    EXPECT_EQ(sockObj.get_port(), 8080);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    EXPECT_TRUE(sockObj.is_bound());
    EXPECT_FALSE(sockObj.is_listen());
}

TEST(SocketV6onlyTestSuite, v6only_on_passive_af_inet_dgram_socket) {
    // Create an unbound socket object
    CSocket sockObj(PF_INET, SOCK_DGRAM);
    ASSERT_NO_THROW(sockObj.load());
#ifdef _MSC_VER
    // This is set on win32 by default but should not matter with PF_INET.
    EXPECT_TRUE(sockObj.is_v6only());
#else
    EXPECT_FALSE(sockObj.is_v6only());
#endif

    // v6only can never be set on a protocol family PF_INET.
    EXPECT_NO_THROW(sockObj.set_v6only(true));
#ifdef _MSC_VER
    // This is set on win32 by default but should not matter with PF_INET.
    EXPECT_TRUE(sockObj.is_v6only());
#else
    EXPECT_FALSE(sockObj.is_v6only());
#endif

    // Test Unit
    // This binds the local address to the socket and always resets ipv6_v6only
    // even with passive mode because we bind to an PF_INET protocol/socket.
    ASSERT_NO_THROW(sockObj.bind("", "8080", AI_PASSIVE));
#ifdef _MSC_VER
    // This is set on win32 by default but should not matter with PF_INET.
    EXPECT_TRUE(sockObj.is_v6only());
#else
    EXPECT_FALSE(sockObj.is_v6only());
#endif

    // It cannot be set afer binding.
    EXPECT_NO_THROW(sockObj.set_v6only(true));

    // Nothing has changed, the socket is valid.
    EXPECT_EQ(sockObj.get_family(), PF_INET);
    EXPECT_EQ(sockObj.get_socktype(), SOCK_DGRAM);
    EXPECT_EQ(sockObj.netaddr(), "0.0.0.0");
    EXPECT_EQ(sockObj.netaddrp(), "0.0.0.0:8080");
    EXPECT_EQ(sockObj.get_port(), 8080);
    EXPECT_EQ(sockObj.get_sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
#ifdef _MSC_VER
    // This is set on win32 by default but should not matter with PF_INET.
    EXPECT_TRUE(sockObj.is_v6only());
#else
    EXPECT_FALSE(sockObj.is_v6only());
#endif
    EXPECT_TRUE(sockObj.is_bound());
    EXPECT_FALSE(sockObj.is_listen());
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
