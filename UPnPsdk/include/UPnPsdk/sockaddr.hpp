#ifndef UPnPsdk_NET_SOCKADDR_HPP
#define UPnPsdk_NET_SOCKADDR_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-11-28
/*!
 * \file
 * \brief Declaration of the Sockaddr class and some free helper functions.
 */

#include <UPnPsdk/visibility.hpp>
#include <UPnPsdk/port.hpp>
#include <UPnPsdk/port_sock.hpp>
/// \cond
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
    ::sockaddr_in6 sin6;
    ::sockaddr_in sin;
    ::sockaddr sa;
};


// Free function to check if a string is a valid port number
// ---------------------------------------------------------
/*! \brief Check if a given string represents a port number
 * \ingroup upnplib-addrmodul
 * \code
 * // Usage e.g.:
 * if (is_numport("55555") == 0) { manage_given_port(); }
 * \endcode
 *
 * Checks if the given string represents a numeric port number between 0 and
 * 65535.
 *
 * \returns
 *  - **-1** if *a_port_str* isn't a numeric number, but it may be a valid
 *           service name (e.g. "https")
 *  - **0** if *a_port_str* is a valid numeric port number between 0 and 65535
 *  - **1** if *a_port_str* is an invalid numeric port number > 65535
 */
int is_numport(const std::string& a_port_str) noexcept;


// Free function to get the address string with port from a sockaddr structure
// ---------------------------------------------------------------------------
/*! \brief Get the [netaddress](\ref glossary_netaddr) with port from a sockaddr
 * structure
 * \ingroup upnplib-addrmodul
 * \code
 * // Usage e.g.:
 * ::sockaddr_storage saddr{};
 * std::cout << "netaddress is " << to_netaddrp(&saddr) << "\n";
 * \endcode
 */
UPnPsdk_API std::string
to_netaddrp(const ::sockaddr_storage* const a_sockaddr) noexcept;


/*!
 * \brief Trivial ::%sockaddr structures enhanced with methods
 * <!--   ==================================================== -->
 * \ingroup upnplib-addrmodul
\code
// Usage e.g.:
::sockaddr_storage saddr{};
SSockaddr saObj;
::memcpy(&saObj.ss, &saddr, saObj.sizeof_ss());
std::cout << "netaddress of saObj is " << saObj.netaddr() << "\n";
\endcode
 *
 * This structure should be usable on a low level like the trival C `struct
 * ::%sockaddr_storage` but provides additional methods to manage its data.
 */
struct UPnPsdk_API SSockaddr {
    /// Reference to sockaddr_storage struct
    sockaddr_storage& ss = m_sa_union.ss;
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
    virtual ~SSockaddr();

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
     * saObj = "[2001:db8::1]";
     * saObj = "[2001:db8::1]:50001";
     * saObj = "192.168.1.1";
     * saObj = "192.168.1.1:50001";
     *  \endcode
     * An empty netaddress "" clears the address storage.
     * \exception std::invalid_argument Invalid netaddress
     *
     * Assign rules are:\n
     * a netaddress consists of two parts, ip address and port. A netaddress
     * has always a port. With an invalid ip address the whole netaddress is
     * unspecified and results to "". Valid special cases are these well
     * defined unspecified addresses:
\verbatim
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
    */
    void operator=(
        /// [in] String with a possible netaddress
        const std::string& a_addr_str); // noexept?


    // Assignment operator to set a port
    // ---------------------------------
    /*! \brief Set [port number](\ref glossary_port) from integer
     * \code
     * // Usage e.g.:
     * SSockaddr saObj;
     * saObj = 50001;
     * \endcode */
    void operator=(const in_port_t a_port);
    /// @} Setter


    /*! \name Getter
     * *************
     * @{ */
    // Compare operator
    // ----------------
    /*! \brief Test if another socket address is logical equal to this
     * \returns
     *  \b true if socket addresses are logical equal\n
     *  \b false otherwise
     *
     * It only supports AF_INET6 and AF_INET. For all other address families it
     * returns false. */
    bool operator==(const ::sockaddr_storage&) const;


    // Getter for a netaddress
    // -----------------------
    /*! \brief Get the assosiated [netaddress](\ref glossary_netaddr) without
     * port
     * \code
     * // Usage e.g.:
     * SSockaddr saObj;
     * if (saObj.netaddr() == "[::1]") { manage_localhost(); }
     * \endcode */
    virtual const std::string& netaddr();


    // Getter for a netaddress with port
    // ---------------------------------
    /*! \brief Get the assosiated [netaddress](\ref glossary_netaddr) with port
     * \code
     * // Usage e.g.:
     * SSockaddr saObj;
     * if (saObj.netaddrp() == "[::1]:49494") { manage_localhost(); }
     * \endcode */
    virtual const std::string& netaddrp();

    /// \brief Get the numeric port
    virtual in_port_t get_port() const;

    /// \brief Get sizeof the Sockaddr Structure
    socklen_t sizeof_ss() const;

    /// Get sizeof the current (sin6 or sin) Sockaddr Structure
    socklen_t sizeof_saddr() const;
    /// @} Getter

  private:
    sockaddr_t m_sa_union{}; // this is the union of trivial sockaddr structures
                             // that is managed.

    // Two buffer to have the strings valid for the lifetime of the object. This
    // is important for pointer to the string, for example with getting a C
    // string by using '.c_str()'.
    SUPPRESS_MSVC_WARN_4251_NEXT_LINE
    std::string m_netaddr; // For a netaddress without port
    SUPPRESS_MSVC_WARN_4251_NEXT_LINE
    std::string m_netaddrp; // For a netaddress with port

    UPnPsdk_LOCAL void handle_ipv6(const std::string& a_addr_str);
    UPnPsdk_LOCAL void handle_ipv4(const std::string& a_addr_str);
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
UPNPLIB_API ::std::ostream& operator<<(::std::ostream& os, SSockaddr& saddr);

} // namespace UPnPsdk

#endif // UPnPsdk_NET_SOCKADDR_HPP
