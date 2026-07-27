// Copyright (C) 2026+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-07-21
/*!
 * \file
 * \brief Manage information from Unix like platforms about internet addresses.
 */

#include <UPnPsdk/addrinfo2.hpp>
#include <UPnPsdk/synclog.hpp>

#include <umock/netdb.hpp>

/// \cond
#include <sstream> // Needed on macOS and win32
#if defined(_MSC_VER) || defined(__APPLE__)
#include <UPnPsdk/netadapter.hpp>
#else
#include <net/if.h>
#endif
/// \endcond


namespace UPnPsdk {

// Constructor for getting an address information from only a netaddress.
// ----------------------------------------------------------------------
CAddrinfo2::CAddrinfo2(std::string_view a_node, //
                       const int a_flags, //
                       const int a_socktype)
    : m_node(a_node), m_service(a_node == "" ? "0" : "") {
    // I cannot use the initialization list of the constructor because the
    // member order in the structure addrinfo is different on Linux, MacOS and
    // win32. I have to use the member names to initialize them, what's not
    // possible for structures in the constructors initialization list.
    m_hints.ai_socktype = a_socktype;
    // Due to specification IPv4 addresses are always mapped to IPv6.
    m_hints.ai_family = AF_INET6;
    m_hints.ai_flags = AI_V4MAPPED | a_flags;
#ifdef _MSC_VER
    // In contrast to other platforms ::getaddrinfo() on win32 does not create
    // AI_V4MAPPED addresses with AI_NUMERICHOST set. The SDK only uses IPv6
    // addresses. All IPv4 addresses are mapped to IPv6. There is only one
    // combination with AF_INET6 and no AI_NUMERICHOST where win32 do
    // AI_V4MAPPED. All others fail. See Unit Test 'GetaddrinfoWin32Test'.
    // if (!inaddr.node.empty() &&
    //     !std::isalpha(inaddr.node.front())) // No alpha-num name
    m_hints.ai_flags = m_hints.ai_flags & ~AI_NUMERICHOST; // Reset flag.
#endif
}


// Destructor
// ----------
/// \cond
CAddrinfo2::~CAddrinfo2() { this->free_addrinfo(); }
/// \endcond

// Private method to free allocated memory for address information
// ---------------------------------------------------------------
void CAddrinfo2::free_addrinfo() noexcept {
    if (m_res != &m_hints) {
        UPnPsdk_LOGINFO("MSG1112") "syscall ::freeaddrinfo(" << m_res << ").\n";
        umock::netdb_h.freeaddrinfo(m_res);
        m_res = &m_hints;
        m_res_current = &m_hints;
    }
}


// Getter for the first entry of an address info from the operating system
// =======================================================================
// Get address information with cached hints.
//
int CAddrinfo2::get_first() {

    // Prepare input for ::getaddrinfo()
    // ---------------------------------
    if (m_hints.ai_socktype == 0) {
        UPnPsdk_LOGERR("MSG1063") "Socket type 0 is not supported.\n";
        return EAI_SOCKTYPE;
    }

    // Prepare input for ::getaddrinfo().
    // node, scope, port for ::getaddrinfo(), may be modified.
    SInaddr inaddr(m_node + (m_service.empty() ? "" : (":" + m_service)));

    if (to_port(inaddr.service) == 1) {
        // Valid number but out of scope 0..65535.
        UPnPsdk_LOGERR("MSG1128") "Port number " << inaddr.service
                                                 << " out of range 0..65535.\n";
        return EAI_SERVICE;
    }

#if defined(_MSC_VER) || defined(__APPLE__)
    // Using my own code. That works.
    // I need this precheck of an unknown net interface name because Win32
    // ::getaddrinfo() only accepts numeric scope_ids and I have to convert
    // netinterface names into its index number (scope_id). ::if_nametoindex()
    // and ::ConvertInterfaceNameToLuidA() with ::ConvertInterfaceLuidToIndex()
    // does not work on Win32. I always get system "Error 123" that means "The
    // filename, directory name, or volume label syntax is incorrect", no
    // matter what I tried.
    //
    // MacOS does not fail ::getaddrinfo() with an unknown netinterface name
    // and instead ignores it and returns an lla addrinfo structure without
    // scope_id. But that is not specified. ::if_nametoindex() on macOS works
    // but I will not use two different system calls.
    //
    // Linux platforms ::getaddrinfo() accept netinterface names but fails if
    // they don't exist. That is what the SDK specifies. Tnere is no need to
    // waste resources with this precheck.
    if (!is_unum_str(inaddr.scope, 10)) {
        CNetadapter naObj;
        try {
            naObj.get_first();
        } catch (const std::exception& ex) {
            UPnPsdk_LOGERR("MSG1095") << ex.what() << "\n";
            return EAI_MEMORY;
        }
        if (!naObj.find_first(inaddr.scope)) {
            UPnPsdk_LOGERR("MSG1116") "Local network interface name=\""
                << inaddr.scope << "\" not found.\n";
            return EAI_NONAME;
        }
        // Note for later use: 'inaddr.scope' is modified.
        inaddr.scope = std::to_string(naObj.index());
    }
#endif

    if (!inaddr.scope.empty() && inaddr.scope != "0")
        // Note for later use: 'inaddr.node' is modified.
        inaddr.node.append("%").append(inaddr.scope);

#ifdef _MSC_VER
    // Microsoft Windows: I love it!
    // It does not resolve the unknown IPv4 address "0.0.0.0". I have to
    // workaround it with a binary similar address to get a list entry in the
    // result of ::getaddrinfo(). When having the entry I modify it to be the
    // unknown IPv4 address. For doing that after calling ::getaddrinf0() I
    // need the flag.
    bool unknown_ip4{false}; // Flag to modify address after call getaddrinfo().
    if (inaddr.node == "0.0.0.0") {
        // Note for later use: 'inaddr.node' is modified.
        inaddr.node = "127.0.0.0";
        unknown_ip4 = true;
    }
#endif

    // syscall ::getaddrinfo() with prepared arguments
    // -----------------------------------------------
    ::addrinfo* new_res{nullptr}; // Result from UPnPsdk::getaddrinfo()

    const int ret = umock::netdb_h.getaddrinfo(
        inaddr.node.empty() ? nullptr : inaddr.node.c_str(),
        inaddr.service.empty() ? nullptr : inaddr.service.c_str(), &m_hints,
        &new_res);

    if (g_dbug) {
        // Very helpful for debugging to see what is given to ::getaddrinfo()
        // clang-format off
        std::ostringstream oss;
        oss << "syscall ::getaddrinfo("
            << (inaddr.node.empty() ? "nullptr, " : "\"" + inaddr.node + "\", ")
            << (inaddr.service.empty() ? "nullptr, " : "\"" + inaddr.service + "\", ")
            << "/*hints*/" << &m_hints << ", /*res*/" << *&new_res << ") "
            << (m_hints.ai_flags & AI_V4MAPPED ? "AI_V4MAPPED, " : "")
            << (m_hints.ai_flags & AI_ALL ? "AI_ALL, " : "")
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
                            "socktype=" + std::to_string(m_hints.ai_socktype))));
        // clang-format on

        if (ret == 0) {
            SSockaddr saObj;
            saObj = *reinterpret_cast<sockaddr_storage*>(new_res->ai_addr);
            oss << ". Get first \"" << saObj.netaddrp()
                << (new_res->ai_next == nullptr ? "\", no more entries."
                                                : "\", more entries...");
            UPnPsdk_LOGINFO("MSG1111") << oss.str() << "\n";

        } else {
            oss << ". Get EAI_ERROR(" << ret << ")=\"" << ::gai_strerror(ret)
                << "\".";
            UPnPsdk_LOGERR("MSG1111") << oss.str() << "\n";
        }
    } // g_dbug

    if (ret != 0) {
        /*! \todo Manage to use WSAEAFNOSUPPORT for EAI_ADDRFAMILY that isn't
         * defined on win32. */
        // Error numbers definded in netdb.h.
        // Maybe an alphanumeric node name that cannot be resolved (e.g. by
        // DNS)? Anyway, the user has to decide what to do. Because this
        // depends on extern available DNS server the error can occur
        // unexpectedly at any time. We have no influence on it but I will give
        // an extended error message.
        return ret;
    }

    // Here we have a valid resoure response from ::getaddrinfo() that must be
    // freed.

