// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-12-21
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
#include <net/if.h>
/// \endcond

namespace UPnPsdk {

CNetadapter::CNetadapter(){
    TRACE2(this, " Construct CNetadapter()") //
}

CNetadapter::~CNetadapter() {
    TRACE2(this, " Destruct CNetadapter()")
    this->free_ifaddrs();
}

void CNetadapter::get_first() {
    TRACE2(this, " Executing CNetadapter::get_first()")

    // Get system adapters addresses.
    this->free_ifaddrs();
    if (umock::ifaddrs_h.getifaddrs(&m_ifa_first) != 0) {
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT +
            "MSG1119: Failed to get information from the network adapters: " +
            std::string(std::strerror(errno)) + '\n');
    }

    for (m_ifa_current = m_ifa_first; m_ifa_current != nullptr;
         m_ifa_current = m_ifa_current->ifa_next) {
        if (is_valid_if())
            break; // Does not increment m_ifa_current
    }
}

bool CNetadapter::get_next() {
    TRACE2(this, " Executing CNetadapter::get_next()")
    if (m_ifa_current == nullptr)
        return false;

    m_ifa_current = m_ifa_current->ifa_next;

    for (; m_ifa_current != nullptr; m_ifa_current = m_ifa_current->ifa_next) {
        if (is_valid_if())
            return true; // Does not increment m_ifa_current
    }
    return false;
}

std::string CNetadapter::name() const {
    TRACE2(this, " Executing CNetadapter::name()")
    if (m_ifa_current == nullptr)
        return "";
    return m_ifa_current->ifa_name;
}

#if 0
// Due to RFC4038 I code IP Version-Independent, so this method is not provided.
//
sa_family_t CNetadapter::in_family() { // noexcept
    TRACE2(this, " Executing CNetadapter::in_family()")
    if (m_ifa_current == nullptr)
        return AF_UNSPEC;
    return m_ifa_current->ifa_addr->sa_family;
}
#endif

void CNetadapter::sockaddr(SSockaddr& a_saddr) const {
    TRACE2(this, " Executing CNetadapter::sockaddr()")
    if (m_ifa_current != nullptr) {
        // Copy address of the network adapter
        memcpy(&a_saddr.ss,
               reinterpret_cast<sockaddr_storage*>(m_ifa_current->ifa_addr),
               sizeof(a_saddr.ss));
    }
}

void CNetadapter::socknetmask(SSockaddr& a_snetmask) const {
    TRACE2(this, " Executing CNetadapter::socknetmask()")
    if (m_ifa_current != nullptr) {
        // Copy netmask of the network adapter
        memcpy(&a_snetmask.ss,
               reinterpret_cast<sockaddr_storage*>(m_ifa_current->ifa_netmask),
               sizeof(a_snetmask.ss));
    }
}

unsigned int CNetadapter::index() const {
    TRACE2(this, " Executing CNetadapter::index()")
    if (m_ifa_current == nullptr)
        return 0;
    return umock::net_if_h.if_nametoindex(m_ifa_current->ifa_name);
}


// Private helper methods
// ----------------------
//
void CNetadapter::free_ifaddrs() noexcept {
    TRACE2(this, " Executing CNetadapter::free_ifaddrs()")
    if (m_ifa_first != nullptr) {
        freeifaddrs(m_ifa_first);
        m_ifa_first = nullptr;
    }
    m_ifa_current = nullptr;
}

inline bool CNetadapter::is_valid_if() const noexcept {
    // Accept IFF_LOOPBACK or up AF_INET6/AF_INET interfaces with address
    // (e.g. not bonded) and that support MULTICAST.
    if (m_ifa_current == nullptr || m_ifa_current->ifa_addr == nullptr)
        return false;

    if (m_ifa_current->ifa_flags & IFF_LOOPBACK &&
        (m_ifa_current->ifa_addr->sa_family == AF_INET6 ||
         m_ifa_current->ifa_addr->sa_family == AF_INET))
        return true;

    if ((m_ifa_current->ifa_addr->sa_family == AF_INET6 ||
         m_ifa_current->ifa_addr->sa_family == AF_INET) &&
        m_ifa_current->ifa_flags & IFF_UP &&
        m_ifa_current->ifa_flags & IFF_MULTICAST)
        return true;

    return false;
}

} // namespace UPnPsdk
