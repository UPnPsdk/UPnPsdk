// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-10-26

#include <utest/utest_unix.hpp>
#include <utest/utest.hpp>

#include <arpa/inet.h>
#include <bitset>
#include <net/if.h> // To be portable, use only general POSIX standard header
                    // and not linux/if.h.

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

TEST_F(UtestHelperFTestSuite, ifaddr_set_ipv6_loopback_netaddress) {
    // Get real network interface address information and emulate the system
    // call with same results.

    ifaddrs* ifa;
    ASSERT_EQ(getifaddrs(&m_ifaddr), 0);

    // Search a loopback address.
    ::sockaddr_in6* sin6{};
    for (ifa = m_ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr)
            continue;
        // Check if IPv6 is_loopback address.
        if (ifa->ifa_addr->sa_family == AF_INET6 &&
            (ifa->ifa_flags & IFF_LOOPBACK)) {
            sin6 = reinterpret_cast<::sockaddr_in6*>(ifa->ifa_addr);
            break;
        }
    }
    ASSERT_NE(ifa, nullptr)
        << "This SDK expects an IPv6 loopback adress, but no one found.";

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
    EXPECT_EQ((ifa->ifa_flags &
               0x0000ffff), // Ignore special linux only flags from linux/if.h.
              0 | IFF_RUNNING | IFF_LOOPBACK | IFF_UP)
#endif
        << "Unexpected flags " << ifa->ifa_flags << " = 0b"
        << std::bitset<8 * sizeof(ifa->ifa_flags)>(ifa->ifa_flags) << ".\n";

    // Check additional items of the netinterface address.
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, 0);

    // Check netinterface netmask
    sin6 = reinterpret_cast<::sockaddr_in6*>(ifa->ifa_netmask);
    uint64_t* sin6_64 = reinterpret_cast<uint64_t*>(&sin6->sin6_addr);
    EXPECT_EQ(sin6_64[0], ~0);
    EXPECT_EQ(sin6_64[1], ~0);
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, 0);

#ifndef __APPLE__
    // The ifa_broadaddr field is effectively unused in macOS for IPv6
    // configurations and has uspecified content.
    // On other platforms there is no pointer to an address structure.
    EXPECT_EQ(ifa->ifa_broadaddr, nullptr); // IPv6 does not use broadcasts.
#endif
    EXPECT_EQ(ifa->ifa_data, nullptr);

    // Test Unit
    // ---------
    CIfaddr ifaddrObj(::if_nametoindex(ifa->ifa_name), ifa->ifa_name, "[::1]",
                      ifa->ifa_flags);
    ifaddrs* ifaddr = ifaddrObj.get();

    EXPECT_EQ(ifaddr->ifa_next, nullptr);
    EXPECT_STREQ(ifaddr->ifa_name, ifa->ifa_name);
    EXPECT_EQ(ifaddr->ifa_flags, ifa->ifa_flags);
    ASSERT_EQ(ifaddr->ifa_addr->sa_family, AF_INET6);

    // Check netinterface address
    sin6 = reinterpret_cast<::sockaddr_in6*>(ifaddr->ifa_addr);
    EXPECT_TRUE(IN6_IS_ADDR_LOOPBACK(&sin6->sin6_addr));
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, 0);

    // Check netinterface netmask
    sin6 = reinterpret_cast<::sockaddr_in6*>(ifaddr->ifa_netmask);
    EXPECT_EQ(sin6_64[0], ~0);
    EXPECT_EQ(sin6_64[1], ~0);
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, 0);

    // Ignore IPv6 netinterface broadcast address and check data field.
    // EXPECT_EQ(ifaddr->ifa_broadaddr, ifa->ifa_broadaddr);
    EXPECT_EQ(ifaddr->ifa_data, ifa->ifa_data);
}

