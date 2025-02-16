// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-02-16

// Tools and helper classes to manage gtests
// =========================================

#include <utest/utest_unix.hpp>
#include <UPnPsdk/sockaddr.hpp>

#include <arpa/inet.h>
#include <net/if.h>

namespace utest {

//
// CIfaddr4
// --------

CIfaddr4::CIfaddr4()
// With constructing the object you get a loopback device by default.
{
    // loopback interface
    //-------------------
    // set network address
    m_ifa_addr.sin_family = AF_INET;
    // m_ifa_addr.sin_port = htons(MYPORT);
    inet_aton("127.0.0.1", &(m_ifa_addr.sin_addr));

    // set netmask
    m_ifa_netmask.sin_family = AF_INET;
    // m_ifa_netmask.sin_port = htons(MYPORT);
    inet_aton("255.0.0.0", &(m_ifa_netmask.sin_addr));

    // set broadcast address or Point-to-point destination address
    m_ifa_ifu.sin_family = AF_INET;
    // m_ifa_ifu.sin_port = htons(MYPORT);
    inet_aton("0.0.0.0", &(m_ifa_ifu.sin_addr));

    m_ifaddr.ifa_next = nullptr; // pointer to next ifaddrs structure
    m_ifaddr.ifa_name = (char*)"lo";
    // v-- Flags from SIOCGIFFLAGS, man 7 netdevice
    m_ifaddr.ifa_flags = 0 | IFF_LOOPBACK | IFF_UP;
    m_ifaddr.ifa_addr = (struct sockaddr*)&m_ifa_addr;
    m_ifaddr.ifa_netmask = (struct sockaddr*)&m_ifa_netmask;
    m_ifaddr.ifa_broadaddr = (struct sockaddr*)&m_ifa_ifu;
    m_ifaddr.ifa_data = nullptr;
}

ifaddrs* CIfaddr4::get()
// Return the pointer to the m_ifaddr structure
{
    return &m_ifaddr;
}

bool CIfaddr4::set(std::string_view a_Ifname, std::string_view a_Ifaddress)
// Set the interface name and the ipv4 address with bitmask. Properties are
// set to an ipv4 UP interface, supporting broadcast and multicast.
// Returns true if successful.
{
    if (a_Ifname == "" or a_Ifaddress == "")
        return false;

    // to be thread save we will have the strings here
    m_Ifname = a_Ifname;
    m_Ifaddress = a_Ifaddress;
    m_ifaddr.ifa_name = (char*)m_Ifname.c_str();

    // get the netmask from the bitmask
    // the bitmask is the offset in the netmasks array.
    std::size_t slashpos = m_Ifaddress.find_first_of("/");
    std::string address = m_Ifaddress;
    std::string bitmask = "32";
    if (slashpos != std::string::npos) {
        address = m_Ifaddress.substr(0, slashpos);
        bitmask = m_Ifaddress.substr(slashpos + 1);
    }
    // std::cout << "DEBUG: set ifa_name: " << m_ifaddr.ifa_name << ",
    // address: '" << address << "', bitmask: '" << bitmask << "', netmask: " <<
    // netmasks[std::stoi(bitmask)] << ", slashpos: " << slashpos << "\n";

    // convert address strings to numbers and store them
    inet_aton(address.c_str(), &(m_ifa_addr.sin_addr));
    std::string netmask = netmasks[std::stoi(bitmask)];
    inet_aton(netmask.c_str(), &(m_ifa_netmask.sin_addr));
    m_ifaddr.ifa_flags = 0 | IFF_UP | IFF_BROADCAST | IFF_MULTICAST;

    // calculate broadcast address as follows: broadcast = ip | ( ~ subnet )
    // broadcast = ip-addr ored the inverted subnet-mask
    m_ifa_ifu.sin_addr.s_addr =
        m_ifa_addr.sin_addr.s_addr | ~m_ifa_netmask.sin_addr.s_addr;

    m_ifaddr.ifa_addr = (struct sockaddr*)&m_ifa_addr;
    m_ifaddr.ifa_netmask = (struct sockaddr*)&m_ifa_netmask;
    m_ifaddr.ifa_broadaddr = (struct sockaddr*)&m_ifa_ifu;
    return true;
}

void CIfaddr4::chain_next_addr(struct ifaddrs* a_ptrNextAddr) {
    m_ifaddr.ifa_next = a_ptrNextAddr;
}

//
// CIfaddr6
// --------
CIfaddr6::CIfaddr6()
// With constructing the object you get a loopback device by default.
{
    // loopback interface
    //-------------------
    m_ifa_name = "lo";
    // No problem with casting const away because we only read the source.
    m_ifaddr.ifa_name = const_cast<char*>(m_ifa_name.c_str());
    m_ifaddr.ifa_flags = 0 | IFF_LOOPBACK | IFF_UP;

    // set network address
    m_ifa_addr.sin6_family = AF_INET6;
    // m_ifa_addr.sin_port = htons(MYPORT);
    inet_pton(AF_INET6, "::1", &(m_ifa_addr.sin6_addr));
    m_ifaddr.ifa_addr = reinterpret_cast<sockaddr*>(&m_ifa_addr);

    // set netmask
    m_ifa_netmask.sin6_family = AF_INET6;
    // m_ifa_netmask.sin_port = htons(MYPORT);
    inet_pton(AF_INET6, "FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF",
              &(m_ifa_netmask.sin6_addr));
    m_ifaddr.ifa_netmask = reinterpret_cast<sockaddr*>(&m_ifa_netmask);
}

ifaddrs* CIfaddr6::get() {
    // Return the pointer to the m_ifaddr structure
    return &m_ifaddr;
}

bool CIfaddr6::set(const std::string_view a_Ifname,
                   std::string_view a_Ifaddress) {
    // Set the interface name and the ipv6 address with bitmask. Properties are
    // set to an ipv6 UP interface, supporting multicast. Returns true if
    // successful.
    if (a_Ifname == "" or a_Ifaddress == "")
        return false;

    m_ifa_name = a_Ifname;
    // No problem with casting const away because we only read the source.
    m_ifaddr.ifa_name = const_cast<char*>(m_ifa_name.c_str());
    m_ifaddr.ifa_flags = 0 | IFF_UP | IFF_MULTICAST;

    // Split address and bitmask.
    std::string address;
    std::string bitmask;
    std::size_t slashpos = a_Ifaddress.find("/");
    if (slashpos != a_Ifaddress.npos) {
        address = a_Ifaddress.substr(0, slashpos);
        bitmask = a_Ifaddress.substr(slashpos + 1);
    } else {
        address = std::string(a_Ifaddress);
        bitmask = "128";
    }

    // Set network address
    // m_ifa_addr.sin_port = htons(MYPORT);
    inet_pton(AF_INET6, address.c_str(), &(m_ifa_addr.sin6_addr));
    m_ifaddr.ifa_addr = reinterpret_cast<sockaddr*>(&m_ifa_addr);

    // Set netmask
    UPnPsdk::SSockaddr sa_netmObj;
    // UPnPsdk::bitmask_to_netmask(
    //     AF_INET6, static_cast<uint8_t>(std::stoi(bitmask)), sa_netmObj);
    m_ifa_netmask = sa_netmObj.sin6;
    m_ifaddr.ifa_netmask = reinterpret_cast<sockaddr*>(&m_ifa_netmask);

    return true;
}

#if 0
void CIfaddr4::chain_next_addr(struct ifaddrs* a_ptrNextAddr) {
    m_ifaddr.ifa_next = a_ptrNextAddr;
}
#endif

} // namespace utest
