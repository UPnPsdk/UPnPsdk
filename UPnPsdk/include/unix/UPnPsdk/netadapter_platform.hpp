#ifndef UPnPsdk_UNIX_NETADAPTER_HPP
#define UPnPsdk_UNIX_NETADAPTER_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-10-28
/*!
 * \file
 * \brief Manage information from Unix like platforms about network adapters.
 */

#include <UPnPsdk/netadapter_if.hpp>
/// \cond
#include <ifaddrs.h>
#include <net/if.h>
/// \endcond


namespace UPnPsdk {

/*!
 * \brief Test if a local network interface is valid and usable
 *
 * Accept IFF_LOOPBACK or up AF_INET6/AF_INET interfaces with address (e.g. not
 * bonded) and that support always MULTICAST and additional BROADCAST for
 * AF_INET.
 */
inline bool is_valid_netif(const ifaddrs* a_ifa) {
    if (a_ifa == nullptr || a_ifa->ifa_addr == nullptr ||
        (a_ifa->ifa_addr->sa_family != AF_INET6 &&
         a_ifa->ifa_addr->sa_family != AF_INET))
        return false;

    if ((a_ifa->ifa_flags & IFF_LOOPBACK))
        return true;

    if ((a_ifa->ifa_flags & IFF_UP) && (a_ifa->ifa_flags & IFF_MULTICAST)) {
        if (a_ifa->ifa_addr->sa_family == AF_INET &&
            !(a_ifa->ifa_flags & IFF_BROADCAST))
            return false;
        return true;
    }

    return false;
}


/*!
 * \brief Manage information from Unix like platforms about network adapters.
 */
class UPnPsdk_API CNetadapter_platform : public INetadapter {
  public:
    // Constructor
    CNetadapter_platform();

    // Destructor
    virtual ~CNetadapter_platform();

    // Methodes
    void get_first() override;
    bool get_next() override;
    unsigned int index() const override;
    std::string name() const override;
    void sockaddr(SSockaddr& a_saddr) const override;
    unsigned int bitmask() const override;

  private:
    // Pointer to the first network adapter structure. This pointer must not be
    // modified because it is needed to free the allocated memory space for the
    // adapter list.
    ifaddrs* m_ifa_first{nullptr};

    // Pointer to the current network adapter in work.
    ifaddrs* m_ifa_current{nullptr};

    // Methodes
    void free_ifaddrs() noexcept;

  protected:
    /*! \brief Reset pointer and point to the first entry of the local network
     * adapter list if available. */
    void reset() noexcept override;
};

} // namespace UPnPsdk

#endif // UPnPsdk_UNIX_NETADAPTER_HPP
