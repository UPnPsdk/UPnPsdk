// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-09-14

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
#include <Pupnp/upnp/src/api/upnpapi.cpp>
#else
#include <Compa/src/api/upnpapi.cpp>
#endif

#ifdef UPNP_HAVE_TOOLS
#include <upnptools.hpp> // For pupnp and compa
#endif

#include <UPnPsdk/upnptools.hpp>
#include <UPnPsdk/netadapter.hpp>

#include <utest/utest.hpp>
#include <umock/sys_socket_mock.hpp>
#include <umock/pupnp_sock_mock.hpp>
#include <umock/winsock2_mock.hpp>


namespace utest {

using ::testing::_;
using ::testing::AnyOf;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::SetErrnoAndReturn;
using ::testing::StartsWith;
using ::testing::StrictMock;

using ::UPnPsdk::errStrEx;
using ::UPnPsdk::SSockaddr;

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
auto& sdkInit_mutex = gSDKInitMutex;
#else
using ::HandleTable;
using ::compa::sdkInit_mutex;
#endif


// The UpnpInit2() call stack to initialize the pupnp library
//===========================================================
/*
clang-format off

     UpnpInit2()
03)  |__ pthread_mutex_lock()
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
     |__ pthread_mutex_unlock()

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
     |__    pthread_mutex_destroy() for clients
     |#endif
     |
     |__ pthread_rwlock_destroy()
     |__ pthread_mutex_destroy()
     |__ UpnpRemoveAllVirtualDirs()

02) TEST(Upnpapi*, GetHandleInfo_*)

clang-format on
*/

// General storage for temporary socket address evaluation
SSockaddr saObj;

// Have the netadapter list global available so its expensive call from the
// operating system is only one time necessary.
UPnPsdk::CNetadapter nadaptObj;


// Real local ip addresses on this nodes network adapters, evaluated with
// CNetadapter.
struct SSaNadap {
    SSockaddr sa;
    unsigned int index{};
    unsigned int bitmask{};
    std::string name;
};
SSaNadap lo6Obj;
SSaNadap lo4Obj;
SSaNadap llaObj;
SSaNadap guaObj;
SSaNadap ip4Obj;

void get_netadapter() {
    // Getting information of the local network adapters is expensive because
    // it allocates memory to return the internal adapter list. So I do it one
    // time on start and provide the needed information.
    UPnPsdk::CNetadapter nadapObj;
    nadapObj.get_first();

    do {
        nadapObj.sockaddr(saObj);
        if (lo6Obj.sa.ss.ss_family == AF_UNSPEC && saObj.is_loopback() &&
            saObj.ss.ss_family == AF_INET6) {
            // Found first ipv6 loopback address.
            lo6Obj.sa = saObj;
            lo6Obj.index = nadapObj.index();
            lo6Obj.bitmask = nadapObj.bitmask();
            lo6Obj.name = nadapObj.name();
#if 0
        } else if (lo4Obj.sa.ss.ss_family == AF_UNSPEC && saObj.is_loopback() &&
                   saObj.ss.ss_family == AF_INET) {
            // Found first ipv4 loopback address.
            lo4Obj.sa = saObj;
            lo4Obj.index = nadapObj.index();
            lo4Obj.bitmask = nadapObj.bitmask();
            lo4Obj.name = nadapObj.name();
#endif
        } else if (llaObj.sa.ss.ss_family == AF_UNSPEC &&
                   saObj.ss.ss_family == AF_INET6 &&
                   IN6_IS_ADDR_LINKLOCAL(&saObj.sin6.sin6_addr)) {
            // Found first LLA address.
            llaObj.sa = saObj;
            llaObj.index = nadapObj.index();
            llaObj.bitmask = nadapObj.bitmask();
            llaObj.name = nadapObj.name();
        } else if (guaObj.sa.ss.ss_family == AF_UNSPEC &&
                   saObj.ss.ss_family == AF_INET6 &&
                   (IN6_IS_ADDR_GLOBAL(&saObj.sin6.sin6_addr) ||
                    IN6_IS_ADDR_V4MAPPED(&saObj.sin6.sin6_addr))) {
            // Found first GUA address.
            guaObj.sa = saObj;
            guaObj.index = nadapObj.index();
            guaObj.bitmask = nadapObj.bitmask();
            guaObj.name = nadapObj.name();
        } else if (ip4Obj.sa.ss.ss_family == AF_UNSPEC &&
                   saObj.ss.ss_family == AF_INET && !saObj.is_loopback()) {
            // Found first IPv4 address.
            throw std::runtime_error("AF_INET detected with address=\"" +
                                     saObj.netaddrp() +
                                     "\". Only AF_INET6 with IPv4 mapped IPv6 "
                                     "addresses are supported.");
            // ip4Obj.sa = saObj;
            // ip4Obj.index = nadapObj.index();
            // ip4Obj.bitmask = nadapObj.bitmask();
            // ip4Obj.name = nadapObj.name();
        }
    } while (nadapObj.get_next() && (lo6Obj.sa.ss.ss_family == AF_UNSPEC ||
                                     // lo4Obj.sa.ss.ss_family == AF_UNSPEC ||
                                     llaObj.sa.ss.ss_family == AF_UNSPEC ||
                                     guaObj.sa.ss.ss_family == AF_UNSPEC ||
                                     ip4Obj.sa.ss.ss_family == AF_UNSPEC));
}

// upnpapi TestSuites
// ==================
class UpnpapiFTestSuite : public ::testing::Test {
  protected:
// Old code UpnpInitLog() does not understand the object logObj and crashes
// on win32 randomly with exeption 0xc0000409 (STATUS_STACK_BUFFER_OVERRUN) if
// g_dbug is enabled. I cannot use Clogging when testing UpnpInitLog().
#if !defined(UPnPsdk_WITH_NATIVE_PUPNP) || !defined(_MSC_VER)
    // pupnp::CLogging logObj; // Output only with build type DEBUG.

    // Constructor
    UpnpapiFTestSuite(){
    // if (UPnPsdk::g_dbug)
    //     logObj.enable(UPNP_ALL);
#else
    // Constructor
    UpnpapiFTestSuite() {
#endif
        // initialize needed global variables.
        memset(&gIF_NAME, 0, sizeof(gIF_NAME));
    gIF_INDEX = 0;
    memset(&gIF_IPV6, 0, sizeof(gIF_IPV6));
    gIF_IPV6_PREFIX_LENGTH = 0;
    LOCAL_PORT_V6 = 0;
    memset(&gIF_IPV6_ULA_GUA, 0, sizeof(gIF_IPV6_ULA_GUA));
    gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
    LOCAL_PORT_V6_ULA_GUA = 0;
    memset(&gIF_IPV4, 0, sizeof(gIF_IPV4));
    memset(&gIF_IPV4_NETMASK, 0, sizeof(gIF_IPV4_NETMASK));
    LOCAL_PORT_V4 = 0;

    // Destroy global variables to detect side effects.
    UpnpSdkInit = 0xAA;
    memset(&errno, 0xAA, sizeof(errno));
    memset(&GlobalHndRWLock, 0xAA, sizeof(GlobalHndRWLock));
    // memset(&gWebMutex, 0xAA, sizeof(gWebMutex));
    memset(&gUpnpSdkNLSuuid, 0, sizeof(gUpnpSdkNLSuuid));
    memset(&HandleTable, 0xAA, sizeof(HandleTable));
    memset(&gSendThreadPool, 0xAA, sizeof(gSendThreadPool));
    memset(&gRecvThreadPool, 0xAA, sizeof(gRecvThreadPool));
    memset(&gMiniServerThreadPool, 0xAA, sizeof(gMiniServerThreadPool));
    memset(&gTimerThread, 0xAA, sizeof(gTimerThread));
    memset(&bWebServerState, 0xAA, sizeof(bWebServerState));
    memset(&sdkInit_mutex, 0xAA, sizeof(sdkInit_mutex));
}
}; // namespace utest

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
    // ASSERT_EQ(pthread_mutex_trylock(&GlobalHndRWLock), 0); // must be rwlock
    // EXPECT_EQ(pthread_mutex_unlock(&GlobalHndRWLock), 0);  // must be rwlock

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
    Handle_Info dummy;
    Handle_Info* hinfo_p{&dummy};

