// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-05-15
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

} // anonymous namespace


// Free function to check if a string represents a valid port number
// -----------------------------------------------------------------
/// \todo Update on MacOS to Clang compiler that supports std::from_chars().
///       See below TODO.
int to_port(std::string_view a_port_str, in_port_t* const a_port_num) noexcept {
    TRACE("Executing to_port() with port=\"a_port_str\"")

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
    // Error conditions of the function is not checked because there is always a
    // pre-checked valid number string given.
    // TODO: Update on MacOS to Clang compiler that supports std::from_chars().
#ifdef __clang__
    std::string port_str(a_port_str);
    int port = atoi(port_str.c_str());
#else
    int port{};
    std::from_chars(a_port_str.data(), a_port_str.data() + a_port_str.size(),
                    port);
#endif
    if (port > 65535) {
        return 1;
    } else if (a_port_num != nullptr) {
        // Type cast is no problem because the port value is checked to be
        // 0..65635 so it always fit into in_port_t(uint16_t).
        *a_port_num = static_cast<in_port_t>(port);
    }

    return 0;
}


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
//               [::101.45.75.219] // deprecated
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
void split_inaddr(const std::string_view a_addr_sv,
                  inaddr_t& a_inaddr) noexcept {
    TRACE("Executing split_inaddr()")

    // Special cases
    if (a_addr_sv.empty()) {
        // An empty address string clears address/scope_id/port.
        a_inaddr.node.clear();
        a_inaddr.scope.clear();
        a_inaddr.service.clear();
        return;
    }

    std::string_view addr_sv;
    std::string_view serv_sv;
    static constexpr std::string_view zero_sv("0");

    auto& npos = std::string_view::npos;
    size_t pos{};
    if (a_addr_sv.size() == 1) {
        // Only one digit belongs to a port number. Port numbers with more
        // digits are tested later.
        if (std::isdigit(a_addr_sv.front())) {
            // If it is a digit, then it's a port number.
            serv_sv = a_addr_sv;
        } else if (a_addr_sv.front() == ':') {
            // Having only the port separator, then the port is reset.
            serv_sv = zero_sv;
        }
    } else if (a_addr_sv.size() < 2) {
        // The shortest possible ip address is "::". This helps to avoid string
        // exceptions 'out_of_range'.
        addr_sv = a_addr_sv; // Give it back as (possible) address.

    } else if (a_addr_sv.front() == '[') {
        // Starting with '[', split address if required
        if ((pos = a_addr_sv.find("]:")) != npos) {
            addr_sv = a_addr_sv.substr(0, pos + 1); // Get IP address
            serv_sv = a_addr_sv.substr(pos + 2); // Get port string
            if (serv_sv.empty())
                serv_sv = zero_sv;
        } else {
            addr_sv = a_addr_sv; // Get IP address
        }

    } else if (a_addr_sv.front() == ':' && a_addr_sv[1] == ':') {
        // Starting with "::", this cannot have a port.
        addr_sv = a_addr_sv;

    } else if (a_addr_sv.front() == ':') {
        // Starting with ':'
        // Only port given, set only port, may be alphanum.
        serv_sv = a_addr_sv.substr(1);
        if (serv_sv.empty())
            serv_sv = zero_sv;
    } else if (a_addr_sv.find_first_of('.') != npos) {
        // Containing '.'
        if ((pos = a_addr_sv.find_last_of(':')) != npos) {
            addr_sv = a_addr_sv.substr(0, pos); // Get IP address
            serv_sv = a_addr_sv.substr(pos + 1); // Get port string
            if (serv_sv.empty())
                serv_sv = zero_sv;
        } else {
            // No port, set only address.
            addr_sv = a_addr_sv;
        }
    } else if (std::ranges::count(a_addr_sv, ':') == 1) {
        // Containing one ':'
        pos = a_addr_sv.find_last_of(':');
        addr_sv = a_addr_sv.substr(0, pos); // Get IP address
        serv_sv = a_addr_sv.substr(pos + 1); // Get port string
        if (serv_sv.empty())
            serv_sv = zero_sv;
    } else {
        // Remaining: here we have a numeric port. If a numeric port doesn't
        // fit, it is either an only numeric address, or any alphanumeric
        // identifier without port. Check for numeric port with type 'in_port_t'
        // (uint16_t). UINT16_MAX (65535) has 5 digits.
        size_t i{};
        for (; i < a_addr_sv.size() && i < 6; i++)
            if (!std::isdigit(a_addr_sv[i]))
                i = 6;
        if (i > 5)
            addr_sv = a_addr_sv; // Any alphanumeric string.
        else
            serv_sv = a_addr_sv; // Numeric value <= MAX_UINT16 not checked.
    }

    // Return result for a_inaddr.node.
    // Remove surounding brackets if any, shortest possible netaddress is
    // "[::]".
    if (addr_sv.length() >= 4 && addr_sv.front() == '[' &&
        addr_sv.back() == ']' && std::ranges::count(addr_sv, ':') >= 2) {
        // Here it can be an IPv6 address without '.', or an IPv4 mapped IPv6
        // address with '.' and prefix "::ffff:".
        // Remove surounding brackets.
        addr_sv.remove_prefix(1);
        addr_sv.remove_suffix(1);
    }

    // Return result for a_inaddr.node and a_inaddr.scope.
    if ((pos = addr_sv.find_first_of('%')) != npos) {
        a_inaddr.node = addr_sv.substr(0, pos);
        a_inaddr.scope = addr_sv.substr(pos + 1);
        if (a_inaddr.scope.empty())
            a_inaddr.scope = zero_sv;
    } else {
        a_inaddr.node = addr_sv;
        a_inaddr.scope.clear();
    }

    // Return result for a_inaddr.service.
    // Check for valid port. ::getaddrinfo accepts invalid ports > 65535.
    a_inaddr.service = serv_sv;
    UPnPsdk_LOGINFO("MSG1043")
        << a_addr_sv << "\" into addr=\"" << a_inaddr.node << "\", scope_id=\""
        << a_inaddr.scope << "\", port=\"" << a_inaddr.service << "\"\n";
}


