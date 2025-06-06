// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-05-30

// All functions of the miniserver module have been covered by a gtest. Some
// tests are skipped and must be completed when missed information is
// available.

// Include source code for testing. So we have also direct access to static
// functions which need to be tested.
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
#include <Pupnp/upnp/src/genlib/miniserver/miniserver.cpp>
#else
#include <Compa/src/genlib/miniserver/miniserver.cpp>
#endif

#include <webserver.hpp>

#include <UPnPsdk/synclog.hpp>
#include <UPnPsdk/socket.hpp>
#include <UPnPsdk/sockaddr.hpp>
#include <UPnPsdk/upnptools.hpp> // for errStrEx

#include <utest/utest.hpp>
#include <utest/threadpool_init.hpp>
#include <umock/sys_socket_mock.hpp>
#include <umock/winsock2_mock.hpp>


namespace utest {

using ::testing::_;
using ::testing::AnyOf;
using ::testing::Between;
using ::testing::DoAll;
using ::testing::EndsWith;
using ::testing::ExitedWithCode;
using ::testing::Ge;
using ::testing::HasSubstr;
using ::testing::NotNull;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetErrnoAndReturn;
using ::testing::StartsWith;
using ::testing::StrictMock;

using ::UPnPsdk::CSocket;
using ::UPnPsdk::errStrEx;
using ::UPnPsdk::g_dbug;
using ::UPnPsdk::SSockaddr;


// Helper class
// ============
/// \brief Helper class for debug log messages in compatible code.
class CLogging { /*
 * Use it for example with:
    CLogging loggingObj; // Output only with build type DEBUG.
    loggingObj.enable(UPNP_ALL); // or other loglevel, e.g. UPNP_INFO.
    loggingObj.disable(); // optional
 */
  public:
    CLogging();
    virtual ~CLogging();
    /// -brief Enable debug logging messages.
    void enable(Upnp_LogLevel a_loglevel);
    /// -brief Disable debug logging messages.
    void disable();
};

CLogging::CLogging() = default;

void CLogging::enable(Upnp_LogLevel a_loglevel) {
    UpnpSetLogLevel(a_loglevel);
    if (UpnpInitLog() != UPNP_E_SUCCESS) {
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG1041") "Failed to initialize pupnp logging.");
    }
}

void CLogging::disable() { UpnpCloseLog(); }

CLogging::~CLogging() { UpnpCloseLog(); }


// Miniserver Run TestSuite
// ========================
class RunMiniServerMockFTestSuite : public ::testing::Test {
    // This is a fixture to provide mocking of sys_socket.
  protected:
    // clang-format off
    // Instantiate mocking objects.
    StrictMock<umock::Sys_socketMock> m_sys_socketObj;
    // Inject the mocking objects into the tested code.
    umock::Sys_socket sys_socket_injectObj = umock::Sys_socket(&m_sys_socketObj);
    // clang-format on

    RunMiniServerMockFTestSuite() {
        TRACE2(this, " Construct RunMiniServerMockFTestSuite()")
    }

    virtual ~RunMiniServerMockFTestSuite() override {
        TRACE2(this, " Destruct RunMiniServerMockFTestSuite()")
    }
};
typedef RunMiniServerMockFTestSuite RunMiniServerMockFDeathTest;


class RunMiniServerFuncFTestSuite : public RunMiniServerMockFTestSuite {
    // This is a fixture to provide mocking of sys_socket and a
    // MiniServerSockArray on the heap to call the RunMiniserver() function.
    // This is needed because RunMiniserver() frees MiniServerSockArray.
  protected:
    MiniServerSockArray* m_minisock{};

    RunMiniServerFuncFTestSuite() {
        TRACE2(this, " Construct RunMiniServerFuncFTestSuite()")
        // We need this on the heap because it is freed by 'RunMiniServer()'.
        m_minisock = static_cast<MiniServerSockArray*>(
            malloc(sizeof(MiniServerSockArray)));
        if (m_minisock == nullptr)
            throw std::runtime_error("Failed to allocate memory.");
        InitMiniServerSockArray(m_minisock);
    }

    virtual ~RunMiniServerFuncFTestSuite() override {
        TRACE2(this, " Destruct RunMiniServerFuncFTestSuite()")
    }
};

TEST_F(RunMiniServerFuncFTestSuite, RunMiniServer_successful) {
    // IMPORTANT! There is a limit FD_SETSIZE = 1024 for socket file
    // descriptors that can be used with 'select()'. This limit is not given
    // when using 'poll()' or 'epoll' instead. Old_code uses 'select()' so we
    // must test with this limited socket file descriptors. Otherwise we may
    // get segfaults with 'FD_SET()'. For details have a look at 'man select'.
    //
    // This would start some other threads. We run into dynamic runtime problems
    // with parallel running threads here. For example running the miniserver
    // with schedule_request_job() in a new thread cannot be finished before the
    // mocked miniserver shutdown in the calling thread has been executed at
    // Unit end. This is why I prevent starting other threads. I only test
    // initialize running the miniserver and stopping it.
    //
    // I have 7 socket file descriptors and additional 2 with client APIs,
    // that are used to listen to the different IPv4 and IPv6 protocols for the
    // miniserver (4 fds), the ssdp service (3 fds) and the ssdp request
    // service (2 fds). These are file descriptors summarized in the structure
    // MiniServerSockArray. For details look there.
    //
    // For this test I use only socket file descriptor miniServerSock4 that is
    // listening on IPv4 for the miniserver. --Ingo
    std::cout
        << CYEL "[ TODO     ] " CRES << __LINE__
        << ": Test must be extended for IPv6 sockets and other sockets.\n";

    CLogging logObj; // Output only with build type DEBUG.
    if (g_dbug)
        logObj.enable(UPNP_ALL);

    // With shutdown = true, maxJobs is ignored.
    std::cout
        << CYEL "[ TODO     ] " CRES << __LINE__
        << ": Fix dynamic problem with shutdown before job initialization.\n";
    // CThreadPoolInit tp(gMiniServerThreadPool,
    //                    /*shutdown*/ false, /*maxJobs*/ 1);
    // Workaround:
    CThreadPoolInit tp(gMiniServerThreadPool,
                       /*shutdown*/ true); //*maxJobs*/ 1);

    // Set needed data, listen miniserver only on IPv4 that will connect to a
    // remote client which has done e rquest.
    m_minisock->miniServerPort4 = 50012;
    m_minisock->stopPort = 50013;
    const std::string remote_connect_port = "50014";
    // m_minisock->miniServerPort6 = 5xxxx;

    // Due to 'select()' have ATTENTION to set select_nfds correct.
    SOCKET select_nfds{umock::sfd_base + 8 + 1}; // Must be highest used fd + 1
    m_minisock->miniServerSock4 = umock::sfd_base + 6;
    // m_minisock->miniServerSock6 = umock::sfd_base + n;
    // m_minisock->ssdpSock4 = umock::sfd_base + n;
    m_minisock->miniServerStopSock = umock::sfd_base + 8;
    // Next is the socket file descriptor of an accepted connection to a remote
    // host and not part of listening sockets monitored by select().
    constexpr SOCKET remote_connect_sockfd = umock::sfd_base + 9;

    // I need this to mock socket addresses.
    SSockaddr ssObj;

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Max socket fd for select() not setting to 0 if "
                     "INVALID_SOCKET in MiniServerSockArray on WIN32.\n";
#ifdef _WIN32
        // On MS Windows INVALID_SOCKET is unsigned -1 =
        // 18446744073709551615 so we get select_nfds with this big number
        // even if there is only one INVALID_SOCKET. Incrementing it by one
        // results in 0. To be portable we must not assume INVALID_SOCKET
        // to be -1. --Ingo
        select_nfds = 0; // Wrong!
#endif
        EXPECT_CALL(m_sys_socketObj,
                    select(select_nfds, _, nullptr, _, nullptr))
            .WillOnce(Return(1));

    } else {

        std::cout << CYEL "[ TODO     ] " CRES << __LINE__
                  << ": Unit must not expect its argument MiniServerSockArray* "
                     "to be on the heap and free it.\n";

        // Check socket in fdset_if_valid() successful
        // EXPECT_CALL(m_sys_socketObj, getsockopt(m_minisock->miniServerSock4,
        EXPECT_CALL(m_sys_socketObj, getsockopt(_, //
                                                SOL_SOCKET, SO_ERROR, _, _))
            .Times(g_dbug ? 4 : 1)
            .WillRepeatedly(Return(0));

        // Mock that the socket fd ist bound to an address of a local interface.
        ssObj = "[2001:db8::cd]:50059";
        // EXPECT_CALL(m_sys_socketObj, getsockname(m_minisock->miniServerSock4,
        EXPECT_CALL(m_sys_socketObj, getsockname(_, //
                                                 _, Pointee(sizeof(ssObj.ss))))
            .Times(g_dbug ? 4 : 1)
            .WillRepeatedly(DoAll(SetArgPointee<1>(ssObj.sa),
                                  SetArgPointee<2>(ssObj.sizeof_saddr()),
                                  Return(0)));

        // select() in RunMiniServer() also succeeds.
        EXPECT_CALL(m_sys_socketObj,
                    select(select_nfds, _, nullptr, _, nullptr))
            .WillOnce(Return(1));
    }

