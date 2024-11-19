#ifndef UPnPsdk_WIN32_NETIFINFO_HPP
#define UPnPsdk_WIN32_NETIFINFO_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-11-19
/*!
 * \file
 * \brief Manage information from Microsoft Windows about network adapters.
 */

#include <UPnPsdk/netadapter_if.hpp>


namespace UPnPsdk {

/*!
 * \brief Manage information from Microsoft Windows about network adapters.
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
};

} // namespace UPnPsdk

#endif // UPnPsdk_WIN32_NETIFINFO_HPP
