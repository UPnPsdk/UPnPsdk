/// \cond
#ifndef INTERFACE_PUPNP_SOCK_HPP
#define INTERFACE_PUPNP_SOCK_HPP
// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-05-31

// Not used at time but have it available for later use. It was too much typing
// to just throw it away.

#include <sock.hpp>

// Interface for the sock module
// =============================
// clang-format off

class UPnPsdk_EXP SockInterface {
  public:
    virtual ~SockInterface();

    virtual int sock_init(
        SOCKINFO* info, SOCKET sockfd) = 0;
    virtual int sock_init_with_ip(
        SOCKINFO* info, SOCKET sockfd, struct sockaddr* foreign_sockaddr) = 0;
#ifdef UPnPsdk_HAVE_OPENSSL
    virtual int sock_ssl_connect(
        SOCKINFO* info) = 0;
#endif
    virtual int sock_destroy(
        SOCKINFO* info, int ShutdownMethod) = 0;
    virtual int sock_read(
        SOCKINFO* info, char* buffer, size_t bufsize, int* timeoutSecs) = 0;
    virtual int sock_write(
        SOCKINFO* info, const char* buffer, size_t bufsize, int* timeoutSecs) = 0;
    virtual int sock_make_blocking(
        SOCKET sock) = 0;
    virtual int sock_make_no_blocking(
        SOCKET sock) = 0;
    virtual int sock_close(
        SOCKET sock) = 0;
};
// clang-format on

#endif // INTERFACE_PUPNP_SOCK_HPP


#define INCLUDE_COMPA_SOCK_HPP
// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-09-13

#include <interface/pupnp-sock.hpp>

namespace compa {

// Declarations for the sock module using the interface
// ====================================================
// clang-format off

class UPNPLIB_API Csock : public ::SockInterface {
  public:
    virtual ~Csock() override = default;

    UPNPLIB_LOCAL int sock_init(SOCKINFO* info, SOCKET sockfd) override {
        return ::sock_init(info, sockfd); }
    UPNPLIB_LOCAL int sock_init_with_ip( SOCKINFO* info, SOCKET sockfd, struct sockaddr* foreign_sockaddr) override {
        return ::sock_init_with_ip(info, sockfd, foreign_sockaddr); }
#ifdef UPNP_ENABLE_OPEN_SSL
    int sock_ssl_connect( SOCKINFO* info) override;
#endif
    UPNPLIB_LOCAL int sock_destroy(SOCKINFO* info, int ShutdownMethod) override {
        return ::sock_destroy(info, ShutdownMethod); }
    UPNPLIB_LOCAL int sock_read(SOCKINFO* info, char* buffer, size_t bufsize, int* timeoutSecs) override {
        return ::sock_read(info, buffer, bufsize, timeoutSecs); }
    UPNPLIB_LOCAL int sock_write(SOCKINFO* info, const char* buffer, size_t bufsize, int* timeoutSecs) override {
        return ::sock_write(info, buffer, bufsize, timeoutSecs); }
    UPNPLIB_LOCAL int sock_make_blocking(SOCKET sock) override {
        return ::sock_make_blocking(sock); }
    UPNPLIB_LOCAL int sock_make_no_blocking(SOCKET sock) override {
        return ::sock_make_no_blocking(sock); }
    UPNPLIB_LOCAL int sock_close(SOCKET sock) override {
        return ::sock_close(sock); }
};
// clang-format on

} // namespace compa

#endif // INCLUDE_COMPA_SOCK_HPP


// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-05-21

#include <interface/pupnp-sock.hpp>

// Interface for the sock module
// =============================

// This destructor can also be defined direct in the header file as usual but
// then the symbol is included and not linked. We have to decorate the symbol
// with __declspec(dllexport) on Microsoft Windows in a header file that
// normaly import symbols. It does not conform to the visibility macro
// UPnPsdk_EXP and would require other advanced special handling. So we link
// the symbol with this source file so it do the right think with it. --Ingo
SockInterface::~SockInterface() = default;

/// \endcond
