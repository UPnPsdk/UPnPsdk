// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-09-03

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

#include <utest/upnpdebug.hpp>
#include <utest/utest.hpp>


namespace utest {

using ::testing::AnyOf;
using ::testing::StartsWith;

using ::UPnPsdk::errStrEx;
using ::UPnPsdk::g_dbug;
using ::UPnPsdk::SInaddr;
using ::UPnPsdk::SSockaddr;
using ADDRS = UPnPsdk::CNetadapter::ADDRS;

#ifndef UPnPsdk_WITH_NATIVE_PUPNP
using ::compa::GetIfInfo;
#endif

// General storage for temporary socket address evaluation.
SSockaddr saObj;

// Have the netadapter list global available so its expensive call from the
// operating system is only one time necessary. Initialisation with
// nadaptObj.get_first() is called in the main() function at the end.
UPnPsdk::CNetadapter nadaptObj;


#ifdef UPnPsdk_WITH_NATIVE_PUPNP
// Some aliases for the old code.
auto& pthread_rwlock_trywrlock = ::ithread_rwlock_wrlock;
auto& pthread_rwlock_rdlock = ::ithread_rwlock_rdlock;
auto& pthread_rwlock_unlock = ::ithread_rwlock_unlock;
auto& pthread_rwlock_destroy = ::ithread_rwlock_destroy;
auto& sdkInit_mutex = ::gSDKInitMutex;
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

class UpnpapiClearFTestSuite : public ::testing::Test {
  private:
    bool m_dbug_flag{UPnPsdk::g_dbug};

  protected:
    // Constructor
    UpnpapiClearFTestSuite() {
        // Set or destroy global variables to detect side effects.
        if (old_code) {
            // Initializing used global variable is important! Otherwise I get
            // wrong initialisation with old code.
            gIF_NAME[0] = '\0';
            gIF_IPV6[0] = '\0';
            gIF_IPV6_PREFIX_LENGTH = 0;
            gIF_IPV6_ULA_GUA[0] = '\0';
            gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
            gIF_IPV4[0] = '\0';
            gIF_IPV4_NETMASK[0] = '\0';
            UpnpSdkInit = 0;
            bWebServerState = WEB_SERVER_DISABLED;
        } else {
            ::strcpy(gIF_NAME, "<untouched>");
            ::strcpy(gIF_IPV6, "<untouched>");
            gIF_IPV6_PREFIX_LENGTH = ~0u;
            ::strcpy(gIF_IPV6_ULA_GUA, "<untouched>");
            gIF_IPV6_ULA_GUA_PREFIX_LENGTH = ~0u;
            ::strcpy(gIF_IPV4, "<untouched>");
            ::strcpy(gIF_IPV4_NETMASK, "<untouched>");
            UpnpSdkInit = 0x55555555;
            memset(&bWebServerState, 0xAA, sizeof(bWebServerState));
            memset(&sdkInit_mutex, 0xAA, sizeof(sdkInit_mutex));
        }
        sdkInit_mutex = PTHREAD_MUTEX_INITIALIZER; // Must be initialized.
        // On table of pointer is no way to detect invalid one; must set to 0.
        memset(&HandleTable, 0, sizeof(HandleTable));
        gIF_INDEX = ~0u;
        LOCAL_PORT_V6 = static_cast<in_port_t>(~0);
        LOCAL_PORT_V6_ULA_GUA = static_cast<in_port_t>(~0);
        LOCAL_PORT_V4 = static_cast<in_port_t>(~0);
        memset(&errno, 0xAA, sizeof(errno));
        memset(&GlobalHndRWLock, 0xAA, sizeof(GlobalHndRWLock));
        memset(&gUpnpSdkNLSuuid, 0xAA, sizeof(gUpnpSdkNLSuuid));
        memset(&gSendThreadPool, 0xAA, sizeof(gSendThreadPool));
        memset(&gRecvThreadPool, 0xAA, sizeof(gRecvThreadPool));
        memset(&gMiniServerThreadPool, 0xAA, sizeof(gMiniServerThreadPool));
        memset(&gTimerThread, 0xAA, sizeof(gTimerThread));
        // memset(&GlobalClientSubscribeMutex, 0xAA,
        //        sizeof(GlobalClientSubscribeMutex));
    }

    // Destructor
    ~UpnpapiClearFTestSuite() { UPnPsdk::g_dbug = m_dbug_flag; }
};

class UpnpapiFTestSuite : public UpnpapiClearFTestSuite {
  protected:
    CPupnplog m_logObj; // Output only with build type DEBUG.

    // Constructor
    UpnpapiFTestSuite() {
        if (g_dbug)
            m_logObj.enable(UPNP_ALL);
    }
};


// Subroutine for multiple check of empty global addresses.
void chk_empty_gifaddr() {
    if (old_code) {
        if (gIF_NAME[0] != '\0')
            std::cout
                << CYEL "[ BUGFIX   ] " CRES << __LINE__
                << ": An invalid netadapter name must not modify gIF_NAME to \""
                << gIF_NAME << "\".\n";
        EXPECT_EQ(gIF_INDEX, ~0u);
        EXPECT_EQ(gIF_IPV6[0], '\0');
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
        EXPECT_EQ(gIF_IPV6_ULA_GUA[0], '\0');
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
        EXPECT_EQ(gIF_IPV4[0], '\0');
        EXPECT_EQ(gIF_IPV4_NETMASK[0], '\0');

    } else {

        EXPECT_STREQ(gIF_NAME, "<untouched>");
        EXPECT_EQ(gIF_INDEX, ~0u);
        EXPECT_STREQ(gIF_IPV6, "<untouched>");
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, ~0u);
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, "<untouched>");
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, ~0u);
        EXPECT_STREQ(gIF_IPV4, "<untouched>");
        EXPECT_STREQ(gIF_IPV4_NETMASK, "<untouched>");
    }
}

