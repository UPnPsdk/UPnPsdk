// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-09-18

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
#include <utest/upnpdebug.hpp>
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
using ::UPnPsdk::g_dbug;
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


// upnpapi TestSuites
// ==================
class UpnpapiFTestSuite : public ::testing::Test {
  private:
    bool m_dbug_flag{UPnPsdk::g_dbug};

  protected:
    // Constructor
    UpnpapiFTestSuite() {
        // Destroy global variables to detect side effects.
        memset(&gIF_NAME, 0xAA, sizeof(gIF_NAME));
        gIF_INDEX = ~0u;
        memset(&gIF_IPV6, 0xAA, sizeof(gIF_IPV6));
        gIF_IPV6_PREFIX_LENGTH = ~0u;
        LOCAL_PORT_V6 = static_cast<in_port_t>(~0);
        memset(&gIF_IPV6_ULA_GUA, 0xAA, sizeof(gIF_IPV6_ULA_GUA));
        gIF_IPV6_ULA_GUA_PREFIX_LENGTH = ~0u;
        LOCAL_PORT_V6_ULA_GUA = static_cast<in_port_t>(~0);
        memset(&gIF_IPV4, 0xAA, sizeof(gIF_IPV4));
        memset(&gIF_IPV4_NETMASK, 0xAA, sizeof(gIF_IPV4_NETMASK));
        LOCAL_PORT_V4 = static_cast<in_port_t>(~0);
        UpnpSdkInit = 0x55555555;
        memset(&errno, 0xAA, sizeof(errno));
        memset(&GlobalHndRWLock, 0xAA, sizeof(GlobalHndRWLock));
        memset(&gUpnpSdkNLSuuid, 0xAA, sizeof(gUpnpSdkNLSuuid));
        memset(&HandleTable, 0xAA, sizeof(HandleTable));
        memset(&gSendThreadPool, 0xAA, sizeof(gSendThreadPool));
        memset(&gRecvThreadPool, 0xAA, sizeof(gRecvThreadPool));
        memset(&gMiniServerThreadPool, 0xAA, sizeof(gMiniServerThreadPool));
        memset(&gTimerThread, 0xAA, sizeof(gTimerThread));
        memset(&bWebServerState, 0xAA, sizeof(bWebServerState));
        memset(&sdkInit_mutex, 0xAA, sizeof(sdkInit_mutex));
        // memset(&GlobalClientSubscribeMutex, 0xAA,
        //        sizeof(GlobalClientSubscribeMutex));
    }