    // Initialize the handle table.
    for (int i = 0; i < NUM_HANDLE; ++i) {
        HandleTable[i] = nullptr;
    }
    Handle_Info hinfo0{};
    hinfo0.HType = HND_INVALID;
    HandleTable[0] = &hinfo0;
    // HandleTable[1] is nullptr from initialization before;
    Handle_Info hinfo2{};
    hinfo2.HType = HND_CLIENT;
    HandleTable[2] = &hinfo2;
    Handle_Info hinfo3{};
    hinfo3.HType = HND_DEVICE;
    HandleTable[3] = &hinfo3;
    Handle_Info hinfo4{};
    hinfo4.HType = HND_CLIENT;
    HandleTable[4] = &hinfo4;

    // Test Unit
    EXPECT_EQ(GetHandleInfo(0, &hinfo_p), HND_INVALID);
    // Out of range, nothing returned.
    EXPECT_EQ(hinfo_p, &dummy);
    EXPECT_EQ(GetHandleInfo(NUM_HANDLE, &hinfo_p), HND_INVALID);
    EXPECT_EQ(hinfo_p, &dummy);
    EXPECT_EQ(GetHandleInfo(NUM_HANDLE + 1, &hinfo_p), HND_INVALID);
    EXPECT_EQ(hinfo_p, &dummy);

    EXPECT_EQ(GetHandleInfo(1, &hinfo_p), HND_INVALID);
    // Nothing returned.
    EXPECT_EQ(hinfo_p, &dummy);

    EXPECT_EQ(GetHandleInfo(3, &hinfo_p), HND_DEVICE);
    // Pointer to handle info is returned.
    EXPECT_EQ(hinfo_p, &hinfo3);

