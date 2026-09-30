// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-09-27

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


// UpnpApi network interfaces TestSuite
//=====================================

class UpnpapiFTestSuite : public ::testing::Test {
  protected:
    // constructor
    UpnpapiFTestSuite() {
        // initialize needed global variables
        std::fill(std::begin(gIF_NAME), std::end(gIF_NAME), 0);
        gIF_INDEX = ~0u;
        std::fill(std::begin(gIF_IPV4), std::end(gIF_IPV4), 0);
        std::fill(std::begin(gIF_IPV4_NETMASK), std::end(gIF_IPV4_NETMASK), 0);
        std::fill(std::begin(gIF_IPV6), std::end(gIF_IPV6), 0);
        gIF_IPV6_PREFIX_LENGTH = 0;
        std::fill(std::begin(gIF_IPV6_ULA_GUA), std::end(gIF_IPV6_ULA_GUA), 0);
        gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
        UpnpSdkInit = 0xAA; // This should not be used and modified here
    }
};

#if 0
TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_empty_netifname_fails) {
    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo("");
    EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);

    // Check if UpnpSdkInit has been modified
    EXPECT_EQ(UpnpSdkInit, 0xAA);
    EXPECT_STREQ(gIF_NAME, "");
    EXPECT_STREQ(gIF_IPV4, "");
    EXPECT_STREQ(gIF_IPV4_NETMASK, "");
    EXPECT_STREQ(gIF_IPV6, "");
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
    std::cout << CYEL "[    FIX   ] " CRES << __LINE__
              << ": Netadapter Index must be 0, but not untouched with high "
                 "number.\n";
    // EXPECT_EQ(gIF_INDEX, 0);
    EXPECT_EQ(gIF_INDEX, ~0u); // Wrong!
    EXPECT_EQ(LOCAL_PORT_V4, 0);
    EXPECT_EQ(LOCAL_PORT_V6, 0);
    EXPECT_EQ(LOCAL_PORT_V6_ULA_GUA, 0);
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_unknown_netifname_fails) {
    // Test Unit
    constexpr char ifname[]{"ZaybX9"};
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo(ifname);
    EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_INVALID_INTERFACE);

    // Check if UpnpSdkInit has been modified
    EXPECT_EQ(UpnpSdkInit, 0xAA);
    std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
              << ": Unknown Netadapter name=\"" << ifname
              << "\" must not be set to be program global available with empty "
                 "info.\n";
    // EXPECT_STREQ(gIF_NAME, "");
    EXPECT_STREQ(gIF_NAME, ifname); // Wrong!
    EXPECT_STREQ(gIF_IPV4, "");
    EXPECT_STREQ(gIF_IPV4_NETMASK, "");
    EXPECT_STREQ(gIF_IPV6, "");
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
    std::cout << CYEL "[    FIX   ] " CRES << __LINE__
              << ": Netadapter Index must be 0, but not untouched with high "
                 "number.\n";
    // EXPECT_EQ(gIF_INDEX, 0);
    EXPECT_EQ(gIF_INDEX, ~0u); // Wrong!
    EXPECT_EQ(LOCAL_PORT_V4, 0);
    EXPECT_EQ(LOCAL_PORT_V6, 0);
    EXPECT_EQ(LOCAL_PORT_V6_ULA_GUA, 0);
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_ip4) {
    // provide a network interface
    CIfaddr4 ifaddr4Obj;
    ifaddr4Obj.set("if0v4", "192.168.99.3/11");
    ifaddrs* ifaddr = ifaddr4Obj.get();
    EXPECT_STREQ(ifaddr->ifa_name, "if0v4");

    // Mock system functions
    umock::IfaddrsMock ifaddrsObj;
    umock::Ifaddrs ifaddrs_injectObj(&ifaddrsObj);
    umock::Net_ifMock net_ifObj;
    umock::Net_if net_if_injectObj(&net_ifObj);
    EXPECT_CALL(ifaddrsObj, getifaddrs(_))
        .WillOnce(DoAll(SetArgPointee<0>(ifaddr), Return(0)));
    EXPECT_CALL(ifaddrsObj, freeifaddrs(ifaddr)).Times(1);
    EXPECT_CALL(net_ifObj, if_nametoindex(_))
        .Times(old_code ? 1 : 2)
        .WillRepeatedly(Return(2));

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
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
    // index mocked with if_nametoindex above
    EXPECT_EQ(gIF_INDEX, 2);
    EXPECT_EQ(LOCAL_PORT_V4, 0);
    EXPECT_EQ(LOCAL_PORT_V6, 0);
    EXPECT_EQ(LOCAL_PORT_V6_ULA_GUA, 0);
}

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_called_with_unknown_interface) {
    // provide a network interface
    CIfaddr4 ifaddr4Obj;
    ifaddr4Obj.set("eth0", "192.168.77.48/22");
    ifaddrs* ifaddr = ifaddr4Obj.get();
    EXPECT_STREQ(ifaddr->ifa_name, "eth0");

    // Mock system functions
    umock::IfaddrsMock ifaddrsObj;
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

TEST_F(UpnpapiFTestSuite, UpnpGetIfInfo_best_choise_from_operating_system) {
    // nullptr as argument finds the first suitable interface for operation.
    // Test Unit
    int ret_UpnpGetIfInfo = ::UpnpGetIfInfo(nullptr);
    EXPECT_EQ(ret_UpnpGetIfInfo, UPNP_E_SUCCESS)
        << errStrEx(ret_UpnpGetIfInfo, UPNP_E_SUCCESS);

    // Check if UpnpSdkInit has been modified
    EXPECT_EQ(UpnpSdkInit, 0xAA);
    // gIF_NAME mocked with getifaddrs above
    EXPECT_STREQ(gIF_NAME, "");
    // gIF_IPV4 mocked with getifaddrs above
    EXPECT_EQ(gIF_IPV4[0], '\0');
    EXPECT_EQ(gIF_IPV4_NETMASK[0], '\0');
    EXPECT_EQ(gIF_IPV6[0], '\0');
    EXPECT_EQ(gIF_IPV6_PREFIX_LENGTH, 0);
    EXPECT_EQ(gIF_IPV6_ULA_GUA[0], '\0');
    EXPECT_EQ(gIF_IPV6_ULA_GUA_PREFIX_LENGTH, 0);
    std::cout << CYEL "[    FIX   ] " CRES << __LINE__
              << ": Netadapter Index must be 0, but not unspecified.\n";
    EXPECT_EQ(gIF_INDEX, ~0u); // Wrong!
    EXPECT_EQ(LOCAL_PORT_V4, 0);
    EXPECT_EQ(LOCAL_PORT_V6, 0);
    EXPECT_EQ(LOCAL_PORT_V6_ULA_GUA, 0);
}
#endif

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
