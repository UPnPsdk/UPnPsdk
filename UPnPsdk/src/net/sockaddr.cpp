// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-12-05
/*!
 * \file
 * \brief Definition of the Sockaddr class and some free helper functions.
 */

#include <UPnPsdk/sockaddr.hpp>
#include <UPnPsdk/global.hpp>
#include <UPnPsdk/synclog.hpp>
#include <umock/netdb.hpp>
/// \cond
#include <cstring>
/// \endcond

namespace UPnPsdk {

namespace {

// Free function to get the address string from a sockaddr structure
// -----------------------------------------------------------------
/*! \brief Get the [netaddress](\ref glossary_netaddr) without port from a
 * sockaddr structure
 * \ingroup upnplib-addrmodul
 * \code
 * // Usage e.g.:
 * ::sockaddr_storage saddr{};
 * std::cout << "netaddress is " << to_netaddr(saddr) << "\n";
 * \endcode
 */
std::string to_netaddr(const ::sockaddr_storage& a_sockaddr) noexcept {
    // TRACE("Executing to_netaddr()") // not usable in chained output.

    // Accept nameinfo only for supported address families.
    switch (a_sockaddr.ss_family) {
    case AF_INET6:
    case AF_INET:
        break;
    case AF_UNSPEC:
        return "";
    default:
        UPnPsdk_LOGERR "MSG1129: Unsupported address family "
            << std::to_string(a_sockaddr.ss_family)
            << ". Continue with empty netaddress \"\".\n";
        return "";
    }

    char addrStr[INET6_ADDRSTRLEN]{};
    int ret = ::getnameinfo(reinterpret_cast<const sockaddr*>(&a_sockaddr),
                            sizeof(a_sockaddr), addrStr, sizeof(addrStr),
                            nullptr, 0, NI_NUMERICHOST);
    if (ret != 0) {
        UPnPsdk_LOGERR "MSG1036: Failed to get netaddress with address family "
            << std::to_string(a_sockaddr.ss_family) << ": "
            << ::gai_strerror(ret)
            << ". Continue with empty netaddress \"\".\n";
        return "";
    }

    // Next throws 'std::length_error' if the length of the constructed
    // std::string would exceed max_size(). This should never happen with given
    // lengths of addrStr (promise noexcept).
    if (a_sockaddr.ss_family == AF_INET6)
        return '[' + std::string(addrStr) + ']';

    return std::string(addrStr);
}


// Free function to get the address string with port from a sockaddr structure
// ---------------------------------------------------------------------------
/*! \brief Get the [netaddress](\ref glossary_netaddr) with port from a sockaddr
 * structure
 * \ingroup upnplib-addrmodul
 * \code
 * // Usage e.g.:
 * ::sockaddr_storage ss{};
 * std::cout << "netaddress is " << to_netaddrp(ss) << "\n";
 * \endcode
 */
std::string to_netaddrp(const ::sockaddr_storage& a_sockaddr) noexcept {
    // TRACE("Executing to_addrport_str()") // not usable in chained output.
    //
    // sin_port and sin6_port are on the same memory location (union of the
    // structures) so I can use it for AF_INET and AF_INET6.
    // 'std::to_string()' may throw 'std::bad_alloc' from the std::string
    // constructor. It is a fatal error that violates the promise to noexcept
    // and immediately terminates the propgram. This is intentional because the
    // error cannot be handled.
    switch (a_sockaddr.ss_family) {
    case AF_INET6:
    case AF_INET:
    case AF_UNSPEC:
        return to_netaddr(a_sockaddr) + ":" +
               std::to_string(
                   ntohs(reinterpret_cast<const ::sockaddr_in6*>(&a_sockaddr)
                             ->sin6_port));
    }
    return to_netaddr(a_sockaddr);
}


// Free function to logical compare two sockaddr structures
// --------------------------------------------------------
/*! \brief logical compare two sockaddr structures
 * \ingroup upnplib-addrmodul
 *
 * To have a logical equal socket address we compare the address family, the ip
 * address and the port.
 *
 * \returns
 *  \b true if socket addresses are logical equal\n
 *  \b false otherwise
 */
bool sockaddrcmp(const ::sockaddr_storage* a_ss1,
                 const ::sockaddr_storage* a_ss2) noexcept {
    // To have a logical equal socket address we compare the address family,
    // the ip address, the port and the scope.
    // Throws no exception.
    if (a_ss1 == nullptr && a_ss2 == nullptr)
        return true;
    if (a_ss1 == nullptr || a_ss2 == nullptr)
        return false;

    switch (a_ss1->ss_family) {
    case AF_UNSPEC:
        if (a_ss2->ss_family != AF_UNSPEC)
            return false;
        break;

    case AF_INET6: {
        // I compare ipv6 addresses which are stored in a 16 byte array
        // (unsigned char s6_addr[16]). So we have to use memcmp() for
        // comparison.
        const ::sockaddr_in6* const s6_addr1 =
            reinterpret_cast<const ::sockaddr_in6*>(a_ss1);
        const ::sockaddr_in6* const s6_addr2 =
            reinterpret_cast<const ::sockaddr_in6*>(a_ss2);

        if (a_ss2->ss_family != AF_INET6 ||
            ::memcmp(&s6_addr1->sin6_addr, &s6_addr2->sin6_addr,
                     sizeof(in6_addr)) != 0 ||
            s6_addr1->sin6_port != s6_addr2->sin6_port ||
            s6_addr1->sin6_scope_id != s6_addr2->sin6_scope_id)
            return false;
    } break;

    case AF_INET: {
        const ::sockaddr_in* const s_addr1 =
            reinterpret_cast<const ::sockaddr_in*>(a_ss1);
        const ::sockaddr_in* const s_addr2 =
            reinterpret_cast<const ::sockaddr_in*>(a_ss2);

        if (a_ss2->ss_family != AF_INET ||
            s_addr1->sin_addr.s_addr != s_addr2->sin_addr.s_addr ||
            s_addr1->sin_port != s_addr2->sin_port)
            return false;
    } break;

    default:
        return false;
    }

    return true;
}

} // anonymous namespace


// Free function to get the port number from a string
// --------------------------------------------------
in_port_t to_port(const std::string& a_port_str) {
    TRACE("Executing to_port() with port=\"" + a_port_str + "\"")
    bool nonzero{false};
    int port;

    // Only non empty strings. I have to check this to avoid stoi() exception
    // below.
    if (a_port_str.empty())
        // An empty port string is defined to be the unknown port 0.
        return 0;

    // Now we check if the string are all digit characters
    for (char ch : a_port_str) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            goto exit_fail;
        } else if (ch != '0') {
            nonzero = true;
        }
    }

    // Only strings with max. 5 char may be valid (uint16_t has max. 65535).
    if (a_port_str.length() > 5) {
        if (nonzero)
            goto exit_overrun; // value valid but more than 5 char.
        else
            goto exit_fail; // string is all zero with more than 5 char.
    }

    // Valid positive number but is it within the port range (uint16_t)?
    // stoi() may throw std::invalid_argument if no conversion could be
    // performed or std::out_of_range. But with the prechecked number string
    // this should never be thrown.
    port = std::stoi(a_port_str);
    if (port > 65535)
        goto exit_overrun;

    // Type cast from int is no problem because the port value is checked to be
    // 0..65535 so it always fit into in_port_t(uint16_t).
    return static_cast<in_port_t>(port);


