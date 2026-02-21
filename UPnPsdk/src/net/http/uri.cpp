// Copyright (C) 2025+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-02-25
/*!
 * \file
 * \brief Manage Uniform Resource Identifier (URI) as specified with <a
 * href="https://www.rfc-editor.org/rfc/rfc3986">RFC 3986</a>.
 */

#include <UPnPsdk/uri.hpp>

#include <UPnPsdk/synclog.hpp>
#include <UPnPsdk/sockaddr.hpp>

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


/*!
 * \brief Normalize percent encoded characters of a URI reference string to
 * upper case hex digits (<a
 * href="https://www.rfc-editor.org/rfc/rfc3986#section-2.1">RFC3986_2.1.</a>)
 * \ingroup upnpsdk-uri
 *
 * Corrections are made in place. The size of the corrected string doesn't
 * change.
 *
 * \exception std::invalid_argument if invalid percent encoding is detected.
 */
void parse_percent_encoded_chars(
    std::string& a_uriref_str ///< [in,out] Reference of string to parse.
) {
    TRACE("Executing parse_percent_encoded_chars()")

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


// CComponent
// ==========
/*!
 * \brief Base class of a component
 * \ingroup upnpsdk-uri
 *
 * All special URI components (scheme, userinfo, host, port, path, query,
 * fragment) are derived from this class.
 */
class CComponent {
  public:
    /*! \brief Defines the possible states of a URI component.
     *
     * Quote <a
     * href="https://www.rfc-editor.org/rfc/rfc3986#section-5.3">RFC3986 5.3.</a>:
     * > Note that we are careful to preserve the distinction between a
     * > component that is undefined, meaning that its separator was not present
     * > in the reference, and a component that is empty, meaning that the
     * > separator was present and was immediately followed by the next
     * > component separator or the end of the reference.
     *
     * The state of a component in this class is not oriented on the input
     * [URI reference](\ref glossary_URIref), but on its output URI reference
     * taking normalization and comparison into account (<a
     * href="https://www.rfc-editor.org/rfc/rfc3986#section-6">RFC3986 6.</a>).
     * For example an input URI reference "https://@[::1]:" will not set
     * authority.userinfo.state() and authority.port.state() to 'STATE::empty'
     * but to 'STATE::undef', because the expected output URI reference is
     * "https://[::1]". */
    enum struct STATE {
        undef, ///< The component is undefined. Accessing it throws an exception
        empty, ///< The component string is empty.
        avail ///< The component string is available means it's a valid content.
    };

    /*! \brief Get state of the component
     * \returns State of the component. */
    STATE state() const;

    /*! \brief Get the string of the component
     * \returns Reference of the component string.
     * \exception std::invalid_argument if trying to read an undefined component
     * string. */
    const std::string& str() const;

  protected:
    /// \cond
    STATE m_state{STATE::undef};
    SUPPRESS_MSVC_WARN_4251_NEXT_LINE
    std::string m_component;
    /// \endcond
};

CComponent::STATE CComponent::state() const {
    TRACE2(this, " Executing CComponent::state()")
    return m_state;
}

const std::string& CComponent::str() const {
    TRACE2(this, " Executing CComponent::str()")
    if (m_state == STATE::undef)
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT("MSG1154") "Reading an undefined URI component "
                                         "is not possible.\n");

    return m_component;
}


// CScheme helper
// --------======
namespace {
/*!
 * \brief Extract the scheme component from a [URI](\ref glossary_URI)
 * \ingroup upnpsdk-uri
 * \returns
 *  - An empty string_view if scheme is undefined.
 *  - Only separator ':' if the scheme is empty.
 *  - A valid scheme string_view that always ends with separator ':'.
 */
std::string_view
get_scheme(std::string_view a_uri_sv ///< [in] string_view to parse
) {
    // A scheme, if any, must begin with a letter, must have alphanum
    // characters, or '-', or '+', or '.', and ends with ':'.
    TRACE("Executing get_scheme(a_uri_sv)")

    size_t pos;
    if ((pos = a_uri_sv.find_first_of(':')) == std::string_view::npos)
        // No separator found means the scheme is undefined. The URI-reference
        // may be a relative reference (RFC3986_4.1).
        return ""; // Guaranteed to be valid after return with size 0.

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
        return ""; // Guaranteed to be valid after return with size 0.
    } else {
        unsigned char ch;
        for (auto it{a_uri_sv.begin()}; it < a_uri_sv.end() - 1; it++) {
            ch = static_cast<unsigned char>(*it);
            if (!(std::isalnum(ch) || (ch == '-') || (ch == '+') ||
                  (ch == '.'))) {
                // Invalid character for scheme. This is not a scheme.
                return ""; // Guaranteed to be valid after return with size 0.
            }
        }
    }