    // accept() in RunMiniServer() succeeds and returns the remote ip address
    // that it is connected to.
    ssObj = "192.168.200.201:" + remote_connect_port;
    EXPECT_CALL(m_sys_socketObj,
                accept(m_minisock->miniServerSock4, NotNull(),
                       Pointee(Ge(static_cast<SIZEP_T>(sizeof(ssObj.ss))))))
        .WillOnce(
            DoAll(SetArgPointee<1>(ssObj.sa), Return(remote_connect_sockfd)));

    // Here I mock receiving of the ShutDown datagram.
    constexpr char shutdown_str[]("ShutDown");
    constexpr SIZEP_T shutdown_strlen{sizeof(shutdown_str)};
    ssObj = "127.0.0.1:" + std::to_string(m_minisock->stopPort);
    // It is important to expect shutdown_strlen.
    EXPECT_CALL(m_sys_socketObj,
                recvfrom(m_minisock->miniServerStopSock, _, Ge(shutdown_strlen),
                         0, _, Pointee(sizeof(ssObj.ss))))
        .WillOnce(DoAll(StrCpyToArg<1>(shutdown_str),
                        SetArgPointee<4>(ssObj.sa), Return(shutdown_strlen)));

    // Test Unit
    RunMiniServer(m_minisock);
}

TEST_F(RunMiniServerFuncFTestSuite, RunMiniServer_select_fails_with_no_memory) {
    // See important note at
    // TEST_F(RunMiniServerFuncFTestSuite, RunMiniServer_successful).

    CLogging logObj; // Output only with build type DEBUG.
    if (g_dbug)
        logObj.enable(UPNP_ALL);

    // With shutdown = true, maxJobs is ignored.
    CThreadPoolInit tp(gMiniServerThreadPool,
                       /*shutdown*/ false, /*maxJobs*/ 1);

    // Set needed data, listen miniserver only on IPv4 that will connect to a
    // remote client which has done a request.
    m_minisock->miniServerPort4 = 50025;
    m_minisock->stopPort = 50026;

    // Due to 'select()' have ATTENTION to set select_nfds correct.
    SOCKET select_nfds{umock::sfd_base + 55 + 1}; // Must be highest used fd + 1
    m_minisock->miniServerSock4 = umock::sfd_base + 54;
    m_minisock->miniServerStopSock = umock::sfd_base + 55;

    // We need this to mock socket addresses.
    SSockaddr ssObj;

    if (old_code) {
#ifdef _WIN32
        // On MS Windows INVALID_SOCKET is unsigned -1 =
        // 18446744073709551615 so we get select_nfds with this big number
        // even if there is only one INVALID_SOCKET. Incrementing it by one
        // results in 0. To be portable we must not assume INVALID_SOCKET
        // to be -1. This bug is fixed in new code. --Ingo
        select_nfds = 0; // Wrong!
#endif
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Ongoing failing select() must not hang in an "
                     "endless loop.\n";
        EXPECT_CALL(m_sys_socketObj,
                    select(select_nfds, _, nullptr, _, nullptr))
            .WillOnce(SetErrnoAndReturn(ENOMEM, SOCKET_ERROR))
            // Next is only to force errors to leave endless loop.
            .WillOnce(Return(1)); // Wrong!
        EXPECT_CALL(m_sys_socketObj, accept(_, _, _))
            .WillOnce(SetErrnoAndReturn(ENOMEM, INVALID_SOCKET));
        EXPECT_CALL(m_sys_socketObj, recvfrom(_, _, _, _, _, _))
            .WillOnce(SetErrnoAndReturn(ENOMEM, SOCKET_ERROR));

    } else {

        // Check socket in fdset_if_valid() successful
        EXPECT_CALL(m_sys_socketObj, getsockopt(m_minisock->miniServerSock4,
                                                SOL_SOCKET, SO_ERROR, _, _))
            .WillOnce(Return(0));

        // Mock that the socket fd ist bound to an address of a local interface.
        ssObj = "192.168.10.10:50060";
        EXPECT_CALL(m_sys_socketObj, getsockname(m_minisock->miniServerSock4, _,
                                                 Pointee(sizeof(ssObj.ss))))
            .WillOnce(DoAll(SetArgPointee<1>(ssObj.sa),
                            SetArgPointee<2>(sizeof(ssObj.ss)), Return(0)));

        // but select in RunMiniServer() fails
        EXPECT_CALL(m_sys_socketObj,
                    select(select_nfds, _, nullptr, _, nullptr))
            .WillOnce(SetErrnoAndReturn(ENOMEM, SOCKET_ERROR));
    }

    // Capture output to stderr
    CaptureStdOutErr captureObj(UPnPsdk::log_fileno);
    captureObj.start();

    // Test Unit
    RunMiniServer(m_minisock);
    std::cout << captureObj.str();

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Unit should critical errors always output, not only "
                     "with build type DEBUG and no garbage.\n";
        if (g_dbug)
#ifdef UPNP_HAVE_DEBUG
            EXPECT_THAT(captureObj.str(),
                        HasSubstr("stopSock Error, aborting..."));
#else
            EXPECT_EQ(captureObj.str(), ""); // Wrong!
#endif
        else
            EXPECT_EQ(captureObj.str(), ""); // Wrong!

    } else {                                 // New code

        EXPECT_THAT(captureObj.str(), HasSubstr("UPnPsdk MSG1021 CRIT  ["));
    }
}

TEST_F(RunMiniServerFuncFTestSuite, RunMiniServer_accept_fails) {
    // See important note at
    // TEST_F(RunMiniServerFuncFTestSuite, RunMiniServer_successful).
    // For this test I use only socket file descriptor miniServerSock4 that is
    // listening on IPv4 for the miniserver. --Ingo

    CLogging logObj; // Output only with build type DEBUG.
    logObj.enable(UPNP_ALL);

    // With shutdown = true, maxJobs is ignored.
    CThreadPoolInit tp(gMiniServerThreadPool,
                       /*shutdown*/ false, /*maxJobs*/ 1);

    // Set needed data, listen miniserver only on IPv4 that will connect to a
    // remote client which has done a request.
    m_minisock->miniServerPort4 = 50045;
    m_minisock->stopPort = 50047;

    // Due to 'select()' have ATTENTION to set select_nfds correct.
    SOCKET select_nfds{umock::sfd_base + 59 + 1}; // Must be highest used fd + 1
    m_minisock->miniServerSock4 = umock::sfd_base + 58;
    m_minisock->miniServerStopSock = umock::sfd_base + 59;

    constexpr char shutdown_str[]{"ShutDown"};
    SIZEP_T shutdown_strlen;

    if (old_code) {
#ifdef _WIN32
        // On MS Windows INVALID_SOCKET is unsigned -1 =
        // 18446744073709551615 so we get select_nfds with this big number
        // even if there is only one INVALID_SOCKET. Incrementing it by one
        // results in 0. To be portable we must not assume INVALID_SOCKET
        // to be -1. --Ingo
        select_nfds = 0;      // Wrong!
#endif
        shutdown_strlen = 25; // This is fixed given by the tested Unit
    } else {
        shutdown_strlen = sizeof(shutdown_str);
    }

    // select()
    if (!old_code) {
        // Mock socket check in 'fdset_if_valid()' to be successful.
        // 'getsockopt()' and 'getsockname()' are called in the production
        // code for verification.
        EXPECT_CALL(m_sys_socketObj, getsockopt(m_minisock->miniServerSock4,
                                                SOL_SOCKET, SO_ERROR, _, _))
            .WillOnce(Return(0));

        // Mock that the socket fd ist bound to an address.
        SSockaddr ssObj;
        ssObj = "[2001:db8::ab]:50044";
        EXPECT_CALL(
            m_sys_socketObj,
            getsockname(m_minisock->miniServerSock4, _,
                        Pointee(Ge(static_cast<SIZEP_T>(sizeof(ssObj.ss))))))
            .WillOnce(DoAll(SetArgPointee<1>(ssObj.sa),
                            SetArgPointee<2>(ssObj.sizeof_saddr()), Return(0)));
    }

    // select in RunMiniServer() also succeeds.
    EXPECT_CALL(m_sys_socketObj, select(select_nfds, _, nullptr, _, nullptr))
        .WillOnce(Return(2)); // data and stopsock available on select()

    // accept() will fail for incomming data. stopsock uses a datagram so it
    // doesn't use accept().
    EXPECT_CALL(
        m_sys_socketObj,
        accept(m_minisock->miniServerSock4, NotNull(),
               Pointee(static_cast<socklen_t>(sizeof(::sockaddr_storage)))))
        .WillOnce(SetErrnoAndReturn(ENOMEM, INVALID_SOCKET));

    // Provide data for stopsock with ShutDown.
    SSockaddr ss_localhost;
    ss_localhost = "127.0.0.1:" + std::to_string(m_minisock->stopPort);

    EXPECT_CALL(
        m_sys_socketObj,
        recvfrom(m_minisock->miniServerStopSock, _, shutdown_strlen, 0, _,
                 Pointee(static_cast<socklen_t>(sizeof(::sockaddr_storage)))))
        .WillOnce(DoAll(StrCpyToArg<1>(shutdown_str),
                        SetArgPointee<4>(ss_localhost.sa),
                        Return(shutdown_strlen)));

    // Test Unit
    RunMiniServer(m_minisock);
}

