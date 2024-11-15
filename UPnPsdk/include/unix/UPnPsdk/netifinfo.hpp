#ifndef UPnPsdk_UNIX_NETIFINFO_HPP
#define UPnPsdk_UNIX_NETIFINFO_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-11-17
/*!
 * \file
 * \brief Manage information from the operating system about network interfaces.
 */

#include <UPnPsdk/port.hpp>
#include <UPnPsdk/sockaddr.hpp>

/// \cond
#include <ifaddrs.h>
#include <string>
/// \endcond


namespace UPnPsdk {

/*!
 * \brief Manage information from the operating system about network interfaces.
 * \ingroup upnplib-addrmodul
 * \code
 * // Usage e.g.:
 * CIfaddrs ifaObj;
 * try {
 *     ifaObj.load();
 * } catch(xcp) { handle_error(); };
 * SSockaddr saddrObj;
 * do {
 *     std::cout << "interface name is " << ifaObj.name() << '\n';
 *     saddrObj = ifaObj.sockaddr();
 *     std::cout << "interface address is " << saddrObj.netaddr() << '\n';
 * } while (ifaObj.get_next());
 * \endcode
 *
 * Used to get information about the local network interfaces. Encapsulate it
 * in a class to get a common C++ interface to the program for different
 * platform realisations, for example on Unix like platforms and on Microsoft
 * Windows. They use different system calls.
 */
class CIfaddrs {
  public:
    // Constructor
    CIfaddrs();

    // Destructor
    virtual ~CIfaddrs();

    /*! \name Setter
     * *************
     * @{ */
    /*!
     * \brief Load object with local network interface information from
     * operating system
     *
     * Example see Detailed Description at CIfaddrs.
     * \exception std::runtime_error Failed to get information from the network
     * interfaces: (detail)*/
    void load();
    /// @} Setter

    /*! \name Getter
     * *************
     * @{ */
    /*! \brief Get next entry from loaded network interface list.
     *
     * Example see Detailed Description at CIfaddrs. */
    bool get_next();

    /*! \brief Get network interface name from current selected list entry.
     *
     * Example see Detailed Description at CIfaddrs. */
    std::string name() const;

    // I code IP Version-Independent, so this method is not provided.
    // sa_family_t in_family() const;

    /*! \brief Get socket address Object from current selected list entry.
     *
     * Example see Detailed Description at CIfaddrs. */
    SSockaddr sockaddr() const;

    /*! \brief Get socket address netmask Object from current selected list
     * entry.
     *
     * This netmask belongs to the network address that is current selected
     * with load() and get_next(). Example see Detailed Description at
     * CIfaddrs. */
    SSockaddr socknetmask() const;
    /// @} Getter

  private:
    ifaddrs* m_ifa_first{nullptr};
    ifaddrs* m_ifa_current{nullptr};

    void free_ifaddrs() noexcept;
    bool is_valid_if() const noexcept;
};

} // namespace UPnPsdk

#endif // UPnPsdk_UNIX_NETIFINFO_HPP
