#ifndef UPnPsdk_NETADAPTER_HPP
#define UPnPsdk_NETADAPTER_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-11-18
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
 * CNetadapter netadapterObj; // Instantiate object
 * INetadapter& nadObj{netadapterObj}; // reference C++ interface
 * try {
 *     nadObj.load();
 * } catch(xcp) { handle_error(); };
 * SSockaddr saddrObj;
 * do {
 *     std::cout << "adapter name is " << nadObj.name() << '\n';
 *     saddrObj = nadObj.sockaddr();
 *     std::cout << "adapter address is " << saddrObj.netaddr() << '\n';
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
    /*! \name Setter
     * *************
     * @{ */
    /*!
     * \brief Load object with local network adapter information from operating
     * system
     *
     * \exception std::runtime_error Failed to get information from the network
     * adapters: (detail)*/
    virtual void load() = 0;
    /// @} Setter

    /*! \name Getter
     * *************
     * @{ */
    /*! \brief Get next entry from loaded network adapter list. */
    virtual bool get_next() = 0;

    /*! \brief Get network adapter name from current selected list entry. */
    virtual std::string name() const = 0;

    // // I code IP Version-Independent, so this method is not provided.
    // sa_family_t in_family() const;

    /*! \brief Get socket address Object from current selected list entry. */
    virtual SSockaddr sockaddr() const = 0;

    /*! \brief Get socket address netmask Object from current selected list
     * entry.
     *
     * This netmask belongs to the adapters network address that is current
     * selected with load() and get_next(). */
    virtual SSockaddr socknetmask() const = 0;
    /// @} Getter
};

} // namespace UPnPsdk

#endif // UPnPsdk_NETADAPTER_HPP
