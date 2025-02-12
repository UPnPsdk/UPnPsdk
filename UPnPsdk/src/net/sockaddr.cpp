// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-02-10
/*!
 * \file
 * \brief Definition of the Sockaddr class and some free helper functions.
 */

#include <UPnPsdk/sockaddr.hpp>
#include <UPnPsdk/synclog.hpp>
#include <umock/netdb.hpp>
/// \cond
#include <algorithm>
#include <regex>
/// \endcond

namespace UPnPsdk {

namespace {

// Free function to logical compare two sockaddr structures
// --------------------------------------------------------
/*! \brief logical compare two sockaddr structures
 * \ingroup upnplib-addrmodul
 *
 * To have logical equal socket addresses I compare the address family, the ip
 * address, the scope, and the port.
 *
 * \returns
 *  \b true if socket addresses are logical equal\n
 *  \b false otherwise
 */
bool sockaddrcmp(const ::sockaddr_storage* a_ss1,
                 const ::sockaddr_storage* a_ss2) noexcept {
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


/*! \brief Free function to check if a string represents a valid port number
<!-- ------------------------------------------------------------------- -->
 * \ingroup upnplib-addrmodul
 * \code
 * // Usage e.g.:
 * in_port_t port{};
 * switch (to_port("65535", &port) {
 * case -1:
 *     std::cout << "Invalid port number string.\n";
 *     break;
 * case 0:
 *     std::cout << "Valid port number: " << std::to_string(port) << '\n';
 *     break;
 * case 1:
 *     std::cout << Valid number but out of scope 0..65535 for ports.\n";
 *     break;
 * }
 *
 * if (to_port("65536") != 0) { // do nothing with port }
 * \endcode
 * \returns
 *   On success: **0**\n
 *      A binary port number in host byte order is returned in **a_port_num**,
 *      so ypu can use it in your application without conversion. If you want
 *      to store it in a netaddr structure you must use <b>%htons()</b>. An
 *      empty input string returns 0.\n
 *   On error:
 *   - **-1** A valid port number was not found.
 *   - &nbsp;**1** Valid numeric value found but out of scope, not in range
 *                 0..65535.
 */
int to_port( //
    /*! [in] String that may represent a port number. */
    const std::string& a_port_str,
    /*! [in,out] Optional: if given, pointer to a variable that will be filled
     *           with the binary port number in host byte order. */
    in_port_t* const a_port_num = nullptr) noexcept {
    TRACE("Executing to_port() with port=\"" + a_port_str + "\"")

    // // Trim input string.
    // std::string port_str;
    // auto start = a_port_str.find_first_not_of(" \t");
    // // Avoid exception with program terminate if all spaces/tabs.
    // if (start != a_port_str.npos) {
    //     auto end = a_port_str.find_last_not_of(" \t");
    //     port_str = a_port_str.substr(start, (end - start) + 1);
    // }

    // Only non empty strings. I have to check this to avoid stoi() exception
    // below.
    if (a_port_str.empty()) {
        if (a_port_num != nullptr)
            *a_port_num = 0;
        return 0;
    }

    // Now I check if the string are all digit characters
    bool nonzero{false};
    for (char ch : a_port_str) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            return -1;
        } else if (ch != '0') {
            nonzero = true;
        }
    }

    // Only strings with max. 5 char may be valid (uint16_t has max. 65535).
    if (a_port_str.length() > 5) {
        if (nonzero)
            return 1; // value valid but more than 5 char.
        else
            return -1; // string is all zero with more than 5 char.
    }

    // Valid positive number but is it within the port range (uint16_t)?
    // stoi may throw std::invalid_argument if no conversion could be
    // performed or std::out_of_range. But with the prechecked number string
    // this should never be thrown.
    int port = std::stoi(a_port_str);
    if (port > 65535) {
        return 1;
    } else if (a_port_num != nullptr) {
        // Type cast is no problem because the port value is checked to be
        // 0..65635 so it always fit into in_port_t(uint16_t).
        *a_port_num = static_cast<in_port_t>(port);
    }

    return 0;
}

} // anonymous namespace