TEST_F(RunMiniServerMockFTestSuite, fdset_if_valid_read_successful) {
    // Socket file descriptor should be added to the read set.
    constexpr SOCKET sockfd{umock::sfd_base + 56};

    if (!old_code) {
        // ::getsockopt() is used to verify a valid socket. Here I mock it to
        // find sockfd valid.
        EXPECT_CALL(m_sys_socketObj,
                    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, _, _))
            .WillOnce(Return(0));

        // ::getsockname() is used to verify if a socket is bound to a local
        // interface address. Here I mock it to find the socket is bound.
        SSockaddr ssObj;
        ssObj = "192.168.10.11:50061";
        EXPECT_CALL(
            m_sys_socketObj,
            getsockname(sockfd, _,
                        Pointee(Ge(static_cast<SIZEP_T>(sizeof(ssObj.ss))))))
            .WillOnce(DoAll(
                SetArgPointee<1>(ssObj.sa),
                SetArgPointee<2>(static_cast<SIZEP_T>(ssObj.sizeof_saddr())),
                Return(0)));
    }

    // Test Unit
    fd_set rdSet;
    FD_ZERO(&rdSet);
    fdset_if_valid(sockfd, &rdSet);

    EXPECT_NE(FD_ISSET(sockfd, &rdSet), 0)
        << "Socket file descriptor " << sockfd
        << " should be added to the FD SET for select().";
}

TEST_F(RunMiniServerMockFTestSuite, fdset_if_valid_fails) {
    fd_set rdSet;
    FD_ZERO(&rdSet);
    ASSERT_FALSE(FD_ISSET(static_cast<SOCKET>(0), &rdSet));

    // Capture output to stderr
    bool dbug_old = g_dbug;
    CaptureStdOutErr captureObj(UPnPsdk::log_fileno);

    if (old_code) {

        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Unit should not terminate program with \"*** buffer "
                     "overflow detected ***\".\n";
        // Next cannot be tested because pupnp fdset_if_valid() does not
        // respect limit FD_SETSIZE and randomly produces "*** buffer overflow
        // detected ***: terminated"
        // FD_ZERO(&rdSet);
        // captureObj.start();
        // g_dbug = true;
        // fdset_if_valid(static_cast<SOCKET>(-17000), &rdSet);
        // g_dbug = dbug_old;
        // EXPECT_EQ(captureObj.str(), ""); // Wrong!

        // Test Unit, fd 0 to 2 are not valid network sockets and should not
        // be set.
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Unit should not accept file descriptors 0 to 2 or >= "
                     "FD_SETSIZE.\n";
        FD_ZERO(&rdSet);
        fdset_if_valid(static_cast<SOCKET>(0), &rdSet);
        EXPECT_TRUE(FD_ISSET(static_cast<SOCKET>(0), &rdSet)); // Wrong!
        FD_ZERO(&rdSet);
        fdset_if_valid(static_cast<SOCKET>(1), &rdSet);
        EXPECT_TRUE(FD_ISSET(static_cast<SOCKET>(1), &rdSet)); // Wrong!
        FD_ZERO(&rdSet);
        fdset_if_valid(static_cast<SOCKET>(2), &rdSet);
        EXPECT_TRUE(FD_ISSET(static_cast<SOCKET>(2), &rdSet)); // Wrong!
        // Next cannot be tested because pupnp fdset_if_valid() does not respect
        // limit FD_SETSIZE and randomly produces "*** buffer overflow detected
        // ***: terminated"
        // FD_ZERO(&rdSet);
        // captureObj.start();
        // g_dbug = true;
        // fdset_if_valid(static_cast<SOCKET>(FD_SETSIZE), &rdSet);
        // g_dbug = dbug_old;
        // EXPECT_EQ(captureObj.str(), ""); // Wrong!

    } else { // if (old_code)

        // Test Unit
        FD_ZERO(&rdSet);
        captureObj.start();
        g_dbug = true;
        fdset_if_valid(static_cast<SOCKET>(-17000), &rdSet);
        g_dbug = dbug_old;

        // Get captured output
        if (g_dbug)
            std::cout << captureObj.str();
        EXPECT_THAT(captureObj.str(), HasSubstr("UPnPsdk MSG1005 ERROR ["));

        // Test Unit
        FD_ZERO(&rdSet);
        fdset_if_valid(static_cast<SOCKET>(0), &rdSet);
        EXPECT_FALSE(FD_ISSET(static_cast<SOCKET>(0), &rdSet));
        FD_ZERO(&rdSet);
        fdset_if_valid(static_cast<SOCKET>(1), &rdSet);
        EXPECT_FALSE(FD_ISSET(static_cast<SOCKET>(1), &rdSet));
        FD_ZERO(&rdSet);
        fdset_if_valid(static_cast<SOCKET>(2), &rdSet);
        EXPECT_FALSE(FD_ISSET(static_cast<SOCKET>(2), &rdSet));
        FD_ZERO(&rdSet);
        captureObj.start();
        g_dbug = true;
        fdset_if_valid(static_cast<SOCKET>(FD_SETSIZE), &rdSet);
        g_dbug = dbug_old;

        // Get captured output
        if (g_dbug)
            std::cout << captureObj.str();
        EXPECT_THAT(captureObj.str(),
                    ContainsStdRegex(
                        "UPnPsdk MSG1005 ERROR \\[.* Prohibited socket 1024"));
    }
}

TEST_F(RunMiniServerMockFTestSuite, fdset_if_valid_fails_with_invalid_socket) {
    // Provide a socket file descriptor.
    constexpr SOCKET sockfd{umock::sfd_base + 7};

    if (!old_code) {
        // ::getsockopt() is used to verify a valid socket. Here I mock it to
        // find sockfd invalid.
        EXPECT_CALL(m_sys_socketObj,
                    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, _, _))
            .WillOnce(SetErrnoAndReturn(EBADF, SOCKET_ERROR));
    }

    // Test Unit
    fd_set rdSet;
    FD_ZERO(&rdSet);
    fdset_if_valid(sockfd, &rdSet);

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Socket file descriptor " << sockfd
                  << " should not be added to the FD SET for ::select().\n";
        EXPECT_NE(FD_ISSET(sockfd, &rdSet), 0); // Wrong!

    } else {

        EXPECT_EQ(FD_ISSET(sockfd, &rdSet), 0)
            << "Socket file descriptor " << sockfd
            << " should not be added to the FD SET for ::select().";
    }
}

TEST_F(RunMiniServerMockFTestSuite, fdset_if_valid_fails_with_unbind_socket) {
    // Provide a socket file descriptor.
    constexpr SOCKET sockfd{umock::sfd_base + 53};

    if (!old_code) {
        // ::getsockopt() is used to verify a valid socket. Here I mock it to
        // find sockfd valid.
        EXPECT_CALL(m_sys_socketObj,
                    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, _, _))
            .WillOnce(Return(0));

        // ::getsockname() is used to verify if a socket is bound to a local
        // interface address. Here I mock it with the unknown sockaddr "[::]"
        // to find the socket is not bound.
        SSockaddr ssObj;
        ssObj = "[::]";
        EXPECT_CALL(
            m_sys_socketObj,
            getsockname(sockfd, _,
                        Pointee(Ge(static_cast<SIZEP_T>(sizeof(ssObj.ss))))))
            .WillOnce(DoAll(SetArgPointee<1>(ssObj.sa),
                            SetArgPointee<2>(sizeof(ssObj.ss)), Return(0)));
    }

    // Test Unit
    fd_set rdSet;
    FD_ZERO(&rdSet);
    fdset_if_valid(sockfd, &rdSet);

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Socket file descriptor " << sockfd
                  << " should not be added to the FD SET for ::select().\n";
        EXPECT_NE(FD_ISSET(sockfd, &rdSet), 0); // Wrong!

    } else {

        EXPECT_EQ(FD_ISSET(sockfd, &rdSet), 0)
            << "Socket file descriptor " << sockfd
            << " should not be added to the FD SET for ::select().";
    }
}

