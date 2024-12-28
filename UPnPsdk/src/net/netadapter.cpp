// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-12-29
/*!
 * \file
 * \brief Manage information about network adapters.
 */

#include <UPnPsdk/netadapter.hpp>
#include <UPnPsdk/synclog.hpp>


namespace UPnPsdk {

CNetadapter::CNetadapter(){
    TRACE2(this, " Constnuct CNetadapter()") //
}

CNetadapter::~CNetadapter() {
    TRACE2(this, " Destruct CNetadapter()")
}

} // namespace UPnPsdk