// Free function to split inet address and port(service)
// -----------------------------------------------------
// For port conversion:
// Don't use '::htons' (with colons) instead of 'htons', MacOS don't like it.
// 'sin6_port' is also 'sin_port' due to union.
//
// Unique pattern recognition, port delimiter is always ':'
//                  Starting with '['
// Pattern e.g.: [2001:db8::1%1]:50001
//               [2001:db8::2]:
//               [2001:db8::3]
//               [::ffff:142.250.185.99]:50001
//                  Starting with "::"
//               ::
//               ::1
//               ::1] // invalid
//               ::ffff:142.250.185.99
//               ::101.45.75.219 // deprecated
//                  Starting with ':' and is port
//               :50002
//               :https
//                  Containing '.'
//               127.0.0.4:50003
//               127.0.0.5:
//               127.0.0.6
//                  Containing one ':'
//               example.com:
//               example.com:50004
//               localhost:
//               localhost:50005
//                  Is port
//               50006
//                  Remaining
//               2001:db8::7
void split_addr_port(const std::string& a_addr_str, std::string& a_addr,
                     std::string& a_serv) {
    TRACE("Executing split_addr_port()")
    // Special cases
    if (a_addr_str.empty()) {
        // An empty address string clears address/port.
        a_addr.clear();
        a_serv.clear();
        return;
    }

    std::string addr_str;
    std::string serv_str;

    size_t pos{};
    if (a_addr_str.length() < 2) {
        // The shortest possible ip address is "::". This helps to avoid string
        // exceptions 'out_of_range'.
        addr_str = a_addr_str; // Give it back as (possible) address.

    } else if (a_addr_str.front() == '[') {
        // Starting with '[', split address if required
        if ((pos = a_addr_str.find("]:")) != std::string::npos) {
            addr_str = a_addr_str.substr(0, pos + 1); // Get IP address
            serv_str = a_addr_str.substr(pos + 2); // Get port string
            if (serv_str.empty())
                serv_str = '0';
        } else {
            addr_str = a_addr_str; // Get IP address
        }

    } else if (a_addr_str.front() == ':' && a_addr_str[1] == ':') {
        // Starting with "::", this cannot have a port.
        addr_str = a_addr_str;

    } else if (a_addr_str.front() == ':') {
        // Starting with ':' and is port
        in_port_t port{};
        switch (to_port(a_addr_str.substr(1), &port)) {
        case -1:
            // No numeric port, check for alphanum port.
            serv_str = a_addr_str.substr(1);
            break;
        case 0:
            // Only port given, set only port.
            serv_str = std::to_string(port);
            break;
        case 1:
            // Value not in range 0..65535.
            goto exit_overrun;
        }
    } else if (a_addr_str.find_first_of('.') != std::string::npos) {
        // Containing '.'
        if ((pos = a_addr_str.find_last_of(':')) != std::string::npos) {
            addr_str = a_addr_str.substr(0, pos); // Get IP address
            serv_str = a_addr_str.substr(pos + 1); // Get port string
            if (serv_str.empty())
                serv_str = '0';
        } else {
            // No port, set only address.
            addr_str = a_addr_str;
        }
    } else if (std::ranges::count(a_addr_str, ':') == 1) {
        // Containing one ':'
        pos = a_addr_str.find_last_of(':');
        addr_str = a_addr_str.substr(0, pos); // Get IP address
        serv_str = a_addr_str.substr(pos + 1); // Get port string
        if (serv_str.empty())
            serv_str = '0';
    } else {
        // Is port
        in_port_t port{};
        switch (to_port(a_addr_str, &port)) {
        case -1:
            // Remaining
            // is either only numeric address, or any alphanumeric identifier.
            addr_str = a_addr_str;
            break;
        case 0:
            // Only port given, set only port.
            serv_str = std::to_string(port);
            break;
        case 1:
            // Value not in range 0..65535
            goto exit_overrun;
        }
    }

    // Check for valid port. ::getaddrinfo accepts invalid ports > 65535.
    switch (to_port(serv_str)) {
    case -1:
    case 0:
        break;
    default:
        goto exit_overrun;
    }

    // remove surounding brackets if any, shortest possible netaddress is
    // "[::]"
    if (addr_str.length() >= 4 && addr_str.front() == '[' &&
        addr_str.back() == ']' && std::ranges::count(addr_str, ':') >= 2 &&
        addr_str.find_first_of('.') == std::string::npos) {

        // Remove surounding brackets
        a_addr = addr_str.substr(1, addr_str.length() - 2);
    } else {
        // Here I have exactly to look for an IPv6 mapped IPv4 address. I could
        // not find any general distinctions, I must use expensive regex. But I
        // do it here only when realy needed. Hints found at
        // REF: [Regular expression that matches valid IPv6 addresses]
        //      (https://stackoverflow.com/a/17871737/5014688)
        const std::regex addr_regex(
            "\\[::(ffff(:0{1,4}){0,1}:){0,1}((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}"
            "[0-"
            "9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\]",
            std::regex_constants::icase);

        if (std::regex_match(addr_str, addr_regex)) {
            // Remove surounding brackets.
            a_addr = addr_str.substr(1, addr_str.length() - 2);
        } else {
            // Haven't found a valid numeric ip address, no need to remove
            // brackets.
            a_addr = addr_str;
        }
    }
    a_serv = serv_str;

    return;

exit_overrun:
    throw std::range_error(UPnPsdk_LOGEXCEPT +
                           "MSG1127: Number string from \"" + a_addr_str +
                           "\" for port is out of range 0..65535.");
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
void SSockaddr::operator=(const std::string& a_addr_str) {
    TRACE2(this, " Executing SSockaddr::operator=(" + a_addr_str + ")")

    if (a_addr_str.empty()) {
        // This clears the complete socket address.
        ::memset(&m_sa_union, 0, sizeof(m_sa_union));
        return;
    }
    std::string ai_addr_str;
    std::string ai_port_str;

    // Throws exception std::out_of_range().
    split_addr_port(a_addr_str, ai_addr_str, ai_port_str);

    // With an empty address part (e.g. ":50001") only set the port and leave
    // the (old) address unmodified.
    if (ai_addr_str.empty()) {
        in_port_t port;
        if (to_port(ai_port_str, &port) != 0)
            goto exit_fail;
        m_sa_union.sin6.sin6_port = htons(port);
        return;
    }

    { // Block needed to avoid error: "goto label crosses initialization".
        // Provide resources for ::getaddrinfo()
        // ai_flags ensure that only numeric values are accepted.
        ::addrinfo hints{};
        hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
        hints.ai_family = AF_UNSPEC;
        ::addrinfo* res{nullptr};

        // Call ::getaddrinfo() to check the ip address string.
        int ret = umock::netdb_h.getaddrinfo(ai_addr_str.c_str(),
                                             ai_port_str.c_str(), &hints, &res);
        if (ret != 0) {
            umock::netdb_h.freeaddrinfo(res);
            goto exit_fail;
        } else {
            if (ai_port_str.empty()) {
                // Preserve old port
                in_port_t port = m_sa_union.sin6.sin6_port;
                ::memcpy(&m_sa_union, res->ai_addr, sizeof(m_sa_union));
                m_sa_union.sin6.sin6_port = port;
            } else {
                ::memcpy(&m_sa_union, res->ai_addr, sizeof(m_sa_union));
            }
            umock::netdb_h.freeaddrinfo(res);
        }

        return;
    }

exit_fail:
    throw std::invalid_argument(UPnPsdk_LOGEXCEPT +
                                "MSG1043: Invalid netaddress \"" + a_addr_str +
                                "\".");
}

// Assignment operator= to set socket port from an integer
// -------------------------------------------------------
void SSockaddr::operator=(const in_port_t a_port) {
    // Don't use ::htons, MacOS don't like it.
    // sin6_port is also sin_port due to union.
    m_sa_union.sin6.sin6_port = htons(a_port);
}

// Assignment operator= to set socket address from a trivial socket address
// structure
// ------------------------------------------------------------------------
void SSockaddr::operator=(const ::sockaddr_storage& a_ss) {
    ::memcpy(&m_sa_union, &a_ss, sizeof(m_sa_union));
}

// Compare operator== to test if another trivial socket address is equal to this
// -----------------------------------------------------------------------------
bool SSockaddr::operator==(const SSockaddr& a_saddr) const {
    return sockaddrcmp(&a_saddr.ss, &ss);
}

// Getter for the assosiated ip address without port
// -------------------------------------------------
// e.g. "[2001:db8::2]" or "192.168.254.253".
const std::string& SSockaddr::netaddr() noexcept {
    // TRACE not usable with chained output.
    // TRACE2(this, " Executing SSockaddr::netaddr()")
    m_netaddr.clear();

    // Accept nameinfo only for supported address families.
    switch (m_sa_union.ss.ss_family) {
    case AF_INET6:
    case AF_INET:
        break;
    case AF_UNSPEC:
        return m_netaddr;
    default:
        UPnPsdk_LOGERR "MSG1129: Unsupported address family "
            << std::to_string(m_sa_union.ss.ss_family)
            << ". Continue with unspecified netaddress \"\".\n";
        return m_netaddr;
    }

    // The buffer fit to an IPv6 address with mapped IPv4 address (max. 46) and
    // also fit to an IPv6 address with scope id (max. 51).
    char addrStr[39 /*sizeof(IPv6_addr)*/ + 1 /*'%'*/ +
                 10 /*sin6_scope_id_max(4294967295)*/ + 1 /*'\0'*/]{};
    int ret = ::getnameinfo(&m_sa_union.sa, sizeof(m_sa_union.ss), addrStr,
                            sizeof(addrStr), nullptr, 0, NI_NUMERICHOST);
    if (ret != 0) {
        // 'std::to_string()' may throw 'std::bad_alloc' from the std::string
        // constructor. It is a fatal error that violates the promise to
        // noexcept and immediately terminates the propgram. This is
        // intentional because the error cannot be handled except improving the
        // hardware.
        UPnPsdk_LOGERR "MSG1036: Failed to get netaddress with address family "
            << std::to_string(m_sa_union.ss.ss_family) << ": "
            << ::gai_strerror(ret)
            << ". Continue with unspecified netaddress \"\".\n";
        return m_netaddr;
    }

    // Next may throw 'std::length_error' if the length of the constructed
    // std::string would exceed max_size(). This should never happen with given
    // lengths of addrStr (promise noexcept).
    if (m_sa_union.ss.ss_family == AF_INET6)
        m_netaddr = '[' + std::string(addrStr) + ']';
    else
        m_netaddr = std::string(addrStr);

    return m_netaddr;
}

// Getter for the assosiated ip address with port
// ----------------------------------------------
// e.g. "[2001:db8::2]:50001" or "192.168.254.253:50001".
const std::string& SSockaddr::netaddrp() noexcept {
    // TRACE not usable with chained output.
    // TRACE2(this, " Executing SSockaddr::netaddrp()")
    //
    // sin_port and sin6_port are on the same memory location (union of the
    // structures) so I can use it for AF_INET and AF_INET6. 'std::to_string()'
    // may throw 'std::bad_alloc' from the std::string constructor. It is a
    // fatal error that violates the promise to noexcept and immediately
    // terminates the propgram. This is intentional because the error cannot be
    // handled except improving the hardware.
    switch (m_sa_union.ss.ss_family) {
    case AF_INET6:
    case AF_INET:
    case AF_UNSPEC:
        m_netaddrp = this->netaddr() + ":" +
                     std::to_string(ntohs(m_sa_union.sin6.sin6_port));
        break;
    case AF_UNIX:
        m_netaddrp = this->netaddr() + ":0";
    }

    return m_netaddrp;
}

// Getter for the assosiated port number
// -------------------------------------
in_port_t SSockaddr::get_port() const {
    TRACE2(this, " Executing SSockaddr::get_port()")
    // sin_port and sin6_port are on the same memory location (union of the
    // structures) so we can use it for AF_INET and AF_INET6.
    // Don't use ::ntohs, MacOS don't like it.
    return ntohs(m_sa_union.sin6.sin6_port);
}

// Getter for sizeof the current (sin6 or sin) Sockaddr Structure.
// ---------------------------------------------------------------
socklen_t SSockaddr::sizeof_saddr() const {
    TRACE2(this, " Executing SSockaddr::sizeof_saddr()")
    switch (m_sa_union.ss.ss_family) {
    case AF_INET6:
        return sizeof(m_sa_union.sin6);
    case AF_INET:
        return sizeof(m_sa_union.sin);
    case AF_UNSPEC:
        return sizeof(m_sa_union.ss);
    default:
        return 0;
    }
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
