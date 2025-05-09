// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-03-25
/*!
 * \file
 * \brief Manage information from Unix like platforms about network adapters.
 */

#include <UPnPsdk/netadapter.hpp>
#include <UPnPsdk/synclog.hpp>
/// \cond
#include <umock/ifaddrs.hpp>
#include <umock/net_if.hpp>
#include <cstring>
/// \endcond

namespace UPnPsdk {

CNetadapter_platform::CNetadapter_platform(){
    TRACE2(this, " Construct CNetadapter_platform()") //
}

CNetadapter_platform::~CNetadapter_platform() {
    TRACE2(this, " Destruct CNetadapter_platform()")
    this->free_ifaddrs();
}

void CNetadapter_platform::get_first() {
    TRACE2(this, " Executing CNetadapter_platform::get_first()")

    // Get system adapters addresses.
    this->free_ifaddrs();
    if (getifaddrs(&m_ifa_first) != 0) {
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG1119") "Failed to get information from "
                                         "the network adapters: " +
            std::string(std::strerror(errno)) + '\n');
    }
    UPnPsdk_LOGINFO("MSG1132") "syscall ::getifaddrs() gets " << m_ifa_first
                                                              << "\n";
    this->reset();
}

bool CNetadapter_platform::get_next() {
    TRACE2(this, " Executing CNetadapter_platform::get_next()")
    if (m_ifa_current == nullptr)
        return false;

    m_ifa_current = m_ifa_current->ifa_next;

    for (; m_ifa_current != nullptr; m_ifa_current = m_ifa_current->ifa_next) {
        if (is_valid_if(m_ifa_current))
            return true; // Does not increment m_ifa_current
    }
    return false;
}

unsigned int CNetadapter_platform::index() const {
    TRACE2(this, " Executing CNetadapter_platform::index()")
    return (m_ifa_current == nullptr)
               ? 0
               : umock::net_if_h.if_nametoindex(m_ifa_current->ifa_name);
}

std::string CNetadapter_platform::name() const {
    TRACE2(this, " Executing CNetadapter_platform::name()")
    return (m_ifa_current == nullptr) ? "" : m_ifa_current->ifa_name;
}

#if 0
// Due to RFC4038 I code IP Version-Independent, so this method is not provided.
//
sa_family_t CNetadapter_platform::in_family() { // noexcept
    TRACE2(this, " Executing CNetadapter_platform::in_family()")
    if (m_ifa_current == nullptr)
        return AF_UNSPEC;
    return m_ifa_current->ifa_addr->sa_family;
}
#endif

void CNetadapter_platform::sockaddr(SSockaddr& a_saddr) const {
    TRACE2(this, " Executing CNetadapter_platform::sockaddr()")
    if (m_ifa_current == nullptr) {
        // If no information found then return an empty netaddress.
        a_saddr = "";
    } else {
        // Copy address of the network adapter.
        a_saddr = reinterpret_cast<sockaddr_storage&>(*m_ifa_current->ifa_addr);
    }
}

void CNetadapter_platform::socknetmask(SSockaddr& a_snetmask) const {
    TRACE2(this, " Executing CNetadapter_platform::socknetmask()")
    if (m_ifa_current != nullptr) {
        // Copy netmask of the network adapter
        a_snetmask =
            reinterpret_cast<sockaddr_storage&>(*m_ifa_current->ifa_netmask);
    }
}


unsigned int CNetadapter_platform::bitmask() const {
    TRACE2(this, " Executing CNetadapter_platform::bitmask()")
    if (m_ifa_current == nullptr)
        return 0;

    return netmask_to_bitmask(reinterpret_cast<const ::sockaddr_storage*>(
        m_ifa_current->ifa_netmask));
}


// Private helper methods
// ----------------------
//
void CNetadapter_platform::free_ifaddrs() noexcept {
    TRACE2(this, " Executing CNetadapter::free_ifaddrs()")
    if (m_ifa_first != nullptr) {
        UPnPsdk_LOGINFO("MSG1116") "syscall ::freeifaddrs(" << m_ifa_first
                                                            << ")\n";
        freeifaddrs(m_ifa_first);
        m_ifa_first = nullptr;
    }
    m_ifa_current = nullptr;
}

inline bool
CNetadapter_platform::is_valid_if(const ifaddrs* a_ifa) const noexcept {
    // Accept IFF_LOOPBACK or up AF_INET6/AF_INET interfaces with address
    // (e.g. not bonded) and that support MULTICAST.
    if (a_ifa == nullptr || a_ifa->ifa_addr == nullptr)
        return false;

    if (a_ifa->ifa_flags & IFF_LOOPBACK &&
        (a_ifa->ifa_addr->sa_family == AF_INET6 ||
         a_ifa->ifa_addr->sa_family == AF_INET))
        return true;

    if ((a_ifa->ifa_addr->sa_family == AF_INET6 ||
         a_ifa->ifa_addr->sa_family == AF_INET) &&
        a_ifa->ifa_flags & IFF_UP && a_ifa->ifa_flags & IFF_MULTICAST)
        return true;

    return false;
}

void CNetadapter_platform::reset() noexcept {
    TRACE2(this, " Executing CNetadapter_platform::reset()")
    m_ifa_current = nullptr;
    // m_ifa_first is not necessary a valid entry. I have to look for the first
    // valid entry.
    for (ifaddrs* ifa_current = m_ifa_first; ifa_current != nullptr;
         ifa_current = ifa_current->ifa_next) {
        if (is_valid_if(ifa_current)) {
            m_ifa_current = ifa_current;
            break; // Does not increment ifa_current
        }
    }
}

} // namespace UPnPsdk
