// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-11-19
/*!
 * \file
 * \brief Manage information from Microsoft Windows about network adapters.
 */

#include <UPnPsdk/netadapter.hpp>
#include <UPnPsdk/synclog.hpp>


namespace UPnPsdk {

// clang-format off
CNetadapter::CNetadapter() {
    TRACE2(this, " Construct CGetAdaptersAddr()")
}

CNetadapter::~CNetadapter() {
    TRACE2(this, " Destruct CGetAdaptersAddr()")
}
// clang-format on

void CNetadapter::load() {}

bool CNetadapter::get_next() { return false; }

std::string CNetadapter::name() const { return ""; }

SSockaddr CNetadapter::sockaddr() const {
    SSockaddr saddrObj;
    return saddrObj;
}

SSockaddr CNetadapter::socknetmask() const {
    SSockaddr saddrObj;
    return saddrObj;
}

} // namespace UPnPsdk
