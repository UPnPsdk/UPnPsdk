#ifndef UPnPsdk_UNIX_NETIFINFO_HPP
#define UPnPsdk_UNIX_NETIFINFO_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-11-19
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
 *
 * For details look at INetadapter
 */
class UPnPsdk_API CNetadapter : public INetadapter {
  public:
    // Constructor
    CNetadapter();

    // Destructor
    virtual ~CNetadapter();

    // Copy constructor
    // We cannot use the default copy constructor because there is also
    // allocated memory for the ifaddrs structure to copy. We get segfaults
    // and program aborts. This class is not used to copy the object.
    CNetadapter(const CNetadapter&) = delete;

    // Copy assignment operator
    // Same as with the copy constructor.
    CNetadapter& operator=(CNetadapter) = delete;

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