    EXPECT_EQ(GetHandleInfo(4, &hinfo_p), HND_CLIENT);
    // Pointer to handle info is returned.
    EXPECT_EQ(hinfo_p, &hinfo4);
}

TEST(UpnpapiDeathTest, GetHandleInfo_with_nullptr_to_handle_table) {
    // Initialize HandleTable bcause it only contains pointer.
    Handle_Info dummy{};
    HandleTable[1] = &dummy;

    // Test Unit with nullptr to result variable
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    std::cout << CYEL "[ BUGFIX   ]" CRES
              << " nullptr to handle table must not segfault.\n";
#if !defined(__APPLE__) || defined(DEBUG)
    // This expects segfault.
    EXPECT_DEATH(GetHandleInfo(1, nullptr), ".*");
#endif
#else
    EXPECT_EQ(GetHandleInfo(1, nullptr), HND_TABLE_INVALID);
#endif
    // This will be filled with a pointer to the requested client info.
    Handle_Info* hinfo_p{nullptr};

    // Test Unit
    // This returns the value from dummy, that is 0 initialized and 0 is handle
    // type HND_CLIENT.
    EXPECT_EQ(GetHandleInfo(1, &hinfo_p), HND_CLIENT);
}

TEST_F(UpnpapiFTestSuite, UpnpFinish_successful) {
    // Doing needed initializations. Otherwise we get segfaults with
    // UpnpFinish() due to uninitialized pointers.
    // Initialize SDK global mutexes.
    ASSERT_EQ(UpnpInitMutexes(), UPNP_E_SUCCESS);

    // Initialize the handle list.
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    HandleLock(__FILE__, __LINE__);
#else
    HandleLock();
#endif
    for (int i = 0; i < NUM_HANDLE; ++i)
        HandleTable[i] = nullptr;
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    HandleUnlock(__FILE__, __LINE__);
#else
    HandleUnlock();
#endif

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

    UpnpDevice_Handle device_handle = -1;
    // Initialize the handle list.
    // for (int i = 0; i < NUM_HANDLE; ++i) {
    //     HandleTable[i] = nullptr;
    // }

    int ret_UpnpInit2 = ::UpnpInit2(llaObj.name.c_str(), 0);
    ASSERT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

    const std::string desc_doc_url{"http://[" + std::string(gIF_IPV6) +
                                   "]:" + std::to_string(LOCAL_PORT_V6) +
                                   "/tvdevicedesc.xml"};
    UpnpSdkInit = 1;

    // Test Unit
    int ret_UpnpRegisterRootDevice3 =
        UpnpRegisterRootDevice3(desc_doc_url.c_str(), CallbackEventHandler,
                                &device_handle, &device_handle, AF_INET6);

    EXPECT_EQ(ret_UpnpRegisterRootDevice3, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpRegisterRootDevice3, UPNP_E_SUCCESS);

    UpnpUnRegisterRootDevice(device_handle);
    UpnpFinish();
}

TEST_F(UpnpapiMockFTestSuite, UpnpRegisterRootDevice3_successful) {
    if (github_actions)
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

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_monitor_if_valid_ip_addresses_set) {
    // Only one network interface is supported for compatibility but will be
    // extended with re-engineering. Ports not set with this Unit so they
    // doesn't matter here.

    // Test Unit
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo(nullptr);
#else
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo();
#endif
    ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

    if (gIF_IPV6[0] != '\0') {
        std::string ipv6{std::string(gIF_IPV6) + "%" +
                         std::to_string(gIF_INDEX)};
        EXPECT_TRUE(nadaptObj.find_first(ipv6));
#ifndef _MSC_VER // todo: detecting gIF_IPV6_PREFIX_LENGTH isn't coded for win32
        EXPECT_GT(gIF_IPV6_PREFIX_LENGTH, 0);
#endif
        if (gIF_IPV6_ULA_GUA[0] != '\0') {
            EXPECT_TRUE(nadaptObj.find_first(gIF_IPV6_ULA_GUA));
            EXPECT_EQ(gIF_INDEX, nadaptObj.index());
        } else
            EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
        if (gIF_IPV4[0] != '\0')
            EXPECT_GT(gIF_INDEX, 0);
        else
            EXPECT_STREQ(gIF_IPV4_NETMASK, "");

    } else if (gIF_IPV6_ULA_GUA[0] != '\0') {
        EXPECT_TRUE(nadaptObj.find_first(gIF_IPV6_ULA_GUA));
        EXPECT_EQ(gIF_INDEX, nadaptObj.index());
        EXPECT_GT(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
        if (gIF_IPV6[0] != '\0') {
            std::string ipv6{std::string(gIF_IPV6) + "%" +
                             std::to_string(gIF_INDEX)};
            EXPECT_TRUE(nadaptObj.find_first(ipv6));
        } else
            EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
        if (gIF_IPV4[0] != '\0') {
            EXPECT_TRUE(nadaptObj.find_first(gIF_IPV4));
            EXPECT_EQ(gIF_INDEX, nadaptObj.index());
        } else
            EXPECT_STREQ(gIF_IPV4_NETMASK, "");

    } else if (gIF_IPV4[0] != '\0') {
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
        EXPECT_GT(gIF_INDEX, 0);
#else
        EXPECT_TRUE(nadaptObj.find_first(gIF_IPV4));
        EXPECT_EQ(gIF_INDEX, nadaptObj.index());
#endif
        EXPECT_THAT(gIF_IPV4_NETMASK, StartsWith("255."));
        if (gIF_IPV6[0] != '\0') {
            std::string ipv6{std::string(gIF_IPV6) + "%" +
                             std::to_string(gIF_INDEX)};
            EXPECT_TRUE(nadaptObj.find_first(ipv6));
        } else
            EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
        if (gIF_IPV6_ULA_GUA[0] != '\0') {
            EXPECT_TRUE(nadaptObj.find_first(gIF_IPV6_ULA_GUA));
            EXPECT_EQ(gIF_INDEX, nadaptObj.index());
        } else
            EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);

    } else
        GTEST_FAIL() << "No network interface found.";
}

// Subroutine for multiple check of empty global addresses.
void chk_empty_gifaddr() {
    if (old_code)
        if (gIF_NAME[0] != '\0')
            std::cout
                << CYEL "[ BUGFIX   ] " CRES << __LINE__
                << ": An invalid netadapter name must not modify gIF_NAME to \""
                << gIF_NAME << "\".\n";
        else
            EXPECT_STREQ(gIF_NAME, "");
    EXPECT_EQ(gIF_INDEX, 0);
    // The loopback address belongs to link-local unicast addresses.
    EXPECT_STREQ(gIF_IPV6, "");
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
    EXPECT_STREQ(gIF_IPV4, "");
    EXPECT_STREQ(gIF_IPV4_NETMASK, "");
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_with_loopback_ipv6_iface_fails) {
    // Ports not set with this Unit so they doesn't matter here.
    // Loopback interfaces are not supported by UpnpGetIfInfo().
    if (!nadaptObj.find_first("[::1]"))
        GTEST_SKIP() << "No local network adapter with usable loopback ip "
                        "address found.";

    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo("[::1]");
    EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);
    {
        SCOPED_TRACE("");
        chk_empty_gifaddr();
    }
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_with_loopback_ipv4_iface_fails) {
    // Ports not set with this Unit so they doesn't matter here.
    // The real used loopback address can be "127.0.0.1" to "127.255.255.254".
    // But loopback interfaces are not supported by UpnpGetIfInfo().
    if (!nadaptObj.find_first("[::ffff:127.0.0.1]"))
        GTEST_SKIP() << "No local network adapter with usable loopback ip "
                        "address found.";

    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo("[::ffff:127.0.0.1]");
    EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);
    {
        SCOPED_TRACE("");
        chk_empty_gifaddr();
    }
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_with_loopback_iface_fails) {
    // Ports not set with this Unit so they doesn't matter here.
    // Loopback interfaces are not supported by UpnpGetIfInfo().
    if (!nadaptObj.find_first("loopback"))
        GTEST_SKIP() << "No local network adapter with usable loopback ip "
                        "address found.";

    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo("loopback");
    EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);
    {
        SCOPED_TRACE("");
        chk_empty_gifaddr();
    }
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_from_lla_successful) {
    // Ports not set with this Unit so they doesn't matter here.
    if (!nadaptObj.find_first())
        GTEST_SKIP()
            << "No local network adapter with usable IP address found.";

    bool found{false};
    do {
        nadaptObj.sockaddr(saObj);
        if (IN6_IS_ADDR_LINKLOCAL(&saObj.sin6.sin6_addr)) {
            found = true;
            break;
        }
    } while (nadaptObj.find_next());
    if (!found)
        GTEST_SKIP()
            << "No local network adapter with link local address found.";

    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo(saObj.netaddr().c_str());

    if (old_code) {
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": Specifying a link local address \"" << saObj
                  << "\" should be supported.\n";
        EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);
        {
            SCOPED_TRACE("");
            chk_empty_gifaddr();
        }

    } else {

        EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

        EXPECT_STREQ(gIF_NAME, nadaptObj.name().c_str());
        EXPECT_EQ(gIF_INDEX, nadaptObj.index());
        EXPECT_EQ(std::strncmp(gIF_IPV6, "fe80::", 6), 0)
            << "gIF_IPV6=\"" << gIF_IPV6 << '\"';
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 64);
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
        EXPECT_STREQ(gIF_IPV4, "");
        EXPECT_STREQ(gIF_IPV4_NETMASK, "");
    }
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_from_gua_successful) {
    // Ports not set with this Unit so they doesn't matter here.
    if (!nadaptObj.find_first())
        GTEST_SKIP()
            << "No local network adapter with usable IP address found.";

    bool found{false};
    do {
        nadaptObj.sockaddr(saObj);
        if (IN6_IS_ADDR_GLOBAL(&saObj.sin6.sin6_addr)) {
            found = true;
            break;
        }
    } while (nadaptObj.find_next());
    if (!found)
        GTEST_SKIP()
            << "No local network adapter with global unicast address found.";

    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo(saObj.netaddr().c_str());

    if (old_code) {
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": Specifying a global unicast address \"" << saObj
                  << "\" should be supported.\n";
        EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);
        {
            SCOPED_TRACE("");
            chk_empty_gifaddr();
        }

    } else {

        EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

        EXPECT_STREQ(gIF_NAME, nadaptObj.name().c_str());
        EXPECT_EQ(gIF_INDEX, nadaptObj.index());
        EXPECT_STREQ(gIF_IPV6, "");
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
        EXPECT_THAT(gIF_IPV6_ULA_GUA[0], AnyOf('2', '3'));
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 64);
        EXPECT_STREQ(gIF_IPV4, "");
        EXPECT_STREQ(gIF_IPV4_NETMASK, "");
    }
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_from_ip4_successful) {
    // Ports not set with this Unit so they doesn't matter here.
    if (!nadaptObj.find_first())
        GTEST_SKIP()
            << "No local network adapter with usable IP address found.";

    // Search for a local interface with IPv4 address.
    bool found{false};
    do {
        nadaptObj.sockaddr(saObj);
        if (IN6_IS_ADDR_V4MAPPED(&saObj.sin6.sin6_addr)) {
            found = true;
            break;
        }
    } while (nadaptObj.find_next());
    if (!found)
        GTEST_SKIP() << "No local network adapter with IPv4 address found.";

    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo(saObj.netaddr().c_str());

    if (old_code) {
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": Specifying an IPv4 address \"" << saObj
                  << "\" should be supported.\n";
        EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);
        {
            SCOPED_TRACE("");
            chk_empty_gifaddr();
        }

    } else {

        EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

        EXPECT_STREQ(gIF_NAME, nadaptObj.name().c_str());
        EXPECT_EQ(gIF_INDEX, nadaptObj.index());
        EXPECT_STREQ(gIF_IPV6, "");
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
        EXPECT_EQ(std::strncmp(gIF_IPV6_ULA_GUA, "::ffff:", 7), 0)
            << "gIF_IPV6_ULA_GUA=\"" << gIF_IPV6_ULA_GUA << '\"';
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 96);
        EXPECT_STREQ(gIF_IPV4, "");
        EXPECT_STREQ(gIF_IPV4_NETMASK, "");
    }
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_with_ifname_successful) {
    // Ports not set with this Unit so they doesn't matter here.
    // For Microsoft Windows there are some TODOs in the old code:
    // TODO: Retrieve IPv6 ULA or GUA address and its prefix. Only keep IPv6
    // link-local addresses.
    // TODO: Retrieve IPv6 LLA prefix?
    // Tests below are marked with // Wrong!
    if (old_code) {
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": gIF_IPV6_ULA_GUA, and gIF_IPV6_PREFIX_LENGTH should be "
                     "set on MS Windows.\n";
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": Unit should not select IPv4 and IPv6 address, but only "
                     "one.\n";
    }
    // Get the first adapter from internal network adapter list.
    if (!nadaptObj.find_first())
        GTEST_SKIP()
            << "No local network adapter with usable ip address found.";

#if defined(UPnPsdk_WITH_NATIVE_PUPNP) && defined(__APPLE__)
    // On macOS there may be ip address "[fe80::1]" at the adapter "lo0" found,
    // that isn't a loopback address. Old pupnp code interpret this as loopback
    // address and fails with UPNP_E_INVALID_INTERFACE. In this case I use the
    // next local ip address that also isn't a loopback address).
    if (nadaptObj.name().starts_with("lo") && !nadaptObj.find_next())
        GTEST_SKIP()
            << "No local network adapter with usable ip address found.";
#endif

    // Test Unit
    // This should find the first (best) local ip address.
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo(nadaptObj.name().c_str());
    ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

    EXPECT_FALSE(gIF_IPV4[0] == '\0' && gIF_IPV6[0] == '\0' &&
                 gIF_IPV6_ULA_GUA[0] == '\0');
    EXPECT_STREQ(gIF_NAME, nadaptObj.name().c_str());
    ASSERT_EQ(gIF_INDEX, nadaptObj.index());

    // Check all ip addresses on the pre-selected first adapter.
    // Selecting the interface restricts find_next() to select only ip
    // addresses on the interface.
    EXPECT_TRUE(nadaptObj.find_first(gIF_INDEX));
    nadaptObj.sockaddr(saObj);
    char ip6[INET6_ADDRSTRLEN + 32];

    if (gIF_IPV6[0] != '\0') {
        // Strip leading bracket on copying.
        std::strncpy(ip6, saObj.netaddr().c_str() + 1, sizeof(ip6) - 1);
        // Strip trailing scope id.
        if (char* chptr{::strchr(ip6, '%')})
            *chptr = '\0';
        if (!old_code) // On old code we can have two ip addresses selected.
            EXPECT_STREQ(gIF_IPV6, ip6);
#if defined(UPnPsdk_WITH_NATIVE_PUPNP) && defined(_MSC_VER)
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0); // Wrong!
#else
        if (!old_code) // On old code we can have two ip addresses selected.
            EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, nadaptObj.bitmask());