    // Destructor
    ~UpnpapiFTestSuite() { UPnPsdk::g_dbug = m_dbug_flag; }
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
    EXPECT_EQ(UpnpSdkInit, 0x55555555);

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


TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_monitor_if_valid_ip_addresses_set) {
    // Only one network interface is supported for compatibility but will be
    // extended with re-engineering. Ports not set with this Unit so they
    // doesn't matter here.

    // Initialize needed global variables.
    if (old_code) {
        gIF_IPV6[0] = '\0';
        gIF_IPV6_ULA_GUA[0] = '\0';
    }
    gIF_IPV6_PREFIX_LENGTH = 0;
    gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;

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

    // Initialize needed global variables.
    gIF_INDEX = 0;
    gIF_IPV6[0] = '\0';
    gIF_IPV6_PREFIX_LENGTH = 0;
    gIF_IPV6_ULA_GUA[0] = '\0';
    gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
    gIF_IPV4[0] = '\0';
    gIF_IPV4_NETMASK[0] = '\0';

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

    // Initialize needed global variables.
    gIF_INDEX = 0;
    gIF_IPV6[0] = '\0';
    gIF_IPV6_PREFIX_LENGTH = 0;
    gIF_IPV6_ULA_GUA[0] = '\0';
    gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
    gIF_IPV4[0] = '\0';
    gIF_IPV4_NETMASK[0] = '\0';

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

    // Initialize needed global variables.
    gIF_INDEX = 0;
    gIF_IPV6[0] = '\0';
    gIF_IPV6_PREFIX_LENGTH = 0;
    gIF_IPV6_ULA_GUA[0] = '\0';
    gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
    gIF_IPV4[0] = '\0';
    gIF_IPV4_NETMASK[0] = '\0';

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

    // Initialize needed global variables.
    if (old_code) {
        gIF_INDEX = 0;
        gIF_IPV6[0] = '\0';
        gIF_IPV6_PREFIX_LENGTH = 0;
        gIF_IPV6_ULA_GUA[0] = '\0';
        gIF_IPV4[0] = '\0';
        gIF_IPV4_NETMASK[0] = '\0';
    }
    gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;

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

    // Initialize needed global variables.
    if (old_code) {
        gIF_INDEX = 0;
        gIF_IPV6[0] = '\0';
        gIF_IPV6_ULA_GUA[0] = '\0';
        gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
        gIF_IPV4[0] = '\0';
        gIF_IPV4_NETMASK[0] = '\0';
    }
    gIF_IPV6_PREFIX_LENGTH = 0;

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

    // Initialize needed global variables.
    if (old_code) {
        gIF_INDEX = 0;
        gIF_IPV6[0] = '\0';
        gIF_IPV6_ULA_GUA[0] = '\0';
        gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
        gIF_IPV4[0] = '\0';
        gIF_IPV4_NETMASK[0] = '\0';
    }
    gIF_IPV6_PREFIX_LENGTH = 0;

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

    // Initialize needed global variables.
    if (old_code) {
        gIF_IPV6[0] = '\0';
        gIF_IPV6_ULA_GUA[0] = '\0';
        gIF_IPV4[0] = '\0';
    } else {
        LOCAL_PORT_V4 = 0;
    }
    gIF_IPV6_PREFIX_LENGTH = 0;
    LOCAL_PORT_V6 = 0;
    LOCAL_PORT_V6_ULA_GUA = 0;
    gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;

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

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_with_ifname_having_only_ipv6) {
    // Select netadapter that has only IPv6 addresses and test UpnpGetIfInfo
    // with its interface name.

    // First collect the netadapter index of all known ip addresses. Condense
    // it to the only needed information, that is: index number and if it is an
    // index number of an IPv4 address. The latter is marked negative.
    std::vector<long int> indexes;
    nadaptObj.find_first();
    do {
        nadaptObj.sockaddr(saObj);
        if (IN6_IS_ADDR_V4MAPPED(&saObj.sin6.sin6_addr))
            // Push negative index. Cast unsigned int to long int is legal.
            indexes.push_back(static_cast<long int>(nadaptObj.index()) * -1);
        else
            // Push positive index.
            indexes.push_back(nadaptObj.index());
    } while (nadaptObj.find_next());

    // Now on the complete list I look for negative index numbers (that belong
    // to IPv4 addresses). Always if found I "delete" (set to 0) all the same
    // index numbers, positive and negative.
    for (long int idx : indexes) {
        if (idx < 0) { // Look if belonging to an IPv4 address, and if so,
                       // "delete" (set to 0) all the same index numbers.
            idx = std::abs(idx);
            for (size_t i{0}; i < indexes.size(); i++) {
                if (std::abs(indexes[i]) == idx)
                    indexes[i] = 0;
            }
        }
    }

    // Here the list can only contain positve indexes if any. For these indexes
    // was not found that they also belong to an IPv4 address. Here I use only
    // the first one, but no problem if we need all. The list is available.
    unsigned int index{0};
    for (size_t i{0}; i < indexes.size(); i++) {
        if (indexes[i] > 0) {
            // Cast is no problem. Here we have only positive unsigned int.
            index = static_cast<unsigned int>(indexes[i]);
            break;
        }
    }
    std::cout << "DEBUG! index=" << index << '\n';

    if (index == 0)
        GTEST_SKIP()
            << "No local network adapter with usable ip address found.";

    // Initialize needed global variables.
    gIF_INDEX = 0;
    gIF_IPV6[0] = '\0';
    gIF_IPV6_PREFIX_LENGTH = 0;
    gIF_IPV6_ULA_GUA[0] = '\0';
    gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
    gIF_IPV4[0] = '\0';
    gIF_IPV4_NETMASK[0] = '\0';

    // Select the local netadapter that has only IPv6 addresses.
    nadaptObj.find_first(index);

#if defined(UPnPsdk_WITH_NATIVE_PUPNP) && defined(__APPLE__)
    // On macOS there may be ip address "[fe80::1]" at the adapter "lo0" found,
    // that isn't a loopback address. Old pupnp code interpret this as loopback
    // address and fails with UPNP_E_INVALID_INTERFACE. In this case I use the
    // next local ip address that also isn't a loopback address).
    if (nadaptObj.name().starts_with("lo")) {
        // Test Unit
        int ret_UpnpGetIfInfo = ::UpnpGetIfInfo(nadaptObj.name().c_str());
        EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);
        nadaptObj.sockaddr(saObj);
        GTEST_SKIP() << "Unusable ip address=\"" << saObj.netaddrp()
                     << "\" on loopback adapter found.";
    }
