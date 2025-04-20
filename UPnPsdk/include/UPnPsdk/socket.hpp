#ifndef UPnPsdk_SOCKET_HPP
#define UPnPsdk_SOCKET_HPP
// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-04-20
/*!
 * \file
 * \brief **Socket Module:** manage properties and methods but not connections
 * of ONE network socket to handle IPv4 and IPv6 streams and datagrams.
 */

/*!
 * \brief The socket module contains all classes and free functions to manage
 * network sockets.
 * \addtogroup upnplib-socket
 *
 * \anchor socket_module
 * This module mainly consists of the CSocket class but also provides free
 * functions to manage a socket. The problem is that socket handling isn't very
 * good portable. There is different behavior on the supported platforms Unix,
 * MacOS and Microsoft Windows. The CSocket class atempts to be consistent
 * portable on all three platforms by using common behavior or by emulating
 * missing functions on a platform.
 *
 * Specification for CSocket
 * =========================
 * The class encapsulates and manages one raw socket file descriptor. The file
 * descriptor of a valid socket object cannot be changed but the object with
 * its immutable file descriptor can be moved and assigned to another socket
 * object. Copying a socket object isn't supported because having two objects
 * with the same file descriptor may be very error prone in particular with
 * multithreading. Effort has been taken to do not cache any socket information
 * outside the socket file descriptor. Nearly all socket informations are
 * direct set and get to/from the operating system with the file descriptor.
 * The socket file descriptor is always valid except on an empty socket object.
 *
 * \anchor empty_socket
 * empty socket object
 * -------------------
 * An empty socket object can be instantiated with the default constructor,
 * e.g. `CSocket sockObj;`. It is a valid object and should be destructed. When
 * moving a socket object, the left over source object is also empty. An empty
 * socket object has an `INVALID_SOCKET` defined and no valid content. It
 * throws an exception if using most of its Setter and Getter. Moving and
 * assigning it is possible. You can test for an empty socket by looking for an
 * `INVALID_SOCKET`, e.g.
 * \code
 * CSocket sockObj; // or CSocket_basic sockObj;
 * if (static_cast<SOCKET>(sockObj) != INVALID_SOCKET) {
 *     in_port_t port = sockObj.get_port(); }
 * \endcode
 *
 * protocol family, address family
 * -------------------------------
 * This SDK handles network connections IP Version - Independent so that a
 * protocoll family (PF_INET6, PF_INET, PF_UNIX) or address family (AF_INET6,
 * AF_INET, AF_UNIX) is never used on the SDKs Application Programming
 * Interface. IP Version is managed by the operating system.
 *
 * socket type
 * -----------
 * Only `SOCK_STREAM` and `SOCK_DGRAM` is supported. Any other type throws an
 * exception.
 *
 * options SO_REUSEADDR and SO_EXCLUSIVEADDRUSE
 * --------------------------------------------
 * I don't set the option to immediately reuse an address and I always set the
 * option `SO_EXCLUSIVEADDRUSE` on Microsoft Windows. For more details of this
 * have a look at [Socket option "reuse address"](\ref overview_reuseaddr).
 *
 * References
 * ----------
 * - <!--REF:--><a href="https://www.rfc-editor.org/rfc/rfc3493">RFC 3493</a> -
 *   Basic Socket Interface Extensions for IPv6
 * - <!--REF:--><a href="https://www.rfc-editor.org/rfc/rfc3542">RFC 3542</a> -
 *   Advanced Sockets Application Program Interface (API) for IPv6
 */

#include <UPnPsdk/sockaddr.hpp>
#include <UPnPsdk/synclog.hpp>
/// \cond
#include <mutex>
#include <memory>

// To be portable with BSD socket error number constants I have to
// define and use these macros with appended 'P' for portable.
#ifdef _MSC_VER
#define EBADFP WSAENOTSOCK
#define ENOTCONNP WSAENOTCONN
#define EINTRP WSAEINTR
#define EFAULTP WSAEFAULT
#define ENOMEMP WSA_NOT_ENOUGH_MEMORY
#define EINVALP WSAEINVAL
#define EACCESP WSAEACCES
#define ENOBUFSP WSAENOBUFS
#else
#define EBADFP EBADF
#define ENOTCONNP ENOTCONN
#define EINTRP EINTR
#define EFAULTP EFAULT
#define ENOMEMP ENOMEM
#define EINVALP EINVAL
#define EACCESP EACCES
#define ENOBUFSP ENOBUFS
#endif
/// \endcond

