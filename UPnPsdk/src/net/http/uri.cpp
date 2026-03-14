// Copyright (C) 2025+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-03-14
/*!
 * \file
 * \brief Manage Uniform Resource Identifier (URI) as specified with <a
 * href="https://www.rfc-editor.org/rfc/rfc3986">RFC 3986</a>.
 */

#include <UPnPsdk/uri.hpp>

#include <UPnPsdk/synclog.hpp>
#include <UPnPsdk/sockaddr.hpp>
#include <UPnPsdk/messages.hpp>
#include <UPnPsdk/addrinfo.hpp>

/// \cond
#include <regex>
/// \endcond


namespace UPnPsdk {

namespace {

// Free functions
// ==============
/*!
 * \brief Check if string is a valid IPv4 address
 * \ingroup upnpsdk-uri
 *
 * This checks that the address consists of four octets, each ranging from 0 to
 * 255, separated by dots.
 *
 * \returns
 *  \b true&nbsp; if the input string is a valid IPv4 pattern. It's not
 *  resolved by DNS.\n
 *  \b false otherwise
 */
bool is_ipv4_addr(const std::string& ip ///< [in] String to test.
) {
    TRACE("Executing is_ipv4_addr()")
    std::regex ipv4_pattern(
        R"(^((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])\.){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])$)");
    return std::regex_match(ip, ipv4_pattern);
}


/*!
 * \brief Check if a string conforms to a DNS name
 * \ingroup upnpsdk-uri
 *
 * \returns
 *  \b true&nbsp; if the input string is a valid DNS label. It's not resolved
 *  by DNS.\n
 *  \b false otherwise
 */
bool is_dns_name(const std::string& label ///< [in] String to test.
) {
    // Regular expression to validate DNS label.
    // Negative Lookbehind: (?<!...) is not supported by C++ STL. I workaround
    // it. std::regex
    // pattern("^((?!-)[A-Za-z0-9-]{1,63}(?<!-)\\.)+[A-Za-z]{2,6}$");
    TRACE("Executing is_dns_name()")
    std::regex pattern("^((?!-)[A-Za-z0-9-]{1,63}\\.)+[A-Za-z]{2,6}$");
    return (std::regex_match(label, pattern) && !label.contains("-.") &&
            !label.contains("--") && !label.ends_with('-')) ||
           label == "localhost";
}

} // anonymous namespace


void remove_dot_segments(std::string& a_path) {
    // The letters (A., B., ...) are the steps as given by the algorithm in
    // RFC3986_5.2.4.
    TRACE("Executing UPnPsdk::remove_dot_segments(\"" + a_path + "\")")

    std::string_view path_sv{a_path};
    std::string output;

    while (!path_sv.empty()) {
        // A.
        if (path_sv.substr(0, 3) == "../") {
            path_sv.remove_prefix(3);
        } else if (path_sv.substr(0, 2) == "./") {
            path_sv.remove_prefix(2);
        }
        // B.
        else if (path_sv.substr(0, 3) == "/./") {
            path_sv.remove_prefix(2);
        } else if (path_sv == "/.") {
            path_sv.remove_suffix(1);
        }
        // C.
        else if (path_sv.substr(0, 4) == "/../") {
            path_sv.remove_prefix(3);
            if (!output.empty()) {
                size_t pos;
                output.erase((pos = output.find_last_of('/')) ==
                                     std::string::npos
                                 ? 0
                                 : pos); // Remove last segment
            }
        } else if (path_sv == "/..") {
            path_sv.remove_suffix(2);
            if (!output.empty()) {
                size_t pos;
                output.erase((pos = output.find_last_of('/')) ==
                                     std::string::npos
                                 ? 0
                                 : pos); // Remove last segment
            }
        }
        // D.
        else if (path_sv == "." || path_sv == "..") {
            path_sv = "";
        }
        // E.
        else {
            size_t start = path_sv.front() == '/' ? 1 : 0;
            size_t end = path_sv.find_first_of('/', start);
            if (end == std::string_view::npos)
                end = path_sv.size();
            output += path_sv.substr(0, end);
            path_sv.remove_prefix(end);
        }
    }

    a_path = output;
}


void decode_esc_chars(std::string& a_encoded) {
    TRACE("Executing decode_esc_chars()")

    std::string decoded;
    int value;

    auto it{a_encoded.begin()};
    auto it_end{a_encoded.end()};
    // clang-format off
    // First check begin and end of the URI for invalid encoded character. With
    // '%' we must always have three characters.
    if (!a_encoded.empty() &&
        ( /*begin*/ (a_encoded.size() <  3 && *it == '%') ||
          /*end*/   (a_encoded.size() >= 3 && (*(it_end - 2) == '%' || *(it_end - 1) == '%'))))
        goto exception;
    // clang-format on

    for (; it < it_end; it++) {
        if (static_cast<unsigned char>(*it) == '%') {
            // Check two hex digit characters after '%'.
            if (!std::isxdigit(static_cast<unsigned char>(*(it + 1))) ||
                !std::isxdigit(static_cast<unsigned char>(*(it + 2))))
                goto exception;

            value =
                stoi(a_encoded.substr(
                         static_cast<size_t>(it - a_encoded.begin()) + 1, 2),
                     nullptr, 16);
            decoded.push_back(static_cast<char>(value));
            it += 2; // Skip the next two characters
        } else {
            decoded.push_back(*it);
        }
    }

    a_encoded = decoded;
    return;

exception:
    throw std::invalid_argument(
        UPnPsdk_LOGEXCEPT(
            "MSG1159") "URI with invalid percent encoding, failed URI=\"" +
        a_encoded + "\".\n");
}


// CComponent
// ==========
CComponent::STATE CComponent::state() const {
    TRACE2(this, " Executing CComponent::state()")
    return m_state;
}

const std::string& CComponent::str() const {
    TRACE2(this, " Executing CComponent::str()")
    return m_component;
}


// get_scheme free helper function
// ==========---------------------
namespace {
/*!
 * \brief Separates the scheme component from a [URI](\ref glossary_URI)
 * \ingroup upnpsdk-uri
 * \returns
 *  - An empty string_view if scheme is undefined.
 *  - Only separator ':' if the scheme is empty.
 *  - A valid scheme pattern that always ends with separator ':'.
 */
std::string_view get_scheme(std::string_view a_uri_sv ///< [in] URI to parse
) {
    // A scheme, if any, must begin with a letter, must have alphanum
    // characters, or '-', or '+', or '.', and ends with ':'.
    TRACE("Executing get_scheme(a_uri_sv)")

    size_t pos;
    if ((pos = a_uri_sv.find_first_of(':')) == std::string_view::npos)
        // No separator found means the scheme is undefined. The URI-reference
        // may be a relative reference (RFC3986_4.1).
        return ""; // Valid after return with size 0 (but dangling pointer).

    if (pos == 0)
        // ':' at the first position means the scheme is empty. This is invalid
        // but will be checked later. I cannot simply return ':' as string_view
        // because the string ":" is only valid within this function.
        return std::string_view(a_uri_sv.data(), 1);

    // Strip the view to have only the scheme with ':'. If pos > size(), the
    // behavior is undefined. But that is guarded above.
    a_uri_sv.remove_suffix(a_uri_sv.size() - pos - 1);

    // Check if the scheme has valid character (RFC3986_3.1.).
    if (!std::isalpha(static_cast<unsigned char>(a_uri_sv.front()))) {
        // First character is not alpha. This is not a scheme.
        return ""; // Valid after return with size 0 (but dangling pointer).
    } else {
        unsigned char ch;
        for (auto it{a_uri_sv.begin()}; it < a_uri_sv.end() - 1; it++) {
            ch = static_cast<unsigned char>(*it);
            if (!(std::isalnum(ch) || (ch == '-') || (ch == '+') ||
                  (ch == '.'))) {
                // Invalid character for scheme. This is not a scheme.
                return ""; // Valid after return with size 0 (but dangling ptr).
            }
        }
    }

    return a_uri_sv;
}
} // anonymous namespace


// class CScheme
// =============
CScheme::CScheme(std::string_view a_uri_sv) {
    TRACE2(this, " Construct CScheme(a_uri_sv)")

    std::string_view scheme_sv = get_scheme(a_uri_sv);

    if (scheme_sv.empty())
        // Undefined scheme.
        return;
    if (scheme_sv == ":") {
        // Empty scheme.
        m_state = STATE::empty;
        return;
    }

    // Normalize scheme to lower case character (RFC3986_6.2.2.1).
    scheme_sv.remove_suffix(1); // Remove trailing ':'
    m_component = scheme_sv;
    for (auto it{m_component.begin()}; it < m_component.end(); it++)
        *it = static_cast<char>(std::tolower(static_cast<unsigned char>(*it)));

    m_state = STATE::avail;
}


// get_authority free helper function
// =============---------------------
namespace {
/*!
 * \brief Separates the authority component from a
 * [URI reference](\ref glossary_URIref)
 * \ingroup upnpsdk-uri
 *
 * A sender MUST NOT generate an "http" URI with an empty host identifier. A
 * recipient that processes such a URI reference MUST reject it as invalid (<a
 * href="https://www.rfc-editor.org/rfc/rfc7230#section-2.7.1">RFC7230
 * 2.7.1.</a>). This means an authority component is mandatory for "http[s]".
 *
 * \returns
 *  - An empty string_view if authority is undefined.
 *  - Double slash ("//") if the authority is empty.
 *  - A valid authority pattern that always starts with "//" but with no end
 * separator.
 */
std::string_view
get_authority(std::string_view a_uriref_sv ///< [in] URI reference to parse.
) {
    // The authority component is preceded by a double slash ("//") and is
    // terminated by the next slash ('/'), question mark ('?'), or number sign
    // ('#') character, or by the end of the URI (RFC3986 3.2.).
    TRACE("Executing get_authority(a_uriref_sv)")

    auto& npos = std::string_view::npos;
    size_t pos;
    if ((pos = a_uriref_sv.find("//")) != npos) {
        // Extract the authority component with its separator "//".
        a_uriref_sv.remove_prefix(pos);
        // Find end of the authority and remove the rest to the end.
        if ((pos = a_uriref_sv.find_first_of("/?#", 2)) != npos)
            a_uriref_sv.remove_suffix(a_uriref_sv.size() - pos);

        // Here we have the authority string-view with separator "//" and end
        // separator (one of '/', '?', '#', ''). Having authority only "//"
        // means authority is empty.
        return a_uriref_sv;
    }
    // No "//" separator found. An empty authority means it is undefined.
    return ""; // Valid after return with size 0 (but dangling pointer).
}
} // anonymous namespace


namespace {
// get_userinfo free helper function
// ============---------------------
/*!
 * \brief Separates the authority userinfo subcomponent from a
 * [URI reference](\ref glossary_URIref)
 * \ingroup upnpsdk-uri
 * \returns
 *  - An empty string_view if userinfo is undefined.
 *  - Only separator ':' or '@' if the userinfo is empty.
 *  - A valid userinfo pattern without any separator.
 */
std::string_view
get_userinfo(std::string_view a_uriref_sv ///< [in] URI reference to parse
) {
    // The user information, if present, starts with "//" and is followed by a
    // commercial at-sign ("@") that delimits it from the host (RFC3986
    // 3.2.1.). The userinfo may be undefined (no "@" found within the
    // authority). I also take an empty userinfo into account (first character
    // of the authority is "@" or ":"). Applications should not render as clear
    // text any password data after the first colon (:) found within a userinfo
    // subcomponent unless the data after the colon is the empty string
    // (indicating no password).
    TRACE("Executing get_userinfo(a_uriref_sv)")

    std::string_view authority_sv = get_authority(a_uriref_sv);

    if (authority_sv.empty())
        // Undefined authority.
        return ""; // Valid after return with size 0 (but dangling pointer).

    authority_sv.remove_prefix(2); // remove authority separator.

    auto& npos = std::string_view::npos;
    size_t pos;
    // Check if there is a separator.
    if ((pos = authority_sv.find_first_of('@')) == npos)
        // No userinfo sub-component available.
        return ""; // Valid after return with size 0 (but dangling pointer).

    if (pos == 0 || authority_sv[0] == ':') {
        // Separator '@' or ':' (for password) at first position means userinfo
        // is empty. Return string_view to that first character from input
        // string_view.
        return std::string_view(authority_sv.data(), 1);
    }

    // Extract userinfo with trailing separator.
    authority_sv.remove_suffix(authority_sv.size() - pos - 1);

    // Check special case with clear text password.
    if ((pos = authority_sv.find_first_of(':')) != npos) {
        // Here we have found a username with clear text password appended.
        // Strip deprecated clear text password without ':' (RFC3986 3.3.1.).
        authority_sv.remove_suffix(authority_sv.size() - pos - 1);
    }

    authority_sv.remove_suffix(1); // Remove trailing separator.
    return authority_sv;
}
} // anonymous namespace


// class CUserinfo
// ===============
CUserinfo::CUserinfo(std::string_view a_uriref_sv) {
    TRACE2(this, " Construct CUserinfo(a_uriref_sv)")

    std::string_view userinfo_sv = get_userinfo(a_uriref_sv);

    if (userinfo_sv.empty())
        // Undefined userinfo.
        return;
    if (userinfo_sv == "@" || userinfo_sv == ":") {
        // Empty userinfo.
        m_state = STATE::empty;
        return;
    }
    m_component = userinfo_sv;
    m_state = STATE::avail;
}


namespace {
// get_host free helper function
// ========---------------------
/*!
 * \brief Separates the authority host subcomponent from a
 * [URI reference](\ref glossary_URIref)
 * \ingroup upnpsdk-uri
 *
 * \returns
 *  - An empty string_view if host is undefined.
 *  - Only a separator ':' or '/' or '?' or '#' if the host is empty.
 *  - A valid host pattern without any separator.
 *
 * \exception std::invalid_argument if URI reference with invalid host address
 * or host name pattern is detected. No DNS lookup is performed.
 */
std::string_view
get_host(std::string_view a_uriref_sv ///< [in] URI reference to parse
) {
    // The Host information starts with "//" when a userinfo is removed. It
    // ends with ':' if a port is available, or with the end of the authority,
    // that is '/', or '?', or '#', or end of uri.
    TRACE("Executing get_host(a_uriref_sv)")

    std::string_view authority_sv = get_authority(a_uriref_sv);

    if (authority_sv.empty())
        // Undefined authority.
        return ""; // Valid after return with size 0 (but dangling pointer).

    // Point to a valid extern '/' character that can later be returned as end
    // saparator.
    std::string_view end_separator = authority_sv.substr(0, 1);
    authority_sv.remove_prefix(2); // remove authority separator.

    auto& npos = std::string_view::npos;
    size_t pos;
    // Strip userinfo from authority string if present.
    if ((pos = authority_sv.find_first_of('@')) != npos)
        authority_sv.remove_prefix(pos + 1);
    // Strip port from authority string if present. I have to look for last
    // occurrance of the port separator. If there is a ']' instead of ':' at
    // last then it is an IPv6 address with colons for the host without port.
    if ((pos = authority_sv.find_last_of("]:")) != npos &&
        authority_sv[pos] == ':')
        authority_sv.remove_suffix(authority_sv.size() - pos);

    // Here we have the extracted host string.
    if (authority_sv.empty()) {
        // An authority has always a non empty host component for "http" scheme,
        // and may be empty for "file" scheme. That is checked later. Here I
        // return an empty host.
        return end_separator;
    }

    // Check if the host_name is valid.
    // Check IPv6 address.
    if (authority_sv.front() == '[') {
        SSockaddr saObj;
        try {
            saObj = authority_sv;
        } catch (const std::exception&) {
            goto exception;
        }
    }
    // Check IPv4 address and DNS name. No DNS name resolution is performed.
    // The syntax rule for host is ambiguous because it does not completely
    // distinguish between an IPv4 address and a reg-name. In order to
    // disambiguate the syntax, we apply the "first-match-wins" algorithm:
    // If host matches the rule for IPv4 address, then it should be
    // considered an IPv4 address literal and not a reg-name.
    // (RFC3986_3.2.2.).
    else {
        std::string host_str{authority_sv};
        if (!is_ipv4_addr(host_str) && !is_dns_name(host_str)) {
            goto exception;
        }
    }

    return authority_sv;

exception:
    throw std::invalid_argument(
        UPnPsdk_LOGEXCEPT(
            "MSG1160") "invalid host address or host name on URI=\"" +
        std::string(a_uriref_sv) + "\".");
}
} // anonymous namespace


// class CHost
// ===========
CHost::CHost(std::string_view a_uriref_sv) {
    TRACE2(this, " Construct CHost(a_uriref_sv)")

    std::string_view host_sv = get_host(a_uriref_sv);

    if (host_sv.empty())
        // Undefined host.
        return;
    if (host_sv.size() == 1 &&
        host_sv.find_first_of(":/?#") != std::string_view::npos) {
        // Empty userinfo.
        m_state = STATE::empty;
        return;
    }
    // Normalize host to lower case character (RFC3986_3.2.2.).
    m_component = host_sv;
    for (auto it{m_component.begin()}; it < m_component.end(); it++)
        *it = static_cast<char>(std::tolower(static_cast<unsigned char>(*it)));

    m_state = STATE::avail;
}


namespace {
// get_port free helper function
// ========---------------------
/*!
 * \brief Separates the authority port subcomponent from a
 * [URI reference](\ref glossary_URIref)
 * \ingroup upnpsdk-uri
 *
 * \returns
 *  - An empty string_view if port is undefined.
 *  - Only a separator ':' if the port is empty.
 *  - A valid port pattern without any separator.
 *
 * \exception std::invalid_argument if invalid port number is detected.
 */
std::string_view
get_port(std::string_view a_uriref_sv ///< [in] URI reference to parse
) {
    // The Port information starts with ':' and ends with the end of the
    // authority, that is '/', or '?', or '#', or end of uri. Because there may
    // be also colons as separator for a password, or within an IPv6 address, I
    // must look at the last occurrence of a colon within the authority
    // component that is the separator for the port.
    TRACE("Executing get_port(a_uriref_sv)")

    std::string_view authority_sv = get_authority(a_uriref_sv);

    if (authority_sv.empty())
        // Undefined authority means also no port.
        return ""; // Valid after return with size 0 (but dangling pointer).

    authority_sv.remove_prefix(2); // remove authority separator "//".

    auto& npos = std::string_view::npos;
    size_t pos;
    // Extract port from authority string if present:
    // first strip userinfo from authority string if present.
    if ((pos = authority_sv.find_first_of('@')) != npos)
        authority_sv.remove_prefix(pos + 1);
    // I have to look for last occurrance of the port separator. If we find ']'
    // before ':' at last then it is an IPv6 address with colons for the host
    // without port.
    if ((pos = authority_sv.find_last_of("]:")) == npos ||
        authority_sv[pos] != ':')
        // No port found. It is undefined.
        return ""; // Valid after return with size 0 (but dangling pointer).

    // There is at least a ':'. Remove host if any.
    authority_sv.remove_prefix(pos);

    // Here we have the port string with preceeding separator ':', but it may be
    // empty.
    if (authority_sv == ":") {
        return authority_sv;
    }

    authority_sv.remove_prefix(1); // Remove preceeding ':'.

    // Check if the port string is valid.
    if (to_port(authority_sv) != 0)
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT("MSG1164") "Invalid port number. Failed URI=\"" +
            std::string(a_uriref_sv) + "\".\n");

