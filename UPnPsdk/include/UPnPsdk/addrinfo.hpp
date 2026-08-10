#ifndef UPnPsdk_ADDRINFO_HPP
#define UPnPsdk_ADDRINFO_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-08-10
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
 * Design specification
 * --------------------
 * The results from getting system information using <a
 * href="https://www.man7.org/linux/man-pages/man3/getaddrinfo.3.html">\b
::%getaddrinfo()</a>
 * are somewhat confusing and lack a clear systematic pattern over all
 * supported platforms. The goal is to have the same behavior across all
 * platforms. In respect to the issues belonging to <a
 * href="https://www.man7.org/linux/man-pages/man3/getaddrinfo.3.html">\b
::%getaddrinfo()</a>
 * as noted next section, following is specified:
 * - Address family is AF_INET6. There is no other address family.
 * - `ai_socktype` is SOCK_STREAM (default), or SOCK_DGRAM. ai_socktype \b 0 is
 *   not supported. The resulting socket type is considered to be the same as
 *   given by argument.
 * - ai_protocol is hard coded set to \b 0, and considered to be always \b 0,
 *   that uses the default protocol for the current socket type.
 * - Getting information for an IPv6 link-local address, or a mulitcast address
 *   with a numeric scope_id always succeeds, no matter if the
 *   [netadapter](\ref glossary_netadapt) index, used as scope_id, really
 *   exist.
 * - Getting information for an IPv6 link-local address, or mulitcast address
with a
 *   [netadapter](\ref glossary_netadapt) name as scope, that doesn't exist,
 *   fails.
 * - A scope_id on any other IPv6 address that isn't a link-local address, or
 *   multicast address is silently removed.
 * - The resulting ai_flags are considered to be the same as given by argument
 *   in addition to AI_V4MAPPED, which is always hard coded set.
 * - 'CAddrinfo' performs a time-consuming DNS name resolution when necessary.
 *   It is not possible to suppress this (see Note 1 below). This means you
 *   cannot suppress DNS-lookups with AI_NUMERICHOST. This ai_flag is silently
 *   ignored. If you want to get information only from a numeric address
 *   without an unnecessary expensive DNS lookup in an error condition, then
 *   use the 'SSockaddr' class.
 * - All other resulting information are that from the operating system.
 *
 *
 * Issues belonging to '::getaddrinfo()'
 * -------------------------------------
 * Note 1:\n
 *   Only on Microsoft Windows ::%getaddrinfo() does not create AI_V4MAPPED
 *   addresses with AI_NUMERICHOST set. The UPnPsdk only uses IPv6 addresses.
 *   All IPv4 addresses are mapped to IPv6. There is only one combination with
 *   AF_INET6 set and AI_NUMERICHOST unset, where win32 do AI_V4MAPPED. All
 *   others fail. Details with Unit Test 'GetaddrinfoWin32Test'.
 *
 * Note 2:\n
 *   In contrast to other supported platforms, ::%getaddrinfo() on Microsoft
 *   Windows accepts only a numeric scope_id and fails with interface names
 *   (e.g. "Ethernet"). I need a precheck and have to convert it into its index
 *   number (scope_id). <a
 *   href="https://www.man7.org/linux/man-pages/man3/if_nametoindex.3.html">\b
 *   ::%if_nametoindex()</a> and <a
 *
href="https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfacenametoluida">\b
 *   ::%ConvertInterfaceNameToLuidA()</a> with <a
 *