namespace UPnPsdk {

/*!
 * \brief Get information from a raw network socket file descriptor
 * <!--   ========================================================= -->
 * \ingroup upnplibAPI-socket
 * \ingroup upnplib-socket
 *
 * For general information have a look at \ref socket_module.
 *
 * This class takes the resources and results as given by the platform (Unix,
 * MacOS, MS Windows). It does not perform any emulations for unification. The
 * behavior can be different on different platforms.
 *
 * An object of this class does not take ownership of the raw socket file
 * descriptor and will never close it. This is also the reason why you cannot
 * modify the socket and only have getter available (except the setter 'load()'
 * for the raw socket file descriptor itself). But it is helpful to easily get
 * information about an existing raw socket file descriptor. Closing the file
 * descriptor is in the responsibility of the caller who created the socket. If
 * you need to manage a socket you must use CSocket.
 */
class UPnPsdk_API CSocket_basic {
  public:
    /*! \brief Default constructor for an empty basic socket object with
     * invalid socket file descriptor */
    CSocket_basic();

    /*! \brief Constructor for the socket file descriptor. Before use, it must
     * be load(). */
    CSocket_basic(SOCKET a_sfd) /*!< [in] Socket file descriptor. */;

    /// \cond
    // I want to restrict to only move the resource.
    // No copy constructor
    CSocket_basic(const CSocket_basic&) = delete;
    // No copy assignment operator
    CSocket_basic& operator=(CSocket_basic) = delete;
    /// \endcond

    // Destructor
    virtual ~CSocket_basic();


    /*! \name Setter
     * *************
     * @{ */
    /*! \brief Load the raw socket file descriptor specified with the
     * constructor into the object
     * \note This function can only be used with the CSocket_basic class. A
     * derived usage e.g. with the CSocket class will throw an exception.
     *
     * \code
     * // Usage e.g.:
     * SOCKET sfd = ::socket(PF_INET6, SOCK_STREAM);
     * {   // Scope for sockObj, sfd must longer exist than sockObj
     *     CSocket_basic sockObj(sfd);
     *     try {
     *         sockObj.load();
     *     } catch(xcp) { handle_error(); };
     *     // Use the getter from sockObj
     * }
     * ::close(sfd);
     * \endcode
     * The socket file descriptor was given with the constructor. This object
     * does not take ownership of the socket file descriptor and will never
     * close it. Closing is in the responsibility of the caller who created the
     * socket. Initializing it again is possible but is only waste of
     * resources. The result is the same as before.
     *
     * \exception std::runtime_error Given socket file descriptor is invalid. */
    void load();
    /// @} Setter


    /*! \name Getter
     * *************
     * @{ */
    /*! \brief Get raw socket file descriptor.
     * \code
     * // Usage e.g.:
     * CSocket_basic sockObj(valid_socket_fd);
     * try {
     *     sockObj.load();
     * } catch(xcp) { handle_error(); };
     * SOCKET sfd = sockObj;
     * \endcode */
    operator const SOCKET&() const;


    /*! \brief Get the local socket address the socket is bound to
     * \code
// Usage e.g.:
SOCKET sfd = ::socket(PF_INET6, SOCK_STREAM, 0);
SSockaddr saObj;
CSocket_basic sockbObj(sfd);
CSocket sockObj;
try {
    sockbObj.load();
    if (sockbObj.local_saddr(&saObj))
        std::cout << "socket is bound to " << saObj << '\n';
    else
        std::cout << "unbound socket unspecified netaddr " << saObj << '\n';

    if (sockbObj.remote_saddr(&saObj))
        std::cout << "socket is connected to " << saObj << '\n';
    else
        std::cout << "unconnected socket unspecified netaddr " << saObj << '\n';
} catch(std::exception& ex) { handle_error(); };
close(sfd);

// CSocket inherit from CSocket_basic
CSocket sockObj;
try {
    sockObj.bind(SOCK_STREAM, nullptr, AI_PASSIVE);
    if (sockObj.local_saddr() == -1 && !sockObj.remote_saddr())
        std::cout << "socket unconnected and listening on incomming requests.";
    else
        std::cerr << "failing to bind socket passive.";
} catch(std::exception& ex) { handle_error(); };
     * \endcode
     *
     * \returns
     *  - \b -1 The socket is passive bound to listen on all local network
     *          adapter for incomming requests.
     *  - \b 0 The socket is not bound to any ip address.
     *  - \b 1 The socket is bound to an ip address ready to use for syscalls
     *         ::%connect(), ::%sendto(), or ::%sendmsg().
     * \exception std::runtime_error system <a
     * href="https://www.man7.org/linux/man-pages/man2/getsockname.2.html#ERRORS">errors
     * as specified</a>. */
    int local_saddr(
        /*! [out] Optional: pointer to a socket address object that will be
         * filled with the address information of a local network adapter that
         * the socket is bound to. If an error is thrown, this socket address
         * object is not modified. If no information is available (the socket
         * is not bound to a local network adapter) an unspecified socket
         * address (":0") is returned, possibly with address family ("[::]:0"
         * or 0.0.0.0:0"). But working with IP-version is not intended. */
        SSockaddr* a_saddr = nullptr) const;


