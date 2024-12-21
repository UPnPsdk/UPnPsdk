#ifndef UPnPsdk_NETADAPTER_HPP
#define UPnPsdk_NETADAPTER_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-12-21
/*!
 * \file
 * \brief Manage information from different platforms about network adapters.
 *
 * This is a C++ Interface class to encapsulate different handling of local
 * network adapters on different platforms, for example Unix like platforms and
 * Microsoft Windows. They use complete different system calls to get
 * information about the local adapters like name, index, IP address etc.
 */

#include <UPnPsdk/sockaddr.hpp>
// \cond
#include <string>
// \endcond


namespace UPnPsdk {

/*!
 * \brief Manage information from different platforms about network adapters.
 * \ingroup upnplib-addrmodul
 * \code
 * // Usage (polymorphism) e.g.:
 * CNetadapter netadapterObj;          // Instantiate object
 * INetadapter& nadObj{netadapterObj}; // reference C++ interface
 * try {
 *     nadObj.get_first();
 * } catch(xcp) { handle_error(); };
 * UPnPsdk::SSockaddr saObj;
 * do {
 *     std::cout << "adapter name is " << nadObj.name() << '\n';
 *     nadObj.sockaddr(saObj);
 *     std::cout << "adapter address is " << saObj.netaddrp() << '\n';
 * } while (nadObj.get_next());
 * \endcode
 *
 * Used to get information from the local network adapters. Encapsulate it
 * in a class to get a common C++ interface to the program for different
 * platform realisations, for example on Unix like platforms and on Microsoft
 * Windows. They use different system calls.
 */
class INetadapter {
  public:
    /*! \brief Load a list of network adapters from the operating system and
     * select its first entry
     *
     * \exception std::runtime_error Failed to get information from the network
     * adapters: (detail)*/
    virtual void get_first() = 0;

    /*! \brief Select next entry from the network adapter list that was initial
     * loaded with INetadapter::get_first(). */
    virtual bool get_next() = 0;

    /*! \brief Get network adapter name from current selected list entry. */
    virtual std::string name() const = 0;

    // // I code IP Version-Independent, so this method is not provided.
    // sa_family_t in_family() const;

    /*! \brief Get socket address from current selected list entry. */
    virtual void sockaddr( //
        /*! [in,out] Reference to a socket address object that will be filled
         * with the socket address from the current selected network adapter
         * list entry. */
        SSockaddr& a_saddr) const = 0;

    /*! \brief Get socket address netmask from current selected list
     * entry.
     *
     * This netmask belongs to the adapters network address that is current
     * selected. */
    virtual void socknetmask( //
        /*! [in,out] Reference to a socket address object that will be filled
         * with the socket address netmask from the current selected network
         * adapter list entry. */
        SSockaddr& a_snetmask) const = 0;

    /*! \brief Get index number from current selected list entry. */
    virtual unsigned int index() const = 0;
};

} // namespace UPnPsdk

#endif // UPnPsdk_NETADAPTER_HPP
