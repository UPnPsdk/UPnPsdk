#ifndef UMOCK_NET_IF_MOCK_HPP
#define UMOCK_NET_IF_MOCK_HPP
// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-12-23

#include <umock/net_if.hpp>
#include <gmock/gmock.h>

namespace umock {

class UPNPLIB_API Net_ifMock : public umock::Net_ifInterface {
  public:
    Net_ifMock();
    virtual ~Net_ifMock() override;
    MOCK_METHOD(unsigned int, if_nametoindex, (const char* ifname), (override));
};

} // namespace umock

#endif // UMOCK_NET_IF_MOCK_HPP
