// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-09-03
/*!
 * \file
 * \brief Manage information about network adapters.
 */

#include <UPnPsdk/netadapter.hpp>
#include <UPnPsdk/synclog.hpp>
/// \cond
#include <utility>
/// \endcond


namespace UPnPsdk {

// \cond
// Overload |, &, ~, |=, &= for bit flags UPnPsdk::CNetadapter::ADDRS
using ADDRS = CNetadapter::ADDRS;

ADDRS operator|(ADDRS lhs, ADDRS rhs) {
    using Underlying = std::underlying_type_t<ADDRS>;
    return static_cast<ADDRS>(static_cast<Underlying>(lhs) |
                              static_cast<Underlying>(rhs));
}
ADDRS operator&(ADDRS lhs, ADDRS rhs) {
    using Underlying = std::underlying_type_t<ADDRS>;
    return static_cast<ADDRS>(static_cast<Underlying>(lhs) &
                              static_cast<Underlying>(rhs));
}
ADDRS operator~(ADDRS rhs) {
    using Underlying = std::underlying_type_t<ADDRS>;
    return static_cast<ADDRS>(~static_cast<Underlying>(rhs));
}
ADDRS& operator|=(ADDRS& lhs, ADDRS rhs) {
    lhs = lhs | rhs;
    return lhs;
}
ADDRS& operator&=(ADDRS& lhs, ADDRS rhs) {
    lhs = lhs & rhs;
    return lhs;
}
// \endcond


uint8_t netmask_to_bitmask(const in6_addr& a_sin6_addr) noexcept {
    uint8_t bitmask{};
    uint8_t nullbits{};

    // Count set bits.
    for (size_t i{}; i < sizeof(a_sin6_addr); i++) {
        uint8_t s6addr = a_sin6_addr.s6_addr[i];
        if (s6addr == 255) {
            bitmask += 8;
        } else {
            while (s6addr) {
                bitmask++;
                s6addr <<= 1;
            }
            break;
        }
    }
    // Check if all remaining bits are zero.
    for (int i{sizeof(a_sin6_addr) - 1}; i >= 0; i--) {
        uint8_t s6addr = a_sin6_addr.s6_addr[i];
        if (s6addr == 0) {
            nullbits += 8;
        } else {
            while ((s6addr & 1) == 0) {
                nullbits++;
                s6addr >>= 1;
            }
            break;
        }
    }
    // Check valid netmask.
    if (bitmask + nullbits != 128) {
        UPnPsdk_LOGERR("MSG1067") "Cannot convert netmask to bitmask.\n";
        return 255;
    }

    return bitmask;
}

uint8_t netmask_to_bitmask(const std::string& a_netmask) {
    auto netmask_size = a_netmask.size();
    if (netmask_size < 4 || // The smalest netaddr is "[::]".
        a_netmask.front() != '[' || a_netmask.back() != ']')
        return 255;

    in6_addr sin6_addr;
    if (::inet_pton(AF_INET6, a_netmask.substr(1, netmask_size - 2).c_str(),
                    &sin6_addr) != 1)
        return 255;

    return netmask_to_bitmask(sin6_addr);
}


std::string bitmask_to_netmask(unsigned int a_prefixlength) noexcept {
    // I have to manage 16 bytes from the binary IPv6 address to set its
    // bits. 15 bytes are all ones or all zero bits. Only one byte may have
    // partly ones and zero bits. First I calculate how many leading bytes
    // have full one bits. Following is the partial set byte. The rest are
    // bytes with full zero bits set.
    in6_addr netmask6;

    // All prefix lengths > 128 will be limited to 128.
    if (a_prefixlength > 128)
        a_prefixlength = 128;

    // Calculate number of leading bytes with full one bits.
    size_t ones_bytes{a_prefixlength / 8};

    // Fill leading bytes with all bit ones.
    size_t i;
    for (i = 0; i < ones_bytes; i++)
        netmask6.s6_addr[i] = 0xff;

    // Handle the one partly bit-set byte.
    if (i < 16 /*bytes*/) {
        // Preset byte with all bit ones.
        netmask6.s6_addr[i] = 0xff;
        // Shift remaining zero bits from right into byte with using the
        // reminder from the byte devision.
        netmask6.s6_addr[i] <<= (8 - (a_prefixlength % 8));
        i++;

        // Fill remaining bytes with zero bits.
        for (; i < 16 /*bytes*/; i++)
            netmask6.s6_addr[i] = 0;
    }

    // Return the result.
    char addr_buf[INET6_ADDRSTRLEN];
    if (::inet_ntop(AF_INET6, &netmask6, addr_buf, sizeof(addr_buf)) == nullptr)
        addr_buf[0] = '\0';
    return "[" + std::string(addr_buf).append("]");
}


// CNetadapter class
// =================
CNetadapter::CNetadapter(PNetadapter_platform a_na_platformPtr)
    : m_na_platformPtr(a_na_platformPtr) {
    TRACE2(this, " Construct CNetadapter()") //
}

CNetadapter::~CNetadapter() {
    TRACE2(this, " Destruct CNetadapter()")
}

void CNetadapter::get_first() {
    m_na_platformPtr->get_first();

    // Get index of the loopback interface and cache it for later use. That is
    // the interface containing the IPv6 Loopback address.
    SSockaddr saObj;
    do {
        m_na_platformPtr->sockaddr(saObj);
        if (IN6_IS_ADDR_LOOPBACK(&saObj.sin6.sin6_addr)) {
            m_index_loop = m_na_platformPtr->index();
            break;
        }
    } while (m_na_platformPtr->get_next());

    this->reset();
}

bool CNetadapter::get_next() { return m_na_platformPtr->get_next(); }

unsigned int CNetadapter::index() const { return m_na_platformPtr->index(); }

std::string CNetadapter::name() const { return m_na_platformPtr->name(); }

void CNetadapter::sockaddr(SSockaddr& a_saddr) const {
    SSockaddr saddr;
    m_na_platformPtr->sockaddr(saddr);

    // If there is an IPv4 address then map it to IPv6.
    if (saddr.ss.ss_family == AF_INET) {
        // We will get something like "[::ffff:192.168.1.2]:49494".
        SSockaddr saddr4to6;
        saddr4to6.sin6.sin6_port = saddr.sin.sin_port;
        uint32_t* sin6_32 =
            reinterpret_cast<uint32_t*>(&saddr4to6.sin6.sin6_addr);
        sin6_32[0] = 0;
        sin6_32[1] = 0;
        sin6_32[2] = htonl(0x0000ffff);
        sin6_32[3] = saddr.sin.sin_addr.s_addr;

        a_saddr = saddr4to6;
    } else {
        a_saddr = saddr;
    }
}

unsigned int CNetadapter::bitmask() const {
    SSockaddr saObj;
    m_na_platformPtr->sockaddr(saObj);
    if (saObj.family == AF_INET) // This will be V4MAPPED
        return 96; // V4MAPPED has bitmask 96
    return m_na_platformPtr->bitmask();
}

/// \cond
void CNetadapter::reset() noexcept { m_na_platformPtr->reset(); }
/// \endcond


bool CNetadapter::find_first(std::string_view a_name_or_addr) {
    SSockaddr nad_saObj;
    this->reset(); // noexcept

    // By default look for a valid ip address without loopback, no matter what
    // local network adapter.
    // -----------------------------------------------------------------------
    // The operating system presents one as best choise.
    if (a_name_or_addr.empty()) {
        if (this->index() == 0)
            return false; // There isn't any adapter.

        uint32_t index;
        do {
            this->sockaddr(nad_saObj);
            index = this->index();
            // To verify mocked expectations for Unit Test next cout is needed.
            /* std::cout << "DEBUG: find_first() index=" << index
                      << ", netaddrp()=\"" << nad_saObj.netaddrp() << "\"\n"; */
            if (index != m_index_loop &&
                !IN6_IS_ADDR_V4MAPPED(&nad_saObj.sin6.sin6_addr)) {
                m_find_flags = ADDRS::best;
                return true;
            }
        } while (this->get_next());

        // No usable address found, nothing more to do.
        return false;
    }

    // Look for the ip address of a local network adapter.
    // ---------------------------------------------------
    // Last attempt to get a socket address from the input argument.
    if (a_name_or_addr.front() == '[') {
        SSockaddr input_saObj;
        input_saObj = SInaddr(a_name_or_addr);
        if (input_saObj.family == AF_UNSPEC || input_saObj.empty()) {
            return false;
        }
        // Parse network adapter list for the given input ip address.
        do {
            this->sockaddr(nad_saObj);
            // To verify mocked expectations for Unit Test next cout is
            // needed.
            /* std::cout << "DEBUG: find_first() a_name_or_addr=\"" //
                      << a_name_or_addr << "\", adapter_addr=\""
                      << nad_saObj.netaddrp() << "\"\n"; */
            if (nad_saObj == input_saObj) {
                // With a unique ip address given, there cannot be another.
                m_find_flags = ADDRS::none;
                return true;
            }
        } while (this->get_next());

        // No usable address found, nothing more to do.
        return false;
    }

    // Look for a local network adapter name that has at least one ip address.
    // Also the loopback interface is accepted.
    // ----------------------------------------------------------------------
    do {
        auto name = this->name();
        // To verify mocked expectations for Unit Test next cout is needed.
        /* std::cout << "DEBUG: find_first() a_name_or_addr=\"" //
                  << a_name_or_addr << "\", name=\"" << name << "\"\n"; */
        if (name == a_name_or_addr) {
            this->sockaddr(nad_saObj);
            if (!IN6_IS_ADDR_V4MAPPED(&nad_saObj.sin6.sin6_addr)) {
                m_index_find = this->index();
                m_find_flags = ADDRS::index;
                return true;
            }
        }
    } while (this->get_next());

    // No usable address found.
    return false;
}

bool CNetadapter::find_first(const uint32_t a_index) {
    TRACE2(this, " Executing CNetadapter::find_first(" +
                     std::to_string(a_index) + ")")
    this->reset(); // noexcept
    if (this->index() == 0)
        return false; // There isn't any adapter.

    SSockaddr saObj;
    uint32_t index;
    do {
        index = this->index();
        // To verify mocked expectations for Unit Test next cout is needed.
        /* std::cout << "DEBUG: find_first() index(" << index << ") == a_index("
                  << a_index << ")\n"; */
        if (index == a_index) {
            this->sockaddr(saObj);
            if (!IN6_IS_ADDR_V4MAPPED(&saObj.sin6.sin6_addr)) {
                m_index_find = a_index;
                m_find_flags = ADDRS::index;
                return true;
            }
        }
    } while (this->get_next());

    return false;
}

bool CNetadapter::find_first(ADDRS a_flags) {
    TRACE2(this, " Executing CNetadapter::find_first(a_flags)")
    this->reset(); // noexcept
    if (this->index() == 0)
        return false; // There isn't any adapter.

    // Look for filtered addresses, no matter what local network adapter.
    // ------------------------------------------------------------------
    m_find_flags = a_flags;
    SSockaddr saObj;
    do {
        this->sockaddr(saObj);
        // To verify mocked expectations for Unit Test next cout is needed.
        /* std::cout << "DEBUG: find_first(ADDRS) netaddrp()=\"" << saObj
                  << "\"\n"; */
        if ((m_find_flags & ADDRS::lo) != ADDRS::none &&
            IN6_IS_ADDR_LOOPBACK(&saObj.sin6.sin6_addr)) {
            return true;
        }
        if ((m_find_flags & ADDRS::lla) != ADDRS::none &&
            UPnPsdk::IN6_ADDR_LINKLOCAL(&saObj.sin6.sin6_addr) &&
            this->index() != m_index_loop) {
            return true;
        }
        if ((m_find_flags & ADDRS::gua) != ADDRS::none &&
            UPnPsdk::IN6_ADDR_GLOBAL(&saObj.sin6.sin6_addr) &&
            this->index() != m_index_loop) {
            return true;
        }
        if ((m_find_flags & ADDRS::map4) != ADDRS::none &&
            IN6_IS_ADDR_V4MAPPED(&saObj.sin6.sin6_addr) &&
            this->index() != m_index_loop) {
            return true;
        }
    } while (this->get_next());

    // No matching interface found, nothing more to do.
    return false;
}

bool CNetadapter::find_next() {
    TRACE2(this, " Executing CNetadapter::find_next()")
    if (m_find_flags == ADDRS::none)
        return false;

    SSockaddr saObj;
    while (this->get_next()) {
        this->sockaddr(saObj);
        // To verify mocked expectations for Unit Test next cout is needed.
        /* std::cout << "DEBUG: find_next() netaddrp()=\"" << saObj.netaddrp()
                  << "\"\n"; */
        if ((m_find_flags & ADDRS::best) != ADDRS::none &&
            !IN6_IS_ADDR_V4MAPPED(&saObj.sin6.sin6_addr) &&
            this->index() != m_index_loop) {
            return true;
        }
        if ((m_find_flags & ADDRS::index) != ADDRS::none &&
            this->index() == m_index_find &&
            !IN6_IS_ADDR_V4MAPPED(&saObj.sin6.sin6_addr)) {
            return true;
        }
        if ((m_find_flags & ADDRS::lo) != ADDRS::none &&
            IN6_IS_ADDR_LOOPBACK(&saObj.sin6.sin6_addr)) {
            return true;
        }
        if ((m_find_flags & ADDRS::lla) != ADDRS::none &&
            UPnPsdk::IN6_ADDR_LINKLOCAL(&saObj.sin6.sin6_addr) &&
            this->index() != m_index_loop) {
            return true;
        }
        if ((m_find_flags & ADDRS::gua) != ADDRS::none &&
            UPnPsdk::IN6_ADDR_GLOBAL(&saObj.sin6.sin6_addr) &&
            this->index() != m_index_loop) {
            return true;
        }
        if ((m_find_flags & ADDRS::map4) != ADDRS::none &&
            IN6_IS_ADDR_V4MAPPED(&saObj.sin6.sin6_addr) &&
            this->index() != m_index_loop) {
            return true;
        }
    }

    // Nothing found anymore. Stop finding.
    m_find_flags = ADDRS::none;
    return false;
}

} // namespace UPnPsdk