TEST(UtestHelperTestSuite, ifaddr_set_wrong_ipv6_netaddress_fails) {
    // Test Unit
#ifndef __APPLE__
    // MacOS ignores an invalid scope_id silently and accept the netaddress.
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

TEST_F(UtestHelperFTestSuite, ifaddr_set_ipv4_loopback_netaddress) {
    // Get real network interface address information and emulate the system
    // call with same results.

    ifaddrs* ifa;
    ASSERT_EQ(getifaddrs(&m_ifaddr), 0);

    // Search an IPv4 loopback address.
    ::sockaddr_in* sin{};
    for (ifa = m_ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr)
            continue;
        // Check if IPv4 is_loopback address.
        if (ifa->ifa_addr->sa_family == AF_INET &&
            (ifa->ifa_flags & IFF_LOOPBACK)) {
            sin = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
            break;
        }
    }
    if (ifa == nullptr)
        GTEST_SKIP() << "No IPv4 Loopback Address found for testing.";

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
    EXPECT_EQ((ifa->ifa_flags &
               0x0000ffff), // Ignore special linux only flags from linux/if.h.
              0 | IFF_RUNNING | IFF_LOOPBACK | IFF_UP)
#endif
        << "Unexpected flags " << ifa->ifa_flags << " = 0b"
        << std::bitset<8 * sizeof(ifa->ifa_flags)>(ifa->ifa_flags) << ".\n";

    // Check netinterface address.
    char addr4[INET_ADDRSTRLEN];
    ASSERT_EQ(inet_ntop(AF_INET, &sin->sin_addr, addr4, sizeof(addr4)), addr4);
    EXPECT_STREQ(addr4, "127.0.0.1");
    EXPECT_EQ(sin->sin_port, 0);

    // Check netinterface netmask.
    ASSERT_EQ(ifa->ifa_netmask->sa_family, AF_INET);
    char netmask4[INET_ADDRSTRLEN];
    sin = reinterpret_cast<sockaddr_in*>(ifa->ifa_netmask);
    ASSERT_EQ(inet_ntop(AF_INET, &sin->sin_addr, netmask4, sizeof(netmask4)),
              netmask4);
    EXPECT_STREQ(netmask4, "255.0.0.0");
    EXPECT_EQ(sin->sin_port, 0);

    // Check netinterface broadcast address.
    ASSERT_EQ(ifa->ifa_broadaddr->sa_family, AF_INET);
    char broadaddr[INET_ADDRSTRLEN];
    sin = reinterpret_cast<sockaddr_in*>(ifa->ifa_broadaddr);
    ASSERT_EQ(inet_ntop(AF_INET, &sin->sin_addr, broadaddr, sizeof(broadaddr)),
              broadaddr);
    EXPECT_STREQ(broadaddr, "127.0.0.1");
    EXPECT_EQ(sin->sin_port, 0);

    EXPECT_EQ(ifa->ifa_data, nullptr);

    // Test Unit
    // ---------
    CIfaddr ifaddrObj(::if_nametoindex(ifa->ifa_name), ifa->ifa_name, addr4,
                      (ifa->ifa_flags & 0x0000ffff), 8);
    ifaddrs* ifaddr = ifaddrObj.get();

    EXPECT_EQ(ifaddr->ifa_next, nullptr);
    EXPECT_STREQ(ifaddr->ifa_name, ifa->ifa_name);
    EXPECT_EQ(ifaddr->ifa_flags, (ifa->ifa_flags & 0x0000ffff));
    ASSERT_EQ(ifaddr->ifa_addr->sa_family, AF_INET);

    char addr4buf[INET_ADDRSTRLEN]{};

    // Check netinterface address
    sin = reinterpret_cast<::sockaddr_in*>(ifaddr->ifa_addr);
    ASSERT_EQ(inet_ntop(AF_INET, &sin->sin_addr, addr4buf, INET_ADDRSTRLEN),
              addr4buf);
    EXPECT_STREQ(addr4buf, addr4);
    EXPECT_EQ(sin->sin_port, 0);

    // Check netinterface netmask
    ASSERT_EQ(ifaddr->ifa_netmask->sa_family, AF_INET);
    sin = reinterpret_cast<::sockaddr_in*>(ifaddr->ifa_netmask);
    ASSERT_EQ(inet_ntop(AF_INET, &sin->sin_addr, addr4buf, INET_ADDRSTRLEN),
              addr4buf);
    EXPECT_STREQ(addr4buf, netmask4);
    EXPECT_EQ(sin->sin_port, 0);

    // Check netinterface broadcast address
    ASSERT_EQ(ifaddr->ifa_broadaddr->sa_family, AF_INET);
    sin = reinterpret_cast<::sockaddr_in*>(ifaddr->ifa_broadaddr);
    ASSERT_EQ(inet_ntop(AF_INET, &sin->sin_addr, addr4buf, INET_ADDRSTRLEN),
              addr4buf);
    EXPECT_STREQ(addr4buf, broadaddr);
    EXPECT_EQ(sin->sin_port, 0);

    EXPECT_EQ(ifaddr->ifa_data, nullptr);
}

