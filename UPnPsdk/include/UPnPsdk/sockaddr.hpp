#ifndef UPnPsdk_NET_SOCKADDR_HPP
#define UPnPsdk_NET_SOCKADDR_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-12-01
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


// Free function to get the port number from a string
// --------------------------------------------------
/*! \brief Get the port number from a string
 * \ingroup upnplib-addrmodul
 * \code
 * // Usage e.g.:
 * in_port_t port;
 * try { port = to_port(service);
 *     // do normal things.
 * } catch (const std::out_of_range& e) {
 *     // manage error: service number too big.
 * } catch (const std::invalid_argument& e) {
 *     // manage error or alphanumeric service name, e.g. "http".
 * }
 * \endcode
 *
 * Checks if the given string represents a numeric value between 0 and 65535.
 * \returns
 *  On success: Value of the port number, an empty string returns 0.
 *
 * \exception std::out_of_range A valid number was given but it is not in the
 * range 0..65535.
 * \exception std::invalid_argument Invalid number string but may be a valid
 * alphanumeric service name like "http".
 * \exception std::logic_error Both exceptions above are derived from this one.
 * If you only want to catch any error you can use this one.
 */
in_port_t to_port(
    /// [in] String that shall be converted to a numeric port number.
    const std::string& a_port_str);


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
