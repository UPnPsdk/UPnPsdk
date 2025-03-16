// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-03-17

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
#include <Pupnp/upnp/src/api/upnpapi.cpp>
#else
#include <Compa/src/api/upnpapi.cpp>
#endif

#ifdef UPNP_HAVE_TOOLS
#include <upnptools.hpp> // For pupnp and compa
#endif

#include <pupnp/upnpdebug.hpp> // for CLogging

#include <UPnPsdk/upnptools.hpp>
#include <UPnPsdk/addrinfo.hpp>
#include <UPnPsdk/netadapter.hpp>

#include <utest/utest.hpp>
#include <umock/sys_socket_mock.hpp>
#include <umock/pupnp_sock_mock.hpp>
#include <umock/winsock2_mock.hpp>


namespace utest {

using ::UPnPsdk::CAddrinfo;
using ::UPnPsdk::errStrEx;
using ::UPnPsdk::g_dbug;

using ::testing::_;
using ::testing::A;
using ::testing::AnyOf;
using ::testing::ExitedWithCode;
using ::testing::Not;
using ::testing::NotNull;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SetErrnoAndReturn;
using ::testing::StartsWith;
using ::testing::StrictMock;

using ::UPnPsdk::CNetadapter;
using ::UPnPsdk::SSockaddr;


// The UpnpInit2() call stack to initialize the pupnp library
//===========================================================
/*
clang-format off

     UpnpInit2()
03)  |__ ithread_mutex_lock()
03)  |__ UpnpInitPreamble()
05)  |   |__ UpnpInitLog()
     |   |__ UpnpInitMutexes()
03)  |   |__ Initialize_handle_list
03)  |   |__ UpnpInitThreadPools()
     |   |__ SetSoapCallback() - if enabled
     |   |__ SetGenaCallback() - if enabled
03)  |   |__ TimerThreadInit()
     |
     |__ UpnpGetIfInfo()
     |#ifdef _WIN32
13)  |   |__ GetAdaptersAddresses() and interface info
     |#else
14)  |   |__ getifaddrs() and interface info
     |   |__ freeifaddrs()
     |#endif
     |
     |__ UpnpInitStartServers()
17)  |   |__ StartMiniServer() - if enabled
     |   |__ UpnpEnableWebserver() - if enabled
     |       |__ if WEB_SERVER_ENABLED
     |              web_server_init()
     |           else
     |              web_server_destroy()
     |
     |__ ithread_mutex_unlock()

03) TEST(UpnpapiTestSuite, UpnpInitPreamble)
05) Tested with ./test_upnpdebug.cpp
11) Tested with ./test_TimerThread.cpp
13) Tested with ./test_upnpapi_win32.cpp
14) Tested with ./test_upnpapi_unix.cpp
17) Tested with ./test_miniserver.cpp


     UpnpFinish()
     |#ifdef UPnPsdk_HAVE_OPENSSL
     |__ SSL_CTX_free()
     |#endif
     |__ if not UpnpSdkInit
            return
     |   else
     |
     |#ifdef COMPA_HAVE_DEVICE_SSDP
     |__ while GetDeviceHandleInfo()
     |      UpnpUnRegisterRootDevice()
     |#endif
     |
     |#ifdef COMPA_HAVE_CTRLPT_SSDP
01)  |__ while GetClientHandleInfo()
02)  |         |__ GetHandleInfo()
     |      UpnpUnRegisterClient()
     |#endif
     |
     |__ TimerThreadShutdown()
     |__ StopMiniServer()
     |__ web_server_destroy()
     |__ ThreadPoolShutdown()
     |
     |#ifdef COMPA_HAVE_CTRLPT_SSDP
     |__    ithread_mutex_destroy() for clients
     |#endif
     |
     |__ ithread_rwlock_destroy()
     |__ ithread_mutex_destroy()
     |__ UpnpRemoveAllVirtualDirs()
     |__ ithread_cleanup_library()

02) TEST(Upnpapi*, GetHandleInfo_*)

clang-format on
*/

// upnpapi TestSuites
// ==================
// General storage for temporary socket address evaluation
SSockaddr saObj;

class UpnpapiFTestSuite : public ::testing::Test {
  protected:
// Old code UpnpInitLog() does not understand the object logObj and crashes
// with an exeption if g_dbug is enabled. I cannot use Clogging when testing
// UpnpInitLog().
#ifndef UPnPsdk_WITH_NATIVE_PUPNP
    pupnp::CLogging logObj; // Output only with build type DEBUG.

