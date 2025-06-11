#ifndef UPnPsdk_UPNP_DEVICE_HPP
#define UPnPsdk_UPNP_DEVICE_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-11
/*!
 * \file
 * \brief Manage UPnP devices
 */

#include <UPnPsdk/visibility.hpp>
#include <UPnPsdk/port.hpp>
/// \cond
#include <string>
/// \endcond


namespace UPnPsdk {

/*! \brief This will become a container object for the root device of a network
 * node */
class UPnPsdk_VIS CRootdevice {
  public:
    // Constructor
    CRootdevice();

    // Destructor
    virtual ~CRootdevice();

    /*! \brief Bind rootdevice to local network interfaces */
    void bind(const std::string& a_ifname);

    /// \brief Get interface name
    std::string ifname() const;

  private:
    SUPPRESS_MSVC_WARN_4251_NEXT_LINE
    std::string m_ifname;
};

} // namespace UPnPsdk

#endif // UPnPsdk_UPNP_DEVICE_HPP
