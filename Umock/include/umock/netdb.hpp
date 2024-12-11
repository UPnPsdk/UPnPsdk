#ifndef MOCK_NETDB_HPP
#define MOCK_NETDB_HPP
// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-12-11

#include <UPnPsdk/visibility.hpp>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <netdb.h> // important for struct addrinfo
#endif

namespace umock {

class UPnPsdk_API NetdbInterface {
  public:
    NetdbInterface();
    virtual ~NetdbInterface();
    virtual int getaddrinfo(const char* node, const char* service,
                            const struct addrinfo* hints,
                            struct addrinfo** res) = 0;
    virtual void freeaddrinfo(struct addrinfo* res) = 0;
#ifndef _MSC_VER
    virtual servent* getservent() = 0;
    virtual servent* getservbyname(const char* name, const char* proto) = 0;
    virtual servent* getservbyport(int port, const char* proto) = 0;
    virtual void setservent(int stayopen) = 0;
    virtual void endservent() = 0;
#endif
};

//
// This is the wrapper class for the real (library?) function
// ----------------------------------------------------------
class NetdbReal : public NetdbInterface {
  public:
    NetdbReal();
    virtual ~NetdbReal() override;
    int getaddrinfo(const char* node, const char* service,
                    const struct addrinfo* hints,
                    struct addrinfo** res) override;
    void freeaddrinfo(struct addrinfo* res) override;
#ifndef _MSC_VER
    servent* getservent() override;
    servent* getservbyname(const char* name, const char* proto) override;
    servent* getservbyport(int port, const char* proto) override;
    void setservent(int stayopen) override;
    void endservent() override;
#endif
};

//
// This is the caller or injector class that injects the class (worker) to be
// used, real or mocked functions.
/* Example:
    NetdbReal netdb_realObj; // already done below
    Netdb(&netdb_realObj);   // already done below
    { // Other scope, e.g. within a gtest
        class NetdbMock : public NetdbInterface { ...; MOCK_METHOD(...) };
        NetdbMock netdb_mockObj;
        Netdb netdb_injectObj(&netdb_mockObj); // obj. name doesn't matter
        EXPECT_CALL(netdb_mockObj, ...);
    } // End scope, mock objects are destructed, worker restored to default.
*/ //------------------------------------------------------------------------
class UPnPsdk_API Netdb {
  public:
    // This constructor is used to inject the pointer to the real function. It
    // sets the default used class, that is the real function.
    Netdb(NetdbReal* a_ptr_realObj);

    // This constructor is used to inject the pointer to the mocking function.
    Netdb(NetdbInterface* a_ptr_mockObj);

    // The destructor is ussed to restore the old pointer.
    virtual ~Netdb();

    virtual int getaddrinfo(const char* node, const char* service,
                            const struct addrinfo* hints,
                            struct addrinfo** res);
    virtual void freeaddrinfo(struct addrinfo* res);
#ifndef _MSC_VER
    virtual servent* getservent();
    virtual servent* getservbyname(const char* name, const char* proto);
    virtual servent* getservbyport(int port, const char* proto);
    virtual void setservent(int stayopen);
    virtual void endservent();
#endif

  private:
    // Next variable must be static. Please note that a static member variable
    // belongs to the class, but not to the instantiated object. This is
    // important here for mocking because the pointer is also valid on all
    // objects of this class. With inline we do not need an extra definition
    // line outside the class. I also make the symbol hidden so the variable
    // cannot be accessed globaly with Netdb::m_ptr_workerObj.
    UPnPsdk_LOCAL static inline NetdbInterface* m_ptr_workerObj;
    NetdbInterface* m_ptr_oldObj{};
};


UPnPsdk_EXTERN Netdb netdb_h;

} // namespace umock

#endif // MOCK_NETDB_HPP
