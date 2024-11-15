// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-11-16
/*!
 * \file
 * \brief Manage information from the operating system about network interfaces.
 */

#include <UPnPsdk/netifinfo.hpp>
#include <UPnPsdk/synclog.hpp>
#include <umock/ifaddrs.hpp>
#include <cstring>
#include <net/if.h>

namespace UPnPsdk {

CIfaddrs::CIfaddrs(){
    TRACE2(this, " Construct CIfaddrs()") //
}

CIfaddrs::~CIfaddrs() {
    TRACE2(this, " Destruct CIfaddrs()")
    this->free_ifaddrs();
}

void CIfaddrs::load() {
    TRACE2(this, " Executing load()")

    // Get system interface addresses.
    this->free_ifaddrs();
    if (umock::ifaddrs_h.getifaddrs(&m_ifa_first) != 0) {
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT +
            "MSG1119: Failed to get information from the network interfaces: " +
            std::string(std::strerror(errno)) + '\n');
    }

    for (m_ifa_current = m_ifa_first; m_ifa_current != nullptr;
         m_ifa_current = m_ifa_current->ifa_next) {
        if (is_valid_if())
            break; // Does not increment m_ifa_current
    }
}

bool CIfaddrs::get_next() {
    TRACE2(this, " Executing get_next()")
    if (m_ifa_current == nullptr)
        return false;

    m_ifa_current = m_ifa_current->ifa_next;

    for (; m_ifa_current != nullptr; m_ifa_current = m_ifa_current->ifa_next) {
        if (is_valid_if())
            return true; // Does not increment m_ifa_current
    }
    return false;
}

std::string CIfaddrs::name() const {
    TRACE2(this, " Executing name()")
    if (m_ifa_current == nullptr)
        return "";
    return m_ifa_current->ifa_name;
}

#if 0
// I code IP Version-Independent, so this method is not provided.
//
sa_family_t CIfaddrs::in_family() { // noexcept
    TRACE2(this, " Executing in_family()")
    if (m_ifa_current == nullptr)
        return AF_UNSPEC;
    return m_ifa_current->ifa_addr->sa_family;
}
#endif

SSockaddr CIfaddrs::sockaddr() const {
    // TRACE maybe not usable with chained output.
    TRACE2(this, " Executing sockaddr()")
    SSockaddr saddr;
    if (m_ifa_current != nullptr) {
        // Copy address of interface
        memcpy(&saddr.ss,
               reinterpret_cast<sockaddr_storage*>(m_ifa_current->ifa_addr),
               sizeof(saddr.ss));
    }
    return saddr; // Return as copy
}

SSockaddr CIfaddrs::socknetmask() const {
    // TRACE maybe not usable with chained output.
    TRACE2(this, " Executing socknetmask()")
    SSockaddr saddr;
    if (m_ifa_current != nullptr) {
        // Copy netmask of interface
        memcpy(&saddr.ss,
               reinterpret_cast<sockaddr_storage*>(m_ifa_current->ifa_netmask),
               sizeof(saddr.ss));
    }
    return saddr; // Return as copy
}


// Private helper methods
// ----------------------
//
void CIfaddrs::free_ifaddrs() noexcept {
    TRACE2(this, " Executing free_ifaddrs()")
    if (m_ifa_first != nullptr) {
        freeifaddrs(m_ifa_first);
        m_ifa_first = nullptr;
    }
    m_ifa_current = nullptr;
}

inline bool CIfaddrs::is_valid_if() const noexcept {
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
