// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-03-22

#include <UPnPsdk/socket.hpp>
#include <UPnPsdk/addrinfo.hpp>
#include <utest/utest.hpp>
#include <umock/sys_socket_mock.hpp>
#ifdef _MSC_VER
#include <umock/winsock2_mock.hpp>
#endif

namespace utest {

using ::testing::_;
using ::testing::AnyOf;
using ::testing::Between;
using ::testing::DoAll;
using ::testing::Ge;
using ::testing::HasSubstr;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetErrnoAndReturn;
using ::testing::StrictMock;
using ::testing::ThrowsMessage;

using ::UPnPsdk::CAddrinfo;
using ::UPnPsdk::CSocket;
using ::UPnPsdk::CSocket_basic;
using ::UPnPsdk::CSocketErr;
using ::UPnPsdk::g_dbug;
using ::UPnPsdk::SSockaddr;


namespace {

// General storage for temporary socket address evaluation
SSockaddr saddr;


// Helper function to examine what's going on with option IPV6_V6ONLY.
// -------------------------------------------------------------------
bool is_v6only(SOCKET a_sfd) {
    if (a_sfd == INVALID_SOCKET)
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG????") "Failed to get socket option "
                                         "IPV6_V6ONLY: Bad file descriptor");
    CSocketErr serrObj;

    int so_option{0};
    socklen_t len{sizeof(so_option)}; // May be modified
    // Type cast (char*)&so_option is needed for Microsoft Windows.
    int err = umock::sys_socket_h.getsockopt(
        a_sfd, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<char*>(&so_option),
        &len);
    if (err) {
        serrObj.catch_error();
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG????") "Failed to get socket option: " +
            serrObj.error_str());
    }
    return so_option;
}

} // anonymous namespace


class SocketMockFTestSuite : public ::testing::Test {
  protected:
    // Instantiate mocking objects.
    StrictMock<umock::Sys_socketMock> m_sys_socketObj;
    // Inject the mocking objects into the tested code. This starts mocking.
    // umock::Sys_socket sys_socket_injectObj{&m_sys_socketObj};
#ifdef _MSC_VER
    StrictMock<umock::Winsock2Mock> m_winsock2Obj;
    // umock::Winsock2 winsock2_injectObj{&m_winsock2Obj};
#endif

    // Constructor
    SocketMockFTestSuite() {
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
#ifdef _MSC_VER
        ON_CALL(m_winsock2Obj, WSAGetLastError())
            .WillByDefault(Return(WSAENOTSOCK));
#endif
    }
};


#if 0
// This is for humans only to check how 'connect()' to a remote host exactly
// works so we can mock it the right way. Don't enable this test permanently
// because it connects to the real internet for DNS name resolution and may
// slow down this gtest dramatically or block the test until timeout expires if
// no connection is possible.

TEST(SockTestSuite, sock_connect_to_host) {
    // Get a socket, bound to a local ip address
    CSocket sockObj;
    ASSERT_NO_THROW(sockObj.bind(SOCK_DGRAM));

    // Get the remote host socket address
    CAddrinfo ai("example.com", "echo", 0 /*flags*/, SOCK_DGRAM);
    ASSERT_NO_THROW(ai.get_first());

    // Show local and remote ip addresses
    SSockaddr saObj;
    sockObj.sockaddr(saObj);
    std::cout << "local netaddress of the socket = \"" << saObj << "\".\n";
    std::cout << "Remote netaddress to connect = \"" << ai.netaddrp()
              << "\".\n";

    // Connect to the remote host
    CSocketErr sockerrObj;
    if (::connect(sockObj, ai->ai_addr, ai->ai_addrlen) == SOCKET_ERROR) {
        sockerrObj.catch_error();
        std::cerr << "Error on connect socket: " << sockerrObj.error_str()
                  << ".\n";
        GTEST_FAIL();
    }
}
#endif


// Instantiate socket objects
// --------------------------
TEST(SocketBasicTestSuite, instantiate_socket_successful) {
    SOCKET sfd = ::socket(PF_INET6, SOCK_STREAM, 0);
    ASSERT_NE(sfd, INVALID_SOCKET);

    // Test Unit
    CSocket_basic sockObj(sfd);
    ASSERT_NO_THROW(sockObj.load()); // UPnPsdk::CSocket_basic
    sockObj.sockaddr(saddr);

    EXPECT_EQ(static_cast<SOCKET>(sockObj), sfd);
    EXPECT_EQ(sockObj.socktype(), SOCK_STREAM);
    EXPECT_EQ(saddr.netaddrp(), "[::]:0");
    EXPECT_EQ(sockObj.sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());

    CLOSE_SOCKET_P(sfd);
}

TEST(SocketBasicTestSuite, instantiate_socket_af_unix) {
    // With family AF_UNIX the types SOCK_STREAM, SOCK_DGRAM, and
    // SOCK_SEQPACKET are valid (man unix(7)). It defines a 'struct
    // sockaddr_un' (man sockaddr).
#ifdef _MSC_VER
    // Microsoft Windows only supports SOCK_STREAM for AF_UNIX
    SOCKET sfd = ::socket(AF_UNIX, SOCK_STREAM, 0);
#else
    SOCKET sfd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
#endif

    ASSERT_NE(sfd, INVALID_SOCKET);
    CSocket_basic sockObj(sfd);
    sockObj.load(); // UPnPsdk::CSocket_basic

    EXPECT_NE(static_cast<SOCKET>(sockObj), INVALID_SOCKET);
#ifdef _MSC_VER
    // Microsoft Windows only supports SOCK_STREAM for AF_UNIX
    EXPECT_EQ(sockObj.socktype(), SOCK_STREAM);
#else
    EXPECT_EQ(sockObj.socktype(), SOCK_DGRAM);
#endif
    EXPECT_EQ(sockObj.sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());

    // The socket is not bound to an address from a local network adapter so we
    // will find an unspecified address of the address family.
    EXPECT_FALSE(sockObj.is_bound());
    memset(&saddr.ss, 0xAA, sizeof(saddr.ss));
    // Next returns a UNIX domain 'struct sockaddr_un'.
    sockObj.sockaddr(saddr);
    EXPECT_EQ(saddr.ss.ss_family, AF_UNIX);
    EXPECT_EQ(saddr.sun.sun_family, AF_UNIX);
    EXPECT_STREQ(saddr.sun.sun_path, "");
    EXPECT_EQ(saddr.netaddr(), "");
    EXPECT_EQ(saddr.port(), 0);
    EXPECT_EQ(saddr.netaddrp(), ":0");

    CLOSE_SOCKET_P(sfd);
}