#endif
        EXPECT_EQ(LOCAL_PORT_V6, 0);
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
        if (!old_code) // Could be set on old code.
            EXPECT_STREQ(gIF_IPV4, "");
    } else {
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
        EXPECT_EQ(LOCAL_PORT_V6, 0);
    }

    if (gIF_IPV6_ULA_GUA[0] != '\0') {
        // Strip leading bracket on copying.
        std::strncpy(ip6, saObj.netaddr().c_str() + 1, sizeof(ip6) - 1);
        // Strip trailing bracket.
        if (char* chptr{::strchr(ip6, ']')})
            *chptr = '\0';
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, ip6);
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, nadaptObj.bitmask());
        EXPECT_EQ(LOCAL_PORT_V6_ULA_GUA, 0);
        EXPECT_STREQ(gIF_IPV6, "");
        EXPECT_STREQ(gIF_IPV4, "");
    } else {
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
        EXPECT_EQ(LOCAL_PORT_V6_ULA_GUA, 0);
    }
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    if (gIF_IPV4[0] != '\0') {
        // Getting IPv4 address string from mapped IPv6 for testing may have
        // wrong priority. The old code Unit should only select one ip address.
        // char ip4[INET_ADDRSTRLEN]{'\0'};
        // if (IN6_IS_ADDR_V4MAPPED(&saObj.sin6.sin6_addr)) {
        //     in_addr* saddr{
        //         reinterpret_cast<in_addr*>(&saObj.sin6.sin6_addr.s6_addr[12])};
        //     ASSERT_NE(::inet_ntop(AF_INET, saddr, ip4, sizeof(ip4)),
        //               nullptr);
        // }
        // EXPECT_STREQ(gIF_IPV4, ip4);
        // EXPECT_THAT(gIF_IPV4_NETMASK, StartsWith("255."));
        // EXPECT_NE(LOCAL_PORT_V4, 0);
        // EXPECT_STREQ(gIF_IPV6, "");
        // EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    } else
#endif
    {
        EXPECT_STREQ(gIF_IPV4, "");
        EXPECT_STREQ(gIF_IPV4_NETMASK, "");
        EXPECT_EQ(LOCAL_PORT_V4, 0);
    }
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_default_successful) {
    // Ports not set with this Unit so they doesn't matter here.
    // For Microsoft Windows there are some TODOs in the old code:
    // TODO: Retrieve IPv6 ULA or GUA address and its prefix. Only keep IPv6
    // link-local addresses.
    // TODO: Retrieve IPv6 LLA prefix?
    // Tests below are marked with // Wrong!
    if (old_code)
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": gIF_IPV6_ULA_GUA, and gIF_IPV6_PREFIX_LENGTH should be "
                     "set on MS Windows.\n";

    // Get the first adapter from internal network adapter list.
    if (!nadaptObj.find_first())
        GTEST_SKIP()
            << "No local network adapter with usable ip address found.";

#if defined(UPnPsdk_WITH_NATIVE_PUPNP) && defined(__APPLE__)
    // On macOS there may be ip address "[fe80::1]" at the adapter "lo0" found,
    // that isn't a loopback address. Old pupnp code interpret this as loopback
    // address and fails with UPNP_E_INVALID_INTERFACE. In this case I use the
    // next local ip address that also isn't a loopback address).
    if (nadaptObj.name().starts_with("lo") && !nadaptObj.find_next())
        GTEST_SKIP()
            << "No local network adapter with usable ip address found.";
#endif

    // Test Unit
    // This should find the first (best) local ip address.
#if defined(UPnPsdk_WITH_NATIVE_PUPNP)
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo(nullptr);
#else
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo();
#endif
    ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

    EXPECT_FALSE(gIF_IPV4[0] == '\0' && gIF_IPV6[0] == '\0' &&
                 gIF_IPV6_ULA_GUA[0] == '\0');
    EXPECT_STREQ(gIF_NAME, nadaptObj.name().c_str());
    EXPECT_EQ(gIF_INDEX, nadaptObj.index());
#if defined(UPnPsdk_WITH_NATIVE_PUPNP) && defined(_MSC_VER)
    EXPECT_THAT(gIF_IPV4_NETMASK, StartsWith("255."));
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
    EXPECT_EQ(LOCAL_PORT_V6, 0);
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
#else
    nadaptObj.sockaddr(saObj);
    char ip6[INET6_ADDRSTRLEN + 32];
    if (gIF_IPV6[0] != '\0') {
        // Strip leading bracket on copying.
        std::strncpy(ip6, saObj.netaddr().c_str() + 1, sizeof(ip6) - 1);
        // Strip trailing scope id.
        if (char* chptr{::strchr(ip6, '%')})
            *chptr = '\0';
        if (!old_code) // On old code we can have two ip addresses selected.
            EXPECT_STREQ(gIF_IPV6, ip6);
#if defined(UPnPsdk_WITH_NATIVE_PUPNP) && defined(_MSC_VER)
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0); // Wrong!
#else
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 64);
#endif
        EXPECT_EQ(LOCAL_PORT_V6, 0);
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
        if (!old_code) // On old code we can have two ip addresses selected.
            EXPECT_STREQ(gIF_IPV4, "");
    } else {
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
        EXPECT_EQ(LOCAL_PORT_V6, 0);
    }
    if (gIF_IPV6_ULA_GUA[0] != '\0') {
        // Strip leading bracket on copying.
        std::strncpy(ip6, saObj.netaddr().c_str() + 1, sizeof(ip6) - 1);
        // Strip trailing bracket.
        if (char* chptr{::strchr(ip6, ']')})
            *chptr = '\0';
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, ip6);
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, nadaptObj.bitmask());
        EXPECT_EQ(LOCAL_PORT_V6_ULA_GUA, 0);
        EXPECT_STREQ(gIF_IPV6, "");
        EXPECT_STREQ(gIF_IPV4, "");
    } else {
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
        EXPECT_EQ(LOCAL_PORT_V6_ULA_GUA, 0);
    }
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    if (gIF_IPV4[0] != '\0') {
        // Get IPv4 address string from mapped IPv6 for testing.
        char ip4[INET_ADDRSTRLEN]{'\0'};
        if (IN6_IS_ADDR_V4MAPPED(&saObj.sin6.sin6_addr)) {
            in_addr* saddr{
                reinterpret_cast<in_addr*>(&saObj.sin6.sin6_addr.s6_addr[12])};
            ASSERT_NE(::inet_ntop(AF_INET, saddr, ip4, sizeof(ip4)), nullptr);
        }
        if (!old_code) // On old code IPv4 is selected but not detected with ip4
            EXPECT_STREQ(gIF_IPV4, ip4);
        EXPECT_THAT(gIF_IPV4_NETMASK, StartsWith("255."));
        EXPECT_EQ(LOCAL_PORT_V4, 0);
        if (!old_code) // On old code we can have two ip addresses selected.
            EXPECT_STREQ(gIF_IPV6, "");
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    } else
#endif
    {
        EXPECT_STREQ(gIF_IPV4, "");
        EXPECT_STREQ(gIF_IPV4_NETMASK, "");
        EXPECT_EQ(LOCAL_PORT_V4, 0);
    }
