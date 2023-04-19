// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-04-18

#include "umock/arpa_inet.inc"

namespace umock {

// clang-format off

const char* Arpa_inetReal::inet_ntop(int af, const void* src, char* dst, socklen_t size) {
    return ::inet_ntop(af, src, dst, size);
}

// This constructor is used to inject the pointer to the real function.
Arpa_inet::Arpa_inet(Arpa_inetReal* a_ptr_realObj) {
    m_ptr_workerObj = (Arpa_inetInterface*)a_ptr_realObj;
}

// This constructor is used to inject the pointer to the mocking function.
Arpa_inet::Arpa_inet(Arpa_inetInterface* a_ptr_mockObj) {
    m_ptr_oldObj = m_ptr_workerObj;
    m_ptr_workerObj = a_ptr_mockObj;
}

// The destructor is ussed to restore the old pointer.
Arpa_inet::~Arpa_inet() { m_ptr_workerObj = m_ptr_oldObj; }

// Methods
const char* Arpa_inet::inet_ntop(int af, const void* src, char* dst, socklen_t size) {
    return m_ptr_workerObj->inet_ntop(af, src, dst, size);
}

// On program start create an object and inject pointer to the real functions.
// This will exist until program end.
Arpa_inetReal arpa_inet_realObj;
UPNPLIB_API Arpa_inet arpa_inet_h(&arpa_inet_realObj);

// clang-format on

} // namespace umock