TEST_F(UpnpapiClearFTestSuite, UpnpInitPreamble_successful) {
    // Initialize needed global variable.
    UpnpSdkInit = 0;

    // Check if sockets are available. This is mainly of interest for Winsock on
    // Microsoft Windows.
    SOCKET sfd = ::socket(AF_INET6, SOCK_STREAM, 0);
    ASSERT_NE(sfd, INVALID_SOCKET);
    CLOSE_SOCKET_P(sfd);

    if (g_dbug)
        // In 'UpnpInitPreamble()' is only 'UpnpInitLog()' called that does not
        // enable old_code debug logging. Next line is needed before to enable
        // logging.
        UpnpSetLogFileNames(nullptr, nullptr); // logs to stderr

    // Test Unit
    // ---------
    // UpnpInitPreamble() should not use and modify the UpnpSdkInit flag.
    int ret_UpnpInitPreamble = UpnpInitPreamble();
    ASSERT_EQ(ret_UpnpInitPreamble, UPNP_E_SUCCESS) // ASSERT needed.
        << errStrEx(ret_UpnpInitPreamble, UPNP_E_SUCCESS);

    // Check initialization of debug output.
#ifdef DEBUG
    EXPECT_EQ(UpnpGetDebugFile(static_cast<Upnp_LogLevel>(NULL),
                               static_cast<Dbg_Module>(NULL)),
              g_dbug ? stderr : nullptr);
#else
    EXPECT_EQ(UpnpGetDebugFile(static_cast<Upnp_LogLevel>(NULL),
                               static_cast<Dbg_Module>(NULL)),
              nullptr);
#endif

    // Check if global read-write locks are initialized
    ASSERT_EQ(pthread_rwlock_trywrlock(&GlobalHndRWLock), 0);
    ASSERT_EQ(pthread_rwlock_unlock(&GlobalHndRWLock), 0);

    // Check creation of a uuid
    EXPECT_THAT(gUpnpSdkNLSuuid,
                MatchesStdRegex("[[:xdigit:]]{8}-[[:xdigit:]]{4}-[[:xdigit:]]{"
                                "4}-[[:xdigit:]]{4}-[[:xdigit:]]{12}"));

    // Check initialization of the UPnP device and client (control point) handle
    // table
    ASSERT_EQ(pthread_rwlock_rdlock(&GlobalHndRWLock), 0);
    bool handleTable_initialized{true};
    for (int i = 0; i < NUM_HANDLE; ++i) {
        if (HandleTable[i] != nullptr) {
            handleTable_initialized = false;
            break;
        }
    }
    ASSERT_EQ(pthread_rwlock_unlock(&GlobalHndRWLock), 0);
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
    EXPECT_EQ(UpnpSdkInit, 0);

    UpnpSdkInit = 1;
    int ret_UpnpFinish = UpnpFinish();
    EXPECT_EQ(ret_UpnpFinish, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpFinish, UPNP_E_SUCCESS);

    EXPECT_EQ(UpnpSdkInit, 0);

    // Needed Deinitialisations
    pthread_rwlock_destroy(&GlobalHndRWLock);
}

TEST_F(UpnpapiFTestSuite, get_error_message) {
#ifndef UPNP_HAVE_TOOLS
    GTEST_SKIP() << "Option UPnPsdk_WITH_TOOLS not available.";
#endif
    EXPECT_STREQ(UpnpGetErrorMessage(0), "UPNP_E_SUCCESS");
    EXPECT_STREQ(UpnpGetErrorMessage(-121), "UPNP_E_INVALID_INTERFACE");
    EXPECT_STREQ(UpnpGetErrorMessage(1), "Unknown error code");
}

TEST_F(UpnpapiFTestSuite, GetHandleInfo_successful) {
    // Will be filled with a pointer to the requested client info.
    Handle_Info* hinfo_p{nullptr};

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
    EXPECT_EQ(hinfo_p, nullptr);
    EXPECT_EQ(GetHandleInfo(NUM_HANDLE, &hinfo_p), HND_INVALID);
    EXPECT_EQ(hinfo_p, nullptr);
    EXPECT_EQ(GetHandleInfo(NUM_HANDLE + 1, &hinfo_p), HND_INVALID);
    EXPECT_EQ(hinfo_p, nullptr);

    EXPECT_EQ(GetHandleInfo(1, &hinfo_p), HND_INVALID);
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
    // Unit needs a nullptr initialized HandleTable because there is no way to
    // detect an invalid pointer. This is done with the test fixture. Otherweise
    // it will segfault.

    // Test Unit with nullptr to result variable
    EXPECT_EQ(GetHandleInfo(1, nullptr), HND_INVALID);

    // This would be filled with a pointer to the requested client info.
    Handle_Info* hinfo_p{nullptr};
    // Test Unit
    EXPECT_EQ(GetHandleInfo(1, &hinfo_p), HND_INVALID);
}

TEST_F(UpnpapiFTestSuite, get_free_handle_successful) {
    Handle_Info hinfo;
    HandleTable[0] = nullptr; // Not used.
    HandleTable[1] = &hinfo;
    HandleTable[2] = nullptr;
    HandleTable[3] = &hinfo;
    HandleTable[4] = nullptr;

    EXPECT_EQ(::GetFreeHandle(), 2);
}

#ifndef UPnPsdk_WITH_NATIVE_PUPNP
TEST_F(UpnpapiFTestSuite, GetIfInfo_with_unspec_address) {
    saObj = SInaddr("[::1]");
    saObj.family = AF_UNSPEC;

    // Test Unit.
    int ret_GetIfInfo = GetIfInfo(saObj);

    ASSERT_EQ(ret_GetIfInfo, UPNP_E_INVALID_INTERFACE)
        << errStrEx(ret_GetIfInfo, UPNP_E_INVALID_INTERFACE);
}

TEST_F(UpnpapiFTestSuite, GetIfInfo_with_ipv6_unspec_address) {
    saObj.clear();

    // Test Unit.
    int ret_GetIfInfo = GetIfInfo(saObj);

    ASSERT_EQ(ret_GetIfInfo, UPNP_E_INVALID_INTERFACE)
        << errStrEx(ret_GetIfInfo, UPNP_E_INVALID_INTERFACE);
}

TEST_F(UpnpapiFTestSuite, GetIfInfo_with_ipv6_loopback_address) {
    saObj.clear();
    saObj.sin6.sin6_addr.s6_addr[15] = 1; // Short setting for "[::1]".

    // Test Unit with IPv6 loopback address.
    int ret_GetIfInfo = GetIfInfo(saObj);

    ASSERT_EQ(ret_GetIfInfo, UPNP_E_SUCCESS)
        << errStrEx(ret_GetIfInfo, UPNP_E_SUCCESS);

    EXPECT_THAT(gIF_NAME, AnyOf(StartsWith("lo"), StartsWith("Lo")));
    EXPECT_EQ(gIF_INDEX, 1);
    EXPECT_STREQ(gIF_IPV6, "");
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "::1");
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 128);
    EXPECT_STREQ(gIF_IPV4, "<untouched>");
    EXPECT_STREQ(gIF_IPV4_NETMASK, "<untouched>");
}

TEST_F(UpnpapiFTestSuite, GetIfInfo_with_ipv4_loopback_address) {
    // Initializing with IP addresses isn't supported by pUPnP, but with
    // UPnPsdk. Ports not set with this Unit so they doesn't matter here.
    saObj = SInaddr("127.0.0.1");

    // Test Unit.
    // The real used loopback address can be "127.0.0.1" to "127.255.255.254".
    // An IPv4 address is mapped to an IPv6 address.
    int ret_GetIfInfo = GetIfInfo(saObj);

    ASSERT_EQ(ret_GetIfInfo, UPNP_E_SUCCESS)
        << errStrEx(ret_GetIfInfo, UPNP_E_SUCCESS);

    EXPECT_THAT(gIF_NAME, AnyOf(StartsWith("lo"), StartsWith("Lo")));
    EXPECT_GT(gIF_INDEX, 0);
    EXPECT_STREQ(gIF_IPV6, "");
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "::ffff:127.0.0.1");
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 96);
    EXPECT_STREQ(gIF_IPV4, "<untouched>");
    EXPECT_STREQ(gIF_IPV4_NETMASK, "<untouched>");
}