href="https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfaceluidtoindex">\b
 *   ::%ConvertInterfaceLuidToIndex()</a> does not work on Win32. I always get
 *   system "Error 123" that means "The filename, directory name, or volume
 *   label syntax is incorrect", no matter what I tried. I use a workaround
 *   with UPnPsdk::CNetadapter.
 *
 *   <a href="https://www.man7.org/linux/man-pages/man3/if_nametoindex.3.html">
 *   \b ::%if_nametoindex()</a> on macOS, and Linux/GNU works.
 *
 * Note 3:\n
 *   MacOS does not fail ::%getaddrinfo() with an unknown netinterface name and
 *   instead ignores it and returns an LLA addrinfo structure without scope_id.
 *   But that is not specified. Due to <a
 *   href="https://www.rfc-editor.org/rfc/rfc4007.html">RFC 4007</a> an IPv6
 *   link-local address must include a scope_id to be valid for routing
 *   purposes.
 *
 *   Linux platforms ::%getaddrinfo() accept netinterface names but fails if
 *   they don't exist. That is what UPnPsdk use. Details with Unit Test
 *   `AddrinfoScopeIdFTestSuite`.
 *
 * Note 4:\n
 *   ::%getaddrinfo() on macOS accepts link-local addresses with subnet, for
 *   example "[fe80:1::2]". BSD-based operating systems (including macOS)
 *   support an alternative, non-standard syntax, where a numeric zone index is
 *   encoded in the second 16-bit word of the address, as shown in the example
 *   before. But macOS also accepts all other subnets on an LLA with
 *   IN6_IS_ADDR_LINKLOCAL() like e.g. "[fe80:0:1::1]". UPnPsdk does not
 *   support this like Linux platforms, and rejects an LLA with subnet as
 *   unspecified.
 *
 * Please note that either node or service, but not both, may be empty.
 *
 * \anchor caddrinfo_example
 * To get default SOCK_STREAM loopback interface just use:
 * \code
 * SSocaddr saObj;
 *
 * CAddrinfo ai1Obj;
 * assert(ai1Obj.get_first(SInaddr(":0")) == 0);
 * // or assert(ai1Obj.get_first(SInaddr("[::1]")) == 0);
 * ai1Obj.sockaddr(saObj);
 * assert(saObj.netaddrp() == "[::1]:0");
 * assert(ai1Obj.get_next() == false);
 *
 * CAddrinfo ai2Obj;
 * assert(ai2Obj.get_first(SInaddr("127.0.0.1")) == 0);
 * ai2Obj.sockaddr(saObj);
 * assert(saObj.netaddrp() == "[::ffff:127.0.0.1]:0");
 * assert(ai2Obj.get_next() == false);
 * \endcode
 *
 * To get address information for **passive listening** on all local network
 * adapters with default SOCK_STREAM, the node internet address must be empty,
 * but not the port and flags must be set at least to AI_PASSIVE, for example:
 * \code
 * SSocaddr saObj;
 *
 * CAddrinfo ai1Obj(AI_PASSIVE);
 * assert(ai1Obj.get_first(SInaddr(":0")) == 0);
 * ai1Obj.sockaddr(saObj);
 * assert(saObj.netaddrp() == "[::]:0"); // Unspec addr
 * assert(ai1Obj.get_next() == false);
 *
 * // Prepare to listen on all local [netadapter](\ref glossary_netadapt) for
 * // datagrams on port 50001. If internet address token needed for later use:
 * SInaddr inaObj("[::]:https"); // saObj doesn't manage empty node addresses.
 * saObj = inaObj;
 * assert(saObj.empty() == true); // saObj can only manage numeric entries.
 *
 * // Look with name resolution
 * CAddrinfo ai2Obj(AI_PASSIVE, SOCK_DGRAM);
 * assert(ai2Obj.get_first(inaObj) == 0);
 * ai2Obj.sockaddr(saObj);
 * assert(saObj.netaddrp() == "[::]:443"); // "https" is resolved.
 * assert(ai2Obj.get_next() == false);
 * \endcode
 */
