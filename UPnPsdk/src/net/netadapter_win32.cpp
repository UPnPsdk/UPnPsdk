// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-02-21
/*!
 * \file
 * \brief Manage information from Microsoft Windows about network adapters.
 */

#include <UPnPsdk/netadapter.hpp>
#include <UPnPsdk/addrinfo.hpp>
#include <UPnPsdk/synclog.hpp>
/// \cond
#include <umock/iphlpapi.hpp>
/// \endcond


namespace UPnPsdk {

CNetadapter_platform::CNetadapter_platform(){
    TRACE2(this, " Construct CNetadapter_platform()") //
}

CNetadapter_platform::~CNetadapter_platform() {
    TRACE2(this, " Destruct CNetadapter_platform()")
    this->free_adaptaddrs();
}

void CNetadapter_platform::get_first() {
    // For the structure look at
    // REF:_https://docs.microsoft.com/en-us/windows/win32/api/iptypes/ns-iptypes-ip_adapter_addresses_lh
    TRACE2(this, " Executing CNetadapter_platform::get_first()")

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

bool CNetadapter_platform::get_next() {
    // A network adapter has several IP addresses. First the address list must
    // be parsed and when at its end the next adapter must be selected. When
    // entering this function there is no synchronous information as given in a
    // loop (or two nested). I have to excamine the current state for the next
    // action. Writing down a state table on paper has helped me.
    TRACE2(this, " Executing CNetadapter_platform::get_next()")
    if (m_adapt_current == nullptr) {
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

bool CNetadapter_platform::find_first(const std::string& a_name_or_addr) {
    TRACE2(this, " Executing CNetadapter_platform::find_first(" +
                     a_name_or_addr + ")")
    // First look for a local network adapter name
    this->reset();
    do {
        if (this->name() == a_name_or_addr)
            return true;
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
    SSockaddr sa_nadObj{};
    this->reset();
    do {
        this->sockaddr(sa_nadObj);
        if (sa_nadObj == sa_inputObj)
            return true;
    } while (this->get_next());

    return false;
}

bool CNetadapter_platform::find_first(unsigned int a_index) {
    TRACE2(this, " Executing CNetadapter_platform::find_first(" +
                     std::to_string(a_index) + ")")
    this->reset();
    do {
        if (this->index() == a_index)
            return true;
    } while (this->get_next());

    m_adapt_current = nullptr;
    m_unicastaddr_current = nullptr;
    return false;
}

std::string CNetadapter_platform::name() const {
    TRACE2(this, " Executing CNetadapter_platform::name()")
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

void CNetadapter_platform::sockaddr(SSockaddr& a_saddr) const {
    TRACE2(this, " Executing CNetadapter_platform::sockaddr()")
    if (m_adapt_current == nullptr) {
        // If no information found then return an empty netaddress.
        a_saddr = "";
    } else {
        // Copy address of the network adapter
        memcpy(&a_saddr.ss,
               reinterpret_cast<sockaddr_storage*>(
                   m_unicastaddr_current->Address.lpSockaddr),
               sizeof(a_saddr.ss));
    }
}

void CNetadapter_platform::socknetmask(SSockaddr& a_snetmask) const {
    TRACE2(this, " Executing CNetadapter_platform::socknetmask()")
    if (m_adapt_current != nullptr) {
        bitmask_to_netmask(reinterpret_cast<sockaddr_storage*>(
                               m_unicastaddr_current->Address.lpSockaddr),
                           m_unicastaddr_current->OnLinkPrefixLength,
                           a_snetmask);
    }
}

unsigned int CNetadapter_platform::bitmask() const {
    TRACE2(this, " Executing CNetadapter_platform::bitmask()")
    if (m_adapt_current == nullptr)
        return 0;

    return m_unicastaddr_current->OnLinkPrefixLength;
}

unsigned int CNetadapter_platform::index() const {
    TRACE2(this, " Executing CNetadapter_platform::index()")
    if (m_adapt_current == nullptr)
        return 0;
    // No matter if the adapter supports only IPv4 or only IPv6 or both, the
    // adapter index should be always the same.
    if (m_adapt_current->IfIndex != 0) // IPv4 interface
        return m_adapt_current->IfIndex;
    if (m_adapt_current->Ipv6IfIndex != 0) // IPv6 interface
        return m_adapt_current->Ipv6IfIndex;
    return 0; // neither IPv4 nor IPv6
}


// Private helper methods
// ----------------------
//
void CNetadapter_platform::free_adaptaddrs() noexcept {
    TRACE2(this, " Executing CNetadapter_platform::free_adaptaddrs()")
    if (m_adapt_first != nullptr) {
        free(m_adapt_first);
        m_adapt_first = nullptr;
    }
    m_adapt_current = nullptr;
    m_unicastaddr_current = nullptr;
}

inline void CNetadapter_platform::reset() noexcept {
    TRACE2(this, " Executing CNetadapter_platform::reset()")
    m_adapt_current = m_adapt_first;
    m_unicastaddr_current = m_adapt_current->FirstUnicastAddress;
}

} // namespace UPnPsdk
