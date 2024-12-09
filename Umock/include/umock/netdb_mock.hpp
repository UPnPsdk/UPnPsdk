#ifndef UMOCK_NETDB_MOCK_HPP
#define UMOCK_NETDB_MOCK_HPP
// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-12-11

#include <umock/netdb.hpp>
#include <UPnPsdk/port.hpp>
#include <gmock/gmock.h>

namespace umock {

class UPNPLIB_API NetdbMock : public NetdbInterface {
  public:
    NetdbMock();
    virtual ~NetdbMock() override;
    DISABLE_MSVC_WARN_4251
    MOCK_METHOD(int, getaddrinfo,
                (const char* node, const char* service,
                 const struct addrinfo* hints, struct addrinfo** res),
                (override));
    MOCK_METHOD(void, freeaddrinfo, (struct addrinfo * res), (override));
#ifndef _MSC_VER
    MOCK_METHOD(servent*, getservent, (), (override));
    MOCK_METHOD(servent*, getservbyname, (const char* name, const char* proto),
                (override));
    MOCK_METHOD(servent*, getservbyport, (int port, const char* proto),
                (override));
    MOCK_METHOD(void, setservent, (int stayopen), (override));
    MOCK_METHOD(void, endservent, (), (override));
#endif
    ENABLE_MSVC_WARN
};

} // namespace umock

#endif // UMOCK_NETDB_MOCK_HPP