TEST_F(RunMiniServerMockFTestSuite, receive_from_stopsock_successful) {
    // The stop socket is got with 'get_miniserver_stopsock()' and uses a
    // datagram with exactly "ShutDown" on AF_INET to 127.0.0.1.
    constexpr char shutdown_str[]{"ShutDown"};
    // This should be the buffer size in the tested code. The test will fail if
    // it does not match so there is no danger to break destination buffer size.
    constexpr SIZEP_T expected_destbuflen{sizeof(shutdown_str)};

    constexpr SOCKET sockfd{umock::sfd_base + 5};
    SSockaddr ssObj;
    ssObj = "127.0.0.1:50015";

    fd_set rdSet;
    FD_ZERO(&rdSet);
    FD_SET(sockfd, &rdSet);

    // Mock system functions
    // expected_destbuflen is important here to avoid buffer limit overwrite.
    EXPECT_CALL(m_sys_socketObj, recvfrom(sockfd, _, Ge(expected_destbuflen), 0,
                                          _, Pointee(sizeof(ssObj.ss))))
        .WillOnce(DoAll(StrnCpyToArg<1>(shutdown_str, expected_destbuflen),
                        SetArgPointee<4>(ssObj.sa),
                        Return(expected_destbuflen)));

    // Test Unit
    // Returns 1 (true) if successfully received "ShutDown" from stopSock
    EXPECT_EQ(receive_from_stopSock(sockfd, &rdSet), 1);
}

TEST_F(RunMiniServerMockFTestSuite, receive_from_stopsock_not_selected) {
    constexpr SOCKET sockfd{umock::sfd_base + 29};

    fd_set rdSet;
    FD_ZERO(&rdSet);
    // Socket not selected to be received
    // FD_SET(sockfd, &rdSet);
    // This should not call recvfrom() and is guarded by StrictMock<>.

    // Capture output to stderr
    CaptureStdOutErr captureObj(UPnPsdk::log_fileno);
    bool dbug_old = g_dbug;

    // Test Unit should silently nothing have received.
    captureObj.start();
    g_dbug = true;
    EXPECT_EQ(receive_from_stopSock(sockfd, &rdSet), 0);
    g_dbug = dbug_old;

    EXPECT_THAT(
        captureObj.str(),
        AnyOf("", ContainsStdRegex(
                      "^TRACE\\[.*\\] Executing receive_from_stopSock\\(\\)")));
}

TEST_F(RunMiniServerMockFTestSuite, receive_from_stopsock_receiving_fails) {
    constexpr SOCKET sockfd{umock::sfd_base + 28};
    fd_set rdSet;
    FD_ZERO(&rdSet);
    FD_SET(sockfd, &rdSet);

    // Mock system functions
    EXPECT_CALL(m_sys_socketObj, recvfrom(sockfd, _, _, _, _, _))
        .WillOnce(SetErrnoAndReturn(EINVAL, SOCKET_ERROR));

    // Test Unit
    // Here we have a critical error. The Unit should return 'true' to stop the
    // miniserver.
    EXPECT_EQ(receive_from_stopSock(sockfd, &rdSet), 1);
}

TEST_F(RunMiniServerMockFTestSuite, receive_from_stopsock_no_bytes_received) {
    CLogging logObj; // Output only with build type DEBUG.
    if (old_code)
        if (g_dbug)
            logObj.enable(UPNP_ALL);

    constexpr char shutdown_str[]{""};
    constexpr SIZEP_T bufsizeof_ShutDown_str{9};
    constexpr SOCKET sockfd{umock::sfd_base + 30};
    SSockaddr ssObj;
    ssObj = "127.0.0.1:50015";

    fd_set rdSet;
    FD_ZERO(&rdSet);
    FD_SET(sockfd, &rdSet);

    // Mock system functions
    EXPECT_CALL(m_sys_socketObj, recvfrom(sockfd, _, Ge(bufsizeof_ShutDown_str),
                                          0, _, Pointee(sizeof(ssObj.ss))))
        .WillOnce(DoAll(StrCpyToArg<1>(shutdown_str),
                        SetArgPointee<4>(ssObj.sa), Return(0)));

    // Test Unit
    // Returns 1 (true) if successfully received "ShutDown" from stopSock
    if (old_code) {
        std::cout
            << CYEL "[ BUGFIX   ] " CRES << __LINE__
            << ": Receiving a datagram with 0 bytes length should fail.\n";
        EXPECT_EQ(receive_from_stopSock(sockfd, &rdSet), 1); // Wrong!

    } else {

        EXPECT_EQ(receive_from_stopSock(sockfd, &rdSet), 0);
    }
}

TEST_F(RunMiniServerMockFTestSuite, receive_from_stopsock_wrong_stop_message) {
    CLogging logObj;
    if (old_code)
        if (g_dbug)
            logObj.enable(UPNP_ALL);

    constexpr char shutdown_str[]{"Nothings"};
    // This should be the buffer size in the tested code. The test will fail if
    // it does not match so there is no danger to break buffer size.
    constexpr SIZEP_T expected_destbuflen{sizeof(shutdown_str)};

    constexpr SOCKET sockfd{umock::sfd_base + 31};
    SSockaddr ssObj;
    ssObj = "127.0.0.1:50017";

    fd_set rdSet;
    FD_ZERO(&rdSet);
    FD_SET(sockfd, &rdSet);

    // Mock system functions
    // expected_destbuflen is important here to avoid buffer limit overwrite.
    EXPECT_CALL(m_sys_socketObj, recvfrom(sockfd, _, Ge(expected_destbuflen), 0,
                                          _, Pointee(sizeof(ssObj.ss))))
        .WillOnce(DoAll(StrnCpyToArg<1>(shutdown_str, expected_destbuflen),
                        SetArgPointee<4>(ssObj.sa),
                        Return(expected_destbuflen)));

    // Test Unit
    EXPECT_EQ(receive_from_stopSock(sockfd, &rdSet), 0);
}

TEST_F(RunMiniServerMockFTestSuite, receive_from_stopsock_from_wrong_address) {
    CLogging logObj; // Output only with build type DEBUG.
    if (old_code)
        if (g_dbug)
            logObj.enable(UPNP_ALL);

    constexpr char shutdown_str[]{"ShutDown"};
    // This should be the buffer size in the tested code. The test will fail if
    // it does not match so there is no danger to break buffer size.
    constexpr SIZEP_T expected_destbuflen{sizeof(shutdown_str)};

    constexpr SOCKET sockfd{umock::sfd_base + 48};
    SSockaddr ssObj;
    ssObj = "192.168.150.151:50018";

    fd_set rdSet;
    FD_ZERO(&rdSet);
    FD_SET(sockfd, &rdSet);

    // Mock system functions
    // expected_destbuflen is important here to avoid buffer limit overwrite.
    EXPECT_CALL(m_sys_socketObj, recvfrom(sockfd, _, Ge(expected_destbuflen), 0,
                                          _, Pointee(sizeof(ssObj.ss))))
        .WillOnce(DoAll(StrnCpyToArg<1>(shutdown_str, expected_destbuflen),
                        SetArgPointee<4>(ssObj.sa),
                        Return(expected_destbuflen)));

    // Test Unit
    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Receiving ShutDown not from 127.0.0.1 must fail.\n";
        EXPECT_EQ(receive_from_stopSock(sockfd, &rdSet), 1); // Wrong!

    } else {

        EXPECT_EQ(receive_from_stopSock(sockfd, &rdSet), 0);
    }
}