TEST(SocketBasicTestSuite, instantiate_invalid_socket_fd) {
    CSocket_basic sockObj(INVALID_SOCKET);

    // Test Unit
    EXPECT_THAT([&sockObj]() { sockObj.load(); }, // UPnPsdk::CSocket_basic
                ThrowsMessage<std::runtime_error>(
                    HasSubstr("UPnPsdk MSG1014 EXCEPT[")));
}

TEST(SocketBasicTestSuite, instantiate_socket_load_two_times) {
    SOCKET sfd = ::socket(PF_INET6, SOCK_STREAM, 0);
    ASSERT_NE(sfd, INVALID_SOCKET);

    // Test Unit
    CSocket_basic sockObj(sfd);
    sockObj.load(); // UPnPsdk::CSocket_basic
    sockObj.load(); // UPnPsdk::CSocket_basic
    sockObj.sockaddr(saddr);

    EXPECT_EQ(static_cast<SOCKET>(sockObj), sfd);
    EXPECT_EQ(sockObj.socktype(), SOCK_STREAM);
    EXPECT_EQ(saddr.netaddrp(), "[::]:0");
    EXPECT_EQ(sockObj.sockerr(), 0);
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

TEST(SocketBasicTestSuite, instantiate_empty_socket) {
    // Test Unit
    CSocket_basic sockObj;
    ASSERT_EQ(static_cast<SOCKET>(sockObj), INVALID_SOCKET);
    ASSERT_FALSE(sockObj.is_bound());

    sockObj.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), ":0");

    EXPECT_THAT([&sockObj]() { sockObj.socktype(); },
                ThrowsMessage<std::runtime_error>(
                    HasSubstr("UPnPsdk MSG1030 EXCEPT[")));
    EXPECT_THAT([&sockObj]() { sockObj.sockerr(); },
                ThrowsMessage<std::runtime_error>(
                    HasSubstr("UPnPsdk MSG1011 EXCEPT[")));
    EXPECT_THAT([&sockObj]() { sockObj.is_reuse_addr(); },
                ThrowsMessage<std::runtime_error>(
                    HasSubstr("UPnPsdk MSG1013 EXCEPT[")));
}

TEST(SocketTestSuite, instantiate_unbind_socket) {
    CSocket sockObj;
    ASSERT_EQ(static_cast<SOCKET>(sockObj), INVALID_SOCKET);
    ASSERT_FALSE(sockObj.is_bound());

    sockObj.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), ":0");

    EXPECT_THAT([&sockObj]() { sockObj.socktype(); },
                ThrowsMessage<std::runtime_error>(
                    HasSubstr("UPnPsdk MSG1030 EXCEPT[")));
    EXPECT_THAT([&sockObj]() { sockObj.sockerr(); },
                ThrowsMessage<std::runtime_error>(
                    HasSubstr("UPnPsdk MSG1011 EXCEPT[")));
    EXPECT_THAT([&sockObj]() { sockObj.is_reuse_addr(); },
                ThrowsMessage<std::runtime_error>(
                    HasSubstr("UPnPsdk MSG1013 EXCEPT[")));
    EXPECT_THAT([&sockObj]() { sockObj.is_listen(); },
                ThrowsMessage<std::runtime_error>(
                    HasSubstr("UPnPsdk MSG1035 EXCEPT[")));
}