#endif
    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo(nadaptObj.name().c_str());
    EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

    EXPECT_STREQ(gIF_NAME, nadaptObj.name().c_str());
    EXPECT_EQ(gIF_INDEX, index);
    nadaptObj.sockaddr(saObj);
    if (gIF_IPV6[0] != '\0') {
        std::string if_addr{"[" + std::string(gIF_IPV6) + "%" +
                            std::to_string(gIF_INDEX) + "]"};
        EXPECT_TRUE(nadaptObj.find_first(if_addr))
            << "Cannot find gIF_IPV6=\"" << if_addr << "\"";
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 64);
    }
    if (gIF_IPV6_ULA_GUA[0] != '\0') {
        std::string if_addr{"[" + std::string(gIF_IPV6_ULA_GUA) + "]"};
        EXPECT_TRUE(nadaptObj.find_first(if_addr))
            << "Cannot find gIF_IPV6_ULA_GUA=\"" << if_addr << "\"";
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 64);
    }
    EXPECT_STREQ(gIF_IPV4, "");
    EXPECT_STREQ(gIF_IPV4_NETMASK, "");
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_default_successful) {
    // Ports not set with this Unit so they doesn't matter here.
    // For Microsoft Windows there are some TODOs in the old code:
    // TODO: Retrieve IPv6 ULA or GUA address and its prefix. Only keep IPv6
    // link-local addresses.
    // TODO: Retrieve IPv6 LLA prefix?
    // Tests below are marked with // Wrong!

    // Initialize needed global variables.
    if (old_code) {
        gIF_IPV6[0] = '\0';
        gIF_IPV6_ULA_GUA[0] = '\0';
        gIF_IPV4[0] = '\0';
    }
    gIF_IPV6_PREFIX_LENGTH = 0;
    LOCAL_PORT_V6 = 0;
    LOCAL_PORT_V6_ULA_GUA = 0;
    gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
    LOCAL_PORT_V4 = 0;

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
    if (nadaptObj.name().starts_with("lo") && !nadaptObj.find_next()) {
        GTEST_SKIP()
            << "No local network adapter with usable ip address found.";
    }
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
    GTEST_SKIP() << "Has to be done.";

    [[maybe_unused]] int ret_GetFreeHandle = ::GetFreeHandle();
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
    // Initialize needed global variables.
    if (!old_code)
        gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;

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

    // Test Unit second time without UpnpFinish()
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
}