TEST_F(UpnpapiFTestSuite, GetIfInfo_from_lla) {
    // Initializing with IP addresses isn't supported by pUPnP, but with
    // UPnPsdk. Ports not set with this Unit so they doesn't matter here.

    ASSERT_TRUE(nadaptObj.find_first(ADDRS::lla))
        << "Fatal error. No link-local address found.";
    nadaptObj.sockaddr(saObj);

    // Test Unit
    int ret_GetIfInfo = GetIfInfo(saObj);

    ASSERT_EQ(ret_GetIfInfo, UPNP_E_SUCCESS)
        << errStrEx(ret_GetIfInfo, UPNP_E_SUCCESS);

    in6_addr sin6_addr;
    ASSERT_EQ(::inet_pton(AF_INET6, gIF_IPV6, &sin6_addr), 1);
    EXPECT_EQ(memcmp(&sin6_addr, &saObj.sin6.sin6_addr, sizeof(in6_addr)), 0);
    EXPECT_EQ(gIF_NAME, nadaptObj.name());
    EXPECT_EQ(gIF_INDEX, nadaptObj.index());
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 64);
    // I test against real network interfaces. So a gua may be set on the
    // same network interface, or not, depending on the current
    // environment. Will be tested in its own Unit Test.
    // EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    // EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
    EXPECT_STREQ(gIF_IPV4, "<untouched>");
    EXPECT_STREQ(gIF_IPV4_NETMASK, "<untouched>");
}

TEST_F(UpnpapiFTestSuite, GetIfInfo_from_gua) {
    // Initializing with IP addresses isn't supported by pUPnP, but with
    // UPnPsdk. Ports not set with this Unit so they doesn't matter here.

    if (!nadaptObj.find_first(ADDRS::gua))
        GTEST_SKIP() << "No usable Global Unicast Address found for testing.";

    nadaptObj.sockaddr(saObj);

    // Test Unit
    int ret_GetIfInfo = GetIfInfo(saObj);

    ASSERT_EQ(ret_GetIfInfo, UPNP_E_SUCCESS)
        << errStrEx(ret_GetIfInfo, UPNP_E_SUCCESS);

    in6_addr sin6_addr;
    ASSERT_EQ(::inet_pton(AF_INET6, gIF_IPV6_ULA_GUA, &sin6_addr), 1);
    EXPECT_EQ(memcmp(&sin6_addr, &saObj.sin6.sin6_addr, sizeof(in6_addr)), 0);
    EXPECT_EQ(gIF_NAME, nadaptObj.name());
    EXPECT_EQ(gIF_INDEX, nadaptObj.index());
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, nadaptObj.bitmask());
    EXPECT_EQ(gIF_IPV6[0], '\0'); // An lla is cleared. We only get the gua.
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
    EXPECT_STREQ(gIF_IPV4, "<untouched>");
    EXPECT_STREQ(gIF_IPV4_NETMASK, "<untouched>");
}