    return authority_sv;
}
} // anonymous namespace


// class CPort
// ===========
CPort::CPort(std::string_view a_uriref_sv) {
    TRACE2(this, " Construct CPort(a_uriref_sv)")

    std::string_view port_sv = get_port(a_uriref_sv);

    if (port_sv.empty())
        // Undefined host.
        return;
    if (port_sv == ":") {
        // Empty port.
        m_state = STATE::empty;
        return;
    }

    // Default ports are normalized to undefined ports. Pattern optimized for
    // minimal calling get_scheme().
    if (port_sv == "80") {
        if (get_scheme(a_uriref_sv) == "http:") {
            m_state = STATE::undef;
            return;
        }
    } else if (port_sv == "443") {
        if (get_scheme(a_uriref_sv) == "https:") {
            m_state = STATE::undef;
            return;
        }
    }

    m_component = port_sv;
    m_state = STATE::avail;
}


// CAuthority
// ==========
CAuthority::CAuthority(std::string_view a_uri_sv)
    : userinfo(a_uri_sv), host(a_uri_sv), port(a_uri_sv) {
    TRACE2(this, " Construct CAuthority(a_uri_sv)");
}

CComponent::STATE CAuthority::state() const {
    TRACE2(this, " Executing CAuthority::state()")
    if (this->host.state() == STATE::avail ||
        this->userinfo.state() == STATE::avail ||
        this->port.state() == STATE::avail)
        return STATE::avail;

    if (this->host.state() == STATE::undef &&
        this->userinfo.state() == STATE::undef &&
        this->port.state() == STATE::undef)
        return STATE::undef;

    return STATE::empty;
}

