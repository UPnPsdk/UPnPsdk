// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-04-10
/*!
 * \file
 * \brief Definition of the 'class Socket'.
 */

#include <UPnPsdk/socket.hpp>

#include <UPnPsdk/addrinfo.hpp>
#include <umock/sys_socket.hpp>
#include <umock/stringh.hpp>
#ifdef _MSC_VER
#include <umock/winsock2.hpp>
/// \cond
#include <array>
/// \endcond
#endif

namespace UPnPsdk {

namespace {

// Free helper functions
// =====================
/*!
 * \brief Wrapper for the ::%getsockname() system function
 * <!--   ------------------------------------------------ -->
 * \ingroup upnplib-socket
 *
 * The system function ::%getsockname() behaves different on different
 * platforms in particular with error handling on Microsoft Windows. This
 * function provides a portable version. The calling options are the same as
 * documented for the system function.
 */
int getsockname(SOCKET a_sockfd, sockaddr* a_addr, socklen_t* a_addrlen) {
    TRACE("Executing getsockname()");

    int ret = umock::sys_socket_h.getsockname(a_sockfd, a_addr, a_addrlen);

    if (ret == 0) {
        return 0;
    }
#ifndef _MSC_VER
    return ret;

#else
    if (umock::winsock2_h.WSAGetLastError() != WSAEINVAL) // Error 10022 not set
        return ret;

    // WSAEINVAL indicates that the socket is unbound. We will return an
    // empty sockaddr with address family set. This is what we get on Unix
    // platforms. On Microsoft Windows we cannot use ::getsockname() like
    // on unix platforms because it returns an error indicating an unbound
    // socket and an untouched socket address storage. Here we have to use
    // an alternative ::getsockopt() that provides additional info with a
    // non-standard option SO_PROTOCOL_INFO.
    ::WSAPROTOCOL_INFO protocol_info{};
    int len{sizeof(protocol_info)}; // May be modified?
    if (umock::sys_socket_h.getsockopt(a_sockfd, SOL_SOCKET, SO_PROTOCOL_INFO,
                                       &protocol_info, &len) != 0)
        return WSAENOBUFS; // Insufficient resources were available in the
                           // system to perform the operation.

    memset(a_addr, 0, *a_addrlen);
    // Microsoft itself defines sockaddr.sa_family as ushort, so the typecast
    // from int should not do any harm.
    a_addr->sa_family =
        static_cast<unsigned short>(protocol_info.iAddressFamily);
    return 0;
#endif
}

/*!
 * \brief Get a socket file descriptor from the operating system
 * <!--   -------------------------------------------------- -->
 * \ingroup upnplib-socket
 *
 * Get a socket file descriptor and set its default options as specified.
 */
SOCKET get_sockfd(sa_family_t a_pf_family, int a_socktype) {
    TRACE(" Executing get_sockfd()")

    // Do some general checks that must always be done according to the
    // specification.
    if (a_pf_family != PF_INET6 && a_pf_family != PF_INET)
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT(
                "MSG1015") "Failed to create socket: invalid protocol family " +
            std::to_string(a_pf_family));
    if (a_socktype != SOCK_STREAM && a_socktype != SOCK_DGRAM)
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT(
                "MSG1016") "Failed to create socket: invalid socket type " +
            std::to_string(a_socktype));

    CSocketErr serrObj;

    // Syscall socket(): get new socket file descriptor.
    SOCKET sfd = umock::sys_socket_h.socket(a_pf_family, a_socktype, 0);
    UPnPsdk_LOGINFO("MSG1135") "syscall ::socket("
        << a_pf_family << ", " << a_socktype << ", 0). Get socket fd " << sfd
        << '\n';
    if (sfd == INVALID_SOCKET) {
        serrObj.catch_error();
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG1017") "Failed to create socket: " +
            serrObj.error_str() + '\n');
    }
    int so_option{0};
    constexpr socklen_t optlen{sizeof(so_option)};

    // Reset SO_REUSEADDR on all platforms if it should be set by default. This
    // is unclear on WIN32. See note below.
    // Type cast (char*)&so_option is needed for Microsoft Windows.
    if (umock::sys_socket_h.setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR,
                                       reinterpret_cast<char*>(&so_option),
                                       optlen) != 0) {
        serrObj.catch_error();
        CLOSE_SOCKET_P(sfd);
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG1018") "Close socket fd " +
            std::to_string(sfd) +
            ". Failed to set socket option SO_REUSEADDR: " +
            serrObj.error_str() + '\n');
    }

    // With protocol family PF_INET6 I always set IPV6_V6ONLY to false. See
    // also note to bind() in the header file.
    if (a_pf_family == AF_INET6) {
        so_option = 0; // false
        // Type cast (char*)&so_option is needed for Microsoft Windows.
        if (umock::sys_socket_h.setsockopt(
                sfd, IPPROTO_IPV6, IPV6_V6ONLY,
                reinterpret_cast<const char*>(&so_option),
                sizeof(so_option)) != 0) {
            serrObj.catch_error();
            CLOSE_SOCKET_P(sfd);
            throw std::runtime_error(
                UPnPsdk_LOGEXCEPT("MSG1007") "Close socket fd " +
                std::to_string(sfd) +
                ". Failed to set socket option IPV6_V6ONLY: " +
                serrObj.error_str() + '\n');
        }
    }