TEST(SocketBasicTestSuite, instantiate_with_bound_raw_socket_fd) {
    saddr = "127.0.0.1:50002";
    CSocket bound_sockObj;
    // Default IPV6_V6ONLY setting is different on different platforms but
    // doesn't matter. It will be reset before binding a socket address.
    ASSERT_NO_THROW(bound_sockObj.bind(SOCK_STREAM, &saddr));
    SOCKET bound_sock = bound_sockObj;

    // Test Unit with a bound socket.
    CSocket_basic sockObj(bound_sock);
    sockObj.load(); // UPnPsdk::CSocket_basic
    SSockaddr sa_sockObj;
    sockObj.sockaddr(sa_sockObj);
    EXPECT_TRUE(sa_sockObj == saddr);

    EXPECT_EQ(sockObj.socktype(), SOCK_STREAM);
    EXPECT_EQ(sockObj.sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
}


// Move and assign sockets
// -----------------------
TEST(SocketTestSuite, move_socket_successful) {
    // Provide a socket object
    CSocket sock1;

    // Get local interface address when node is empty with flag AI_PASSIVE.
    saddr = "";
    saddr = 8080;
    ASSERT_NO_THROW(sock1.bind(SOCK_STREAM, &saddr, AI_PASSIVE));
    ASSERT_EQ(sock1.is_bound(), -1);
    sock1.sockaddr(saddr);

    SOCKET old_fd_sock1 = sock1;

    // Configure socket to listen so it will accept connections.
    ASSERT_NO_THROW(sock1.listen());

    // Test Unit, move sock1 to a new sock2
    // CSocket sock2 = sock1; // This does not compile because it needs a copy
    //                           constructor that isn't available. I restrict
    //                           to move only.
    // This moves the socket file descriptor.
    CSocket sock2{std::move(sock1)};
    sock2.sockaddr(saddr);

    // The socket file descriptor has been moved to the new object.
    EXPECT_EQ(static_cast<SOCKET>(sock2), old_fd_sock1);
    // The old object has now an invalid socket file descriptor.
    EXPECT_EQ(static_cast<SOCKET>(sock1), INVALID_SOCKET);

    // Tests of socket object (sock1) with INVALID_SOCKET see other tests.
    // Check if new socket is valid.
    EXPECT_EQ(sock2.socktype(), SOCK_STREAM);
    EXPECT_THAT(saddr.netaddrp(), AnyOf("[::]:8080", "0.0.0.0:8080"));
    EXPECT_EQ(sock2.sockerr(), 0);
    EXPECT_FALSE(sock2.is_reuse_addr());
    EXPECT_EQ(sock2.is_bound(), -1);
    EXPECT_TRUE(sock2.is_listen());
}

TEST(SocketTestSuite, assign_socket_successful) {
    // Provide first of two socket objects.
    // Get local interface address when node is empty with flag AI_PASSIVE.
    saddr = "";
    saddr = 8080;
    CSocket sock1;
    ASSERT_NO_THROW(sock1.bind(SOCK_STREAM, &saddr, AI_PASSIVE));
    ASSERT_EQ(sock1.is_bound(), -1);
    sock1.sockaddr(saddr);

    SOCKET old_fd_sock1 = sock1;

    // Configure socket to listen so it will accept connections.
    ASSERT_NO_THROW(sock1.listen());

    // Provide second empty socket.
    CSocket sock2;

    // Test Unit. We can only move. Copy a socket resource is not useful.
    sock2 = std::move(sock1);
    sock2.sockaddr(saddr);

    // The socket file descriptor has been moved to the destination object.
    EXPECT_EQ((SOCKET)sock2, old_fd_sock1);
    // The old object has now an invalid socket file descriptor.
    EXPECT_EQ((SOCKET)sock1, INVALID_SOCKET);

    // Tests of socket object (sock1) with INVALID_SOCKET see other tests.
    // Check if new socket is valid.
    EXPECT_EQ(sock2.socktype(), SOCK_STREAM);
    EXPECT_THAT(saddr.netaddrp(), AnyOf("[::]:8080", "0.0.0.0:8080"));
    EXPECT_EQ(sock2.sockerr(), 0);
    EXPECT_FALSE(sock2.is_reuse_addr());
    EXPECT_EQ(sock2.is_bound(), -1);
    EXPECT_TRUE(sock2.is_listen());
}

TEST(SocketTestSuite, move_socket_via_allocated_list) {
    /* There are some // commented outputs for humans. Uncomment them if you
     * like to see whats going on. */
    struct SSocketList {
        CSocket* pSock1Obj;
    };

    SSocketList* socket_list =
        static_cast<SSocketList*>(malloc(sizeof(SSocketList)));
    ASSERT_NE(socket_list, nullptr);

    saddr = "";
    saddr = 8080;
    CSocket sock1Obj;
    ASSERT_NO_THROW(sock1Obj.bind(SOCK_STREAM, &saddr, AI_PASSIVE));
    // sock1Obj.sockaddr(saddr);
    // std::cerr << "Source socket is bound to " << saddr.netaddrp()
    //           << ". Moving...\n";

    /* Move the socket object via pointer in allocated list. */
    socket_list->pSock1Obj = &sock1Obj;
    CSocket sock2Obj{std::move(*socket_list->pSock1Obj)};

    ASSERT_EQ((SOCKET)sock1Obj, INVALID_SOCKET);
    // sock1Obj.sockaddr(saddr);
    // std::cerr << "Source socket is now empty with " << saddr.netaddrp() <<
    // ".\n";
    ASSERT_NE((SOCKET)sock2Obj, INVALID_SOCKET);
    // sock2Obj.sockaddr(saddr);
    // std::cerr << "Moved socket is still bound to " << saddr.netaddrp() <<
    // ".\n";

    // std::cerr << "Free socket_list.\n";
    free(socket_list);
    // std::cerr << "Test finished.\n";
}


// Bind socket objects
// -------------------
TEST(SocketTestSuite, bind_ipv6only) {
    // This is to verify the situation of binding a socket with option
    // IPV6_V6ONLY. This option can only be switched on IPv6 (AF_INET6) created
    // sockets listening on the "any" address for incomming requests. For other
    // situations it isn't applicable and has different results on differnt
    // platforms after binding the socket fd to an ip address with no useful
    // common meaning.
    //
    // This SDK uses IPv6 mapped IPv4 addresses and it always reset option
    // IPV6_V6ONLY when getting a socket from the operating system to have the
    // same situation on all platforms for listening.
    // REF: [How to support both IPv4 and IPv6 connections]
    // (https://stackoverflow.com/a/1618259/5014688)

    // "any" IPv6 address, unspec. address will accept resetting IPV6_V6ONLY.
    saddr = "[::]:50001";
    CSocket sock1Obj;
    ASSERT_NO_THROW(sock1Obj.bind(SOCK_STREAM, &saddr));
    EXPECT_TRUE(sock1Obj.is_bound());
    EXPECT_FALSE(is_v6only(sock1Obj)); // resetted

    // Unicast IPv6 address.
    saddr = "[::1]:50002";
    CSocket sock2Obj;
    ASSERT_NO_THROW(sock2Obj.bind(SOCK_STREAM, &saddr));
    EXPECT_TRUE(sock2Obj.is_bound());
#ifdef __unix__
    EXPECT_TRUE(is_v6only(sock2Obj)); // Not resetted
#else
    EXPECT_FALSE(is_v6only(sock2Obj));
#endif

    // "any" IPv4 address, fails with exception.
    saddr = "0.0.0.0:50003";
    CSocket sock3Obj;
    ASSERT_NO_THROW(sock3Obj.bind(SOCK_STREAM, &saddr));
    EXPECT_TRUE(sock3Obj.is_bound());
#ifdef _WIN32
    EXPECT_TRUE(is_v6only(sock3Obj)); // Not resetted
#else
    // "Operation not supported".
    EXPECT_THROW(is_v6only(sock3Obj), std::runtime_error);
#endif

    // Unicast IPv4 address, fails with exception.
    saddr = "127.0.0.1:50004";
    CSocket sock4Obj;
    ASSERT_NO_THROW(sock4Obj.bind(SOCK_STREAM, &saddr));
    EXPECT_TRUE(sock4Obj.is_bound());
#ifdef _WIN32
    EXPECT_TRUE(is_v6only(sock4Obj)); // Not resetted
#else
    // "Operation not supported".
    EXPECT_THROW(is_v6only(sock4Obj), std::runtime_error);
#endif
}

TEST(SocketTestSuite, bind_ipv6_successful) {
    saddr = "[::1]:50001";
    CSocket sockObj;
    ASSERT_NO_THROW(sockObj.bind(SOCK_STREAM, &saddr));
    EXPECT_TRUE(sockObj.is_bound());

    // Compare bound ip address from socket with given ip address.
    SSockaddr sa_sockObj;
    sockObj.sockaddr(sa_sockObj);
    EXPECT_TRUE(sa_sockObj == saddr);

    EXPECT_EQ(sockObj.socktype(), SOCK_STREAM);
    EXPECT_EQ(sockObj.sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    EXPECT_FALSE(sockObj.is_listen());
}

TEST(SocketTestSuite, bind_ipv6_rapid_same_port_two_times) {
    saddr = "[::1]:50011";
    SSockaddr saddr2;
    saddr2 = "[::1]:50011"; // Can be modified to different saddr.
    CSocket sock2Obj;

    // Test Unit
    {
        CSocket sock1Obj;
        ASSERT_NO_THROW(sock1Obj.bind(SOCK_STREAM, &saddr));
    }
    ASSERT_NO_THROW(sock2Obj.bind(SOCK_STREAM, &saddr2));

    EXPECT_TRUE(sock2Obj.is_bound());

    // Compare bound ip address from socket with given ip address.
    SSockaddr sa_sock2Obj;
    sock2Obj.sockaddr(sa_sock2Obj);
    EXPECT_TRUE(sa_sock2Obj == saddr2);

    EXPECT_EQ(sock2Obj.socktype(), SOCK_STREAM);
    EXPECT_EQ(sock2Obj.sockerr(), 0);
    EXPECT_FALSE(sock2Obj.is_reuse_addr());
    EXPECT_FALSE(sock2Obj.is_listen());
}

TEST(SocketTestSuite, bind_ipv4_successful) {
    saddr = "127.0.0.1:50002";
    CSocket sockObj;
    ASSERT_NO_THROW(sockObj.bind(SOCK_DGRAM, &saddr));
    EXPECT_TRUE(sockObj.is_bound());

    // Compare bound ip address from socket with given ip address.
    SSockaddr sa_sockObj;
    sockObj.sockaddr(sa_sockObj);
    EXPECT_TRUE(sa_sockObj == saddr);

    EXPECT_EQ(sockObj.socktype(), SOCK_DGRAM);
    EXPECT_EQ(sockObj.sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    EXPECT_FALSE(sockObj.is_listen());
}

TEST(SocketBasicTestSuite, bind_socket_two_times_successful) {
    // Get a raw socket file descriptor
    saddr = "[::1]:50003";
    CSocket bound_sockObj;
    ASSERT_NO_THROW(bound_sockObj.bind(SOCK_DGRAM, &saddr));
    SOCKET bound_sock = bound_sockObj;

    // Test Unit with a bound socket.
    CSocket_basic sockObj(bound_sock);
    ASSERT_NO_THROW(sockObj.load()); // UPnPsdk::CSocket_basic
    ASSERT_NO_THROW(sockObj.load()); // UPnPsdk::CSocket_basic
    EXPECT_NE(static_cast<SOCKET>(sockObj), INVALID_SOCKET);

    EXPECT_TRUE(sockObj.is_bound());
    SSockaddr sa_sockObj;
    sockObj.sockaddr(sa_sockObj);
    EXPECT_TRUE(sa_sockObj == saddr);

    EXPECT_EQ(sockObj.socktype(), SOCK_DGRAM);
    EXPECT_EQ(sockObj.sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
}

TEST(SocketTestSuite, bind_default_passive_successful) {
    CSocket sockObj;
    ASSERT_NO_THROW(sockObj.bind(SOCK_DGRAM, nullptr, AI_PASSIVE));
    EXPECT_EQ(sockObj.is_bound(), -1);

    EXPECT_EQ(sockObj.socktype(), SOCK_DGRAM);
    // With AI_PASSIVE setting (for listening) the presented address is the
    // unspecified address. When using this to listen, it will listen on all
    // local network interfaces.
    sockObj.sockaddr(saddr);
    EXPECT_THAT(saddr.netaddr(), AnyOf("[::]", "0.0.0.0"));
    // A port number was given by ::bind().
    EXPECT_NE(saddr.port(), 0);
    EXPECT_EQ(sockObj.sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    EXPECT_FALSE(sockObj.is_listen());
}

TEST(SocketTestSuite, bind_only_service_passive_successful) {
    saddr = "";
    saddr = 8080;
    CSocket sockObj;
    ASSERT_NO_THROW(sockObj.bind(SOCK_STREAM, &saddr, AI_PASSIVE));
    EXPECT_EQ(sockObj.is_bound(), -1);

    EXPECT_EQ(sockObj.socktype(), SOCK_STREAM);
    // With AI_PASSIVE setting (for listening) the presented address is the
    // unspecified address. When using this to listen, it will listen on all
    // local network interfaces.
    sockObj.sockaddr(saddr);
    EXPECT_THAT(saddr.netaddrp(), AnyOf("[::]:8080", "0.0.0.0:8080"));
    EXPECT_EQ(sockObj.sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    EXPECT_FALSE(sockObj.is_listen());
}

TEST(SocketTestSuite, bind_default_not_passive_successful) {
    CSocket sockObj;
    ASSERT_NO_THROW(sockObj.bind(SOCK_DGRAM));
    EXPECT_EQ(sockObj.is_bound(), 1);

    EXPECT_EQ(sockObj.socktype(), SOCK_DGRAM);
    // With empty node the loopback address is selected.
    sockObj.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddr(), "[::1]");
    // A port number was given by ::bind().
    EXPECT_NE(saddr.port(), 0);
    EXPECT_EQ(sockObj.sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    EXPECT_FALSE(sockObj.is_listen());
}

TEST(SocketTestSuite, bind_only_service_not_passive_successful) {
    saddr = "";
    saddr = 8080;

    // Test Unit
    CSocket sockObj;
    ASSERT_NO_THROW(sockObj.bind(SOCK_STREAM, &saddr));
    EXPECT_EQ(sockObj.is_bound(), 1);

    EXPECT_EQ(sockObj.socktype(), SOCK_STREAM);
    // With empty node the loopback address is selected.
    sockObj.sockaddr(saddr);
    EXPECT_THAT(saddr.netaddrp(), AnyOf("[::1]:8080", "127.0.0.1:8080"));
    EXPECT_EQ(sockObj.sockerr(), 0);
    EXPECT_FALSE(sockObj.is_reuse_addr());
    EXPECT_FALSE(sockObj.is_listen());
}

TEST_F(SocketMockFTestSuite, bind_ipv6_lla_successful) {
    constexpr SOCKET sockfd{umock::sfd_base + 10};
    saddr = "[fe80::fedc:cdef:0:3]";

    // --- Mock get_sockfd() ---
    // Provide a socket file descriptor
    EXPECT_CALL(m_sys_socketObj, socket(PF_INET6, SOCK_STREAM, 0))
        .WillOnce(Return(sockfd));
    // Expect resetting SO_REUSEADDR.
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, _, _))
        .WillOnce(Return(0));
    // Expect setting IPV6_V6ONLY.
    EXPECT_CALL(m_sys_socketObj, setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY,
                                            PointeeVoidToConstInt(0), _))
        .WillOnce(Return(0));
#ifdef _MSC_VER
    // Expect setting SO_EXCLUSIVEADDRUSE.
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sockfd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, _, _))
        .WillOnce(Return(0));
#endif
    // --- Mock bind() ---
    // Bind socket to an ip address, provide port if port was 0.
    EXPECT_CALL(m_sys_socketObj, bind(sockfd, _, _)).WillOnce(Return(0));

    if (g_dbug) {
        EXPECT_CALL(
            m_sys_socketObj,
            getsockname(sockfd, _,
                        Pointee(Ge(static_cast<socklen_t>(sizeof(saddr.ss))))))
            .WillOnce(DoAll(StructCpyToArg<1>(&saddr.ss, sizeof(saddr.ss)),
                            SetArgPointee<2>(saddr.sizeof_saddr()), Return(0)));
    }

    // Inject mocking functions
    umock::Sys_socket sys_socket_injectObj(&m_sys_socketObj);

    // Test Unit
    CSocket sockObj;
    ASSERT_NO_THROW(sockObj.bind(SOCK_STREAM, &saddr));
}

TEST_F(SocketMockFTestSuite, bind_fails_to_get_socket) {
    // Mock to get an invalid socket id
    EXPECT_CALL(m_sys_socketObj,
                socket(AnyOf(AF_INET6, AF_INET), SOCK_STREAM, 0))
        .Times(Between(1, 2)) // EXPECT_THAT calls second time if fails
        .WillRepeatedly(SetErrnoAndReturn(EINVALP, INVALID_SOCKET));

    // Inject mocking functions
    umock::Sys_socket sys_socket_injectObj(&m_sys_socketObj);

    // Test Unit
    CSocket sockObj;
    ASSERT_THAT([&sockObj]() { sockObj.bind(SOCK_STREAM); },
                ThrowsMessage<std::runtime_error>(
                    HasSubstr("UPnPsdk MSG1017 EXCEPT[")));
}

TEST_F(SocketMockFTestSuite, bind_fails_to_set_option_reuseaddr) {
    // sfd is given to the Unit and it will close it.
    SOCKET sfd = ::socket(PF_INET6, SOCK_STREAM, 0);
    ASSERT_NE(sfd, INVALID_SOCKET);

    // Mock system functions.
    EXPECT_CALL(m_sys_socketObj,
                socket(AnyOf(AF_INET6, AF_INET), SOCK_STREAM, 0))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(Return(sfd));
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, _, _))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(SetErrnoAndReturn(EBADFP, SOCKET_ERROR));

    // Inject mocking functions
    umock::Sys_socket sys_socket_injectObj(&m_sys_socketObj);

    // Test Unit
    CSocket sockObj;
    EXPECT_THAT([&sockObj]() { sockObj.bind(SOCK_STREAM); },
                ThrowsMessage<std::runtime_error>(
                    HasSubstr("UPnPsdk MSG1018 EXCEPT[")));
}

