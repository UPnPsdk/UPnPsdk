#ifndef UPnPsdk_NETADAPTER_HPP
#define UPnPsdk_NETADAPTER_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-12-29
/*!
 * \file
 * \brief Manage information about network adapters.
 */

#include <UPnPsdk/netadapter_platform.hpp>


namespace UPnPsdk {

/*!
 * \brief Get information from local network adapters
 * \ingroup upnplib-addrmodul
 * \code
 * // Usage e.g:
 * CNetadapter nadaptObj;
 * try {
 *     nadaptObj.get_first();
 * } catch (xcp) { handle_error() }
 * if (nadaptObj.find_first(1))
 *     std::cout << "loopback interface name is " << nadaptObj.name() << '\n';
 *
 * if (nadaptObj.find_first("eth0")) {
 *     SSockaddr saddrObj;
 *     nadaptObj.sockaddr(saddrObj);
 *     std::cout << "first ip address on eth0 is " << saddrObj.netaddrp << '\n';
 * }
 * \endcode
 * The operating system manages an internal list of the local network adapters.
 * With this class you can get information about them. <i>"Typically, nodes, not
 * applications, automatically solve the source address selection. A node will
 * choose the source address for a communication following some rules of best
 * choice, per [RFC3484]."</i> (REF: <a
 * href=https://datatracker.ietf.org/doc/html/rfc4038#section-5.4.1>RFC4038 -
 * IP Address Selection</a>).
 */
class CNetadapter : public CNetadapter_platform {
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
