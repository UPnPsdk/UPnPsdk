// Copyright (C) 2022+ GPL 3 And higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-08-09
/*!
 * \file
 * \brief Definition of the Sockaddr class and some free helper functions.
 */

#include <UPnPsdk/sockaddr.hpp>
#include <UPnPsdk/synclog.hpp>
#ifdef _MSC_VER
#include <UPnPsdk/netadapter.hpp>
#endif

/// \cond
#include <algorithm>
#ifdef _MSC_VER
#include <netioapi.h> // for ::if_nametoindex()
#else
#include <net/if.h>
#endif
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
//        See below TODO.
int to_port(std::string_view a_port_str, in_port_t* const a_port_num) noexcept {
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


// Structure for a tokenized internet address
// ==========================================
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
// -----------------------------------------------------
// Constructor
SInaddr::SInaddr(const std::string_view a_addr_sv) noexcept {
    if (!a_addr_sv.empty()) {

        this->node.reserve(INET6_ADDRSTRLEN);

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
            // The shortest possible ip address is "::". This helps to avoid
            // string exceptions 'out_of_range'.
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
            // identifier without port. Check for numeric port with type
            // 'in_port_t' (uint16_t). UINT16_MAX (65535) has 5 digits.
            if (is_unum_str(a_addr_sv, 5))
                serv_sv = a_addr_sv; // Numeric value <= MAX_UINT16 not checked.
            else
                addr_sv = a_addr_sv; // Any alphanumeric string.
        }

        // Prepare result for a_inaddr.node.
        // Remove surounding brackets if any, shortest possible netaddress is
        // "[::]".
        if (addr_sv.length() >= 4 && addr_sv.front() == '[' &&
            addr_sv.back() == ']' && std::ranges::count(addr_sv, ':') >= 2) {
            // Here it can be an IPv6 address without '.', or an IPv4 mapped
            // IPv6 address with '.' and prefix "::ffff:". Remove surounding
            // brackets.
            addr_sv.remove_prefix(1);
            addr_sv.remove_suffix(1);
        }

        // Store result to this->node and this->scope.
        if ((pos = addr_sv.find_first_of('%')) != npos) {
            this->node = addr_sv.substr(0, pos);
            this->scope = addr_sv.substr(pos + 1);
            if (this->scope.empty())
                this->scope = zero_sv;
        } else {
            this->node = addr_sv;
            this->scope.clear();
        }

        // Store result to this->service.
        // Check for valid port. ::getaddrinfo accepts invalid ports > 65535.
        this->service = serv_sv;

        /* Normalize this->node to lower case.
        for (auto it{this->node.begin()}; it < this->node.end(); it++)
            *it = static_cast<char>(std::tolower(static_cast<unsigned
        char>(*it)));
        */
        // Trim empty this->node.
        size_t idx{};
        for (; this->node[idx] == ' ' && idx < this->node.size(); idx++)
            ;
        if (idx == this->node.size())
            this->node.clear();

        // Clear this->scope if it results to 0 in any way.
        uint32_t scope_id = is_unum_str(this->scope, 10)
                                ? static_cast<uint32_t>(std::stoul(this->scope))
                                : ~0u;
        if (scope_id == 0)
            this->scope.clear();

        // Convert scope_id to its numeric value string if possible.
        // ---------------------------------------------------------
        // Win32 ::getaddrinfo() only accepts numeric scope_ids and I have to
        // convert netinterface names into its index number (scope_id).
        // ::if_nametoindex() and ::ConvertInterfaceNameToLuidA() with
        // ::ConvertInterfaceLuidToIndex() does not work on Win32. I always get
        // system "Error 123" that means "The filename, directory name, or
        // volume label syntax is incorrect", no matter what I tried.
        // I use my own code. That works.
        //
        // MacOS does not fail ::getaddrinfo() with an unknown netinterface name
        // and instead ignores it and returns an lla addrinfo structure without
        // scope_id. But that is not specified. ::if_nametoindex() on macOS
        // works.
        //
        // Linux platforms ::getaddrinfo() accept netinterface names but fails
        // if they don't exist. That is what the SDK specifies.
        //
        // This all makes it useful to always normalize the scope_id to its
        // numeric value if possible. That's specified by the internet standard
        // and must be supported on all platforms.

        if (!this->scope.empty() && !is_unum_str(this->scope, 10) &&
            this->scope.find_first_of(":") == npos) {
#ifdef _MSC_VER
            CNetadapter nadObj;
            try {
                nadObj.get_first(); // Throws exception if the system cannot
                                    // provide information.
            } catch (const std::exception&) {
                this->scope = "0";
            }
            if (nadObj.find_first(this->scope))
                this->scope = std::to_string(nadObj.index());
#else
            uint32_t scope_id = ::if_nametoindex(this->scope.c_str());
            if (scope_id != 0)
                this->scope = std::to_string(scope_id);
#endif
            else {
                // If no index found, then clear scope_id silently only for IPv6
                // addresses. An IPv4 address with scope_id is an error and
                // should be rejected in follow up functions.
                in6_addr sin6_addr;
                if (::inet_pton(AF_INET6, this->node.c_str(), &sin6_addr) == 1)
                    this->scope.clear();
            }
        }

    } /* if (!a_addr_sv.empty()) // from start */

    UPnPsdk_LOGINFO("MSG1043") << "split \"" << a_addr_sv << "\" into node=\""
                               << this->node << "\", scope=\"" << this->scope
                               << "\", service=\"" << this->service << "\"\n";
}