TEST_F(SocketMockFTestSuite, bind_fails_to_set_ipv6_only) {
    // sfd is given to the Unit and it will close it.
    SOCKET sfd = ::socket(PF_INET6, SOCK_STREAM, 0);
    ASSERT_NE(sfd, INVALID_SOCKET);

    // Mock system functions.
    // Provide a socket file descriptor
    EXPECT_CALL(m_sys_socketObj,
                socket(AnyOf(AF_INET6, AF_INET), SOCK_STREAM, 0))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(Return(sfd));
    // Expect resetting SO_REUSEADDR.
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, _, _))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(Return(0));
    // Expect setting IPV6_V6ONLY.
    EXPECT_CALL(m_sys_socketObj, setsockopt(sfd, IPPROTO_IPV6, IPV6_V6ONLY,
                                            PointeeVoidToConstInt(0), _))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(SetErrnoAndReturn(EBADFP, SOCKET_ERROR));

    // Inject mocking functions
    umock::Sys_socket sys_socket_injectObj(&m_sys_socketObj);

    // Test Unit
    CSocket sockObj;
    EXPECT_THAT([&sockObj]() { sockObj.bind(SOCK_STREAM); },
                ThrowsMessage<std::runtime_error>(
                    HasSubstr("UPnPsdk MSG1007 EXCEPT[")));
}

