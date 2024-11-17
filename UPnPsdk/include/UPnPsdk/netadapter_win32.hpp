#ifndef UPnPsdk_WIN32_NETIFINFO_HPP
#define UPnPsdk_WIN32_NETIFINFO_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-11-18
/*!
 * \file
 * \brief Manage information from Microsoft Windows about network adapters.
 */

#include <UPnPsdk/netadapter.hpp>


namespace UPnPsdk {

/*!
 * \brief Manage information from Microsoft Windows about network adapters.
 *
 * For details look at INetadapter
 */
class CNetadapter : public INetadapter {
  public:
    // Constructor
    CNetadapter();

    // Destructor
    virtual ~CNetadapter();

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
