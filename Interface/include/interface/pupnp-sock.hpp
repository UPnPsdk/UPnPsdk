#ifndef INTERFACE_PUPNP_SOCK_HPP
#define INTERFACE_PUPNP_SOCK_HPP
// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-05-21

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