    return a_uri_sv;
}
} // anonymous namespace


// class CScheme
// =============
/*!
 * \brief Scheme component of a [URI](\ref glossary_URI)
 * \ingroup upnpsdk-uri
 *
 * All of the requirements for the "http" scheme are also requirements for the
 * "https" scheme, except that TCP port 443 is the default instead port 80 for
 * "http" (<a
 * href="https://www.rfc-editor.org/rfc/rfc7230#section-2.7.2">RFC7230
 * 2.7.2.</a>).
 */
class CScheme : public CComponent {
  public:
    /// \brief Initialize the scheme component
    CScheme(std::string_view a_uri_sv ///< [in] Input URI string
    );
};

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


// Authority helper
// ----------======
namespace {
/*!
 * \brief Extract the authority component from a
 * [URI reference](\ref glossary_URIref)
 * \ingroup upnpsdk-uri
 *
 * A sender MUST NOT generate an "http" URI with an empty host identifier. A
 * recipient that processes such a URI reference MUST reject it as invalid (<a
 * href="https://www.rfc-editor.org/rfc/rfc7230#section-2.7.1">RFC7230
 * 2.7.1.</a>). This means an authority component is mandatory for "http[s]".
 * \returns
 *  - An empty string_view if authority is undefined.
 *  - Double slash ("//") if the authority is empty.
 *  - A valid authority string_view that always starts with "//".
 */
std::string_view
get_authority(std::string_view a_uri_sv ///< [in] string_view to parse.
) {
    // The authority component is preceded by a double slash ("//") and is
    // terminated by the next slash ('/'), question mark ('?'), or number sign
    // ('#') character, or by the end of the URI (RFC3986 3.2.).
    TRACE("Executing get_authority(a_uri_sv)")

    auto& npos = std::string_view::npos;
    size_t pos;
    if ((pos = a_uri_sv.find("//")) != npos) {
        // Extract the authority component with its separator "//".
        a_uri_sv.remove_prefix(pos);
        // Find end of the authority and remove the rest to the end.
        if ((pos = a_uri_sv.find_first_of("/?#", 2)) != npos)
            a_uri_sv.remove_suffix(a_uri_sv.size() - pos);

        // Here we have the authority string-view with separator "//" and end
        // separator (one of '/', '?', '#', ''). Having authority only "//"
        // means authority is empty.
        return a_uri_sv;
    }
    // No "//" separator found. An empty authority means it is undefined.
    return ""; // Guaranteed to be valid after return with size 0.
}
} // anonymous namespace


// Authority sub-component CUserinfo
// ------------------------=========
/*!
 * \brief Userinfo subcomponent from an authority component of a
 * [URI reference](\ref glossary_URIref)
 * \ingroup upnpsdk-uri
 */
class CUserinfo : public CComponent {
  public:
    /// \brief Initialize the userinfo subcomponent
    CUserinfo(std::string_view a_uri_sv ///< [in] Input URI string
    );
};

CUserinfo::CUserinfo(std::string_view a_uri_sv) {
    // The user information, if present, starts with "//" and is followed by a
    // commercial at-sign ("@") that delimits it from the host (RFC3986
    // 3.2.1.). The userinfo may be undefined (no "@" found within the
    // authority). I also take an empty userinfo into account (first character
    // of the authority is "@"). Applications should not render as clear text
    // any password data after the first colon (:) found within a userinfo
    // subcomponent unless the data after the colon is the empty string
    // (indicating no password).
    TRACE2(this, " Construct CUserinfo(a_uri_sv)")

    std::string_view authority_sv = get_authority(a_uri_sv);
    if (authority_sv.empty())
        return; // Undefined authority.
    authority_sv.remove_prefix(2); // remove authority separator.

    auto& npos = std::string_view::npos;
    size_t pos;
    if ((pos = authority_sv.find_first_of("/?#")) != npos)
        authority_sv.remove_suffix(authority_sv.size() - pos);

    // Check if there is a separator, or if it is on the first position (no
    // content preceeding).
    if ((pos = authority_sv.find_first_of('@')) == npos)
        return; // No userinfo sub-component available.

    if (pos == 0 || authority_sv[0] == ':') {
        // Separator '@' or ':' (for password) at first position means userinfo
        // is empty.
        m_component.clear();
        m_state = STATE::empty;
        return;
    }

    // Extract userinfo
    authority_sv.remove_suffix(authority_sv.size() - pos);

    // Check special case with clear text password.
    if ((pos = authority_sv.find_first_of(':')) != npos) {
        // Here we have found a username with clear text password appended.
        // Strip deprecated clear text password incl. ':' (RFC3986 3.3.1.).
        authority_sv.remove_suffix(authority_sv.size() - pos);
    }
    m_component = authority_sv;
    m_state = STATE::avail;
    return;
}


