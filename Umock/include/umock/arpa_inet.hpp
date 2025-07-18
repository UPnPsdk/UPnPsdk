#ifndef UMOCK_ARPA_INET_HPP
#define UMOCK_ARPA_INET_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-11

#include <UPnPsdk/port_sock.hpp>
#include <UPnPsdk/visibility.hpp>

namespace umock {

// clang-format off

class UPnPsdk_VIS Arpa_inetInterface {
  public:
    Arpa_inetInterface();
    virtual ~Arpa_inetInterface();
    virtual const char* inet_ntop(int af, const void* src, char* dst, socklen_t size) = 0;
};

//
// This is the wrapper class for the real (library?) function
// ----------------------------------------------------------
class Arpa_inetReal : public Arpa_inetInterface {
  public:
    Arpa_inetReal();
    virtual ~Arpa_inetReal() override;
    const char* inet_ntop(int af, const void* src, char* dst, socklen_t size) override;
};

//
// This is the caller or injector class that injects the class (worker) to be
// used, real or mocked functions.
/* Example:
    Arpa_inetReal arpa_inet_realObj;           // already done
    Arpa_inet arpa_inet_h(&arpa_inet_realObj); // already done
    { // Other scope, e.g. within a gtest
        class Arpa_inetMock : public Arpa_inetInterface { ...; MOCK_METHOD(...) };
        Arpa_inetMock arpa_inet_mockObj;
        Arpa_inet arpa_inet_injectObj(&arpa_inet_mockObj); // obj. name doesn't matter
        EXPECT_CALL(arpa_inet_mockObj, ...);
    } // End scope, mock objects are destructed, worker restored to default.
*/ //------------------------------------------------------------------------
class UPnPsdk_VIS Arpa_inet {
  public:
    // This constructor is used to inject the pointer to the real function. It
    // sets the default used class, that is the real function.
    Arpa_inet(Arpa_inetReal* a_ptr_realObj);

    // This constructor is used to inject the pointer to the mocking function.
    Arpa_inet(Arpa_inetInterface* a_ptr_mockObj);

    // The destructor is ussed to restore the old pointer.
    virtual ~Arpa_inet();

    // Methods
    virtual const char* inet_ntop(int af, const void* src, char* dst, socklen_t size);

  private:
    // Next variable must be static. Please note that a static member variable
    // belongs to the class, but not to the instantiated object. This is
    // important here for mocking because the pointer is also valid on all
    // objects of this class. With inline we do not need an extra definition
    // line outside the class. I also make the symbol hidden so the variable
    // cannot be accessed globaly with Arpa_inet::m_ptr_workerObj.
    UPnPsdk_LOCAL static inline Arpa_inetInterface* m_ptr_workerObj;
    Arpa_inetInterface* m_ptr_oldObj{};
};
// clang-format off


UPnPsdk_EXTERN Arpa_inet arpa_inet_h;

} // namespace umock

#endif // UMOCK_ARPA_INET_HPP