// Specialized sockaddr_structure
// ==============================

/// \cond
// Constructor
// -----------
SSockaddr::SSockaddr() { m_sa_union.ss.ss_family = AF_INET6; }

// Destructor
// ----------
SSockaddr::~SSockaddr() = default;
/// \endcond


// Assignment operator= to set socket address from a trivial socket address
// storage
// ------------------------------------------------------------------------
void SSockaddr::operator=(const ::sockaddr_storage& a_ss) noexcept {
    switch (a_ss.ss_family) {
    case AF_INET6: {
        char addr_buf[INET6_ADDRSTRLEN];
        auto* a_sin6 = reinterpret_cast<const sockaddr_in6*>(&a_ss);

        if (IN6_IS_ADDR_LINKLOCAL(&a_sin6->sin6_addr)) { // Is it "[fe80]"?
            // An lla, but is it valid and has no subnet prefix "[fe80::"?
            if (!IN6_IS_ADDR_LINKLOCAL2(&a_sin6->sin6_addr) ||
                a_sin6->sin6_scope_id == 0) {
                // A valid link-local address must have a scope_id.
                ::inet_ntop(AF_INET6, &a_sin6->sin6_addr, addr_buf,
                            sizeof(addr_buf));
                UPnPsdk_LOGERR("MSG1127") "lla=\"["
                    << addr_buf
                    << (a_sin6->sin6_scope_id == 0
                            ? ""
                            : "%" + std::to_string(a_sin6->sin6_scope_id))
                    << "]\" with subnet, or without scope_id.";
                break; // Error
            }
            // Valid lla.
            m_sa_union.ss = a_ss;
            return;

        } else if (IN6_IS_ADDR_MULTICAST(&a_sin6->sin6_addr)) {
            if (a_sin6->sin6_scope_id == 0) {
                // A valid multicast address must have a scope_id.
                ::inet_ntop(AF_INET6, &a_sin6->sin6_addr, addr_buf,
                            sizeof(addr_buf));
                UPnPsdk_LOGERR("MSG1045") "mcast=\"["
                    << addr_buf << "]\" without scope_id.";
                break; // Error
            }
            // Valid mcast.
            m_sa_union.ss = a_ss;
            return;
        }

        // An IPv6 address but not a link-local address, or multicast address.
        // A scope_id is silently discarded.
        m_sa_union.ss = a_ss;
        m_sa_union.sin6.sin6_scope_id = 0;
        return;
    }

    case AF_INET:
        m_sa_union.ss = a_ss;
        return;

    case AF_UNSPEC:
        m_sa_union = {};
        m_sa_union.ss.ss_family = AF_UNSPEC;
        return;

    default:
        UPnPsdk_LOGERR("MSG1179") "Unsupported address family "
            << a_ss.ss_family << ".";
        break; // Error

    } // switch

    // Finish error message.
    if (g_dbug)
        std::cerr << " Set unspecified socket address.\n";
    m_sa_union = {};
    m_sa_union.ss.ss_family = AF_UNSPEC;
}