std::string CAuthority::str() const {
    TRACE2(this, " Executing CAuthority::str()")
    return //
        (this->userinfo.state() == STATE::avail ? this->userinfo.str() : "") +
        (this->userinfo.state() == STATE::avail ? "@" : "") +
        (this->host.state() == STATE::avail ? this->host.str() : "") +
        (this->port.state() == STATE::avail ? ":" : "") +
        (this->port.state() == STATE::avail ? this->port.str() : "");
}


namespace {
// get_path free helper function
// ========---------------------
/*!
 * \brief Separates the path component from a
 * [URI reference](\ref glossary_URIref)
 * \ingroup upnpsdk-uri
 *
 * \returns
 *  - An empty string_view if the path is empty. A path is always defined for a
 * URI (<a
 * href="https://www.rfc-editor.org/rfc/rfc3986#section-3.3">RFC3986 3.3</a>).
 *  - A valid path pattern without any separator.
 */
std::string_view
get_path(std::string_view a_uriref_sv ///< [in] URI reference to parse
) {
    // A path is always defined for a URI, though the defined path may be empty
    // (zero length) (RFC3986_3.3.). To get the path I strip all other
    // components from the URI reference.
    // If a URI contains an authority component, then the path component must
    // either be empty or begin with a slash ("/") character. The path is
    // terminated by the first question mark ("?") or number sign ("#")
    // character, or by the end of the URI (RFC3986_3.3.). Schemes "http", and
    // "https" must always have an authority component (RFC7230_2.7.1.).
    // In addition, a URI reference may be a relative-path reference, in which
    // case the first path segment cannot contain a colon (":") character
    // (RFC3986_3.3.).
    TRACE("Executing get_path(a_uriref_sv)")

    auto& npos = std::string_view::npos;
    size_t pos;

    // Remove possible query and/or fragment. Scheme, authority and path does
    // not contain '?' or '#'.
    if ((pos = a_uriref_sv.find_first_of("?#")) != npos)
        a_uriref_sv.remove_suffix(a_uriref_sv.size() - pos);

    // Remove possible scheme.
    if ((pos = get_scheme(a_uriref_sv).size()) != 0)
        a_uriref_sv.remove_prefix(pos);

    // Remove possible authority.
    if ((pos = get_authority(a_uriref_sv).size()) != 0) {
        a_uriref_sv.remove_prefix(pos);
    }

    // a_uriref_sv can also be empty that means an empty path. The path cannot
    // be undefined by definition (RFC3986 3.3.).
    return a_uriref_sv;
}
} // anonymous namespace