TEST_F(UpnpapiFTestSuite, GetIfInfo_with_netadapter_index) {
    // Initializing with IP addresses isn't supported by pUPnP, but with
    // UPnPsdk. Ports not set with this Unit so they doesn't matter here.

    // Get an LLA and use its index.
    ASSERT_TRUE(nadaptObj.find_first(ADDRS::lla))
        << "Fatal error. No link-local address found.";
    auto index = nadaptObj.index();

    // Test Unit
    int ret_GetIfInfo = GetIfInfo(index);
    ASSERT_EQ(ret_GetIfInfo, UPNP_E_SUCCESS)
        << errStrEx(ret_GetIfInfo, UPNP_E_SUCCESS);

    ASSERT_TRUE(nadaptObj.find_first("[" + std::string(gIF_IPV6) + "%" +
                                     std::to_string(index) + "]"));

    EXPECT_EQ(gIF_NAME, nadaptObj.name());
    EXPECT_EQ(gIF_INDEX, index);
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, nadaptObj.bitmask());
    EXPECT_STREQ(gIF_IPV4, "<untouched>");
    EXPECT_STREQ(gIF_IPV4_NETMASK, "<untouched>");

    ASSERT_TRUE(nadaptObj.find_first(index));
    bool found{false};
    do {
        nadaptObj.sockaddr(saObj);
        if (UPnPsdk::IN6_ADDR_GLOBAL(&saObj.sin6.sin6_addr)) {
            found = true;
            break;
        }
    } while (nadaptObj.find_next());

    if (!found) {
        EXPECT_EQ(gIF_IPV6_ULA_GUA[0], '\0');
    } else {
        ASSERT_TRUE(
            nadaptObj.find_first("[" + std::string(gIF_IPV6_ULA_GUA) + "]"));
        ASSERT_EQ(nadaptObj.index(), index);
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, nadaptObj.bitmask());
    }
}
#endif // UPnPsdk_WITH_NATIVE_PUPNP

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_with_ipv6_loopback_address) {
    // Initializing with IP addresses isn't supported by pUPnP, but with
    // UPnPsdk. Ports not set with this Unit so they doesn't matter here.

    // Test Unit with IPv6 loopback address.
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo("[::1]");

    if (old_code) {
        // pUPnP does not support the loopback interface.
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": UpnpGetIfInfo() only from network interface name "
                     "supported, not from IP-addresses.\n";
        ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);
        {
            SCOPED_TRACE("");
            chk_empty_gifaddr();
        }

    } else {

        ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

        EXPECT_THAT(gIF_NAME, AnyOf(StartsWith("lo"), StartsWith("Lo")));
        EXPECT_EQ(gIF_INDEX, 1);
        EXPECT_STREQ(gIF_IPV6, "");
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, "::1");
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 128);
        EXPECT_STREQ(gIF_IPV4, "<untouched>");
        EXPECT_STREQ(gIF_IPV4_NETMASK, "<untouched>");
    }
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_with_ipv4_loopback_address) {
    // Initializing with IP addresses isn't supported by pUPnP, but with
    // UPnPsdk. Ports not set with this Unit so they doesn't matter here.

    // Test Unit.
    // The real used loopback address can be "127.0.0.1" to "127.255.255.254".
    // An IPv4 address is mapped to an IPv6 address.
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo("127.0.0.1");

    if (old_code) {
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": UpnpGetIfInfo() only from network interface name "
                     "supported, not from IP-addresses.\n";
        EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);
        {
            SCOPED_TRACE("");
            chk_empty_gifaddr();
        }

    } else {

        ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

        EXPECT_THAT(gIF_NAME, AnyOf(StartsWith("lo"), StartsWith("Lo")));
        EXPECT_GT(gIF_INDEX, 0);
        EXPECT_STREQ(gIF_IPV6, "");
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, "::ffff:127.0.0.1");
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 96);
        EXPECT_STREQ(gIF_IPV4, "<untouched>");
        EXPECT_STREQ(gIF_IPV4_NETMASK, "<untouched>");
    }
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_with_ifname_having_only_ipv6) {
    // Select netadapter that has only LLA addresses and test UpnpGetIfInfo
    // with its interface name.

    // First collect the netadapter index of all known ip addresses. Condense
    // it to the only needed information, that is: index number and if it is an
    // index number of an IPv4 address. The latter is marked negative.
    std::vector<long int> indexes;
    nadaptObj.find_first(ADDRS::lla | ADDRS::gua | ADDRS::map4);
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
    if (index == 0)
        GTEST_SKIP()
            << "No local network adapter with usable ip address found.";

    // Select the local netadapter that has only IPv6 addresses.
    // ---------------------------------------------------------
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
    ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

    EXPECT_STREQ(gIF_NAME, nadaptObj.name().c_str());
    EXPECT_EQ(gIF_INDEX, index);
    nadaptObj.sockaddr(saObj);
    if (gIF_IPV6[0] != '\0') {
        std::string if_addr{"[" + std::string(gIF_IPV6) + "%" +
                            std::to_string(gIF_INDEX) + "]"};
        EXPECT_TRUE(nadaptObj.find_first(if_addr))
            << "Cannot find gIF_IPV6=\"" << if_addr << "\"";
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, nadaptObj.bitmask());
    }
    if (gIF_IPV6_ULA_GUA[0] != '\0') {
        std::string if_addr{"[" + std::string(gIF_IPV6_ULA_GUA) + "]"};
        EXPECT_TRUE(nadaptObj.find_first(if_addr))
            << "Cannot find gIF_IPV6_ULA_GUA=\"" << if_addr << "\"";
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, nadaptObj.bitmask());
    }
    if (old_code) {
        EXPECT_EQ(gIF_IPV4[0], '\0');
        EXPECT_EQ(gIF_IPV4_NETMASK[0], '\0');
    } else {
        EXPECT_STREQ(gIF_IPV4, "<untouched>");
        EXPECT_STREQ(gIF_IPV4_NETMASK, "<untouched>");
    }
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_from_lla) {
    // pUPnP does not support initialisation with IP addresses, but the UPnPsdk
    // do. Ports not set with this Unit so they don't matter here.
    ASSERT_TRUE(nadaptObj.find_first(ADDRS::lla));
    nadaptObj.sockaddr(saObj);

    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo(saObj.netaddr().c_str());

    if (old_code) {
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": UpnpGetIfInfo() only from network interface name "
                     "supported, not from IP-addresses.\n";
        EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);
        {
            SCOPED_TRACE("");
            chk_empty_gifaddr();
        }

    } else {

        ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

        in6_addr sin6_addr;
        ASSERT_EQ(::inet_pton(AF_INET6, gIF_IPV6, &sin6_addr), 1);
        EXPECT_EQ(memcmp(&sin6_addr, &saObj.sin6.sin6_addr, sizeof(in6_addr)),
                  0);
        EXPECT_EQ(gIF_NAME, nadaptObj.name());
        EXPECT_EQ(gIF_INDEX, nadaptObj.index());
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, nadaptObj.bitmask());
        // I test against real network interfaces. So a gua may be set on the
        // same network interface, or not, depending on the current
        // environment. Will be tested in its own Unit Test.
        // EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
        // EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
        EXPECT_STREQ(gIF_IPV4, "<untouched>");
        EXPECT_STREQ(gIF_IPV4_NETMASK, "<untouched>");
    }
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_with_lla_ifname_successful) {
    // Ports not set with this Unit so they doesn't matter here.
    // For Microsoft Windows there are some TODOs in the old code:
    // TODO: Retrieve IPv6 ULA or GUA address and its prefix. Only keep IPv6
    // link-local addresses.
    // TODO: Retrieve IPv6 LLA prefix.

    if (old_code)
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": gIF_IPV6_ULA_GUA, and gIF_IPV6_PREFIX_LENGTH should be "
                     "set on MS Windows.\n";

    if (!nadaptObj.find_first(ADDRS::lla))
        GTEST_SKIP() << "No usable link-local Address found for testing.";

    // Test Unit.
    // Using adapter name of the lla interface.
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo(nadaptObj.name().c_str());
    ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

    EXPECT_EQ(gIF_NAME, nadaptObj.name());
    nadaptObj.sockaddr(saObj);
    EXPECT_EQ("[" + std::string(gIF_IPV6) + "%" + std::to_string(gIF_INDEX) +
                  "]",
              saObj.netaddr());
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH,
              (old_code && compiler == CO::msc) ? 0 : nadaptObj.bitmask());

    // Look for additional GUA and friends on current selected netadapter.
    nadaptObj.find_first(nadaptObj.index()); // Restricts find_next() to
                                             // netadapter scope.
    bool gua(false), map4(false);
    do {
        nadaptObj.sockaddr(saObj);
        if (UPnPsdk::IN6_ADDR_GLOBAL(&saObj.sin6.sin6_addr) ||
            IN6_IS_ADDR_LOOPBACK(&saObj.sin6.sin6_addr))
            gua = true;
        else if (IN6_IS_ADDR_V4MAPPED(&saObj.sin6.sin6_addr))
            map4 = true;
    } while (!gua && !map4 && nadaptObj.find_next());

    if (gua) {
        EXPECT_EQ("[" + std::string(gIF_IPV6_ULA_GUA) + "]", saObj.netaddr());
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, nadaptObj.bitmask());
    } else if (map4) {
        if (old_code) {
            EXPECT_EQ(gIF_IPV6_ULA_GUA[0], '\0');
            EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
            EXPECT_EQ("[::ffff:" + std::string(gIF_IPV4) + "]",
                      saObj.netaddr());
            EXPECT_THAT(gIF_IPV4_NETMASK, StartsWith("255."));
        } else {
            EXPECT_EQ("[::ffff:" + std::string(gIF_IPV6_ULA_GUA) + "]",
                      saObj.netaddr());
            EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, nadaptObj.bitmask());
            EXPECT_STREQ(gIF_IPV4, "<untouched>");
            EXPECT_STREQ(gIF_IPV4_NETMASK, "<untouched>");
        }
    } else {
        EXPECT_EQ(gIF_IPV6_ULA_GUA[0], '\0');
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
        if (old_code) {
            // EXPECT_EQ(gIF_IPV4[0], '\0'); DEBUG! May be set
            // EXPECT_EQ(gIF_IPV4_NETMASK[0], '\0'); DEBUG! May be set
        } else {
            EXPECT_STREQ(gIF_IPV4, "<untouched>");
            EXPECT_STREQ(gIF_IPV4_NETMASK, "<untouched>");
        }
    }
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_from_lla_without_scope_id_fails) {
    // pUPnP does not support initialisation with IP addresses, but the UPnPsdk
    // do. Ports not set with this Unit so they don't matter here.

    // Get link-local network interface address.
    ASSERT_TRUE(nadaptObj.find_first(ADDRS::lla));
    nadaptObj.sockaddr(saObj);

    // Remove scope_id from socket address.
    saObj.sin6.sin6_scope_id = 0;

    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo(saObj.netaddr().c_str());

    EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);
    {
        SCOPED_TRACE("");
        chk_empty_gifaddr();
    }
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_from_gua) {
    // pUPnP does not support initialisation with IP addresses, but the UPnPsdk
    // do. Ports not set with this Unit so they doesn't matter here.

    if (!nadaptObj.find_first(ADDRS::gua))
        GTEST_SKIP() << "No usable Global Unicast Address found for testing.";

    nadaptObj.sockaddr(saObj);

    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo(saObj.netaddr().c_str());

    if (old_code) {
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": UpnpGetIfInfo() only from network interface name "
                     "supported, not from IP-addresses.\n";
        ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);
        {
            SCOPED_TRACE("");
            chk_empty_gifaddr();
        }

    } else {

        ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

        char ip6[INET6_ADDRSTRLEN + 1];
        // Strip leading character on copying.
        std::strncpy(ip6, saObj.netaddr().c_str() + 1, sizeof(ip6) - 1);
        // Strip trailing bracket.
        if (char* chptr{::strchr(ip6, ']')})
            *chptr = '\0';

        EXPECT_EQ(gIF_NAME, nadaptObj.name());
        EXPECT_EQ(gIF_INDEX, nadaptObj.index());
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, ip6);
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, nadaptObj.bitmask());
        EXPECT_STREQ(gIF_IPV6, ""); // An lla is cleared. We only get the gua.
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
        EXPECT_STREQ(gIF_IPV4, "<untouched>");
        EXPECT_STREQ(gIF_IPV4_NETMASK, "<untouched>");
    }
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_with_gua_ifname_successful) {
    // Ports not set with this Unit so they doesn't matter here.
    // For Microsoft Windows there are some TODOs in the old code:
    // TODO: Retrieve IPv6 ULA or GUA address and its prefix. Only keep IPv6
    // link-local addresses.
    // TODO: Retrieve IPv6 LLA prefix?

    if (old_code)
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": gIF_IPV6_ULA_GUA, and gIF_IPV6_PREFIX_LENGTH should be "
                     "set on MS Windows.\n";

    if (!nadaptObj.find_first(ADDRS::gua))
        GTEST_SKIP() << "No usable Global Unicast Address found for testing.";

    // Test Unit.
    // Using adapter name of the gua interface.
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo(nadaptObj.name().c_str());
    ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

    EXPECT_EQ(gIF_NAME, nadaptObj.name());
    nadaptObj.sockaddr(saObj);
    EXPECT_EQ("[" + std::string(gIF_IPV6_ULA_GUA) + "]", saObj.netaddr());
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, nadaptObj.bitmask());

    // Look for additional LLA on current selected netadapter.
    nadaptObj.find_first(nadaptObj.index()); // Restricts find_next() to
                                             // netadapter scope.
    bool found(false);
    do {
        nadaptObj.sockaddr(saObj);
        if (UPnPsdk::IN6_ADDR_LINKLOCAL(&saObj.sin6.sin6_addr))
            found = true;
    } while (!found && nadaptObj.find_next());

    if (found) {
        EXPECT_EQ("[" + std::string(gIF_IPV6) + "%" +
                      std::to_string(gIF_INDEX) + "]",
                  saObj.netaddr());
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, nadaptObj.bitmask());
    } else {
        EXPECT_EQ(gIF_IPV6[0], '\0');
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
    }

    if (old_code) {
        EXPECT_EQ(gIF_IPV4[0], '\0');
        EXPECT_EQ(gIF_IPV4_NETMASK[0], '\0');
    } else {
        EXPECT_STREQ(gIF_IPV4, "<untouched>");
        EXPECT_STREQ(gIF_IPV4_NETMASK, "<untouched>");
    }
}