#ifdef _MSC_VER
    // Set socket option SO_EXCLUSIVEADDRUSE on Microsoft Windows. THIS IS AN
    // IMPORTANT SECURITY ISSUE! But it needs special handling with binding a
    // socket fd to an ip address because it may be possible that even a new
    // unbound socket fd is blocked by the operating system. Lock at
    // REF:_[Using SO_REUSEADDR and SO_EXCLUSIVEADDRUSE]
    // (https://learn.microsoft.com/en-us/windows/win32/winsock/using-so-reuseaddr-and-so-exclusiveaddruse#application-strategies)
    so_option = 1; // Set SO_EXCLUSIVEADDRUSE
    // Type cast (char*)&so_option is needed for Microsoft Windows.
    if (umock::sys_socket_h.setsockopt(sfd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
                                       reinterpret_cast<char*>(&so_option),
                                       optlen) != 0) {
        serrObj.catch_error();
        CLOSE_SOCKET_P(sfd);
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG1019") "Close socket fd " +
            std::to_string(sfd) +
            ". Failed to set socket option SO_EXCLUSIVEADDRUSE: " +
            serrObj.error_str() + '\n');
    }
#endif
    return sfd;
}

} // anonymous namespace


// CSocket_basic class
// ===================
// Default constructor for an empty socket object
CSocket_basic::CSocket_basic(){
    TRACE2(this, " Construct default CSocket_basic()") //
}

// Constructor for the socket file descriptor. Before use, it must be load().
CSocket_basic::CSocket_basic(SOCKET a_sfd)
    : m_sfd_hint(a_sfd) {
    TRACE2(this, " Construct CSocket_basic(SOCKET)") //
}

// Setter with given file desciptor
void CSocket_basic::load() {
    TRACE2(this, " Executing CSocket_basic::load()")

    CSocketErr serrObj;
    // Check if we have a valid socket file descriptor
    int so_option{-1};
    socklen_t optlen{sizeof(so_option)}; // May be modified
    // Type cast (char*)&so_option is needed for Microsoft Windows.
    TRACE2(this, " Calling system function ::getsockopt().")
    if (umock::sys_socket_h.getsockopt(m_sfd_hint, SOL_SOCKET, SO_ERROR,
                                       reinterpret_cast<char*>(&so_option),
                                       &optlen) != 0) {
        serrObj.catch_error();
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG1014") "Failed to create socket=" +
            std::to_string(m_sfd_hint) + ": " + serrObj.error_str() + '\n');
    }
    m_sfd = m_sfd_hint;
}

// Destructor
CSocket_basic::~CSocket_basic(){
    TRACE2(this, " Destruct CSocket_basic()") //
}

// Get the raw socket file descriptor
CSocket_basic::operator const SOCKET&() const {
    TRACE2(this, " Executing CSocket_basic::operator SOCKET&() (get "
                 "raw socket fd)")
    // There is no problem with cast here. We cast to const so we can only read.
    return const_cast<SOCKET&>(m_sfd);
}