TEST_F(RunMiniServerMockFTestSuite, receive_from_stopsock_without_0_termbyte) {
    CLogging logObj; // Output only with build type DEBUG.
    if (old_code)
        if (g_dbug)
            logObj.enable(UPNP_ALL);

    constexpr char shutdown_str[]{"ShutDown "};
    // With this buffer size the last byte is a space but not a '\0'.
    constexpr SIZEP_T expected_destbuflen{sizeof(shutdown_str) - 1};

    constexpr SOCKET sockfd{umock::sfd_base + 49};
    SSockaddr ssObj;
    ssObj = "127.0.0.1:50019";

    fd_set rdSet;
    FD_ZERO(&rdSet);
    FD_SET(sockfd, &rdSet);

    // Mock system functions
    // expected_destbuflen is important here to avoid buffer limit overwrite.
    EXPECT_CALL(m_sys_socketObj, recvfrom(sockfd, _, Ge(expected_destbuflen), 0,
                                          _, Pointee(sizeof(ssObj.ss))))
        .WillOnce(DoAll(StrnCpyToArg<1>(shutdown_str, expected_destbuflen),
                        SetArgPointee<4>(ssObj.sa),
                        Return(expected_destbuflen)));

    // Test Unit
    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Receiving \"ShutDown\" without terminating nullbyte "
                     "must fail.\n";
        EXPECT_EQ(receive_from_stopSock(sockfd, &rdSet), 1); // Wrong!

    } else {

        EXPECT_EQ(receive_from_stopSock(sockfd, &rdSet), 0);
    }
}

TEST_F(RunMiniServerMockFTestSuite, ssdp_read_successful) {
    CLogging logObj; // Output only with build type DEBUG.
    if (g_dbug)
        logObj.enable(UPNP_ALL);

    // Initialize the Recv Threadpool, allow only one job.
    // With shutdown = true, maxJobs is ignored.
    CThreadPoolInit tp(gRecvThreadPool,
                       /*shutdown*/ false, /*maxJobs*/ 1);

    constexpr char ssdpdata_str[]{
        "Some SSDP test data for a request of a remote client."};
    ASSERT_LE(sizeof(ssdpdata_str), BUFSIZE - 1);

    SOCKET ssdp_sockfd{umock::sfd_base + 32};
    SSockaddr ssObj;
    ssObj = "192.168.71.82:50023";

    fd_set rdSet;
    FD_ZERO(&rdSet);
    FD_SET(ssdp_sockfd, &rdSet);

    EXPECT_CALL(
        m_sys_socketObj,
        recvfrom(ssdp_sockfd, _, BUFSIZE - 1, 0, _,
                 Pointee(static_cast<socklen_t>(sizeof(::sockaddr_storage)))))
        .WillOnce(DoAll(StrCpyToArg<1>(ssdpdata_str),
                        SetArgPointee<4>(ssObj.sa),
                        Return((SSIZEP_T)sizeof(ssdpdata_str))));

    // Capture output to stderr
    CaptureStdOutErr captureObj(UPnPsdk::log_fileno);
    captureObj.start();

    // Test Unit
    ssdp_read(&ssdp_sockfd, &rdSet); // Will return an error to ssdp_sockfd

    EXPECT_THAT(
        captureObj.str(),
        AnyOf(
            "",
            HasSubstr("Some SSDP test data for a request of a remote client."),
            ContainsStdRegex("^TRACE\\[.*\\] Executing ssdp_read\\(\\)")));
    EXPECT_NE(ssdp_sockfd, INVALID_SOCKET);
}

TEST_F(RunMiniServerMockFTestSuite, ssdp_read_fails) {
    // Due to error there is no job added to the Threadpool. Initializing it
    // is not needed.

    CLogging logObj; // Output only with build type DEBUG.
    if (g_dbug)
        logObj.enable(UPNP_ALL);

    constexpr char ssdpdata_str[]{
        "Some SSDP test data for a request of a remote client."};
    ASSERT_LE(sizeof(ssdpdata_str), BUFSIZE - 1);

    constexpr SOCKET ssdp_sockfd_valid{umock::sfd_base + 46};

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

    // Capture output to stderr
    CaptureStdOutErr captureObj(UPnPsdk::log_fileno);
    captureObj.start();

    // Test Unit
    ssdp_read(&ssdp_sockfd, &rdSet);

    EXPECT_THAT(
        captureObj.str(),
        AnyOf("",
              HasSubstr("]: miniserver: Error in readFromSSDPSocket(46): "
                        "closing socket"),
              ContainsStdRegex("^TRACE\\[.*\\] Executing ssdp_read\\(\\)")));
    EXPECT_EQ(ssdp_sockfd, INVALID_SOCKET);
}

TEST_F(RunMiniServerMockFTestSuite, web_server_accept_successful) {
    // The tested units are different for old_code (pupnp) and new code (compa).
    // We have void      ::web_server_accept() and
    //         int  compa::web_server_accept().
    constexpr SOCKET listen_sockfd{umock::sfd_base + 33};
    constexpr SOCKET connected_sockfd{umock::sfd_base + 34};
    const std::string connected_port = "50062";
    fd_set set;
    FD_ZERO(&set);
    FD_SET(listen_sockfd, &set);

    // Prevent to add jobs, I test jobs isolated. See note at
    // TEST_F(RunMiniServerFuncFTestSuite, RunMiniServer_successful).
    // With shutdown = true, maxJobs is ignored.
    CThreadPoolInit tp(gMiniServerThreadPool,
                       /*shutdown*/ false, /*maxJobs*/ 0);

    SSockaddr ssObj;
    ssObj = "192.168.201.202:" + connected_port;
    EXPECT_CALL(m_sys_socketObj,
                accept(listen_sockfd, NotNull(), Pointee(sizeof(ssObj.ss))))
        .WillOnce(DoAll(SetArgPointee<1>(ssObj.sa), Return(connected_sockfd)));

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    // Capture output to stderr
    CLogging logObj; // Output only with build type DEBUG.
    logObj.enable(UPNP_ALL);
    CaptureStdOutErr captureObj(UPnPsdk::log_fileno);
    captureObj.start();

    // Test Unit
    web_server_accept(listen_sockfd, &set);

    // Get captured output
    // '\n' is not matched by regex '.'-wildcard so we just replace it.
    std::replace(captureObj.str().begin(), captureObj.str().end(), '\n', '@');
#ifdef DEBUG
    EXPECT_THAT(captureObj.str(),
                ContainsStdRegex(" UPNP-MSER-2: .* mserv " +
                                 std::to_string(connected_sockfd) +
                                 ": cannot schedule request"));
#else
    EXPECT_EQ(captureObj.str(), "libupnp ThreadPoolAdd too many jobs: 0@");
#endif
    if (!github_actions)
        GTEST_FAIL() << "Fix to schedule request. The error is only with "
                        "build_type DEBUG reported.";

#else  // UPnPsdk_WITH_NATIVE_PUPNP

    if (g_dbug) {
        EXPECT_CALL(m_sys_socketObj, getsockopt(_, SOL_SOCKET, SO_ERROR, _, _))
            .Times(3)
            .WillRepeatedly(Return(0));

        EXPECT_CALL(m_sys_socketObj,
                    getsockname(_, _, Pointee(sizeof(ssObj.ss))))
            .Times(3)
            .WillRepeatedly(DoAll(SetArgPointee<1>(ssObj.sa),
                                  SetArgPointee<2>(ssObj.sizeof_saddr()),
                                  Return(0)));
    }

    // Test Unit
    int ret_web_server_accept = web_server_accept(listen_sockfd, set);
    EXPECT_EQ(ret_web_server_accept, UPNP_E_SUCCESS)
        << errStrEx(ret_web_server_accept, UPNP_E_SUCCESS);

#endif // UPnPsdk_WITH_NATIVE_PUPNP
}

TEST_F(RunMiniServerMockFTestSuite, web_server_accept_with_invalid_socket) {
    // The tested units are different for old_code (pupnp) and new code (compa).
    // We have void      ::web_server_accept() and
    //         int  compa::web_server_accept().
    constexpr SOCKET listen_sockfd = INVALID_SOCKET;

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    // Capture output to stderr
    CLogging logObj; // Output only with build type DEBUG.
    logObj.enable(UPNP_ALL);
    CaptureStdOutErr captureObj(UPnPsdk::log_fileno);
    captureObj.start();

    // Test Unit
    web_server_accept(listen_sockfd, nullptr);

    // Get captured output
    EXPECT_TRUE(captureObj.str().empty());

#else  // UPnPsdk_WITH_NATIVE_PUPNP

    // Test Unit
    fd_set set; // unused, only for reference
    FD_ZERO(&set);
    int ret_web_server_accept = web_server_accept(listen_sockfd, set);
    EXPECT_EQ(ret_web_server_accept, UPNP_E_SOCKET_ERROR)
        << errStrEx(ret_web_server_accept, UPNP_E_SOCKET_ERROR);

#endif // UPnPsdk_WITH_NATIVE_PUPNP
}

