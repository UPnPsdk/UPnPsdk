#ifndef UPnPsdk_NETADAPTER_HPP
#define UPnPsdk_NETADAPTER_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-03-16
/*!
 * \file
 * \brief Manage information about network adapters.
 */

#include <UPnPsdk/netadapter_platform.hpp>

namespace UPnPsdk {

/*!
 * \brief Get prefix bit number from a network address mask.
 * \ingroup upnplib-addrmodul
 * \code
// Usage e.g.:
::sockaddr_storage saddr{};
try {
    std::cout << "bitmask is " << netmask_to_bitmask(&saddr) << '\n';
} catch (const std::runtime_error& ex) { handle_error() }
 * \endcode
 * Returns the length, in bits, of the prefix or network part of the IP
 * address, e.g. 64 from "[2001:db8::1]/64". A value of 255 is commonly used to
 * represent an illegal value but this function throws an exception instead.
 *
 * \returns The length, in bits, of the prefix or network part of the IP
 * address, 0..32 for IPv4, 0..128 for IPv6.
 * \exception std::runtime_error
 *  - with an invalid netmask
 *  - with an invalid \glos{af,address family}.
 */
uint8_t netmask_to_bitmask(
    /// [in] Pointer to a socket address structure containing the netmask.
    const ::sockaddr_storage* a_netmask);


/*!
 * \brief Get network address mask from address prefix bit number.
 * \ingroup upnplib-addrmodul
 * \code
// Usage e.g.:
::sockaddr_storage saddr{};
SSockaddr saObj;
try {
    bitmask_to_netmask(&saddr, 64, saObj);
} catch (const std::runtime_error& ex) { handle_error() }
std::cout << "netmask is " << saObj.netaddr() << '\n';
 * \endcode
 * \exception std::runtime_error
 *  - if the associated socket address **a_saddr** is not given.
 *  - if the prefix length exceeds its maximum size (128 for IPv6, 32 for IPv4),
 *  - with an invalid \glos{af,address family}.
 */
void bitmask_to_netmask(
    /*! [in] Pointer to a structure containing the socket address the netmask
     * is associated. */
    const ::sockaddr_storage* a_saddr,
    /*! [in] IPv6 or IPv4 address prefix length as number of set bits as given
     * e.g. with 64 in [2001:db8::1]/64. */
    const uint8_t a_prefixlength,
    /*! [out] Reference to a socket address object that will be filled with the
     * netmask. */
    SSockaddr& a_saddrObj);


/*!
 * \brief Get information from local network adapters
 * \ingroup upnplib-addrmodul
 * \code
// Example to get a link local address from a network adapter:
SSockaddr saObj;
CNetadapter nadaptObj;
try {
    nadaptObj.get_first();
    do {
        nadaptObj.sockaddr(saObj);
        if (saObj.ss.ss_family == AF_INET6 &&
            IN6_IS_ADDR_LINKLOCAL(&saObj.sin6.sin6_addr))
            break;
    } while (nadaptObj.get_next());
} catch (const std::exception& ex) { handle_error() }

if (saObj.ss.ss_family != AF_INET6 ||
    !IN6_IS_ADDR_LINKLOCAL(&saObj.sin6.sin6_addr))
    std::cout << "No local network adapter with link local address found.\n";
else
    std::cout << "Adapter " << nadaptObj.name() << " has lla " << saObj << '\n';
 * \endcode
 * The operating system manages an internal list of the local network adapters.
 * With this class you can get information about them. <i>"Typically, nodes, not
 * applications, automatically solve the source address selection. A node will
 * choose the source address for a communication following some rules of best
 * choice, per [RFC3484]."</i> (REF: <a
 * href=https://datatracker.ietf.org/doc/html/rfc4038#section-5.4.1>RFC4038 -
 * IP Address Selection</a>).
 */
class CNetadapter {
    // Due to warning C4251 "'type' : class 'type1' needs to have dll-interface
    // to be used by clients of class 'type2'" on Microsoft Windows each member
    // function needs to be decorated with UPnPsdk_API instead of just only the
    // class. The reason is 'm_na_platformPtr'.
  public:
    /*! \brief Constructor */
    UPnPsdk_API CNetadapter(
        /*! [in] Inject the used \glos{depinj,di-service} object that is by
         * default the productive one but may also be a mocked object for Unit
         * Tests. */
        PNetadapter_platform a_na_platformPtr =
            std::make_shared<CNetadapter_platform>());

