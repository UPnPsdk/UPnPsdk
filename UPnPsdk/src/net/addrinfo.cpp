// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-12-06
/*!
 * \file
 * \brief Definition of the Addrinfo class and free helper functions.
 */

#include <UPnPsdk/addrinfo.hpp>
#include <UPnPsdk/synclog.hpp>
#include <UPnPsdk/sockaddr.hpp>

#include <umock/netdb.hpp>
#include <cstring>

namespace UPnPsdk {

namespace {

// Free function to check for a netaddress without port
// ----------------------------------------------------
/*! \brief Check for a [netaddress](\ref glossary_netaddr) and return its
 * address family
 * \ingroup upnplib-addrmodul
 * \code
 * // Usage e.g.:
 * if (is_netaddr("[2001:db8::1]") != AF_UNSPEC) { manage_given_netaddress(); }
 * if (is_netaddr("[2001:db8::1]", AF_INET) == AF_INET) { // nothing to do }
 * if (is_netaddr("[fe80::1%2]") == AF_INET6) { manage_link_local_addr(); }
 * \endcode
 *
 * Checks if a string is a netaddress without port and returns its address
 * family.
 *
 * \returns
 *  On success: Address family AF_INET6 or AF_INET the address belongs to\n
 *  On error: AF_UNSPEC, the address is alphanumeric (maybe a DNS name?)
 */
// I use the system function ::getaddrinfo() to check if the node string is
// acceptable. Using ::getaddrinfo() is needed to cover all special address
// features like scope id for link local addresses, Internationalized Domain
// Names, and so on.
sa_family_t is_netaddr(
    /// [in] string to check for a netaddress.
    const std::string& a_node,
    /// [in] optional: AF_INET6 or AF_INET to preset the address family to look
    /// for.
    const int a_addr_family = AF_UNSPEC) noexcept {
    // clang-format off
    TRACE("Executing is_netaddr(\"" + a_node + "\", " +
          (a_addr_family == AF_INET6 ? "AF_INET6" :
          (a_addr_family == AF_INET ? "AF_INET" :
          (a_addr_family == AF_UNSPEC ? "AF_UNSPEC" :
          std::to_string(a_addr_family)))) + ")")
    // clang-format on

    // The shortest numeric netaddress string is "[::]".
    if (a_node.size() < 4) { // noexcept
        return AF_UNSPEC;
    }

    // Provide resources for ::getaddrinfo()
    // AI_NUMERICHOST ensures that only numeric addresses accepted.
    std::string node;
    ::addrinfo hints{};
    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = a_addr_family;
    ::addrinfo* res{nullptr};

    // Check for ipv6 addresses and remove surounding brackets for
    // ::getaddrinfo().
    // front() and back() have undefined behavior with an empty string. Here
    // its size() is at least 4. Substr() throws exception out of range if pos
    // > size(). All this means that we cannot get an exception here.
    if (a_node.front() == '[' && a_node.back() == ']' &&
        (a_addr_family == AF_UNSPEC || a_addr_family == AF_INET6)) {
        node = a_node.substr(1, a_node.length() - 2);
        hints.ai_family = AF_INET6;

    } else if (a_node.find_first_of(":") != std::string::npos) {
        // Ipv6 addresses are already checked and here are only ipv4 addresses
        // and URL names possible. Both are not valid if they contain a colon.
        // find_first_of() does not throw an exception.
        return AF_UNSPEC;

    } else {
        node = a_node;
    }

    // Call ::getaddrinfo() to check the remaining node string.
    int rc = umock::netdb_h.getaddrinfo(node.c_str(), nullptr, &hints, &res);
    TRACE2("syscall ::getaddrinfo() with res = ", res)
    if (rc != 0) {
        TRACE2("syscall ::freeaddrinfo() with res = ", res)
        umock::netdb_h.freeaddrinfo(res);
        UPnPsdk_LOGINFO "MSG1116: syscall ::getaddrinfo(\""
            << node.c_str() << "\", nullptr, " << &hints << ", " << &res
            << "), (" << rc << ") " << gai_strerror(rc) << '\n';
        return AF_UNSPEC;
    }

    int af_family = res->ai_family;
    TRACE2("syscall ::freeaddrinfo() with res = ", res)
    umock::netdb_h.freeaddrinfo(res);
    // Guard different types on different platforms (win32); need to cast to
    // af_family (unsigned short).
    if (af_family < 0 || af_family > 65535) {
        return AF_UNSPEC;
    }
    return static_cast<sa_family_t>(af_family);
}

} // anonymous namespace


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
    TRACE2(this, " Construct CAddrinfo() with service")
    this->set_ai_flags(a_family, a_socktype, a_flags, a_protocol);
}

