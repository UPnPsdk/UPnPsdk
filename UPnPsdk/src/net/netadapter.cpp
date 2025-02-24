// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-02-25
/*!
 * \file
 * \brief Manage information about network adapters.
 */

#include <UPnPsdk/netadapter.hpp>
#include <UPnPsdk/addrinfo.hpp>
#include <UPnPsdk/synclog.hpp>


namespace UPnPsdk {

uint8_t netmask_to_bitmask(const ::sockaddr_storage* a_netmask) {
    TRACE("Executing netmask_to_bitmask()")
    if (a_netmask == nullptr)
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT +
            "MSG1059: No network socket address information given.\n");

    uint8_t bitmask{};
    uint8_t nullbits{};
    switch (a_netmask->ss_family) {
    case AF_INET6: {
        // Count set bits.
        for (size_t i{}; i < sizeof(in6_addr); i++) {
            uint8_t s6addr = reinterpret_cast<const ::sockaddr_in6*>(a_netmask)
                                 ->sin6_addr.s6_addr[i];
            if (s6addr == 255) {
                bitmask += 8;
            } else {
                while (s6addr) {
                    bitmask++;
                    s6addr <<= 1;
                }
                break; // for() loop
            }
        }
        // Check if all remaining bits are zero.
        for (int i{sizeof(in6_addr) - 1}; i >= 0; i--) {
            uint8_t s6addr = reinterpret_cast<const ::sockaddr_in6*>(a_netmask)
                                 ->sin6_addr.s6_addr[i];
            if (s6addr == 0) {
                nullbits += 8;
            } else {
                while ((s6addr & 1) == 0) {
                    nullbits++;
                    s6addr >>= 1;
                }
                break; // for() loop
            }
        }
        // Check valid netmask.
        if (bitmask + nullbits != 128) {
            char ip_str[INET6_ADDRSTRLEN]{};
            ::inet_ntop(
                AF_INET6,
                &reinterpret_cast<const ::sockaddr_in6*>(a_netmask)->sin6_addr,
                ip_str, sizeof(ip_str));
            throw std::runtime_error(
                UPnPsdk_LOGEXCEPT +
                "MSG1067: Invalid ip-address prefix bitmask \"[" +
                std::string(ip_str) + "]\".\n");
        }
    } break; // switch()

    case AF_INET: {
        in_addr_t saddr = ntohl(
            reinterpret_cast<const ::sockaddr_in*>(a_netmask)->sin_addr.s_addr);
        in_addr_t saddr0 = saddr;
        // Count set bits.
        while (saddr) {
            bitmask++;
            saddr <<= 1;
        }
        // Check if all remaining bits are zero.
        for (int i{}; (saddr0 & 1) == 0 && i < 32; i++) {
            nullbits++;
            saddr0 >>= 1;
        }
        // Check valid netmask.
        if (bitmask + nullbits != 32) {
            char ip_str[INET_ADDRSTRLEN]{};
            ::inet_ntop(
                AF_INET,
                &reinterpret_cast<const ::sockaddr_in*>(a_netmask)->sin_addr,
                ip_str, sizeof(ip_str));
            throw std::runtime_error(
                UPnPsdk_LOGEXCEPT +
                "MSG1069: Invalid ip-address prefix bitmask \"" +
                std::string(ip_str) + "\".\n");
        }
    } break;

    case AF_UNSPEC:
        return 0;

    default:
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT + "MSG1028: Unsupported address family(" +
            std::to_string(a_netmask->ss_family) + "), only AF_INET6(" +
            std::to_string(AF_INET6) + "), AF_INET(" + std::to_string(AF_INET) +
            "), or AF_UNSPEC(" + std::to_string(AF_UNSPEC) + ") are valid.\n");
    } // switch()

    return bitmask;
}


