#ifndef UPnPsdk_WIN32_NETADAPTER_HPP
#define UPnPsdk_WIN32_NETADAPTER_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-03-31
/*!
 * \file
 * \brief Manage information from Microsoft Windows about network adapters.
 */

#include <UPnPsdk/netadapter_if.hpp>


namespace UPnPsdk {

/*!
 * \brief Manage information from Microsoft Windows about network adapters.
 */
class UPnPsdk_API CNetadapter_platform : public INetadapter {
  public:
    // Constructor
    CNetadapter_platform();

    // Destructor
    virtual ~CNetadapter_platform();

    // methodes
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
    ::PIP_ADAPTER_ADDRESSES m_adapt_first{nullptr};

    // Pointer to the current network adapter in work. It will be set to Next
    // when its IP addresses are parsed.
    ::PIP_ADAPTER_ADDRESSES m_adapt_current{nullptr};

    // Pointer to the current IP address in work of a network adapter. It will
    // be set to Next by parsing the address list.
    ::PIP_ADAPTER_UNICAST_ADDRESS_LH m_unicastaddr_current{nullptr};

    void free_adaptaddrs() noexcept;

    /*! \brief Reset pointer and point to the first entry of the local network
     * adapter list if available. */
    inline void reset() noexcept;
};

} // namespace UPnPsdk

#endif // UPnPsdk_WIN32_NETADAPTER_HPP
