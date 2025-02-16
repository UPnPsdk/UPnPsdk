// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-02-12

// Mock network interfaces
// For further information look at https://stackoverflow.com/a/66498073/5014688

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
#include <Pupnp/upnp/src/api/upnpapi.cpp>
#else
#include <Compa/src/api/upnpapi.cpp>
#endif

#include <UPnPsdk/upnptools.hpp> // For ErrStrEx

#include <utest/utest.hpp>
#include <utest/utest_unix.hpp>
#include <umock/ifaddrs_mock.hpp>
#include <umock/net_if_mock.hpp>
#include <umock/sys_socket_mock.hpp>


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
class UpnpapiFTestSuite : public ::testing::Test {
  protected:
    // constructor of this testsuite
    UpnpapiFTestSuite() {
        // initialize needed global variables
        std::fill(std::begin(gIF_NAME), std::end(gIF_NAME), 0);
        std::fill(std::begin(gIF_IPV4), std::end(gIF_IPV4), 0);
        std::fill(std::begin(gIF_IPV4_NETMASK), std::end(gIF_IPV4_NETMASK), 0);
        std::fill(std::begin(gIF_IPV6), std::end(gIF_IPV6), 0);
        gIF_IPV6_PREFIX_LENGTH = 0;
        std::fill(std::begin(gIF_IPV6_ULA_GUA), std::end(gIF_IPV6_ULA_GUA), 0);
        gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
        gIF_INDEX = (unsigned)-1;
        UpnpSdkInit = 0xAA; // This should not be used and modified here
    }
};

class UpnpapiIPv4MockTestSuite : public UpnpapiFTestSuite {
    // Fixtures for this Testsuite
  protected:
    umock::IfaddrsMock ifaddrsObj;
};


TEST_F(UpnpapiIPv4MockTestSuite, UpnpGetIfInfo_called_with_valid_interface) {
    // provide a network interface
    CIfaddr4 ifaddr4Obj;
    ifaddr4Obj.set("if0v4", "192.168.99.3/11");
    ifaddrs* ifaddr = ifaddr4Obj.get();
    EXPECT_STREQ(ifaddr->ifa_name, "if0v4");

    // Mock system functions
    umock::Ifaddrs ifaddrs_injectObj(&ifaddrsObj);
    umock::Net_ifMock net_ifObj;
    umock::Net_if net_if_injectObj(&net_ifObj);
    EXPECT_CALL(ifaddrsObj, getifaddrs(_))
        .WillOnce(DoAll(SetArgPointee<0>(ifaddr), Return(0)));
    EXPECT_CALL(ifaddrsObj, freeifaddrs(ifaddr)).Times(1);
    EXPECT_CALL(net_ifObj, if_nametoindex(_)).WillOnce(Return(2));

    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo("if0v4");
    EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

    // Check if UpnpSdkInit has been modified
    EXPECT_EQ(UpnpSdkInit, 0xAA);

    // gIF_NAME mocked with getifaddrs above
    EXPECT_STREQ(gIF_NAME, "if0v4");
    // gIF_IPV4 mocked with getifaddrs above
    EXPECT_STREQ(gIF_IPV4, "192.168.99.3");
    // EXPECT_THAT(gIF_IPV4,
    // MatchesRegex("[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}"));
    EXPECT_STREQ(gIF_IPV4_NETMASK, "255.224.0.0");
    EXPECT_STREQ(gIF_IPV6, "");
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, (unsigned)0);
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, (unsigned)0);
    // index mocked with if_nametoindex above
    EXPECT_EQ(gIF_INDEX, (unsigned int)2);
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

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_called_with_loopback_interface) {
    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo("::1");

    if (old_code) {
        ASSERT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
            << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);
        GTEST_SKIP()
            << "Using the local network loopback interface is not supported.";
    }

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