void bitmask_to_netmask(const ::sockaddr_storage* a_saddr,
                        const uint8_t a_prefixlength, SSockaddr& a_saddrObj) {
    TRACE("Executing bitmask_to_netmask()")
    if (a_saddr == nullptr)
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT +
            "MSG1070: No associated socket address for the netmask given.\n");

    switch (a_saddr->ss_family) {
    case AF_INET6: {
        // I have to manage 16 bytes from the binary IPv6 address to set its
        // bits. 15 bytes are all ones or all zero bits. Only one byte may have
        // partly ones and zero bits. First I calculate how many leading bytes
        // have full one bits. Following is the partial set byte. The rest are
        // bytes with full zero bits set.
        in6_addr netmask6{};

        // All prefix lengths > 128 will throw this exception.
        if (a_prefixlength > 128)
            throw std::runtime_error(
                UPnPsdk_LOGEXCEPT +
                "MSG1124: Invalid IPv6 address prefix bitmask(" +
                std::to_string(a_prefixlength) + ") exceeds 128.\n");

        // Calculate number of leading bytes with full one bits.
        int ones_bytes{a_prefixlength / 8};

        // Fill leading bytes with all bit ones.
        int i{};
        for (; i < ones_bytes; i++)
            netmask6.s6_addr[i] = static_cast<uint8_t>(~0);

        // Handle the one partly bit-set byte.
        if (i < 16 /*bytes*/) {
            // Preset byte with all bit ones.
            netmask6.s6_addr[i] = static_cast<uint8_t>(~0);
            // Shift remaining zero bits from right into byte with using the
            // reminder from the byte devision.
            netmask6.s6_addr[i] <<= (8 - (a_prefixlength % 8));
            i++;

            // Fill remaining bytes with zero bits.
            for (; i < 16 /*bytes*/; i++)
                netmask6.s6_addr[i] = 0;
        }

        // Return the result.
        a_saddrObj = "";
        a_saddrObj.sin6.sin6_family = AF_INET6;
        memcpy(&a_saddrObj.sin6.sin6_addr, &netmask6,
               sizeof(a_saddrObj.sin6.sin6_addr));
    } break;

    case AF_INET: {
        // All prefix lengths > 32 will throw this exception.
        if (a_prefixlength > 32) {
            throw std::runtime_error(
                UPnPsdk_LOGEXCEPT +
                "MSG1125: Invalid IPv4 address prefix bitmask(" +
                std::to_string(a_prefixlength) + ") exceeds 32.\n");
        }
        // Prepare socket address and shift zero bits from right into the mask.
        ::sockaddr_in saddr{};
        saddr.sin_family = AF_INET;
        if (a_prefixlength) {
            in_addr_t bitshift{~0u}; // all one bits
            bitshift <<= (32 - a_prefixlength); // shift zero bits from right
            saddr.sin_addr.s_addr = htonl(bitshift);
        }
        // Return the result.
        a_saddrObj.sin = saddr;
    } break;

    default: {
        UPnPsdk_LOGCRIT "MSG1126: Invalid address family("
            << a_saddr->ss_family << "), only AF_INET6(" << AF_INET6
            << ") or AF_INET(" << AF_INET
            << ") are valid. Continue with AF_UNSPEC(" << AF_UNSPEC << ").\n";
        a_saddrObj = "";
        a_saddrObj.ss.ss_family = AF_UNSPEC;
    }
    } // switch
}


CNetadapter::CNetadapter(){
    TRACE2(this, " Constnuct CNetadapter()") //
}

CNetadapter::~CNetadapter() {
    TRACE2(this, " Destruct CNetadapter()")
}

bool CNetadapter::find_first(const std::string& a_name_or_addr) {
    TRACE2(this, " Executing CNetadapter::find_first(" + a_name_or_addr + ")")
    // First look for a loopback interface.
    if (a_name_or_addr == "loopback") {
        SSockaddr sa_nadObj;
        this->reset();
        do {
            this->sockaddr(sa_nadObj);
            if ((sa_nadObj.ss.ss_family == AF_INET6 &&
                 IN6_IS_ADDR_LOOPBACK(&sa_nadObj.sin6.sin6_addr)) ||
                (sa_nadObj.ss.ss_family == AF_INET &&
                 // address between "127.0.0.0" and "127.255.255.255"
                 ntohl(sa_nadObj.sin.sin_addr.s_addr) >= 2130706432 &&
                 ntohl(sa_nadObj.sin.sin_addr.s_addr) <= 2147483647)) {
                m_find_index = this->index();
                return true;
            }
        } while (this->get_next());

        // No loopback interface found, nothing more to do.
        return false;
    }

    // Look for a local network adapter name
    this->reset();
    do {
        if (this->name() == a_name_or_addr) {
            m_find_index = this->index();
            return true;
        }
    } while (this->get_next());

    // No name found, look for the ip address of a local network adapter.
    // Try to translate input argument to a socket address.
    CAddrinfo ainfoObj(a_name_or_addr, AI_NUMERICHOST, 0);
    if (!ainfoObj.get_first())
        return false;

    // Valid ip address string given as input argument. Get its socket address.
    SSockaddr sa_inputObj{};
    ainfoObj.sockaddr(sa_inputObj);

    // Parse network adapter list for the given input argument.
    SSockaddr sa_nadObj;
    this->reset();
    do {
        this->sockaddr(sa_nadObj);
        if (sa_nadObj == sa_inputObj) {
            m_find_index = this->index();
            return true;
        }
    } while (this->get_next());

    return false;
}

bool CNetadapter::find_first(unsigned int a_index) {
    TRACE2(this, " Executing CNetadapter::find_first(" +
                     std::to_string(a_index) + ")")
    this->reset();
    do {
        if (this->index() == a_index) {
            m_find_index = a_index;
            return true;
        }
    } while (this->get_next());

    return false;
}

bool CNetadapter::find_next() {
    TRACE2(this, " Executing CNetadapter::find_next() (on netadapter index " +
                     std::to_string(m_find_index) + ")")
    if (m_find_index == 0)
        return false;

    while (this->get_next()) {
        if (this->index() == m_find_index)
            return true;
    }
    m_find_index = 0;
    return false;
}

} // namespace UPnPsdk