#if 0 // Work in progress. Continue when netadapter accepts map4.
TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_with_map4_ifname_successful) {
    // Ports not set with this Unit so they doesn't matter here.
    // For Microsoft Windows there are some TODOs in the old code:
    // TODO: Retrieve IPv6 ULA or GUA address and its prefix. Only keep IPv6
    // link-local addresses.
    // TODO: Retrieve IPv6 LLA prefix?

    if (old_code)
        std::cout << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": gIF_IPV6_ULA_GUA, and gIF_IPV6_PREFIX_LENGTH should be "
                     "set on MS Windows.\n";

    if (!nadaptObj.find_first(ADDRS::map4))
        GTEST_SKIP() << "No usable IPv4-mapped IPv6 Address found for testing.";

    // Test Unit.
    // Using adapter name of the interface with map4 address.
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo(nadaptObj.name().c_str());
    ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

    EXPECT_EQ(gIF_NAME, nadaptObj.name());
    nadaptObj.sockaddr(saObj);
    if (old_code) {
        // gIF_IPV6_ULA_GUA is empty, gIF_IPV4 is set.
        EXPECT_EQ(gIF_IPV6_ULA_GUA[0], '\0');
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
        EXPECT_EQ("[::ffff:" + std::string(gIF_IPV4) + "]", saObj.netaddr());
        EXPECT_THAT(gIF_IPV4_NETMASK, StartsWith("255."));
    } else {
        // gIF_IPV6_ULA_GUA has V4MAPPED address, gIF_IPV4 is not used.
        EXPECT_EQ("[" + std::string(gIF_IPV6_ULA_GUA) + "]", saObj.netaddr());
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, nadaptObj.bitmask());
        EXPECT_STREQ(gIF_IPV4, "<untouched>");
        EXPECT_STREQ(gIF_IPV4_NETMASK, "<untouched>");
    }

    // Look for additional LLA on current selected netadapter.
    nadaptObj.find_first(nadaptObj.index()); // Restricts find_next() to
                                             // netadapter scope.
    bool found(false);
    do {
        nadaptObj.sockaddr(saObj);
        if (UPnPsdk::IN6_ADDR_LINKLOCAL(&saObj.sin6.sin6_addr))
            found = true;
    } while (!found && nadaptObj.find_next());

    if (found) {
        EXPECT_EQ("[" + std::string(gIF_IPV6) + "%" +
                      std::to_string(gIF_INDEX) + "]",
                  saObj.netaddr());
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, nadaptObj.bitmask());
    } else {
        EXPECT_EQ(gIF_IPV6[0], '\0');
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
    }
}
#endif

#if 0 // Work in progress. Continue when netadapter accepts map4.
TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_default_best_choise_successful) {
    // Ports not set with this Unit so they doesn't matter here.

    // Test Unit
    // This should find the first (best) local ip address.
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo();
    ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

    // A link-local address must always be available.
    ASSERT_NE(gIF_IPV6[0], '\0');
    // Check if it's the right one.
    nadaptObj.find_first(ADDRS::lla);
    nadaptObj.sockaddr(saObj);

    char ip6[INET6_ADDRSTRLEN + 1];
    // Strip leading character on copying.
    std::strncpy(ip6, saObj.netaddr().c_str() + 1, sizeof(ip6) - 1);
    // Strip trailing scope.
    if (char* chptr{::strchr(ip6, '%')})
        *chptr = '\0';

    // Not empty.
    EXPECT_EQ(gIF_NAME, nadaptObj.name());
    // Should have the scope_id of gIF_IPV6, that's the link-local address.
    EXPECT_EQ(gIF_INDEX, nadaptObj.index());
    EXPECT_STREQ(gIF_IPV6, ip6);
    EXPECT_THAT(gIF_IPV6_PREFIX_LENGTH, nadaptObj.bitmask());

    // Check if there is also a global unicast address.
    if (nadaptObj.find_first(ADDRS::gua)) {
        ASSERT_NE(gIF_IPV6_ULA_GUA[0], '\0');
        nadaptObj.sockaddr(saObj);
        // Strip leading character on copying.
        std::strncpy(ip6, saObj.netaddr().c_str() + 1, sizeof(ip6) - 1);
        // Strip trailing bracket.
        if (char* chptr{::strchr(ip6, ']')})
            *chptr = '\0';

        // Should still have the scope_id of gIF_IPV6, for that we need it.
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, ip6);
        EXPECT_EQ(gIF_INDEX, nadaptObj.index());
        EXPECT_THAT(gIF_IPV6_ULA_GUA_PREFIX_LENGTH,
                    compiler == CO::msc ? 0 : nadaptObj.bitmask());
    } else {
        ASSERT_EQ(gIF_IPV6_ULA_GUA[0], '\0');
    }
}
#endif

