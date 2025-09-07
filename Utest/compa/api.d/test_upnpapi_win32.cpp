// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-09-14

// Mock network interfaces
// For further information look at https://stackoverflow.com/a/66498073/5014688

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
#include <Pupnp/upnp/src/api/upnpapi.cpp>
#else
#include <Compa/src/api/upnpapi.cpp>
#endif

#include <UPnPsdk/upnptools.hpp> // For errStrEx

#include <utest/utest.hpp>
#include <utest/utest_win32.hpp>
#include <umock/iphlpapi_mock.hpp>


namespace utest {

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;

using ::UPnPsdk::errStrEx;


// UpnpApi Testsuite for IP4
//==========================

// This TestSuite is with instantiating mocks
//-------------------------------------------
class UpnpapiIPv4MockTestSuite : public ::testing::Test
// Fixtures for this Testsuite
{
  protected:
    // constructor of this testsuite
    UpnpapiIPv4MockTestSuite() {
        // initialize needed global variables
        memset(&gIF_NAME, 0, sizeof(gIF_NAME));
        memset(&gIF_IPV4, 0, sizeof(gIF_IPV4));
        memset(&gIF_IPV4_NETMASK, 0, sizeof(gIF_IPV4_NETMASK));
        memset(&gIF_IPV6, 0, sizeof(gIF_IPV6));
        gIF_IPV6_PREFIX_LENGTH = 0;
        memset(&gIF_IPV6_ULA_GUA, 0, sizeof(gIF_IPV6_ULA_GUA));
        gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
        gIF_INDEX = (unsigned)-1;
        UpnpSdkInit = 0xAA; // This should not be used and modified here
    }
};

TEST_F(UpnpapiIPv4MockTestSuite, UpnpGetIfInfo_called_with_valid_interface) {
    // provide a network interface
    CNetIf4 ifaddr4Obj{};
    ifaddr4Obj.set(L"if0v4", "192.168.99.3/11");
    ifaddr4Obj.set_ifindex(2);
    ::PIP_ADAPTER_ADDRESSES adapts = ifaddr4Obj.get();
    EXPECT_STREQ(adapts->FriendlyName, L"if0v4");
    ::ULONG Size = 16383;

    // Mock system functions
    umock::IphlpapiMock iphlpapiObj;
    umock::Iphlpapi iphlpapi_injectObj(&iphlpapiObj);
    EXPECT_CALL(iphlpapiObj, GetAdaptersAddresses(_, _, _, _, _))
        .Times(2)
        .WillOnce(
            DoAll(SetArgPointee<4>(*&Size), Return(ERROR_BUFFER_OVERFLOW)))
        .WillOnce(DoAll(SetArgPointee<3>(*adapts), Return(ERROR_SUCCESS)));

    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo("if0v4");
    EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

    // Check if UpnpSdkInit has been modified
    EXPECT_EQ(UpnpSdkInit, 0xAA);

    // Check results
    EXPECT_STREQ(gIF_NAME, "if0v4");
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    EXPECT_STREQ(gIF_IPV4, "192.168.99.3");
#else
    EXPECT_STREQ(gIF_IPV4, ""); // Not supported on new code
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "::ffff:192.168.99.3");
#endif

    if (old_code) {
        EXPECT_STREQ(gIF_IPV4_NETMASK, "255.224.0.0");
        std::cout << CRED "[ BUG      ] " CRES << __LINE__
                  << ": gIF_IPV6 must not be set to any floating ip address "
                     "when IPv6 is disabled.\n";
        EXPECT_STRNE(gIF_IPV6, "");
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0u);

    } else {

        EXPECT_STREQ(gIF_IPV4_NETMASK, ""); // Not supported on new code
        EXPECT_STREQ(gIF_IPV6, "");
        EXPECT_STREQ(gIF_IPV6_ULA_GUA, "::ffff:192.168.99.3");
        EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 96u);
    }

    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0U);
    EXPECT_EQ(gIF_INDEX, 2U);
    EXPECT_EQ(LOCAL_PORT_V4, (unsigned short)0);
    EXPECT_EQ(LOCAL_PORT_V6, (unsigned short)0);
    EXPECT_EQ(LOCAL_PORT_V6_ULA_GUA, (unsigned short)0);
}

TEST_F(UpnpapiIPv4MockTestSuite, UpnpGetIfInfo_called_with_loopback_interface) {
    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo("::1");

    if (old_code) {
        // Not supported
        EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);

    } else if (!github_actions) {

        EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);
    }
}