// Both following exceptions are derived from std::logic_error() so that can be
// used to catch any of them.
exit_overrun:
    throw std::out_of_range(UPnPsdk_LOGEXCEPT +
                            "MSG1127: Valid number string \"" + a_port_str +
                            "\" is out of port range 0..65535.");
exit_fail:
    throw std::invalid_argument(
        UPnPsdk_LOGEXCEPT + "MSG1128: Failed to get port number for string \"" +
        a_port_str + "\".");
}


// Specialized sockaddr_structure
// ==============================
// Constructor
SSockaddr::SSockaddr(){
    TRACE2(this, " Construct SSockaddr()") //
}

// Destructor
SSockaddr::~SSockaddr() {
    TRACE2(this, " Destruct SSockaddr()")
    // Destroy structure
    ::memset(&m_sa_union, 0xAA, sizeof(m_sa_union));
}

// Get reference to the sockaddr_storage structure.
// Only as example, we don't use it.
// SSockaddr::operator const ::sockaddr_storage&() const {
//     return this->ss;
// }

// Copy constructor
// ----------------
SSockaddr::SSockaddr(const SSockaddr& that) {
    TRACE2(this, " Construct copy SSockaddr()")
    m_sa_union = that.m_sa_union;
}

// Copy assignment operator
// ------------------------
SSockaddr& SSockaddr::operator=(SSockaddr that) {
    TRACE2(this,
           " Executing SSockaddr::operator=(SSockaddr) (struct assign op).")
    std::swap(m_sa_union, that.m_sa_union);

    return *this;
}