// class CPath
// ===========
CPath::CPath(std::string_view a_uriref_sv) {
    TRACE2(this, " Construct CPath(a_uriref_sv)")

    std::string_view path_sv = get_path(a_uriref_sv);

    // The path cannot be undefined by definition (RFC3986 3.3.).
    if (path_sv.empty()) {
        // Empty path.
        m_state = STATE::empty;
        return;
    }

    m_component = path_sv;
    m_state = STATE::avail;
}


void CPath::remove_dot_segments() {
    // Normalize by removing dot segments in place.
    UPnPsdk::remove_dot_segments(m_component);
}


namespace {
// get_query free helper function
// =========---------------------
/*!
 * \brief Separates the query component from a
 * [URI reference](\ref glossary_URIref)
 * \ingroup upnpsdk-uri
 *
 * \returns
 *  - An empty string_view if the query component is undefined.
 *  - Only a separator "?" if the query component is empty.
 *  - A valid query component pattern without any separator.
 */
std::string_view
get_query(std::string_view a_uriref_sv ///< [in] URI reference to parse
) {
    // The query component is indicated by the first question mark ("?")
    // character and terminated by a number sign ("#") character or by the end
    // of the URI. The characters slash ("/") and question mark ("?") may
    // represent data within the query component (RFC3986_3.4.) and within the
    // fragment identifier (RFC3986_3.5.).
    TRACE("Executing get_query(a_uriref_sv)")

    // Find begin of the query component.
    auto& npos = std::string_view::npos;
    size_t pos;
    if ((pos = a_uriref_sv.find_first_of("?#")) == npos ||
        a_uriref_sv[pos] == '#')
        // No query component found. Leave it undefined.
        return ""; // Valid after return with size 0 (but dangling pointer).

    // There is a '?'. Strip all before the query component.
    a_uriref_sv.remove_prefix(pos);
    // Strip all behind the query component.
    if ((pos = a_uriref_sv.find_first_of('#')) != npos)
        a_uriref_sv.remove_suffix(a_uriref_sv.size() - pos);

    if (a_uriref_sv == "?")
        // Only '?' means the query is empty.
        return a_uriref_sv;

    // Return query without separator.
    a_uriref_sv.remove_prefix(1);
    return a_uriref_sv;
}
} // anonymous namespace