// Authority sub-component CHost
// ------------------------=====
/*!
 * \brief Host subcomponent from an authority component of a
 * [URI reference](\ref glossary_URIref).
 * \ingroup upnpsdk-uri
 *
 * With interpreting section <a
 * href="https://www.rfc-editor.org/rfc/rfc3986#section-3.2.2">RFC3986_3.2.2.</a>
 * DNS name resolution is not performed for registered names.
 *
 * Note beside other things from that RFC:
 * > URI references in information retrieval systems are designed to be
 * > late-binding: the result of an access is generally determined when it is
 * > accessed and may vary over time or due to other aspects of the interaction.
 * > These references are created in order to be used in the future: what is
 * > being identified is not some specific result that was obtained in the past,
 * > but rather some characteristic that is expected to be true for future
 * > results.
 * Registered name pattern are only checked to be syntactical correct. Validity
 * will be verified when accessing the resource. This speeds up URI reference
 * parsing a lot.
 */
class CHost : public CComponent {
  public:
    /*! \brief Initialize the host subcomponent
     * \exception std::invalid_argument if URI with invalid host address or host
     * name pattern is detected. No DNS lookup is performed. */
    CHost(std::string_view a_uri_sv ///< [in] Input URI string
    );
};

CHost::CHost(std::string_view a_uri_sv) {
    // The Host information starts with "//" when a userinfo is removed. It
    // ends with ':' if a port is available, or with the end of the authority,
    // that is '/', or '?', or '#', or end of uri.
    TRACE2(this, " Construct CHost(a_uri_sv)")

    std::string_view authority_sv = get_authority(a_uri_sv);

    if (authority_sv.empty())
        return; // Undefined authority.
    authority_sv.remove_prefix(2); // remove authority separator.

    auto& npos = std::string_view::npos;
    size_t pos;
    if ((pos = authority_sv.find_first_of("/?#")) != npos)
        authority_sv.remove_suffix(authority_sv.size() - pos);

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
        m_component.clear();
        m_state = STATE::empty;
        return;
    }

    m_component = authority_sv;
    // Normalize host to lower case character (RFC3986_3.2.2.).
    for (auto it{m_component.begin()}; it < m_component.end(); it++)
        *it = static_cast<char>(std::tolower(static_cast<unsigned char>(*it)));

    // Check if the host_name is valid.
    // Check IPv6 address.
    if (m_component.front() == '[') {
        SSockaddr saObj;
        try {
            saObj = m_component;
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
    else if (!is_ipv4_addr(m_component) && !is_dns_name(m_component)) {
        goto exception;
    }

    m_state = STATE::avail;
    return;

exception:
    throw std::invalid_argument(
        UPnPsdk_LOGEXCEPT(
            "MSG1160") "URI with invalid host address or host name \"" +
        m_component + "\".\n");
}


// Authority sub-component CPort
// ------------------------=====
/*!
 * \brief Port subcomponent from an authority component of a
 * [URI reference](\ref glossary_URIref).
 * \ingroup upnpsdk-uri
 */
class CPort : public CComponent {
  public:
    /*! \brief Initialize the port subcomponent
     * \exception std::invalid_argument if invalid port number is detected. */
    CPort(std::string_view a_uri_sv ///< [in] Input URI string
    );
};

CPort::CPort(std::string_view a_uri_sv) {
    // The Port information starts with ':' and ends with the end of the
    // authority, that is '/', or '?', or '#', or end of uri. Because there may
    // be also colons as separator for a password, or within an IPv6 address, I
    // must look at the last occurrence of a colon within the authority
    // information that is the separator for the port.
    TRACE2(this, " Construct CPort(a_uri_sv)")

    std::string_view authority_sv = get_authority(a_uri_sv);

    if (authority_sv.empty())
        return; // Undefined authority means also no port.
    authority_sv.remove_prefix(2); // remove authority separator "//".

    auto& npos = std::string_view::npos;
    size_t pos;
    // Remove path, query, fragment if any.
    if ((pos = authority_sv.find_first_of("/?#")) != npos)
        authority_sv.remove_suffix(authority_sv.size() - pos);

    // Extract port from authority string if present:
    // first strip userinfo from authority string if present.
    if ((pos = authority_sv.find_first_of('@')) != npos)
        authority_sv.remove_prefix(pos + 1);
    // I have to look for last occurrance of the port separator. If we find ']'
    // before ':' at last then it is an IPv6 address with colons for the host
    // without port.
    if ((pos = authority_sv.find_last_of("]:")) == npos ||
        authority_sv[pos] != ':')
        return; // No port found. It is undefined.

    // Remove host if any.
    authority_sv.remove_prefix(pos + 1);

    // Here we have the port string that is defined because we found a ':', but
    // it may be empty.
    if (authority_sv.empty()) {
        m_component.clear();
        m_state = STATE::empty;
        return;
    }
    // Default ports are normalized to undefined ports. Pattern optimized for
    // minimal calling get_scheme().
    if (authority_sv == "80") {
        if (get_scheme(a_uri_sv) == "http:") {
            m_state = STATE::undef;
            return;
        }
    } else if (authority_sv == "443") {
        if (get_scheme(a_uri_sv) == "https:") {
            m_state = STATE::undef;
            return;
        }
    }

    // Check if the port string is valid.
    if (to_port(authority_sv) != 0)
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT("MSG1164") "Invalid port number. Failed URI=\"" +
            std::string(a_uri_sv) + "\".\n");

    m_component = authority_sv;
    m_state = STATE::avail;
}