    // Constructor
    UpnpapiFTestSuite() {
        if (g_dbug)
            logObj.enable(UPNP_INFO);
#else
    // Constructor
    UpnpapiFTestSuite() {
#endif
        // initialize needed global variables
        memset(&gIF_NAME, 0, sizeof(gIF_NAME));
        memset(&gIF_IPV4, 0, sizeof(gIF_IPV4));
        memset(&gIF_IPV4_NETMASK, 0, sizeof(gIF_IPV4_NETMASK));
        memset(&gIF_IPV6, 0, sizeof(gIF_IPV6));
        gIF_IPV6_PREFIX_LENGTH = 0;
        memset(&gIF_IPV6_ULA_GUA, 0, sizeof(gIF_IPV6_ULA_GUA));
        gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
        gIF_INDEX = 0;

        // Destroy global variables to avoid side effects.
        UpnpSdkInit = 0xAA;
        memset(&errno, 0xAA, sizeof(errno));
        memset(&GlobalHndRWLock, 0xAA, sizeof(GlobalHndRWLock));
        // memset(&gWebMutex, 0xAA, sizeof(gWebMutex));
        memset(&gUUIDMutex, 0xAA, sizeof(gUUIDMutex));
        memset(&GlobalClientSubscribeMutex, 0xAA,
               sizeof(GlobalClientSubscribeMutex));
        memset(&gUpnpSdkNLSuuid, 0, sizeof(gUpnpSdkNLSuuid));
        memset(&HandleTable, 0xAA, sizeof(HandleTable));
        memset(&gSendThreadPool, 0xAA, sizeof(gSendThreadPool));
        memset(&gRecvThreadPool, 0xAA, sizeof(gRecvThreadPool));
        memset(&gMiniServerThreadPool, 0xAA, sizeof(gMiniServerThreadPool));
        memset(&gTimerThread, 0xAA, sizeof(gTimerThread));
        memset(&bWebServerState, 0xAA, sizeof(bWebServerState));
    }
};

class UpnpapiMockFTestSuite : public UpnpapiFTestSuite {
  protected:
    // clang-format off
    // Instantiate mocking objects.
    StrictMock<umock::PupnpSockMock> m_pupnpSockObj;
    StrictMock<umock::Sys_socketMock> m_sys_socketObj;
    // Inject the mocking objects into the tested code.
    umock::PupnpSock pupnp_sock_injectObj = umock::PupnpSock(&m_pupnpSockObj);
    umock::Sys_socket sys_socket_injectObj = umock::Sys_socket(&m_sys_socketObj);
#ifdef _WIN32
    umock::Winsock2Mock m_winsock2Obj;
    umock::Winsock2 winsock2_injectObj = umock::Winsock2(&m_winsock2Obj);
#endif
    // clang-format on
};


TEST_F(UpnpapiFTestSuite, UpnpInitPreamble_successful) {
    // Test Unit
    // ---------
    // UpnpInitPreamble() should not use and modify the UpnpSdkInit flag.
    int ret_UpnpInitPreamble = UpnpInitPreamble();
    ASSERT_EQ(ret_UpnpInitPreamble, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInitPreamble, UPNP_E_SUCCESS);

    // Check initialization of debug output.
    int ret_UpnpInitLog = UpnpInitLog();
    ASSERT_EQ(ret_UpnpInitLog, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInitLog, UPNP_E_SUCCESS);

    // Check if global mutexe are initialized
    ASSERT_EQ(pthread_mutex_trylock(&GlobalHndRWLock), 0);
    EXPECT_EQ(pthread_mutex_unlock(&GlobalHndRWLock), 0);

    ASSERT_EQ(pthread_mutex_trylock(&gUUIDMutex), 0);
    EXPECT_EQ(pthread_mutex_unlock(&gUUIDMutex), 0);

    ASSERT_EQ(pthread_mutex_trylock(&GlobalClientSubscribeMutex), 0);
    EXPECT_EQ(pthread_mutex_unlock(&GlobalClientSubscribeMutex), 0);

    // Check creation of a uuid
    EXPECT_THAT(gUpnpSdkNLSuuid,
                MatchesStdRegex("[[:xdigit:]]{8}-[[:xdigit:]]{4}-[[:xdigit:]]{"
                                "4}-[[:xdigit:]]{4}-[[:xdigit:]]{12}"));

    // Check initialization of the UPnP device and client (control point) handle
    // table
    bool handleTable_initialized{true};
    for (int i = 0; i < NUM_HANDLE; ++i) {
        if (HandleTable[i] != nullptr) {
            handleTable_initialized = false;
            break;
        }
    }
    EXPECT_TRUE(handleTable_initialized);

    // Check threadpool initialization
    EXPECT_EQ(gSendThreadPool.totalThreads, 3);
    EXPECT_EQ(gSendThreadPool.busyThreads, 1);
    EXPECT_EQ(gSendThreadPool.persistentThreads, 1);

    EXPECT_EQ(gRecvThreadPool.totalThreads, 2);
    EXPECT_EQ(gRecvThreadPool.busyThreads, 0);
    EXPECT_EQ(gRecvThreadPool.persistentThreads, 0);

    EXPECT_EQ(gMiniServerThreadPool.totalThreads, 2);
    EXPECT_EQ(gMiniServerThreadPool.busyThreads, 0);
    EXPECT_EQ(gMiniServerThreadPool.persistentThreads, 0);

    // Check settings of MiniServer callback functions SetSoapCallback() and
    // SetGenaCallback() aren't possible wihout access to static gSoapCallback
    // and gGenaCallback variables in miniserver.cpp. This is tested with
    // MiniServer module.

    // Check timer thread initialization
    EXPECT_EQ(gTimerThread.lastEventId, 0);
    EXPECT_EQ(gTimerThread.shutdown, 0);
    EXPECT_EQ(gTimerThread.tp, &gSendThreadPool);

    // Check if UpnpSdkInit has been modified
    EXPECT_EQ(UpnpSdkInit, 0xAA);

    // TODO
    // Check if all this is cleaned up successfully
    // --------------------------------------------
    UpnpSdkInit = 1;
    int ret_UpnpFinish = UpnpFinish();
    EXPECT_EQ(ret_UpnpFinish, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpFinish, UPNP_E_SUCCESS);

    EXPECT_EQ(UpnpSdkInit, 0);
}

TEST_F(UpnpapiFTestSuite, get_error_message) {
#ifndef UPNP_HAVE_TOOLS
    std::cout
        << "               Skipped: Option UPnPsdk_WITH_TOOLS not available.\n";
#else
    EXPECT_STREQ(UpnpGetErrorMessage(0), "UPNP_E_SUCCESS");
    EXPECT_STREQ(UpnpGetErrorMessage(-121), "UPNP_E_INVALID_INTERFACE");
    EXPECT_STREQ(UpnpGetErrorMessage(1), "Unknown error code");
#endif
}

TEST_F(UpnpapiFTestSuite, GetHandleInfo_successful) {
    // Will be filled with a pointer to the requested client info.
    Handle_Info* hinfo_p{nullptr};

    // Initialize the handle list.
    for (int i = 0; i < NUM_HANDLE; ++i) {
        HandleTable[i] = nullptr;
    }
    Handle_Info hinfo0{};
    HandleTable[0] = &hinfo0;
    HandleTable[0]->HType = HND_INVALID;
    // HandleTable[1] is nullptr from initialization before;
    Handle_Info hinfo2{};
    HandleTable[2] = &hinfo2;
    HandleTable[2]->HType = HND_CLIENT;
    Handle_Info hinfo3{};
    HandleTable[3] = &hinfo3;
    HandleTable[3]->HType = HND_DEVICE;
    Handle_Info hinfo4{};
    HandleTable[4] = &hinfo4;
    HandleTable[4]->HType = HND_CLIENT;

    // Test Unit
    EXPECT_EQ(GetHandleInfo(0, &hinfo_p), HND_INVALID);
    // Out of range, nothing returned.
    EXPECT_EQ(hinfo_p, nullptr);
    EXPECT_EQ(GetHandleInfo(NUM_HANDLE, &hinfo_p), HND_INVALID);
    EXPECT_EQ(hinfo_p, nullptr);
    EXPECT_EQ(GetHandleInfo(NUM_HANDLE + 1, &hinfo_p), HND_INVALID);
    EXPECT_EQ(hinfo_p, nullptr);

    EXPECT_EQ(GetHandleInfo(1, &hinfo_p), HND_INVALID); // HandleTable nullptr
    // Nothing returned.
    EXPECT_EQ(hinfo_p, nullptr);

    EXPECT_EQ(GetHandleInfo(3, &hinfo_p), HND_DEVICE);
    // Pointer to handle info is returned.
    EXPECT_EQ(hinfo_p, &hinfo3);

    EXPECT_EQ(GetHandleInfo(4, &hinfo_p), HND_CLIENT);
    // Pointer to handle info is returned.
    EXPECT_EQ(hinfo_p, &hinfo4);
}

TEST_F(UpnpapiFTestSuite, GetHandleInfo_with_nullptr_to_handle_table) {
    // Initialize HandleTable bcause it only contains pointer.
    HandleTable[1] = nullptr;

    // Test Unit with nullptr to result variable
    EXPECT_EQ(GetHandleInfo(1, nullptr), HND_INVALID);

    // This will be filled with a pointer to the requested client info.
    Handle_Info* hinfo_p{nullptr};

    // Test Unit
    EXPECT_EQ(GetHandleInfo(1, &hinfo_p), HND_INVALID);
}

TEST_F(UpnpapiFTestSuite, UpnpFinish_successful) {
    // Doing needed initializations. Otherwise we get segfaults with
    // UpnpFinish() due to uninitialized pointers.
    // Initialize SDK global mutexes.
    ASSERT_EQ(UpnpInitMutexes(), UPNP_E_SUCCESS);

    // Initialize the handle list.
    HandleLock();
    for (int i = 0; i < NUM_HANDLE; ++i)
        HandleTable[i] = nullptr;
    HandleUnlock();

    // Initialize SDK global thread pools.
    ASSERT_EQ(UpnpInitThreadPools(), UPNP_E_SUCCESS);

    // Initialize the SDK timer thread.
    ASSERT_EQ(TimerThreadInit(&gTimerThread, &gSendThreadPool), UPNP_E_SUCCESS);

    UpnpSdkInit = 1;

    // Test Unit
    int ret_UpnpFinish{UPNP_E_INTERNAL_ERROR};
    ret_UpnpFinish = UpnpFinish();
    EXPECT_EQ(ret_UpnpFinish, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpFinish, UPNP_E_SUCCESS);

    EXPECT_EQ(UpnpSdkInit, 0);
}

TEST_F(UpnpapiFTestSuite, UpnpFinish_without_initialization) {
    UpnpSdkInit = 0;

    // Test Unit
    int ret_UpnpFinish{UPNP_E_INTERNAL_ERROR};
    ret_UpnpFinish = UpnpFinish();
    EXPECT_EQ(ret_UpnpFinish, UPNP_E_FINISH)
        << errStrEx(ret_UpnpFinish, UPNP_E_FINISH);

    EXPECT_EQ(UpnpSdkInit, 0);
}

TEST_F(UpnpapiFTestSuite, webserver_enable_and_disable) {
    // Note that UpnpSetWebServerRootDir(<rootDir>) also enables the webserver,
    //  and that UpnpSetWebServerRootDir(nullptr) also disables the webserver.

    // The Unit needs a defined state, otherwise it will fail with
    // SEH exception 0xc0000005 on WIN32.
    bWebServerState = WEB_SERVER_DISABLED;
    UpnpSdkInit = 1;

    // Test Unit enable
    int ret_UpnpEnableWebserver = UpnpEnableWebserver(WEB_SERVER_ENABLED);
    EXPECT_EQ(ret_UpnpEnableWebserver, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpEnableWebserver, UPNP_E_SUCCESS);

    EXPECT_EQ(bWebServerState, WEB_SERVER_ENABLED);

    // Test Unit enable it again should not do any harm.
    ret_UpnpEnableWebserver = UpnpEnableWebserver(WEB_SERVER_ENABLED);
    EXPECT_EQ(ret_UpnpEnableWebserver, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpEnableWebserver, UPNP_E_SUCCESS);

    EXPECT_EQ(bWebServerState, WEB_SERVER_ENABLED);

    // Test Unit disable
    ret_UpnpEnableWebserver = UpnpEnableWebserver(WEB_SERVER_DISABLED);
    EXPECT_EQ(ret_UpnpEnableWebserver, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpEnableWebserver, UPNP_E_SUCCESS);

    EXPECT_EQ(bWebServerState, WEB_SERVER_DISABLED);

    // Test Unit disable again should not do any harm.
    ret_UpnpEnableWebserver = UpnpEnableWebserver(WEB_SERVER_DISABLED);
    EXPECT_EQ(ret_UpnpEnableWebserver, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpEnableWebserver, UPNP_E_SUCCESS);

    EXPECT_EQ(bWebServerState, WEB_SERVER_DISABLED);
}

TEST_F(UpnpapiFTestSuite, webserver_set_rootdir_successful) {
    UpnpSdkInit = 1;

    // Test Unit
    int ret_UpnpSetWebServerRootDir = UpnpSetWebServerRootDir("sample/web/");
    EXPECT_EQ(ret_UpnpSetWebServerRootDir, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpSetWebServerRootDir, UPNP_E_SUCCESS);

    EXPECT_STREQ(gDocumentRootDir.buf, "sample/web");

    ret_UpnpSetWebServerRootDir = UpnpSetWebServerRootDir("/");
    EXPECT_EQ(ret_UpnpSetWebServerRootDir, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpSetWebServerRootDir, UPNP_E_SUCCESS);

    EXPECT_STREQ(gDocumentRootDir.buf, "");

    ret_UpnpSetWebServerRootDir = UpnpSetWebServerRootDir("//");
    EXPECT_EQ(ret_UpnpSetWebServerRootDir, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpSetWebServerRootDir, UPNP_E_SUCCESS);

    EXPECT_STREQ(gDocumentRootDir.buf, "/");

    ret_UpnpSetWebServerRootDir = UpnpSetWebServerRootDir(".");
    EXPECT_EQ(ret_UpnpSetWebServerRootDir, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpSetWebServerRootDir, UPNP_E_SUCCESS);

    EXPECT_STREQ(gDocumentRootDir.buf, ".");

    ret_UpnpSetWebServerRootDir = UpnpSetWebServerRootDir("./");
    EXPECT_EQ(ret_UpnpSetWebServerRootDir, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpSetWebServerRootDir, UPNP_E_SUCCESS);

    EXPECT_STREQ(gDocumentRootDir.buf, ".");

    ret_UpnpSetWebServerRootDir = UpnpSetWebServerRootDir("..");
    EXPECT_EQ(ret_UpnpSetWebServerRootDir, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpSetWebServerRootDir, UPNP_E_SUCCESS);

    EXPECT_STREQ(gDocumentRootDir.buf, "..");

    ret_UpnpSetWebServerRootDir = UpnpSetWebServerRootDir("../");
    EXPECT_EQ(ret_UpnpSetWebServerRootDir, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpSetWebServerRootDir, UPNP_E_SUCCESS);

    EXPECT_STREQ(gDocumentRootDir.buf, "..");
}

TEST_F(UpnpapiFTestSuite, webserver_set_rootdir_fails) {
    // Test Unit
    UpnpSdkInit = 0;
    int ret_UpnpSetWebServerRootDir = UpnpSetWebServerRootDir("sample/web");
    EXPECT_EQ(ret_UpnpSetWebServerRootDir, UPNP_E_FINISH)
        << errStrEx(ret_UpnpSetWebServerRootDir, UPNP_E_FINISH);

    UpnpSdkInit = 1;
    ret_UpnpSetWebServerRootDir = UpnpSetWebServerRootDir(nullptr);
    EXPECT_EQ(ret_UpnpSetWebServerRootDir, UPNP_E_INVALID_PARAM)
        << errStrEx(ret_UpnpSetWebServerRootDir, UPNP_E_INVALID_PARAM);

    ret_UpnpSetWebServerRootDir = UpnpSetWebServerRootDir("");
    EXPECT_EQ(ret_UpnpSetWebServerRootDir, UPNP_E_INVALID_PARAM)
        << errStrEx(ret_UpnpSetWebServerRootDir, UPNP_E_INVALID_PARAM);
}

TEST_F(UpnpapiFTestSuite, webserver_sdk_not_initialized) {
    // The Unit needs a defined state, otherwise it will fail with
    // SEH exception 0xc0000005 on WIN32.
    bWebServerState = WEB_SERVER_DISABLED;
    UpnpSdkInit = 0;

    // Test Unit
    int ret_UpnpEnableWebserver = UpnpEnableWebserver(WEB_SERVER_ENABLED);
    EXPECT_EQ(ret_UpnpEnableWebserver, UPNP_E_FINISH)
        << errStrEx(ret_UpnpEnableWebserver, UPNP_E_FINISH);

    EXPECT_EQ(bWebServerState, WEB_SERVER_DISABLED);
}


int CallbackEventHandler(Upnp_EventType EventType, const void* Event,
                         [[maybe_unused]] void* Cookie) {

    // Print a summary of the event received
    std::cout << "Received event type \"" << EventType << "\" with event '"
              << Event << "'\n";
    return 0;
}

TEST_F(UpnpapiFTestSuite, UpnpRegisterRootDevice3_successful) {
    if (github_actions)
        GTEST_SKIP() << "             test needs inprovements.";

    constexpr char desc_doc_url[]{
        "http://[2003:d5:2748:ae00:5054:ff:fe7f:c021]:53779/tvdevicedesc.xml"};
    UpnpDevice_Handle device_handle = -1;
    // Initialize the handle list.
    // for (int i = 0; i < NUM_HANDLE; ++i) {
    //     HandleTable[i] = nullptr;
    // }

    int ret_UpnpInit2 = ::UpnpInit2("ens1", 50001);
    ASSERT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

    UpnpSdkInit = 1;

    // Test Unit
    int ret_UpnpRegisterRootDevice3 =
        UpnpRegisterRootDevice3(desc_doc_url, CallbackEventHandler,
                                &device_handle, &device_handle, AF_INET6);
    EXPECT_EQ(ret_UpnpRegisterRootDevice3, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpRegisterRootDevice3, UPNP_E_SUCCESS);

    UpnpUnRegisterRootDevice(device_handle);
    UpnpFinish();
}

TEST_F(UpnpapiMockFTestSuite, UpnpRegisterRootDevice3_successful) {
    GTEST_SKIP() << "Need to test subroutines first.";

    constexpr char desc_doc_url[]{"http://192.168.99.4:50010/tvdevicedesc.xml"};
    constexpr SOCKET sockfd{umock::sfd_base + 45};
    UpnpDevice_Handle device_handle = -1;

    // Initialization preamble to have essential structures initialized.
    int ret_UpnpInitPreamble = UpnpInitPreamble();
    ASSERT_EQ(ret_UpnpInitPreamble, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInitPreamble, UPNP_E_SUCCESS);

    EXPECT_CALL(m_sys_socketObj, socket(AF_INET, SOCK_STREAM, 0))
        .WillOnce(Return(sockfd));
    EXPECT_CALL(m_pupnpSockObj, sock_make_no_blocking(sockfd)).Times(1);
    EXPECT_CALL(m_pupnpSockObj, sock_make_blocking(sockfd)).Times(1);
    EXPECT_CALL(m_sys_socketObj,
                select(sockfd + 1, NULL, NotNull(), NULL, NotNull()))
        .WillOnce(Return(1));
    EXPECT_CALL(m_sys_socketObj,
                select(sockfd + 1, NotNull(), NotNull(), NULL, NotNull()))
        .Times(2)
        .WillRepeatedly(Return(1));
    EXPECT_CALL(m_sys_socketObj, connect(sockfd, _, sizeof(sockaddr_in)))
        .WillOnce(SetErrnoAndReturn(EINPROGRESS, -1));
    EXPECT_CALL(m_sys_socketObj, getsockopt(sockfd, SOL_SOCKET, SO_ERROR, _, _))
        .Times(1);
    EXPECT_CALL(m_sys_socketObj, send(sockfd, _, _, _)).WillOnce(Return(200));
    EXPECT_CALL(m_sys_socketObj, recv(sockfd, _, 1024, _)).Times(1);
    EXPECT_CALL(m_sys_socketObj, shutdown(sockfd, _)).Times(1);

    UpnpSdkInit = 1;
    { // Scope for logging
        // pupnp::CLogging logObj; // Output only with build type DEBUG.
        // logObj.enable(UPNP_ALL);

        // Test Unit
        int ret_UpnpRegisterRootDevice3 =
            UpnpRegisterRootDevice3(desc_doc_url, CallbackEventHandler,
                                    &device_handle, &device_handle, AF_INET6);
        EXPECT_EQ(ret_UpnpRegisterRootDevice3, UPNP_E_SUCCESS)
            << errStrEx(ret_UpnpRegisterRootDevice3, UPNP_E_SUCCESS);

        EXPECT_EQ(device_handle, 1);

        // The handle table was initialized with UpnpInitPreamble() and should
        // have the device_handle info now.
        ASSERT_NE(HandleTable[device_handle], nullptr);
        Handle_Info* HInfo{HandleTable[device_handle]};
        EXPECT_EQ(HInfo->aliasInstalled, 0);

        // Finish
        int ret_UpnpUnRegisterRootDevice =
            UpnpUnRegisterRootDevice(device_handle);
        EXPECT_EQ(ret_UpnpUnRegisterRootDevice, UPNP_E_SUCCESS)
            << errStrEx(ret_UpnpUnRegisterRootDevice, UPNP_E_SUCCESS);

    } // End scope for logging

    // Finish the library
    int ret_UpnpFinish = UpnpFinish();
    EXPECT_EQ(ret_UpnpFinish, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpFinish, UPNP_E_SUCCESS);
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_with_loopback_ipv6_iface_successful) {
    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo("[::1]");

    if (old_code) {
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": Using the local network loopback interface should be "
                     "supported.\n";
        ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);

    } else {

        EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

        EXPECT_STRNE(gIF_NAME, "");
        EXPECT_NE(gIF_INDEX, 0);
        EXPECT_STREQ(gIF_IPV4, "");
        EXPECT_STREQ(gIF_IPV4_NETMASK, "");
        // The loopback address belongs to link-local unicast addresses.
        EXPECT_STREQ(gIF_IPV6, "::1");
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 128);
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
    }
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_with_loopback_ipv4_iface_successful) {
    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo("127.0.0.1");

    if (old_code) {
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": Using the local network loopback interface should be "
                     "supported.\n";
        ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);

    } else {

        EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

        EXPECT_STRNE(gIF_NAME, "");
        EXPECT_NE(gIF_INDEX, 0);
        EXPECT_STREQ(gIF_IPV4, "127.0.0.1");
        EXPECT_STREQ(gIF_IPV4_NETMASK, "255.0.0.0");
        // The loopback address belongs to link-local unicast addresses.
        EXPECT_STREQ(gIF_IPV6, "");
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
    }
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_with_loopback_iface_successful) {
    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo("loopback");

    if (old_code) {
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": Using the loopback address should be supported.\n";
        ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);

    } else {

        ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

        EXPECT_NE(gIF_NAME[0], '\0');
        EXPECT_NE(gIF_INDEX, 0);
        if (gIF_IPV4[0] != '\0') {
            EXPECT_STREQ(gIF_IPV4, "127.0.0.1");
            EXPECT_STREQ(gIF_IPV4_NETMASK, "255.0.0.0");
        } else {
            EXPECT_EQ(gIF_IPV4_NETMASK[0], '\0');
            ASSERT_STREQ(gIF_IPV6, "::1");
        }
        // The loopback address belongs to link-local unicast addresses.
        if (gIF_IPV6[0] != '\0') {
            EXPECT_STREQ(gIF_IPV6, "::1");
            EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 128);
        } else {
            EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
            ASSERT_STREQ(gIF_IPV4, "127.0.0.1");
        }
        EXPECT_EQ(gIF_IPV6_ULA_GUA[0], '\0');
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
    }
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_from_lla_successful) {
    // Get a valid IPv6 address from a local network adapter.
    CNetadapter nadaptObj;
    nadaptObj.get_first();
    do {
        nadaptObj.sockaddr(saObj);
        if (saObj.ss.ss_family == AF_INET6 &&
            IN6_IS_ADDR_LINKLOCAL(&saObj.sin6.sin6_addr))
            break;
    } while (nadaptObj.get_next());

    if (saObj.ss.ss_family != AF_INET6 ||
        !IN6_IS_ADDR_LINKLOCAL(&saObj.sin6.sin6_addr))
        GTEST_SKIP()
            << "No local network adapter with link local address found.";

    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo(saObj.netaddr().c_str());

    if (old_code) {
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": Using a link local address should be supported.\n";
        ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);

    } else {

        EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

        EXPECT_STRNE(gIF_NAME, "");
        EXPECT_NE(gIF_INDEX, 0);
        EXPECT_STREQ(gIF_IPV4, "");
        EXPECT_STREQ(gIF_IPV4_NETMASK, "");
        // The loopback address belongs to link-local unicast addresses.
        EXPECT_STRNE(gIF_IPV6, "");
        EXPECT_NE(gIF_IPV6_PREFIX_LENGTH, 0);
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
    }
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_from_gua_successful) {
    // Get a valid IPv6 address from a local network adapter.
    CNetadapter nadaptObj;
    nadaptObj.get_first();
    do {
        nadaptObj.sockaddr(saObj);
        if (saObj.ss.ss_family == AF_INET6 &&
            IN6_IS_ADDR_GLOBAL(&saObj.sin6.sin6_addr))
            break;
    } while (nadaptObj.get_next());

    if (saObj.ss.ss_family != AF_INET6 ||
        !IN6_IS_ADDR_GLOBAL(&saObj.sin6.sin6_addr))
        GTEST_SKIP()
            << "No local network adapter with link local address found.";

    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo(saObj.netaddr().c_str());

    if (old_code) {
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": Using a global unicast address should be supported.\n";
        ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);

    } else {

        ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

        EXPECT_STRNE(gIF_NAME, "");
        EXPECT_NE(gIF_INDEX, 0);
        EXPECT_STREQ(gIF_IPV4, "");
        EXPECT_STREQ(gIF_IPV4_NETMASK, "");
        // The loopback address belongs to link-local unicast addresses.
        EXPECT_STREQ(gIF_IPV6, "");
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
        EXPECT_STRNE(gIF_IPV6_ULA_GUA, "");
        EXPECT_NE(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
    }
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_ipv6_address_successful) {
    if (!github_actions)
        GTEST_FAIL() << "Still needs to be done.";
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_ipv4_address_successful) {
    // Get a valid IPv4 address from a local network adapter.
    CNetadapter nadaptObj;
    nadaptObj.get_first();
    do {
        nadaptObj.sockaddr(saObj);
        if (saObj.ss.ss_family == AF_INET)
            break;
    } while (nadaptObj.get_next());

    if (saObj.ss.ss_family != AF_INET)
        GTEST_SKIP() << "No local network adapter with IPv4 address found.";

    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo(saObj.netaddr().c_str());

    if (old_code) {
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": Using an IPv4 address should be supported.\n";
        ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);

    } else {

        ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

        EXPECT_STRNE(gIF_NAME, "");
        EXPECT_NE(gIF_INDEX, 0);
        EXPECT_STRNE(gIF_IPV4, "");
        EXPECT_STRNE(gIF_IPV4_NETMASK, "");
        // The loopback address belongs to link-local unicast addresses.
        EXPECT_STREQ(gIF_IPV6, "");
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
    }
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_with_ifname_successful) {
    if (github_actions)
        GTEST_SKIP() << "IP addresses need to be mocked.";

    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo("ens1");

    ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

    EXPECT_STREQ(gIF_NAME, "ens1");
    EXPECT_EQ(gIF_INDEX, 2);
    EXPECT_STREQ(gIF_IPV4, "");
    EXPECT_STREQ(gIF_IPV4_NETMASK, "");
    // The loopback address belongs to link-local unicast addresses.
    EXPECT_STREQ(gIF_IPV6, "fe80::5054:ff:fe7f:c021");
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 64);
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "2003:d5:2732:f300:5054:ff:fe7f:c021");
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 64);
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_default_successful) {
    if (github_actions)
        GTEST_SKIP() << "Still needs to be done.";

    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo(nullptr);

    ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

    EXPECT_STREQ(gIF_NAME, "ens1");
    EXPECT_EQ(gIF_INDEX, 2);
    EXPECT_STREQ(gIF_IPV4, "");
    EXPECT_STREQ(gIF_IPV4_NETMASK, "");
    // The loopback address belongs to link-local unicast addresses.
    EXPECT_STREQ(gIF_IPV6, "fe80::5054:ff:fe7f:c021");
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 64);
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "2003:d5:2732:f300:5054:ff:fe7f:c021");
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 64);
}