#endif
}

TEST_F(UpnpapiFTestSuite, get_free_handle_successful) {
    if (!github_actions)
        GTEST_FAIL() << "Still needs to be done.";

    [[maybe_unused]] int ret_GetFreeHandle = ::GetFreeHandle();
}

TEST_F(UpnpapiFTestSuite, download_xml_successful) {
#ifdef _MSC_VER
    if (github_actions)
        GTEST_SKIP()
            << "Download XML files must be reworked to be stable on win32.";
#endif

    // The Unit needs a defined state, otherwise it will fail with segfault
    // because internal pupnp media_list_init() isn't executed;
    bWebServerState = WEB_SERVER_DISABLED;
    sdkInit_mutex = PTHREAD_MUTEX_INITIALIZER;

#ifdef __APPLE__
    if (old_code)
        if (llaObj.sa.netaddr() == "[fe80::1%lo0]") {
            // Maybe the netadapter code has found this lla as address on the
            // loopback adapter. This is a special case on macOS. When given
            // the adapter name to the old pupnp init function then it selects
            // the first address on the adapter and that is mostly "[::1]" or
            // "127.0.0.1" (followed by "[fe80::1]"). Pupnp does not support
            // loopback addresses and fails.
            int ret_UpnpInit2 = ::UpnpInit2(llaObj.name.c_str(), 0);
            EXPECT_EQ(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE)
                << errStrEx(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE);
            UpnpFinish();
            GTEST_SKIP() << "Specify a local network loopback interface is not "
                            "supported by pupnp.";
        }
#endif

    int ret_UpnpInit2 = ::UpnpInit2(llaObj.name.c_str(), 0);
    ASSERT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

    // Create an url.
    // Example url maybe "http://[fe80::1]:50001/tvdevicedesc.xml".
    std::string url;
    if (gIF_IPV6[0] != '\0')
        url = "http://[" + std::string(gIF_IPV6) +
              "]:" + std::to_string(LOCAL_PORT_V6) + "/tvdevicedesc.xml";
    else if (gIF_IPV6_ULA_GUA[0] != '\0')
        url = "http://[" + std::string(gIF_IPV6_ULA_GUA) +
              "]:" + std::to_string(LOCAL_PORT_V6_ULA_GUA) +
              "/tvdevicedesc.xml";
    else if (gIF_IPV4[0] != '\0')
        url = "http://[" + std::string(gIF_IPV4) +
              "]:" + std::to_string(LOCAL_PORT_V4) + "/tvdevicedesc.xml";

    IXML_Document* xmldocbuf_ptr{nullptr};
    EXPECT_EQ(UpnpSetWebServerRootDir(SAMPLE_SOURCE_DIR "/web"), 0);

    // Test Unit
    int ret_UpnpDownloadXmlDoc =
        UpnpDownloadXmlDoc(url.c_str(), &xmldocbuf_ptr);
    EXPECT_EQ(ret_UpnpDownloadXmlDoc, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpDownloadXmlDoc, UPNP_E_SUCCESS);

    if (ret_UpnpDownloadXmlDoc == UPNP_E_SUCCESS) {
        EXPECT_STREQ(xmldocbuf_ptr->n.nodeName, "#document");
        EXPECT_EQ(xmldocbuf_ptr->n.nodeValue, nullptr);
        free(xmldocbuf_ptr);
    }
    UpnpFinish();
}

TEST_F(UpnpapiFTestSuite, UpnpInit2_ipv6_loopback_address_fails) {
    if (!nadaptObj.find_first("[::1]"))
        GTEST_SKIP() << "No local network adapter with usable loopback ip "
                        "address found.";

    // The Unit needs a defined state, otherwise it will fail with
    // SEH exception 0xc0000005 on WIN32.
    bWebServerState = WEB_SERVER_DISABLED;
    sdkInit_mutex = PTHREAD_MUTEX_INITIALIZER;

    // Test Unit
    int ret_UpnpInit2 = ::UpnpInit2("::1", 61234);

    EXPECT_EQ(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE)
        << errStrEx(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE);

    UpnpFinish();
}

TEST_F(UpnpapiFTestSuite, UpnpInit2_ipv4_loopback_address_fails) {
    if (!nadaptObj.find_first("[::ffff:127.0.0.1]"))
        GTEST_SKIP() << "No local network adapter with usable loopback ip "
                        "address found.";

    // The Unit needs a defined state, otherwise it will fail with
    // SEH exception 0xc0000005 on WIN32.
    bWebServerState = WEB_SERVER_DISABLED;
    sdkInit_mutex = PTHREAD_MUTEX_INITIALIZER;

    // Test Unit
    int ret_UpnpInit2 = ::UpnpInit2("127.0.0.1", 49234);

    EXPECT_EQ(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE)
        << errStrEx(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE);

    UpnpFinish();
}

TEST_F(UpnpapiFTestSuite, UpnpInit2_loopback_interface_fails) {
    if (!nadaptObj.find_first("loopback"))
        GTEST_SKIP() << "No local network adapter with usable loopback ip "
                        "address found.";

    // The Unit needs a defined state, otherwise it will fail with
    // SEH exception 0xc0000005 on WIN32.
    bWebServerState = WEB_SERVER_DISABLED;
    sdkInit_mutex = PTHREAD_MUTEX_INITIALIZER;

    // Test Unit
    int ret_UpnpInit2 = ::UpnpInit2("loopback", 0);

    EXPECT_EQ(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE)
        << errStrEx(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE);

    UpnpFinish();
}

