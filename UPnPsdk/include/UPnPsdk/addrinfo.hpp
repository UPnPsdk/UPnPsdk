#ifndef UPnPsdk_INCLUDE_ADDRINFO_HPP
#define UPnPsdk_INCLUDE_ADDRINFO_HPP
// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-11
/*!
 * \file
 * \brief Declaration of the Addrinfo class.
 */

#include <UPnPsdk/sockaddr.hpp>

namespace UPnPsdk {

/*!
 * \brief Get information from the operating system about an internet address
<!-- ==================================================================== -->
 * \ingroup upnplib-addrmodul
 *
 * An empty node returns information of the loopback interface, but either node
 * or service, but not both, may be empty. With setting everything unspecified,
 * except service, we get all available combinations with loopback interfaces
 * but different on platforms. \b a_socktype specifies the preferred socket
 * type SOCK_STREAM or SOCK_DGRAM. Specifying \b 0 for this argument indicates
 * that socket addresses of any socket type can be returned. For example:
 * \code
 * CAddrinfo ai("", 0, 0); // same as
 * CAddrinfo ai("", "0", 0, 0);
 * \endcode
 * may find\n
 * "[::1]" (SOCK_STREAM), "[::1]" (SOCK_DGRAM),\n
 * "127.0.0.1" (SOCK_STREAM), "127.0.0.1" (SOCK_DGRAM).
 *
 * To get default SOCK_STREAM loopback interfaces just use:
 * \code
 * CAddrinfo ai("");
 * \endcode
 * May find, where find_first() should be preferred what ever it finds:\n
 * find_first() -> "[::1]"\n
 * find_next()  -> "127.0.0.1"
 *
 * To get address information for **passive listening** on all local network
 * adapters with default SOCK_STREAM, \b a_node must be empty, but not \b
 * a_service and \b a_flags must be set at least to AI_PASSIVE, for example:
 * \code
 * CAddrinfo ai("", AI_PASSIVE | AI_NUMERICHOST); // same as
 * CAddrinfo ai("", "0", AI_PASSIVE | AI_NUMERICHOST);
 * \endcode
 * Of course you can set a specific port (a_service) other than \b 0.
 */
// This is a stripped version. It is only as snapshot to get information about
// a netaddress. There is no need to copy the object. The last full featured
// version with copy constructor, copy asignment operator, compare operator,
// additional getter and its unit tests can be found at Github commit
// e2ffc0c46a2d8f15390f2816e1a18782e500fd09
class UPnPsdk_VIS CAddrinfo {
  public:
    /// \brief Constructor for getting an address information with service name
    //  -----------------------------------------------------------------------
    CAddrinfo(
        /*! [in] Name or address string of a node, e.g. "example.com" or
           "[2001.db8::1]". */
        std::string_view a_node,
        /*! [in] Service name resp. port can also be a port number string, e.g.
         * "https" or "443". */
        std::string_view a_service,
        /*! [in] Optional: flags that can be "or-ed", e.g. AI_PASSIVE |
         * AI_NUMERICHOST. Details at <a
         * href="https://www.man7.org/linux/man-pages/man3/getaddrinfo.3.html">getaddrinfo
         * — Linux manual page</a> or <a
         * href="https://learn.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-getaddrinfo#use-of-ai-flags-in-the-phints-parameter">getaddrinfo
         * — Microsoft Learn</a> */
        const int a_flags = 0,
        /*! [in] Optional: can be SOCK_STREAM, SOCK_DGRAM, or \b 0 (any
         * possible socket type). */
        const int a_socktype = SOCK_STREAM);


    /*! \brief Constructor for getting an address information from only an
     * internet address */
    // -------------------------------------------------------------------
    CAddrinfo(
        /*! [in] Name or address string of a node, e.g. "example.com:50001" or
         * "[2001.db8::1]:50002" or "2001.db8::2". */
        std::string_view a_node,
        /*! [in] Optional: flags that can be "or-ed", e.g. AI_PASSIVE |
         * AI_NUMERICHOST. Details at <a
         * href="https://www.man7.org/linux/man-pages/man3/getaddrinfo.3.html">getaddrinfo
         * — Linux manual page</a> or <a
         * href="https://learn.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-getaddrinfo#use-of-ai-flags-in-the-phints-parameter">getaddrinfo
         * — Microsoft Learn</a> */
        const int a_flags = 0,
        /*! [in] Optional: can be SOCK_STREAM, SOCK_DGRAM, or \b 0 (any
         * possible socket type). */
        const int a_socktype = SOCK_STREAM);

    /// \cond
    // Destructor
    virtual ~CAddrinfo();

    // Copy constructor
    // We cannot use the default copy constructor because there is also
    // allocated memory for the addrinfo structure to copy. We get segfaults
    // and program aborts. This class is not usable for copying the object.
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
     * if (!aiObj.get_first())
     *     handle_error();
     * if (aiObj->ai_socktype == SOCK_DGRAM) {} // is SOCK_STREAM here
     * if (aiObj->ai_family == AF_INET6) { handle_ipv6(); };
     * \endcode
     *
     * The operating system returns the information in a structure that you can
     * read to get all details. */
    // REF:_<a_href="https://stackoverflow.com/a/8782794/5014688">Overloading_member_access_operators_->,_.*</a>
    const ::addrinfo* operator->() const noexcept;


    /*! \brief Get the first entry of an address info from the operating system
     * \code
// Usage e.g.:
CAddrinfo ai("[2001:db8::1]", "50050", AF_UNSPEC, SOCK_STREAM, AI_NUMERICHOST);
// or
CAddrinfo ai("[2001:db8::1]:50050");
if (!ai.get_first()) {
    std::cerr << ai.what() << '\n';
    handle_failed_address_info();
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
     * \returns
     *  \b true if address information is available\n
     *  \b false otherwise */
    bool get_first();


    /*! \brief Get next available address information
     * \code
     * // Usage e.g.:
     * CAddrinfo aiObj("localhost");
     * if (!aiObj.get_first())
     *     handle_error();
     * do {
     *     int af = aiObj->ai_family;
     *     std::cout << "AF=" << af << "\n";
     * } while (aiObj.get_next()); // handle next addrinfo
     * \endcode
     * If more than one address information is available this is used to switch
     * to the next addrinfo.
     * \returns
     *  \b true if address information is available\n
     *  \b false otherwise */
    bool get_next() noexcept;


    /// \brief Get the socket address from current selcted address information
    void sockaddr( //
        /*! [out] Reference to a socket address structure that will be filled
         * with the address information. If no information is available (e.g.
         * CAddrinfo::get_first() wasn't called) an unspecified socket address
         * is returned (netaddr "", netaddrp ":0"). */
        SSockaddr& a_saddr);

    /*! \brief Get cached error message
     * \code
     * // Usage e.g.:
     * CAddrinfo ai("[2001:db8::1]:50050");
     * if (!ai.get_first()) {
     *     std::cerr << ai.what() << '\n';
     *     handle_failed_address_info();
     * }
     * \endcode
     * It should be noted that are different error messages returned by
     * different platforms. */
    const std::string& what() const;
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
    // info is available it is modified with CAddrinfo::get_next() or
    // CAddrinfo::find().
    ::addrinfo* m_res_current{&m_hints};

    // Storage for a message in case of an error, that can be called afterwards.
    SUPPRESS_MSVC_WARN_4251_NEXT_LINE
    std::string m_error_msg{"Success."};

    // Private method to free allocated memory for address information.
    void free_addrinfo() noexcept;
};

} // namespace UPnPsdk

#endif // UPnPsdk_INCLUDE_ADDRINFO_HPP
