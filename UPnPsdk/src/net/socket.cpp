// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-04-22
/*!
 * \file
 * \brief Definition of the 'class Socket'.
 */

#include <UPnPsdk/socket.hpp>

#include <UPnPsdk/addrinfo.hpp>
#include <UPnPsdk/netadapter.hpp>
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
bool CSocket_basic::local_saddr(SSockaddr* a_saddr) const {
    TRACE2(this, " Executing CSocket_basic::local_saddr()")
    if (m_sfd == INVALID_SOCKET) {
        if (a_saddr)
            *a_saddr = "";
        return false;
    }

    // Get local address from socket file descriptor.
    SSockaddr saObj;
    socklen_t addrlen = sizeof(saObj.ss); // May be modified
    CSocketErr serrObj;
    // syscall
    int ret = UPnPsdk::getsockname(m_sfd, &saObj.sa, &addrlen);
    if (ret != 0) {
        serrObj.catch_error();
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG1001") "Failed to get address from socket(" +
            std::to_string(m_sfd) + "): " + serrObj.error_str());
    }
    sa_family_t af = saObj.ss.ss_family;
    if (af != AF_INET6 && af != AF_INET)
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG1091") "Unsupported address family " +
            std::to_string(saObj.ss.ss_family));

    // Check if there is a complete address structure returned from
    // UPnPsdk::getsockname(). On macOS the function returns only part of the
    // address structure if the socket file descriptor isn't bound to an
    // address of a local network adapter. It trunkates the address part.
    if (!(af == AF_INET6 && addrlen == sizeof(saObj.sin6)) &&
        !(af == AF_INET && addrlen == sizeof(saObj.sin))) {
        // If there is no complete address structure returned from
        // UPnPsdk::getsockname() but no error reported, it is considered to be
        // unbound. I return here an empty socket address with preserved
        // address family.
        if (a_saddr) {
            *a_saddr = "";
            a_saddr->ss.ss_family = af;
        }
        return false;
    }

    if (a_saddr)
        *a_saddr = saObj;
    return true;
}

bool CSocket_basic::remote_saddr(SSockaddr* a_saddr) const {
    TRACE2(this, " Executing CSocket_basic::remote_saddr()")
    if (m_sfd == INVALID_SOCKET) {
        if (a_saddr)
            *a_saddr = "";
        return false;
    }

    // Get remote address from socket file descriptor.
    SSockaddr saObj;
    socklen_t addrlen = sizeof(saObj.ss); // May be modified
    CSocketErr serrObj;
    // syscall
    int ret = umock::sys_socket_h.getpeername(m_sfd, &saObj.sa, &addrlen);
    if (ret != 0) {
        serrObj.catch_error();
        if (serrObj == ENOTCONNP) {
            // Return with empty socket address if not connected.
            if (a_saddr)
                *a_saddr = "";
            return false;
        }
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG1044") "Failed to get address from socket(" +
            std::to_string(m_sfd) + "): " + serrObj.error_str());
    }

    // Check if there is a complete address structure returned from
    // ::getpeername(). On macOS the function returns only part of the address
    // structure if the socket file descriptor isn't connected to an address.
    // It trunkates the address part.
    sa_family_t af = saObj.ss.ss_family;
    if ((af == AF_INET6 && addrlen == sizeof(saObj.sin6)) ||
        (af == AF_INET && addrlen == sizeof(saObj.sin))) {
        if (a_saddr)
            *a_saddr = saObj;
        return true;
    }
    // If there is no complete address structure returned from ::getpeername()
    // I do not modify the result object *a_saddr.
    throw std::invalid_argument(
        UPnPsdk_LOGEXCEPT(
            "MSG1088") "Unknown socket address detected with address family " +
        std::to_string(saObj.ss.ss_family));
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
void CSocket::bind(const int a_socktype, const SSockaddr* const a_saddr,
                   const int a_flags) {
    TRACE2(this, " Executing CSocket::bind()")

    // Protect binding.
    std::scoped_lock lock(m_bound_mutex);

    // Check if socket is already bound.
    if (m_sfd != INVALID_SOCKET) {
        SSockaddr saObj;
        this->local_saddr(&saObj);
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG1137") "Failed toa bind socket to an "
                                         "address. Socket fd " +
            std::to_string(m_sfd) + " already bound to netaddress \"" +
            saObj.netaddrp() + '\"');
    }

    // If no socket address is given then get a valid one, either the
    // unspecified address when a passive address is requested, or the best
    // choise from the operating system.
    SSockaddr saddr; // Unspecified
    if (a_saddr == nullptr) {
        if (!(a_flags & AI_PASSIVE)) {
            // Get best choise sockaddr from operating system.
            CNetadapter nadapObj;
            nadapObj.get_first(); // May throw exception.
            if (!nadapObj.find_first())
                throw std::runtime_error(
                    UPnPsdk_LOGEXCEPT("MSG1037") "No usable link local or "
                                                 "global ip address found. Try "
                                                 "to use \"loopback\".\n");
            nadapObj.sockaddr(saddr);
        }

    } else {
        saddr = *a_saddr;
    }

    // Get the address info for binding.
    CAddrinfo ai(saddr.netaddrp(), AI_NUMERICHOST | AI_NUMERICSERV | a_flags,
                 a_socktype);
    if (!ai.get_first())
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG1092") "detect error next line ...\n" +
            ai.what());


    // Get a socket file descriptor from operating system and try to bind it.
    // ----------------------------------------------------------------------
    // Get a socket file descriptor.
    SOCKET sockfd =
        get_sockfd(static_cast<sa_family_t>(ai->ai_family), ai->ai_socktype);

    // Try to bind the socket.
    int ret_code{SOCKET_ERROR};
    int count{1};
    ret_code = umock::sys_socket_h.bind(sockfd, ai->ai_addr,
                                        static_cast<socklen_t>(ai->ai_addrlen));
    CSocketErr serrObj;
    if (ret_code == 0)
        // Store valid socket file descriptor.
        m_sfd = sockfd;
    else
        serrObj.catch_error();

    if (g_dbug) {
        SSockaddr saObj;
        this->local_saddr(&saObj); // Get new bound socket address.
        UPnPsdk_LOGINFO("MSG1115") "syscall ::bind("
            << sockfd << ", " << &saObj.sa << ", " << saObj.sizeof_saddr()
            << ") Tried " << count << " times \"" << saddr.netaddrp()
            << (ret_code != 0 ? "\". Get ERROR"
                              : "\". Bound to \"" + saObj.netaddrp())
            << "\".\n";
    }

    if (ret_code == SOCKET_ERROR) {
        CLOSE_SOCKET_P(sockfd);
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
