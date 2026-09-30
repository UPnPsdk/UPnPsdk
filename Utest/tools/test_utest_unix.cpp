// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-10-06

#include <utest/utest_unix.hpp>
#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <bitset>
#ifdef __APPLE__
#include <net/if.h>   // To be portable, use general POSIX standard header.
#else
#include <linux/if.h> // For linux specific IFF_* flags like IFF_LOWER_UP.
                      // Don't use it for portable program code.
#endif

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


class UtestHelperFTestSuite : public ::testing::Test {
  protected:
    ::ifaddrs* m_ifaddr{};

    // Destructor
    ~UtestHelperFTestSuite() {
        if (m_ifaddr != nullptr)
            ::freeifaddrs(m_ifaddr);
    }
};


TEST(UtestHelperTestSuite, ifaddr_set_empty_netaddress) {
    CIfaddr ifaddrObj;
    ifaddrs* ifaddr = ifaddrObj.get();

    // should be constructed as empty netinterface address.
    EXPECT_EQ(ifaddr->ifa_next, nullptr);
    EXPECT_EQ(ifaddr->ifa_flags, 0);
    EXPECT_STREQ(ifaddr->ifa_name, "");
    EXPECT_EQ(ifaddr->ifa_flags, 0);
    EXPECT_EQ(ifaddr->ifa_addr, nullptr);
    EXPECT_EQ(ifaddr->ifa_netmask, nullptr);
    EXPECT_EQ(ifaddr->ifa_broadaddr, nullptr);
    EXPECT_EQ(ifaddr->ifa_data, nullptr);
}

TEST(UtestHelperTestSuite, ifaddr_set_ipv6_loopback_netaddress) {
    // Test Unit
    CIfaddr ifaddrObj(1, "lo", "[::1]", IFF_UP | IFF_BROADCAST | IFF_MULTICAST);
    ifaddrs* ifaddr = ifaddrObj.get();

    EXPECT_EQ(ifaddr->ifa_next, nullptr);
    EXPECT_STREQ(ifaddr->ifa_name, "lo");
    EXPECT_EQ(ifaddr->ifa_flags, 0 | IFF_UP | IFF_BROADCAST | IFF_MULTICAST);
    ASSERT_EQ(ifaddr->ifa_addr->sa_family, AF_INET6);

    ::sockaddr_in6* sin6;
    // Check netinterface address
    sin6 = reinterpret_cast<::sockaddr_in6*>(ifaddr->ifa_addr);
    EXPECT_TRUE(IN6_IS_ADDR_LOOPBACK(&sin6->sin6_addr));
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, 0);

    // Check netinterface netmask
    sin6 = reinterpret_cast<::sockaddr_in6*>(ifaddr->ifa_netmask);
    uint64_t* sin6_64 = reinterpret_cast<uint64_t*>(&sin6->sin6_addr);
    EXPECT_EQ(sin6_64[0], 0xffffffffffffffff);
    EXPECT_EQ(sin6_64[1], 0xffffffffffffffff);
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, 0);
}

TEST(UtestHelperTestSuite, ifaddr_set_wrong_ipv6_netaddress_fails) {
    // Test Unit
#ifndef __APPLE__
    EXPECT_THROW(CIfaddr ifaddrObj(1, "lo1", "[::1%1]",
                                   IFF_UP | IFF_BROADCAST | IFF_MULTICAST),
                 std::invalid_argument);
#endif
    EXPECT_THROW(CIfaddr ifaddrObj(1, "lo2", "[::1]:50001",
                                   IFF_UP | IFF_BROADCAST | IFF_MULTICAST),
                 std::invalid_argument);
    EXPECT_THROW(CIfaddr ifaddrObj(1, "lo3", "[::1]/64",
                                   IFF_UP | IFF_BROADCAST | IFF_MULTICAST),
                 std::invalid_argument);
}

TEST_F(UtestHelperFTestSuite, ifaddr_set_ipv4_loopback_netaddress_syscall) {
    // Test Unit
    ifaddrs* ifa;
    ASSERT_EQ(getifaddrs(&m_ifaddr), 0);

    sockaddr_in* sin;
    for (ifa = m_ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr)
            continue;
        sin = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
        // Check if IPv4 is_loopback address.
        if (ifa->ifa_addr->sa_family == AF_INET &&
            // address between "127.0.0.0" and "127.255.255.255" binary.
            ntohl(sin->sin_addr.s_addr) >= 2130706432 &&
            ntohl(sin->sin_addr.s_addr) <= 2147483647)
            break;
    }
    if (ifa == nullptr) {
        GTEST_SKIP() << "No IPv4 Loopback Address found for testing.";
    }

    EXPECT_STRNE(ifa->ifa_name, "");
    // IFF_LOWER_UP is a special Linux flag with header 'linux/if.h' instead of
    // 'net/if.h'. This flag isn't set on MacOS. Instead the previous flag
    // 0x8000 is set and on linux it is named IFF_DYNAMIC but this name isn't
    // available on MacOS. MacOS has only a portable 'net/if.h' header. Don't
    // know what this bit on MacOS means and what it's name.