TEST_F(UpnpapiFTestSuite, UpnpInit2_with_lla_no_brackets_successful) {
    // Initialize needed global variables.
    if (!old_code)
        gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;

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
    std::strncpy(lla, saObj.netaddr().c_str() + 1, sizeof(lla) - 1);
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
    // Initialize needed global variables.
    gIF_INDEX = 0;
    gIF_IPV6[0] = '\0';
    gIF_IPV6_PREFIX_LENGTH = 0;
    gIF_IPV6_ULA_GUA[0] = '\0';
    gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
    gIF_IPV4[0] = '\0';
    gIF_IPV4_NETMASK[0] = '\0';

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
    // Initialize needed global variables.
    if (!old_code)
        gIF_IPV6_PREFIX_LENGTH = 0;

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
    // Initialize needed global variables.
    if (!old_code) {
        gIF_IPV6_PREFIX_LENGTH = 0;
        gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
    }

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

    // Initialize needed global variables.
    gIF_IPV6[0] = '\0';
    gIF_IPV6_PREFIX_LENGTH = 0;
    gIF_IPV6_ULA_GUA[0] = '\0';
    gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;

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

#else  // UPnPsdk_WITH_NATIVE_PUPNP

TEST_F(UpnpapiFTestSuite, UpnpInit2_with_netadapter_name_successful) {
    // Initialize needed global variables.
    gIF_IPV6_PREFIX_LENGTH = 0;
    if (!old_code)
        gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;

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
#endif // UPnPsdk_WITH_NATIVE_PUPNP

TEST_F(UpnpapiFTestSuite, UpnpInit2_default_successful) {
    // For Microsoft Windows there are some TODOs in the old code:
    // TODO: Retrieve IPv6 ULA or GUA address and its prefix. Only keep IPv6
    // link-local addresses.
    // TODO: Retrieve IPv6 LLA prefix?

    // Initialize needed global variables.
    if (old_code) {
        gIF_IPV6[0] = '\0';
        gIF_IPV6_ULA_GUA[0] = '\0';
    }
    gIF_IPV6_PREFIX_LENGTH = 0;
    gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;

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
// Disabled due to bug with IPv6 parse_hostport(). Will be enabled as soon as
// the bug is fixed.
TEST_F(UpnpapiFTestSuite, download_xml_successful) {
    if (github_actions) // Always disable extended debug messages.
        g_dbug = false; // Will be restored by the tests destructor.

    CPupnplog logObj;   // Output only with build type DEBUG.
    if (g_dbug)
        logObj.enable(UPNP_INFO);

    // The Unit needs a defined state, otherwise it may fail with
    // SEH exception 0xc0000005 on WIN32.
    bWebServerState = WEB_SERVER_DISABLED;
    sdkInit_mutex = PTHREAD_MUTEX_INITIALIZER;
    gIF_IPV6[0] = '\0';
    gIF_IPV6_PREFIX_LENGTH = 0;
    gIF_IPV6_ULA_GUA[0] = '\0';
    gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
    gIF_IPV4[0] = '\0';
    gIF_IPV4_NETMASK[0] = '\0';

    int ret_UpnpInit2 = ::UpnpInit2(nullptr, 0);
    ASSERT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

    // clang-format off
    std::cerr << "DEBUG! gIF_IPV4=\"" << ::gIF_IPV4
              << "\", LOCAL_PORT_V4=" << ::LOCAL_PORT_V4
              << ", gIF_IPV6=\"" << ::gIF_IPV6
              << "\", LOCAL_PORT_V6=" << LOCAL_PORT_V6
              << ", gIF_IPV6_ULA_GUA=\"" << ::gIF_IPV6_ULA_GUA
              << "\", LOCAL_PORT_V6_ULA_GUA=" << LOCAL_PORT_V6_ULA_GUA
              << ".\n";
    // clang-format on

    EXPECT_EQ(::UpnpSetWebServerRootDir(SAMPLE_SOURCE_DIR "/web"), 0);

    // Create an url.
    // Example url maybe "http://[fe80::1]:50001/tvdevicedesc.xml".
    std::string url;
    if (gIF_IPV6_ULA_GUA[0] != '\0')
        url = "http://[" + std::string(gIF_IPV6_ULA_GUA) +
              "]:" + std::to_string(LOCAL_PORT_V6_ULA_GUA) +
              "/tvdevicedesc.xml";
    else if (gIF_IPV6[0] != '\0')
        // url = "http://[" + std::string(gIF_IPV6) + "%" + std::to_string(gIF_INDEX) + "]:" + std::to_string(LOCAL_PORT_V6) + "/tvdevicedesc.xml";
        url = "http://" + std::string(gIF_IPV6) + "%" + std::to_string(gIF_INDEX) + ":" + std::to_string(LOCAL_PORT_V6) + "/tvdevicedesc.xml";
        // url = "http://" + std::string(gIF_IPV6) + ":" + std::to_string(LOCAL_PORT_V6) + "/tvdevicedesc.xml";
    else if (gIF_IPV4[0] != '\0')
        url = "http://" + std::string(gIF_IPV4) + ":" +
              std::to_string(LOCAL_PORT_V4) + "/tvdevicedesc.xml";

    std::cerr << "DEBUG! url=\"" << url << "\"\n";
    IXML_Document* xmldocbuf_ptr{nullptr};

    // Test Unit
    int ret_UpnpDownloadXmlDoc =
        ::UpnpDownloadXmlDoc(url.c_str(), &xmldocbuf_ptr);
    EXPECT_EQ(ret_UpnpDownloadXmlDoc, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpDownloadXmlDoc, UPNP_E_SUCCESS);

    if (ret_UpnpDownloadXmlDoc == UPNP_E_SUCCESS) {
        EXPECT_STREQ(xmldocbuf_ptr->n.nodeName, "#document");
        EXPECT_EQ(xmldocbuf_ptr->n.nodeValue, nullptr);
        free(xmldocbuf_ptr);
    }
    UpnpFinish();
}
#endif

int CallbackEventHandler(Upnp_EventType EventType, const void* Event,
                         [[maybe_unused]] void* Cookie) {

    // Print a summary of the event received
    std::cout << "Received event type \"" << EventType << "\" with event '"
              << Event << "'\n";
    return 0;
}

#if 0
// Disabled due to bug with IPv6 parse_hostport(). Will be enabled as soon as
// the bug is fixed.
TEST_F(UpnpapiFTestSuite, UpnpRegisterRootDevice3_successful) {
    if (github_actions) {
        //     if (old_code)
        //         GTEST_SKIP() << "Old code Unit Test takes too long time and "
        //                         "dynamically triggers run timeouts.";

        // Always disable extended debug messages.
        g_dbug = false; // Will be restored by the tests destructor.
    }

    CPupnplog logObj; // Output only with build type DEBUG.
    if (g_dbug)
        logObj.enable(UPNP_INFO);

    // The Unit needs a defined state, otherwise it may fail with
    // SEH exception 0xc0000005 on WIN32.
    bWebServerState = WEB_SERVER_DISABLED;
    sdkInit_mutex = PTHREAD_MUTEX_INITIALIZER;
    gIF_IPV6[0] = '\0';
    gIF_IPV6_PREFIX_LENGTH = 0;
    gIF_IPV6_ULA_GUA[0] = '\0';
    gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
    gIF_IPV4[0] = '\0';
    gIF_IPV4_NETMASK[0] = '\0';

    int ret_UpnpInit2 = ::UpnpInit2(nullptr, 0);
    ASSERT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

    EXPECT_EQ(UpnpSetWebServerRootDir(SAMPLE_SOURCE_DIR "/web"), 0);

    // Prepare used local ip address.
    // Example: "http://192.168.24.88:49153/tvdevicedesc.xml"
    std::string desc_doc_url;
    int addr_family{AF_UNSPEC};
    if (gIF_IPV4[0] != '\0') {
        desc_doc_url = "http://" + std::string(gIF_IPV4) + ":" +
                       std::to_string(LOCAL_PORT_V4) + "/tvdevicedesc.xml";
        addr_family = AF_INET;
    } else if (gIF_IPV6_ULA_GUA[0] != '\0') {
        desc_doc_url = "http://[" + std::string(gIF_IPV6_ULA_GUA) +
                       "]:" + std::to_string(LOCAL_PORT_V6_ULA_GUA) +
                       "/tvdevicedesc.xml";
        addr_family = AF_INET6;
    } else if (gIF_IPV6[0] != '\0') {
        desc_doc_url = "http://[" + std::string(gIF_IPV6) + "%" +
                       std::to_string(gIF_INDEX) +
                       "]:" + std::to_string(LOCAL_PORT_V6) +
                       "/tvdevicedesc.xml";
        addr_family = AF_INET6;
    }

    std::cerr << "DEBUG! desc_doc_url=\"" << desc_doc_url << "\"\n";
    UpnpDevice_Handle device_handle = -1;

    // Test Unit
    int ret_UpnpRegisterRootDevice3 =
        ::UpnpRegisterRootDevice3(desc_doc_url.c_str(), CallbackEventHandler,
                                  &device_handle, &device_handle, addr_family);
    EXPECT_EQ(ret_UpnpRegisterRootDevice3, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpRegisterRootDevice3, UPNP_E_SUCCESS);

    EXPECT_GE(device_handle, 1);

    UpnpUnRegisterRootDevice(device_handle);
    UpnpFinish();
}
#endif

} // namespace utest

int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
    utest::nadaptObj.get_first();
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