    /*! \brief Get the remote socket address the socket is connected to
     *
     * For an example look at CSocket_basic::local_saddr().
     * \returns
     *  \b true&nbsp; if socket is connected\n
     *  \b false otherwise
     * \exception std::runtime_error system <a
     * href="https://www.man7.org/linux/man-pages/man2/getpeername.2.html#ERRORS">errors
     * as specified</a> except "The socket is not connected". */
    bool remote_saddr(
        /*! [out] Optional: pointer to a socket address object that will be
         * filled with the remote address information. If an error is thrown,
         * this socket address object is not modified. If no information is
         * available (the socket is not connected to a remote peer) an
         * unspecified socket address (":0") is returned, possibly with address
         * family ("[::]:0" or 0.0.0.0:0"). But working with IP-version is not
         * intended.*/
        SSockaddr* a_saddr = nullptr) const;


    /*! \brief Get the [socket type](\ref glossary_socktype).
     * \returns `SOCK_STREAM` or `SOCK_DGRAM`.
     * \exception std::runtime_error if query option fails. */
    int socktype() const;

    /*! \brief Get the error that is given from the socket as option
     *
     * This is not a system error from the operating system (with POSIX
     * returned in \b errno). It is the error that can be queried as option
     * from the socket.
     * \returns error number.
     * \exception std::runtime_error if query option fails. */
    int sockerr() const;

    /*! \brief Get status if reusing address is enabled.
     *
     * For details to this option have a look at
     * [option "reuse address"](\ref overview_reuseaddr).
     * \returns
     *  \b true&nbsp; if reuse address is enabled\n
     *  \b false otherwise
     * \exception std::runtime_error if query option fails. */
    bool is_reuse_addr() const;
    /// @} Getter

  protected:
    /// \cond
    // This is the raw socket file descriptor
    SOCKET m_sfd{INVALID_SOCKET};

    // Mutex to protect concurrent binding a socket.
    SUPPRESS_MSVC_WARN_4251_NEXT_LINE
    mutable std::mutex m_bound_mutex;
    /// \endcond

  private:
    // Hint from the constructor what socket file descriptor to use.
    const SOCKET m_sfd_hint{INVALID_SOCKET};
};


/*!
 * \brief Manage all aspects of a network socket.
 * \ingroup upnplibAPI-socket
 * \ingroup upnplib-socket
 *
 * For general information have a look at \ref socket_module.
 ********************************************************* */
class UPnPsdk_API CSocket : public CSocket_basic {
  public:
    /*! \brief Default constructor for an
     * [empty socket object](\ref empty_socket) */
    CSocket();

    /*! \brief Move constructor
     *
     * This moves the socket object to a new instantiated socket object and
     * also transfers ownership to the new object. That means the destination
     * also manage and frees its resources now. After moving, the source object
     * is still valid but empty with an INVALID_SOCKET. Using its methods will
     * throw exceptions. But you can assign (operator=()) another socket object
     * to it again.
     * \code
     * // Usage e.g.:
     * CSocket sock1Obj;
     * try {
     *     sock1Obj.bind(SOCK_STREAM);
     * } catch(xcp) { // handle error }
     * CSocket sock2Obj{std::move(sock1Obj)};
     * \endcode */
    CSocket(CSocket&&);

