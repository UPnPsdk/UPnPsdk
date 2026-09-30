// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-10-05

// Tools and helper classes to manage gtests
// =========================================

#include <UPnPsdk/synclog.hpp>
#include <utest/utest_unix.hpp>

#include <arpa/inet.h>
#include <stdexcept>
#include <net/if.h>

namespace utest {

//
// CIfaddr
// -------

#if false
#if 0
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
#else
CIfaddr4::CIfaddr4()
// With constructing the object you get an empty interface info.
{
    // Initialize network address
    m_ifa_addr.sin_family = AF_INET;
    // Initialize netmask
    m_ifa_netmask.sin_family = AF_INET;
    // Initialize broadcast address or Point-to-point destination address
    m_ifa_ifu.sin_family = AF_INET;

    // Initialize m_ifaddr structure.
    m_ifaddr.ifa_name = &m_str_empty;
    // v-- Flags from SIOCGIFFLAGS, man 7 netdevice
    // m_ifaddr.ifa_flags = 0 | IFF_LOOPBACK | IFF_UP; // DEBUG! check what set.
}
#endif

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
#endif

CIfaddr::CIfaddr()
// With constructing the object you get an empty networi interface info.
{
    // Initialize m_ifaddr structure. Point only to an empty network interface
    // name. All other values are 0 or nullptr.
    m_ifaddr.ifa_name = &m_str_empty;
}


CIfaddr::CIfaddr(uint32_t a_if_index, std::string_view a_if_name,
                 std::string_view a_if_addr, unsigned int a_ifa_flags,
                 uint8_t a_prefixbits)
// Construct a network interface with given arguments.
{
    m_if_index = a_if_index;
    m_if_name = a_if_name;

    if (a_if_addr.front() == '[') {
        // Set IPv6 interface address
        m_ifa_addr.sa.sa_family = AF_INET6;

        char if_addr_raw[INET6_ADDRSTRLEN]{};
        a_if_addr.copy(if_addr_raw, a_if_addr.size() - 2,
                       1); // Strip brackets.
        if (inet_pton(AF_INET6, if_addr_raw, &m_ifa_addr.sin6.sin6_addr) != 1)
            throw std::invalid_argument(
                UPnPsdk_LOGEXCEPT(
                    "MSG0088") "Invalid plain netaddress (without scope_id, "
                               "port, prefix bitmask) specified. Wrong "
                               "address is \"" +
                std::string(a_if_addr) + "\".");

        // Set netmask to the IPv6 interface address. a_prefixbits count is
        // ignored. Instead the official specified netmask is used.
        m_ifa_netmask.sa.sa_family = AF_INET6;
        uint32_t* sin6_32 =
            reinterpret_cast<uint32_t*>(m_ifa_netmask.sin6.sin6_addr.s6_addr);
        if (IN6_IS_ADDR_LOOPBACK(&m_ifa_addr.sin6.sin6_addr)) {
            sin6_32[0] = 0xffffffff;
            sin6_32[1] = 0xffffffff;
            sin6_32[2] = 0xffffffff;
            sin6_32[3] = 0xffffffff;
        } else if (IN6_IS_ADDR_V4MAPPED(&m_ifa_addr.sin6.sin6_addr)) {
            sin6_32[0] = 0xffffffff;
            sin6_32[1] = 0xffffffff;
            sin6_32[2] = htonl(0xffff);
        } else if (UPnPsdk::IN6_ADDR_LINKLOCAL(&m_ifa_addr.sin6.sin6_addr)) {
            sin6_32[0] = 0xffffffff;
            sin6_32[1] = 0xffffffff;
            m_ifa_addr.sin6.sin6_scope_id = a_if_index;
        } else if (UPnPsdk::IN6_ADDR_GLOBALALL(&m_ifa_addr.sin6.sin6_addr)) {
            sin6_32[0] = 0xffffffff;
            sin6_32[1] = 0xffffffff;
        }

    } else {
        // Set IPv4 interface address.
        m_ifa_addr.sa.sa_family = AF_INET;

        if (inet_pton(AF_INET, std::string(a_if_addr).c_str(),
                      &m_ifa_addr.sin.sin_addr) != 1)
            throw std::invalid_argument(
                UPnPsdk_LOGEXCEPT(
                    "MSG0087") "Invalid plain netaddress (without port, prefix "
                               "bitmask) specified. Wrong address is \"" +
                std::string(a_if_addr) + "\".");

        // Set netmask for IPv4 interface address.
        m_ifa_netmask.sa.sa_family = AF_INET;
        if (a_prefixbits > 32)
            a_prefixbits = 32;
        // Set all ones in netmask if bits count is greater 0.
        ::uint32_t netmask{a_prefixbits == 0 ? 0u : ~0u};
        // Shift zero bits from right into netmask with all ones. No shifting is
        // done with bits count 0 (edge condition).
        netmask <<= (32 - a_prefixbits);
        m_ifa_netmask.sin.sin_addr.s_addr = htonl(netmask);
    }

    m_ifaddr.ifa_name = const_cast<char*>(m_if_name.c_str());
    m_ifaddr.ifa_flags |= a_ifa_flags;
    m_ifaddr.ifa_addr = &m_ifa_addr.sa;
    m_ifaddr.ifa_netmask = &m_ifa_netmask.sa;
    m_ifaddr.ifa_broadaddr = &m_ifa_ifu.sa;
    m_ifaddr.ifa_dstaddr = &m_ifa_ifu.sa;
}


CIfaddr::~CIfaddr() = default;


ifaddrs* CIfaddr::get()
// Return the pointer to the m_ifaddr structure
{
    return &m_ifaddr;
}

} // namespace utest