// Assignment operator= to set socket address from string.
// -------------------------------------------------------
// For port conversion:
// Don't use '::htons' (with colons) instead of 'htons', MacOS don't like it.
// 'sin6_port' is also 'sin_port' due to union.
//
// Unique pattern recognition, port delimiter is always ':'
//                  Starting with '['
// Pattern e.g.: [2001:db8::1]:50001
//               [2001:db8::2]:
//               [2001:db8::3]
//                  Starting with ':' and is port
//               :50002
//                  Containing '.'
//               127.0.0.4:50004
//               127.0.0.5:
//               127.0.0.6
//                  Is port
//               50004
//                  Remaining
//               2001:db8::7
void SSockaddr::operator=(const std::string& a_addr_str) {
    TRACE2(this, " Executing SSockaddr::operator=(" + a_addr_str + ")")
    // An empty address string clears the address storage.
    if (a_addr_str.empty()) {
        ::memset(&m_sa_union, 0, sizeof(m_sa_union));
        return;
    }

    std::string addr_str;
    std::string serv_str;

    size_t pos{};
    if (a_addr_str.length() < 2) {
        // The shortest possible ip address is "::". This helps to avoid string
        // exceptions 'out_of_range'.
        addr_str = a_addr_str; // ::getaddrinfo() shall look what to do.

    } else if (a_addr_str.front() == '[') {
        // Starting with '[', split address if required
        if ((pos = a_addr_str.find("]:")) != std::string::npos) {
            addr_str = a_addr_str.substr(0, pos + 1); // Get IP address
            serv_str = a_addr_str.substr(pos + 2); // Get port string
        } else {
            addr_str = a_addr_str; // Get IP address
            serv_str = std::to_string(ntohs(m_sa_union.sin6.sin6_port));
        }

    } else if (a_addr_str.front() == ':') {
        // Starting with ':' and is port
        try {
            in_port_t port = to_port(a_addr_str.substr(1));
            // Only service given, set only port.
            m_sa_union.sin6.sin6_port = htons(port);
            return;
        } catch (const std::invalid_argument&) {
            addr_str = a_addr_str;
        }
    } else if (a_addr_str.find_first_of('.') != std::string::npos) {
        // Containing '.'
        if ((pos = a_addr_str.find_last_of(':')) != std::string::npos) {
            addr_str = a_addr_str.substr(0, pos); // Get IP address
            serv_str = a_addr_str.substr(pos + 1); // Get port string
        } else {
            addr_str = a_addr_str;
            serv_str = std::to_string(ntohs(m_sa_union.sin6.sin6_port));
        }
    } else {
        // Is port
        try {
            in_port_t port = to_port(a_addr_str);
            // Only service given, set only port.
            m_sa_union.sin6.sin6_port = htons(port);
            return;
        } catch (const std::invalid_argument&) {
            // Remaining
            addr_str = a_addr_str;
            serv_str = std::to_string(ntohs(m_sa_union.sin6.sin6_port));
        }
    }
    // std::cout << "DEBUG: addr_str \"" << addr_str << "\", serv_str \""
    //           << serv_str << "\"\n";

    // Check for valid port. ::getaddrinfo accepts invalid ports > 65535.
    to_port(serv_str); // Will throw an exception.

    // remove surounding brackets if any, shortest possible netaddress is "[::]"
    if (addr_str.length() >= 4 && addr_str.front() == '[' &&
        addr_str.back() == ']')
        addr_str = addr_str.substr(1, addr_str.length() - 2);

    // Provide resources for ::getaddrinfo()
    // ai_flags ensure that only numeric values are accepted.
    ::addrinfo hints{};
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
    hints.ai_family = AF_UNSPEC;
    ::addrinfo* res{nullptr};

    // Call ::getaddrinfo() to check the ip address string.
    int ret = umock::netdb_h.getaddrinfo(addr_str.c_str(), serv_str.c_str(),
                                         &hints, &res);
    if (ret == 0) {
#if 0
        // Helpful for debugging, takes some time for typing.
        for (::addrinfo* pres = res; pres != nullptr; pres = pres->ai_next) {
            ::memcpy(&m_sa_union, pres->ai_addr, sizeof(m_sa_union));
            std::cout << "DEBUG: " << pres << ", ai_flags: " << pres->ai_flags
                      << ", ai_family: " << pres->ai_family
                      << ", ai_socktype: " << pres->ai_socktype
                      << ", ai_protocol: " << pres->ai_protocol << ", \""
                      << this->netaddrp() << "\"\n";
        }
#else
        ::memcpy(&m_sa_union, res->ai_addr, sizeof(m_sa_union));
#endif
        umock::netdb_h.freeaddrinfo(res);
    } else {
        umock::netdb_h.freeaddrinfo(res);
        throw std::invalid_argument(UPnPsdk_LOGEXCEPT +
                                    "MSG1043: Invalid netaddress \"" +
                                    a_addr_str + "\".");
    }
}