TEST_F(UpnpapiIPv4MockTestSuite, UpnpGetIfInfo_called_with_unknown_interface) {
    // provide a network interface
    CIfaddr4 ifaddr4Obj;
    ifaddr4Obj.set("eth0", "192.168.77.48/22");
    ifaddrs* ifaddr = ifaddr4Obj.get();
    EXPECT_STREQ(ifaddr->ifa_name, "eth0");

    umock::Ifaddrs ifaddrs_injectObj(&ifaddrsObj);
    umock::Net_ifMock net_ifObj;
    umock::Net_if net_if_injectObj(&net_ifObj);
    EXPECT_CALL(ifaddrsObj, getifaddrs(_))
        .WillOnce(DoAll(SetArgPointee<0>(ifaddr), Return(0)));
    EXPECT_CALL(ifaddrsObj, freeifaddrs(ifaddr)).Times(1);
    EXPECT_CALL(net_ifObj, if_nametoindex(_)).Times(0);

    // Test Unit
    // "ATTENTION! There is a wrong upper case 'O', not zero in 'ethO'";
    int ret_UpnpGetIfInfo = UpnpGetIfInfo("ethO");
    EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);

    // Check if UpnpSdkInit has been modified
    EXPECT_EQ(UpnpSdkInit, 0xAA);

    if (old_code) {
        std::cout
            << CYEL "[ BUGFIX   ] " CRES << __LINE__
            << ": interface name (e.g. ethO with upper case O), ip "
            << "address and netmask should not be modified on wrong entries.\n";
        // gIF_NAME mocked with getifaddrs above
        // ATTENTION! There is a wrong upper case 'O', not zero in "ethO";
        EXPECT_STREQ(gIF_NAME, "ethO");
        // gIF_IPV4 with "192.68.77.48/22" mocked by getifaddrs above
        // but get an empty ip address from initialization. That's OK.
        EXPECT_STREQ(gIF_IPV4, "");
        // get an empty netmask from initialization. That's also OK.
        EXPECT_STREQ(gIF_IPV4_NETMASK, "");

    } else {

        // gIF_NAME mocked with getifaddrs above
        EXPECT_STREQ(gIF_NAME, "")
            << "  # ATTENTION! There is a wrong upper case 'O', not zero in "
               "\"ethO\".\n"
            << "  # Interface name should not be modified on wrong entries.";
        // gIF_IPV4 mocked with getifaddrs above
        EXPECT_STREQ(gIF_IPV4, "")
            << "  # Ip address should not be modified on wrong entries.";
        EXPECT_STREQ(gIF_IPV4_NETMASK, "")
            << "  # Netmask should not be modified on wrong entries.";
    }

    EXPECT_STREQ(gIF_IPV6, "");
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, (unsigned)0);
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, (unsigned)0);
    // index mocked with if_nametoindex above
    EXPECT_EQ(gIF_INDEX, (unsigned)4294967295) << "    Which is: (unsigned)-1";
    EXPECT_EQ(LOCAL_PORT_V4, (unsigned short)0);
    EXPECT_EQ(LOCAL_PORT_V6, (unsigned short)0);
    EXPECT_EQ(LOCAL_PORT_V6_ULA_GUA, (unsigned short)0);
}

// This is too complex to mock all deep detailed system calls. Rework it when I
// can mock GetIfInfo().
//
TEST_F(UpnpapiIPv4MockTestSuite, UpnpInit2_successful) {
    if (!github_actions)
        GTEST_FAIL()
            << "This test must be reworked with mocking UpnpGetIfInfo().";
#if 0
    // CLogging logObj; // Output only with build type DEBUG.
    // logObj.enable(UPNP_ALL);

    // provide a network interface
    CIfaddr4 ifaddr4Obj;
    ifaddr4Obj.set("if0v4", "192.168.99.3/20");
    ifaddrs* ifaddr = ifaddr4Obj.get();
    EXPECT_STREQ(ifaddr->ifa_name, "if0v4");
    constexpr SOCKET listen_sockfd{445};
    constexpr SOCKET stop_sockfd{446};

    // Mock to get local network interface address and its index number
    umock::Ifaddrs ifaddrs_injectObj(&ifaddrsObj);
    umock::Net_ifMock net_ifObj;
    umock::Net_if net_if_injectObj(&net_ifObj);
    EXPECT_CALL(ifaddrsObj, getifaddrs(_))
        .WillOnce(DoAll(SetArgPointee<0>(ifaddr), Return(0)));
    EXPECT_CALL(ifaddrsObj, freeifaddrs(ifaddr)).Times(1);
    EXPECT_CALL(net_ifObj, if_nametoindex(_)).Times(1);

    // Mock socket, bind local ip address to it and listen to it
    umock::Sys_socketMock sys_socketObj;
    umock::Sys_socket sys_socket_injectObj(&sys_socketObj);
    // Provide listen socket
    EXPECT_CALL(sys_socketObj, socket(AF_INET, SOCK_STREAM, 0))
        .WillOnce(Return(listen_sockfd));
    EXPECT_CALL(sys_socketObj, bind(listen_sockfd, _, _)).Times(1);
    EXPECT_CALL(sys_socketObj, listen(listen_sockfd, SOMAXCONN)).Times(1);
    EXPECT_CALL(sys_socketObj, getsockname(listen_sockfd, _, _)).Times(1);
    // Provide stop socket
    EXPECT_CALL(sys_socketObj, socket(AF_INET, SOCK_DGRAM, 0))
        .WillOnce(Return(stop_sockfd));
    EXPECT_CALL(sys_socketObj, bind(stop_sockfd, _, _)).Times(1);
    EXPECT_CALL(sys_socketObj, getsockname(stop_sockfd, _, _)).Times(1);

    // Test Unit
    UpnpSdkInit = 0;
    int ret_UpnpInit2 = UpnpInit2(nullptr, 0);
    EXPECT_EQ(ret_UpnpInit2, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpInit2, UPNP_E_SUCCESS);

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
    return gtest_return_code; // managed in gtest_main.inc
}
