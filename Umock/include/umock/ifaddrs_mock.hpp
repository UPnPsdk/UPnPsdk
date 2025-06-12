#ifndef UMOCK_IFADDRS_MOCK_HPP
#define UMOCK_IFADDRS_MOCK_HPP
// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-11

#include <umock/ifaddrs.hpp>
#include <gmock/gmock.h>

namespace umock {

class UPnPsdk_VIS IfaddrsMock : public IfaddrsInterface {
  public:
    IfaddrsMock();
    virtual ~IfaddrsMock() override;
    MOCK_METHOD(int, getifaddrs, (struct ifaddrs**), (override));
    MOCK_METHOD(void, freeifaddrs, (struct ifaddrs*), (override));
};

} // namespace umock

#endif // UMOCK_IFADDRS_MOCK_HPP
