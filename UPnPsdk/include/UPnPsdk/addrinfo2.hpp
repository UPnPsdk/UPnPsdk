#ifndef UPnPsdk_ADDRINFO_HPP
#define UPnPsdk_ADDRINFO_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-05-31
/*!
 * \file
 * \brief Manage information about internet addresses
 */

#include <UPnPsdk/sockaddr.hpp>

namespace UPnPsdk {

/*!
 * \brief Get information from the operating system about an internet address
<!-- ==================================================================== -->
 * \ingroup upnplib-addrmodul
 *
 * The results from getting system information using `::%getaddrinfo()` are
 * somewhat confusing and lack a clear systematic pattern over all supported
 * platforms. For example a scope_id of an IPv6 link-local address can be
 * specified with the local network interface index number, or by its name,
 * maybe "[fe80::1%2]" or "[fe80::1%eth0]". Microsoft Windows accepts only
 * numeric scope_ids. MacOs accepts link-local addresses with wrong or missing
 * scope_id, and returns the resulting socket address with no scope_Id (set to
 * 0). This is out of specification. Due to <a
 * href="https://www.rfc-editor.org/rfc/rfc4007.html">RFC 4007</a> an IPv6
 * link-local address must include a scope_id to be valid for routing purposes.
 * All supported platforms accept a valid scope_id on an IPv6 global-unicast
 * address that cannot use it. Different platforms return different values for
 * other properties. Exact details and verification are internally made with
 * `AddrinfoScopeIdFTestSuite`. All of this makes it necessary for the SDK to
 * define the properties of this class as follows:
 * - Address family is AF_INET6. There is no other address family.
 * - Default ai_socktype is SOCK_STREAM. ai_socktype \b 0 is not supported. The
 *   resulting socket type is considered to be the same as given by argument.
 * - ai_protocol is hard coded set to \b 0, and considered to be always \b 0,
 *   that uses the default protocol for the current socket type.
 * - Getting information for an IPv6 link-local address with a numeric scope_id
 *   always succeeds, no matter if the scope_id really exist.
 * - Getting information for an IPv6 link-local address without a valid
 *   scope_id always fails.
 * - Getting information for an IPv6 link-local address with a name of a local
 *   network interface that doesn't exist, fails.
 * - A scope_id on any other IPv6 address that isn't a link-local address,
 *   fails.
 * - The resulting ai_flags are considered to be the same as given by argument
 *   in addition to AI_V4MAPPED, which is always set.
 * - All other resulting information are that from the operating system.
 *
 * \note The SDK uses IPv4-mapped IPv6 addresses. To get the right address
 * information the flag AI_V4MAPPED must always be used. This is hard coded. An
 * issue is that **Microsoft Windows** supports IPv4 mapping only without
 * AI_NUMERICHOST flag set. This means that the flag must be ignored. You can
 * set it but without any effect. You cannot suppress unwanted DNS lookups on
 * Microsoft Windows with CAddrinfo2. Other platforms are not effected.
 *
 * An empty node returns information of the loopback interface, but either node
 * or service, but not both, may be empty. \b a_socktype specifies the socket
 * type SOCK_STREAM (default) or SOCK_DGRAM. Specifying \b 0 for this argument
 * is not supported.
 *
 * To get default SOCK_STREAM loopback interface just use:
 * \code
 * CAddrinfo2 aiObj("");
 * \endcode
 * Will get:\n
 * aiObj.get_first() -> "[::1]"\n
 * aiObj.get_next()  -> false
 * \code
 * CAddrinfo2 aiObj("127.0.0.1");
 * \endcode
 * Will get:\n
 * aiObj.get_first() -> "[::ffff:127.0.0.1]"\n
 * aiObj.get_next()  -> false
 *
 * To get address information for **passive listening** on all local network
 * adapters with default SOCK_STREAM, \b a_node must be empty, but not \b
 * a_service and \b a_flags must be set at least to AI_PASSIVE, for example:
 * \code
 * CAddrinfo2 ai("", AI_PASSIVE | AI_NUMERICHOST);
 * \endcode
 * Will get:\n
 * aiObj.get_first() -> "[::]"\n
 * aiObj.get_next()  -> false
 *
 * Of course you can set a specific port (a_service) other than default \b 0.
 */
// A more featured but maybe outdated version of CAddrinfo2 with copy
// constructor, copy asignment operator, compare operator, additional getter
// and its unit tests can be found at Github commit
// e2ffc0c46a2d8f15390f2816e1a18782e500fd09
class UPnPsdk_VIS CAddrinfo2 {
  public:
    /// \brief Constructor for getting an address information with service name
    //  -----------------------------------------------------------------------
    CAddrinfo2(
        /*! [in] Name or address string of a node, e.g. "example.com:50001" or
         * "[fe80::1%eth0]:50002" or "2001.db8::2". */
        std::string_view a_node,
        /*! [in] Optional: flags that can be "or-ed", e.g. AI_PASSIVE |
         * AI_NUMERICHOST. Details at <a
         * href="https://www.man7.org/linux/man-pages/man3/getaddrinfo.3.html">getaddrinfo
         * — Linux manual page</a> or <a
         * href="https://learn.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-getaddrinfo#use-of-ai-flags-in-the-phints-parameter">getaddrinfo
         * — Microsoft Learn</a> */
        const int a_flags = 0,
        /*! [in] Optional: can be SOCK_STREAM, or SOCK_DGRAM */
        const int a_socktype = SOCK_STREAM);