// CAuthority
// ==========
/*!
 * \brief Authority component of a [URI reference](\ref glossary_URIref)
 * \ingroup upnpsdk-uri
 * */
class CAuthority {
  public:
    ///@{
    /// authority subcomponent
    CUserinfo userinfo;
    CHost host;
    CPort port;
    ///@}

    /*! \brief Initialize the authority component
     * \exception std::invalid_argument
     *  - if trying to read an undefined component string.
     *  - if host pattern is invalid. No DNS lookup is performed.
     *  - if port number is invalid. */
    CAuthority(std::string_view a_uri_sv ///< [in] Input URI string
    );
    /*! \brief Get state of the authority
     * \returns State of the authority component. */
    CComponent::STATE state() const;

    /*! \brief Get the string of the authority component
     * \returns the authority component string. */
    std::string str() const;

  private:
    using STATE = CComponent::STATE;
};

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


// CPath
// =====
/*!
 * \brief Path component of a [URI reference](\ref glossary_URIref)
 * \ingroup upnpsdk-uri
 */
class CPath : public CComponent {
  public:
    /// \brief Initialize the path component
    CPath(std::string_view a_uri_sv ///< [in] Input URI string
    );
    /// \brief UPnPsdk::remove_dot_segments() from path component
    void remove_dot_segments();
};

CPath::CPath(std::string_view a_uri_sv) {
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
    TRACE2(this, " Construct CPath(a_uri_sv)")

    auto& npos = std::string_view::npos;
    size_t pos;

    // Remove possible query and/or fragment. Scheme, authority and path does
    // not contain '?' or '#'.
    if ((pos = a_uri_sv.find_first_of("?#")) != npos)
        a_uri_sv.remove_suffix(a_uri_sv.size() - pos);

    // Remove possible scheme.
    if ((pos = get_scheme(a_uri_sv).size()) != 0)
        a_uri_sv.remove_prefix(pos);

    // Remove possible authority.
    if ((pos = get_authority(a_uri_sv).size()) != 0) {
        a_uri_sv.remove_prefix(pos);
    }

    // Store remaining path.
    if (a_uri_sv.empty()) {
        m_component.clear();
        m_state = STATE::empty;
    } else {
        m_component = a_uri_sv;
        m_state = STATE::avail;
    }
}

void CPath::remove_dot_segments() {
    // Normalize by removing dot segments in place.
    UPnPsdk::remove_dot_segments(m_component);
}


// CQuery
// ======
/*!
 * \brief Query component of a [URI reference](\ref glossary_URIref)
 * \ingroup upnpsdk-uri
 */