TEST(UpnpapiTestSuite, UpnpGetIfInfo_with_deprecated_ip_addresses) {
    if (!github_actions)
        GTEST_FAIL() << "Still needs to be done.";

    /* clang-format off
Error with deprecated ip-addresses needs to be mocked
=====================================================
On dynamic change of the IPv6 address I have temporary seen the following
IP address configuration. This is done by protocol when the remote server
changes its global unicast address. It is no problem when following the
protocol and just select the first offered IP address, that is the prefered
address and not deprecated. Older IP addresses marked as "deprecated" and
should not be used anymore for new connections.

Old code does not follow the protocol and selects the last GUA first. That is
the deprecated ip-address and could cause problems.

      Start 40: ctest_upnpapi2-pst
40/69 Test #40: ctest_upnpapi2-pst ...............***Failed    0.15 sec
Note: Randomizing tests' orders with a seed of 88422 .
[==========] Running 27 tests from 2 test suites.
--- snip ---
             NOTE! This is testing 'gua_ifname'
                                   ############
[ RUN      ] UpnpapiFTestSuite.UpnpGetIfInfo_with_gua_ifname_successful
[    FIX   ] 842: gIF_IPV6_ULA_GUA, and gIF_IPV6_PREFIX_LENGTH should be set on MS Windows.
/home/ingo/devel/UPnPsdk-dev/UPnPsdk-project/Utest/compa/api.d/test_upnpapi2.cpp:857: Failure
Expected equality of these values:
  "[" + std::string(gIF_IPV6_ULA_GUA) + "]"
    Which is: "[2003:d5:2722:700:5054:ff:fe7f:c021]"
  saObj.netaddr()
    Which is: "[2003:d5:2704:9d00:5054:ff:fe7f:c021]"

[  FAILED  ] UpnpapiFTestSuite.UpnpGetIfInfo_with_gua_ifname_successful (0 ms)
--- snip ---
             NOTE! This is testing 'lla_ifname'
                                   ############
[ RUN      ] UpnpapiFTestSuite.UpnpGetIfInfo_with_lla_ifname_successful
[    FIX   ] 703: gIF_IPV6_ULA_GUA, and gIF_IPV6_PREFIX_LENGTH should be set on MS Windows.
/home/ingo/devel/UPnPsdk-dev/UPnPsdk-project/Utest/compa/api.d/test_upnpapi2.cpp:737: Failure
Expected equality of these values:
  "[" + std::string(gIF_IPV6_ULA_GUA) + "]"
    Which is: "[2003:d5:2722:700:5054:ff:fe7f:c021]"
  saObj.netaddr()
    Which is: "[2003:d5:2704:9d00:5054:ff:fe7f:c021]"

[  FAILED  ] UpnpapiFTestSuite.UpnpGetIfInfo_with_lla_ifname_successful (0 ms)
--- snip ---

:ingo@vdeb13-devel01 15:32:48 UPnPsdk-project$ ip addr
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host noprefixroute
       valid_lft forever preferred_lft forever
2: ens1: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP group default qlen 1000
    link/ether 52:54:00:7f:c0:21 brd ff:ff:ff:ff:ff:ff
    altname enp2s1
    altname enx5254007fc021
    inet6 2003:d5:2704:9d00:5054:ff:fe7f:c021/64 scope global dynamic mngtmpaddr noprefixroute
       valid_lft 6984sec preferred_lft 1027sec
    inet6 fd00::5054:ff:fe7f:c021/64 scope global deprecated dynamic mngtmpaddr noprefixroute
       valid_lft 6150sec preferred_lft 0sec
    inet6 2003:d5:2722:700:5054:ff:fe7f:c021/64 scope global deprecated dynamic mngtmpaddr noprefixroute
       valid_lft 5537sec preferred_lft 0sec
    inet6 fe80::5054:ff:fe7f:c021/64 scope link proto kernel_ll
       valid_lft forever preferred_lft forever
3: ens2: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP group default qlen 1000
    link/ether 52:54:00:d4:b4:67 brd ff:ff:ff:ff:ff:ff
    altname enp2s2
    altname enx525400d4b467
    inet 192.168.24.88/24 metric 1024 brd 192.168.24.255 scope global dynamic ens2
       valid_lft 17091sec preferred_lft 17091sec
:ingo@vdeb13-devel01 15:34:42 UPnPsdk-project$
clang-format on
*/
}

TEST_F(UpnpapiClearFTestSuite, UpnpInit2_loopback_address) {
    if (g_dbug)
        // Needed to enable logging for old_code.
        UpnpSetLogFileNames(nullptr, nullptr); // Enable logging to stderr

    // Test Unit
    int ret_UpnpInit2 = ::UpnpInit2("[::1]", 61234);

    if (old_code) {
        ASSERT_EQ(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE);
        UpnpFinish();
        GTEST_SKIP() << "Specify a loopback address is not supported by pUPnP.";
    }

    if (github_actions && compiler == CO::msc) // DEBUG! Fix it with miniserver.
        EXPECT_EQ(ret_UpnpInit2, UPNP_E_SOCKET_BIND)
            << errStrEx(ret_UpnpInit2, UPNP_E_SOCKET_BIND);
    else
        EXPECT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
            << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

    UpnpFinish();
}

