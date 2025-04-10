#ifndef UPnPsdk_UNIX_NETADAPTER_HPP
#define UPnPsdk_UNIX_NETADAPTER_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-02-24
/*!
 * \file
 * \brief Manage information from Unix like platforms about network adapters.
 */

#include <UPnPsdk/netadapter_if.hpp>
/// \cond
#include <ifaddrs.h>
/// \endcond


namespace UPnPsdk {

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
    void socknetmask(SSockaddr& a_snetmask) const override;
    unsigned int bitmask() const override;

  private:
    // Pointer to the first network adapter structure. This pointer must not be
    // modified because it is needed to free the allocated memory space for the
    // adapter list.
    ifaddrs* m_ifa_first{nullptr};

    // Pointer to the current network adapter in work.
    ifaddrs* m_ifa_current{nullptr};

    // Methodes
    inline bool is_valid_if(const ifaddrs* a_ifa) const noexcept;
    void free_ifaddrs() noexcept;

  protected:
    /*! \brief Reset pointer and point to the first entry of the local network
     * adapter list if available. */
    void reset() noexcept override;
};

} // namespace UPnPsdk

#endif // UPnPsdk_UNIX_NETADAPTER_HPP