class CQuery : public CComponent {
  public:
    /// \brief Initialize the query component
    CQuery(std::string_view a_uri_sv ///< [in] Input URI string
    );
};

CQuery::CQuery(std::string_view a_uri_sv) {
    // The query component is indicated by the first question mark ("?")
    // character and terminated by a number sign ("#") character or by the end
    // of the URI. The characters slash ("/") and question mark ("?") may
    // represent data within the query component (RFC3986_3.4.) and within the
    // fragment identifier (RFC3986_3.5.).
    TRACE2(this, " Construct CQuery(a_uri_sv)")

    // Find begin of the query component.
    auto& npos = std::string_view::npos;
    size_t pos;
    if ((pos = a_uri_sv.find_first_of("?#")) == npos || a_uri_sv[pos] == '#')
        // No query component found. Leave it undefined.
        return;

    // There is a '?'. Strip all before the query component.
    a_uri_sv.remove_prefix(pos + 1);
    if (a_uri_sv.empty() || a_uri_sv.front() == '#') {
        m_component.clear();
        m_state = STATE::empty;
        return;
    }
    // Strip all behind the query component.
    if ((pos = a_uri_sv.find_first_of('#')) != npos)
        a_uri_sv.remove_suffix(a_uri_sv.size() - pos);

    m_component = a_uri_sv;
    m_state = STATE::avail;
}


// CFragment
// =========
/*!
 * \brief Fragment component of a [URI reference](\ref glossary_URIref)
 * \ingroup upnpsdk-uri
 */
class CFragment : public CComponent {
  public:
    /// \brief Initialize the fragment component
    CFragment(std::string_view a_uri_sv ///< [in] Input URI string
    );
};

CFragment::CFragment(std::string_view a_uri_sv) {
    // A fragment identifier component is indicated by the presence of a number
    // sign ("#") character and terminated by the end of the URI (RFC3986_3.5.).
    TRACE2(this, " Construct CFragment(a_uri_sv)")

    // Find begin of the fragment component.
    auto& npos = std::string_view::npos;
    size_t pos;
    if ((pos = a_uri_sv.find_first_of('#')) == npos)
        // No fragment component found. Leave it undefined.
        return;

    // Strip all before the fragment component.
    a_uri_sv.remove_prefix(pos + 1);
    if (a_uri_sv.empty()) {
        m_component.clear();
        m_state = STATE::empty;
        return;
    }

    m_component = a_uri_sv;
    m_state = STATE::avail;
}


// Class CPrepUriStr
// =================
/*!
 * \brief Internal class to prepare the input URI string, Not publicly usable.
 * \ingroup upnpsdk-uri
 */
class CPrepUriStr {
  public:
    /*! \brief Initialize the helper class
     * \exception std::invalid_argument if invalid percent encoding is detected.
     */
    CPrepUriStr(std::string& a_uri_str ///< [in] Input URI string
    );
};

CPrepUriStr::CPrepUriStr(std::string& a_uri_str) {
    TRACE2(this, " Construct CPrepUriStr(a_uri_str)")

    // Normalize percent encoded character in place to uppercase.
    parse_percent_encoded_chars(a_uri_str);
}


// Class CUriRef
// =============
/*!
 * \brief This is a [URI reference](\ref glossary_URIref).
 * \ingroup upnpsdk-uri
 */
class CUriRef {
  public:
    ///@{
    /// Component of the URI reference
    CScheme scheme;
    CAuthority authority;
    CPath path;
    CQuery query;
    CFragment fragment;
    ///@}

    /*! \brief Initialize the URI reference
     * \exception std::invalid_argument
     *  - if trying to read an undefined component string.
     *  - if host pattern is invalid. No DNS lookup is performed.
     *  - if port number is invalid. */
    CUriRef(std::string_view a_uri_sv ///< [in] Input URI string
    );
    /// Get state of the URI reference
    CComponent::STATE state() const;
    /// Get URI reference string
    std::string str() const;

  private:
    using STATE = CComponent::STATE;
};