// Getter
// ------
void CSocket_basic::sockaddr(SSockaddr& a_saddr) const {
    TRACE2(this, " Executing CSocket_basic::sockaddr()")
    if (m_sfd == INVALID_SOCKET) {
        a_saddr = "";
        return;
    }
    // Get address from socket file descriptor.
    socklen_t addrlen = sizeof(a_saddr.ss); // May be modified
    CSocketErr serrObj;
    int ret = UPnPsdk::getsockname(m_sfd, &a_saddr.sa, &addrlen);
    if (ret != 0) {
        serrObj.catch_error();
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG1001") "Failed to get address from socket: " +
            serrObj.error_str() + '\n');
    }
    // Check if there is a complete address structure returned from
    // UPnPsdk::getsockname(). On macOS the function returns only part of the
    // address structure if the socket file descriptor isn't bound to an
    // address of a local network adapter. It trunkates the address part.
    sa_family_t af = a_saddr.ss.ss_family;
    if ((af == AF_INET6 && addrlen == sizeof(a_saddr.sin6)) ||
        (af == AF_INET && addrlen == sizeof(a_saddr.sin)) ||
        (af == AF_UNIX && addrlen == sizeof(a_saddr.sun)))
        return;

    // If there is no complete address structure returned from
    // UPnPsdk::getsockname() we return here an empty socket address with
    // preserved address family.
    a_saddr = "";
    a_saddr.ss.ss_family = af;
    UPnPsdk_LOGERR("MSG1133") "Unspecified socket address detected. Is the "
                              "socket bound to a local address? Continue with "
                              "unspecified address and address family "
        << af << ".\n";
}

int CSocket_basic::socktype() const {
    TRACE2(this, " Executing CSocket_basic::socktype()")
    int so_option{-1};
    socklen_t len{sizeof(so_option)}; // May be modified
    CSocketErr serrObj;
    // Type cast (char*)&so_option is needed for Microsoft Windows.
    if (umock::sys_socket_h.getsockopt(m_sfd, SOL_SOCKET, SO_TYPE,
                                       reinterpret_cast<char*>(&so_option),
                                       &len) != 0) {
        serrObj.catch_error();
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG1030") "Failed to get socket option SO_TYPE "
                                         "(SOCK_STREAM, SOCK_DGRAM, etc.): " +
            serrObj.error_str() + '\n');
    }
    return so_option;
}

int CSocket_basic::sockerr() const {
    TRACE2(this, " Executing CSocket_basic::sockerr()")
    int so_option{-1};
    socklen_t len{sizeof(so_option)}; // May be modified
    CSocketErr serrObj;
    // Type cast (char*)&so_option is needed for Microsoft Windows.
    if (umock::sys_socket_h.getsockopt(m_sfd, SOL_SOCKET, SO_ERROR,
                                       reinterpret_cast<char*>(&so_option),
                                       &len) != 0) {
        serrObj.catch_error();
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT(
                "MSG1011") "Failed to get socket option SO_ERROR: " +
            serrObj.error_str() + '\n');
    }
    return so_option;
}

bool CSocket_basic::is_reuse_addr() const {
    TRACE2(this, " Executing CSocket_basic::is_reuse_addr()")
    int so_option{-1};
    socklen_t len{sizeof(so_option)}; // May be modified
    CSocketErr serrObj;
    // Type cast (char*)&so_option is needed for Microsoft Windows.
    if (umock::sys_socket_h.getsockopt(m_sfd, SOL_SOCKET, SO_REUSEADDR,
                                       reinterpret_cast<char*>(&so_option),
                                       &len) != 0) {
        serrObj.catch_error();
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT(
                "MSG1013") "Failed to get socket option SO_REUSEADDR: " +
            serrObj.error_str() + '\n');
    }
    return so_option;
}