// class CQuery
// ============
CQuery::CQuery(std::string_view a_uriref_sv) {
    TRACE2(this, " Construct CQuery(a_uriref_sv)")

    std::string_view query_sv = get_query(a_uriref_sv);

    if (query_sv.empty())
        // Undefined query.
        return;
    if (query_sv == "?") {
        // Empty query.
        m_state = STATE::empty;
        return;
    }

    m_component = query_sv;
    m_state = STATE::avail;
}


namespace {
// get_fragment free helper function
// ============---------------------
/*!
 * \brief Separates the fragment component from a
 * [URI reference](\ref glossary_URIref)
 * \ingroup upnpsdk-uri
 *
 * \returns
 *  - An empty string_view if fragment component is undefined.
 *  - Only a separator '#' if the fragment component is empty.
 *  - A valid fragment component pattern without any separator.
 */
std::string_view
get_fragment(std::string_view a_uriref_sv ///< [in] URI reference to parse
) {
    // A fragment identifier component is indicated by the presence of a number
    // sign ("#") character and terminated by the end of the URI (RFC3986_3.5.).
    TRACE("Executing get_fragment(a_uriref_sv)")

    // Find begin of the fragment component.
    auto& npos = std::string_view::npos;
    size_t pos;
    if ((pos = a_uriref_sv.find_first_of('#')) == npos)
        // No fragment component found. Leave it undefined.
        return ""; // Valid after return with size 0 (but dangling pointer).

    // Strip all before the fragment component.
    a_uriref_sv.remove_prefix(pos);

    if (a_uriref_sv == "#")
        // Only '#' means the fragment is empty.
        return a_uriref_sv;

    // Return fragment without separator.
    a_uriref_sv.remove_prefix(1);
    return a_uriref_sv;
}
} // anonymous namespace


