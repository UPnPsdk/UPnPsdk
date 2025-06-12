#ifndef UMOCK_STDLIB_MOCK_HPP
#define UMOCK_STDLIB_MOCK_HPP
// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-11

#include <umock/stdlib.hpp>
#include <UPnPsdk/port.hpp>
#include <gmock/gmock.h>

namespace umock {

class UPnPsdk_VIS StdlibMock : public umock::StdlibInterface {
  public:
    StdlibMock();
    virtual ~StdlibMock() override;
    DISABLE_MSVC_WARN_4251
    MOCK_METHOD(void*, malloc, (size_t size), (override));
    MOCK_METHOD(void*, calloc, (size_t nmemb, size_t size), (override));
    MOCK_METHOD(void*, realloc, (void* ptr, size_t size), (override));
    MOCK_METHOD(void, free, (void* ptr), (override));
    ENABLE_MSVC_WARN
};

} // namespace umock

#endif // UMOCK_STDLIB_MOCK_HPP
