#ifndef UPnPsdk_NETADAPTER_HPP
#define UPnPsdk_NETADAPTER_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-02-19
/*!
 * \file
 * \brief Manage information about network adapters.
 */

#include <UPnPsdk/netadapter_platform.hpp>

namespace UPnPsdk {

/*!
 * \brief Get prefix bit number from a network address mask.
 * \ingroup upnplib-addrmodul
 *
 * The length, in bits, of the prefix or network part of the IP address, e.g.
 * 64 from "[2001:db8::1]/64". For a unicast IPv4 address, any value greater
 * than 32 is an illegal value. For a unicast IPv6 address, any value greater
 * than 128 is an illegal value. A value of 255 is commonly used to represent
 * an illegal value.
 * \returns
 *  - On success: The length, in bits, of the prefix or network part of the IP
 * address.
 *  _ On error: \b 255
 */
uint8_t netmask_to_bitmask(
    // [in] Pointer to a socket address structure containing the netmask.
    const ::sockaddr_storage* a_netmask);


/*!
 * \brief Get network address mask from address prefix bit number.
 * \ingroup upnplib-addrmodul
 * \code
// Usage e.g.:
SSockaddr saObj;
bitmask_to_netmask(AF_INET6, 64, saObj);
std::cout << "netmask is " << saObj.netaddr() << '\n';
 * \endcode
 * \exception std::invalid_argument An invalid ip-address prefix bitmask >32
 * (IPv4) or >128 (IPv6) was used.
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
class UPnPsdk_API CNetadapter : public CNetadapter_platform {
  public:
    // Constructor
    CNetadapter();

    // Destructor
    virtual ~CNetadapter();

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
};

} // namespace UPnPsdk

#endif // UPnPsdk_NETADAPTER_HPP