// Class CFragment
// ===============
CFragment::CFragment(std::string_view a_uriref_sv) {
    TRACE2(this, " Construct CFragment(a_uriref_sv)")

    std::string_view fragment_sv = get_fragment(a_uriref_sv);

    if (fragment_sv.empty())
        // Undefined fragment.
        return;
    if (fragment_sv == "#") {
        // Empty fragment.
        m_state = STATE::empty;
        return;
    }

    m_component = fragment_sv;
    m_state = STATE::avail;
}


// Class CPrepUriStr
// =================
CPrepUriStr::CPrepUriStr(std::string& a_uriref_str) {
    TRACE2(this, " Construct CPrepUriStr(a_uriref_str)")

    auto it{a_uriref_str.begin()};
    auto it_end{a_uriref_str.end()};
    // clang-format off
    // First check begin and end of the URI for invalid encoded character.
    if (!a_uriref_str.empty() &&
        ( /*begin*/ (a_uriref_str.size() <  3 && *it == '%') ||
          /*end*/   (a_uriref_str.size() >= 3 && (*(it_end - 2) == '%' || *(it_end - 1) == '%'))))
        goto exception;
    // clang-format on

    // Then parse the URI to set upper case hex digits.
    for (; it < it_end; it++) {
        if (static_cast<unsigned char>(*it) == '%') {
            it++;
            if (!std::isxdigit(static_cast<unsigned char>(*it)))
                goto exception;
            *it = static_cast<char>(
                std::toupper(static_cast<unsigned char>(*it)));
            it++;
            if (!std::isxdigit(static_cast<unsigned char>(*it)))
                goto exception;
            *it = static_cast<char>(
                std::toupper(static_cast<unsigned char>(*it)));
        }
    }
    return;

exception:
    throw std::invalid_argument(
        UPnPsdk_LOGEXCEPT(
            "MSG1165") "URI with invalid percent encoding, failed URI=\"" +
        a_uriref_str + "\".\n");
}


// Class CUriRef
// =============
CUriRef::CUriRef(std::string a_uriref_str)
    : prepare_uri_str(a_uriref_str), scheme(a_uriref_str),
      authority(a_uriref_str), path(a_uriref_str), query(a_uriref_str),
      fragment(a_uriref_str) {
    TRACE2(this, " Construct CUriRef(a_uriref_str)");

    if (this->scheme.state() == STATE::undef)
        // It's a relative refefence. The constructor checks only a base URI.
        return;

    if (this->scheme.str() == "http" || this->scheme.str() == "https") {
        if (this->authority.host.state() != STATE::avail)
            throw std::invalid_argument(
                UPnPsdk_LOGEXCEPT("MSG1168") "Scheme \"" + this->scheme.str() +
                "\" must have a host. Invalid URI=\"" + a_uriref_str + "\"\n");

    } else if (this->scheme.str() == "file") {
        // Due to RFC8089 2. Syntax:
        // 0. file       relative URI reference, not a file URI
        // 1. file:      invalid
        // 1. file://    invalid
        // 2. file:///
        // 3. file:///path/to/file
        // 4. file://localhost/
        // 5. file://localhost/path/to/file
        // 6. file://a.aa/
        // 7. file://a.aa/path/to/file
        // 8. file:/
        // 9. file:/path/to/file
        if (this->authority.userinfo.state() == STATE::undef &&
            this->authority.port.state() == STATE::undef &&
            this->path.state() != STATE::empty &&
            this->query.state() == STATE::undef &&
            this->fragment.state() == STATE::undef) {

            return;
        }
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT("MSG1169") "Invalid URI=\"" +
            std::string(a_uriref_str) + "\"\n");
    }
}