TEST_F(UpnpapiFTestSuite, get_free_handle_successful) {
    if (!github_actions)
        GTEST_FAIL() << "Still needs to be done.";

    [[maybe_unused]] int ret_GetFreeHandle = GetFreeHandle();
}

TEST_F(UpnpapiFTestSuite, download_xml_successful) {
    if (github_actions)
        GTEST_SKIP() << "Still needs to be done.";

    // A possible url is http://127.0.0.1:50001/tvdevicedesc.xml
    IXML_Document* xmldocbuf_ptr{nullptr};

    int ret_UpnpDownloadXmlDoc = UpnpDownloadXmlDoc(
        "https://localhost:443/sample/web/tvdevicedesc.xml", &xmldocbuf_ptr);
    EXPECT_EQ(ret_UpnpDownloadXmlDoc, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpDownloadXmlDoc, UPNP_E_SUCCESS);
}

TEST_F(UpnpapiFTestSuite, UpnpInit2_loopback_interface) {
    // The Unit needs a defined state, otherwise it will fail with
    // SEH exception 0xc0000005 on WIN32.
    bWebServerState = WEB_SERVER_DISABLED;

    // Test Unit
    int ret_UpnpInit2 = ::UpnpInit2("loopback", 0);

    if (old_code) {
        EXPECT_EQ(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE);
        GTEST_SKIP() << "Using the local network loopback interface is not "
                        "supported with pupnp.";
    }

    EXPECT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

    EXPECT_STRNE(gIF_NAME, "");
    EXPECT_NE(gIF_INDEX, 0);
    EXPECT_THAT(std::string(gIF_IPV4), AnyOf("", StartsWith("127.")));
    EXPECT_THAT(std::string(gIF_IPV4_NETMASK), AnyOf("", StartsWith("255.")));
    // A loopback address belongs to link-local unicast addresses.
    EXPECT_THAT(std::string(gIF_IPV6), AnyOf("::1", "", "fe80::1"));
    EXPECT_THAT(gIF_IPV6_PREFIX_LENGTH, AnyOf(128, 64, 0));
    // A loopback address is never a global unicast address.
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);

    UpnpFinish();
}

