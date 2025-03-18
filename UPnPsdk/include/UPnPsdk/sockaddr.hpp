#ifndef UPnPsdk_NET_SOCKADDR_HPP
#define UPnPsdk_NET_SOCKADDR_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-03-18
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
 * \brief Free function to split inet address and port(service)
 * <!-- --------------------------------------------------- -->
 * \ingroup upnplib-addrmodul
 * \code
 * // Usage e.g., not a complete list:
 * std::string addr, serv;
 * split_addr_port("[2001:db8::1]:50001", addr, serv);
 * split_addr_port("2001:DB8::1", addr, serv);
 * split_addr_port("127.0.0.1:0", addr, serv);
 * split_addr_port("127.0.0.1", addr, serv);
 * split_addr_port(":50002", addr, serv);
 * split_addr_port("example.COM:50003", addr, serv);
 * split_addr_port("example.com:HTTPS", addr, serv);
 * split_addr_port("[::FFff:142.250.185.99]:ssh", addr, serv);
 * \endcode
 *
 * This is a function for special use to prepare input for system call
 * ::%getaddrinfo(). Its results returned in \b a_addr and \b a_serv are only
 * useful for ::%getaddrinfo() and are not meant for general usage. For example
 * returned IPv6 addresses never have brackets because ::%getaddrinfo() does not
 * accept them, port numbers are limited to 65535 because ::%getaddrinfo()
 * accepts also greater numbers with overrun to 65535 + 1 = 0.
 * */
void split_addr_port( //
    /*! [in] Any string. If it can be interpreted as an ip-address or -name
     * with or without port number or name, its parts will be retured. */
    const std::string& a_addr_str,
    /*! [in,out] Reference of a string that will be filled with the ip address
       part. This can also be a alphanumeric name like "example.com" */
    std::string& a_addr,
    /// [in,out] Reference of a string that will be filled with the port part.
    std::string& a_serv);


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
\endverbatim
     * \exception std::invalid_argument
     *            Invalid [netaddress](\ref glossary_netaddr).
    */
    void operator=(
        /// [in] String with a possible netaddress
        const std::string& a_addr_str); // noexept?


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
     *  \b true if socket addresses are logical equal\n
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
     * \endcode */
    const std::string& netaddr() noexcept;


    // Getter for a netaddress with port
    // ---------------------------------
    /*! \brief Get the assosiated [netaddress](\ref glossary_netaddr) with port
     * \code
     * // Usage e.g.:
     * SSockaddr saObj;
     * if (saObj.netaddrp() == "[::1]:49494") { manage_localhost(); }
     * \endcode */
    const std::string& netaddrp() noexcept;


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

    // Two buffer to have the strings valid for the lifetime of the object. This
    // is important for pointer to the string, for example with getting a C
    // string by using '.c_str()'.
    SUPPRESS_MSVC_WARN_4251_NEXT_LINE
    std::string m_netaddr; // For a netaddress without port
    SUPPRESS_MSVC_WARN_4251_NEXT_LINE
    std::string m_netaddrp; // For a netaddress with port
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