TEST_F(UpnpapiClearFTestSuite, UpnpInit2_lla_brackets_and_scope_id_successful) {
    ASSERT_TRUE(nadaptObj.find_first(ADDRS::lla));
    nadaptObj.sockaddr(saObj);

    if (g_dbug)
        // Needed to enable logging for old_code.
        UpnpSetLogFileNames(nullptr, nullptr); // Enable logging to stderr

    // Test Unit
    int ret_UpnpInit2 = ::UpnpInit2(saObj.netaddr().c_str(), 0);

    if (old_code) {
        EXPECT_EQ(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE);
        UpnpFinish();
        GTEST_SKIP()
            << "Specify an ipv6 link local address is not supported by pUPnP.";
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
    if (!github_actions) {
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
        EXPECT_EQ(LOCAL_PORT_V6_ULA_GUA, 0);
    }
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
    if (!github_actions) {
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
        EXPECT_EQ(LOCAL_PORT_V6_ULA_GUA, 0);
    }

    UpnpFinish();
}

TEST_F(UpnpapiClearFTestSuite, UpnpInit2_lla_no_brackets_with_scope_id) {
    ASSERT_TRUE(nadaptObj.find_first(ADDRS::lla));
    nadaptObj.sockaddr(saObj);

    // Create link-local address whithout brackets, with scope_id.
    char lla[INET6_ADDRSTRLEN + 1 + 10]{}; // + '%' + UINT_32_MAX with 10 digits
    ASSERT_NE(::inet_ntop(AF_INET6, &saObj.sin6.sin6_addr, lla, sizeof(lla)),
              nullptr);
    auto str_len = strlen(lla);
    ASSERT_GE(::snprintf(lla + str_len, sizeof(lla) - str_len, "%%%d",
                         nadaptObj.index()),
              0);

    if (g_dbug)
        // Needed to enable logging for old_code.
        UpnpSetLogFileNames(nullptr, nullptr); // Enable logging to stderr

    // Test Unit
    int ret_UpnpInit2 = ::UpnpInit2(lla, 0);

    if (old_code) {
        EXPECT_EQ(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE);
        UpnpFinish();
        GTEST_SKIP()
            << "Specify an ipv6 link-local address is not supported by pupnp.";
    }

    ASSERT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
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

    UpnpFinish();
}

TEST_F(UpnpapiClearFTestSuite, UpnpInit2_lla_no_scope_id_fails) {
    ASSERT_TRUE(nadaptObj.find_first(ADDRS::lla));
    nadaptObj.sockaddr(saObj);
    // Remove scope_id.
    saObj.sin6.sin6_scope_id = 0;

    if (g_dbug)
        // Needed to enable logging for old_code.
        UpnpSetLogFileNames(nullptr, nullptr); // Enable logging to stderr

    // Test Unit
    int ret_UpnpInit2 = ::UpnpInit2(saObj.netaddr().c_str(), 0);

    EXPECT_EQ(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE)
        << errStrEx(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE);

    UpnpFinish();
}

TEST_F(UpnpapiClearFTestSuite, UpnpInit2_gua_successful) {
    if (!nadaptObj.find_first(ADDRS::gua))
        GTEST_SKIP()
            << "No local network adapter with global unicast address found.";

    if (g_dbug)
        // Needed to enable logging for old_code.
        UpnpSetLogFileNames(nullptr, nullptr); // Enable logging to stderr

    // Test Unit
    nadaptObj.sockaddr(saObj);
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

    EXPECT_EQ(gIF_NAME, nadaptObj.name());
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

    UpnpFinish();
}

TEST_F(UpnpapiClearFTestSuite, UpnpInit2_with_netadapter_index_successful) {
    // Find a usable adapter.
    ASSERT_TRUE(nadaptObj.find_first())
        << "No local network adapter with usable IP address found.";
    uint32_t index = nadaptObj.index();

    // Get socket addresses from the found netadapter. Must have an lla, may
    // have a gua.
    SSockaddr lla_saObj, gua_saObj;
    nadaptObj.find_first(index);
    do {
        nadaptObj.sockaddr(saObj);
        if (lla_saObj.empty() &&
            UPnPsdk::IN6_ADDR_LINKLOCAL(&saObj.sin6.sin6_addr))
            lla_saObj = saObj;
        else if (gua_saObj.empty() &&
                 UPnPsdk::IN6_ADDR_GLOBAL(&saObj.sin6.sin6_addr))
            gua_saObj = saObj;
    } while (nadaptObj.find_next());

    if (g_dbug)
        // Needed to enable logging for old_code.
        UpnpSetLogFileNames(nullptr, nullptr); // Enable logging to stderr

    // Test Unit
    int ret_UpnpInit2 = ::UpnpInit2(std::to_string(index).c_str(), 0);

    if (old_code) {
        ASSERT_EQ(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpInit2, UPNP_E_INVALID_INTERFACE);
        UpnpFinish();
        GTEST_SKIP() << "Specify netadapter index is not supported by pUPnP.";
    }

    EXPECT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

    // Normalize gIF_IPV6.
    SSockaddr if_ipv6Obj;
    ASSERT_EQ(::inet_pton(AF_INET6, gIF_IPV6, &if_ipv6Obj.sin6.sin6_addr), 1);
    if_ipv6Obj.sin6.sin6_scope_id = gIF_INDEX;

    nadaptObj.find_first(index);
    EXPECT_EQ(gIF_NAME, nadaptObj.name());
    EXPECT_EQ(gIF_INDEX, nadaptObj.index());
    EXPECT_EQ(if_ipv6Obj, lla_saObj);
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 64);
    if (gIF_IPV6_ULA_GUA[0] != '\0') {
        saObj.clear();
        ASSERT_EQ(
            ::inet_pton(saObj.family, gIF_IPV6_ULA_GUA, &saObj.sin6.sin6_addr),
            1);
        EXPECT_EQ(saObj, gua_saObj);
        EXPECT_THAT(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, AnyOf(64, 96));
    }

    UpnpFinish();
}

// DEBUG! Modify test to support netinterfaces without lla,  only IPv4.
// That needs to remove IN6_IS_ADDR_V4MAPPED filter from netadapter module.
// Also split test into one with netadapter name and one with default.
TEST_F(UpnpapiClearFTestSuite, UpnpInit2_default_and_with_name_successful) {
    // For Microsoft Windows there are some TODOs in the old code:
    // TODO: Retrieve IPv6 ULA or GUA address and its prefix. Only keep IPv6
    // link-local addresses.
    // TODO: Retrieve IPv6 LLA prefix?

    if (old_code)
        std::cout
            << CYEL "[ BUGFIX   ] " CRES << __LINE__
            << ": Unit must not select oldest deprecated IPv6 address, when "
               "multiple local addresses on netadapter are available.\n";

    // Find a usable adapter.
    ASSERT_TRUE(nadaptObj.find_first())
        << "No local network adapter with usable IP address found.";
    uint32_t index = nadaptObj.index(); // That's the netadapter I'm using.

    // Get socket addresses from the found netadapter. Must have an lla, may
    // have a gua.
    SSockaddr lla_saObj, gua_saObj;
    nadaptObj.find_first(index); // Restricts find_next() to netinterface scope.
    do {
        nadaptObj.sockaddr(saObj);
        if (lla_saObj.empty() &&
            UPnPsdk::IN6_ADDR_LINKLOCAL(&saObj.sin6.sin6_addr))
            lla_saObj = saObj;
        else if (gua_saObj.empty() &&
                 UPnPsdk::IN6_ADDR_GLOBAL(&saObj.sin6.sin6_addr))
            gua_saObj = saObj;
    } while ((lla_saObj.empty() || gua_saObj.empty()) && nadaptObj.find_next());
    ASSERT_EQ(lla_saObj.family, AF_INET6);


    // Test Unit call with adapter name
    // --------------------------------
    nadaptObj.find_first(index);
    int ret_UpnpInit2 = ::UpnpInit2(nadaptObj.name().c_str(), 0);

    ASSERT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

    EXPECT_EQ(gIF_NAME, nadaptObj.name());
    EXPECT_EQ(gIF_INDEX, nadaptObj.index());
    saObj.clear();
    ::inet_pton(saObj.family, gIF_IPV6, &saObj.sin6.sin6_addr);
    saObj.sin6.sin6_scope_id = gIF_INDEX;

    if (old_code) {
        if (saObj != lla_saObj) {
            std::cout << "Wrong (deprecated?) local IP address selected:\n"
                      << "gIF_IPV6 netaddrp()=\"" << saObj.netaddrp()
                      << "\", netadapter netaddrp()=\"" << lla_saObj.netaddrp()
                      << "\"\n";
        }
    } else {
        EXPECT_EQ(saObj, lla_saObj)
            << "gIF_IPV6 netaddrp()=\"" << saObj.netaddrp()
            << "\", netadapter netaddrp()=\"" << lla_saObj.netaddrp() << "\"";
    }
#ifndef _MSC_VER
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 64);
#endif
    saObj.clear();
    ::inet_pton(saObj.family, gIF_IPV6_ULA_GUA, &saObj.sin6.sin6_addr);

    if (old_code) {
        if (saObj != gua_saObj) {
            std::cout << "Wrong (deprecated?) local IP address selected:\n"
                      << "gIF_IPV6_ULA_GUA netaddrp()=\"" << saObj.netaddrp()
                      << "\", netadapter netaddrp()=\"" << gua_saObj.netaddrp()
                      << "\"\n";
        }
    } else {
        EXPECT_EQ(saObj, gua_saObj)
            << "gIF_IPV6_ULA_GUA netaddrp()=\"" << saObj.netaddrp()
            << "\", netadapter netaddrp()=\"" << gua_saObj.netaddrp() << "\"";
    }
    // EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, DEBUG! May be 0 for AF_INET6
    //           gua_saObj.family == AF_INET6 ? 64 : 0);


    // Test Unit call default setting
    // ------------------------------
    // Cannot be used with old_code because it uses IPv4 by default.
    if (!old_code) {
        nadaptObj.find_first(index);
        ret_UpnpInit2 = ::UpnpInit2(nullptr, 0);

        EXPECT_EQ(gIF_NAME, nadaptObj.name());
        EXPECT_EQ(gIF_INDEX, nadaptObj.index());
        saObj.clear();
        ::inet_pton(saObj.family, gIF_IPV6, &saObj.sin6.sin6_addr);
        saObj.sin6.sin6_scope_id = gIF_INDEX;

        EXPECT_EQ(saObj, lla_saObj)
            << "gIF_IPV6 netaddrp()=\"" << saObj.netaddrp()
            << "\", netadapter netaddrp()=\"" << lla_saObj.netaddrp() << "\"";
#ifndef _MSC_VER
        EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 64);
