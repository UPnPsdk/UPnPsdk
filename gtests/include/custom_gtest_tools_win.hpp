// Copyright (C) 2021 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2021-11-27

#ifndef UPNP_IFADDRS_WIN_HPP
#define UPNP_IFADDRS_WIN_HPP

#include "port.hpp"
#include <winsock2.h>
#include <iphlpapi.h>
#include <iostream>
#include <vector>

namespace upnp {

class UPNP_API CIfaddr4
// Tool to manage and fill a network adapter structure. This is needed for
// mocked network interfaces. References: [GetAdaptersAddresses function
// (iphlpapi.h)]
// (https://docs.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getadaptersaddresses)
// [IP_ADAPTER_ADDRESSES_LH structure (iptypes.h)]
// (https://docs.microsoft.com/en-us/windows/win32/api/iptypes/ns-iptypes-ip_adapter_addresses_lh)
{
  public:
    CIfaddr4();
    // With constructing the object you get a loopback device by default.

    virtual ~CIfaddr4();

    ::PIP_ADAPTER_ADDRESSES get() const;
    // Return the pointer to the ifaddr structure.
    // Exception no-fail guarantee.

    void set(std::wstring a_Ifname, std::string_view a_Ifaddress);
    // Set the interface name and the ipv4 address with bitmask. Properties are
    // set to an ipv4 UP interface, supporting broadcast and multicast.
    // An empty a_Ifname or a wrong a_Ifaddress will do nothing.
    // An empty a_Ifaddress will set an adapter structure with a zero ip
    // address and bitmask. If there are wrong arguments the adapter structure
    // will not be modified.
    //
    // Exception: no-fail guarantee

    void set_ifindex(::IF_INDEX a_IfIndex);
    void chain_next_addr(::PIP_ADAPTER_ADDRESSES a_ptrNextAddr);

  private:
    // https://docs.microsoft.com/en-us/windows/win32/winsock/sockaddr-2
    ::sockaddr_in m_inaddr{};
    // https://docs.microsoft.com/en-us/windows/win32/api/ws2def/ns-ws2def-socket_address
    ::SOCKET_ADDRESS m_saddr{};
    // https://docs.microsoft.com/en-us/windows/win32/api/iptypes/ns-iptypes-ip_adapter_unicast_address_lh?redirectedfrom=MSDN
    ::IP_ADAPTER_UNICAST_ADDRESS m_uniaddr{};
    // https://docs.microsoft.com/en-us/windows/win32/api/iptypes/ns-iptypes-ip_adapter_addresses_lh#see-also
    ::IP_ADAPTER_ADDRESSES m_adapts{};

    // On the adapter structure we only have pointer to strings so we need to
    // save them here to be sure we do not get dangling pointer.
    std::wstring m_Description{L"Mocked Adapter for Unit testing"};
    std::wstring m_FriendlyName{L"Loopback Pseudo-Interface 1"};
};

//
class UPNP_API CIfaddr4Container
// This is a Container for multiple network interface structures that are
// chained in m_adapts.Next as given by the low level structure
// IP_ADAPTER_ADDRESSES. If setting a loopback interface with a_Ifaddress
// "127.0.0.1/8" then a_Ifname is ignored and always set to its default name
// (e.g. "lo" on Unix or L"Loopback Pseudo-Interface 1" on MS Windows).
//
// It is IMPORTANT to know that the m_adapts.Next address pointer chain changes
// when adding an interface address object. You MUST get_ifaddr(..) the new
// address pointer for ongoing work.
//
// Exception:
{
  public:
    void add(std::wstring a_Ifname, std::string_view a_Ifaddress);
    PIP_ADAPTER_ADDRESSES get_ifaddr(long unsigned int a_idx) const;

  private:
    std::vector<CIfaddr4> m_ifaddr4Container{};
};

} // namespace upnp

#endif // UPNP_IFADDRS_WIN_HPP