#ifdef _MSC_VER
TEST_F(SocketMockFTestSuite, bind_fails_to_set_win32_exclusiveaddruse) {
    // sfd is given to the Unit and it will close it.
    SOCKET sfd = ::socket(PF_INET6, SOCK_STREAM, 0);
    ASSERT_NE(sfd, INVALID_SOCKET);

    // Mock system functions.
    // Provide a socket file descriptor
    EXPECT_CALL(m_sys_socketObj,
                socket(AnyOf(AF_INET6, AF_INET), SOCK_STREAM, 0))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(Return(sfd));
    // Expect resetting SO_REUSEADDR.
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, _, _))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(Return(0));
    // Expect setting IPV6_V6ONLY.
    EXPECT_CALL(m_sys_socketObj, setsockopt(sfd, IPPROTO_IPV6, IPV6_V6ONLY,
                                            PointeeVoidToConstInt(0), _))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(Return(0));
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sfd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, _, _))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(SetErrnoAndReturn(EBADFP, SOCKET_ERROR));

    // Inject mocking functions
    umock::Sys_socket sys_socket_injectObj(&m_sys_socketObj);

    // Test Unit
    CSocket sockObj;
    EXPECT_THAT([&sockObj]() { sockObj.bind(SOCK_STREAM); },
                ThrowsMessage<std::runtime_error>(
                    HasSubstr("UPnPsdk MSG1019 EXCEPT[")));
}
#endif