// Constructor for getting an address information from only a netaddress.
// ----------------------------------------------------------------------
CAddrinfo::CAddrinfo(std::string_view a_node, const int a_family,
                     const int a_socktype, const int a_flags,
                     const int a_protocol)
    : m_node(a_node) {
    // Just do a simple check for a possible port and split it from address
    // string. Detailed tests will be done with this->load().
    TRACE2(this, " Construct CAddrinfo() without service")

    size_t pos;
    // The smalest valid netaddress is "[::]".
    if (m_node.size() < 4 || (m_node.front() == '[' && m_node.back() == ']')) {
        // IPv6 without port, use given address string
        goto set_flags;
    }
    pos = m_node.rfind("]:"); // noexcept
    if (pos != m_node.npos) {
        // IPv6 with port, split the address string. substr() throws exception
        // std::out_of_range if pos > size(). We have at least 4 character. pos
        // is 0-based, size() is 1-based so with "]:" is size()==2, pos==0,
        // pos+2==size() does not throw.
        m_service = m_node.substr(pos + 2);
        m_node = m_node.substr(0, pos + 1);
        goto set_flags;
    }
    pos = m_node.find_last_of(']'); // noexcept
    if (pos != m_node.npos) {
        // Maybe IPv6 with any remaining character, split address string
        m_service = m_node.substr(pos + 1);
        m_node = m_node.substr(0, pos + 1);
        goto set_flags;
    }
    pos = m_node.find_last_of(':'); // noexcept
    if (pos != m_node.npos) {
        // IPv4 or URL name with port, split address string
        m_service = m_node.substr(pos + 1);
        m_node = m_node.substr(0, pos);
    }

set_flags:
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


// Setter to load an addrinfo from the operating system to the object
// ------------------------------------------------------------------
// Get address information with cached hints.
void CAddrinfo::load() {
    TRACE2(this, " Executing CAddrinfo::load()")

    // Local working copies: modified node, service, and hints to use for
    // syscall ::getaddrinfo().
    std::string node;
    std::string service;
    ::addrinfo hints{m_hints}; // user given opgtions have priority

    // Correct weak hints
    // ------------------
    // Always call UPnPsdk::is_netaddr() with AF_UNSPEC (default argument) to
    // check all types. I always set AI_NUMERICHOST if it match, no matter what
    // 'a_family' was requested by the caller. But if AF_UNSPEC was requested,
    // the found address family will also be set.
    const sa_family_t addr_family =
        is_netaddr(m_node, m_hints.ai_family); // noexcept
    if (addr_family != AF_UNSPEC) { // Here we have a numeric netaddress
        hints.ai_flags |= AI_NUMERICHOST;
        if (m_hints.ai_family == AF_UNSPEC) { // Correct ai_family, we know it
            hints.ai_family = addr_family;
        }
    }
    // Check node name
    // ---------------
    if (addr_family == AF_INET6) {
        // Here we have only ipv6 node strings representing a numeric ip
        // address (netaddress) without port that is at least "[::]". Remove
        // surounding brackets for ::getaddrinfo()
        node = m_node.substr(1, m_node.length() - 2);
    } else if (addr_family == AF_INET) {
        // Here we have a valid ipv4 neetaddress. It can be unmodified given to
        // ::getaddrinfo(). No need to set AI_NUMERICHOST, it's already done
        // with getting addr_family above.
        node = m_node;
    } else if (is_netaddr('[' + m_node + ']', AF_INET6) == AF_INET6) {
        // ipv6 addresses without brackets would be accepted by ::getaddrinfo()
        // but they are not valid netaddresses. So I make them invalid.
        hints.ai_flags |= AI_NUMERICHOST;
        node = m_node + "(no_brackets)";
    } else if (m_node.find_first_of("[]:") != m_node.npos) { // noexcept
        // An address string without port (m_node is without port) containing
        // one of theese characters cannot be a valid alphanumeric URL. It can
        // only be a numeric address (or be invalid).
        hints.ai_flags |= AI_NUMERICHOST;
        node = m_node;
    } else {
        // Here we have a non numeric node name (no netaddress). Is it a valid
        // (maybe alphanumeric) node name? ::getaddrinfo() shall decide it.
        node = m_node;
    }
    const char* c_node = node.empty() ? nullptr : node.c_str();
    std::string node_out = node.empty() ? "nullptr" : "\"" + node + "\"";

    // Check service/port
    // ------------------
    // I have to do this because ::getaddrinfo() does not detect wrong port
    // numbers >65535. It silently overruns to port number 0, 1, 2...
    try {
        to_port(m_service);
        // Valid numeric port number
        service = m_service;
        hints.ai_flags |= AI_NUMERICSERV;

    } catch (const std::out_of_range&) {
        // Valid numeric port number > 65535
        service = m_service + "(invalid)";
        hints.ai_flags |= AI_NUMERICSERV;

    } catch (const std::invalid_argument&) {
        // Any alphanumeric port name
        service = m_service;
    }
    if (service.empty()) {
        service = "0";
        hints.ai_flags |= AI_NUMERICSERV;
    }

    // syscall ::getaddrinfo() with prepared arguments
    // -----------------------------------------------
    ::addrinfo* new_res{nullptr}; // Result from ::getaddrinfo()
    const int ret =
        umock::netdb_h.getaddrinfo(c_node, service.c_str(), &hints, &new_res);
    TRACE2("syscall ::getaddrinfo() with new_res = ", new_res)

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
        UPnPsdk_LOGINFO << "MSG1111: syscall ::getaddrinfo(" << node_out
            << ", " << "\"" << service << "\", "
            << &hints << ", " << &new_res
            << ") node=\"" << m_node << "\", "
            << (hints.ai_flags & AI_NUMERICHOST ? "AI_NUMERICHOST, " : "")
            << (hints.ai_flags & AI_NUMERICSERV ? "AI_NUMERICSERV, " : "")
            << (hints.ai_flags & AI_PASSIVE ? "AI_PASSIVE, " : "")
            << (hints.ai_family == AF_INET6 ? "AF_INET6" :
                    (hints.ai_family == AF_INET ? "AF_INET" :
                        (hints.ai_family == AF_UNSPEC ? "AF_UNSPEC" :
                            "hints.ai_family=" + std::to_string(hints.ai_family))))
            << (ret != 0
                ? ". Get GAI_ERROR(" + std::to_string(ret) + ")"
                : ". Get first \"" + std::string(addrStr) + "\", port "
                  + std::string(servStr)) << " (maybe more)\n";
    }
    if (ret == EAI_SERVICE    /* Servname not supported for ai_socktype */
        || ret == EAI_NONAME  /* Node or service not known */
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
        throw std::runtime_error(UPnPsdk_LOGEXCEPT + "MSG1112: errid(" +
             std::to_string(ret) + ")=\"" + ::gai_strerror(ret) + "\", " +
             ((hints.ai_family == AF_UNSPEC) ? "IPv?_" :
             ((hints.ai_family == AF_INET6) ? "IPv6_" : "IPv4_")) +
             ((hints.ai_flags & AI_NUMERICHOST) ? "numeric_host=\"" : "alphanum_name=\"") +
              node + "\", service=\"" +
              service + "\"" +
             ((hints.ai_flags & AI_PASSIVE) ? ", passive_listen" : "") +
             ((hints.ai_flags & AI_NUMERICHOST) ? "" : ", (maybe DNS query temporary failed?)"));
    }
    // clang-format on

    if (ret != 0) {
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT +
            "MSG1037: Failed to get address information: errid(" +
            std::to_string(ret) + ")=\"" + ::gai_strerror(ret) + "\"\n");
    }

    for (::addrinfo* res{new_res}; res != nullptr; res = res->ai_next) {
        // First ai_socktype is different on platforms: man getsockaddr says
        // "Specifying 0 in hints.ai_socktype indicates that socket addresses
        // of any type can be returned". Linux returns SOCK_STREAM first, MacOS
        // returns SOCK_DGRAM first and win32 returns 0.
        // if (hints.ai_socktype == 0)
        //     res->ai_socktype = 0;
        //
        // Different on platforms: Ubuntu & MacOS return protocol number, win32
        // returns 0. I just return what was used to call ::getaddrinfo().
        res->ai_protocol = hints.ai_protocol;
        //
        // Different on platforms: Ubuntu returns set flags, MacOS & win32
        // return 0. I just return what was used to call ::getaddrinfo().
        res->ai_flags = hints.ai_flags;
    }
    // Man getaddrinfo says: "If service is NULL, then the port number of the
    // returned socket addresses will be left uninitialized." The service is
    // never set to NULL so we always have a defined service/portnumber.
    // if (service.empty())
    //     // port for AF_INET6 is also valid for AF_INET
    //     reinterpret_cast<sockaddr_in6*>(new_res->ai_addr)->sin6_port = 0;

    // If load() is called the second time then m_res still points to the
    // previous allocated memory. To avoid a memory leak it must be freed
    // before pointing to the new allocated memory.
    this->free_addrinfo();
    // finaly point to the new address information from the operating
    // system.
    m_res = new_res;
    m_res_current = new_res;
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
        UPnPsdk_LOGERR "MSG1032: Failed to get name information: "
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

} // namespace UPnPsdk