    /* \brief Destructor */
    UPnPsdk_API virtual ~CNetadapter();

    /// \cond
    // Copy constructor
    // We cannot use the default copy constructor because there is also
    // allocated memory for the ifaddrs structure to copy. We get segfaults
    // and program aborts. This class is not usable to copy the object.
    CNetadapter(const CNetadapter&) = delete;

    // Copy assignment operator
    // Same as with the copy constructor.
    CNetadapter& operator=(CNetadapter) = delete;
    /// \endcond

    // methods from injected object
    /// \copydoc INetadapter::get_first()
    UPnPsdk_API void get_first();
    /// \copydoc INetadapter::get_next()
    UPnPsdk_API bool get_next();
    /// \copydoc INetadapter::index()
    UPnPsdk_API unsigned int index() const;
    /// \copydoc INetadapter::name()
    UPnPsdk_API std::string name() const;
    /// \copydoc INetadapter::sockaddr()
    UPnPsdk_API void sockaddr(SSockaddr& a_saddr) const;
    /// \copydoc INetadapter::socknetmask()
    UPnPsdk_API void socknetmask(SSockaddr& a_snetmask) const;
    /// \copydoc INetadapter::bitmask()
    UPnPsdk_API unsigned int bitmask() const;
    /// \copydoc INetadapter::reset()
    UPnPsdk_API void reset() noexcept;

    // Own methods
    /*! \brief Find local network adapter with given name or ip address
     * \code
     * // Usage e.g.:
     * CNetadapter nadaptObj;
     * try {
     *     nadaptObj.get_first();
     * } catch(xcp) { handle_error(); };
     * if (nadaptObj.find_first()) {
     *     SSockaddr saObj;
     *     nadaptObj.sockaddr(saObj);
     *     std::cout << "used local address is " << saObj << '\n';
     *  }
     * if (nadaptObj.find_first("[2001.db8::1:0:2]"))
     *     std::cout << "adapter name is " << nadaptObj.name() << '\n';
     * \endcode
     *
     * You have to get_first() entry from the internal network adapter list to
     * load it. Then you can try to \b %find_first() an adapter. With no
     * argument the operating system presents one as best choise. Due to
     * <!--REF:-->_<a
     * href="https://datatracker.ietf.org/doc/html/rfc4038#section-5.4.1">RFC
     * 4038, 5.4.1 - IP_Address Selection</a>, specifying the source address is
     * not typically required.
     *
     * If found, the adapter is selected so that all its properties can be
     * retrieved. If the adapter has more than one ip address you can get them
     * with find_next().
     * \returns
     *  - \b true if adapter with given name or ip address was found
     *  - \b false otherwise */
    UPnPsdk_API bool find_first(
        /*! [in]
         * - no argument will select a local ip address by the operating system
         * - "loopback" (all lower case) will select the first loopback address
         * - local network interface name (like "lo", "eth0", "Ethernet", etc.)
         * - an ip address. */
        std::string_view a_name_or_addr);

    /*! \brief Find local network adapter with given index number.
     * \details Example at find_first(const std::string&). Of course you have to
     * use an index number.
     *
     * You have to get_first() entry of the internal network adapter
     * list to load it. Then you can try to \b %find_first() adapter with the
     * given property. If found, the adapter is selected so that all its
     * properties can be retrieved.
     * \returns
     *  - \b true if adapter with given index number was found
     *  - \b false otherwise */
    UPnPsdk_API bool find_first(
        /*! [in] Index number of the local network adapter. */
        const unsigned int a_index);

    /*! \brief Find next ip address from a selected network adapter
     * \details Before using this method you have to use get_first() to load
     * the internal network adapter list from the operating system and then use
     * find_first() to select a network adapter. If the adapter has more than
     * one item (ip address) you can select them with this method.
     * \returns
     *  - \b true if the next item on the selected adatper was found
     *  - \b false otherwise, or if you haven't used get_first() and
     * find_first() */
    UPnPsdk_API bool find_next();

  private:
    /// \cond
    // Injected smart pointer to the netadapter object of the current operating
    // system. It may also point to a mocking object.
    PNetadapter_platform m_na_platformPtr;

    // Index of the current found network adapter.
    unsigned int m_find_index{};
    /// \endcond
};

} // namespace UPnPsdk

#endif // UPnPsdk_NETADAPTER_HPP
