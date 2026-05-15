#ifndef UPnPsdk_NET_SOCKADDR_HPP
#define UPnPsdk_NET_SOCKADDR_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-05-15
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
 *      so you can use it in your application without conversion. If you want
 *      to store it in a netaddr structure you must use <b>%htons()</b>. An
 *      empty input string returns 0.\n
 *   On error:
 *   - **-1** A valid port number was not found.
 *   - &nbsp;**1** Valid numeric value found but out of scope, not in range
 *                 0..65535.
 */
int to_port( //
    /*! [in] String that may represent a port number. */
    std::string_view a_port_str,
    /*! [in,out] Optional: if given, pointer to a variable that will be filled
     *           with the binary port number in host byte order. */
    in_port_t* const a_port_num = nullptr) noexcept;


/*! \brief Components of an internet address
 * \details Typical LLA example with all [netaddress](\ref glossary_netaddr)
 * components:\n
 * "[fe80::1%2]:443" with node "fe80::1", scope "2", service "443"\n
 * "[fe80::2%eth0]:https" with node "fe80::2", scope "eth0", service "https" */
struct inaddr_t {
    std::string node; /*!< IP address without brackets. This can also be a
                         alphanumeric name like "example.com". */
    std::string scope; /*!< scope_id is the index number or name  of a
                         [netadapter](\ref glossary_netadapt). Only available
                         on a link-local address. */
    std::string service; /*!< Port number, or service name (e.g. "https"). */
};

/*!
 * \brief Free function to split inet address, scope_id, and port(service)
 * <!-- -------------------------------------------------------------- -->
 * \ingroup upnplib-addrmodul
 * \code
 * // Usage e.g., not a complete list:
 * inaddr_t inaddr;
 * split_inaddr("[2001:db8::1]:50001", inaddr);
 * split_inaddr("2001:DB8::1", inaddr);
 * split_inaddr("[fe80::2%3]:50002", inaddr);
 * split_inaddr("127.0.0.1:0", inaddr);
 * split_inaddr("127.0.0.1", inaddr);
 * split_inaddr(":50002", inaddr);
 * split_inaddr("example.COM:50003", inaddr);
 * split_inaddr("example.com:HTTPS", inaddr);
 * split_inaddr("[::FFff:142.250.185.99]:ssh", inaddr);
 * \endcode
 *
 * This is a function for special use to prepare input for system calls
 * without brackets. Its results returned in \b a_addr, \b a_scope, and \b
 * a_serv are only useful for this purpose and not meant for general usage. The
 * function only syntactical split the components on its separator '\%' and ':'.
 * No syntactical tests are made. For example a scope_id on a global unicast
 * address, or a port number greater 65535 is not valid but also provided in \b
 * a_scope, resp. \b a_serv. These tests must be made on a higher abstraction
 * layer.
 * */
void split_inaddr( //
    /*! [in] Any string. If it can be interpreted as an ip-address or -name with
       or without scope_id and/or service (port), its components will be
       returned. */
    const std::string_view a_addr_sv,
    /*! [out] Reference of an internet address structure that will be filled
       with node, scope_id, and service (port). */
    inaddr_t& a_spl_inaddr) noexcept;


