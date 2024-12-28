// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-12-23
/*!
 * \file
 * \brief Manage UPnP devices
 */

#include <UPnPsdk/upnp_device.hpp>
#include <UPnPsdk/synclog.hpp>
#include <UPnPsdk/netadapter.hpp>


namespace UPnPsdk {

// Constructor
CRootdevice::CRootdevice(){
    TRACE2(this, " Construct CRootdevice()") //
}

// Destructor
CRootdevice::~CRootdevice() {
    TRACE2(this, " Destruct CRootdevice()") //
}

// Bind rootdevice to local network interfaces
void CRootdevice::bind(const std::string& a_ifname) {
    TRACE2(this, " Executing CRootdevice::bind()")

    CNetadapter nadaptObj; // Instantiate object
    nadaptObj.get_first(); // May throw exception std::runtime_error

    enum struct Found { none, name, index, addr } iface_found{};
    do {
        if (nadaptObj.name() == a_ifname) {
            iface_found = Found::name;
            break;
        }
    } while (nadaptObj.get_next());

    switch (iface_found) {
    case Found::none:
        break;
    case Found::name:
        m_ifname = a_ifname;
        break;
    case Found::index:
        break;
    case Found::addr:
        break;
    }
}

// Get interface name
std::string CRootdevice::ifname() const {
    TRACE2(this, " Executing CRootdevice::ifname()")
    return m_ifname;
}

} // namespace UPnPsdk
