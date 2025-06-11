#ifdef UPnPsdk_HAVE_OPENSSL
#ifndef UMOCK_SSL_MOCK_HPP
#define UMOCK_SSL_MOCK_HPP
// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-11

#include <umock/ssl.hpp>
#include <UPnPsdk/port.hpp>
#include <gmock/gmock.h>

namespace umock {

class UPnPsdk_VIS SslMock : public umock::SslInterface {
  public:
    SslMock();
    virtual ~SslMock() override;
    // clang-format off
    DISABLE_MSVC_WARN_4251
    MOCK_METHOD(int, SSL_read, (SSL* ssl, void* buf, int num), (override));
    MOCK_METHOD(int, SSL_write, (SSL* ssl, const void* buf, int num), (override));
    ENABLE_MSVC_WARN
    // clang-format on
};

} // namespace umock

#endif // UMOCK_SSL_MOCK_HPP
#endif // UPnPsdk_HAVE_OPENSSL