/*!
 * \brief Trivial ::%sockaddr structures enhanced with methods
 * <!--   ==================================================== -->
 * \ingroup upnplib-addrmodul
\code
// Usage e.g.:
::sockaddr_storage saddr{};
SSockaddr saObj;
::memcpy(&saObj.ss, &saddr, sizeof(saObj.ss));
std::cout << "netaddress of saObj is " << saObj << "\n";
\endcode
 *
 * This structure should be usable on a low level like the trival C `struct
 * ::%sockaddr_storage` but provides additional methods to manage its data.
 * When ever this SDK manage a network address it uses an object of this class.
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

    // Constructor
    // -----------
    SSockaddr();

    // Destructor
    // ----------
    ~SSockaddr();

    // Get reference to the sockaddr_storage structure.
    // Only as example, I don't use it because it may be confusing. I only use
    // SSockaddr::ss (instantiated e.g. ssObj.ss) to access the trivial
    // member structure.
    // operator const ::sockaddr_storage&() const;

    // Copy constructor
    /*! \brief Copy constructor, also needed for copy assignment operator.
     * <!-- ---------------------------------------------------------- -->
     * \code
     * // Usage e.g.:
     * SSockaddr saddr2 = saddr1; // saddr1 is an instantiated object.
     * // or
     * SSockaddr saddr2{saddr1};
     * \endcode */
    SSockaddr(const SSockaddr&);


    // Copy assignment operator
    /*! \brief Copy assignment operator, needs user defined copy contructor
     * <!-- ----------------------------------------------------------- -->
     * \code
     * // Usage e.g.:
     * saddr2 = saddr1; // saddr? are two instantiated valid objects.
     * \endcode */
    // Strong exception guarantee with value argument as given.
    SSockaddr& operator=(SSockaddr); // value argument


    /*! \name Setter
     * *************
     * @{ */
    // Assignment operator to set a netaddress
    // ---------------------------------------
    /*! \brief Set socket address from a [netaddress](\ref glossary_netaddr)
     * \code
     * // Usage e.g.:
     * SSockaddr saObj;
     * saObj = ""; // Clears the address storage.
     * saObj = "[2001:db8::1]";
     * saObj = "[2001:db8::1]:50001";
     * saObj = "192.168.1.1";
     * saObj = "192.168.1.1:50001";
     *  \endcode
     * Assign rules are:\n
     * a netaddress consists of two parts, ip address and port. A netaddress
     * has always a port. A cleared socket address is empty. On an empty socket
     * address\n
     * SSockaddr::netaddr() returns "" (empty string)\n
     * SSockaddr::netaddrp() returns ":0"\n\n
     * Valid special cases are these well defined unspecified addresses:
\verbatim
""              results to  ":0"
":0"            results to  ":0"
"65535"         results to  ":65535"
"::"            results to  "[::]:0"
"[::]"          results to  "[::]:0"
"[::]:"         results to  "[::]:0"
"[::]:0"        results to  "[::]:0"
"[::]:65535"    results to  "[::]:65535" // port 0 to 65535
"0.0.0.0"       results to  "0.0.0.0:0"
"0.0.0.0:"      results to  "0.0.0.0:0"
"0.0.0.0:0"     results to  "0.0.0.0:0"
"0.0.0.0:65535" results to  "0.0.0.0:65535" // port 0 to 65535
\endverbatim
     * A valid address with an invalid port results to port 0, for example\n
\verbatim
"[2001:db8::51]:98765" results to "[2001:db8::51]:0"
\endverbatim
     * Setting only the port number does not modify the address part.\n
\verbatim
"[2001:db8::52]:50001" results to "[2001:db8::52]:50001"
              ":55555" results to "[2001:db8::52]:55555" (address prev setting)
               "55556" same as before with leading colon.
\endverbatim
    * **On error**\n
    * The socket-address Object is reset, means it is empty. Check with
    * `saObj.ss.ss_family == AF_UNSPEC`. Typical errors are:
    * - link-local address without scope_id
    * - other IPv6 address with scope_id
    * - port number greater than 65535
    */
    void operator=(
        /// [in] String with a possible netaddress
        const std::string_view a_addr_str) noexcept;


    // Assignment operator= to set socket port from an integer
    // -------------------------------------------------------
    /*! \brief Set [port number](\ref glossary_port) from integer
     * \code
     * // Usage e.g.:
     * SSockaddr saObj;
     * saObj = 50001;
     * \endcode */
    void operator=(const in_port_t a_port);


    // Assignment operator= to set socket address from a trivial socket address
    // structure
    // ------------------------------------------------------------------------
    /*! \brief Set socket address from a trivial socket address structure
     * \code
     * // Usage e.g.:
     * ::sockaddr_storage ss{};
     * SSockaddr saObj;
     * saObj = ss;
     * \endcode */
    void operator=(const ::sockaddr_storage& a_ss);
    /// @} Setter


    /*! \name Getter
     * *************
     * @{ */
    // Compare operator
    // ----------------
    /*! \brief Test if another socket address is logical equal to this
     * \returns
     *  \b true&nbsp; if socket addresses are logical equal\n
     *  \b false otherwise
     *
     * It only supports AF_INET6 and AF_INET. For all other address families \b
     * false is returned. */
    bool operator==(const SSockaddr&) const;


    // Getter for a netaddress
    // -----------------------
    /*! \brief Get the assosiated [netaddress](\ref glossary_netaddr) without
     * port
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
    /*! \brief Get the assosiated [netaddress](\ref glossary_netaddr) with port
     * \code
     * // Usage e.g.:
     * SSockaddr saObj;
     * if (saObj.netaddrp() == "[::1]:49494") { manage_localhost(); }
     * \endcode
     * \returns
     *   Netaddress with port from socket address. */
    std::string netaddrp() noexcept;


    /// \brief Get the numeric port
    // ----------------------------
    in_port_t port() const;


    /// \brief Get sizeof the current filled (sin6 or sin) Sockaddr Structure
    // ----------------------------------------------------------------------
    socklen_t sizeof_saddr() const;
    /// @} Getter


    /// \brief Get if the socket address is a loopback address
    // -------------------------------------------------------
    bool is_loopback() const;


  private:
    sockaddr_t m_sa_union{}; // this is the union of trivial sockaddr structures
                             // that is managed.
};


// Getter of the netaddress to output stream
// -----------------------------------------
/*! \brief output the [netaddress](\ref glossary_netaddr)
 * \ingroup upnplib-addrmodul
 * \code
 * // Usage e.g.:
 * SSockaddr saObj;
 * saObj = "[2001:db8::1]:56789";
 * std::cout << saObj << "\n"; // output "[2001:db8::1]:56789"
 * \endcode
 */
UPnPsdk_API ::std::ostream& operator<<(::std::ostream& os, SSockaddr& saddr);

} // namespace UPnPsdk

#endif // UPnPsdk_NET_SOCKADDR_HPP