    /// \cond
    // Destructor
    ~CAddrinfo2();

    // Copy constructor
    // I cannot use the default copy constructor because there is also allocated
    // memory for the addrinfo structure to copy. We get segfaults and program
    // aborts. I need a deep copy to resolve this but that isn't worth the
    // effort. This class is not usable to copy the object.
    CAddrinfo2(const CAddrinfo2&) = delete;

    // Copy assignment operator
    // Same as with the copy constructor.
    CAddrinfo2& operator=(CAddrinfo2) = delete;
    /// \endcond

    /*! \name Getter
     * *************
     * @{ */
    /*! \brief Get the first entry of an address info from the operating system
     *
     * No need to use set_first();
     * \code
// Usage e.g.:
CAddrinfo2 aiObj("example.com:50050"); // Would trigger a DNS lookup.
// same as
CAddrinfo2 aiObj("example.com:50050", 0, SOCK_STREAM);
if (!aiObj.get_first()) {
    handle_failed_address_info();
}
SSockaddr saObj;
aiObj.sockaddr(saObj);
std::cout << "netaddress=\"" << saObj << "\"\n";
     * \endcode
     * \note It is important to careful check the error situation because
     * loading information depends on the real environment that we cannot
     * control. Name resolution may fail because to be unspecified, remote DNS
     * server may be temporary down, etc.
     *
     * Usually this getter is called one time after constructing the object.
     * This gets address information from the operating system that may also
     * use its internal name resolver inclusive contacting external DNS server.
     * If you use the flag **AI_NUMERICHOST** with the constructor then a
     * possible expensive name resolution to DNS server is suppressed.
     *
     * If you have iterated address information entries with
     * CAddrinfo2::get_next() and want to start a new lockup you can do it with
     * call CAddrinfo2::set_first(). It only reset the internal pointer, what's
     * very effective if you only want to examine the same information a second
     * time. If you use CAddrinfo2::get_first() again it will read the
     * information from the operating system again and can be used to monitor
     * changes of address information. But note that this is quite expensive
     * because always memory is freed and new allocated for the information
     * list so doing this in a busy loop is not very useful.
     * \returns
     *  \b true if address information is available\n
     *  \b false otherwise */
    bool get_first();

    /*! \brief Reset the internal list pointer to the first entry
     */
    void set_first() noexcept;

    /// \brief Point to next available address information
    bool get_next() noexcept;

    /// \brief Get the socket address from current selcted address information
    void sockaddr(SSockaddr& a_saddr) const noexcept;
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
    // info is available it is modified with CAddrinfo2::get_next().
    ::addrinfo* m_res_current{&m_hints};

    // Private method to free allocated memory for address information.
    void free_addrinfo() noexcept;
};

} // namespace UPnPsdk

#endif // UPnPsdk_ADDRINFO_HPP
