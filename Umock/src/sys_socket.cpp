// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-11

#include <umock/sys_socket.hpp>

namespace umock {

Sys_socketInterface::Sys_socketInterface() = default;
Sys_socketInterface::~Sys_socketInterface() = default;

Sys_socketReal::Sys_socketReal() = default;
Sys_socketReal::~Sys_socketReal() = default;
// clang-format off
SOCKET Sys_socketReal::socket(int domain, int type, int protocol) {
    return ::socket(domain, type, protocol);
}

int Sys_socketReal::bind(SOCKET sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    return ::bind(sockfd, addr, addrlen);
}

int Sys_socketReal::listen(SOCKET sockfd, int backlog) {
    return ::listen(sockfd, backlog);
}

SOCKET Sys_socketReal::accept(SOCKET sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    return ::accept(sockfd, addr, addrlen);
}

SSIZEP_T Sys_socketReal::recv(SOCKET sockfd, char* buf, SIZEP_T len, int flags) {
    return ::recv(sockfd, buf, len, flags);
}

SSIZEP_T Sys_socketReal::recvfrom(SOCKET sockfd, char* buf, SIZEP_T len, int flags, struct sockaddr* src_addr, socklen_t* addrlen) {
    return ::recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
}

SSIZEP_T Sys_socketReal::send(SOCKET sockfd, const char* buf, SIZEP_T len, int flags) {
    return ::send(sockfd, buf, len, flags);
}

SSIZEP_T Sys_socketReal::sendto(SOCKET sockfd, const char* buf, SIZEP_T len, int flags, const struct sockaddr* dest_addr, socklen_t addrlen) {
    return ::sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

int Sys_socketReal::connect(SOCKET sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    return ::connect(sockfd, addr, addrlen);
}

int Sys_socketReal::getsockopt(SOCKET sockfd, int level, int optname, void* optval, socklen_t* optlen) {
#ifdef _WIN32
    return ::getsockopt(sockfd, level, optname, (char*)optval, optlen);
#else
    return ::getsockopt(sockfd, level, optname, optval, optlen);
#endif
}

int Sys_socketReal::setsockopt(SOCKET sockfd, int level, int optname, const void* optval, socklen_t optlen) {
#ifdef _WIN32
    return ::setsockopt(sockfd, level, optname, (const char*)optval, optlen);
#else
    return ::setsockopt(sockfd, level, optname, optval, optlen);
#endif
}

int Sys_socketReal::getsockname(SOCKET sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    return ::getsockname(sockfd, addr, addrlen);
}

int Sys_socketReal::getpeername(SOCKET sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    return ::getpeername(sockfd, addr, addrlen);
}

int Sys_socketReal::shutdown(SOCKET sockfd, int how) {
    return ::shutdown(sockfd, how);
}

int Sys_socketReal::select(SOCKET nfds, fd_set* readfds, fd_set* writefds,
                           fd_set* exceptfds, struct timeval* timeout) {
    // Call real standard library function. On MS Windows nfds is only provided
    // for compatibility but ignored so the type cast doesn't matter.
    return ::select((int)nfds, readfds, writefds, exceptfds, timeout);
}
// clang-format on


// This constructor is used to inject the pointer to the real function.
Sys_socket::Sys_socket(Sys_socketReal* a_ptr_realObj) {
    m_ptr_workerObj = (Sys_socketInterface*)a_ptr_realObj;
}

// This constructor is used to inject the pointer to the mocking function.
Sys_socket::Sys_socket(Sys_socketInterface* a_ptr_mockObj) {
    m_ptr_oldObj = m_ptr_workerObj;
    m_ptr_workerObj = a_ptr_mockObj;
}

// The destructor is ussed to restore the old pointer.
Sys_socket::~Sys_socket() { m_ptr_workerObj = m_ptr_oldObj; }

// Methods
// clang-format off
SOCKET Sys_socket::socket(int domain, int type, int protocol) {
    return m_ptr_workerObj->socket(domain, type, protocol);
}
int Sys_socket::bind(SOCKET sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    return m_ptr_workerObj->bind(sockfd, addr, addrlen);
}
int Sys_socket::listen(SOCKET sockfd, int backlog) {
    return m_ptr_workerObj->listen(sockfd, backlog);
}
SOCKET Sys_socket::accept(SOCKET sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    return m_ptr_workerObj->accept(sockfd, addr, addrlen);
}
SSIZEP_T Sys_socket::recv(SOCKET sockfd, char* buf, SIZEP_T len, int flags) {
    return m_ptr_workerObj->recv(sockfd, buf, len, flags);
}
SSIZEP_T Sys_socket::recvfrom(SOCKET sockfd, char* buf, SIZEP_T len, int flags, struct sockaddr* src_addr, socklen_t* addrlen) {
    return m_ptr_workerObj->recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
}
SSIZEP_T Sys_socket::send(SOCKET sockfd, const char* buf, SIZEP_T len, int flags) {
    return m_ptr_workerObj->send(sockfd, buf, len, flags);
}
SSIZEP_T Sys_socket::sendto(SOCKET sockfd, const char* buf, SIZEP_T len, int flags, const struct sockaddr* dest_addr, socklen_t addrlen) {
    return m_ptr_workerObj->sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}
int Sys_socket::connect(SOCKET sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    return m_ptr_workerObj->connect(sockfd, addr, addrlen);
}
int Sys_socket::getsockopt(SOCKET sockfd, int level, int optname, void* optval, socklen_t* optlen) {
#ifdef _WIN32
    return m_ptr_workerObj->getsockopt(sockfd, level, optname, (char*)optval, optlen);
#else
    return m_ptr_workerObj->getsockopt(sockfd, level, optname, optval, optlen);
#endif
}
int Sys_socket::setsockopt(SOCKET sockfd, int level, int optname, const void* optval, socklen_t optlen) {
#ifdef _WIN32
    return m_ptr_workerObj->setsockopt(sockfd, level, optname, (const char*)optval, optlen);
#else
    return m_ptr_workerObj->setsockopt(sockfd, level, optname, optval, optlen);
#endif
}
int Sys_socket::getsockname(SOCKET sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    return m_ptr_workerObj->getsockname(sockfd, addr, addrlen);
}
int Sys_socket::getpeername(SOCKET sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    return m_ptr_workerObj->getpeername(sockfd, addr, addrlen);
}
int Sys_socket::shutdown(SOCKET sockfd, int how) {
    return m_ptr_workerObj->shutdown(sockfd, how);
}
int Sys_socket::select(SOCKET nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout) {
    return m_ptr_workerObj->select(nfds, readfds, writefds, exceptfds, timeout);
}
// clang-format on

//
// On program start create an object and inject pointer to the real functions.
// This will exist until program end.
Sys_socketReal sys_socket_realObj;
SUPPRESS_MSVC_WARN_4273_NEXT_LINE
UPnPsdk_VIS Sys_socket sys_socket_h(&sys_socket_realObj);

} // namespace umock