TEST_F(UtestHelperFTestSuite, ifaddr_set_ipv6_lla) {
    // Get real network interface address information and emulate the system
    // call with same results.

    ifaddrs* ifa;
    ASSERT_EQ(getifaddrs(&m_ifaddr), 0);

    // Search a link-local address.
    ::sockaddr_in6* sin6{};
    for (ifa = m_ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr)
            continue;
        // Check if IPv6 is_link-local address.
        if (ifa->ifa_addr->sa_family == AF_INET6) {
            sin6 = reinterpret_cast<::sockaddr_in6*>(ifa->ifa_addr);
            if (UPnPsdk::IN6_ADDR_LINKLOCAL(&sin6->sin6_addr))
                break;
        }
    }
    ASSERT_NE(ifa, nullptr)
        << "This SDK expects an IPv6 link-local adress, but no one found.";

    EXPECT_STRNE(ifa->ifa_name, "");
    std::cerr << "DEBUG! ifa_name=\"" << ifa->ifa_name << "\".\n";
#ifdef __APPLE__
    EXPECT_EQ(ifa->ifa_flags,
              0 | 0x8000 /*IFF_DYNAMIC?*/ | IFF_RUNNING | IFF_LOOPBACK | IFF_UP)
    // EXPECT_EQ(ifa->ifa_flags, 0b00000000000000001000000001001001);
#else
    // EXPECT_EQ(ifa->ifa_flags, 0b00000000000000010001000001000011)
    EXPECT_EQ((ifa->ifa_flags &
               0x0000ffff), // Ignore special linux only flags from linux/if.h.
              0 | IFF_UP | IFF_BROADCAST | IFF_RUNNING |
                  IFF_MULTICAST /*| IFF_LOWER_UP*/) // lower to higher
#endif
        << "Unexpected flags " << ifa->ifa_flags << " = 0b"
        << std::bitset<8 * sizeof(ifa->ifa_flags)>(ifa->ifa_flags) << ".\n";

    // Get netinterface address.
    char addr6[INET6_ADDRSTRLEN + 2]{};
    addr6[0] = '[';
    ASSERT_EQ(
        inet_ntop(AF_INET6, &sin6->sin6_addr, addr6 + 1, sizeof(addr6) - 1),
        addr6 + 1);
    addr6[strlen(addr6)] = ']';
    EXPECT_EQ(sin6->sin6_scope_id, ::if_nametoindex(ifa->ifa_name));
    EXPECT_EQ(sin6->sin6_port, 0);

    // Get netinterface netmask.
    char netmask6[INET6_ADDRSTRLEN];
    ASSERT_EQ(ifa->ifa_netmask->sa_family, AF_INET6);
    sin6 = reinterpret_cast<::sockaddr_in6*>(ifa->ifa_netmask);
    ASSERT_EQ(inet_ntop(AF_INET6, &sin6->sin6_addr, netmask6, sizeof(netmask6)),
              netmask6);
    uint64_t* sin6_64 = reinterpret_cast<uint64_t*>(&sin6->sin6_addr);
    EXPECT_EQ(sin6_64[0], ~0);
    EXPECT_EQ(sin6_64[1], 0);
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, 0);

#ifndef __APPLE__
    // The ifa_broadaddr field is effectively unused in macOS for IPv6
    // configurations and has uspecified content.
    // On other platforms there is no pointer to an address structure.
    EXPECT_EQ(ifa->ifa_broadaddr, nullptr); // IPv6 does not use broadcasts.