int CSocket_basic::is_bound() const {
    // I get the socket address from the file descriptor and check if its
    // address and port are all zero. I have to do this different for AF_INET6
    // and AF_INET. Because I only use ::getaddrinfo() to get internet address
    // information I can use its criteria to detect binding. With
    // ::getaddrinfo():
    // - Either node or service (port), but not both, may be NULL means a bound
    //   address must have at least a node address or a port.
    // - If the AI_PASSIVE flag was specified, and node is NULL, then the
    // returned socket addresses will be suitable for binding a socket that
    // will ::accept() connections. The returned socket address will contain the
    // "wildcard  address" (INADDR_ANY for IPv4 addresses, IN6ADDR_ANY_INIT for
    // IPv6 address).
    // - If the AI_PASSIVE flag was not specified, then the returned socket
    // addresses will be suitable for use with ::connect(), ::sendto(), or
    // ::sendmsg(). If node is NULL, then the network address will be set to
    // the loopback interface  address (INADDR_LOOPBACK for IPv4 addresses,
    // IN6ADDR_LOOPBACK_INIT for IPv6 address).
    // - Syscall ::bind() accepts a socket address structure that always has a
    // numeric service/port number. 0 is interpreted as "unspecified port" and
    // ::bind() will use a free random port number, so a bound socket always
    // have a port number > 0.
    //
    // This all means for a socket when asking for its bound address with
    // syscall ::getsockname():
    // - A socket address with port 0 cannot be bound, otherwise
    // - an unspecified socket address of the address family AF_INET6 or
    //   AF_INET (means with the "wildcard address" (all zero)) is passive
    //   bound to listen on local network adapters with ::accept().
    // - A socket address with any other ip address is (active) bound to the
    //   local network adapter with that ip address for use with syscalls
    //   ::connect(), ::sendto(), or ::sendmsg().
    //
    // An older but incomplete version of this member function with direct
    // compare of the socket address with a 16 null byte array AF_INET6 can be
    // found at commit
    // a5ec86a93608234016630123c776c09f8ff276fb:upnplib/src/net/socket.cpp.
    // TRACE2(this, " Executing CSocket::is_bound()")

    // binding is protected.
    std::scoped_lock lock(m_bound_mutex);

    // Get the socket address from the socket file descriptor.
    SSockaddr saObj;
    this->sockaddr(saObj);

    // A socket address with port number 0 cannot be bound. 'sin6_port' is on
    // the same structure location as 'sin_port' so it can be used for AF_INET6
    // and AF_INET together.
    if (saObj.sin6.sin6_port == 0u)
        return 0;

    // With an unspecified IP address of a given address family AF_INET6 or
    // AF_INET the socket is "passive" bound for listening, otherwise it is
    // "active" bound.
    switch (saObj.ss.ss_family) {
    case AF_INET6:
        for (size_t i{}; i < sizeof(in6_addr); i++) {
            if (saObj.sin6.sin6_addr.s6_addr[i] != 0u)
                return 1; // Bound active
        }
        return -1; // All zero, bound passive

    case AF_INET:
        return (saObj.sin.sin_addr.s_addr == INADDR_ANY) ? -1 // Bound passive
                                                         : 1; // Bound active
    } // switch

    throw std::invalid_argument(
        "MSG1091: Program error must be fixed. Unsupported address family " +
        std::to_string(saObj.ss.ss_family) + ".\n");
}


// CSocket class
// =============
// Default constructor for an empty socket object
CSocket::CSocket(){
    TRACE2(this, " Construct default CSocket()") //
}

// Move constructor
CSocket::CSocket(CSocket&& that) {
    TRACE2(this, " Construct move CSocket()")
    m_sfd = that.m_sfd;
    that.m_sfd = INVALID_SOCKET;

    // Following variables are protected
    std::scoped_lock lock(m_listen_mutex);
    m_listen = that.m_listen;
    that.m_listen = false;
}

// Assignment operator (parameter as value)
CSocket& CSocket::operator=(CSocket that) {
    TRACE2(this, " Executing CSocket::operator=()")
    std::swap(m_sfd, that.m_sfd);

    // Following variables are protected
    std::scoped_lock lock(m_listen_mutex);
    std::swap(m_listen, that.m_listen);

    return *this;
}

// Destructor
CSocket::~CSocket() {
    TRACE2(this, " Destruct CSocket()")
    if (m_sfd != INVALID_SOCKET)
        UPnPsdk_LOGINFO("MSG1136") "shutdown and close socket fd " << m_sfd
                                                                   << ".\n";
    ::shutdown(m_sfd, SHUT_RDWR);
    CLOSE_SOCKET_P(m_sfd);
}

// Setter
// ------
#if 0
void CSocket::set_reuse_addr(bool a_reuse) {
    // Set socket option SO_REUSEADDR on other platforms.
    // --------------------------------------------------
    // REF: [How do SO_REUSEADDR and SO_REUSEPORT differ?]
    // (https://stackoverflow.com/a/14388707/5014688)
    so_option = a_reuse_addr ? 1 : 0;
    // Type cast (char*)&so_reuseaddr is needed for Microsoft Windows.
    if (umock::sys_socket_h.setsockopt(m_listen_sfd, SOL_SOCKET, SO_REUSEADDR,
                     reinterpret_cast<char*>(&so_option), optlen) != 0)
        throw_error("MSG1004: Failed to set socket option SO_REUSEADDR:");
}
#endif