#ifdef __APPLE__
    EXPECT_EQ(ifa->ifa_flags,
              0 | 0x8000 /*IFF_DYNAMIC?*/ | IFF_RUNNING | IFF_LOOPBACK | IFF_UP)
    // EXPECT_EQ(ifa->ifa_flags, 0b00000000000000001000000001001001);
#else
    // EXPECT_EQ(ifa->ifa_flags, 0b00000000000000010000000001001001);
    EXPECT_EQ(ifa->ifa_flags,
              0 | IFF_LOWER_UP | IFF_RUNNING | IFF_LOOPBACK | IFF_UP)
#endif
        << "Wrong flags " << ifa->ifa_flags << " = 0b"
        << std::bitset<8 * sizeof(ifa->ifa_flags)>(ifa->ifa_flags) << ".\n";

    // Check netinterface address.
    char addr4buf[INET_ADDRSTRLEN];
    ASSERT_EQ(inet_ntop(AF_INET, &sin->sin_addr, addr4buf, INET_ADDRSTRLEN),
              addr4buf);
    EXPECT_STREQ(addr4buf, "127.0.0.1");
    EXPECT_EQ(sin->sin_port, 0);

    // Check netinterface netmask.
    ASSERT_EQ(ifa->ifa_netmask->sa_family, AF_INET);
    sin = reinterpret_cast<sockaddr_in*>(ifa->ifa_netmask);
    ASSERT_EQ(inet_ntop(AF_INET, &sin->sin_addr, addr4buf, INET_ADDRSTRLEN),
              addr4buf);
    EXPECT_STREQ(addr4buf, "255.0.0.0");
    EXPECT_EQ(sin->sin_port, 0);

    // Check netinterface broadcast address.
    ASSERT_EQ(ifa->ifa_broadaddr->sa_family, AF_INET);
    sin = reinterpret_cast<sockaddr_in*>(ifa->ifa_broadaddr);
    ASSERT_EQ(inet_ntop(AF_INET, &sin->sin_addr, addr4buf, INET_ADDRSTRLEN),
              addr4buf);
    EXPECT_STREQ(addr4buf, "127.0.0.1");
    EXPECT_EQ(sin->sin_port, 0);

    EXPECT_EQ(ifa->ifa_data, nullptr);
}

TEST(UtestHelperTestSuite, ifaddr_set_ipv4_loopback_netaddress) {
#ifdef __APPLE__
    unsigned int iff_flags =
        0x8000 /*IFF_DYNAMIC?*/ | IFF_RUNNING | IFF_LOOPBACK | IFF_UP;
#else
    unsigned int iff_flags = IFF_LOWER_UP | IFF_RUNNING | IFF_LOOPBACK | IFF_UP;
#endif

    // Test Unit
    CIfaddr ifaddrObj(1, "lo", "127.0.0.1", iff_flags, 8);
    ifaddrs* ifaddr = ifaddrObj.get();

    EXPECT_EQ(ifaddr->ifa_next, nullptr);
    EXPECT_STREQ(ifaddr->ifa_name, "lo");
    EXPECT_EQ(ifaddr->ifa_flags, 0 | iff_flags);
    ASSERT_EQ(ifaddr->ifa_addr->sa_family, AF_INET);

    char addr4buf[INET_ADDRSTRLEN]{};
    ::sockaddr_in* sin;

    // Check netinterface address
    sin = reinterpret_cast<::sockaddr_in*>(ifaddr->ifa_addr);
    ASSERT_EQ(inet_ntop(AF_INET, &sin->sin_addr, addr4buf, INET_ADDRSTRLEN),
              addr4buf);
    EXPECT_STREQ(addr4buf, "127.0.0.1");
    EXPECT_EQ(sin->sin_port, 0);

    // Check netinterface netmask
    ASSERT_EQ(ifaddr->ifa_netmask->sa_family, AF_INET);
    sin = reinterpret_cast<::sockaddr_in*>(ifaddr->ifa_netmask);
    ASSERT_EQ(inet_ntop(AF_INET, &sin->sin_addr, addr4buf, INET_ADDRSTRLEN),
              addr4buf);
    EXPECT_STREQ(addr4buf, "255.0.0.0");
    EXPECT_EQ(sin->sin_port, 0);

    // Check netinterface broadcast address
    // ASSERT_EQ(ifaddr->ifa_broadaddr->sa_family, AF_INET);
    // sin = reinterpret_cast<::sockaddr_in*>(ifaddr->ifa_broadaddr);
    // ASSERT_EQ(inet_ntop(AF_INET, &sin->sin_addr, addr4buf, INET_ADDRSTRLEN),
    //           addr4buf);
    // EXPECT_STREQ(addr4buf, "127.0.0.0");
    // EXPECT_EQ(sin->sin_port, 0);

    EXPECT_EQ(ifaddr->ifa_data, nullptr);
}

} // namespace utest

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