// Assignment operator= to set socket address from an internet address
// -------------------------------------------------------------------
void SSockaddr::operator=(const SInaddr& a_inaddr) noexcept {
    // Please note that inet_pton() on Microsoft Windows modifies its
    // destination (here 'saddr') even if it fails.
    ::UPnPsdk::sockaddr_t saddr{};
    saddr.ss.ss_family = AF_UNSPEC;
    bool saddr_empty{false};

    if (a_inaddr.node.empty()) {
        m_sa_union = saddr;
        return;
    }

    // Check if IPv6 address.
    // ----------------------
    if (::inet_pton(AF_INET6, a_inaddr.node.c_str(),
                    &saddr.sin6.sin6_addr) == 1) //
    {
        if (!a_inaddr.scope.empty()) {
            // Here are only numeric scope_ids valid.
            if (!is_unum_str(a_inaddr.scope, 10)) {
                m_sa_union = {};
                m_sa_union.ss.ss_family = AF_UNSPEC;
                return;
            }
            if (!saddr_empty) {
                saddr.sin6.sin6_scope_id =
                    static_cast<uint32_t>(std::stoul(a_inaddr.scope));
                saddr.ss.ss_family = AF_INET6;
            }
        }
    }

    // Check if IPv4 address and map it to IPv6.
    // -----------------------------------------
    else if (in_addr sin_addr;
             ::inet_pton(AF_INET, a_inaddr.node.c_str(), &sin_addr) == 1) //
    {
        // Check if no scope_id set.
        uint32_t scope_id =
            is_unum_str(a_inaddr.scope, 10)
                ? static_cast<uint32_t>(std::stoul(a_inaddr.scope))
                : ~0u;
        if (!a_inaddr.scope.empty() && scope_id != 0) {
            // If we have a scope_id with IPv4, then that's an error.
            m_sa_union = {};
            m_sa_union.ss.ss_family = AF_UNSPEC;
            return;
        }

        // Map IPv4 to IPv6.
        if (!saddr_empty) {
            uint32_t* sin6_32 =
                reinterpret_cast<uint32_t*>(&saddr.sin6.sin6_addr);
            sin6_32[0] = 0;
            sin6_32[1] = 0;
            sin6_32[2] = htonl(0x0000ffff);
            sin6_32[3] = sin_addr.s_addr;
            saddr.ss.ss_family = AF_INET6;
        }
    }

    // Check possible DNS host name for valid characters.
    // --------------------------------------------------
    else {
        // Check for valid host name characters.
        unsigned char ch;
        size_t i;
        for (i = 0; i < a_inaddr.node.size(); i++) {
            ch = static_cast<unsigned char>(a_inaddr.node[i]);
            if (!std::isalnum(ch) && ch != '.' && ch != '-')
                break;
        }
        if (i < a_inaddr.node.size()) {
            // Invalid character for a DNS host name found. Indicate invalid
            // node entry.
            m_sa_union = {};
            m_sa_union.ss.ss_family = AF_UNSPEC;
            return;
        }
        // Seems there is an alphanumeric node. I indicate that with an empty
        // (numeric) socket-address.
        saddr = {};
        saddr.ss.ss_family = AF_INET6;
        saddr_empty = true;
    }

    // Manage service/port.
    // --------------------
    ::in_port_t port;
    switch (to_port(a_inaddr.service, &port)) {
    case -1: // Not a valid port number string.
        // May be an alpha-numeric port name (e.g. "https"). I indicate this
        // with an empty socket-address for possible name resolution by the
        // calling function.
        saddr = {};
        saddr.ss.ss_family = AF_INET6;
        saddr_empty = true;
        break;

    case 0: // Valid numeric port number.
        // Store the port number.
        if (!saddr_empty) {
            saddr.sin6.sin6_port = htons(port);
            saddr.ss.ss_family = AF_INET6;
        }
        break;

    case 1: // Valid numeric number, but not in range 0..65535.
        UPnPsdk_LOGERR("MSG1184") "port number=\""
            << a_inaddr.service
            << "\" not in range 0..65535. Set unspecified socket address.\n";
        [[fallthrough]];

    default:
        // When default comes up, it is a bug.
        m_sa_union = {};
        m_sa_union.ss.ss_family = AF_UNSPEC;
        return;
    }

    if (saddr_empty) {
        // Seems there is any alphanumeric component. I indicate that with an
        // empty (numeric) socket-address for the calling function to use name
        // resolution.
        m_sa_union = {};
        m_sa_union.ss.ss_family = AF_INET6;
        return;
    }

    // Attention! saddr must still be valid till returning from following method
    // to check valid dependencies.
    *this = saddr.ss;
}