TEST_F(UpnpapiFTestSuite, UpnpInit2_with_complete_lla) {
    if (!nadaptObj.find_first())
        GTEST_SKIP()
            << "No local network adapter with usable IP address found.";

    bool found{false};
    do {
        nadaptObj.sockaddr(saObj);
        if (IN6_IS_ADDR_LINKLOCAL(&saObj.sin6.sin6_addr)) {
            found = true;
            break;
        }
    } while (nadaptObj.find_next());
    if (!found)
        GTEST_SKIP()
            << "No local network adapter with link local address found.";

    // The Unit needs a defined state, otherwise it will fail with
    // SEH exception 0xc0000005 on WIN32.
    bWebServerState = WEB_SERVER_DISABLED;
    sdkInit_mutex = PTHREAD_MUTEX_INITIALIZER;

    // Test Unit
    int ret_UpnpInit2 = ::UpnpInit2(saObj.netaddr().c_str(), 0);

    if (old_code) {
        EXPECT_EQ(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE);
        UpnpFinish();
        GTEST_SKIP()
            << "Specify an ipv6 link local address is not supported by pupnp.";
    }

    EXPECT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

    EXPECT_STREQ(gIF_NAME, nadaptObj.name().c_str());
    EXPECT_EQ(gIF_INDEX, nadaptObj.index());
    // Create bare link local address.
    char lla[INET6_ADDRSTRLEN + 32];
    // Strip leading bracket on copying.
    std::strncpy(lla, saObj.netaddr().c_str() + 1, sizeof(lla) - 1);
    // Strip trailing scope id if any.
    if (char* chptr{::strchr(lla, '%')})
        *chptr = '\0';
    // Strip trailing bracket.
    if (char* chptr{::strchr(lla, ']')})
        *chptr = '\0';
    EXPECT_STREQ(gIF_IPV6, lla);
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, nadaptObj.bitmask());
    EXPECT_NE(LOCAL_PORT_V6, 0);
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
    EXPECT_EQ(LOCAL_PORT_V6_ULA_GUA, 0);
    EXPECT_STREQ(gIF_IPV4, "");
    EXPECT_STREQ(gIF_IPV4_NETMASK, "");
    EXPECT_EQ(LOCAL_PORT_V4, 0);

    // Test Unit
    ret_UpnpInit2 = ::UpnpInit2(saObj.netaddr().c_str(), 0);

    EXPECT_EQ(ret_UpnpInit2, UPNP_E_INIT)
        << errStrEx(ret_UpnpInit2, UPNP_E_INIT);

    // Nothing has changed.
    EXPECT_STREQ(gIF_NAME, nadaptObj.name().c_str());
    EXPECT_EQ(gIF_INDEX, nadaptObj.index());
    EXPECT_STREQ(gIF_IPV6, lla);
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, nadaptObj.bitmask());
    EXPECT_NE(LOCAL_PORT_V6, 0);
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
    EXPECT_EQ(LOCAL_PORT_V6_ULA_GUA, 0);
    EXPECT_STREQ(gIF_IPV4, "");
    EXPECT_STREQ(gIF_IPV4_NETMASK, "");
    EXPECT_EQ(LOCAL_PORT_V4, 0);

    UpnpFinish();
    if (!github_actions) {
        // Global variables are resetted now.
        SCOPED_TRACE("");
        chk_empty_gifaddr();
    }
}

TEST_F(UpnpapiFTestSuite, UpnpInit2_with_lla_no_brackets_successful) {
    if (!nadaptObj.find_first())
        GTEST_SKIP()
            << "No local network adapter with usable IP address found.";

    bool found{false};
    do {
        nadaptObj.sockaddr(saObj);
        if (IN6_IS_ADDR_LINKLOCAL(&saObj.sin6.sin6_addr)) {
            found = true;
            break;
        }
    } while (nadaptObj.find_next());
    if (!found)
        GTEST_SKIP()
            << "No local network adapter with link local address found.";

    // The Unit needs a defined state, otherwise it will fail with
    // SEH exception 0xc0000005 on WIN32.
    bWebServerState = WEB_SERVER_DISABLED;
    sdkInit_mutex = PTHREAD_MUTEX_INITIALIZER;

    // Create bare link local address.
    char lla[INET6_ADDRSTRLEN + 32];
    // Strip leading bracket on copying.
    std::strncpy(lla, llaObj.sa.netaddr().c_str() + 1, sizeof(lla) - 1);
    // Strip trailing bracket.
    if (char* chptr{::strchr(lla, ']')})
        *chptr = '\0';

    // Test Unit
    int ret_UpnpInit2 = ::UpnpInit2(lla, 0);

    if (old_code) {
        EXPECT_EQ(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE);
        UpnpFinish();
        GTEST_SKIP()
            << "Specify an ipv6 link local address is not supported by pupnp.";
    }

    EXPECT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

    EXPECT_STREQ(gIF_NAME, nadaptObj.name().c_str());
    EXPECT_EQ(gIF_INDEX, nadaptObj.index());
    // Strip trailing scope id if any.
    if (char* chptr{::strchr(lla, '%')})
        *chptr = '\0';
    EXPECT_STREQ(gIF_IPV6, lla);
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, nadaptObj.bitmask());
    EXPECT_NE(LOCAL_PORT_V6, 0);
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
    EXPECT_EQ(LOCAL_PORT_V6_ULA_GUA, 0);
    EXPECT_STREQ(gIF_IPV4, "");
    EXPECT_STREQ(gIF_IPV4_NETMASK, "");
    EXPECT_EQ(LOCAL_PORT_V4, 0);

    UpnpFinish();
}

TEST_F(UpnpapiFTestSuite, UpnpInit2_with_lla_no_scope_id_fails) {
    if (!nadaptObj.find_first())
        GTEST_SKIP()
            << "No local network adapter with usable IP address found.";

    bool found{false};
    do {
        nadaptObj.sockaddr(saObj);
        if (IN6_IS_ADDR_LINKLOCAL(&saObj.sin6.sin6_addr)) {
            found = true;
            break;
        }
    } while (nadaptObj.find_next());
    if (!found)
        GTEST_SKIP()
            << "No local network adapter with link local address found.";

    // The Unit needs a defined state, otherwise it will fail with
    // SEH exception 0xc0000005 on WIN32.
    bWebServerState = WEB_SERVER_DISABLED;
    sdkInit_mutex = PTHREAD_MUTEX_INITIALIZER;

    // Create link local address without scope id.
    char lla[INET6_ADDRSTRLEN + 32];
    std::strncpy(lla, saObj.netaddr().c_str(), sizeof(lla) - 1);
    // Strip trailing scope id if any.
    if (char* chptr{::strchr(lla, '%')}) {
        *chptr = ']';
        chptr++;
        *chptr = '\0';
    }

    // Test Unit
    int ret_UpnpInit2 = ::UpnpInit2(lla, 0);

    EXPECT_EQ(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE)
        << errStrEx(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE);

    { // All interface globals should be untouched.
        SCOPED_TRACE("");
        chk_empty_gifaddr();
    }
    UpnpFinish();
}

TEST_F(UpnpapiFTestSuite, UpnpInit2_with_complete_gua_successful) {
    if (!nadaptObj.find_first())
        GTEST_SKIP()
            << "No local network adapter with usable IP address found.";

    bool found{false};
    do {
        nadaptObj.sockaddr(saObj);
        if (IN6_IS_ADDR_GLOBAL(&saObj.sin6.sin6_addr)) {
            found = true;
            break;
        }
    } while (nadaptObj.find_next());
    if (!found)
        GTEST_SKIP()
            << "No local network adapter with global unicast address found.";

    // The Unit needs a defined state, otherwise it will fail with
    // SEH exception 0xc0000005 on WIN32.
    bWebServerState = WEB_SERVER_DISABLED;
    sdkInit_mutex = PTHREAD_MUTEX_INITIALIZER;

    // Test Unit
    int ret_UpnpInit2 = ::UpnpInit2(saObj.netaddr().c_str(), 0);

    if (old_code) {
        EXPECT_EQ(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE);
        UpnpFinish();
        GTEST_SKIP()
            << "Specify an ipv6 global unicast address is not supported "
               "by pupnp.";
    }

    EXPECT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

    EXPECT_STREQ(gIF_NAME, nadaptObj.name().c_str());
    EXPECT_EQ(gIF_INDEX, nadaptObj.index());
    // Create bare link local address.
    char gua[INET6_ADDRSTRLEN + 32];
    // Strip leading bracket on copying.
    std::strncpy(gua, saObj.netaddr().c_str() + 1, sizeof(gua) - 1);
    // Strip trailing bracket.
    if (char* chptr{::strchr(gua, ']')})
        *chptr = '\0';
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, gua);
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, nadaptObj.bitmask());
    EXPECT_NE(LOCAL_PORT_V6_ULA_GUA, 0);
    EXPECT_EQ(gIF_IPV6[0], '\0');
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
    EXPECT_EQ(LOCAL_PORT_V6, 0);
    EXPECT_EQ(gIF_IPV4[0], '\0');
    EXPECT_EQ(gIF_IPV4_NETMASK[0], '\0');
    EXPECT_EQ(LOCAL_PORT_V4, 0);

    UpnpFinish();
}

