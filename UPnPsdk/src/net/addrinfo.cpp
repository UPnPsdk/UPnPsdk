// Copyright (C) 2026+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-08-10
/*!
 * \file
 * \brief Manage information about internet addresses
 */

#include <UPnPsdk/addrinfo.hpp>
#include <UPnPsdk/global.hpp>
#include <UPnPsdk/synclog.hpp>

#include <umock/netdb.hpp>

/// \cond
#include <sstream>
/// \endcond


namespace UPnPsdk {

// Constructor for getting an address information from only a netaddress.
// ----------------------------------------------------------------------
CAddrinfo::CAddrinfo(const int a_flags, const int a_socktype) noexcept {
    // I cannot use the initialization list of the constructor because the
    // member order in the structure addrinfo is different on Linux, MacOS and
    // win32. I have to use the member names to initialize them, what's not
    // possible for structures in the constructors initialization list.
    m_hints.ai_socktype = a_socktype;
    // Due to specification IPv4 addresses are always mapped to IPv6.
    m_hints.ai_family = AF_INET6;
    m_hints.ai_flags = AI_V4MAPPED;
    m_hints.ai_flags = AI_V4MAPPED | a_flags;
    // In contrast to other platforms ::getaddrinfo() on win32 does not create
    // AI_V4MAPPED addresses with AI_NUMERICHOST set. UPnPsdk only uses IPv6
    // addresses. All IPv4 addresses are mapped to IPv6. There is only one
    // combination with AF_INET6 and no AI_NUMERICHOST where win32 do
    // AI_V4MAPPED. All others fail. To be portable, UPnPsdk ignores
    // AI_NUMERICHOST on all platforms. For numeric only internet addresses you
    // can use SSockaddr.
    m_hints.ai_flags &= ~AI_NUMERICHOST; // Reset flag.
}

// Destructor
// ----------
/// \cond
CAddrinfo::~CAddrinfo() noexcept {
    if (m_res != nullptr) {
        UPnPsdk_LOGINFO("MSG1112") "syscall ::freeaddrinfo(" << m_res << ").\n";
        umock::netdb_h.freeaddrinfo(m_res);
        m_res = nullptr;
        m_res_current = nullptr;
    }
}
/// \endcond


// Getter for the first entry of an address info from the operating system
// =======================================================================
// Get address information with cached hints.
//
int CAddrinfo::get_first(SInaddr a_inaddr) {
    if (m_res != nullptr)
        throw std::runtime_error(UPnPsdk_LOGEXCEPT(
            "MSG1189") "Object has already got address info. "
                       "Cannot get_first(inaddr) a second time.");

    // If only the port is given then don't check node. Instead let
    // ::getaddrinfo() handle special case to return "[::1]" (active), or
    // "[::]" with AI_PASSIVE.
    if (!(a_inaddr.node.empty() && a_inaddr.scope.empty() &&
          !a_inaddr.service.empty())) //
    {
        // Doing a numeric precheck.
        SSockaddr saObj;
        saObj = a_inaddr;
        if (saObj.family == AF_UNSPEC) {
            UPnPsdk_LOGERR("MSG1190") "CAddrinfo fails with node=\""
                << a_inaddr.node << "\", scope=\"" << a_inaddr.scope
                << "\", service=\"" << a_inaddr.service << "\".\n";
            return EAI_NONAME;
        }
    }

    // Concatenate node and scope_id that needs ::getaddrinfo().
    if (!a_inaddr.scope.empty()) {
        // Note for later use: 'inaddr.node' is modified.
        a_inaddr.node.reserve(a_inaddr.node.size() + 1 + a_inaddr.scope.size());
        a_inaddr.node.append("%").append(a_inaddr.scope);
    }

    // syscall ::getaddrinfo() with prepared arguments
    // -----------------------------------------------
    ::addrinfo* new_res{nullptr}; // Result from UPnPsdk::getaddrinfo()

    int ret = umock::netdb_h.getaddrinfo(
        a_inaddr.node.empty() ? nullptr : a_inaddr.node.c_str(),
        a_inaddr.service.empty() ? nullptr : a_inaddr.service.c_str(), &m_hints,
        &new_res);

    if (g_dbug) {
        // Very helpful for debugging to see what is given to ::getaddrinfo()
        // clang-format off
        std::ostringstream oss;
        oss << "syscall ::getaddrinfo("
            << (a_inaddr.node.empty() ? "nullptr, " : "\"" + a_inaddr.node + "\", ")
            << (a_inaddr.service.empty() ? "nullptr, " : "\"" + a_inaddr.service + "\", ")
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
            // Should be set to 'saObj.ss' to get raw data info.
            saObj.ss = *reinterpret_cast<sockaddr_storage*>(new_res->ai_addr);
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

    // Have new valid address information with pointer 'new_res' to it?
    // ----------------------------------------------------------------
#if 0 // Maybe we don't need this?
    if (ret == 0) {
        SSockaddr saObj;
        // Must be set to 'saObj' to get checked data.
        saObj = *reinterpret_cast<sockaddr_storage*>(new_res->ai_addr);
        if (saObj.family == AF_UNSPEC) {
            UPnPsdk_LOGINFO("MSG1190") "syscall ::freeaddrinfo(" << new_res
                                                                 << ").\n";
            umock::netdb_h.freeaddrinfo(new_res);
            ret = EAI_NONAME;
        } else {
            m_res = new_res;
            m_res_current = new_res;
        }
    }
#endif
    if (ret == 0) {
        m_res = new_res;
        m_res_current = new_res;
    }

    return ret;
}

} // namespace UPnPsdk