TEST_F(UpnpapiFTestSuite, UpnpInit2_default_successful) {
    // For Microsoft Windows there are some TODOs in the old code:
    // TODO: Retrieve IPv4 netmask
    // TODO: Retrieve IPv6 ULA or GUA address and its prefix. Only keep IPv6
    // link-local addresses.
    // TODO: Retrieve IPv6 LLA prefix

    // The Unit needs a defined state, otherwise it will fail with
    // SEH exception 0xc0000005 on WIN32.
    bWebServerState = WEB_SERVER_DISABLED;

    // Test Unit
    int ret_UpnpInit2 = ::UpnpInit2(nullptr, 0);
    EXPECT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

    // std::cerr << "DEBUG: gIF_IPV4=\"" << ::gIF_IPV4 << "\",gIF_IPV6=\""
    //           << ::gIF_IPV6 << "\", gIF_IPV6_ULA_GUA=\"" <<
    //           ::gIF_IPV6_ULA_GUA
    //           << "\".\n";

    EXPECT_STRNE(gIF_NAME, "");
    EXPECT_NE(gIF_INDEX, 0);
    EXPECT_FALSE(gIF_IPV4[0] == '\0' && gIF_IPV6[0] == '\0' &&
                 gIF_IPV6_ULA_GUA[0] == '\0');
#if defined(UPnPsdk_WITH_NATIVE_PUPNP) && defined(_MSC_VER)
    EXPECT_STREQ(gIF_IPV4_NETMASK, "");
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
    std::cout
        << CYEL "[ BUGFIX   ] " CRES << __LINE__
        << ": gIF_IPV4_NETMASK, gIF_IPV6_PREFIX_LENGTH, gIF_IPV6_ULA_GUA, and "
           "gIF_IPV6_PREFIX_LENGTH should be retrieved on wih32.\n";
#else
    if (gIF_IPV4[0] != '\0')
        EXPECT_STRNE(gIF_IPV4_NETMASK, "");
    if (gIF_IPV6[0] != '\0')
        EXPECT_NE(gIF_IPV6_PREFIX_LENGTH, 0);
    if (gIF_IPV6_ULA_GUA[0] != '\0')
        EXPECT_NE(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
#endif
    UpnpFinish();
}

TEST_F(UpnpapiFTestSuite, UpnpInit2_call_two_times) {
    if (github_actions)
        GTEST_SKIP() << "Still needs to be done.";

    // The Unit needs a defined state, otherwise it will fail with
    // SEH exception 0xc0000005 on WIN32.
    bWebServerState = WEB_SERVER_DISABLED;

    // Test Unit
    int ret1_UpnpInit2 = ::UpnpInit2(nullptr, 0);
    EXPECT_EQ(ret1_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret1_UpnpInit2, UPNP_E_SUCCESS);
    UpnpFinish();

    int ret2_UpnpInit2 = ::UpnpInit2(nullptr, 0);
    EXPECT_EQ(ret2_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret2_UpnpInit2, UPNP_E_SUCCESS);
    UpnpFinish();
}

} // namespace utest

int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