TEST_F(RunMiniServerMockFDeathTest, web_server_accept_with_invalid_set) {
    // The tested units are different for old_code (pupnp) and new code (compa).
    // We have void      ::web_server_accept() and
    //         int  compa::web_server_accept().
    constexpr SOCKET listen_sockfd{umock::sfd_base + 57};

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    std::cerr << CYEL "[ BUGFIX   ] " CRES << __LINE__
              << ": nullptr to socket select set must not segfault.\n";
    // There may be a multithreading test running before. Due to problems
    // running this together with death tests as noted in the gtest docu the
    // GTEST_FLAG has to be set. If having more death tests it should be
    // considered to run multithreading tests with an own test file without
    // death tests.
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    EXPECT_DEATH(web_server_accept(listen_sockfd, nullptr), "");

#else  // UPnPsdk_WITH_NATIVE_PUPNP

    // Test Unit
    fd_set set; // unused, only for reference
    FD_ZERO(&set);
    int ret_web_server_accept = web_server_accept(listen_sockfd, set);
    EXPECT_EQ(ret_web_server_accept, UPNP_E_SOCKET_ERROR)
        << errStrEx(ret_web_server_accept, UPNP_E_SOCKET_ERROR);

#endif // UPnPsdk_WITH_NATIVE_PUPNP
}

TEST_F(RunMiniServerMockFTestSuite, web_server_accept_with_empty_set) {
    // The tested units are different for old_code (pupnp) and new code (compa).
    // We have void      ::web_server_accept() and
    //         int  compa::web_server_accept().
    constexpr SOCKET listen_sockfd{umock::sfd_base + 35};
    fd_set set;
    FD_ZERO(&set);

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    // Capture output to stderr
    CLogging logObj; // Output only with build type DEBUG.
    logObj.enable(UPNP_ALL);
    CaptureStdOutErr captureObj(UPnPsdk::log_fileno);
    captureObj.start();

    // Test Unit
    web_server_accept(listen_sockfd, &set);

    // Get captured output
    EXPECT_TRUE(captureObj.str().empty());

#else  // UPnPsdk_WITH_NATIVE_PUPNP

    // Test Unit
    int ret_web_server_accept = web_server_accept(listen_sockfd, set);
    EXPECT_EQ(ret_web_server_accept, UPNP_E_SOCKET_ERROR)
        << errStrEx(ret_web_server_accept, UPNP_E_SOCKET_ERROR);

#endif // UPnPsdk_WITH_NATIVE_PUPNP
}

TEST_F(RunMiniServerMockFTestSuite, web_server_accept_fails) {
    // The tested units are different for old_code (pupnp) and new code (compa).
    // We have void      ::web_server_accept() and
    //         int  compa::web_server_accept().
    constexpr SOCKET listen_sockfd{umock::sfd_base + 50};
    fd_set set;
    FD_ZERO(&set);
    FD_SET(listen_sockfd, &set);

    EXPECT_CALL(
        m_sys_socketObj,
        accept(listen_sockfd, NotNull(),
               Pointee(static_cast<socklen_t>(sizeof(::sockaddr_storage)))))
        .WillOnce(SetErrnoAndReturn(EINVAL, INVALID_SOCKET));

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    // Capture output to stderr
    CLogging logObj; // Output only with build type DEBUG.
    logObj.enable(UPNP_ALL);
    CaptureStdOutErr captureObj(UPnPsdk::log_fileno);
    captureObj.start();

    // Test Unit
    web_server_accept(listen_sockfd, &set);
#ifdef DEBUG
    EXPECT_THAT(captureObj.str(),
                ContainsStdRegex(
                    " UPNP-MSER-2: .* miniserver: Error in accept\\(\\): "));
#else
    std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
              << ": Failing accept() must not silently be ignored and continue "
                 "to fail recvfrom.\n";
    EXPECT_TRUE(captureObj.str().empty()); // Wrong!
#endif

#else  // UPnPsdk_WITH_NATIVE_PUPNP

    // Test Unit
    int ret_web_server_accept = web_server_accept(listen_sockfd, set);
    EXPECT_EQ(ret_web_server_accept, UPNP_E_SOCKET_ACCEPT)
        << errStrEx(ret_web_server_accept, UPNP_E_SOCKET_ACCEPT);

#endif // UPnPsdk_WITH_NATIVE_PUPNP
}

TEST_F(RunMiniServerMockFTestSuite, get_numeric_host_redirection) {
    // getNumericHostRedirection() returns the ip address with port as text
    // (e.g. "192.168.1.2:54321") that is bound to a socket.

    constexpr SOCKET sockfd{umock::sfd_base + 37};
    char host_port[INET6_ADDRSTRLEN + 1 + 5]{"<no message>"};

    // Provide a sockaddr structure that will be returned by mocked
    // getsockname().
    SSockaddr ssObj;
    ssObj = "192.168.123.122:54321";

    // Test Unit
    if (old_code) {
        EXPECT_CALL(m_sys_socketObj, getsockname(sockfd, _, _))
            .WillOnce(DoAll(SetArgPointee<1>(ssObj.sa), Return(0)));
        EXPECT_TRUE(
            getNumericHostRedirection(sockfd, host_port, sizeof(host_port)));

    } else {

        EXPECT_CALL(m_sys_socketObj,
                    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, _, _))
            .WillOnce(Return(0));
        EXPECT_CALL(m_sys_socketObj, getsockname(sockfd, _, _))
            .WillOnce(DoAll(SetArgPointee<1>(ssObj.sa),
                            SetArgPointee<2>(ssObj.sizeof_saddr()), Return(0)));
        EXPECT_TRUE(
            getNumericHostRedirection(sockfd, host_port, sizeof(host_port)));
    }

    EXPECT_STREQ(host_port, "192.168.123.122:54321");
}

TEST_F(RunMiniServerMockFTestSuite,
       get_numeric_host_redirection_with_insufficient_resources) {
    constexpr SOCKET sockfd{umock::sfd_base + 38};
    char host_port[INET6_ADDRSTRLEN + 1 + 5]{"<no message>"};
#ifdef _MSC_VER
    StrictMock<umock::Winsock2Mock> m_winsock2Obj;
    umock::Winsock2 winsock2_injectObj = umock::Winsock2(&m_winsock2Obj);
#endif

    // Mock system function ::getsockname() to fail with insufficient
    // resources. It is called here within the free function
    // UPnPsdk::getsockname().
    EXPECT_CALL(m_sys_socketObj, getsockname(sockfd, _, _))
#ifndef _MSC_VER
        .WillOnce(SetErrnoAndReturn(ENOBUFS, -1));
#else
        // errno isn't used with Winsock. That uses WSAGetLastError() as mocked
        // below with new code.
        .WillOnce(Return(-1));
#endif

    // Capture output to stderr
    CaptureStdOutErr captureObj(UPnPsdk::log_fileno);
    captureObj.start();

    // Test Unit
    if (old_code) {
        EXPECT_FALSE(
            getNumericHostRedirection(sockfd, host_port, sizeof(host_port)));
        // Get captured output. This doesn't give any error messages.
        EXPECT_EQ(captureObj.str(), "");

    } else {

        // ::getsockopt() is called here within the CSocket constructor.
        EXPECT_CALL(m_sys_socketObj,
                    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, _, _))
            .WillOnce(Return(0));
#ifdef _MSC_VER
        // ::WSAGetLastError() is called here within the free function
        // UPnPsdk::getsockname() and when throwing the error used by CSocket.
        EXPECT_CALL(m_winsock2Obj, WSAGetLastError())
            .Times(2)
            .WillRepeatedly(Return(WSAENOBUFS));
#endif

        // Test Unit
        EXPECT_FALSE(
            getNumericHostRedirection(sockfd, host_port, sizeof(host_port)));
        // Get captured output
        if (g_dbug)
            EXPECT_THAT(captureObj.str(),
                        // Different on MacOS
                        AnyOf(HasSubstr("UPnPsdk MSG1057 EXCEPT["),
                              HasSubstr("UPnPsdk MSG1001 EXCEPT[")));
        else
            EXPECT_THAT(captureObj.str(), AnyOf("", StartsWith("TRACE")));
    }

    EXPECT_STREQ(host_port, "<no message>");
}