// Bind socket to an ip address
// ----------------------------
void CSocket::bind(const int a_socktype, SSockaddr* a_saddr,
                   const int a_flags) {
    TRACE2(this, " Executing CSocket::bind()")

    // Protect binding.
    std::scoped_lock lock(m_bound_mutex);

    // Check if socket is already bound.
    if (m_sfd != INVALID_SOCKET) {
        SSockaddr saddr;
        this->sockaddr(saddr);
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG1137") "Failed toa bind socket to an "
                                         "address. Socket fd " +
            std::to_string(m_sfd) + " already bound to netaddress \"" +
            saddr.netaddrp() + '\"');
    }

    // If no socket address is given then point to a valid one, either to the
    // loopback address or to the unspecified address when a passive address is
    // requested.
    SSockaddr unspec_saddr;
    if (a_saddr == nullptr) {
        if (!(a_flags & AI_PASSIVE)) {
            unspec_saddr = "[::1]";
        }
        a_saddr = &unspec_saddr;
    }

    // Get an adress info to bind.
    CAddrinfo ai(a_saddr->netaddrp(), AI_NUMERICHOST | AI_NUMERICSERV | a_flags,
                 a_socktype);
    if (!ai.get_first()) {
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG1037") "detect error next line ...\n" +
            ai.what());
    }
    CSocketErr serrObj;


    // Get a socket file descriptor from operating system and try to bind it.
    // ----------------------------------------------------------------------
    SOCKET sockfd{INVALID_SOCKET};
    // Get a socket file descriptor with address family from the address info.
    sockfd =
        get_sockfd(static_cast<sa_family_t>(ai->ai_family), ai->ai_socktype);

    // Try to bind the socket.
    int ret_code{SOCKET_ERROR};
    int count{1};
    ret_code = umock::sys_socket_h.bind(sockfd, ai->ai_addr,
                                        static_cast<socklen_t>(ai->ai_addrlen));
    if (ret_code == 0)
        // Store valid socket file descriptor.
        m_sfd = sockfd;
    else
        serrObj.catch_error();

    if (g_dbug) {
        SSockaddr saddr;
        this->sockaddr(saddr); // Get new bound socket address.
        UPnPsdk_LOGINFO("MSG1115") "syscall ::bind("
            << sockfd << ", " << &a_saddr->sa << ", " << a_saddr->sizeof_saddr()
            << ") Tried " << count << " times \"" << a_saddr->netaddrp()
            << (ret_code != 0 ? ". Get ERROR"
                              : ". Bound to " + saddr.netaddrp())
            << ".\"\n";
    }

    if (ret_code == SOCKET_ERROR) {
        CLOSE_SOCKET_P(sockfd);
        SSockaddr saddr;
        saddr = *reinterpret_cast<sockaddr_storage*>(ai->ai_addr);
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG1008") "Close socket fd " +
            std::to_string(sockfd) + ". Failed to bind socket to address=\"" +
            saddr.netaddrp() + "\": (errid " + std::to_string(serrObj) + ") " +
            serrObj.error_str() + '\n');
    }
}

// Set socket to listen
void CSocket::listen() {
    TRACE2(this, " Executing CSocket::listen()")

    // Protect set listen and storing its state (m_listen).
    std::scoped_lock lock(m_listen_mutex);

    if (m_listen)
        return;

    CSocketErr serrObj;
    // Second argument backlog (maximum length of the queue for pending
    // connections) is hard coded for now.
    if (umock::sys_socket_h.listen(m_sfd, SOMAXCONN) != 0) {
        serrObj.catch_error();
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG1034") "Failed to set socket to listen: " +
            serrObj.error_str() + '\n');
    }
    UPnPsdk_LOGINFO("MSG1032") "syscall ::listen(" << m_sfd << ", " << SOMAXCONN
                                                   << ").\n";
    m_listen = true;
}

// Getter
// ------
bool CSocket::is_listen() const {
    TRACE2(this, " Executing CSocket::is_listen()")
    if (m_sfd == INVALID_SOCKET)
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG1035") "Failed to get socket option "
                                         "'is_Listen': Bad file descriptor.\n");

    // m_listen is protected.
    std::scoped_lock lock(m_listen_mutex);
    return m_listen;
}


// Portable handling of socket errors
// ==================================
CSocketErr::CSocketErr() = default;

CSocketErr::~CSocketErr() = default;

CSocketErr::operator const int&() {
    // TRACE not usable with chained output.
    // TRACE2(this, " Executing CSocketErr::operator int&() (get socket error
    // number)")
    return m_errno;
}

void CSocketErr::catch_error() {
#ifdef _MSC_VER
    m_errno = umock::winsock2_h.WSAGetLastError();
#else
    m_errno = errno;
#endif
    TRACE2(this, " Executing CSocketErr::catch_error()")
}

std::string CSocketErr::error_str() const {
    // TRACE not usable with chained output, e.g.
    // std::cerr << "Error: " << sockerrObj.error_str();
    // TRACE2(this, " Executing CSocketErr::error_str()")

    // Portable C++ statement
    return std::system_category().message(m_errno);
    // return std::generic_category().message(m_errno);
    // return std::strerror(m_errno); // Alternative for Unix platforms
}

} // namespace UPnPsdk