#endif
    EXPECT_EQ(ifa->ifa_data, nullptr);

    // Test Unit
    // ---------
    std::cerr << "DEBUG! addr6=\"" << addr6 << "\".\n";
    CIfaddr ifaddrObj(::if_nametoindex(ifa->ifa_name), ifa->ifa_name, addr6,
                      (ifa->ifa_flags & 0x0000ffff), 64);
    ifaddrs* ifaddr = ifaddrObj.get();

    EXPECT_EQ(ifaddr->ifa_next, nullptr);
    EXPECT_STREQ(ifaddr->ifa_name, ifa->ifa_name);
    EXPECT_EQ(ifaddr->ifa_flags, (ifa->ifa_flags & 0x0000ffff));
    ASSERT_EQ(ifaddr->ifa_addr->sa_family, AF_INET6);

    // Check netinterface address
    ::sockaddr_in6* sin6_ifa = reinterpret_cast<::sockaddr_in6*>(ifa->ifa_addr);
    sin6 = reinterpret_cast<::sockaddr_in6*>(ifaddr->ifa_addr);
    EXPECT_EQ(
        ::memcmp(&sin6_ifa->sin6_addr, &sin6->sin6_addr, sizeof(::in6_addr)),
        0);
    EXPECT_EQ(sin6_ifa->sin6_scope_id, sin6->sin6_scope_id);
    EXPECT_EQ(sin6_ifa->sin6_port, sin6->sin6_port);

    // Check netinterface netmask
    sin6 = reinterpret_cast<::sockaddr_in6*>(ifaddr->ifa_netmask);
    sin6_64 = reinterpret_cast<uint64_t*>(&sin6->sin6_addr);
    EXPECT_EQ(sin6_64[0], ~0);
    EXPECT_EQ(sin6_64[1], 0);
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, 0);

    // Ignore IPv6 netinterface broadcast address and check data field.
    EXPECT_EQ(ifaddr->ifa_data, ifa->ifa_data);
}

TEST_F(UtestHelperFTestSuite, ifaddr_set_ipv6_gua) {
    // Get real network interface address information and emulate the system
    // call with same results.

    ifaddrs* ifa; // Pointer to the current used network interface address.
    ASSERT_EQ(getifaddrs(&m_ifaddr), 0);

    // Search a global unicast address.
    ::sockaddr_in6* sin6{};
    for (ifa = m_ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr)
            continue;
        // Check if IPv6 is a global unicast address.
        if (ifa->ifa_addr->sa_family == AF_INET6) {
            sin6 = reinterpret_cast<::sockaddr_in6*>(ifa->ifa_addr);
            if (UPnPsdk::IN6_ADDR_GLOBAL(&sin6->sin6_addr))
                break;
        }
    }
    if (ifa == nullptr)
        GTEST_SKIP() << "No IPv6 Global Unicast Address found for testing.";

    EXPECT_STRNE(ifa->ifa_name, "");
    std::cerr << "DEBUG! ifa_name=\"" << ifa->ifa_name << "\".\n";
    // EXPECT_EQ(ifa->ifa_flags, 0b00000000000000010001000001000011)
    EXPECT_EQ((ifa->ifa_flags &
               0x0000ffff), // Ignore special linux only flags from linux/if.h.
              0 | IFF_UP | IFF_BROADCAST | IFF_RUNNING |
                  IFF_MULTICAST /*| IFF_LOWER_UP*/) // lower to higher
        << "Unexpected flags " << ifa->ifa_flags << " = 0b"
        << std::bitset<8 * sizeof(ifa->ifa_flags)>(ifa->ifa_flags) << ".\n";

    // Get netinterface address.
    char addr6[INET6_ADDRSTRLEN + 2]{};
    addr6[0] = '[';
    ASSERT_EQ(
        inet_ntop(AF_INET6, &sin6->sin6_addr, addr6 + 1, sizeof(addr6) - 1),
        addr6 + 1);
    addr6[strlen(addr6)] = ']';
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, 0);

    // Get netinterface netmask.
    char netmask6[INET6_ADDRSTRLEN];
    ASSERT_EQ(ifa->ifa_netmask->sa_family, AF_INET6);
    sin6 = reinterpret_cast<::sockaddr_in6*>(ifa->ifa_netmask);
    ASSERT_EQ(inet_ntop(AF_INET6, &sin6->sin6_addr, netmask6, sizeof(netmask6)),
              netmask6);
    uint64_t* sin6_64 = reinterpret_cast<uint64_t*>(&sin6->sin6_addr);
    EXPECT_EQ(sin6_64[0], ~0);
    EXPECT_EQ(sin6_64[1], 0);
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, 0);

