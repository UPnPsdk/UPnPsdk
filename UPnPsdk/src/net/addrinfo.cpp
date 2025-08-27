// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-09-05
/*!
 * \file
 * \brief Definition of the Addrinfo class and free helper functions.
 */

#include <UPnPsdk/addrinfo.hpp>
#include <UPnPsdk/synclog.hpp>
#include <UPnPsdk/sockaddr.hpp>

#include <umock/netdb.hpp>
/// cond
#include <cstring>
/// endcond


namespace UPnPsdk {
namespace {

/*!
 * \brief Wrapper to emulate missing functionality to be compatible between
 * different platforms
 */
int getaddrinfo(const std::string& a_node, const std::string& a_service,
                addrinfo a_hints, addrinfo*& a_res) {
    TRACE("Executing UPnPsdk::getaddrinfo()")
#ifdef _MSC_VER
    // Correct result for V4MAPPED unspecified address "0.0.0.0". On win32
    // getaddrinfo() fails with this conversion. To be compatible with other
    // platforms the conversion is emulated. Only little quirk is that the IPv4
    // mapped IPv6 is shown as "[::ffff:0:0] (hex format) instead of usual
    // "[::ffff:0.0.0.0]" (num base 10 format). But it shouldn't harm because
    // it is only a different view of the same binary address value.
    // getnameinfo() fails to show it with num base 10 values.
    if ((a_hints.ai_flags & AI_V4MAPPED) && a_hints.ai_family == AF_INET6) {
        if (a_node == "0.0.0.0") {
            addrinfo hints{}, *res{nullptr};
            hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | AI_PASSIVE;
            hints.ai_family = AF_INET6;
            hints.ai_socktype = a_hints.ai_socktype;
            hints.ai_protocol = a_hints.ai_protocol;
            int ret = umock::netdb_h.getaddrinfo(nullptr, a_service.c_str(),
                                                 &hints, &res);
            if (ret != 0)
                return ret;
            if (res->ai_next != nullptr)
                return EAI_NONAME; // "No such host is known. "

            in6_addr& sa6 =
                reinterpret_cast<sockaddr_in6*>(res->ai_addr)->sin6_addr;
            sa6.s6_addr[10] = 0xff; // prefix id for ipv4 mapping.
            sa6.s6_addr[11] = 0xff; //          ./.

            // Reset flag to have same result. See note below.
            a_hints.ai_flags = a_hints.ai_flags & ~AI_NUMERICHOST;

            a_res = res;
            return 0;
        }

        // In contrast to other platforms ::getaddrinfo() on win32 does not
        // create AI_V4MAPPED addresses with AI_NUMERICHOST set. This is to
        // workaround it. The SDK only uses IPv6 addresses. All IPv4 addresses
        // are mapped to IPv6. There is only one combination with AF_INET6 and
        // no AI_NUMERICHOST where win32 do AI_V4MAPPED. All others fail. See
        // Unit Test 'GetaddrinfoWin32Test how_it_works'.
        if (!a_node.empty() &&
            !std::isalpha(a_node.front())) // No alpha-num name
            a_hints.ai_flags =
                a_hints.ai_flags & ~AI_NUMERICHOST; // Reset flag.
    }
#endif

    return umock::netdb_h.getaddrinfo(
        (a_node.empty() ? nullptr : a_node.c_str()),
        (a_service.empty() ? nullptr : a_service.c_str()), &a_hints, &a_res);
}

} // anonymous namespace


// CAddrinfo class to encapsulate ::getaddrinfo() system calls
// ===========================================================
// On Microsoft Windows this needs to have Windows Sockets initialized with
// WSAStartup().

// Constructor for getting an address information with service name that can
// also be a port number string.
// -------------------------------------------------------------------------
CAddrinfo::CAddrinfo(std::string_view a_node, //
                     std::string_view a_service, //
                     const int a_flags, //
                     const int a_socktype)
    : m_node(a_node), m_service(a_service) {
    TRACE2(this, " Construct CAddrinfo() with extra service")
    // I cannot use the initialization list of the constructor because the
    // member order in the structure addrinfo is different on Linux, MacOS and
    // win32. I have to use the member names to initialize them, what's not
    // possible for structures in the constructors initialization list.
    m_hints.ai_socktype = a_socktype;
    // Due to specification IPv4 addresses are always mapped to IPv6.
    m_hints.ai_family = AF_INET6;
    m_hints.ai_flags = AI_V4MAPPED | a_flags;
}

// Constructor for getting an address information from only a netaddress.
// ----------------------------------------------------------------------
CAddrinfo::CAddrinfo(std::string_view a_node, //
                     const int a_flags, //
                     const int a_socktype)
    : m_node(a_node), m_service(a_node == "" ? "0" : "") {
    TRACE2(this, " Construct CAddrinfo() with netaddress")
    // I cannot use the initialization list of the constructor because the
    // member order in the structure addrinfo is different on Linux, MacOS and
    // win32. I have to use the member names to initialize them, what's not
    // possible for structures in the constructors initialization list.
    m_hints.ai_socktype = a_socktype;
    // Due to specification IPv4 addresses are always mapped to IPv6.
    m_hints.ai_family = AF_INET6;
    m_hints.ai_flags = AI_V4MAPPED | a_flags;
}


// Destructor
// ----------
/// \cond
CAddrinfo::~CAddrinfo() {
    TRACE2(this, " Destruct CAddrinfo()")
    this->free_addrinfo();
}
/// \endcond

// Private method to free allocated memory for address information
// ---------------------------------------------------------------
void CAddrinfo::free_addrinfo() noexcept {
    if (m_res != &m_hints) {
        TRACE2("syscall ::freeaddrinfo() with m_res = ", m_res)
        umock::netdb_h.freeaddrinfo(m_res);
        m_res = &m_hints;
        m_res_current = &m_hints;
    }
}


// Member access operator ->
// -------------------------
const ::addrinfo* CAddrinfo::operator->() const noexcept {
    return m_res_current;
}


// Getter for the first entry of an address info from the operating system
// -----------------------------------------------------------------------
// Get address information with cached hints.
bool CAddrinfo::get_first() {
    TRACE2(this, " Executing CAddrinfo::get_first()")

    // Prepare input for ::getaddrinfo()
    std::string node, service;
    try {
        if (m_service.empty())
            split_addr_port(m_node, node, service);
        else
            split_addr_port(m_node + ":" + m_service, node, service);
    } catch (const std::range_error& ex) {
        m_error_msg =
            UPnPsdk_LOGWHAT "MSG1128: catched next line ...\n" + ex.what();

        return false;
    }

    // syscall ::getaddrinfo() with prepared arguments
    // -----------------------------------------------
    ::addrinfo* new_res{nullptr}; // Result from ::getaddrinfo()
    const int ret = UPnPsdk::getaddrinfo(node, service, m_hints, new_res);
#ifndef __APPLE__
    // std::format() since c++20 isn't supported on AppleClang 15, even on c++23
    TRACE2(this, " syscall ::getaddrinfo(" + node + ", " + service +
                     ") with new_res = " +
                     std::format("{:#x}", reinterpret_cast<uintptr_t>(new_res)))
#endif
    if (g_dbug) {
        // Very helpful for debugging to see what is given to ::getaddrinfo()
        char addrStr[INET6_ADDRSTRLEN]{};
        char servStr[NI_MAXSERV]{};
        if (ret == 0)
            ::getnameinfo(new_res->ai_addr,
                          static_cast<socklen_t>(new_res->ai_addrlen), addrStr,
                          sizeof(addrStr), servStr, sizeof(servStr),
                          NI_NUMERICHOST | NI_NUMERICSERV);
        // clang-format off
        UPnPsdk_LOGINFO("MSG1111") "syscall ::getaddrinfo("
            << (node.empty() ? "nullptr, " : "\"" + node + "\", ")
            << (service.empty() ? "nullptr, " : "\"" + service + "\", ")
            << &m_hints << ", " << &new_res
            << ") node=\"" << m_node << "\", "
            << (m_hints.ai_flags & AI_V4MAPPED ? "AI_V4MAPPED, " : "")
            << (m_hints.ai_flags & AI_NUMERICHOST ? "AI_NUMERICHOST, " : "")
            << (m_hints.ai_flags & AI_NUMERICSERV ? "AI_NUMERICSERV, " : "")
            << (m_hints.ai_flags & AI_PASSIVE ? "AI_PASSIVE, " : "")
            << (m_hints.ai_family == AF_INET6 ? "AF_INET6" :
                    (m_hints.ai_family == AF_INET ? "AF_INET" :
                        (m_hints.ai_family == AF_UNSPEC ? "AF_UNSPEC" :
                            "m_hints.ai_family=" + std::to_string(m_hints.ai_family))))
            << ", "
            << (m_hints.ai_socktype == SOCK_STREAM ? "SOCK_STREAM" :
                    (m_hints.ai_socktype == SOCK_DGRAM ? "SOCK_DGRAM" :
                        (m_hints.ai_socktype == SOCK_RAW ? "SOCK_RAW" :
                            "socktype=" + std::to_string(m_hints.ai_socktype))))
            << (ret != 0
                ? ". Get EAI_ERROR(" + std::to_string(ret) + ")"
                : ". Get first \"" + std::string(addrStr) + "\" port "
                  + std::string(servStr)
                  + (new_res->ai_next == nullptr ? ", no more entries." : ", more entries..."))
            << '\n';
    }

    if (ret != 0) {
        /*! \todo Manage to use WSAEAFNOSUPPORT for EAI_ADDRFAMILY that isn't defined on win32. */
        // Error numbers definded in netdb.h.
        // Maybe an alphanumeric node name that cannot be resolved (e.g. by
        // DNS)? Anyway, the user has to decide what to do. Because this
        // depends on extern available DNS server the error can occur
        // unexpectedly at any time. We have no influence on it but I will give
        // an extended error message.
         m_error_msg = UPnPsdk_LOGWHAT + "MSG1112: errid(" +
             std::to_string(ret) + ")=\"" + ::gai_strerror(ret) + "\", " +
             ((m_hints.ai_family == AF_UNSPEC) ? "IPv?_" :
             ((m_hints.ai_family == AF_INET6) ? "IPv6_" : "IPv4_")) +
             ((m_hints.ai_flags & AI_NUMERICHOST) ? "numeric_host=\"" : "alphanum_name=\"") +
              m_node + "\", service=\"" +
              m_service + "\"" +
             ((m_hints.ai_flags & AI_PASSIVE) ? ", passive_listen" : "") +
             ((m_hints.ai_flags & AI_NUMERICHOST) ? "" : ", (maybe DNS query temporary failed?)") +
             ", ai_socktype=" + std::to_string(m_hints.ai_socktype);
        UPnPsdk_LOGERR("MSG1040") << m_error_msg << '\n';

        return false;
    }
    // clang-format on

    for (::addrinfo* res{new_res}; res != nullptr; res = res->ai_next) {
        // Different on platforms: Ubuntu & MacOS return protocol number, win32
        // returns 0. I just return what was used to call ::getaddrinfo().
        res->ai_protocol = m_hints.ai_protocol;
        //
        // Different on platforms: Ubuntu returns set flags, MacOS & win32
        // return 0. I just return what was used to call ::getaddrinfo().
        res->ai_flags = m_hints.ai_flags;
    }

    // If get_first() is called the second time then m_res still points to the
    // previous allocated memory. To avoid a memory leak it must be freed before
    // pointing to the new allocated memory.
    this->free_addrinfo();
    // finaly point to the new address information from the operating
    // system.
    m_res = new_res;
    m_res_current = new_res;

    return true;
}


// Getter for the next available address information
// -------------------------------------------------
bool CAddrinfo::get_next() noexcept {
    if (m_res_current->ai_next == nullptr) {
        // It doesn't matter if already pointing to m_hints. m_hints->ai_next is
        // also nullptr.
        m_res_current = &m_hints;
        return false;
    }
    m_res_current = m_res_current->ai_next;
    return true;
}


// Get the socket address from current selcted address information
// ---------------------------------------------------------------
void CAddrinfo::sockaddr(SSockaddr& a_saddr) {
    if (m_res == &m_hints)
        a_saddr = "";
    else
        a_saddr = reinterpret_cast<sockaddr_storage&>(*m_res_current->ai_addr);
}


// Get cached error message
// ------------------------
const std::string& CAddrinfo::what() const { return m_error_msg; }

} // namespace UPnPsdk