// Compare operator== to test if another trivial socket address is equal to this
// -----------------------------------------------------------------------------
bool SSockaddr::operator==(const SSockaddr& a_saddr) const {
    return sockaddrcmp(&a_saddr.ss, &ss);
}


// Getter for the assosiated ip address without port
// -------------------------------------------------
// e.g. "[fe80::3%2]:51000".
std::string SSockaddr::netaddr() noexcept {
    // Some more statements, but due to frequently usage, it's optimized to
    // reduce expensive memory allocation. I don't use ::getnameinfo() because
    // it doesn't return the scope_id numeric on Unix like platforms. This
    // would confuse the internal program logik and it is simpler to handle it
    // only here.

    std::string netaddr_st;

    switch (m_sa_union.ss.ss_family) {
    case AF_INET6: {
        // Get IPv6 address string.
        char addr_buf[INET6_ADDRSTRLEN];
#ifdef _MSC_VER
        // Yet another annoing quirk from Microsoft Windows: only for the
        // unknown netaddress "[0.0.0.0]" it returns (the correct)
        // "[::ffff:0:0]" instead of expected "[::ffff:0.0.0.0]". If I binary
        // detect this address I convert it "by hand". Other platforms do it as
        // expected.
        const uint32_t* sin6_32 =
            reinterpret_cast<uint32_t*>(&m_sa_union.sin6.sin6_addr);
        if (*sin6_32 == 0 && *(sin6_32 + 1) == 0 &&
            *(sin6_32 + 2) == htonl(0x0000ffff) && *(sin6_32 + 3) == 0) {
            strcpy(addr_buf, "::ffff:0.0.0.0");
        } else
#endif
        {
            auto ret = ::inet_ntop(AF_INET6, &m_sa_union.sin6.sin6_addr,
                                   addr_buf, sizeof(addr_buf));
            if (ret == nullptr)
                break; // Error
        }
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
                // is it? Anyway, it's controlled reported as error.
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
// e.g. "[2001:db8::2]:50001".
std::string SSockaddr::netaddrp() noexcept {
    // sin_port and sin6_port are on the same memory location (union of the
    // structures) so I can use it for AF_INET and AF_INET6. 'std::to_string()'
    // may throw 'std::bad_alloc' from the std::string constructor. It is a
    // fatal error that violates the promise to noexcept and immediately
    // terminates the propgram. This is intentional because the error cannot be
    // handled except improving the hardware.
    switch (m_sa_union.ss.ss_family) {
    case AF_INET6:
    case AF_INET:
    case AF_UNSPEC: {
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


// Getter for sizeof the current (sin6 or sin) Sockaddr Structure.
// ---------------------------------------------------------------
socklen_t SSockaddr::sizeof_saddr() const noexcept {
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

} // namespace UPnPsdk
