#ifndef UMOCK_IFADDRS_HPP
#define UMOCK_IFADDRS_HPP
// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-11-07

#include <UPnPsdk/visibility.hpp>
#include <ifaddrs.h>

namespace umock {

class UPnPsdk_API IfaddrsInterface {
  public:
    IfaddrsInterface();
    virtual ~IfaddrsInterface();
    virtual int getifaddrs(struct ifaddrs** ifap) = 0;
    virtual void freeifaddrs(struct ifaddrs* ifa) = 0;
};

//
// This is the wrapper class for the real (library?) function
// ----------------------------------------------------------
class IfaddrsReal : public IfaddrsInterface {
  public:
    IfaddrsReal();
    virtual ~IfaddrsReal() override;
    int getifaddrs(struct ifaddrs** ifap) override;
    void freeifaddrs(struct ifaddrs* ifa) override;
};

//
// This is the caller or injector class that injects the class (worker) to be
// used, real or mocked functions.
// clang-format off
/* Example:
    IfaddrsReal ifaddrs_realObj; // already done below
    Ifaddrs(&ifaddrs_realObj);   // already done below
    { // Other scope, e.g. within a gtest
        class IfaddrsMock : public IfaddrsInterface { ...; MOCK_METHOD(...) };
        IfaddrsMock ifaddrs_mockObj;
        Ifaddrs ifaddrs_injectObj(&ifaddrs_mockObj); // obj. name doesn't matter
        EXPECT_CALL(ifaddrs_mockObj, ...);
    } // End scope, mock objects are destructed, worker restored to default.
*/ //----------------------------------------------------------------------------
// clang-format on
class UPnPsdk_API Ifaddrs {
  public:
    // This constructor is used to inject the pointer to the real function. It
    // sets the default used class, that is the real function.
    Ifaddrs(IfaddrsReal* a_ptr_mockObj);

    // This constructor is used to inject the pointer to the mocking function.
    Ifaddrs(IfaddrsInterface* a_ptr_mockObj);

    // The destructor is ussed to restore the old pointer.
    virtual ~Ifaddrs();

    // Methods
    virtual int getifaddrs(struct ifaddrs** ifap);
    virtual void freeifaddrs(struct ifaddrs* ifa);

  private:
    // Next variable must be static. Please note that a static member variable
    // belongs to the class, but not to the instantiated object. This is
    // important here for mocking because the pointer is also valid on all
    // objects of this class. With inline we do not need an extra definition
    // line outside the class. I also make the symbol hidden so the variable
    // cannot be accessed globaly with Ifaddrs::m_ptr_workerObj.
    UPnPsdk_LOCAL static inline IfaddrsInterface* m_ptr_workerObj;
    IfaddrsInterface* m_ptr_oldObj{};
};


UPnPsdk_EXTERN Ifaddrs ifaddrs_h;

} // namespace umock

#endif // UMOCK_IFADDRS_HPP