    /*! \brief Assignment operator
     *
     * <!-- With parameter as value this is used as copy- and move-assignment
     * operator. The correct usage (move) is evaluated by the compiler. Here
     * only the move constructor can be used. -->
     * This moves the socket object to another already existing socket object
     * and also transfers ownership to it. That means the destination object
     * also manage and frees its resources now. After moving, the source object
     * is still valid but empty with an INVALID_SOCKET. Using its methods will
     * throw exceptions.
     * \code
     * // Usage e.g.:
     * CSocket sock1Obj;
     * try {
     *     sock1Obj.bind(SOCK_STREAM);
     * } catch(xcp) { // handle error }
     * CSocket sock2Obj;
     * sock2Obj = std::move(sock1Obj);
     * \endcode */
    CSocket& operator=(CSocket);

    /// \brief Destructor
    virtual ~CSocket();

    /*! \name Setter
     * *************
     * @{ */
    /*! \brief Bind socket to an ip address of a local network adapter.
     * <!-- ------------------------------------------------------- -->
     * \details Usage e.g. to bind with default settings to an IP address of a
     * local network adapter for use with **connect**, **sendto**, or
     * **sendmsg** to send requests (typically control points). The address is
     * selected by the operating system and considered to be the best choise.
     * \code
     * CSocket sock1Obj;
     * try {
     *     sock1Obj.bind(SOCK_STREAM);
     * } catch(std::exception& ex) { handle_error(); }
     * \endcode
     *
     * Usage e.g. to bind with default settings for listening on local network
     * adapters for incomming requests (typically UPnP devices).
     * \code
     * CSocket sock2Obj;
     * try {
     *     sock2Obj.bind(SOCK_STREAM, nullptr, AI_PASSIVE);
     * } catch(std::exception& ex) { handle_error(); }
     * \endcode
     *
     * Usage e.g. to bind for listening on the link local address of a local
     * network adapter.
     * \code
     * SSockaddr saddr3;
     * saddr3 = "[fe80::fedc:cdef:0:1]";
     * CSocket sock3Obj;
     * try {
     *     sock3Obj.bind(SOCK_STREAM, &saddr3, AI_PASSIVE);
     * } catch(std::exception& ex) { handle_error(); }
     * \endcode
     *
     * Usage e.g. to bind to a global unicast address for use with **connect**,
     * **sendto**, or **sendmsg**.
     * \code
     * SSockaddr saddr4;
     * saddr4 = "[2001:db8::abc]:50001";
     * CSocket sock4Obj;
     * try {
     *     sock4Obj.bind(SOCK_STREAM, &saddr4);
     * } catch(std::exception& ex) { handle_error(); }
     * \endcode
     *
     * Usage e.g. to bind to "localhost", resp. to one of the loopback
     * addresses of best choise.
     * \code
     * SSockaddr saddr5; // Unspecified address selects a loopback address.
     * CSocket sock5Obj;
     * try {
     *     sock5Obj.bind(SOCK_STREAM, &saddr5);
     * } catch(std::exception& ex) { handle_error(); }
     * \endcode
     *
     * You can also use "localhost" with CAddrinfo, but that allocates memory
     * and do a DNS lookup. You should prefer the previous example.
     * \code
     * SSockaddr saddr6;
     * CSocket sock6Obj;
     * CAddrinfo ai("localhost");
     * try {
     *     ai.get_first();
     *     ai.local_saddr(&saddr6);
     *     sock6Obj.bind(SOCK_STREAM, &saddr6);
     * } catch(std::exception& ex) { handle_error(); }
     * \endcode
     *
     * This method uses internally the system function <a
     * href="https://www.man7.org/linux/man-pages/man3/getaddrinfo.3.html">::%getaddrinfo()</a>
     * to provide possible socket addresses. If the AI_PASSIVE flag is
     * specified with **a_flags**, and **a_saddr** is an unspecified socket
     * address (netaddress ":0") , then the selected local socket addresses
     * will be suitable for **binding** a socket that will **accept**
     * connections. The selected local socket address will contain the
     * "wildcard address" (INADDR_ANY for IPv4 addresses, IN6ADDR_ANY_INIT for
     * IPv6 address). The wildcard address is used by applications (typically
     * UPnP devices) that intend to accept connections on any of the host's
     * network addresses. If **a_saddr** is specified, then the AI_PASSIVE flag
     * is ignored.
     *
     * If the AI_PASSIVE flag is not set, then the selected local socket
     * addresses will be suitable for use with **connect**, **sendto**, or
     * **sendmsg** (typically control points).
     *
     * I internally always set IPV6_V6ONLY to false to use IPv6 mapped IPv4
     * addresses. This is default on Unix platforms when binding the address
     * and cannot be modified after binding. MacOS does not modify IPV6_V6ONLY
     * with binding. On Microsoft Windows IPV6_V6ONLY is set by default. */
    void bind(
        /*! [in] This property must always be specified with SOCK_STREAM, or
         * SOCK_DGRAM. */
        const int a_socktype,
        /*! [in] Optional: Pointer to a socket address the socket shall be bound
         * to. */
        const SSockaddr* const a_saddr = nullptr,
        /*! [in] Optional: this field specifies additional options, as
         * described at <a
         * href="https://www.man7.org/linux/man-pages/man3/getaddrinfo.3.html#DESCRIPTION">getaddrinfo(3)
         * — Linux manual page</a> or at <a
         * href="https://learn.microsoft.com/en-us/windows/desktop/api/ws2def/ns-ws2def-addrinfoa">Microsoft
         * Build — addrinfo structure</a>. Multiple flags are specified by
         * bitwise OR-ing them together. Example is: `AI_PASSIVE |
         * AI_NUMERICHOST | AI_NUMERICSERV` */
        const int a_flags = 0);

