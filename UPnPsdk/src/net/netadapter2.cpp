// Copyright (C) 2025+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-03-15
/*!
 * \file
 * \brief Definition of the 'class CNetadapter2'.
 */

#include <UPnPsdk/netadapter2.hpp>
#include <UPnPsdk/synclog.hpp>
#ifdef _MSC_VER
#include <winsock2.h>
#endif

namespace UPnPsdk {

// Socket Error di-interface
ISocketErr::ISocketErr() = default;
ISocketErr::~ISocketErr() = default;

// Socket Error di-client
CSocketErr2::CSocketErr2(PSocketErr a_socket_errObj)
    : m_socket_errObj(a_socket_errObj) {
    TRACE2(this, " Construct CSocketErr2()") //
}
CSocketErr2::~CSocketErr2() {
    TRACE2(this, " Destruct CSocketErr2()") //
}
CSocketErr2::operator const int&() { //
    return *m_socket_errObj;
}
void CSocketErr2::catch_error() { //
    m_socket_errObj->catch_error();
}
std::string CSocketErr2::get_error_str() const {
    return m_socket_errObj->get_error_str();
}


// Portable handling of socket errors
// ==================================
CSocketErrService::CSocketErrService(){
    TRACE2(this, " Construct CSocketErrService()")}

CSocketErrService::~CSocketErrService(){
    TRACE2(this, " Destruct CSocketErrService()")}

CSocketErrService::operator const int&() {
    // TRACE not usable with chained output.
    // TRACE2(this,
    //     " Executing CSocketErrService::operator int&() (get socket error
    //     number)")
    return m_errno;
}

void CSocketErrService::catch_error() {
#ifdef _MSC_VER
    m_errno = WSAGetLastError();
#else
    m_errno = errno;
#endif
    TRACE2(this, " Executing CSocketErrService::catch_error()")
}

std::string CSocketErrService::get_error_str() const {
    // TRACE not usable with chained output, e.g.
    // std::cerr << "Error: " << sockerrObj.get_error_str();
    // TRACE2(this, " Executing CSocketErrService::get_error_str()")

    // Portable C++ statement
    return std::system_category().message(m_errno);
    // return std::generic_category().message(m_errno);
    // return std::strerror(m_errno); // Alternative for Unix platforms
}

} // namespace UPnPsdk
