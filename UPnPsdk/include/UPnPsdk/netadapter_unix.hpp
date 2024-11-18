#ifndef UPnPsdk_UNIX_NETIFINFO_HPP
#define UPnPsdk_UNIX_NETIFINFO_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-11-18
/*!
 * \file
 * \brief Manage information from Unix like platforms about network adapters.
 */

#include <UPnPsdk/netadapter.hpp>
/// \cond
#include <ifaddrs.h>
/// \endcond


namespace UPnPsdk {

/*!
 * \brief Manage information from Unix like platforms about network adapters.
 *
 * For details look at INetadapter
 */
class CNetadapter : public INetadapter {
  public:
    // Constructor
    CNetadapter();

    // Destructor
    virtual ~CNetadapter() override;

    /*! \name Setter
     * *************
     * @{ */
    void load() override;
    /// @} Setter

    /*! \name Getter
     * *************
     * @{ */
    bool get_next() override;
    std::string name() const override;
    SSockaddr sockaddr() const override;
    SSockaddr socknetmask() const override;
    /// @} Getter

  private:
    ifaddrs* m_ifa_first{nullptr};
    ifaddrs* m_ifa_current{nullptr};

    void free_ifaddrs() noexcept;
    bool is_valid_if() const noexcept;
};

} // namespace UPnPsdk

#endif // UPnPsdk_UNIX_NETIFINFO_HPP