CUriRef::CUriRef(std::string_view a_uri_sv)
    : scheme(a_uri_sv), authority(a_uri_sv), path(a_uri_sv), query(a_uri_sv),
      fragment(a_uri_sv) {
    TRACE2(this, " Construct CUriRef(a_uri_sv)");

    if (this->scheme.state() == STATE::undef)
        // It's a relative refefence. The constructor checks only a base URI.
        return;

    if (this->scheme.str() == "http" || this->scheme.str() == "https") {
        if (this->authority.host.state() != STATE::avail)
            throw std::invalid_argument(
                UPnPsdk_LOGEXCEPT("MSG1168") "Scheme \"" + this->scheme.str() +
                "\" must have a host. Invalid URI=\"" + std::string(a_uri_sv) +
                "\"\n");

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
            std::string(a_uri_sv) + "\"\n");
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


// Class CUri
// ==========
/*!
 * \brief Representing a [URI](\ref glossary_URI) that can be modified with a
 * [relative reference](\ref glossary_URIrel)
 * \ingroup upnpsdk-uri
 * \code
// Usage e.g.:
try {
    CUri uriObj("https://example.com/path/");
    uriObj = "/to/res";
    std::cout << uriObj.str() << '\n'; // "https://example.com/to/res"
    uriObj = "to/res";
    std::cout << uriObj.str() << '\n'; // "https://example.com/path/to/res"
} catch (const std::invalid_argument& ex) {
    std::cerr << "Error! " << ex.what() << '\n';
    handle_error();
}
 * \endcode
 *
 * On the once given base URI with the constructor, a relative reference can be
 * modified as often as you wish.
 *
 * \note
 * This class succeeds the normal examples as given at <a
 * href="https://www.rfc-editor.org/rfc/rfc3986#section-5.4">RFC3986 5.4.</a>
 * If in daubt have look there.
 */
class CUri {
  public:
    /*! \brief Initialize with the base URI
     * \exception std::invalid_argument
     *  - if trying to read an undefined component string.
     *  - if host pattern is invalid. No DNS lookup is performed.
     *  - if port number is invalid.
     *  - if invalid percent encoding is detected.
     *  - if a [relative reference](\ref glossary_URIrel) without scheme
     * component is given. */
    CUri(
        /// [in] Setting an absolute URI, means must have a scheme.
        std::string a_uri_str);

    // Setter
    // ------
    /*! \brief Set a [relative resource reference](\ref glossary_URIrel)
     * \exception std::invalid_argument
     *  - if trying to read an undefined component string.
     *  - if host pattern is invalid. No DNS lookup is performed.
     *  - if port number is invalid.
     *  - if invalid percent encoding is detected.
     *  - if an absolute [URI](\ref glossary_URI) with scheme component is
     * given. */
    void operator=(
        /*! [in] String with a relative reference for the Base URI set with the
         * constructor. */
        std::string a_relref_str);

    // Getter
    // ------
    /// \brief Get state of the URI
    CComponent::STATE state() const;

    /*! \brief Get the resulting URI string merged with the
     * [relative reference](\ref glossary_URIref)
     *
     * If no relative reference is given then just the base URI is returned. */
    std::string str() const;

    // member classes
    // --------------
    // Order of following declarations is importent due to correct
    // initialization of the member classes.
    /// \brief Prepare input URI string
  private:
    CPrepUriStr prepare_uri_str;

  public:
    /// \brief Base URI
    CUriRef base;
    /// \brief Resulting URI of merged relative reference to the base URI
    CUriRef target;
};


CUri::CUri(std::string a_uriref_str)
    : prepare_uri_str(a_uriref_str), base(a_uriref_str), target("") {
    // It is important that the argument 'a_uriref_str' is given by value. So
    // it is coppied to the stack and available for the lifetime of the
    // constructor and can be used as stable base for string_views.
    TRACE2(this, " Construct CUri(a_uriref_str)")

    if (this->base.scheme.state() != CComponent::STATE::avail)
        // It's a relative refefence. The constructor accepts only a base URI.
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT(
                "MSG1170") "Only base URI accepted. Invalid URI=\"" +
            a_uriref_str + "\"\n");
}


void CUri::operator=(std::string a_uriref_str) {
    // It is important that the argument 'a_uriref_str' is given by value. So
    // it is coppied to the stack and available for the lifetime of the method
    // and can be used as stable base for string_views.
    TRACE2(this, " Executing CUri::operator=(a_uriref_str)")

    // Normalize percent encoded character in place to uppercase.
    parse_percent_encoded_chars(a_uriref_str);

    // Get a working object with splitted components of the URI input string.
    CUriRef relObj(a_uriref_str);

    if (relObj.scheme.state() != CComponent::STATE::undef)
        // It's not a relative refefence. Only that is accepted.
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT(
                "MSG1171") "Only relative reference accepted. Invalid URI=\"" +
            a_uriref_str + "\"\n");

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
