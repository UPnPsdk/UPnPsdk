#ifndef UTEST_TOOLS_UNIX_HPP
#define UTEST_TOOLS_UNIX_HPP
// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-10-02

#include <UPnPsdk/visibility.hpp>
#include <UPnPsdk/sockaddr.hpp>

#include <ifaddrs.h>
#include <netinet/in.h> // for sockaddr_in
#include <string>

namespace utest {

#if 0
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
    // Provide storrage for emulated values.
    char m_str_empty{'\0'};
    std::string m_Ifname;        // interface name
    std::string m_Ifaddress;     // interface ip address
    ::sockaddr_in m_ifa_addr;    // network address
    ::sockaddr_in m_ifa_netmask; // netmask
    ::sockaddr_in m_ifa_ifu;     // broadcast addr or point-to-point dest addr

    // Storage for emulated pointer and values.
    ::ifaddrs m_ifaddr{};

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
#endif

class UPnPsdk_VIS CIfaddr
// Tool to provide an network interface address for emulation and mocking.
{
  public:
    // Default Constructor for empty netinterface address.
    CIfaddr();

    // Constructor for a specific netinterface address.
    CIfaddr(uint32_t a_if_index, std::string_view a_if_name,
            std::string_view a_if_addr, unsigned int a_ifa_flags,
            uint8_t a_prefixbits = 0);

    // Default Destructor
    ~CIfaddr();

    // Get pointer to the netinterface address info structure.
    ifaddrs* get();

    // Set pointer in netinterface address info to next info.
    void chain_next_addr(ifaddrs* a_next_addr);

  private:
    // Provide storrage for emulated values.
    char m_str_empty{'\0'};
    uint32_t m_if_index;                 // netinterface index
    std::string m_if_name;               // netinterface name
    UPnPsdk::sockaddr_t m_ifa_addr{};    // netinterface address
    UPnPsdk::sockaddr_t m_ifa_netmask{}; // netmask
    UPnPsdk::sockaddr_t m_ifa_ifu{}; // broadcast addr|point-to-point dest addr

    // Storage for emulated pointer and values.
    ::ifaddrs m_ifaddr{};
};

} // namespace utest

#endif // UTEST_TOOLS_UNIX_HPP
