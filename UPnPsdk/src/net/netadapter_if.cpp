// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-03-24
/*!
 * \file
 * \brief C++ interface to manage information from different platforms about
 * network adapters.
 */

#include <UPnPsdk/netadapter_if.hpp>
#include <UPnPsdk/synclog.hpp>

namespace UPnPsdk {

INetadapter::INetadapter(){
    TRACE2(this, " Construct INetadapter()") //
}

INetadapter::~INetadapter() {
    TRACE2(this, " Destruct INetadapter()")
}

} // namespace UPnPsdk