TEST_F(SocketMockFTestSuite, bind_syscall_fails) {
    // sfd is given to the Unit and it will close it.
    SOCKET sfd = ::socket(PF_INET6, SOCK_DGRAM, 0);
    ASSERT_NE(sfd, INVALID_SOCKET);

    // --- Mock get_sockfd() ---
    // Provide a socket file descriptor
    EXPECT_CALL(m_sys_socketObj, socket(AF_INET6, SOCK_DGRAM, 0))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(Return(sfd));
    // Expect resetting SO_REUSEADDR.
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, _, _))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(Return(0));
    // Expect setting IPV6_V6ONLY.
    EXPECT_CALL(m_sys_socketObj, setsockopt(sfd, IPPROTO_IPV6, IPV6_V6ONLY,
                                            PointeeVoidToConstInt(0), _))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(Return(0));
#ifdef _MSC_VER
    // Expect setting SO_EXCLUSIVEADDRUSE.
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sfd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, _, _))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(Return(0));
#endif
    // --- Mock bind() ---
    // Bind socket to an ip address, provide port if port was 0.
    EXPECT_CALL(m_sys_socketObj, bind(sfd, _, _))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(SetErrnoAndReturn(EBADFP, SOCKET_ERROR));

    // Inject mocking functions
    umock::Sys_socket sys_socket_injectObj(&m_sys_socketObj);

    // Test Unit
    SSockaddr saddrObj;
    saddrObj = "[2001:db8::fedc:cdef:0:4]";
    CSocket sockObj;
    EXPECT_THAT(
        ([&sockObj, &saddrObj]() { sockObj.bind(SOCK_DGRAM, &saddrObj); }),
        ThrowsMessage<std::runtime_error>(
            HasSubstr("UPnPsdk MSG1008 EXCEPT[")));
}

#ifdef _MSC_VER
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
// application socket numbers from 49152 to 65535 and shows with which port
// number it fails. It seems the problem exists only for this port range.
// For example Reusing port 8080 or 8081 works as expected.
TEST(SocketTestSuite, check_binding_passive_all_free_ports) {
    // Binding a socket again is possible after destruction of the socket that
    // shutdown/close it.
    in_port_t port{49152};
    saddr = "";

    std::cerr << "TESTOUT: start port = " << port << "\n";
    for (; port < 65535; port++) {
        // std::this_thread::sleep_for(std::chrono::milliseconds(1));
        CSocket sockObj;
        saddr = port; // Modifies only the port
        try {
            sockObj.bind(SOCK_STREAM, &saddr, AI_PASSIVE);
        } catch (const std::runtime_error& e) {
            std::cerr << "TESTOUT: port " << port << ": " << e.what() << '\n';
        }
    }

    std::cerr << "TESTOUT: finished port = " << port << "\n";
}
#endif // if 0

