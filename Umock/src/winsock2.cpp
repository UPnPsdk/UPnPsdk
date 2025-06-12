// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-11

#include "umock/winsock2.inc"

namespace umock {

int Winsock2Real::WSAGetLastError() { return ::WSAGetLastError(); }

// This constructor is used to inject the pointer to the real function.
Winsock2::Winsock2(Winsock2Real* a_ptr_realObj) {
    m_ptr_workerObj = (Winsock2Interface*)a_ptr_realObj;
}

// This constructor is used to inject the pointer to the mocking function.
Winsock2::Winsock2(Winsock2Interface* a_ptr_mockObj) {
    m_ptr_oldObj = m_ptr_workerObj;
    m_ptr_workerObj = a_ptr_mockObj;
}

// The destructor is ussed to restore the old pointer.
Winsock2::~Winsock2() { m_ptr_workerObj = m_ptr_oldObj; }

// Methods
int Winsock2::WSAGetLastError() { return m_ptr_workerObj->WSAGetLastError(); }

// On program start create an object and inject pointer to the real functions.
// This will exist until program end.
Winsock2Real winsock2_realObj;
UPnPsdk_VIS Winsock2 winsock2_h(&winsock2_realObj);

} // namespace umock