#endif
        saObj.clear();
        ::inet_pton(saObj.family, gIF_IPV6_ULA_GUA, &saObj.sin6.sin6_addr);

        EXPECT_EQ(saObj, gua_saObj)
            << "gIF_IPV6_ULA_GUA netaddrp()=\"" << saObj.netaddrp()
            << "\", netadapter netaddrp()=\"" << gua_saObj.netaddrp() << "\"";
        // EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, DEBUG! Maybe 0 for AF_INET6
        //           gua_saObj.family == AF_INET6 ? 64 : 0);
    }

    UpnpFinish();
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

TEST(UpnpapiTestSuite, download_xml_with_loopback_successful) {
    if (!github_actions)
        GTEST_FAIL() << "Still needs to be done.";
}

TEST(UpnpapiTestSuite, download_xml_with_lla_successful) {
    if (!github_actions)
        GTEST_FAIL() << "Still needs to be done.";
}

TEST_F(UpnpapiClearFTestSuite, download_xml_with_gua_successful) {
    if (!nadaptObj.find_first(ADDRS::gua))
        GTEST_SKIP() << "No Global Unicast Address on a local netadapter "
                        "found. Works only with it at time.";

    // The Unit needs a defined state, otherwise it may fail with
    // SEH exception 0xc0000005 on WIN32.
    bWebServerState = WEB_SERVER_DISABLED;

    if (github_actions) // Always disable extended debug messages.
        g_dbug = false; // Will be restored by the tests destructor.
    if (g_dbug)
        // Needed to enable logging for old_code.
        UpnpSetLogFileNames(nullptr, nullptr); // Enable logging to stderr

    int ret_UpnpInit2 = ::UpnpInit2(nadaptObj.name().c_str(), 0);
    ASSERT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    const char sample_dir[] = CMAKE_SOURCE_DIR "/Pupnp/sample/web";
#else
    const char sample_dir[] = CMAKE_SOURCE_DIR "/Sample/web";
#endif
    ASSERT_EQ(::UpnpSetWebServerRootDir(sample_dir), 0);

    // Create an url.
    // std::string url = "http://[::1]:50001/tvdevicedesc.xml";
    // std::string url =
    //    "http://[" + std::string(gIF_IPV6) + "%" + std::to_string(gIF_INDEX) +
    //    "]:" + std::to_string(LOCAL_PORT_V6) + "/tvdevicedesc.xml";
    std::string url = "http://[" + std::string(gIF_IPV6_ULA_GUA) +
                      "]:" + std::to_string(LOCAL_PORT_V6_ULA_GUA) +
                      "/tvdevicedesc.xml";

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

int CallbackEventHandler(Upnp_EventType EventType, const void* Event,
                         [[maybe_unused]] void* Cookie) {

    // Print a summary of the event received
    std::cout << "Received event type \"" << EventType << "\" with event '"
              << Event << "'\n";
    return 0;
}

TEST(UpnpapiTestSuite, UpnpRegisterRootDevice3_with_loopback_successful) {
    if (!github_actions)
        GTEST_FAIL() << "Still needs to be done.";
}

TEST_F(UpnpapiClearFTestSuite, UpnpRegisterRootDevice3_with_gua_successful) {
    if (!nadaptObj.find_first(ADDRS::gua))
        GTEST_SKIP() << "No Global Unicast Address on a local netadapter "
                        "found. Works only with it at time.";

    // The Unit needs a defined state, otherwise it may fail with
    // SEH exception 0xc0000005 on WIN32.
    bWebServerState = WEB_SERVER_DISABLED;

    if (github_actions) // Always disable extended debug messages.
        g_dbug = false; // Will be restored by the tests destructor.
    if (g_dbug)
        // Needed to enable logging for old_code.
        UpnpSetLogFileNames(nullptr, nullptr); // Enable logging to stderr

    int ret_UpnpInit2 = ::UpnpInit2(nadaptObj.name().c_str(), 0);
    ASSERT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    const char sample_dir[] = CMAKE_SOURCE_DIR "/Pupnp/sample/web";
#else
    const char sample_dir[] = CMAKE_SOURCE_DIR "/Sample/web";
#endif
    ASSERT_EQ(::UpnpSetWebServerRootDir(sample_dir), 0);

    // Prepare used local ip address.
    // Example: "http://192.168.24.88:49153/tvdevicedesc.xml"
    std::string desc_doc_url = "http://[" + std::string(gIF_IPV6_ULA_GUA) +
                               "]:" + std::to_string(LOCAL_PORT_V6_ULA_GUA) +
                               "/tvdevicedesc.xml";

    UpnpDevice_Handle device_handle = -1;

    // Test Unit
    int ret_UpnpRegisterRootDevice3 =
        ::UpnpRegisterRootDevice3(desc_doc_url.c_str(), CallbackEventHandler,
                                  &device_handle, &device_handle, AF_INET6);
    EXPECT_EQ(ret_UpnpRegisterRootDevice3, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpRegisterRootDevice3, UPNP_E_SUCCESS);

    EXPECT_GE(device_handle, 1);

    UpnpUnRegisterRootDevice(device_handle);
    UpnpFinish();
}

TEST_F(UpnpapiFTestSuite, UpnpFinish_successful) {
    // Doing needed initializations. Otherwise we get segfaults with
    // UpnpFinish() due to uninitialized pointers.
    // Initialize SDK global mutexes.

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    ASSERT_EQ(UpnpInitMutexes(), UPNP_E_SUCCESS);
    // Initialize the handle list.
    HandleLock(__FILE__, __LINE__);
    for (int i = 0; i < NUM_HANDLE; ++i)
        HandleTable[i] = nullptr;
    HandleUnlock(__FILE__, __LINE__);
#else
    ASSERT_EQ(UpnpInitRwLocks(), UPNP_E_SUCCESS);
    // Initialize the handle list.
    HandleLock();
    for (int i = 0; i < NUM_HANDLE; ++i)
        HandleTable[i] = nullptr;
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

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
    utest::nadaptObj.get_first();
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