TEST_F(SocketMockFTestSuite, bind_syscall_win32_exclusive_addr_use_successful) {
    SSockaddr saddrObj;
    saddrObj = "[2001:db8::fedc:cdef:0:5]";
    // sfd is given to the Unit and it will close it.
    SOCKET sfd = ::socket(PF_INET6, SOCK_DGRAM, 0);
    ASSERT_NE(sfd, INVALID_SOCKET);

    // --- Mock get_sockfd() ---
    // Provide a socket file descriptor
    EXPECT_CALL(m_sys_socketObj, socket(AF_INET6, SOCK_DGRAM, 0))
        .WillOnce(Return(sfd));
    // Expect resetting SO_REUSEADDR.
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, _, _))
        .WillOnce(Return(0));
    // Expect setting IPV6_V6ONLY.
    EXPECT_CALL(m_sys_socketObj, setsockopt(sfd, IPPROTO_IPV6, IPV6_V6ONLY,
                                            PointeeVoidToConstInt(0), _))
        .WillOnce(Return(0));
    // Expect setting SO_EXCLUSIVEADDRUSE.
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sfd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, _, _))
        .WillOnce(Return(0));

    // --- Mock bind() ---
    // Bind socket to an ip address, provide port if port was 0.
    EXPECT_CALL(m_sys_socketObj, bind(sfd, _, _))
        // .Times(3)
        // .WillOnce(Return(SOCKET_ERROR))
        // .WillOnce(Return(SOCKET_ERROR))
        .WillOnce(Return(0));

    if (g_dbug) {
        EXPECT_CALL(
            m_sys_socketObj,
            getsockname(sfd, _,
                        Pointee(Ge(static_cast<socklen_t>(sizeof(saddr.ss))))))
            .WillOnce(
                DoAll(StructCpyToArg<1>(&saddrObj.ss, sizeof(saddrObj.ss)),
                      SetArgPointee<2>(saddrObj.sizeof_saddr()), Return(0)));
    }

    // Inject mocking functions
    umock::Sys_socket sys_socket_injectObj(&m_sys_socketObj);
    umock::Winsock2 winsock2_injectObj{&m_winsock2Obj};

    // Test Unit
    CSocket sockObj;
    sockObj.bind(SOCK_DGRAM, &saddrObj);
}

TEST_F(SocketMockFTestSuite, bind_syscall_win32_exclusive_addr_use_fails) {
    // sfd is given to the Unit and it will close it.
    SOCKET sfd = ::socket(PF_INET6, SOCK_DGRAM, 0);
    ASSERT_NE(sfd, INVALID_SOCKET);

    // --- Mock get_sockfd() ---
    // Provide a socket file descriptor
    EXPECT_CALL(m_sys_socketObj, socket(AF_INET6, SOCK_DGRAM, 0))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(Return(sfd));
    // Expect resetting SO_REUSEADDR.
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, _, _))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(Return(0));
    // Expect setting IPV6_V6ONLY.
    EXPECT_CALL(m_sys_socketObj, setsockopt(sfd, IPPROTO_IPV6, IPV6_V6ONLY,
                                            PointeeVoidToConstInt(0), _))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(Return(0));
    // Expect setting SO_EXCLUSIVEADDRUSE.
    EXPECT_CALL(m_sys_socketObj,
                setsockopt(sfd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, _, _))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(Return(0));

    // --- Mock bind() ---
    // Bind socket to an ip address, provide port if port was 0.
    EXPECT_CALL(m_sys_socketObj, bind(sfd, _, _))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(Return(SOCKET_ERROR));
    // Set expected error numnber.
    EXPECT_CALL(m_winsock2Obj, WSAGetLastError())
        .Times(Between(1, 2))
        .WillRepeatedly(Return(EACCESP));

    // Inject mocking functions
    umock::Sys_socket sys_socket_injectObj(&m_sys_socketObj);
    umock::Winsock2 winsock2_injectObj{&m_winsock2Obj};

    // Test Unit
    SSockaddr saddrObj;
    saddrObj = "[2001:db8::fedc:cdef:0:6]";
    CSocket sockObj;
    EXPECT_THAT(
        ([&sockObj, &saddrObj]() { sockObj.bind(SOCK_DGRAM, &saddrObj); }),
        ThrowsMessage<std::runtime_error>(
            HasSubstr("UPnPsdk MSG1008 EXCEPT[")));
}
#endif // MSC_VER

TEST(SocketTestSuite, bind_same_address_multiple_times_successful) {
    // Binding a socket again is possible after destruction of the socket that
    // shutdown/close it. On Microsoft Windows we have the issue with the
    // SO_EXCLUSIVEADDRUSE socket option as shown in the previous test. But it
    // should not be a problem when we use CSocket::bind() to select a free
    // port number.
    // Test Unit
    {
        CSocket sockObj;
        ASSERT_NO_THROW(sockObj.bind(SOCK_STREAM, nullptr, AI_PASSIVE));
    }
    {
        CSocket sockObj;
        ASSERT_NO_THROW(sockObj.bind(SOCK_STREAM, nullptr, AI_PASSIVE));
    }
    CSocket sockObj;
    ASSERT_NO_THROW(sockObj.bind(SOCK_STREAM, nullptr, AI_PASSIVE));
}

