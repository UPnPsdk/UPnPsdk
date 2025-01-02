// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-12-30
/*!
 * \file
 * \brief C++ interface to manage information from different platforms about
 * network adapters.
 */

#include <UPnPsdk/netadapter_if.hpp>
#include <UPnPsdk/addrinfo.hpp>
#include <UPnPsdk/synclog.hpp>

namespace UPnPsdk {

INetadapter::INetadapter(){
    TRACE2(this, " Constnuct INetadapter()") //
}

INetadapter::~INetadapter() {
    TRACE2(this, " Destruct INetadapter()")
}

bool INetadapter::find_first(const std::string& a_name_or_addr) {
    TRACE2(this, " Executing INetadapter::find_first(" + a_name_or_addr + ")")
    // First look for a local network adapter name
    this->reset();
    do {
        if (this->name() == a_name_or_addr)
            return true;
    } while (this->get_next());

    // No name found, look for the ip address of a local network adapter.
    // Try to translate input argument to a socket address.
    CAddrinfo ainfoObj(a_name_or_addr, AI_NUMERICHOST, 0);
    if (!ainfoObj.get_first())
        return false;

    // Valid ip address string given as input argument. Get its socket address.
    SSockaddr sa_inputObj{};
    ainfoObj.sockaddr(sa_inputObj);

    // Parse network adapter list for the given input argument.
    SSockaddr sa_nadObj{};
    this->reset();
    do {
        this->sockaddr(sa_nadObj);
        if (sa_nadObj == sa_inputObj.ss)
            return true;
    } while (this->get_next());

    return false;
}

bool INetadapter::find_first(unsigned int a_index) {
    TRACE2(this, " Executing INetadapter::find_first(" +
                     std::to_string(a_index) + ")")
    this->reset();
    do {
        if (this->index() == a_index)
            return true;
    } while (this->get_next());

    return false;
}

} // namespace UPnPsdk