TEST_F(UpnpapiFTestSuite, UpnpInit2_with_netadapter_index_successful) {
    if (!nadaptObj.find_first())
        GTEST_SKIP()
            << "No local network adapter with usable IP address found.";

    // The Unit needs a defined state, otherwise it will fail with
    // SEH exception 0xc0000005 on WIN32.
    bWebServerState = WEB_SERVER_DISABLED;
    sdkInit_mutex = PTHREAD_MUTEX_INITIALIZER;

    // Test Unit
    int ret_UpnpInit2 =
        ::UpnpInit2(std::to_string(nadaptObj.index()).c_str(), 0);

    if (old_code) {
        EXPECT_EQ(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE);
        UpnpFinish();
        GTEST_SKIP() << "Specify netadapter index is not supported by pupnp.";
    }

    EXPECT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

    nadaptObj.sockaddr(saObj);
    // Create bare IPv6 address.
    char ip6[INET6_ADDRSTRLEN + 32];
    // Strip leading bracket on copying.
    std::strncpy(ip6, saObj.netaddr().c_str() + 1, sizeof(ip6) - 1);
    // Strip trailing scope id if any.
    if (char* chptr{::strchr(ip6, '%')})
        *chptr = '\0';
    // Strip trailing bracket.
    if (char* chptr{::strchr(ip6, ']')})
        *chptr = '\0';

    EXPECT_STREQ(gIF_NAME, nadaptObj.name().c_str());
    EXPECT_EQ(gIF_INDEX, nadaptObj.index());
    EXPECT_FALSE(gIF_IPV6[0] == '\0' && gIF_IPV6_ULA_GUA[0] == '\0');
    EXPECT_STREQ(gIF_IPV4, "");
    EXPECT_STREQ(gIF_IPV4_NETMASK, "");
    EXPECT_EQ(LOCAL_PORT_V4, 0);
    if (gIF_IPV6[0] != '\0') {
        EXPECT_STREQ(gIF_IPV6, ip6);
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, nadaptObj.bitmask());
        EXPECT_NE(LOCAL_PORT_V6, 0);
    } else {
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
        EXPECT_EQ(LOCAL_PORT_V6, 0);
    }
    if (gIF_IPV6_ULA_GUA[0] != '\0') {
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, ip6);
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, nadaptObj.bitmask());
        EXPECT_NE(LOCAL_PORT_V6_ULA_GUA, 0);
    } else {
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
        EXPECT_EQ(LOCAL_PORT_V6_ULA_GUA, 0);
    }

    UpnpFinish();
}

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
TEST_F(UpnpapiFTestSuite, UpnpInit2_with_netadapter_name_successful) {
    // For Microsoft Windows there are some TODOs in the old code:
    // TODO: Retrieve IPv6 ULA or GUA address and its prefix. Only keep IPv6
    // link-local addresses.
    // TODO: Retrieve IPv6 LLA prefix?

    // Find a usable adapter.
    if (!nadaptObj.find_first())
        GTEST_SKIP()
            << "No local network adapter with usable IP address found.";
#ifdef __APPLE__
    // Only for macOS special check for "lo" due to buggy handling of
    // "[fe80::1]" on loopback adapter.
    if (nadaptObj.name().starts_with("lo") && !nadaptObj.find_next())
        GTEST_SKIP()
            << "No local network adapter with usable IP address found.";
#endif
    // The Unit needs a defined state, otherwise it will fail with
    // SEH exception 0xc0000005 on WIN32.
    bWebServerState = WEB_SERVER_DISABLED;
    sdkInit_mutex = PTHREAD_MUTEX_INITIALIZER;

    // Test Unit
    int ret_UpnpInit2 = ::UpnpInit2(nadaptObj.name().c_str(), 0);
    EXPECT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

    EXPECT_STREQ(gIF_NAME, nadaptObj.name().c_str());
    EXPECT_EQ(gIF_INDEX, nadaptObj.index());
    EXPECT_FALSE(gIF_IPV4[0] == '\0' && gIF_IPV6[0] == '\0' &&
                 gIF_IPV6_ULA_GUA[0] == '\0');

    std::cout << CYEL "[    FIX   ] " CRES << __LINE__
              << ": gIF_IPV6_ULA_GUA, and gIF_IPV6_PREFIX_LENGTH should be "
                 "retrieved on wih32.\n";
#ifdef _MSC_VER
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
    EXPECT_NE(LOCAL_PORT_V6, 0);
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
#else
    nadaptObj.sockaddr(saObj);

    // Get IPv4 address string from mapped IPv6 for testing.
    char ip4[INET_ADDRSTRLEN]{'\0'};
    if (IN6_IS_ADDR_V4MAPPED(&saObj.sin6.sin6_addr)) {
        in_addr* saddr{
            reinterpret_cast<in_addr*>(&saObj.sin6.sin6_addr.s6_addr[12])};
        ASSERT_NE(::inet_ntop(AF_INET, saddr, ip4, sizeof(ip4)), nullptr);
    }

    // Create bare IPv6 address.
    char ip6[INET6_ADDRSTRLEN + 32];
    // Strip leading bracket on copying.
    std::strncpy(ip6, saObj.netaddr().c_str() + 1, sizeof(ip6) - 1);
    // Strip trailing scope id if any.
    if (char* chptr{::strchr(ip6, '%')})
        *chptr = '\0';
    // Strip trailing bracket.
    if (char* chptr{::strchr(ip6, ']')})
        *chptr = '\0';

    if (gIF_IPV6[0] != '\0') {
        if (!old_code) // On old code we can have two ip addresses selected.
            EXPECT_STREQ(gIF_IPV6, ip6);
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 64);
        EXPECT_NE(LOCAL_PORT_V6, 0);
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
        if (!old_code) // On old code we can have two ip addresses selected.
            EXPECT_STREQ(gIF_IPV4, "");
    } else {
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
        EXPECT_EQ(LOCAL_PORT_V6, 0);
    }
    if (gIF_IPV6_ULA_GUA[0] != '\0') {
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, ip6);
        EXPECT_THAT(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, AnyOf(64, 96));
        EXPECT_NE(LOCAL_PORT_V6_ULA_GUA, 0);
        EXPECT_STREQ(gIF_IPV6, "");
        EXPECT_STREQ(gIF_IPV4, "");
    } else {
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
        EXPECT_EQ(LOCAL_PORT_V6_ULA_GUA, 0);
    }
    if (gIF_IPV4[0] != '\0') {
        if (!old_code) // On old code IPv4 is selected but not detected with ip4
            EXPECT_STREQ(gIF_IPV4, ip4);
        nadaptObj.socknetmask(saObj);
        EXPECT_THAT(gIF_IPV4_NETMASK, StartsWith("255."));
        EXPECT_NE(LOCAL_PORT_V4, 0);
        if (!old_code) // On old code we can have two ip addresses selected.
            EXPECT_STREQ(gIF_IPV6, "");
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    } else {
        EXPECT_STREQ(gIF_IPV4_NETMASK, "");
        EXPECT_EQ(LOCAL_PORT_V4, 0);
    }
