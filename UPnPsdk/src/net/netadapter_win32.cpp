// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-11-23
/*!
 * \file
 * \brief Manage information from Microsoft Windows about network adapters.
 */

#include <UPnPsdk/netadapter.hpp>
#include <UPnPsdk/synclog.hpp>
#include <umock/iphlpapi.hpp>


namespace UPnPsdk {

namespace {

/*!
 * \internal
 * \brief Get network address mask from address prefix bit number.
\code
// Usage e.g.:
SSockaddr saObj;
bitnum_to_netmask(AF_INET6, 64, saObj);
std::cout << "netmask is " << saObj.netaddr() << '\n';
\endcode
 */
void bitnum_to_netmask(
    /*! [in] Address family AF_INET6 or AF_INET. */
    const sa_family_t a_family,
    /*! [in] IPv6 or IPv4 address prefix length as number of set bits as given
       e.g. with [2001:db8::1]/64. */
    uint8_t a_prefixlength,
    /*! [in,out] Referene to a socket address structure that will be filled with
       the netmask. */
    SSockaddr& a_saddrObj) {
    switch (a_family) {
    case AF_INET6: {
        // I have to manage 16 bytes from the binary IPv6 address to set its
        // bits. 15 bytes are all ones or all zero bits. Only one byte may have
        // partly ones and zero bits. First I calculate how many leading bytes
        // have full one bits. Following is the partial set byte. The rest are
        // bytes with full zero bits set.
        in6_addr netmask6{};

        // Also all prefix length > 128 will return a full netmask. Note that we
        // have an unsigned value.
        if (a_prefixlength > 128) {
            UPnPsdk_LOGCRIT "MSG1124: Invalid IPv6 address prefix bit number("
                << std::to_string(a_prefixlength)
                << "). Continue with truncated to 128.\n";
            a_prefixlength = 128;
        }

        // Calculate number of leading bytes with full one bits.
        int ones_bytes{a_prefixlength / 8};

        // Fill leading bytes with all bit ones.
        int i{0};
        for (; i < ones_bytes; i++)
            netmask6.s6_addr[i] = static_cast<uint8_t>(~0);

        // Handle the one partly bit-set byte.
        if (i < 16 /*bytes*/) {
            // Preset byte with all bit ones.
            netmask6.s6_addr[i] = static_cast<uint8_t>(~0);
            // Shift remaining zero bits from right into byte with using the
            // reminder from the byte devision.
            netmask6.s6_addr[i] = netmask6.s6_addr[i]
                                  << (8 - (a_prefixlength % 8));
            i++;

            // Fill remaining bytes with zero bits.
            for (; i < 16 /*bytes*/; i++)
                netmask6.s6_addr[i] = 0;
        }

        // Return the result.
        memset(&a_saddrObj.ss, 0, sizeof(a_saddrObj.ss));
        a_saddrObj.sin6.sin6_family = AF_INET6;
        memcpy(&a_saddrObj.sin6.sin6_addr, &netmask6,
               sizeof(a_saddrObj.sin6.sin6_addr));
    } break;

    case AF_INET: {
        if (a_prefixlength > 32) {
            UPnPsdk_LOGCRIT "MSG1125: Invalid IPv4 address prefix bit number("
                << std::to_string(a_prefixlength)
                << "). Continue with truncated to 32.\n";
            a_prefixlength = 32;
        }
        in_addr netmask4;
        netmask4.s_addr = ~0u; // all one bits
        netmask4.s_addr = htonl(
            netmask4.s_addr
            << (32 - a_prefixlength)); // shift zero bits from right into mask

        // Return the result.
        memset(&a_saddrObj.ss, 0, sizeof(a_saddrObj.ss));
        a_saddrObj.sin.sin_family = AF_INET;
        memcpy(&a_saddrObj.sin.sin_addr, &netmask4,
               sizeof(a_saddrObj.sin.sin_addr));
    } break;

    default: {
        UPnPsdk_LOGCRIT "MSG1126: Invalid address family("
            << a_family << "), only AF_INET6(" << AF_INET6 << ") or AF_INET("
            << AF_INET << ") are valid. Continue with AF_UNSPEC(" << AF_UNSPEC
            << ").\n";
        memset(&a_saddrObj.ss, 0, sizeof(a_saddrObj.ss));
        a_saddrObj.ss.ss_family = AF_UNSPEC;
    }
    } // switch
}

} // anonymous namespace


CNetadapter::CNetadapter(){
    TRACE2(this, " Construct CNetadapter()") //
}

CNetadapter::~CNetadapter() {
    TRACE2(this, " Destruct CNetadapter()")
    this->free_adaptaddrs();
}

void CNetadapter::load() {
    // For the structure look at
    // REF:_https://docs.microsoft.com/en-us/windows/win32/api/iptypes/ns-iptypes-ip_adapter_addresses_lh
    TRACE2(this, " Executing load()")

    ULONG adapts_size{};
    // Get Adapters addresses required size. Check with adapts_size = 0
    // will fail with ERROR_BUFFER_OVERFLOW but returns required size.
    ULONG ret = umock::iphlpapi_h.GetAdaptersAddresses(
        AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER, nullptr,
        m_adapt_first, &adapts_size);
    if (ret != ERROR_BUFFER_OVERFLOW) {
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT +
            "MSG1120: failed to get buffer size for list of adapters (errid=" +
            std::to_string(ret) + ").");
    }