#ifndef __APPLE__
    // The ifa_broadaddr field is effectively unused in macOS for IPv6
    // configurations and has uspecified content.
    // On other platforms there is no pointer to an address structure.
    EXPECT_EQ(ifa->ifa_broadaddr, nullptr); // IPv6 does not use broadcasts.
#endif
    EXPECT_EQ(ifa->ifa_data, nullptr);

    // Test Unit
    // ---------
    std::cerr << "DEBUG! addr6=\"" << addr6 << "\".\n";
    CIfaddr ifaddrObj(::if_nametoindex(ifa->ifa_name), ifa->ifa_name, addr6,
                      (ifa->ifa_flags & 0x0000ffff), 64);
    ifaddrs* ifaddr = ifaddrObj.get();

    EXPECT_EQ(ifaddr->ifa_next, nullptr);
    EXPECT_STREQ(ifaddr->ifa_name, ifa->ifa_name);
    EXPECT_EQ(ifaddr->ifa_flags, (ifa->ifa_flags & 0x0000ffff));
    ASSERT_EQ(ifaddr->ifa_addr->sa_family, AF_INET6);

    // Check netinterface address
    ::sockaddr_in6* sin6_ifa = reinterpret_cast<::sockaddr_in6*>(ifa->ifa_addr);
    sin6 = reinterpret_cast<::sockaddr_in6*>(ifaddr->ifa_addr);
    EXPECT_EQ(
        ::memcmp(&sin6_ifa->sin6_addr, &sin6->sin6_addr, sizeof(::in6_addr)),
        0);
    EXPECT_EQ(sin6_ifa->sin6_scope_id, sin6->sin6_scope_id);
    EXPECT_EQ(sin6_ifa->sin6_port, sin6->sin6_port);

    // Check netinterface netmask
    sin6 = reinterpret_cast<::sockaddr_in6*>(ifaddr->ifa_netmask);
    sin6_64 = reinterpret_cast<uint64_t*>(&sin6->sin6_addr);
    EXPECT_EQ(sin6_64[0], ~0);
    EXPECT_EQ(sin6_64[1], 0);
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, 0);

    // Ignore IPv6 netinterface broadcast address and check data field.
    EXPECT_EQ(ifaddr->ifa_data, ifa->ifa_data);
}

TEST_F(UtestHelperFTestSuite, ifaddr_set_ipv6_ula) {
    // Get real network interface address information and emulate the system
    // call with same results.

    ifaddrs* ifa; // Pointer to the current used network interface address.
    ASSERT_EQ(getifaddrs(&m_ifaddr), 0);

    // Search a unique local address.
    ::sockaddr_in6* sin6{};
    for (ifa = m_ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr)
            continue;
        // Check if IPv6 is a unique local address.
        if (ifa->ifa_addr->sa_family == AF_INET6) {
            sin6 = reinterpret_cast<::sockaddr_in6*>(ifa->ifa_addr);
            if (UPnPsdk::IN6_ADDR_UNIQUELOCAL(&sin6->sin6_addr))
                break;
        }
    }
    if (ifa == nullptr)
        GTEST_SKIP() << "No IPv6 Unique Local Address found for testing.";

    EXPECT_STRNE(ifa->ifa_name, "");
    std::cerr << "DEBUG! ifa_name=\"" << ifa->ifa_name << "\".\n";
    // EXPECT_EQ(ifa->ifa_flags, 0b00000000000000010001000001000011)
    EXPECT_EQ((ifa->ifa_flags &
               0x0000ffff), // Ignore special linux only flags from linux/if.h.
              0 | IFF_UP | IFF_BROADCAST | IFF_RUNNING |
                  IFF_MULTICAST /*| IFF_LOWER_UP*/) // lower to higher
        << "Unexpected flags " << ifa->ifa_flags << " = 0b"
        << std::bitset<8 * sizeof(ifa->ifa_flags)>(ifa->ifa_flags) << ".\n";

    // Get netinterface address.
    char addr6[INET6_ADDRSTRLEN + 2]{};
    addr6[0] = '[';
    ASSERT_EQ(
        inet_ntop(AF_INET6, &sin6->sin6_addr, addr6 + 1, sizeof(addr6) - 1),
        addr6 + 1);
    addr6[strlen(addr6)] = ']';
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, 0);

    // Get netinterface netmask.
    char netmask6[INET6_ADDRSTRLEN];
    ASSERT_EQ(ifa->ifa_netmask->sa_family, AF_INET6);
    sin6 = reinterpret_cast<::sockaddr_in6*>(ifa->ifa_netmask);
    ASSERT_EQ(inet_ntop(AF_INET6, &sin6->sin6_addr, netmask6, sizeof(netmask6)),
              netmask6);
    uint64_t* sin6_64 = reinterpret_cast<uint64_t*>(&sin6->sin6_addr);
    EXPECT_EQ(sin6_64[0], ~0);
    EXPECT_EQ(sin6_64[1], 0);
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, 0);

