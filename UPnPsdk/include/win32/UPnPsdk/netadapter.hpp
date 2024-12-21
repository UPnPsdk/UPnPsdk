#ifndef UPnPsdk_WIN32_NETIFINFO_HPP
#define UPnPsdk_WIN32_NETIFINFO_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-12-21
/*!
 * \file
 * \brief Manage information from Microsoft Windows about network adapters.
 */

#include <UPnPsdk/netadapter_if.hpp>


namespace UPnPsdk {

/*!
 * \brief Manage information from Microsoft Windows about network adapters.
 *
 * For details look at UPnPsdk::INetadapter
 */
class UPnPsdk_API CNetadapter : public INetadapter {
    /// \cond
  public:
    // Constructor
    CNetadapter();

    // Destructor
    virtual ~CNetadapter();

    // Copy constructor
    // We cannot use the default copy constructor because there is also
    // allocated memory for the ifaddrs structure to copy. We get segfaults
    // and program aborts. This class is not usable to copy the object.
    CNetadapter(const CNetadapter&) = delete;

    // Copy assignment operator
    // Same as with the copy constructor.
    CNetadapter& operator=(CNetadapter) = delete;

    void get_first() override;
    bool get_next() override;
    std::string name() const override;
    void sockaddr(SSockaddr& a_saddr) const override;
    void socknetmask(SSockaddr& a_snetmask) const override;
    unsigned int index() const override;
    /// \endcond

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
};

} // namespace UPnPsdk

#endif // UPnPsdk_WIN32_NETIFINFO_HPP