    this->free_adaptaddrs();

    // Allocate enough memory that size was detected above.
    m_adapt_first = static_cast<PIP_ADAPTER_ADDRESSES>(malloc(adapts_size));
    if (m_adapt_first == nullptr) {
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT +
            "MSG1121: failed to allocate memory for list of adapters.");
    }
    // Do the call that will actually return the info.
    ret = umock::iphlpapi_h.GetAdaptersAddresses(
        AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER, nullptr,
        m_adapt_first, &adapts_size);
    if (ret != ERROR_SUCCESS) {
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT +
            "MSG1122: failed to find list of adapters (errid=" +
            std::to_string(ret) + ").");
    }

    m_adapt_current = m_adapt_first;
    m_unicastaddr_current = m_adapt_current->FirstUnicastAddress;
}

bool CNetadapter::get_next() {
    // A network adapter has several IP addresses. First the address list must
    // be parsed and when at its end the next adapter must be selected. When
    // entering this function there is no synchronous information as given in a
    // loop (or two nested). I have to excamine the current state for the next
    // action. Writing down a state table on paper has helped me.
    TRACE2(this, " Executing get_next()")
    if (m_adapt_current == nullptr && m_unicastaddr_current == nullptr) {
        return false;
    }
    if (m_adapt_current == nullptr && m_unicastaddr_current != nullptr) {
        m_unicastaddr_current = nullptr;
        return false;
    }
    if (m_adapt_current != nullptr && m_unicastaddr_current == nullptr) {
        m_adapt_current = m_adapt_current->Next;
        if (m_adapt_current == nullptr) {
            return false;
        } else {
            m_unicastaddr_current = m_adapt_current->FirstUnicastAddress;
            return true;
        }
    }
    if (m_adapt_current != nullptr && m_unicastaddr_current != nullptr) {
        m_unicastaddr_current = m_unicastaddr_current->Next;
        if (m_unicastaddr_current == nullptr) {
            m_adapt_current = m_adapt_current->Next;
            if (m_adapt_current == nullptr) {
                return false;
            } else {
                m_unicastaddr_current = m_adapt_current->FirstUnicastAddress;
                return true;
            }
        } else {
            return true;
        }
    }
    throw std::invalid_argument(
        UPnPsdk_LOGEXCEPT +
        "MSG1123: Failed to get next network adapter entry. This should never "
        "come up and must be fixed. m_adapt_current=" +
        std::to_string(reinterpret_cast<uintptr_t>(m_adapt_current)) +
        ", m_unicastaddr_current=" +
        std::to_string(reinterpret_cast<uintptr_t>(m_unicastaddr_current)) +
        "\n");
}

std::string CNetadapter::name() const {
    TRACE2(this, " Executing name()")
    if (m_adapt_current == nullptr)
        return "";

    /* Partial fix for Windows: Friendly name is wchar string, but currently
     * compatible names are char string. For now try to convert it, which will
     * work with many (but not all) adapters. A full fix would require a lot of
     * big changes (using UTF-8 as planned). */

    // If I need this constant more than once, I'll make it global, Here it is
    // used for the friendly name of a network adapter and is compatible to old
    // gIF_NAME size. ('grep' is your friend)
    constexpr size_t LINE_SIZE{180};

    char if_name[LINE_SIZE]{};
    size_t i;
    wcstombs_s(&i, if_name, sizeof(if_name), m_adapt_current->FriendlyName,
               sizeof(if_name) - 1); // -1 so the appended '\0' doesn't fall
                                     // outside the allocated buffer
    return if_name;
}

SSockaddr CNetadapter::sockaddr() const {
    // TRACE maybe not usable with chained output.
    TRACE2(this, " Executing sockaddr()")
    SSockaddr saddrObj;

    if (m_adapt_current != nullptr) {
        // Copy address of the network adapter
        memcpy(&saddrObj.ss,
               reinterpret_cast<sockaddr_storage*>(
                   m_unicastaddr_current->Address.lpSockaddr),
               sizeof(saddrObj.ss));
    }
    return saddrObj; // Return as copy
}

SSockaddr CNetadapter::socknetmask() const {
    // TRACE maybe not usable with chained output.
    TRACE2(this, " Executing socknetmask()")
    SSockaddr saObj;
    if (m_adapt_current != nullptr) {
        bitnum_to_netmask(m_unicastaddr_current->Address.lpSockaddr->sa_family,
                          m_unicastaddr_current->OnLinkPrefixLength, saObj);
    }
    return saObj; // Return as copy
}


// Private helper methods
// ----------------------
//
void CNetadapter::free_adaptaddrs() noexcept {
    TRACE2(this, " Executing free_adaptaddrs()")
    if (m_adapt_first != nullptr) {
        free(m_adapt_first);
        m_adapt_first = nullptr;
    }
    m_adapt_current = nullptr;
    m_unicastaddr_current = nullptr;
}

} // namespace UPnPsdk