#ifndef __APPLE__
    // The ifa_broadaddr field is effectively unused in macOS for IPv6
    // configurations and has uspecified content.
    // On other platforms there is no pointer to an address structure.
    EXPECT_EQ(ifa->ifa_broadaddr, nullptr); // IPv6 does not use broadcasts.
#endif
    EXPECT_EQ(ifa->ifa_data, nullptr);

    // Test Unit
    // ---------
    std::cerr << "DEBUG! addr6=\"" << addr6 << "\".\n";
    CIfaddr ifaddrObj(::if_nametoindex(ifa->ifa_name), ifa->ifa_name, addr6,
                      (ifa->ifa_flags & 0x0000ffff), 64);
    ifaddrs* ifaddr = ifaddrObj.get();

    EXPECT_EQ(ifaddr->ifa_next, nullptr);
    EXPECT_STREQ(ifaddr->ifa_name, ifa->ifa_name);
    EXPECT_EQ(ifaddr->ifa_flags, (ifa->ifa_flags & 0x0000ffff));
    ASSERT_EQ(ifaddr->ifa_addr->sa_family, AF_INET6);

    // Check netinterface address
    ::sockaddr_in6* sin6_ifa = reinterpret_cast<::sockaddr_in6*>(ifa->ifa_addr);
    sin6 = reinterpret_cast<::sockaddr_in6*>(ifaddr->ifa_addr);
    EXPECT_EQ(
        ::memcmp(&sin6_ifa->sin6_addr, &sin6->sin6_addr, sizeof(::in6_addr)),
        0);
    EXPECT_EQ(sin6_ifa->sin6_scope_id, sin6->sin6_scope_id);
    EXPECT_EQ(sin6_ifa->sin6_port, sin6->sin6_port);

    // Check netinterface netmask
    sin6 = reinterpret_cast<::sockaddr_in6*>(ifaddr->ifa_netmask);
    sin6_64 = reinterpret_cast<uint64_t*>(&sin6->sin6_addr);
    EXPECT_EQ(sin6_64[0], ~0);
    EXPECT_EQ(sin6_64[1], 0);
    EXPECT_EQ(sin6->sin6_scope_id, 0);
    EXPECT_EQ(sin6->sin6_port, 0);

    // Ignore IPv6 netinterface broadcast address and check data field.
    EXPECT_EQ(ifaddr->ifa_data, ifa->ifa_data);
}

