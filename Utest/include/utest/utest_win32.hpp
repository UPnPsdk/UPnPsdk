#ifndef UTEST_TOOLS_WIN32_HPP
#define UTEST_TOOLS_WIN32_HPP
// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-10-08

#include <UPnPsdk/visibility.hpp> // for UPnPsdk_API
#include <winsock2.h>
#include <iphlpapi.h>             // must be after <winsock2.h>
#include <iostream>

namespace utest {

class UPnPsdk_API CNetIf4
// Object to manage and fill a network adapter structure. This is needed for
// mocked network interfaces. References:
// [GetAdaptersAddresses_function_(iphlpapi.h)]
// (https://docs.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getadaptersaddresses)
// [IP_ADAPTER_ADDRESSES_LH_structure_(iptypes.h)]
// (https://docs.microsoft.com/en-us/windows/win32/api/iptypes/ns-iptypes-ip_adapter_addresses_lh)
{
  public:
    CNetIf4();
    // With constructing the object you get a loopback device by default.
    // Properties are set to an ipv4 UP interface, supporting broadcast and
    // multicast.

    virtual ~CNetIf4() = default;

    ::PIP_ADAPTER_ADDRESSES get();
    // Return the pointer to a network interface structure.
    //
    // Exception: no-fail guarantee.

    void set(std::wstring_view a_Ifname, std::string_view a_Ifaddress);
    // Set the interface name and the ipv4 address with bitmask.
    // An empty a_Ifname will do nothing. An empty a_Ifaddress will set an
    // adapter structure with a zero ip address and bitmask. An ip address
    // without a bitmask will set a host ip address with bitmask '/32'.
    //
    // Exception: Strong guarantee (no modifications)
    //    throws: [std::logic_error] <- std::invalid_argument
    // A wrong ip address will throw an exception.

    void set_ifindex(::IF_INDEX a_IfIndex);
    // Sets the interface index as shown by the operating system tools.
    //
    // Exception: No-fail guarantee

    void chain_next(::PIP_ADAPTER_ADDRESSES a_ptrNextIf);
    // Sets the pointer to the next network interface structure in the interface
    // chain.
    //
    // Exception: No-fail guarantee

  private:
    // Structures needed to form the interface structure.
    // https://docs.microsoft.com/en-us/windows/win32/winsock/sockaddr-2
    UPnPsdk_LOCAL ::sockaddr_in m_inaddr{};
    // https://docs.microsoft.com/en-us/windows/win32/api/ws2def/ns-ws2def-socket_address
    UPnPsdk_LOCAL ::SOCKET_ADDRESS m_saddr{};
    // https://docs.microsoft.com/en-us/windows/win32/api/iptypes/ns-iptypes-ip_adapter_unicast_address_lh?redirectedfrom=MSDN
    UPnPsdk_LOCAL ::IP_ADAPTER_UNICAST_ADDRESS m_uniaddr{};
    // https://docs.microsoft.com/en-us/windows/win32/api/iptypes/ns-iptypes-ip_adapter_addresses_lh#see-also
    UPnPsdk_LOCAL ::IP_ADAPTER_ADDRESSES m_adapts{};

    // On the adapter (net interface) structure we only have pointer to strings
    // so we need to save them here to be sure we do not get dangling pointer.
    // Due to warning C4251: "'m_FriendlyName' needs to have dll-interface" I
    // don't use a STL class (std::string).
    // https://stackoverflow.com/q/5661738/5014688
    // FriendlyName is same as IfAlias[DisplayString] (RFC 2863). Cannot find
    // include file for DisplayString but may not exceed 255 character
    // (http://www.net-snmp.org/docs/mibs/ucdavis.html#DisplayString). --Ingo
#define DisplayString 255
    UPnPsdk_LOCAL WCHAR m_FriendlyName[DisplayString]{
        L"Loopback Pseudo-Interface 1"};
    UPnPsdk_LOCAL WCHAR m_Description[DisplayString]{
        L"Mocked Adapter for Unit testing"};
};

} // namespace utest

#endif // UTEST_TOOLS_WIN32_HPP