// CUriRef Getter
CComponent::STATE CUriRef::state() const {
    TRACE2(this, " Executing CUriRf::state()")
    using STATE = STATE;
    if (this->scheme.state() == STATE::avail ||
        this->authority.state() == STATE::avail ||
        this->path.state() == STATE::avail ||
        this->query.state() == STATE::avail ||
        this->fragment.state() == STATE::avail)
        return STATE::avail;

    if (this->scheme.state() == STATE::undef &&
        this->authority.state() == STATE::undef &&
        this->path.state() == STATE::undef &&
        this->query.state() == STATE::undef &&
        this->fragment.state() == STATE::undef)
        return STATE::undef;

    return STATE::empty;
}

std::string CUriRef::str() const {
    TRACE2(this, " Executing CUriRef::str()")

    // This rule always appends a '/' to an authority, even the path is empty.
    // For example, the URI "http://example.com/" is the normal form for the
    // "http" scheme, as specified by RFC3986 6.2.3.
    return (scheme.state() == STATE::avail ? scheme.str() : "") +
           (scheme.state() == STATE::undef ? "" : ":") +
           (authority.state() == STATE::undef ? "" : "//") +
           (authority.state() == STATE::avail ? authority.str() : "") +
           (authority.state() == STATE::undef
                ? ""
                : (path.state() != STATE::empty && path.str().front() == '/'
                       ? ""
                       : "/")) +
           (path.state() == STATE::avail ? path.str() : "") +
           (query.state() == STATE::undef ? "" : "?") +
           (query.state() == STATE::avail ? query.str() : "") +
           (fragment.state() == STATE::undef ? "" : "#") +
           (fragment.state() == STATE::avail ? fragment.str() : "");
}


// CUri
// ==========
namespace {
/*!
 * \brief Merge a [relative reference](\ref glossary_URIrel) to a base
 * [URI](\ref glossary_URI)
 * \ingroup upnpsdk-uri
 */
void merge_paths(CPath& a_path, ///< [out] Resulting merged paths object
                 const CUriRef& a_base, ///< [in] base URI object
                 const CUriRef& a_rel ///< [in] Relative reference object
) {
    // For an overview of the following algorithm have a look to the description
    // at RFC3986_5.2.3.
    std::string path_str;
    if (a_base.authority.state() != CComponent::STATE::undef &&
        a_base.path.state() == CComponent::STATE::empty) {
        path_str = '/' + a_rel.path.str();
    } else {
        size_t pos;
        if ((pos = a_base.path.str().find_last_of('/')) == std::string::npos)
            path_str.clear();
        else
            path_str = a_base.path.str().substr(0, pos);
        path_str += '/' + a_rel.path.str();
    }
    a_path = CPath(path_str);
}
} // anonymous namespace


CUri::CUri(std::string a_uriref_str) : base(a_uriref_str), target("") {
    TRACE2(this, " Construct CUri(a_uriref_str)")

    if (this->base.scheme.state() != CComponent::STATE::avail)
        // It's a relative resource refefence. The constructor accepts only a
        // base URI.
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT(
                "MSG1170") "Only base URI accepted. Invalid URI=\"" +
            a_uriref_str + "\"\n");
}


void CUri::operator=(std::string a_relref_str) {
    TRACE2(this, " Executing CUri::operator=(a_relref_str)")
    // Get a working object with splitted components of the URI input string.
    CUriRef relObj(a_relref_str);

    if (relObj.scheme.state() != CComponent::STATE::undef)
        // It's not a relative refefence. Only that is accepted.
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT(
                "MSG1171") "Only relative reference accepted. Invalid URI=\"" +
            a_relref_str + "\"\n");

    // Transform References
    // --------------------
    // For an overview of the following algorithm have a look to the pseudo code
    // at RFC3986_5.2.2.
    auto& baseObj = this->base;
    auto& targetObj = this->target;

    if (relObj.scheme.state() != CComponent::STATE::undef) {
        targetObj.scheme = relObj.scheme;
        targetObj.authority = relObj.authority;
        targetObj.path = relObj.path;
        targetObj.path.remove_dot_segments();
        targetObj.query = relObj.query;
    } else {
        if (relObj.authority.state() != CComponent::STATE::undef) {
            targetObj.authority = relObj.authority;
            targetObj.path = relObj.path;
            targetObj.path.remove_dot_segments();
            targetObj.query = relObj.query;
        } else {
            if (relObj.path.state() == CComponent::STATE::empty) {
                targetObj.path = baseObj.path;
                targetObj.path.remove_dot_segments();
                if (relObj.query.state() != CComponent::STATE::undef)
                    targetObj.query = relObj.query;
                else
                    targetObj.query = baseObj.query;
            } else {
                if (relObj.path.str().front() == '/') {
                    targetObj.path = relObj.path;
                    targetObj.path.remove_dot_segments();
                } else {
                    merge_paths(targetObj.path, baseObj, relObj);
                    targetObj.path.remove_dot_segments();
                }
                targetObj.query = relObj.query;
            }
            targetObj.authority = baseObj.authority;
        }
        targetObj.scheme = baseObj.scheme;
    }
    targetObj.fragment = relObj.fragment;
}


