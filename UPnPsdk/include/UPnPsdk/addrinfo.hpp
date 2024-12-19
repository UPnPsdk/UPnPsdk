#ifndef UPnPsdk_INCLUDE_ADDRINFO_HPP
#define UPnPsdk_INCLUDE_ADDRINFO_HPP
// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-12-19
/*!
 * \file
 * \brief Declaration of the Addrinfo class.
 */

#include <UPnPsdk/visibility.hpp>
#include <UPnPsdk/port.hpp>
#include <UPnPsdk/port_sock.hpp>
#include <UPnPsdk/sockaddr.hpp>
#include <string>


namespace UPnPsdk {

/*!
 * \brief Get information from the operating system about an internet address
<!-- ==================================================================== -->
 * \ingroup upnplib-addrmodul
 */
// This is a stripped version. It is only as snapshot to get information about
// a netaddress. There is no need to copy the object. The last full featured
// version with copy constructor, copy asignment operator, compare operator,
// additional getter and its unit tests can be found at Github commit
// e2ffc0c46a2d8f15390f2816e1a18782e500fd09
class UPnPsdk_API CAddrinfo {
  public:
    /*! \brief Constructor for getting an address information with service name
     * \details The service name can also be a port number string, e.g. "http"
     * or "80" */
    CAddrinfo(std::string_view a_node, std::string_view a_service,
              const int a_family = AF_UNSPEC,
              const int a_socktype = SOCK_STREAM, const int a_flags = 0,
              const int a_protocol = 0);

    /*! \brief Constructor for getting an address information from only a
     * [netaddress](\ref glossary_netaddr) */
    CAddrinfo(std::string_view a_node, const int a_family = AF_UNSPEC,
              const int a_socktype = SOCK_STREAM, const int a_flags = 0,
              const int a_protocol = 0);

  private:
    /// \brief Helper method for common tasks on different constructors
    void set_ai_flags(const int a_family, const int a_socktype,
                      const int a_flags, const int a_protocol) noexcept;

  public:
    /// \cond
    // Destructor
    virtual ~CAddrinfo();

    // Copy constructor
    // We cannot use the default copy constructor because there is also
    // allocated memory for the addrinfo structure to copy. We get segfaults
    // and program aborts. This class is not used to copy the object.
    CAddrinfo(const CAddrinfo&) = delete;

    // Copy assignment operator
    // Same as with the copy constructor.
    CAddrinfo& operator=(CAddrinfo) = delete;
    /// \endcond


    /*! \name Getter
     * *************
     * @{ */
    /*! \brief Read access to members of the <a
     * href="https://www.man7.org/linux/man-pages/man3/getaddrinfo.3.html#DESCRIPTION">addrinfo
     * structure</a>
     * \code
     * // Usage e.g.:
     * CAddrinfo aiObj("localhost", "50001", AF_UNSPEC, SOCK_STREAM);
     * try {
     *     aiObj.get_first();
     * } catch (xcp) { handle_error(); }
     * if (aiObj->ai_socktype == SOCK_DGRAM) {} // is SOCK_STREAM here
     * if (aiObj->ai_family == AF_INET6) { handle_ipv6(); };
     * \endcode
     *
     * The operating system returns the information in a structure that you can
     * read to get all details. */
    // REF:_<a_href="https://stackoverflow.com/a/8782794/5014688">Overloading_member_access_operators_->,_.*</a>
    ::addrinfo* operator->() const noexcept;


    /*! \brief Get the first entry of an address info from the operating system
     * \code
// Usage e.g.:
CAddrinfo ai("[2001:db8::1]", "50050", AF_UNSPEC, SOCK_STREAM, AI_NUMERICHOST);
// or
CAddrinfo ai("[2001:db8::1]:50050");
try {
    ai.get_first();
} catch (const std::runtime_error& e) { handle_failed_address_info(); }
// will also catch (const std::range_error& e) { port out of range 0..65535(); }
// if that is not explicit specified.
}
normal_execution();
     * \endcode
     * \note It is important to careful check the error situation because
     * loading information depends on the real environment that we cannot
     * control. Name resolution may fail because to be unspecified, DNS server
     * may be temporary down, etc.
     *
     * Usually this getter is called one time after constructing the object.
     * This gets an address information from the operating system that may also
     * use its internal name resolver inclusive contacting external DNS server.
     * If you use the flag **AI_NUMERICHOST** with the constructor then a
     * possible expensive name resolution to DNS server is suppressed.
     *
     * If you have iterated address information entries with
     * CAddrinfo::get_next() and want to restart you can do it with call
     * CAddrinfo::get_first() again. It will read the information from the
     * operating system again and could be used to monitor changes of address
     * information. But note that this is quite expensive because always memory
     * is freed and new allocated for the information list so doing this in a
     * busy loop is not very useful.
     *
     * \exception std::runtime_error Failed to get address information, node or
     * service not known. Maybe an alphanumeric node name that cannot be
     * resolved. Or the DNS server is temporary not available. This exeption is
     * parent from std::range_error and will also catch it if that is not
     * explicit catched.
     * \exception std::range_error The service number representing a port
     * number is a numeric value but out of valid range 0..65535. This
     * exception is derived from std::runtime_error.
     *
     * It should be noted that are different error messages returned by
     * different platforms.
     */
    void get_first();


    /*! \brief Get next available address information
     * \code
     * // Usage e.g.:
     * CAddrinfo aiObj("localhost");
     * try {
     *     aiObj.get_first();
     * } catch (xcp) { handle_error(); }
     * do {
     *     int af = aiObj->ai_family;
     *     std::cout << "AF=" << af << "\n";
     * } while (aiObj.get_next()); // handle next addrinfo
     * \endcode
     *
     * If more than one address information is available this is used to switch
     * to the next addrinfo.
     * \returns
     *  \b true if address information is available\n
     *  \b false otherwise */
    bool get_next() noexcept;


    /*! \brief Get [netaddress](\ref glossary_netaddr) with port from current
     * selcted address information */
    std::string netaddrp() noexcept;


    /// \brief Get the socket address from current selcted address information
    void sockaddr( //
        /*! [in,out] Reference to a socket address structure that will be
         * filled with the address information. If no information is available
         * (at least CAddrinfo::get_first() wasn't called) the structure is
         * not modified. */
        SSockaddr& a_saddr);
    /// @} Getter

  private:
    // Cache the hints that are given with the constructor by the user, so we
    // can always get identical address information from the operating system.
    DISABLE_MSVC_WARN_4251
    const std::string m_node;
    const std::string m_service;
    ENABLE_MSVC_WARN
    addrinfo m_hints{};

    // Pointer to the address information returned from systemcall
    // ::getaddrinfo(). This pointer must be freed. That is done with the
    // destructor. It is initialized to point to the hints so there is never a
    // dangling pointer that may segfault. Pointing to the hints means there is
    // no information available, e.g.
    // if (m_res == &m_hints) { // do nothing }
    ::addrinfo* m_res{&m_hints};
    // This points to the current used address info. If more than one address
    // info is available it is modified with this->get_next().
    ::addrinfo* m_res_current{&m_hints};

    // Private method to free allocated memory for address information.
    void free_addrinfo() noexcept;
};

} // namespace UPnPsdk

#endif // UPnPsdk_INCLUDE_ADDRINFO_HPP