// Specialized sockaddr_structure
// ==============================
// Constructor
SSockaddr::SSockaddr(){
    TRACE2(this, " Construct SSockaddr()") //
}

// Destructor
SSockaddr::~SSockaddr(){TRACE2(this, " Destruct SSockaddr()")}

// Get reference to the sockaddr_storage structure.
// Only as example, I don't use it.
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
// =======================================================
void SSockaddr::operator=(const std::string_view a_addr_sv) noexcept {
    TRACE2(this, " Executing SSockaddr::operator=(a_addr_sv)")

    if (a_addr_sv.empty()) {
        // This clears the complete socket address.
        m_sa_union = {};
        m_sa_union.ss.ss_family = AF_UNSPEC;
        return;
    }

    do {
        // Split address, scope_id, port
        // -----------------------------
        inaddr_t inaddr;
        split_inaddr(a_addr_sv, inaddr);


        // Manage IP address.
        // ------------------
        if (!inaddr.node.empty()) {

            // Look for IPv6 address.
            auto& s6 = m_sa_union.sin6;
            if (inet_pton(AF_INET6, inaddr.node.c_str(), &s6.sin6_addr) == 1) {
                m_sa_union.ss.ss_family = AF_INET6;
                s6.sin6_scope_id = 0;

                // Check scope_id only valid for link-local addresses.
                if (!inaddr.scope.empty() && inaddr.scope != "0") {
                    if (IN6_IS_ADDR_LINKLOCAL(&s6.sin6_addr)) {
                        // scope_id and link-local address: store scope_id?

                        // Check if there are all digits for scope_id.
                        // UINT32_MAX (4,294,967,295) has 10 digits.
                        size_t i{};
                        for (; i < inaddr.scope.size() && i < 11; i++)
                            if (!std::isdigit(inaddr.scope[i]))
                                i = 11;
                        if (i > 10) {
                            // An alphanumeric string.
                            break; // Error
                        } else

                            // Numeric value, store scope_id.
                            // ------------------------------
                            // stoi() may throw exception, but not possible due
                            // to guarded value above.
                            s6.sin6_scope_id =
                                static_cast<uint32_t>(std::stoi(inaddr.scope));

                    } else {
                        // scope_id but no link-local address.
                        break; // Error
                    }

                } else { // No scope_id.

                    if (IN6_IS_ADDR_LINKLOCAL(&s6.sin6_addr)) {
                        // No scope_id but link-local address.
                        break; // Error
                    }
                }

                // Look for IPv4 address.
            } else if (inet_pton(AF_INET, inaddr.node.c_str(),
                                 &m_sa_union.sin.sin_addr) == 1) {
                m_sa_union.ss.ss_family = AF_INET;

            } else {
                // No valid IP address found.
                break; // Error
            }
        }


        // Manage port. Also valid for AF_INET.
        // ------------------------------------
        if (!inaddr.service.empty()) {
            // Check if port string is numeric. UINT16_MAX (65535) has 5 digits.
            size_t i{};
            for (; i < inaddr.service.size() && i < 6; i++)
                if (!std::isdigit(inaddr.service[i]))
                    i = 6;
            if (i > 5)
                break; // Error

            // Valid uint16_t value, but limited to UINT16_MAX?
            int iport = std::stoi(inaddr.service);
            if (iport > UINT16_MAX)
                break; // Error

            // Store valid port number.
            m_sa_union.sin6.sin6_port = htons(static_cast<in_port_t>(iport));
        }

        return; // OK, finished.

    } while (false);
    // Here we are from all the breaks.

    // This clears the complete socket address.
    m_sa_union = {};
    m_sa_union.ss.ss_family = AF_UNSPEC;
    UPnPsdk_LOGERR("MSG1033") "SSockaddr::=\""
        << a_addr_sv
        << "\" is invalid. Look at netaddress, port value, or scope_id MUST "
           "only on lla. Hint: only numeric values accepted. For name "
           "resolution use CAddrinfo.\n";
}


