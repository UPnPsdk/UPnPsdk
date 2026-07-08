#ifndef UPnPsdk_NET_SOCKADDR_HPP
#define UPnPsdk_NET_SOCKADDR_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-07-15
/*!
 * \file
 * \brief Declaration of the Sockaddr class and some free helper functions.
 */

#include <UPnPsdk/visibility.hpp>
#include <UPnPsdk/port.hpp>
#include <UPnPsdk/port_sock.hpp>
/// \cond
#ifdef _MSC_VER
#include <afunix.h>
#else
#include <sys/un.h>
#endif

#include <string>
/// \endcond


namespace UPnPsdk {

/*!
 * \brief Helpful union of the different socket address structures
 * \ingroup upnplib-addrmodul
 *
 * Never need to use type casts with pointer to different socket address
 * structures. For details about using this helpful union have a look at
 * <!--REF:--> <a href="https://stackoverflow.com/a/76548581/5014688">sockaddr
 * structures as union</a>
 */
union sockaddr_t {
    ::sockaddr_storage ss;
    ::sockaddr_un sun;
    ::sockaddr_in6 sin6;
    ::sockaddr_in sin;
    ::sockaddr sa;
};

/*!
 * \brief Test if a socket address is a global unicast address
 * \ingroup upnplib-addrmodul
 *
 * This function isn't provided by the system on Unix/Linux platforms as member
 * of the IN6_IS_ADDR test macros. On Microsoft Windows it replaces the
 * imprecise corresponding system function. There it does not check the current
 * specified range for global unicast addresses but only the wide range
 * reservation for future use.
 */
// For GCC compiler the macros can be found in 'netinet/in.h'.
// For MSVC compiler the inline functions can be found in 'ws2ipdef.h'.
// For details look at Unit Tests 'SockaddrTestSuite.verify_in6_is_addr_*'.
inline bool IN6_IS_ADDR_GLOBAL2(
    /*! [in] Pointer to the address structure of a socket address that shall be
       checked. */
    const in6_addr* a_addr) {
    const uint32_t* sin6_32 = reinterpret_cast<const uint32_t*>(a_addr);
    return ((sin6_32[0] & htonl(0xe0000000)) == htonl(0x20000000));
}

/*!
 * \brief Test if a socket address is a link-local address
 * \ingroup upnplib-addrmodul
 *
 * The IN6_IS_ADDR_LINKLOCAL test as member of the IN6_IS_ADDR test macros on
 * all supported platforms is weak. They do not check that no subnet is
 * specified. BSD-based operating systems (including macOS) also support an
 * alternative, **non-standard syntax**, where a numeric zone index is encoded
 * in the second 16-bit word of the address. E.g.:
 * `[e80:3::1ff:fe23:4567:890a]`. Following the link-local address standard, it
 * is an lla with subnet. That is a contradiction. I do not support this
 * non-standard and use IN6_IS_ADDR_LINKLOCAL2 for tests, that fails on lla with
 * subnet.
 */
// For GCC compiler the macros can be found in 'netinet/in.h'.
// For MSVC compiler the inline functions can be found in 'ws2ipdef.h'.
// For details look at Unit Tests 'SockaddrTestSuite.verify_in6_is_addr_*'.
inline bool IN6_IS_ADDR_LINKLOCAL2(
    /*! [in] Pointer to the address structure of a socket address that shall be
       checked. */
    const in6_addr* a_addr) {
    const uint32_t* sin6_32 = reinterpret_cast<const uint32_t*>(a_addr);
    return (sin6_32[0] == htonl(0xfe800000) && sin6_32[1] == 0x00000000);
}


/*! \brief Free function to check if a string represents an unsigned integer
<!-- ----------------------------------------------------------------- - -->
 * \ingroup upnplib-addrmodul
 *
 * For example a number string that should fit into an uint32_t variable after
 * converting:
 * \code
 * // Usage e.g.:
 * std::string uint32_max = "4294967295"; // has 10 digits
 * if (is_unum_str(uint32_max, 10))
 *     uint32_t num = static_cast<uint32_t>(std::stoul(uint32_max));
 * \endcode
 * \returns
 *  \b true if the string represents an all digit number\n
 *  \b false otherwise
*/
inline bool is_unum_str( //
    std::string_view a_num_sv, ///< [in] String to check.
    size_t a_digits ///< [in] Max. amount of digits the number has.
) {
    size_t i;
    for (i = 0; i < a_num_sv.size() && i < a_digits + 1; i++)
        if (!std::isdigit(static_cast<unsigned char>(a_num_sv[i])))
            i = a_digits + 1; // This finishes the loop.
    return ((i == 0) || (i > a_digits)) ? false : true;
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
 *     std::cout << Valid number but out of range 0..65535 for ports.\n";
 *     break;
 * }
 *
 * if (to_port("65536") != 0) { // do nothing with port }
 * \endcode
 * \returns
 *   On success: **0**\n
 *      A binary port number in host byte order is returned in **a_port_num**,
 *      so you can use it in your application without conversion. If you want
 *      to store it in a netaddr structure you must use <b>%htons()</b>. An
 *      empty input string returns 0.\n
 *   On error:
 *   - **-1** A valid port number was not found.
 *   - &nbsp;**1** Valid numeric value found but not in range 0..65535 for
 *     ports.
 */
UPnPsdk_VIS int to_port( //
    /*! [in] String that may represent a port number. */
    std::string_view a_port_str,
    /*! [out] Optional: if given, pointer to a variable that will be filled with
              the binary port number in host byte order. */
    in_port_t* const a_port_num = nullptr) noexcept;


/*!
 * \brief Components of an internet address
 *
 * Typical LLA example with all [netaddress](\ref glossary_netaddr)
 * components:\n
 * "[fe80::1%2]:443" with node "fe80::1", scope "2", service "443"\n
 * "[fe80::2%eth0]:https" with node "fe80::2", scope "eth0", service "https"
 */
struct inaddr_token_t {
    std::string node; /*!< IP address without brackets. This can also be an
                         alphanumeric name like "example.com". */
    std::string scope; /*!< scope_id is the index number or name of a
                         [netadapter](\ref glossary_netadapt). Only valid on a
                         link-local address. */
    std::string service; /*!< Port number, or service name (e.g. "https"). */
};


/*!
 * \brief Free function to split inet address, scope_id, and port(service)
 * <!-- -------------------------------------------------------------- -->
 * \ingroup upnplib-addrmodul
 * \code
 * // Usage e.g., not a complete list:
 * inaddr_token_t inaddr;
 * inaddr_tokenize("[2001:db8::1]:50001", inaddr);
 * inaddr_tokenize("2001:DB8::1", inaddr);
 * inaddr_tokenize("[fe80::2%3]:50002", inaddr);
 * inaddr_tokenize("127.0.0.1:0", inaddr);
 * inaddr_tokenize("127.0.0.1", inaddr);
 * inaddr_tokenize(":50002", inaddr);
 * inaddr_tokenize("example.COM:50003", inaddr);
 * inaddr_tokenize("example.com:HTTPS", inaddr);
 * inaddr_tokenize("[::FFff:142.250.185.99]:ssh", inaddr);
 * \endcode
 *
 * This is a function for special use to prepare input for system calls without
 * brackets. Its results returned in structue inaddr_token_t with member \b
 * node, \b scope, and **service (port)** are only useful for this purpose and
 * not meant for general usage. The function only syntactical split the
 * components on its separator '\%' for scope_id, and last ':' for port. No
 * symantical tests are made. For example a scope_id on a global unicast
 * address, or a port number greater 65535 is not valid but also returned in \b
 * a_inaddr for scope, resp. for service. These tests must be made on a higher
 * abstraction layer.
 * */
UPnPsdk_VIS void inaddr_tokenize( //
    /*! [in] Any string. If it can be interpreted as an ip-address or -name with
       or without scope_id and/or service (port), its components will be
       returned. */
    const std::string_view a_addr_sv,
    /*! [out] Reference of an internet address structure that will be filled
       with node, scope_id, and service (port). */
    inaddr_token_t& a_inaddr) noexcept;


/*!
 * \brief Trivial ::%sockaddr structures enhanced with methods
 * <!--   ================================================ -->
 * \ingroup upnplib-addrmodul
\code
// Usage e.g.:
::sockaddr_storage saddr{};
SSockaddr saObj;
::memcpy(&saObj.ss, &saddr, sizeof(saObj.ss)); // possible, but error prone
// better with verification by the object:
saObj = saddr;
if (saObj.ss.ss_family == AF_INET6)
    std::cout << "netaddress of saObj is " << saObj << "\n";
\endcode
 *
 * This structure should be usable on a low level like the trival C `struct
 * ::%sockaddr_storage` but provides additional methods to manage its data.
 * When ever this SDK manage a network address it uses an object of this class.
 *
 * Design specification:
 *
 * - An empty socket address object has the address family AF_INET6 by default
 *   and returns netaddrp() `"[::]:0"`.
 * - Detected errors (e.g. invalid entries) result in an unspecified socket
 *   address object and can be tested with `saObj.ss.ss_family == AF_UNSPEC`.
 *   netaddrp() returns then `":0"`.
 * - A link-local address (e.g. "[fe80::1%2]") must always have a scope_id. An
 *   LLA without scope_id is rejected as error. If in doubt you should test the
 *   address family not to be AF_UNSPEC.
 * - If an IPv6 address, that isn't a link-local address, has a scope_id (e.g.
 *   "[2001:db8::1%2]") then the scope_id is silently removed.
 * - SSockaddr can also hold IPv4 addresses but these are only provided for
 *   special use of the user. The SDK itself does not understand them and
 *   cannot work with them.
 *
 * \note This class is frequently used so performance has to taken into
 * account. This is why the destructor isn't virtual and **you should not
 * derive from this class as base class. Also runtime polymorphism should not
 * be used** (deleting through a base class pointer).
 */
struct UPnPsdk_API SSockaddr {
    /// Reference to sockaddr_storage struct
    sockaddr_storage& ss = m_sa_union.ss;
    /// Reference to sockaddr_un struct
    sockaddr_un& sun = m_sa_union.sun;
    /// Reference to sockaddr_in6 struct
    sockaddr_in6& sin6 = m_sa_union.sin6;
    /// Reference to sockaddr_in struct
    sockaddr_in& sin = m_sa_union.sin;
    /// Reference to sockaddr struct
    sockaddr& sa = m_sa_union.sa;

    /// \cond
    // Constructor
    // -----------
    SSockaddr();

    // Destructor
    // ----------
    ~SSockaddr();
    /// \endcond


    // Copy constructor
    /*! \brief Copy constructor, also needed for copy assignment operator.
     * <!-- ---------------------------------------------------------- -->
     * \code
     * // Usage e.g.:
     * SSockaddr sa2Obj = sa1Obj; // sa1Obj is an instantiated object.
     * // or
     * SSockaddr sa2Obj{sa1Obj};
     * \endcode */
    SSockaddr(const SSockaddr& that) { m_sa_union = that.m_sa_union; }


    // Copy assignment operator
    /*! \brief Copy assignment operator, needs user defined copy contructor
     * <!-- ----------------------------------------------------------- -->
     * \code
     * // Usage e.g.:
     * sa2Obj = sa1Obj; // sa?Obj are two instantiated valid objects.
     * \endcode */
    // Strong exception guarantee with value argument as given.
    SSockaddr& operator=(SSockaddr that) { // value argument
        std::swap(m_sa_union, that.m_sa_union);
        return *this;
    }

    /*! \name Setter
     * *************
     * @{ */
    // Assignment operator= to set socket address from a trivial socket
    // address structure
    // ----------------------------------------------------------------
    /*! \brief Set socket address from a trivial socket address structure
     * \code
     * // Usage e.g.:
     * ::sockaddr_storage ss{};
     * SSockaddr saObj;
     * saObj = ss;
     * \endcode */
    void operator=(const ::sockaddr_storage& a_ss) noexcept;


    /*! \brief clear socket address */
    // -------------------------------
    void clear() noexcept {
        m_sa_union = {};
        m_sa_union.ss.ss_family = AF_UNSPEC;
    }
    /// @} Setter


    /*! \name Getter
     * *************
     * @{ */
    // Compare operator
    // ----------------
    /*! \brief Test if another socket address is logical equal to this
     * \code
     * // Usage e.g.:
     * if(sa1Obj == sa2Obj) { do_it(); }
     * \endcode
     * \returns
     *  \b true&nbsp; if socket addresses are logical equal\n
     *  \b false otherwise
     *
     * It only supports AF_INET6 and AF_INET. For all other address families
     * \b false is returned. */
    bool operator==(const SSockaddr&) const;


    // Getter for a netaddress
    // -----------------------
    /*! \brief Get the assosiated [netaddress](\ref glossary_netaddr)
     * without port
     * \code
     * // Usage e.g.:
     * SSockaddr saObj;
     * if (saObj.netaddr() == "[::1]") { manage_localhost(); }
     * \endcode
     * \returns
     *   Netaddress from socket address. */
    std::string netaddr() noexcept;


    // Getter for a netaddress with port
    // ---------------------------------
    /*! \brief Get the assosiated [netaddress](\ref glossary_netaddr) with
     * port
     * \code
     * // Usage e.g.:
     * SSockaddr saObj;
     * if (saObj.netaddrp() == "[::1]:49494") { manage_localhost(); }
     * \endcode
     * \returns
     *   Netaddress with port from socket address. */
    std::string netaddrp() noexcept;


    /// \brief Get sizeof the current filled (sin6 or sin) Sockaddr
    /// Structure
    // ------------------------------------------------------------
    socklen_t sizeof_saddr() const noexcept;

    /// @} Getter


  private:
    sockaddr_t m_sa_union{}; // this is the union of trivial sockaddr
                             // structures that is managed.
};


// Getter of the netaddress to output stream
// -----------------------------------------
/*! \brief output the [netaddress](\ref glossary_netaddr) to a stream.
 * \ingroup upnplib-addrmodul
 * \code
 * // Usage e.g.:
 * SSockaddr saObj;
 * std::cout << saObj << "\n"; // output "[::]:0", saObj wasn't set.
 * \endcode
 */
UPnPsdk_API ::std::ostream& operator<<(::std::ostream& os, SSockaddr& saddr);

} // namespace UPnPsdk

#endif // UPnPsdk_NET_SOCKADDR_HPP