#ifdef _MSC_VER
    if (unknown_ip4) {
        for (::addrinfo* ptr{new_res}; ptr != nullptr; ptr = ptr->ai_next) {
            // ::getaddrinfo() has returned "127.0.0.0" for
            // this flagged special case. It is easy to modify it to the unknown
            // IPv4 mapped address by resetting its byte containing 127.
            // Huh.. double pointer cast :-(
            sockaddr_in6* sin6 = reinterpret_cast<sockaddr_in6*>(ptr->ai_addr);
            uint32_t* sin6_32 = reinterpret_cast<uint32_t*>(&sin6->sin6_addr);
            if (sin6_32[0] == 0 && sin6_32[1] == 0 &&
                sin6_32[2] == htonl(0x0000ffff) && sin6_32[3] == 127) {
                sin6_32[3] = 0; // Clear bytes containing 127.
            }
        } // for
    }
#endif

    // Now I check on address binary level if it fulfills the common
    // specifications of the SDK, that is here: link-local address only with
    // scope_id, and other IPv6 addresses without scope_id. All entries from
    // the address info are checked.
    for (::addrinfo* ptr{new_res}; ptr != nullptr; ptr = ptr->ai_next) {
        if (ptr->ai_family == AF_INET6) {
            const auto& sin6 = reinterpret_cast<sockaddr_in6*>(ptr->ai_addr);
            if (IN6_IS_ADDR_LINKLOCAL2(&sin6->sin6_addr)) {
                if (sin6->sin6_scope_id == 0) {
                    umock::netdb_h.freeaddrinfo(new_res);
                    UPnPsdk_LOGERR("MSG1129") "Link-local address=\""
                        << m_node << "\" must have a valid scope_id.\n";
                    return EAI_NONAME;
                }
            } else {
                if (sin6->sin6_scope_id != 0) {
                    umock::netdb_h.freeaddrinfo(new_res);
                    UPnPsdk_LOGERR("MSG1132") "Address=\""
                        << m_node << "\" must not have a scope_id.\n";
                    return EAI_NONAME;
                }
            }
        }
    } // for

    // If get_first() is called the second time then m_res still points to
    // the previous allocated memory. To avoid a memory leak it must be
    // freed before pointing to the new allocated memory.
    this->free_addrinfo();
    // finaly point to the new address information from the operating
    // system.
    m_res = new_res;
    m_res_current = new_res;

    return 0;
}


// Reset the internal list pointer to the first entry
// --------------------------------------------------
void CAddrinfo2::set_first() noexcept { m_res_current = m_res; }


// Getter for the next available address information
// -------------------------------------------------
bool CAddrinfo2::get_next() noexcept {
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
void CAddrinfo2::sockaddr(SSockaddr& a_saddr) const noexcept {
    if (m_res == &m_hints)
        a_saddr.clear();
    else
        a_saddr = reinterpret_cast<sockaddr_storage&>(*m_res_current->ai_addr);
}

} // namespace UPnPsdk