    /*! \brief Set socket to listen
     *
     * On Linux there is a socket option SO_ACCEPTCONN that can be get with
     * system function ::%getsockopt(). This option shows if the socket is set
     * to passive listen. But it is not portable. MacOS does not support it. So
     * this flag has to be managed here. Look for details at <!--REF:--><a
     * href="https://stackoverflow.com/q/75942911/5014688">How to get option on
     * MacOS if a socket is set to listen?</a> */
    void listen();
    /// @} Setter


    /*! \name Getter
     * *************
     * @{ */
    /*! \brief Get status if the socket is listen to incomming network packets.
     * \returns
     *  - \b true&nbsp; Socket is listen to incomming network packets
     *  - \b false otherwise.
     * \exception std::runtime_error Failed to get socket option from unbind
     *             socket. */
    bool is_listen() const;
    /// @} Getter

  private:
    /// \brief Mutex to protect concurrent listen a socket.
    SUPPRESS_MSVC_WARN_4251_NEXT_LINE
    mutable std::mutex m_listen_mutex;
    bool m_listen{false}; // Protected by a mutex.
};


// Portable handling of socket errors
// ==================================
/*! \brief Class for portable handling of network socket errors.
 * \ingroup upnplibAPI-socket
 * \ingroup upnplib-socket
 * \code
 * // Usage e.g.:
 * CSocketErr serrObj;
 * int ret = some_function_1();
 * if (ret != 0) {
 *     serrObj.catch_error();
 *     int errid = serrObj;
 *     std::cout << "Error " << errid << ": "
 *               << serrObj.error_str() << "\n";
 * }
 * ret = some_function_2();
 * if (ret != 0) {
 *     serrObj.catch_error();
 *     std::cout << "Error " << static_cast<int>(serrObj) << ": "
 *               << serrObj.error_str() << "\n";
 * }
 * \endcode
 *
 * There is a compatibility problem with Winsock2 on the Microsoft Windows
 * platform that does not support detailed error information given in the global
 * variable 'errno' that is used by POSIX. Instead it returns them with calling
 * 'WSAGetLastError()'. This class encapsulates differences so there is no need
 * to always check the platform to get the error information.
 *
 * This class is optimized for frequent short-term use. It's a simple class
 * without inheritence and virtual methods.
 */
class UPnPsdk_API CSocketErr {
  public:
    CSocketErr();
    ~CSocketErr();
    // Delete copy constructor
    CSocketErr(const CSocketErr&) = delete;
    // Delete assignment operator
    CSocketErr& operator=(const CSocketErr&) = delete;
    /// Get error number.
    operator const int&();
    /// Catch error for later use.
    void catch_error();
    /// Get human readable error description of the catched error.
    std::string error_str() const;

  private:
    int m_errno{}; // Cached error number
};

} // namespace UPnPsdk

#endif // UPnPsdk_SOCKET_HPP