TEST_F(UpnpapiIPv4MockTestSuite, UpnpGetIfInfo_called_with_unknown_interface) {
    // provide a valid network interface
    CNetIf4 ifaddr4Obj{};
    ifaddr4Obj.set(L"eth0", "192.168.77.48/22");
    ifaddr4Obj.set_ifindex(2);
    ::PIP_ADAPTER_ADDRESSES adapts = ifaddr4Obj.get();
    EXPECT_STREQ(adapts->FriendlyName, L"eth0");
    ::ULONG Size = 16383;

    umock::IphlpapiMock iphlpapiObj;
    umock::Iphlpapi iphlpapi_injectObj(&iphlpapiObj);
    EXPECT_CALL(iphlpapiObj, GetAdaptersAddresses(_, _, _, _, _))
        .Times(4)
        .WillOnce(
            DoAll(SetArgPointee<4>(*&Size), Return(ERROR_BUFFER_OVERFLOW)))
        .WillOnce(DoAll(SetArgPointee<3>(*adapts), Return(ERROR_SUCCESS)))
        .WillOnce(
            DoAll(SetArgPointee<4>(*&Size), Return(ERROR_BUFFER_OVERFLOW)))
        .WillOnce(DoAll(SetArgPointee<3>(*adapts), Return(ERROR_SUCCESS)));

    // Test Unit
    // First set a valid interface
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo("eth0");
    EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);
    // Then ask again
    // "ATTENTION! There is a wrong upper case 'O', not zero in 'ethO'";
    ret_UpnpGetIfInfo = ::UpnpGetIfInfo("ethO");
    EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);

    // Check if UpnpSdkInit has been modified
    EXPECT_EQ(UpnpSdkInit, 0xAA);

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": interface name (e.g. ethO with upper case O), ip "
                  << "address should not be modified on wrong entries.\n";
        // ATTENTION! There is a wrong upper case 'O', not zero in "ethO";
        EXPECT_STREQ(gIF_NAME, "ethO");
        EXPECT_STREQ(gIF_IPV4_NETMASK, "255.255.252.0");
        std::cout << CRED "[ BUG      ] " CRES << __LINE__
                  << ": gIF_IPV6 must not be set to any floating ip "
                     "address when IPv6 is disabled.\n";
        EXPECT_STRNE(gIF_IPV6, "");

    } else {

        // There is a zero in the valid name. It is a preset name that should
        // not be modified.
        EXPECT_STREQ(gIF_NAME, "eth0")
            << "  BUG! Interface name (e.g. ethO with upper case O), ip "
            << "address should not be modified on wrong entries.";
        if (!github_actions) {
            EXPECT_STREQ(gIF_IPV4_NETMASK, "255.255.252.0");
            EXPECT_STREQ(gIF_IPV6, "");
        }
    }

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    EXPECT_STREQ(gIF_IPV4, "192.168.77.48");
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0u);
#else
    EXPECT_STREQ(gIF_IPV4, ""); // Not supported on new code
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "::ffff:192.168.77.48");
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 96u);
#endif
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0U);
    EXPECT_EQ(gIF_INDEX, 2U);
    EXPECT_EQ(LOCAL_PORT_V4, (unsigned short)0);
    EXPECT_EQ(LOCAL_PORT_V6, (unsigned short)0);
    EXPECT_EQ(LOCAL_PORT_V6_ULA_GUA, (unsigned short)0);
}

// This is too complex to mock all deep detailed system calls. Rework it when I
// can mock UpnpGetIfInfo().
//
TEST_F(UpnpapiIPv4MockTestSuite, initialize_default_UpnpInit2) {
    if (!github_actions)
        GTEST_FAIL()
            << "This test must be reworked with mocking UpnpGetIfInfo().";
#if 0
    // provide a network interface
    CNetIf4 ifaddr4Obj{};
    ifaddr4Obj.set(L"if0v4", "192.168.99.3/20");
    ifaddr4Obj.set_ifindex(2);
    ::PIP_ADAPTER_ADDRESSES adapts = ifaddr4Obj.get();
    EXPECT_STREQ(adapts->FriendlyName, L"if0v4");
    ::ULONG Size = 16383;

    // expect calls to system functions (which are mocked)
    umock::IphlpapiMock iphlpapiObj;
    umock::Iphlpapi iphlpapi_injectObj(&iphlpapiObj);
    EXPECT_CALL(iphlpapiObj, GetAdaptersAddresses(_, _, _, _, _))
        .Times(2)
        .WillOnce(
            DoAll(SetArgPointee<4>(*&Size), Return(ERROR_BUFFER_OVERFLOW)))
        .WillOnce(DoAll(SetArgPointee<3>(*adapts), Return(ERROR_SUCCESS)));

    // Initialize capturing of the stderr output
    CaptureStdOutErr captureObj(UPnPsdk::log_fileno);
    captureObj.start();

    // Test Unit
    UpnpSdkInit = 0;
    int ret_UpnpInit2 = UpnpInit2(nullptr, 0);
    EXPECT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

    // Get and check the captured data
    EXPECT_EQ(captureObj.str(), "")
        << "  There should not be any output to stderr.";

    EXPECT_EQ(UpnpSdkInit, 1);

    // call the unit again to check if it returns to be already initialized
    ret_UpnpInit2 = UpnpInit2(nullptr, 0);
    EXPECT_EQ(ret_UpnpInit2, UPNP_E_INIT)
        << errStrEx(ret_UpnpInit2, UPNP_E_INIT);
    EXPECT_EQ(UpnpSdkInit, 1);

    // Finish library
    int ret_UpnpFinish = UpnpFinish();
    EXPECT_EQ(ret_UpnpFinish, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpFinish, UPNP_E_SUCCESS);
    EXPECT_EQ(UpnpSdkInit, 0);

    // Finish library again
    ret_UpnpFinish = UpnpFinish();
    EXPECT_EQ(ret_UpnpFinish, UPNP_E_FINISH)
        << errStrEx(ret_UpnpFinish, UPNP_E_FINISH);
    EXPECT_EQ(UpnpSdkInit, 0);
#endif
}

} // namespace utest

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in utest/utest_main.inc
}