// A more featured but outdated version of CAddrinfo with copy constructor,
// copy asignment operator, compare operator, additional getter and its unit
// tests can be found at Github commit e2ffc0c46a2d8f15390f2816e1a18782e500fd09
class UPnPsdk_API CAddrinfo {
  public:
    /// \brief Constructor for getting an address information
    //  -----------------------------------------------------
    CAddrinfo(
        /*! [in] Optional: flags that can be "or-ed", e.g. AI_PASSIVE |
         * AI_NUMERICSERV. Details at <a
         * href="https://www.man7.org/linux/man-pages/man3/getaddrinfo.3.html">getaddrinfo
         * — Linux manual page</a> or <a
         * href="https://learn.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-getaddrinfo#use-of-ai-flags-in-the-phints-parameter">getaddrinfo
         * — Microsoft Learn</a>
         * - Usable flags are:
         *   - AI_PASSIVE
         *   - AI_NUMERICSERV
         *   - AI_ALL
         *   .
         * - Silently ignored flags are:
         *   - AI_V4MAPPED - hard coded set
         *   - AI_NUMERICHOST - hard coded reset
         *   - all other */
        const int a_flags = 0,
        /*! [in] Optional: can be SOCK_STREAM (is default), or SOCK_DGRAM. */
        const int a_socktype = SOCK_STREAM) noexcept;

    /// \cond
    // Destructor
    ~CAddrinfo() noexcept;

    // Copy constructor
    // I cannot use the default copy constructor because there is also allocated
    // memory for the addrinfo structure to copy. We get segfaults and program
    // aborts. I need a deep copy to resolve this but that isn't worth the
    // effort. This class is not usable to copy the object.
    CAddrinfo(const CAddrinfo&) = delete;

    // Copy assignment operator
    // Same as with the copy constructor.
    CAddrinfo& operator=(CAddrinfo) = delete;
    /// \endcond

    /*! \name Getter
     * *************
     * @{ */

    /*! \brief Get the first entry of an address information list from the
     * operating system
     * <!-- ---------------------------------------------------------- -->
     * \code
     * // Usage e.g.:
     * CAddrinfo aiObj;
     * // Triggers a DNS lookup
     * if (aiObj.get_first(SInaddr("example.com:50050") != 0) {
     *     handle_failed_address_info();
     * }
     * SSockaddr saObj;
     * aiObj.sockaddr(saObj);
     * std::cout << "netaddress=\"" << saObj << "\"\n";
     * \endcode
     * For more examples have a look at [CAddrinfo](\ref caddrinfo_example).
     *
     * \exception std::runtime_error Trying to call \b %get_first() a second
     *            time. Information can be loaded only one time.
     */
    // Argument is modified internal. It must not be declared by reference.
    int get_first(SInaddr a_inaddr);

    /// \brief Point to first entry again of an already loaded address info
    void get_first() noexcept { m_res_current = m_res; }

    /// \brief Point to next available address information
    bool get_next() noexcept {
        if (m_res_current == nullptr)
            return false;
        return (m_res_current = m_res_current->ai_next) == nullptr ? false
                                                                   : true;
    }

    /*! \brief Get the socket address from current selcted address information
     * <!-- ----------------------------------------------------------- --> */
    void sockaddr( //
        /*! [out] Reference to a socket address structure that will be filled
         * with the address information. If no information is available (e.g.
         * CAddrinfo::get_first() wasn't called) an unspecified socket address
         * is returned (netaddr "", netaddrp ":0"). */
        SSockaddr& a_saObj) const noexcept {
        if (m_res == nullptr)
            a_saObj.clear();
        else if (m_res_current != nullptr)
            a_saObj =
                *reinterpret_cast<sockaddr_storage*>(m_res_current->ai_addr);
    }
    /// @} Getter

  private:
    // Cache the hints that are given with the constructor by the user.
    addrinfo m_hints{};

    // Pointer to the address information returned from systemcall
    // ::getaddrinfo(). This pointer must be freed. That is done with the
    // destructor.
    ::addrinfo* m_res{nullptr};

    // This points to the current used address info. It is modified with
    // get_first(), and get_next().
    ::addrinfo* m_res_current{nullptr};
};

} // namespace UPnPsdk

#endif // UPnPsdk_ADDRINFO_HPP
