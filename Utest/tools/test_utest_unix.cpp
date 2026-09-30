// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-09-29

#include <utest/utest_unix.hpp>
#include <gtest/gtest.h>

#include <net/if.h>
#include <arpa/inet.h>

namespace utest {

#if 0
TEST(ToolsTestSuite, initialize_ipv4_interface_addresses) {
    CIfaddr4 ifaddr4Obj;
    ifaddrs* ifaddr = ifaddr4Obj.get();

    // should be constructed as empty netinterface address.
    EXPECT_EQ(ifaddr->ifa_next, nullptr);
    EXPECT_STREQ(ifaddr->ifa_name, "");
    EXPECT_EQ(ifaddr->ifa_flags, 0);
    EXPECT_EQ(ifaddr->ifa_addr, nullptr);
    EXPECT_EQ(ifaddr->ifa_netmask, nullptr);
    EXPECT_EQ(ifaddr->ifa_broadaddr, nullptr);
    EXPECT_EQ(ifaddr->ifa_data, nullptr);

    EXPECT_FALSE(ifaddr4Obj.set("", "192.168.168.3/24"));
    EXPECT_FALSE(ifaddr4Obj.set("if0v4", ""));

    // Set a loopback interface
    EXPECT_TRUE(ifaddr4Obj.set("lo", "127.0.0.1"));
#if 0
    [[maybe_unused]] sockaddr_in* ifa_addr_in(reinterpret_cast<sockaddr_in*>(ifaddr->ifa_addr));
    [[maybe_unused]] sockaddr_in* ifa_netmask_in(reinterpret_cast<sockaddr_in*>(ifaddr->ifa_netmask));
    [[maybe_unused]] sockaddr_in* ifa_ifu_in(reinterpret_cast<sockaddr_in*>(ifaddr->ifa_broadaddr));

    // should be constructed as empty netinterface address.
    EXPECT_EQ(ifaddr->ifa_next, nullptr);
    EXPECT_STREQ(ifaddr->ifa_name, "");
    EXPECT_EQ(ifaddr->ifa_flags, 0);
    EXPECT_EQ(ifa_addr_in, nullptr);
    EXPECT_EQ(ifa_netmask_in, nullptr);
    EXPECT_EQ(ifa_ifu_in, nullptr);
    EXPECT_EQ(ifaddr->ifa_data, nullptr);
    // This throws a segfault by C++ and does not need to be tested
    // EXPECT_ANY_THROW(ifaddr4Obj.set(NULL, "192.168.168.3/24"));
    // EXPECT_ANY_THROW(ifaddr4Obj.set("if0v4", NULL));
    EXPECT_FALSE(ifaddr4Obj.set("", "192.168.168.3/24"));
    EXPECT_FALSE(ifaddr4Obj.set("if0v4", ""));

    EXPECT_TRUE(ifaddr4Obj.set("if0v4", "192.168.168.168/20"));
    EXPECT_STREQ(ifaddr->ifa_name, "if0v4");

    char addr4buf[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, &ifa_addr_in->sin_addr.s_addr, addr4buf,
              INET_ADDRSTRLEN);
    EXPECT_STREQ(addr4buf, "192.168.168.168")
        << "    addr4buf contains the ip address";
    inet_ntop(AF_INET, &ifa_netmask_in->sin_addr.s_addr, addr4buf,
              INET_ADDRSTRLEN);
    EXPECT_STREQ(addr4buf, "255.255.240.0")
        << "    addr4buf contains the netmask";
    EXPECT_EQ(ifaddr->ifa_flags,
              (unsigned int)0 | IFF_UP | IFF_BROADCAST | IFF_MULTICAST);
    inet_ntop(AF_INET, &ifa_ifu_in->sin_addr.s_addr, addr4buf, INET_ADDRSTRLEN);
    EXPECT_STREQ(addr4buf, "192.168.175.255")
        << "    addr4buf contains the broadcast address";

    EXPECT_TRUE(ifaddr4Obj.set("if1v4", "10.168.168.200"));
    inet_ntop(AF_INET, &ifa_addr_in->sin_addr.s_addr, addr4buf,
              INET_ADDRSTRLEN);
    EXPECT_STREQ(addr4buf, "10.168.168.200")
        << "    addr4buf contains the ip address";
    inet_ntop(AF_INET, &ifa_netmask_in->sin_addr.s_addr, addr4buf,
              INET_ADDRSTRLEN);
    EXPECT_STREQ(addr4buf, "255.255.255.255")
        << "    addr4buf contains the netmask";
    EXPECT_EQ(ifaddr->ifa_flags,
              (unsigned int)0 | IFF_UP | IFF_BROADCAST | IFF_MULTICAST);
    inet_ntop(AF_INET, &ifa_ifu_in->sin_addr.s_addr, addr4buf, INET_ADDRSTRLEN);
    EXPECT_STREQ(addr4buf, "10.168.168.200")
        << "    addr4buf contains the broadcast address";

    EXPECT_ANY_THROW(ifaddr4Obj.set("if2v4", "10.168.168.47/"));
#endif
}

TEST(ToolsTestSuite, initialize_ipv6_interface_addresses) {
#if 0
    CIfaddr6 ifaddr6Obj;
    ifaddrs* ifaddr = ifaddr6Obj.get();
    sockaddr_in6* ifa_addr_in6 = reinterpret_cast<sockaddr_in6*>(ifaddr->ifa_addr);
    sockaddr_in6* ifa_netmask_in6 = reinterpret_cast<sockaddr_in6*>(ifaddr->ifa_netmask);
    sockaddr_in6* ifa_ifu_in6 = reinterpret_cast<sockaddr_in6*>(ifaddr->ifa_broadaddr);
    // char addr6buf[INET6_ADDRSTRLEN]{};

    // should be constructed with a loopback interface
    EXPECT_EQ(ifaddr->ifa_next, nullptr);
    EXPECT_STREQ(ifaddr->ifa_name, "lo");
    EXPECT_EQ(ifaddr->ifa_flags, (unsigned int)0 | IFF_LOOPBACK | IFF_UP);
    EXPECT_EQ(ifa_addr_in6->sin6_family, AF_INET6);
    // EXPECT_EQ(ifa_addr_in6->sin6_addr.s6_addr, (unsigned int)16777343);
    EXPECT_EQ(ifa_netmask_in6->sin6_family, AF_INET6);
    // EXPECT_EQ(ifa_netmask_in6->sin6_addr.s6_addr, (unsigned int)255);
    EXPECT_EQ(ifa_ifu_in6->sin6_family, AF_INET6);
    // EXPECT_EQ(ifa_ifu_in6->sin6_addr.s6_addr, (unsigned int)0);
    EXPECT_EQ(ifaddr->ifa_data, nullptr);

    // This throws a segfault by C++ and does not need to be tested
    // EXPECT_ANY_THROW(ifaddr6Obj.set(nullptr, "[fe80::5054:ff:fe7f:c021]"));
    // EXPECT_ANY_THROW(ifaddr6Obj.set("if0v4", NULL));
    // EXPECT_FALSE(ifaddr6Obj.set("", "[fe80::5054:ff:fe7f:c021]"));
    // EXPECT_FALSE(ifaddr6Obj.set("if0v6", ""));

    EXPECT_TRUE(ifaddr6Obj.set("if0v4", "192.168.168.168/20"));
    EXPECT_STREQ(ifaddr->ifa_name, "if0v4");
    inet_ntop(AF_INET6, &ifa_addr_in6->sin6_addr.s6_addr, addr6buf,
              INET6_ADDRSTRLEN);
    EXPECT_STREQ(addr6buf, "192.168.168.168")
        << "    addr6buf contains the ip address";
    inet_ntop(AF_INET6, &ifa_netmask_in6->sin6_addr.s6_addr, addr6buf,
              INET6_ADDRSTRLEN);
    EXPECT_STREQ(addr6buf, "255.255.240.0")
        << "    addr6buf contains the netmask";
    EXPECT_EQ(ifaddr->ifa_flags,
              (unsigned int)0 | IFF_UP | IFF_BROADCAST | IFF_MULTICAST);
    inet_ntop(AF_INET6, &ifa_ifu_in6->sin6_addr.s6_addr, addr6buf, INET6_ADDRSTRLEN);
    EXPECT_STREQ(addr6buf, "192.168.175.255")
        << "    addr6buf contains the broadcast address";

    EXPECT_TRUE(ifaddr6Obj.set("if1v4", "10.168.168.200"));
    inet_ntop(AF_INET6, &ifa_addr_in6->sin6_addr.s6_addr, addr6buf,
              INET6_ADDRSTRLEN);
    EXPECT_STREQ(addr6buf, "10.168.168.200")
        << "    addr6buf contains the ip address";
    inet_ntop(AF_INET6, &ifa_netmask_in6->sin6_addr.s6_addr, addr6buf,
              INET6_ADDRSTRLEN);
    EXPECT_STREQ(addr6buf, "255.255.255.255")
        << "    addr6buf contains the netmask";
    EXPECT_EQ(ifaddr->ifa_flags,
              (unsigned int)0 | IFF_UP | IFF_BROADCAST | IFF_MULTICAST);
    inet_ntop(AF_INET6, &ifa_ifu_in6->sin6_addr.s6_addr, addr6buf, INET6_ADDRSTRLEN);
    EXPECT_STREQ(addr6buf, "10.168.168.200")
        << "    addr6buf contains the broadcast address";

    EXPECT_ANY_THROW(ifaddr6Obj.set("if2v4", "10.168.168.47/"));
#endif
}
#endif

TEST(ToolsTestSuite, ifaddr_get_empty_netaddress) {
    CIfaddr ifaddrObj;
    ifaddrs* ifaddr = ifaddrObj.get();

    // should be constructed as empty netinterface address.
    EXPECT_EQ(ifaddr->ifa_next, nullptr);
    EXPECT_STREQ(ifaddr->ifa_name, "");
    EXPECT_EQ(ifaddr->ifa_flags, 0);
    EXPECT_EQ(ifaddr->ifa_addr, nullptr);
    EXPECT_EQ(ifaddr->ifa_netmask, nullptr);
    EXPECT_EQ(ifaddr->ifa_broadaddr, nullptr);
    EXPECT_EQ(ifaddr->ifa_data, nullptr);
}

} // namespace utest

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