// Assignment operator= to set socket port from an integer,
// --------------------------------------------------------
void SSockaddr::operator=(const in_port_t a_port) {
    // Don't use ::htons, MacOS don't like it.
    // sin6_port is also sin_port due to union.
    sin6.sin6_port = htons(a_port);
}

// Compare operator== to test if another trivial socket address is equal to this
// -----------------------------------------------------------------------------
bool SSockaddr::operator==(const ::sockaddr_storage& a_ss) const {
    return sockaddrcmp(&a_ss, &ss);
}

// Getter for the assosiated ip address without port
// -------------------------------------------------
// e.g. "[2001:db8::2]" or "192.168.254.253".
const std::string& SSockaddr::netaddr() {
    // TRACE not usable with chained output.
    // TRACE2(this, " Executing SSockaddr::netaddr()")
    //
    // It is important to have the string available as long as the object lives,
    // otherwise you may get dangling pointer, e.g. with getting .c_str().
    m_netaddr = to_netaddr(ss);
    return m_netaddr;
}

// Getter for the assosiated ip address with port
// ----------------------------------------------
// e.g. "[2001:db8::2]:50001" or "192.168.254.253:50001".
const std::string& SSockaddr::netaddrp() {
    // TRACE not usable with chained output.
    // TRACE2(this, " Executing SSockaddr::netaddrp()")
    //
    // It is important to have the string available as long as the object lives,
    // otherwise you may get dangling pointer, e.g. with getting .c_str().
    m_netaddrp = to_netaddrp(m_sa_union.ss);
    return m_netaddrp;
}

// Getter for the assosiated port number
// -------------------------------------
in_port_t SSockaddr::get_port() const {
    TRACE2(this, " Executing SSockaddr::get_port()")
    // sin_port and sin6_port are on the same memory location (union of the
    // structures) so we can use it for AF_INET and AF_INET6.
    // Don't use ::ntohs, MacOS don't like it.
    return ntohs(sin6.sin6_port);
}

// Getter for sizeof the Sockaddr Structure.
// -----------------------------------------
socklen_t SSockaddr::sizeof_ss() const {
    TRACE2(this, " Executing SSockaddr::sizeof_ss()")
    return sizeof(ss);
}

// Getter for sizeof the current (sin6 or sin) Sockaddr Structure.
// ---------------------------------------------------------------
socklen_t SSockaddr::sizeof_saddr() const {
    TRACE2(this, " Executing SSockaddr::sizeof_saddr()")
    switch (ss.ss_family) {
    case AF_INET6:
        return sizeof(sin6);
    case AF_INET:
        return sizeof(sin);
    default:
        return 0;
    }
}


// private member functions
// ------------------------
void SSockaddr::handle_ipv6(const std::string& a_addr_str) {
    TRACE2(this, " Executing SSockaddr::handle_ipv6()")
    // remove surounding brackets
    std::string addr_str = a_addr_str.substr(1, a_addr_str.length() - 2);

    int ret = ::inet_pton(AF_INET6, addr_str.c_str(), &sin6.sin6_addr);
    if (ret == 0) {
        throw std::invalid_argument(UPnPsdk_LOGEXCEPT +
                                    "MSG1043: Invalid netaddress \"" +
                                    a_addr_str + "\".");
    }
    ss.ss_family = AF_INET6;
}

void SSockaddr::handle_ipv4(const std::string& a_addr_str) {
    TRACE2(this, " Executing SSockaddr::handle_ipv4()")
    int ret = ::inet_pton(AF_INET, a_addr_str.c_str(), &sin.sin_addr);
    if (ret == 0) {
        throw std::invalid_argument(UPnPsdk_LOGEXCEPT +
                                    "MSG1044: Invalid netaddress \"" +
                                    a_addr_str + "\".");
    }
    ss.ss_family = AF_INET;
}

/// \cond
// Getter of the netaddress to output stream
// -----------------------------------------
std::ostream& operator<<(std::ostream& os, SSockaddr& saddr) {
    os << saddr.netaddrp();
    return os;
}
/// \endcond

} // namespace UPnPsdk