TEST(SocketTestSuite, bind_same_address_again_fails) {
    // Test Unit
    CSocket sockObj;
    ASSERT_NO_THROW(sockObj.bind(SOCK_STREAM, nullptr, AI_PASSIVE));

    // Doing the same again will fail.
    EXPECT_THAT(
        [&sockObj]() { sockObj.bind(SOCK_STREAM, nullptr, AI_PASSIVE); },
        ThrowsMessage<std::runtime_error>(ContainsStdRegex(
            "UPnPsdk MSG1137 EXCEPT\\[.* bound to "
            "netaddress \"(\\[::\\]|0\\.0\\.0\\.0):\\d\\d\\d\\d\\d\"")));
}

TEST(SocketTestSuite, bind_two_times_different_addresses_fail) {
    // Binding a socket two times isn't possible. The socket must be
    // shutdown/closed before bind it again.
    // Provide a socket object
    saddr = "";
    saddr = ":50001"; // Modifies only port
    CSocket sockObj;
    ASSERT_NO_THROW(sockObj.bind(SOCK_STREAM, &saddr));
    EXPECT_TRUE(sockObj.is_bound());

    // Try to bind the socket a second time to another address.
    SSockaddr saddr2;
    saddr2 = ":50002"; // Modifies only port
    EXPECT_THAT(([&sockObj, &saddr2]() { sockObj.bind(SOCK_STREAM, &saddr2); }),
                ThrowsMessage<std::runtime_error>(ContainsStdRegex(
                    "UPnPsdk MSG1137 EXCEPT\\[.* bound to "
                    "netaddress \"(\\[::1\\]|127\\.0\\.0\\.1):50001\"")));
}

TEST(SocketTestSuite, bind_with_invalid_argument_fails) {
    // Test Unit, set invalid socket type.
    EXPECT_THAT(
        []() {
            CSocket sockObj;
            sockObj.bind(-1);
        },
        ThrowsMessage<std::runtime_error>(ContainsStdRegex(
            "UPnPsdk MSG1037 EXCEPT\\[.*\n.* WHAT MSG1112: errid\\(")));

    // Test Unit, set invalid flag number.
    EXPECT_THAT(
        []() {
            CSocket sockObj;
            sockObj.bind(SOCK_STREAM, nullptr, -1);
        },
        ThrowsMessage<std::runtime_error>(ContainsStdRegex(
            "UPnPsdk MSG1037 EXCEPT\\[.*\n.* WHAT MSG1112: errid\\(")));
}


// Listen socket objects
// ---------------------
TEST(SocketTestSuite, listen_on_passive_bound_socket_successful) {
    // Test Unit
    CSocket sockObj;
    ASSERT_NO_THROW(sockObj.bind(SOCK_STREAM, nullptr, AI_PASSIVE));
    ASSERT_NO_THROW(sockObj.listen());
    EXPECT_TRUE(sockObj.is_listen());
}

TEST(SocketTestSuite, listen_on_single_address_active_bound_socket_successful) {
    // Test Unit
    CSocket sockObj;
    ASSERT_NO_THROW(sockObj.bind(SOCK_STREAM));
    ASSERT_NO_THROW(sockObj.listen());
    EXPECT_TRUE(sockObj.is_listen());
}

TEST(SocketTestSuite, listen_to_same_address_multiple_times_successful) {
    // Listen on the same address again of a valid socket is possible and should
    // do nothing.

    // Test Unit
    CSocket sockObj;
    ASSERT_NO_THROW(sockObj.bind(SOCK_STREAM, nullptr, AI_PASSIVE));
    ASSERT_NO_THROW(sockObj.listen());

    EXPECT_NO_THROW(sockObj.listen());
}

TEST(SocketTestSuite, listen_on_datagram_socket_fails) {
    CSocket sockObj;
    ASSERT_NO_THROW(sockObj.bind(SOCK_DGRAM, nullptr, AI_PASSIVE));

    // Test Unit
    EXPECT_THAT([&sockObj]() { sockObj.listen(); },
                ThrowsMessage<std::runtime_error>(
                    HasSubstr("UPnPsdk MSG1034 EXCEPT[")));
}

TEST_F(SocketMockFTestSuite, listen_fails) {
    CSocket sockObj;
    ASSERT_NO_THROW(sockObj.bind(SOCK_DGRAM));
    SOCKET sfd = sockObj;

    EXPECT_CALL(m_sys_socketObj, listen(sfd, SOMAXCONN))
        .Times(Between(1, 2)) // EXPECT_THAT calls a second time if it fails
        .WillRepeatedly(SetErrnoAndReturn(EBADFP, SOCKET_ERROR));

    // Inject mocking functions
    umock::Sys_socket sys_socket_injectObj(&m_sys_socketObj);

    // Test Unit
    EXPECT_THAT([&sockObj]() { sockObj.listen(); },
                ThrowsMessage<std::runtime_error>(
                    HasSubstr("UPnPsdk MSG1034 EXCEPT[")));
}


// Socket error
// ------------
#ifdef _MSC_VER
TEST(SocketErrorTestSuite, check_WSA_errorcode_compatibillity) {
    // The result of this test with the real system functions is, that there is
    // no clear assignment between BSD socket error constant numbers (e.g.
    // ENOTSOCK) and its counterparts on win32 (e.g. WSAENOTSOCK). I have to
    // map them with a new macro e.g. EBADFP.
    //
    // For details look at
    // REF:_[Error_Codes_-_errno,_h_errno_and_WSAGetLastError]
    // (https://learn.microsoft.com/en-us/windows/win32/winsock/error-codes-errno-h-errno-and-wsagetlasterror-2)
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
    ASSERT_NE(ret_getsockopt, 0);

    EXPECT_EQ(static_cast<int>(sockerrObj), EBADFP);
    // Don't know what exact message is given. It depends on the platform.
    EXPECT_GE(sockerrObj.error_str().size(), 10);
    std::cout << sockerrObj.error_str() << "\n";
}


} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