CComponent::STATE CUri::state() const {
    if (this->target.state() != CComponent::STATE::avail)
        return this->base.state();
    else
        return this->target.state();
}

std::string CUri::str() const {
    if (this->target.state() != CComponent::STATE::avail)
        return this->base.str();
    else
        return this->target.str();
}

} // namespace UPnPsdk


int parse_uri(const char* in, size_t max, uri_type* out) {
    // The CUriRef class coppies its input URI reference string to the stack to
    // be more thread safe. So it cannot provide pointer to the external source.
    // For that I use the "get component" free functions.
    using STATE = UPnPsdk::CComponent::STATE;
    std::string_view uriref_sv = std::string_view(in, max);
    // std::cerr << "DEBUG: parse_uri: uriref_sv=\"" << uriref_sv << "\"\n";

    try {
        UPnPsdk::CUriRef uriObj{std::string(uriref_sv)};

#if 0 // Helpful for developers for deep analysis.
        std::cerr << "DEBUG: scheme=\"" << uriObj.scheme.str() << "\"\n";
        std::cerr << "DEBUG: authority=\"" << uriObj.authority.str() << "\"\n";
        std::cerr << "DEBUG: path=\"" << uriObj.path.str() << "\"\n";
        std::cerr << "DEBUG: query=\"" << uriObj.query.str() << "\"\n";
        std::cerr << "DEBUG: fragment=\"" << uriObj.fragment.str() << "\"\n";
#endif
        ::memset(out, 0, sizeof(*out));

        // 'out->type' and 'out->path_type'
        if (uriObj.scheme.state() == STATE::avail) {
            out->type = uriType::Absolute;
            out->path_type = pathType::OPAQUE_PART;
        } else {
            out->type = uriType::Relative;
            out->path_type = pathType::REL_PATH;
        }
        // Correct 'out->path_type' if absolute path is available.
        if (uriObj.path.state() == STATE::avail &&
            uriObj.path.str().front() == '/')
            out->path_type = pathType::ABS_PATH;

        // out->scheme
        if (uriObj.scheme.state() == STATE::avail) {
            std::string_view scheme_sv = UPnPsdk::get_scheme(uriref_sv);
            scheme_sv.remove_suffix(1); // Remove trailing ':'
            out->scheme.buff = scheme_sv.data();
            out->scheme.size = scheme_sv.size();
        }

        // out->hostport
        if (uriObj.authority.host.state() == STATE::avail) {
            // Get pointer to host component on the input URI string and store
            // it to 'out'.
            std::string_view host_sv = UPnPsdk::get_host(uriref_sv);
            out->hostport.text.buff = host_sv.data();

            // Calculate size of the host+port string and store it to 'out'. If
            // there is no port available we must only store the size of the
            // host string, otherwise we need the sum of host and port sizes
            // plus 1 for the host:port separator ':'.
            if (uriObj.authority.port.state() == STATE::avail)
                out->hostport.text.size = uriObj.authority.host.str().size() +
                                          1 +
                                          uriObj.authority.port.str().size();
            else
                out->hostport.text.size = uriObj.authority.host.str().size();

            // Calculate the port to be used to get the remote host network
            // address. If it is available then that is used; if not, then the
            // defaults for "https" and "http" are used, or the unspecified one
            // (port=0).
            std::string port_str;
            if (uriObj.authority.port.state() == STATE::avail) {
                port_str = uriObj.authority.port.str();
            } else {
                if (uriObj.scheme.state() == STATE::avail) {
                    if (uriObj.scheme.str() == "https")
                        port_str = "443";
                    else if (uriObj.scheme.str() == "http")
                        port_str = "80";
                }
            }
            // Get the network address. If necessary with DNS lookup.
            UPnPsdk::CAddrinfo ai(uriObj.authority.host.str(), port_str);
            if (!ai.get_first())
                throw std::invalid_argument(
                    UPnPsdk_LOGEXCEPT(
                        "MSG1155") "Host not found. Failed URI=\"" +
                    std::string(uriref_sv) + "\"\n");
            // Store the address to 'out'.
            out->hostport.IPaddress =
                *reinterpret_cast<sockaddr_storage*>(ai->ai_addr);
        }

        // out->pathquery
        if (uriObj.path.state() == STATE::avail) {
            std::string_view path_sv = UPnPsdk::get_path(uriref_sv);
            out->pathquery.buff = path_sv.data();
            if (uriObj.query.state() == STATE::avail) {
                std::string_view query_sv = UPnPsdk::get_query(uriref_sv);
                out->pathquery.size = path_sv.size() + 1 + query_sv.size();
            } else {
                out->pathquery.size = path_sv.size();
            }
        }

        // out->fragment
        if (uriObj.fragment.state() == STATE::avail) {
            std::string_view fragment_sv = UPnPsdk::get_fragment(uriref_sv);
            out->fragment.buff = fragment_sv.data();
            out->fragment.size = fragment_sv.size();
        }

        return HTTP_SUCCESS;

    } catch (const std::invalid_argument& ex) {
        UPnPsdk_LOGCATCH("MSG1046") "Catched next line...\n"
            << ex.what() << '\n';
        return UPNP_E_INVALID_URL;
    }
}
