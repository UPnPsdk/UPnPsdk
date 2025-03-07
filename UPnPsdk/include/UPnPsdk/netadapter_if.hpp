#ifndef UPnPsdk_NETADAPTER_IF_HPP
#define UPnPsdk_NETADAPTER_IF_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-03-08
/*!
 * \file
 * \brief C++ interface to manage information from different platforms about
 * network adapters.
 *
 * This is a C++ Interface class to encapsulate different handling of local
 * network adapters on different platforms, for example Unix like platforms and
 * Microsoft Windows. They use complete different system calls to get
 * information about the local adapters like name, index, IP address etc.
 */

#include <UPnPsdk/sockaddr.hpp>


namespace UPnPsdk {

/*!
 * \brief Manage information from different platforms about network adapters.
 * \code
 * // Usage e.g.:
 * CNetadapter nadaptObj;
 * try {
 *     nadaptObj.get_first();
 * } catch(xcp) { handle_error(); };
 * SSockaddr saddrObj;
 * do {
 *     std::cout << "adapter name is " << nadaptObj.name() << '\n';
 *     nadaptObj.sockaddr(saddrObj);
 *     std::cout << "adapter address is " << saddrObj.netaddrp() << '\n';
 *     nadaptObj.socknetmask(saddrObj);
 *     std::cout << "with netmask " << saddrObj.netaddr() << '\n';
 * } while (nadaptObj.get_next());
 * \endcode
 * Used to get information from the local network adapters. Encapsulate it
 * in a class to get a common C++ interface to the program for different
 * platform realisations, for example on Unix like platforms and on Microsoft
 * Windows. They use different system calls.
 */
class UPnPsdk_API INetadapter {
  public:
    // Constructor
    INetadapter();

    // Destructor
    virtual ~INetadapter();

  private:
    /*! \brief Load a list of network adapters from the operating system and
     * select its first entry
     *
     * \exception std::runtime_error Failed to get information from the network
     * adapters: (detail information appended) */
    virtual void get_first() = 0;

    /*! \brief Select next entry from the network adapter list that was initial
     * loaded with get_first().
     * \returns
     *  - \b true if next adapter in the list exists
     *  - \b false otherwise */
    virtual bool get_next() = 0;

    /*! \brief Get index number from current selected list entry.
     *
     * This is the unique number of a network adapter as given by the operating
     * system. It is the best way to identify a network adapter. 0 means the
     * unspecified, unavailable adapter.
     * \returns
     *  - \b 0 if the local network adapter does not exist. This should only be
     *  possible if you haven't successful selected an adapter before.
     *  - otherwise index number of the selected adapter. */
    virtual unsigned int index() const = 0;

    /*! \brief Get network adapter name from current selected list entry.
     * \returns
     *  - \b "" (empty string) if the local network adapter does not exist.
     *  This should only be possible if you haven't successful selected an
     *  adapter before.
     *  - otherwise name of the local network adapter like "lo", "wlan0",
     *  "Ethernet". etc. */
    virtual std::string name() const = 0;

    // // I code IP Version-Independent, so this method is not provided.
    // sa_family_t in_family() const;

    /*! \brief Get socket address from current selected list entry. */
    virtual void sockaddr(
        /*! [in,out] Reference to a socket address object that will be filled
         * with the socket address from the current selected network adapter
         * list entry. */
        SSockaddr& a_saddr) const = 0;

    /*! \brief Get socket address netmask from current selected list
     * entry.
     *
     * This netmask belongs to the adapters network address that is current
     * selected. */
    virtual void socknetmask(
        /*! [in,out] Reference to a socket address object that will be filled
         * with the socket address netmask from the current selected network
         * adapter list entry. */
        SSockaddr& a_snetmask) const = 0;

    /*! \brief Get prefix length from the ip address of the current selected
     * local network adapter.
     * \returns Prefix length of the ip address from the current selected local
     * network adapter. */
    virtual unsigned int bitmask() const = 0;

    /*! \brief Reset pointer and point to the first entry of the local network
     * adapter list if available. */
    virtual void reset() noexcept = 0;

  protected:
    /// \cond
    // Index of the current found network adapter.
    unsigned int m_find_index{};
    /// \endcond
};

} // namespace UPnPsdk

#endif // UPnPsdk_NETADAPTER_IF_HPP