TEST(RunMiniServerTestSuite,
     get_numeric_host_redirection_with_wrong_address_family) {
    const SOCKET sockfd{::socket(AF_UNIX, SOCK_STREAM, 0)};
    char host_port[INET6_ADDRSTRLEN + 1 + 5]{"<no message>"};

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    // Capture output to stderr
    CaptureStdOutErr captureObj(UPnPsdk::log_fileno);
    captureObj.start();

    // Test Unit
    bool ret_getNumericHostRedirection = getNumericHostRedirection(
        static_cast<int>(sockfd), host_port, sizeof(host_port));

    // Get captured output
    EXPECT_TRUE(captureObj.str().empty());

    std::cout << CYEL "[    FIX   ] " CRES << __LINE__
              << ": A wrong address family AF_UNIX should not be accepted "
                 "on Unix like platforms.\n";
#ifdef _MSC_VER
    EXPECT_FALSE(ret_getNumericHostRedirection);
    EXPECT_STREQ(host_port, "<no message>");
#else
    EXPECT_TRUE(ret_getNumericHostRedirection);
    EXPECT_STRNE(host_port,
                 "<no message>"); // Wrong! Should not be modified.
#endif

#else // UPnPsdk_WITH_NATIVE_PUPNP

    // Capture output to stderr
    CaptureStdOutErr captureObj(UPnPsdk::log_fileno);
    bool g_dbug_old = g_dbug;

    // Test Unit
    captureObj.start();
    g_dbug = false;
    bool ret_getNumericHostRedirection =
        getNumericHostRedirection(sockfd, host_port, sizeof(host_port));

    // Get captured output
    EXPECT_THAT(captureObj.str(), AnyOf("", StartsWith("TRACE")));
    EXPECT_FALSE(ret_getNumericHostRedirection);

    // Test Unit
    captureObj.start();
    g_dbug = true;
    ret_getNumericHostRedirection =
        getNumericHostRedirection(sockfd, host_port, sizeof(host_port));

    g_dbug = g_dbug_old;
    // Get captured output, "Unsupported address family 1".
    EXPECT_THAT(captureObj.str(), HasSubstr("UPnPsdk MSG1093 CATCH "));
    EXPECT_FALSE(ret_getNumericHostRedirection);
    EXPECT_STREQ(host_port, "<no message>");
#endif

    CLOSE_SOCKET_P(sockfd);
}

TEST(RunMiniServerTestSuite, schedule_request_job) {
    SOCKET connected_sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    SSockaddr saddrObj;
    saddrObj = "192.168.1.1:50063";

    // Prevent to add jobs, we test jobs isolated. See note at
    // TEST(RunMiniServerTestSuite, RunMiniServer_successful).
    // With shutdown = true, maxJobs is ignored.
    CThreadPoolInit tp(gMiniServerThreadPool,
                       /*shutdown*/ false, /*maxJobs*/ 0);

    // Capture output to stderr
    CLogging logObj; // Output only with build type DEBUG.
    logObj.enable(UPNP_ALL);
    CaptureStdOutErr captureObj(UPnPsdk::log_fileno);
    captureObj.start();

    // Test Unit
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    schedule_request_job(connected_sockfd, &saddrObj.sa);
#else
    schedule_request_job(connected_sockfd, saddrObj);
#endif

    // Get captured output
    EXPECT_THAT(captureObj.str(),
                HasSubstr("libupnp ThreadPoolAdd too many jobs: 0\n"));
    if (old_code) {
#ifdef DEBUG
        EXPECT_THAT(
            captureObj.str(),
            ContainsStdRegex("\\]: mserv \\d+: cannot schedule request"));
#endif
    } else {
        if (g_dbug)
            EXPECT_THAT(captureObj.str(),
                        ContainsStdRegex("UPnPsdk MSG1025 ERROR \\["));
    }

    CLOSE_SOCKET_P(connected_sockfd);
}

TEST(RunMiniServerTestSuite, free_handle_request_arg_successful) {
    // Provide null set request structure
    mserv_request_t* request =
        static_cast<mserv_request_t*>(calloc(1, sizeof(mserv_request_t)));

    // Test Unit
    free_handle_request_arg(request);

    request = static_cast<mserv_request_t*>(malloc(sizeof(mserv_request_t)));
    memset(request, 0xAA, sizeof(mserv_request_t));

    // Test Unit
    free_handle_request_arg(request);
}

TEST(RunMiniServerTestSuite, free_handle_request_arg_with_valid_socket) {
    // Provide null set request structure
    mserv_request_t* request =
        static_cast<mserv_request_t*>(calloc(1, sizeof(mserv_request_t)));
    // and set a valid socket
    ASSERT_NE(request->connfd = socket(AF_INET6, SOCK_STREAM, 0),
              INVALID_SOCKET);

    // Test Unit
    free_handle_request_arg(request);

    // This test isn't possible because the compiler complains
    // error: pointer ‘request’ used after ‘void free(void*)’
    // [-Werror=use-after-free]
    /*
    // Check if closed socket file descriptor is invalid now.
    int so_option{-1};
    socklen_t optlen{sizeof(so_option)}; // May be modified
    // Type cast (char*)&so_option is needed for Microsoft Windows.
    EXPECT_NE(getsockopt(request->connfd, SOL_SOCKET, SO_ERROR,
                         reinterpret_cast<char*>(&so_option), &optlen),
              0);
    */
}

TEST(RunMiniServerTestSuite, free_handle_request_arg_with_invalid_socket) {
    // Provide null set request structure
    mserv_request_t* request =
        static_cast<mserv_request_t*>(calloc(1, sizeof(mserv_request_t)));
    // and set an invalid socket
    request->connfd = INVALID_SOCKET;

    // Test Unit
    free_handle_request_arg(request);
}

TEST(RunMiniServerDeathTest, free_handle_request_arg_with_nullptr) {
    if (old_code) {
#if !defined __APPLE__ || DEBUG
        std::cerr << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": free_handle_re4quest with nullptr must not segfault.\n";
        // There may be a multithreading test running before with
        // TEST(RunMiniServerTestSuite, schedule_request_job). Due to problems
        // running this together with death tests as noted in the gtest docu the
        // GTEST_FLAG has to be set. If having more death tests it should be
        // considered to run multithreading tests with an own test file without
        // death tests.
        GTEST_FLAG_SET(death_test_style, "threadsafe");

        // Test Unit
        EXPECT_DEATH(free_handle_request_arg(nullptr), "");
#endif
    } else {

        // Test Unit, this expects NO segfault.
        ASSERT_EXIT((free_handle_request_arg(nullptr), exit(0)),
                    ExitedWithCode(0), ".*");
    }
}

#if !defined _WIN32 || !defined DEBUG
// MS Windows assert this with DEBUG enabled and blocks (does not return from)
// free.
TEST(RunMiniServerDeathTest, free_handle_request_arg_double_free) {
    // See note at previous test.
    GTEST_FLAG_SET(death_test_style, "threadsafe");

    EXPECT_DEATH(
        {
            // Provide null set request structure
            mserv_request_t* request = static_cast<mserv_request_t*>(
                calloc(1, sizeof(mserv_request_t)));

            // Test Unit
            free_handle_request_arg(request);

            // This should abort with "double free".
            free_handle_request_arg(request);
        },
        "");
}
#endif

TEST_F(RunMiniServerMockFTestSuite, handle_request_successful) {
    // This test depends on mocking of http_RecvMessage() with a correct request
    // message from a control point and will be completed if that is done.
    GTEST_SKIP() << "Still needs to be completed when tests for "
                    "http_RecvMessage() has been made.";
#if 0
    CLogging logObj; // Output only with build type DEBUG.
    if (g_dbug)
        logObj.enable(UPNP_ALL);

    constexpr SOCKET connfd{umock::sfd_base + 60};
    mserv_request_t request_in{};
    request_in.connfd = connfd;

    constexpr char received_request_str[]{
        "Test data of a request from a remote client."};
    constexpr SSIZEP_T req_strlen{sizeof(received_request_str)};

    // Mock received data string.
    EXPECT_CALL(m_sys_socketObj,
                recv(connfd, _, Ge(req_strlen), 16384))
        .WillOnce(DoAll(StrCpyToArg<1>(received_request_str),
                        Return(req_strlen)));

    // Test Unit
    handle_request(&request_in);
#endif
}

TEST_F(RunMiniServerMockFTestSuite, handle_request_with_failing_select) {
    // This test depends on mocking of http_RecvMessage() and will be completed
    // if that is done. I have to decide if a status response is to send (http
    // 400) or not.
    GTEST_SKIP() << "Still needs to be completed when tests for "
                    "http_RecvMessage() has been made.";
#if 0
    CLogging logObj; // Output only with build type DEBUG.
    if (g_dbug)
        logObj.enable(UPNP_ALL);

    constexpr SOCKET connfd{umock::sfd_base + 61};
    mserv_request_t reqest_in{};
    reqest_in.connfd = connfd;

    // select() fails.
    EXPECT_CALL(m_sys_socketObj, select(connfd + 1, _, _, nullptr, _))
        .WillOnce(SetErrnoAndReturn(ENOMEM, SOCKET_ERROR));

    // Test Unit
    handle_request(&reqest_in);
#endif
}

TEST(RunMiniServerTestSuite, dispatch_request_fails) {
    // Provide resources
    SOCKINFO sockinfo{};

    http_message_t httpmsg{};

    http_parser_t hparser{};
    hparser.msg = httpmsg;

    // Test Unit
    int ret_dispatch_request = dispatch_request(&sockinfo, &hparser);
    EXPECT_EQ(ret_dispatch_request, HTTP_INTERNAL_SERVER_ERROR)
        << errStrEx(ret_dispatch_request, HTTP_INTERNAL_SERVER_ERROR);
}