TEST_F(UtestHelperFTestSuite, ifaddr_set_ipv4) {
    // Get real network interface address information and emulate the system
    // call with same results.

    ifaddrs* ifa; // Pointer to the current used network interface address.
    ASSERT_EQ(getifaddrs(&m_ifaddr), 0);

    // Search an IPv4 address.
    ::sockaddr_in* sin{};
    for (ifa = m_ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr)
            continue;
        // Check if IPv4, but not IPv4 loopback address.
        if (ifa->ifa_addr->sa_family == AF_INET &&
            !(ifa->ifa_flags & IFF_LOOPBACK)) {
            sin = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
            break;
        }
    }
    if (ifa == nullptr)
        GTEST_SKIP() << "No IPv4 Address found for testing.";

    EXPECT_STRNE(ifa->ifa_name, "");
    std::cerr << "DEBUG! ifa_name=\"" << ifa->ifa_name << "\".\n";
    // EXPECT_EQ(ifa->ifa_flags, 0b00000000000000010001000001000011)
    EXPECT_EQ((ifa->ifa_flags &
               0x0000ffff), // Ignore special linux only flags from linux/if.h.
              0 | IFF_UP | IFF_BROADCAST | IFF_RUNNING |
                  IFF_MULTICAST /*| IFF_LOWER_UP*/) // lower to higher
        << "Unexpected flags " << ifa->ifa_flags << " = 0b"
        << std::bitset<8 * sizeof(ifa->ifa_flags)>(ifa->ifa_flags) << ".\n";

    // Get netinterface address.
    char addr4[INET_ADDRSTRLEN]{};
    ASSERT_EQ(inet_ntop(AF_INET, &sin->sin_addr, addr4, sizeof(addr4)), addr4);
    EXPECT_EQ(sin->sin_port, 0);

    // Get netinterface netmask.
    ASSERT_EQ(ifa->ifa_netmask->sa_family, AF_INET);
    char netmask4[INET_ADDRSTRLEN];
    sin = reinterpret_cast<::sockaddr_in*>(ifa->ifa_netmask);
    ASSERT_EQ(inet_ntop(AF_INET, &sin->sin_addr, netmask4, sizeof(netmask4)),
              netmask4);
    std::bitset<32> binary(sin->sin_addr.s_addr);
    uint8_t subnetbits = static_cast<uint8_t>(binary.count());
    std::cerr << "DEBUG! IPv4 subnetbits=" << std::to_string(subnetbits)
              << ".\n";
    EXPECT_EQ(sin->sin_port, 0);

    // Check netinterface broadcast address.
    ASSERT_EQ(ifa->ifa_broadaddr->sa_family, AF_INET);
    char broadaddr[INET_ADDRSTRLEN];
    sin = reinterpret_cast<sockaddr_in*>(ifa->ifa_broadaddr);
    ASSERT_EQ(inet_ntop(AF_INET, &sin->sin_addr, broadaddr, sizeof(broadaddr)),
              broadaddr);
    std::cerr << "DEBUG! IPv4 broadcast address=\"" << broadaddr << "\".\n";
    EXPECT_EQ(sin->sin_port, 0);

    EXPECT_EQ(ifa->ifa_data, nullptr);

    // Test Unit
    // ---------
    std::cerr << "DEBUG! addr4=\"" << addr4 << "\".\n";
    CIfaddr ifaddrObj(::if_nametoindex(ifa->ifa_name), ifa->ifa_name, addr4,
                      (ifa->ifa_flags & 0x0000ffff), subnetbits);
    ifaddrs* ifaddr = ifaddrObj.get();

    EXPECT_EQ(ifaddr->ifa_next, nullptr);
    EXPECT_STREQ(ifaddr->ifa_name, ifa->ifa_name);
    EXPECT_EQ(ifaddr->ifa_flags, (ifa->ifa_flags & 0x0000ffff));
    ASSERT_EQ(ifaddr->ifa_addr->sa_family, AF_INET);

    // Check netinterface address
    ::sockaddr_in* sin_ifa = reinterpret_cast<::sockaddr_in*>(ifa->ifa_addr);
    sin = reinterpret_cast<::sockaddr_in*>(ifaddr->ifa_addr);
    EXPECT_EQ(::memcmp(&sin_ifa->sin_addr, &sin->sin_addr, sizeof(::in_addr)),
              0);
    EXPECT_EQ(sin_ifa->sin_port, sin->sin_port);

    // Check netinterface netmask
    sin_ifa = reinterpret_cast<::sockaddr_in*>(ifa->ifa_netmask);
    sin = reinterpret_cast<::sockaddr_in*>(ifaddr->ifa_netmask);
    EXPECT_EQ(::memcmp(&sin_ifa->sin_addr, &sin->sin_addr, sizeof(::in_addr)),
              0);
    EXPECT_EQ(sin->sin_port, 0);

    // Check netinterface broadcast address
    sin_ifa = reinterpret_cast<::sockaddr_in*>(ifa->ifa_broadaddr);
    sin = reinterpret_cast<::sockaddr_in*>(ifaddr->ifa_broadaddr);
    EXPECT_EQ(::memcmp(&sin_ifa->sin_addr, &sin->sin_addr, sizeof(::in_addr)),
              0);
    EXPECT_EQ(sin->sin_port, 0);

    EXPECT_EQ(ifaddr->ifa_data, ifa->ifa_data);
}

} // namespace utest

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