// Assignment operator= to set socket port from an integer
// -------------------------------------------------------
void SSockaddr::operator=(const in_port_t a_port) {
    // Don't use ::htons, MacOS don't like it.
    // sin6_port is also sin_port due to union.
    m_sa_union.sin6.sin6_port = htons(a_port);
}

// Assignment operator= to set socket address from a trivial socket address
// storage
// ------------------------------------------------------------------------
void SSockaddr::operator=(const ::sockaddr_storage& a_ss) {
    m_sa_union.ss = a_ss;
    // Correct possible wrong setting.
    if (m_sa_union.ss.ss_family == AF_INET6 &&
        !IN6_IS_ADDR_LINKLOCAL(&m_sa_union.sin6.sin6_addr))
        m_sa_union.sin6.sin6_scope_id = 0;
}

// Compare operator== to test if another trivial socket address is equal to this
// -----------------------------------------------------------------------------
bool SSockaddr::operator==(const SSockaddr& a_saddr) const {
    return sockaddrcmp(&a_saddr.ss, &ss);
}

// Getter for the assosiated ip address without port
// -------------------------------------------------
// e.g. "[fe80::3%2]:51000" or "192.168.254.253:51001".
std::string SSockaddr::netaddr() noexcept {
    // TRACE not usable with chained output.
    // TRACE2(this, " Executing SSockaddr::netaddr()")
    //
    // Some more statements, but due to frequently usage, it's optimized to
    // reduce expensive memory allocation. I don't use ::getnameinfo() because
    // it doesn't return the scope_id numeric on Unix like platforms. This
    // would confuse the internal program structure and it is simpler to handle
    // it only here.

    std::string netaddr_st;

    switch (m_sa_union.ss.ss_family) {
    case AF_INET6: {
        // Get IPv6 address string.
        char addr_buf[INET6_ADDRSTRLEN];
        auto ret = ::inet_ntop(AF_INET6, &m_sa_union.sin6.sin6_addr, addr_buf,
                               sizeof(addr_buf));
        if (ret == nullptr)
            break; // Error

        // Build IPv6 netaddress string with IP address, and scope_id, if
        // available.
        size_t str_len =
            strlen(addr_buf) + 2 /*brackets*/ + 15 /*default reserve*/;

        // UINT32_MAX (4,294,967,295) has 10 digits.
        char scope_buf[10 + 1]; // Incl. null terminator.
        if (m_sa_union.sin6.sin6_scope_id) {
            int ret = ::snprintf(scope_buf, sizeof(scope_buf), "%u",
                                 m_sa_union.sin6.sin6_scope_id);
            if (ret < 0)
                // ret is an integer so this can fail on a huge amount of
                // available local netadapter ((4,294,967,295 / 2) index number
                // = scope_id). Only to mention it, but not really a problem,
                // is it? Anyway, it's controled reported as error.
                break; // Error

            // Separator '%' and UINT32_MAX digits (without null terminator).
            str_len += 1 + 10;
        }

        // Optimize string with reserve its memory usage before filling it.
        netaddr_st.reserve(str_len);
        netaddr_st.append("[").append(addr_buf);
        if (m_sa_union.sin6.sin6_scope_id)
            netaddr_st.append("%").append(scope_buf);
        netaddr_st.append("]");

        return netaddr_st;
    }

    case AF_INET: {
        // Get IPv4 address string.
        char addr_buf[INET_ADDRSTRLEN];
        auto ret = ::inet_ntop(AF_INET, &m_sa_union.sin.sin_addr, addr_buf,
                               sizeof(addr_buf));
        if (ret == nullptr)
            break; // Error

        // Optimize string with reserve its memory usage before filling it.
        netaddr_st.reserve(sizeof(addr_buf) + 15); // Incl. default reserve.
        netaddr_st.append(addr_buf);

        return netaddr_st;
    }

    case AF_UNSPEC:
        return "";

    } // switch

    UPnPsdk_LOGERR("MSG1036") "Failed to get netaddress for address family "
        << m_sa_union.ss.ss_family << ".\n";

    return "";
}