TEST(RunMiniServerTestSuite, dispatch_request) {
    GTEST_SKIP() << "Still needs to be done when we have complete tests for "
                    "httpreadwrite.";
}

TEST(RunMiniServerTestSuite, host_header_is_numeric_modifies_argument) {
    // This test is to show that the Unit is modifying its C string argument. It
    // addresses it with sizeof(arg) - 1. That is a serious problem if working
    // on an empty string. Then it modifies the byte before the first string
    // character and results in random segfaults.
    char str1[]{"[2001:db8::42]:59878"};

    // Test Unit
    // Length is without terminating '\0`.
    EXPECT_TRUE(host_header_is_numeric(str1, sizeof(str1) - 1))
        << "\"" << str1 << "\"";

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": The Unit must not modify its argument it point to, may "
                     "segfault with empty string \"\".\n";
        EXPECT_STREQ(str1,
                     "[2001:db8::42"); // Wrong! Was "[2001:db8::42]:59878"

    } else {

        EXPECT_STREQ(str1, "[2001:db8::42]:59878");
    }
}

class HeaderIsNumTest
    : public ::testing::TestWithParam<std::tuple<const char*, const bool>> {};

TEST_P(HeaderIsNumTest, host_header_is_numeric) {
    // Because the old_code tested Unit modifies its argument '*netaddr' it
    // must be copied to a local variable, otherwise we may get a segfault with
    // old_code.
    constexpr int buflen{32};
    char netaddr[buflen]; // The buffer is modified by the tested Unit.

    // Get parameter
    std::tuple params = GetParam();
    ::strcpy(netaddr, std::get<0>(params));
    const size_t netaddrlen{strlen(netaddr)};
    const bool valid{std::get<1>(params)};

    // Testing for "[::]" is to ensure that the messages only one time output
    // and not with every test loop.
    if (old_code)
        if (strcmp(netaddr, "[::]") == 0) {
            std::cout
                << CYEL "[ BUGFIX   ] " CRES << __LINE__
                << ": Unit must not check unknown netaddresses as valid.\n";
            std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                      << ": Unit must not accept invalid port number 65536 as "
                         "valid.\n";
        }

    // Test Unit
    EXPECT_EQ(host_header_is_numeric(netaddr, netaddrlen), valid);
}

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
// clang-format off
INSTANTIATE_TEST_SUITE_P(
    // The netaddresses must not exceed 'buflen', see TEST_P(HeaderIsNumTest, ..
    HeaderIsNumOldCode, HeaderIsNumTest,
    ::testing::Values(
                      // std::make_tuple("", false), // This randomly segfaults.
                      // Next conditions must be false. They are Wrong!
                      std::make_tuple("::", false),
                      std::make_tuple("[::]", true),
                      std::make_tuple("[::]:0", true),
                      std::make_tuple("[::]:50016", true),
                      std::make_tuple("0.0.0.0", true),
                      std::make_tuple("0.0.0.0:0", true),
                      std::make_tuple("0.0.0.0:50046", true),
                      // The highest valid port number is 65535.
                      std::make_tuple("192.168.88.91:65536", true)
                      ));
#else
INSTANTIATE_TEST_SUITE_P(
    // The netaddresses must not exceed 'buflen'.
    HeaderIsNumNewCode, HeaderIsNumTest,
    ::testing::Values(
                      std::make_tuple("", false), // segfaults fixed.
                      std::make_tuple("::", true),
                      std::make_tuple("[::]", false),
                      std::make_tuple("[::]:0", false),
                      std::make_tuple("[::]:50016", false),
                      std::make_tuple("0.0.0.0", false),
                      std::make_tuple("0.0.0.0:0", false),
                      std::make_tuple("0.0.0.0:50046", false),
                      // The highest valid port number is 65535.
                      std::make_tuple("192.168.88.92:65536", false)
                      ));
#endif
INSTANTIATE_TEST_SUITE_P(
    // The netaddresses must not exceed 'buflen'.
    HeaderIsNum, HeaderIsNumTest,
    ::testing::Values(
                      std::make_tuple("[", false),
                      std::make_tuple("]", false),
                      std::make_tuple("[]", false),
                      std::make_tuple(":", false),
                      std::make_tuple(".", false),
                      std::make_tuple(".:", false),
                      std::make_tuple(":.", false),
                      std::make_tuple(":::", false),
                      std::make_tuple("[::", false),
                      std::make_tuple("::]", false),
                      std::make_tuple("[::1", false),
                      std::make_tuple("::1]", false),
                      std::make_tuple("[::1]", true),
                      std::make_tuple("[::1]:0", true),
                      std::make_tuple("[::1].4", false),
                      std::make_tuple("127.0.0.1", true),
                      std::make_tuple("127.0.0.1:", true),
                      std::make_tuple("127.0.0.1:0", true),
                      std::make_tuple("127.0.0.1.4", false),
                      std::make_tuple("[2001:db8::42]:59877", true),
                      std::make_tuple("[2001:db8::46]", true),
                      std::make_tuple("192.168.88.99:59876", true),
                      std::make_tuple("192.168.88.256:59866", false),
                      std::make_tuple("192.168.88.91", true),
                      std::make_tuple("[2001:db8::42]:", true),
                      std::make_tuple("2001:db8::41:59897", false),
                      std::make_tuple("[2001:db8::fg]:59877", false),
                      std::make_tuple("garbage", false)
                      ));
// clang-format on

TEST(RunMiniServerTestSuite, set_http_get_callback) {
    memset(&gGetCallback, 0xAA, sizeof(gGetCallback));
    SetHTTPGetCallback(web_server_callback);
    EXPECT_EQ(gGetCallback,
              static_cast<MiniServerCallback>(web_server_callback));
}

TEST(RunMiniServerTestSuite, set_soap_callback) {
    memset(&gSoapCallback, 0xAA, sizeof(gSoapCallback));
    SetSoapCallback(nullptr);
    EXPECT_EQ(gSoapCallback, static_cast<MiniServerCallback>(nullptr));
}

TEST(RunMiniServerTestSuite, set_gena_callback) {
    memset(&gGenaCallback, 0xAA, sizeof(gGenaCallback));
    SetGenaCallback(nullptr);
    EXPECT_EQ(gGenaCallback, static_cast<MiniServerCallback>(nullptr));
}

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
TEST(RunMiniServerTestSuite, do_reinit) {
    // On reinit the socket file descriptor will be closed and a new file
    // descriptor is requested. Mostly it is the same but it is possible that
    // it changes when other socket fds are requested.

    ASSERT_FALSE(MINISERVER_REUSEADDR);
    SSockaddr saddrObj;
    const char text_addr[]{"192.168.202.244"};
    saddrObj = text_addr;

    // Get a valid socket, needs initialized sockets on MS Windows.
    CSocket sockObj;

    s_SocketStuff s;
    // Fill all fields of struct s_SocketStuff
    s.ss.ss_family = saddrObj.ss.ss_family;
    s.serverAddr = &saddrObj.sa;
    s.ip_version = 4;
    s.text_addr = text_addr;
    s.serverAddr4->sin_port = saddrObj.port(); // not used
    s.serverAddr4->sin_addr = saddrObj.sin.sin_addr;
    s.fd = sockObj;
    s.try_port = 0; // not used
    s.actual_port = 0;
    s.address_len = sizeof(*s.serverAddr4);

    // Test Unit
    EXPECT_EQ(do_reinit(&s), 0);

    EXPECT_STREQ(s.text_addr, text_addr);
    EXPECT_EQ(s.serverAddr4->sin_addr.s_addr, saddrObj.sin.sin_addr.s_addr);
    // Valid real socket
    // EXPECT_EQ(s.fd, sockfd); This is an invalid condition. The fd may change.
    EXPECT_NE(s.fd, INVALID_SOCKET);
    EXPECT_EQ(s.try_port, 0);
    EXPECT_EQ(s.actual_port, 0);
    EXPECT_EQ(s.address_len, sizeof(*s.serverAddr4));
}
#endif

TEST(StopMiniServerTestSuite, sock_close) {
    // Close invalid sockets
    EXPECT_EQ(sock_close(INVALID_SOCKET), -1);
    EXPECT_EQ(sock_close(1234), -1);

    // Get a valid socket, needs initialized sockets on MS Windows.
    const SOCKET sockfd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_NE(sockfd, static_cast<SOCKET>(-1));
    // Close a valid socket.
    EXPECT_EQ(sock_close(sockfd), 0);
}

} // namespace utest

int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include "utest/utest_main.inc"
    return gtest_return_code; // managed in gtest_main.inc
}
