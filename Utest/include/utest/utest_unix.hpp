#ifndef UTEST_TOOLS_UNIX_HPP
#define UTEST_TOOLS_UNIX_HPP
// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-12

#include <UPnPsdk/visibility.hpp>
#include <ifaddrs.h>
#include <netinet/in.h> // for sockaddr_in
#include <string>
#include <vector>

namespace utest {

class UPnPsdk_VIS CIfaddr4
// Tool to manage and fill an IPv4 socket address structure. This is needed for
// mocked network interfaces.
{
  public:
    CIfaddr4();
    ifaddrs* get();
    bool set(std::string_view a_Ifname, std::string_view a_Ifaddress);
    void chain_next_addr(struct ifaddrs* a_ptrNextAddr);

  private:
    struct ifaddrs m_ifaddr;

    struct sockaddr_in m_ifa_addr;    // network address
    struct sockaddr_in m_ifa_netmask; // netmask
    struct sockaddr_in m_ifa_ifu; // broadcast addr or point-to-point dest addr

    std::string m_Ifname;         // interface name
    std::string m_Ifaddress;      // interface ip address

    // clang-format off
    // the bitmask is the offset in the netmasks array.
    std::string netmasks[33] = {"0.0.0.0",
            "128.0.0.0", "192.0.0.0", "224.0.0.0", "240.0.0.0",
            "248.0.0.0", "252.0.0.0", "254.0.0.0", "255.0.0.0",
            "255.128.0.0", "255.192.0.0", "255.224.0.0", "255.240.0.0",
            "255.248.0.0", "255.252.0.0", "255.254.0.0", "255.255.0.0",
            "255.255.128.0", "255.255.192.0", "255.255.224.0", "255.255.240.0",
            "255.255.248.0", "255.255.252.0", "255.255.254.0", "255.255.255.0",
            "255.255.255.128", "255.255.255.192", "255.255.255.224", "255.255.255.240",
            "255.255.255.248", "255.255.255.252", "255.255.255.254", "255.255.255.255"};
    // clang-format on
};


class CIfaddr6 {
    // Tool to manage and fill an IPv6 socket address structure. This is needed
    // for mocked network interfaces.
  public:
    CIfaddr6();
    ifaddrs* get();
    bool set(const std::string_view a_Ifname, std::string_view a_Ifaddress);
    void chain_next_addr(struct ifaddrs* a_ptrNextAddr);

  private:
    ifaddrs m_ifaddr{};

    std::string m_ifa_name{};     // name of the adapter/interface
    sockaddr_in6 m_ifa_addr{};    // network address
    sockaddr_in6 m_ifa_netmask{}; // netmask
};

} // namespace utest

#endif // UTEST_TOOLS_UNIX_HPP