// Getter for the assosiated ip address with port
// ----------------------------------------------
// e.g. "[2001:db8::2]:50001" or "192.168.254.253:50001".
std::string SSockaddr::netaddrp() noexcept {
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
    case AF_UNSPEC: { // Setting port on an empty sockaddr should be possible.
        // UINT16_MAX (65535) for port has 5 digits.
        char port_buf[5 + 1]; // Incl. null terminator.
        int ret = ::snprintf(port_buf, sizeof(port_buf), "%u",
                             ntohs(m_sa_union.sin6.sin6_port));
        if (ret < 0)
            // ret is an integer so this can fail on huge scope_ids
            // (4,294,967,295 / 2). Only to mention it, but not really a
            // problem, is it?
            break; // Error

        return this->netaddr().append(":").append(port_buf);
    }

    } // switch

    UPnPsdk_LOGERR(
        "MSG1015") "Failed to get netaddress with port for address family "
        << m_sa_union.ss.ss_family << ".\n";

    return ":0";
}

// Getter for the assosiated port number
// -------------------------------------
in_port_t SSockaddr::port() const {
    TRACE2(this, " Executing SSockaddr::port()")
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

// Get if the socket address is a loopback address
//  ----------------------------------------------
bool SSockaddr::is_loopback() const {
    // I handle only IPv6 addresses and check if I have either the IPv6
    // loopback address or any IPv4 mapped IPv6 address between
    // "[::ffff:127.0.0.0]" and "[::ffff:127.255.255.255]".
    return (
        m_sa_union.ss.ss_family == AF_INET6 &&
        (IN6_IS_ADDR_LOOPBACK(&m_sa_union.sin6.sin6_addr) ||
         (ntohl(static_cast<uint32_t>(m_sa_union.sin6.sin6_addr.s6_addr[12])) >=
              2130706432 &&
          ntohl(static_cast<uint32_t>(m_sa_union.sin6.sin6_addr.s6_addr[12])) <=
              2147483647)));
#if 0
            (m_sa_union.ss.ss_family == AF_INET &&
             // address between "127.0.0.0" and "127.255.255.255"
             ntohl(m_sa_union.sin.sin_addr.s_addr) >= 2130706432 &&
             ntohl(m_sa_union.sin.sin_addr.s_addr) <= 2147483647));
#endif
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
