// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-12-23
/*!
 * \file
 * \brief Definition of the Addrinfo class and free helper functions.
 */

#include <UPnPsdk/addrinfo.hpp>
#include <UPnPsdk/synclog.hpp>
#include <UPnPsdk/sockaddr.hpp>

#include <umock/netdb.hpp>
/// cond
#include <cstring>
/// endcond


namespace UPnPsdk {

// CAddrinfo class to wrap ::addrinfo() system calls
// =================================================
// On Microsoft Windows this needs to have Windows Sockets initialized with
// WSAStartup().

// Constructor for getting an address information with service name that can
// also be a port number string.
// -------------------------------------------------------------------------
CAddrinfo::CAddrinfo(std::string_view a_node, std::string_view a_service,
                     const int a_family, const int a_socktype,
                     const int a_flags, const int a_protocol)
    : m_node(a_node), m_service(a_service) {
    TRACE2(this, " Construct CAddrinfo() with extra service")
    this->set_ai_flags(a_family, a_socktype, a_flags, a_protocol);
}

// Constructor for getting an address information from only a netaddress.
// ----------------------------------------------------------------------
CAddrinfo::CAddrinfo(std::string_view a_node, const int a_family,
                     const int a_socktype, const int a_flags,
                     const int a_protocol)
    : m_node(a_node) {
    TRACE2(this, " Construct CAddrinfo() with netaddress")
    this->set_ai_flags(a_family, a_socktype, a_flags, a_protocol);
}


// Helper method for common tasks on different constructors
// --------------------------------------------------------
inline void CAddrinfo::set_ai_flags(const int a_family, const int a_socktype,
                                    const int a_flags,
                                    const int a_protocol) noexcept {
    // I cannot use the initialization list of the constructor because the
    // member order in the structure addrinfo is different on Linux, MacOS and
    // win32. I have to use the member names to initialize them, what's not
    // possible for structures in the constructors initialization list.
    m_hints.ai_flags = a_flags;
    m_hints.ai_family = a_family;
    m_hints.ai_socktype = a_socktype;
    m_hints.ai_protocol = a_protocol;
}


// Destructor
// ----------
/// \cond
CAddrinfo::~CAddrinfo() {
    TRACE2(this, " Destruct CAddrinfo()")
    this->free_addrinfo();
}
/// \endcond

// Private method to free allocated memory for address information
// ---------------------------------------------------------------
void CAddrinfo::free_addrinfo() noexcept {
    if (m_res != &m_hints) {
        TRACE2("syscall ::freeaddrinfo() with m_res = ", m_res)
        umock::netdb_h.freeaddrinfo(m_res);
        m_res = &m_hints;
        m_res_current = &m_hints;
    }
}


// Member access operator ->
// -------------------------
::addrinfo* CAddrinfo::operator->() const noexcept { return m_res_current; }


// Getter for the first entry of an address info from the operating system
// -----------------------------------------------------------------------
// Get address information with cached hints.
bool CAddrinfo::get_first() {
    TRACE2(this, " Executing CAddrinfo::get_first()")

    // Prepare input for ::getaddrinfo()
    std::string node, service;
    try {
        if (m_service.empty())
            split_addr_port(m_node, node, service);
        else
            split_addr_port(m_node + ":" + m_service, node, service);
    } catch (const std::range_error& ex) {
        m_error_msg =
            UPnPsdk_LOGWHAT + "MSG1128: catched next line ...\n" + ex.what();

        return false;
    }

    // syscall ::getaddrinfo() with prepared arguments
    // -----------------------------------------------
    ::addrinfo* new_res{nullptr}; // Result from ::getaddrinfo()
    const int ret = umock::netdb_h.getaddrinfo(
        (node.empty() ? nullptr : node.c_str()),
        (service.empty() ? nullptr : service.c_str()), &m_hints, &new_res);
#ifndef __APPLE__
    // std::format() since c++20 isn't supported on AppleClang 15, even on c++23
    TRACE2(this, " syscall ::getaddrinfo(" + node + ", " + service +
                     ") with new_res = " +
                     std::format("{:#x}", reinterpret_cast<uintptr_t>(new_res)))
#endif
    if (g_dbug) {
        // Very helpful for debugging to see what is given to ::getaddrinfo()
        char addrStr[INET6_ADDRSTRLEN]{};
        char servStr[NI_MAXSERV]{};
        if (ret == 0)
            ::getnameinfo(new_res->ai_addr,
                          static_cast<socklen_t>(new_res->ai_addrlen), addrStr,
                          sizeof(addrStr), servStr, sizeof(servStr),
                          NI_NUMERICHOST | NI_NUMERICSERV);
        // clang-format off
        UPnPsdk_LOGINFO << "MSG1111: syscall ::getaddrinfo("
            << (node.empty() ? "nullptr, " : "\"" + node + "\", ")
            << (service.empty() ? "nullptr, " : "\"" + service + "\", ")
            << &m_hints << ", " << &new_res
            << ") node=\"" << m_node << "\", "
            << (m_hints.ai_flags & AI_NUMERICHOST ? "AI_NUMERICHOST, " : "")
            << (m_hints.ai_flags & AI_NUMERICSERV ? "AI_NUMERICSERV, " : "")
            << (m_hints.ai_flags & AI_PASSIVE ? "AI_PASSIVE, " : "")
            << (m_hints.ai_family == AF_INET6 ? "AF_INET6" :
                    (m_hints.ai_family == AF_INET ? "AF_INET" :
                        (m_hints.ai_family == AF_UNSPEC ? "AF_UNSPEC" :
                            "m_hints.ai_family=" + std::to_string(m_hints.ai_family))))
            << ", "
            << (m_hints.ai_socktype == SOCK_STREAM ? "SOCK_STREAM" :
                    (m_hints.ai_socktype == SOCK_DGRAM ? "SOCK_DGRAM" :
                        (m_hints.ai_socktype == SOCK_RAW ? "SOCK_RAW" :
                            "socktype=" + std::to_string(m_hints.ai_socktype))))
            << (ret != 0
                ? ". Get GAI_ERROR(" + std::to_string(ret) + ")"
                : ". Get first \"" + std::string(addrStr) + "\" port "
                  + std::string(servStr)
                  + (new_res->ai_next == nullptr ? ", no more entries." : ", more entries..."))
            << '\n';
    }

    if (ret == EAI_SERVICE    /* Servname not supported for ai_socktype */
        || ret == EAI_NONAME  /* Node or service not known */
        || ret == EAI_MEMORY  /* Out of memory */
        || ret == EAI_AGAIN   /* The name server returned a temporary failure indication, try again later */
        /*! \todo Manage to use WSAEAFNOSUPPORT for EAI_ADDRFAMILY that isn't defined on win32. */
#ifndef _MSC_VER
        || ret == EAI_ADDRFAMILY /* Address family for NAME not supported */
#endif
        || ret == EAI_NODATA)  /* No address associated with hostname */ {
        // Error numbers definded in netdb.h.
        // Maybe an alphanumeric node name that cannot be resolved (e.g. by
        // DNS)? Anyway, the user has to decide what to do. Because this
        // depends on extern available DNS server the error can occur
        // unexpectedly at any time. We have no influence on it but I will give
        // an extended error message.
        m_error_msg = UPnPsdk_LOGWHAT + "MSG1112: errid(" +
             std::to_string(ret) + ")=\"" + ::gai_strerror(ret) + "\", " +
             ((m_hints.ai_family == AF_UNSPEC) ? "IPv?_" :
             ((m_hints.ai_family == AF_INET6) ? "IPv6_" : "IPv4_")) +
             ((m_hints.ai_flags & AI_NUMERICHOST) ? "numeric_host=\"" : "alphanum_name=\"") +
              m_node + "\", service=\"" +
              m_service + "\"" +
             ((m_hints.ai_flags & AI_PASSIVE) ? ", passive_listen" : "") +
             ((m_hints.ai_flags & AI_NUMERICHOST) ? "" : ", (maybe DNS query temporary failed?)");

             return false;
    }
    // clang-format on

    for (::addrinfo* res{new_res}; res != nullptr; res = res->ai_next) {
        // Different on platforms: Ubuntu & MacOS return protocol number, win32
        // returns 0. I just return what was used to call ::getaddrinfo().
        res->ai_protocol = m_hints.ai_protocol;
        //
        // Different on platforms: Ubuntu returns set flags, MacOS & win32
        // return 0. I just return what was used to call ::getaddrinfo().
        res->ai_flags = m_hints.ai_flags;
    }

    // If get_first() is called the second time then m_res still points to the
    // previous allocated memory. To avoid a memory leak it must be freed before
    // pointing to the new allocated memory.
    this->free_addrinfo();
    // finaly point to the new address information from the operating
    // system.
    m_res = new_res;
    m_res_current = new_res;

    return true;
}


// Getter for the next available address information
// -------------------------------------------------
bool CAddrinfo::get_next() noexcept {
    if (m_res_current->ai_next == nullptr) {
        // It doesn't matter if already pointing to m_hints. m_hints->ai_next is
        // also nullptr.
        m_res_current = &m_hints;
        return false;
    }
    m_res_current = m_res_current->ai_next;
    return true;
}


// Get netaddress with port from current selcted address information
// -----------------------------------------------------------------
std::string CAddrinfo::netaddrp() noexcept {
    if (m_res == &m_hints)
        return "";

    char addrStr[INET6_ADDRSTRLEN];
    char servStr[NI_MAXSERV];
    int ret = ::getnameinfo(m_res_current->ai_addr,
                            static_cast<socklen_t>(m_res_current->ai_addrlen),
                            addrStr, static_cast<socklen_t>(sizeof(addrStr)),
                            servStr, static_cast<socklen_t>(sizeof(servStr)),
                            NI_NUMERICHOST | NI_NUMERICSERV);
    if (ret != 0) {
        UPnPsdk_LOGERR "MSG1130: Failed to get name information: "
            << gai_strerror(ret) << ". Continue with empty netaddress \"\".\n";
        return "";
    }

    switch (m_res_current->ai_family) {
    case AF_INET6:
        return "[" + std::string(addrStr) + "]:" + std::string(servStr);
    case AF_INET:
        return std::string(addrStr) + ":" + std::string(servStr);
    case AF_UNSPEC:
        return ":" + std::string(servStr);
    }

    UPnPsdk_LOGERR "MSG1033: Unsupported address family "
        << m_res_current->ai_family
        << ". Continue with empty netaddress \"\".\n";
    return "";
}


// Get the socket address from current selcted address information
// ---------------------------------------------------------------
void CAddrinfo::sockaddr(SSockaddr& a_saddr) {
    if (m_res != &m_hints)
        memcpy(&a_saddr.ss, m_res_current->ai_addr, sizeof(a_saddr.ss));
}


// Get cached error message
// ------------------------
const std::string& CAddrinfo::what() const { return m_error_msg; }

} // namespace UPnPsdk