#endif
    UpnpFinish();
}

#else

TEST_F(UpnpapiFTestSuite, UpnpInit2_with_netadapter_name_successful) {
    // Find a usable adapter.
    if (!nadaptObj.find_first())
        GTEST_SKIP()
            << "No local network adapter with usable IP address found.";

    // The Unit needs a defined state, otherwise it will fail with
    // SEH exception 0xc0000005 on WIN32.
    bWebServerState = WEB_SERVER_DISABLED;
    sdkInit_mutex = PTHREAD_MUTEX_INITIALIZER;

    // Test Unit
    int ret_UpnpInit2 = ::UpnpInit2(nadaptObj.name().c_str(), 0);
    EXPECT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

    EXPECT_STREQ(gIF_NAME, nadaptObj.name().c_str());
    EXPECT_EQ(gIF_INDEX, nadaptObj.index());
    EXPECT_STREQ(gIF_IPV4, "");
    EXPECT_STREQ(gIF_IPV4_NETMASK, "");
    EXPECT_EQ(LOCAL_PORT_V4, 0);
    EXPECT_FALSE(gIF_IPV6[0] == '\0' && gIF_IPV6_ULA_GUA[0] == '\0');

    nadaptObj.sockaddr(saObj);
    // Create bare IPv6 address.
    char ip6[INET6_ADDRSTRLEN + 32];
    // Strip leading bracket on copying.
    std::strncpy(ip6, saObj.netaddr().c_str() + 1, sizeof(ip6) - 1);
    // Strip trailing scope id if any.
    if (char* chptr{::strchr(ip6, '%')})
        *chptr = '\0';
    // Strip trailing bracket.
    if (char* chptr{::strchr(ip6, ']')})
        *chptr = '\0';

    if (gIF_IPV6[0] != '\0') {
        EXPECT_STREQ(gIF_IPV6, ip6);
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, nadaptObj.bitmask());
        EXPECT_NE(LOCAL_PORT_V6, 0);
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    } else {
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
        EXPECT_EQ(LOCAL_PORT_V6, 0);
    }
    if (gIF_IPV6_ULA_GUA[0] != '\0') {
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, ip6);
        EXPECT_THAT(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, AnyOf(64, 96));
        EXPECT_NE(LOCAL_PORT_V6_ULA_GUA, 0);
        EXPECT_STREQ(gIF_IPV6, "");
    } else {
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
        EXPECT_EQ(LOCAL_PORT_V6_ULA_GUA, 0);
    }

    UpnpFinish();
}
#endif

TEST_F(UpnpapiFTestSuite, UpnpInit2_default_successful) {
    // For Microsoft Windows there are some TODOs in the old code:
    // TODO: Retrieve IPv6 ULA or GUA address and its prefix. Only keep IPv6
    // link-local addresses.
    // TODO: Retrieve IPv6 LLA prefix?

    if (old_code)
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": Unit should not select IPv4 and IPv6 address, but only "
                     "one.\n";

    // The Unit needs a defined state, otherwise it will fail with
    // SEH exception 0xc0000005 on WIN32.
    bWebServerState = WEB_SERVER_DISABLED;
    sdkInit_mutex = PTHREAD_MUTEX_INITIALIZER;

    // Test Unit
    int ret_UpnpInit2 = ::UpnpInit2(nullptr, 0);
    EXPECT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

    // clang-format off
    // std::cerr << "DEBUG: gIF_IPV4=\"" << ::gIF_IPV4
    //           << "\", LOCAL_PORT_V4=" << ::LOCAL_PORT_V4
    //           << ", gIF_IPV6=\"" << ::gIF_IPV6
    //           << "\", LOCAL_PORT_V6=" << LOCAL_PORT_V6
    //           << ", gIF_IPV6_ULA_GUA=\"" << ::gIF_IPV6_ULA_GUA
    //           << "\", LOCAL_PORT_V6_ULA_GUA=" << LOCAL_PORT_V6_ULA_GUA
    //           << ".\n";
    // clang-format on

    EXPECT_STRNE(gIF_NAME, "");
    EXPECT_NE(gIF_INDEX, 0);
    EXPECT_FALSE(gIF_IPV4[0] == '\0' && gIF_IPV6[0] == '\0' &&
                 gIF_IPV6_ULA_GUA[0] == '\0');
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    std::cout << CYEL "[    FIX   ] " CRES << __LINE__
              << ": gIF_IPV6_ULA_GUA, and gIF_IPV6_PREFIX_LENGTH should be "
                 "retrieved on wih32.\n";
#endif
#if defined(UPnPsdk_WITH_NATIVE_PUPNP) && defined(_MSC_VER)
    EXPECT_THAT(gIF_IPV4_NETMASK, StartsWith("255."));
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
    EXPECT_NE(LOCAL_PORT_V6, 0);
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
#else
    if (gIF_IPV6[0] != '\0') {
        EXPECT_NE(gIF_IPV6_PREFIX_LENGTH, 0);
        EXPECT_NE(LOCAL_PORT_V6, 0);
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
        if (!old_code) // On old code we can have two ip addresses selected.
            EXPECT_STREQ(gIF_IPV4, "");
    } else {
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
        EXPECT_EQ(LOCAL_PORT_V6, 0);
    }
    if (gIF_IPV6_ULA_GUA[0] != '\0') {
        EXPECT_NE(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
        EXPECT_NE(LOCAL_PORT_V6_ULA_GUA, 0);
        EXPECT_STREQ(gIF_IPV6, "");
        EXPECT_STREQ(gIF_IPV4, "");
    } else {
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
        EXPECT_EQ(LOCAL_PORT_V6_ULA_GUA, 0);
    }
    if (gIF_IPV4[0] != '\0') {
        EXPECT_THAT(gIF_IPV4_NETMASK, StartsWith("255."));
        EXPECT_NE(LOCAL_PORT_V4, 0);
        if (!old_code) // On old code we can have two ip addresses selected.
            EXPECT_STREQ(gIF_IPV6, "");
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    } else {
        EXPECT_STREQ(gIF_IPV4_NETMASK, "");
        EXPECT_EQ(LOCAL_PORT_V4, 0);
    }
#endif
    UpnpFinish();
}

#if 0
// This is difficult to implement on the old structures with Lla, UlaGua and
// IPV4. Listening on all local ip addresses has no distinction on these
// address types. How to handle this? Maybe I find a solution later.
TEST_F(UpnpapiFTestSuite, UpnpInit2_listen_on_all_local_addr) {
    // The Unit needs a defined state, otherwise it will fail with
    // SEH exception 0xc0000005 on WIN32.
    bWebServerState = WEB_SERVER_DISABLED;
    sdkInit_mutex = PTHREAD_MUTEX_INITIALIZER;

    // Test Unit
    int ret_UpnpInit2 = ::UpnpInit2("", 0);

    if (old_code) {
        EXPECT_EQ(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE);
        UpnpFinish();
        GTEST_SKIP() << "Using unspecified local ip address for listening on "
                        "all addresses is not supported by pupnp.";
    }

    EXPECT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

    UpnpFinish();
}
#endif

} // namespace utest

int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
    utest::get_netadapter();
    utest::nadaptObj.get_first();
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
