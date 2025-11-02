// Copyright (C) 2025+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-11-08
/*!
 * \file
 */

#include <UPnPsdk/uri.hpp>

#include <UPnPsdk/synclog.hpp>
#include <UPnPsdk/sockaddr.hpp>

/// \cond
#include <regex>
/// \endcond


namespace UPnPsdk {

namespace {

/*!
 * \brief Check if a string conforms to a DNS name
 */
bool is_dns_name(const std::string& label) {
    // Regular expression to validate DNS label.
    // Negative Lookbehind: (?<!...) is not supported by C++ STL. I workaround
    // it. std::regex
    // pattern("^((?!-)[A-Za-z0-9-]{1,63}(?<!-)\\.)+[A-Za-z]{2,6}$");
    std::regex pattern("^((?!-)[A-Za-z0-9-]{1,63}\\.)+[A-Za-z]{2,6}$");
    return (std::regex_match(label, pattern) && !label.contains("-.") &&
            !label.contains("--") && !label.ends_with('-')) ||
           label == "localhost";
}

} // anonymous namespace


// Free functions
// ==============
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


// CUri::CComponent
// ================
// Constructor
CUri::CComponent::CComponent(){
    TRACE2(this, " Construct CUri::CComponent()") //
}

// Destructor
CUri::CComponent::~CComponent(){
    TRACE2(this, " Destruct CUri::CComponent()") //
}

CUri::STATE CUri::CComponent::state() const {
    TRACE2(this, " Executing CUri::CComponent::state()")
    return m_state;
}

std::string& CUri::CComponent::str() const {
    TRACE2(this, " Executing CUri::CComponent::str()")
    return m_component;
}


// CUri::CScheme
// =============
// Constructor
CUri::CScheme::CScheme(){
    TRACE2(this, " Construct CUri::CScheme()") //
}

// Destructor
CUri::CScheme::~CScheme() {
    TRACE2(this, " Destruct CUri::CScheme()") //
}

void CUri::CScheme::construct_from(std::string_view a_uri_stv) {
    // This should be called only one time from a constructor.

    // A uri starts always with a scheme that must begin with a letter, must
    // have alphanum characters, or '-', or '+', or '.', and ends with ':'.
    TRACE2(this, " Executing CUri::CScheme::construct_from(a_uri_stv)")

    auto& npos = std::string_view::npos;
    size_t pos;
    if ((pos = a_uri_stv.find_first_of(':')) == npos || pos == 0) {
        // No separator found or separator at first position means the scheme
        // is undefined or empty. But Each URI begins with a scheme name that
        // refers to a specification for assigning identifiers within that
        // scheme (RFC3986_3.1.).
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT(
                "MSG1046") "Ill formed URI, no scheme specified. Failed \"" +
            std::string(a_uri_stv) + "\"\n");
    }

    // Strip the view to only have the scheme. If pos > size(), the behavior is
    // undefined. But that is checked above.
    a_uri_stv.remove_suffix(a_uri_stv.size() - pos);

    // Check if the scheme has valid character and normalize it to lower case
    // character (RFC3986_3.1., RFC3986_6.2.2.1).
    m_state = STATE::undef;
    m_component = a_uri_stv;
    bool error{false};
    if (!std::isalpha(static_cast<unsigned char>(a_uri_stv.front()))) {
        // First character is not alpha.
        error = true;
    } else {
        unsigned char ch;
        for (auto it{m_component.begin()}; it < m_component.end(); it++) {
            ch = static_cast<unsigned char>(*it);
            if (!(std::isalnum(ch) || (ch == '-') || (ch == '+') ||
                  (ch == '.'))) {
                error = true;
                break;
            }
            *it = static_cast<char>(std::tolower(ch));
        }
    }

    // Check if valid or supported scheme given.
    if (error || !(m_component == "https" || m_component == "http" ||
                   m_component == "file"))
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT("MSG1157") "Invalid or unsupported scheme=\"" +
            std::string(a_uri_stv) + "\" given.\n");

    m_state = STATE::avail;
}


// CUri::CAuthority
// ================
// Constructor
CUri::CAuthority::CAuthority(){
    TRACE2(this, " Construct CUri::CAuthority()") //
}

// Destructor
CUri::CAuthority::~CAuthority() {
    TRACE2(this, " Destruct CUri::CAuthority()") //
}

void CUri::CAuthority::construct_from(std::string_view a_scheme_stv,
                                      std::string_view a_uri_stv) {
    // This should be called only one time from a constructor.

    // The authority component is preceded by a double slash ("//") and is
    // terminated by the next slash ('/'), question mark ('?'), or number sign
    // ('#') character, or by the end of the URI (RFC3986 3.2.). A sender MUST
    // NOT generate an "http" URI with an empty host identifier. A recipient
    // that processes such a URI reference MUST reject it as invalid (RFC7230
    // 2.7.1.). This means an authority component is mandatory.
    //
    // For scheme "file" an empty authority "file:///" is accepted because it
    // has implicit a default authority.host "localhost" defined
    // (RFC3986_3.2.2.). When a scheme defines a default for authority and a
    // URI reference to that default is desired, the reference should be
    // normalized to an empty authority (RFC3986_6.2.3.).
    TRACE2(
        this,
        " Executing CUri::CAuthority::construct_from(a_scheme_stv, a_uri_stv)")

    auto& npos = std::string_view::npos;
    size_t pos;
    if ((pos = a_uri_stv.find("//")) == npos || pos + 2 >= a_uri_stv.size())
        // There is no valid authority component
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT("MSG1154") "Ill formed URI. It must have a host "
                                         "identifier. Failed \"" +
            std::string(a_uri_stv) + "\".\n");

    std::string_view authority_stv = a_uri_stv; // Working copy

    // Find end of the authority and remove the rest to the end.
    authority_stv.remove_prefix(pos + 2);
    if ((pos = authority_stv.find_first_of("/?#")) != npos)
        authority_stv.remove_suffix(authority_stv.size() - pos);

    // Check if we have something like "http[s]:///" that should fail, or
    // "file:///" that should be accepted with host state set to empty (done
    // with the host component).
    if (!authority_stv.empty() && authority_stv.front() == '/' &&
        a_scheme_stv != "file") {
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT("MSG1155") "Ill formed URI. An empty "
                                         "authority isn't specified "
                                         "for this scheme. Failed \"" +
            std::string(a_uri_stv) + "\".\n");
    }
    // Here we have the authority string. Construct subcomponents.
    this->host.construct_from(a_scheme_stv, authority_stv); // First to call.
    this->userinfo.construct_from(authority_stv);
    this->port.construct_from(authority_stv);

    return;
}

CUri::STATE CUri::CAuthority::state() const {
    if (this->host.state() == STATE::undef)
        // This is guarded by the construction of CAuthority.
        return STATE::undef;

    if (this->host.state() == STATE::avail ||
        this->userinfo.state() == STATE::avail ||
        this->port.state() == STATE::avail)
        return STATE::avail;

    return STATE::empty;
}

std::string CUri::CAuthority::str() const {
    return (userinfo.state() == STATE::avail ? userinfo.str() : "") +
           (userinfo.state() == STATE::undef ? "" : "@") +
           (host.state() == STATE::avail ? host.str() : "") +
           (port.state() == STATE::undef ? "" : ":") +
           (port.state() == STATE::avail ? port.str() : "");
}


// CUri::CAuthority::CUserinfo
// ===========================
// Constructor
CUri::CAuthority::CUserinfo::CUserinfo(){
    TRACE2(this, " Construct CUri::CAuthority::CUserinfo()") //
}

// Destructor
CUri::CAuthority::CUserinfo::~CUserinfo() {
    TRACE2(this, " Destruct CUri::CAuthority::CUserinfo()") //
}

void CUri::CAuthority::CUserinfo::construct_from(
    std::string_view a_authority_stv) {
    // This should be called only one time from a constructor.

    // The user information, if present, is followed by a commercial at-sign
    // ("@") that delimits it from the host (RFC3986 3.2.1.). The userinfo may
    // be undefined (no "@" found within the authority). I also take an empty
    // userinfo into account (first character of the authority is "@").
    // Applications should not render as clear text any password data after the
    // first colon (:) found within a userinfo subcomponent unless the data
    // after the colon is the empty string (indicating no password).
    TRACE2(this, " Executing "
                 "CUri::CAuthority::CUserinfo::construct_from(a_authority_stv)")

    auto& npos = std::string_view::npos;
    size_t pos;
    // Check if there is a separator, or if it is on the first position (no
    // content preceeding).
    if ((pos = a_authority_stv.find_first_of('@')) == npos || pos == 0)
        return; // No userinfo subcomponent available. Leave userinfo undefined.

    // Extract userinfo
    a_authority_stv.remove_suffix(a_authority_stv.size() - pos);

    // Check special case with clear text password.
    if ((pos = a_authority_stv.find_first_of(':')) != npos) {
        // Here we have found a username with clear text password appended.
        // That will be stripped incl. ':'.
        if (pos == 0) {
            return;
        }
        // Strip deprecated clear text password (RFC3986 3.3.1.).
        a_authority_stv.remove_suffix(a_authority_stv.size() - pos);
    }

    m_component = a_authority_stv;
    m_state = STATE::avail;
    return;
}


// CUri::CAuthority::CHost
// =======================
// Constructor
CUri::CAuthority::CHost::CHost(){
    TRACE2(this, " Construct CUri::CAuthority::CHost()") //
}

// Destructor
CUri::CAuthority::CHost::~CHost() {
    TRACE2(this, " Destruct CUri::CAuthority::CHost()") //
}

void CUri::CAuthority::CHost::construct_from(std::string_view a_scheme_stv,
                                             std::string_view a_authority_stv) {
    // This should be called only one time from a constructor.
    TRACE2(this,
           " Executing CUri::CAuthority::CHost::construct_from(a_scheme_stv, "
           "a_authority_stv)")

    auto authority_stv = a_authority_stv; // Working copy

    auto& npos = std::string_view::npos;
    size_t pos;
    // Strip userinfo from authority string if present.
    if ((pos = authority_stv.find_first_of('@')) != npos)
        authority_stv.remove_prefix(pos + 1);
    // Strip port from authority string if present. I have to look for last
    // ocurance of the separator. The host may have an IPv6 address that also
    // have colons.
    if ((pos = authority_stv.find_last_of("]:")) != npos &&
        authority_stv[pos] == ':')
        authority_stv.remove_suffix(authority_stv.size() - pos);

    // Here we have the extracted host string.
    if (authority_stv.empty()) {
        if (a_scheme_stv == "file") {
            m_component.clear();
            m_state = STATE::empty;
            return;
        }
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT("MSG1163") "Ill formed URI. Scheme '" +
            std::string(a_scheme_stv) +
            "' must have a host identifier. Failed authority \"" +
            std::string(a_authority_stv) + "\".\n");
    }

    m_state = STATE::undef;
    m_component = authority_stv;
    // Normalize host to lower case character (RFC3986_3.2.2.).
    for (auto it{m_component.begin()}; it < m_component.end(); it++)
        *it = static_cast<char>(std::tolower(static_cast<unsigned char>(*it)));

    // Check if the host_name is valid.
    SSockaddr saObj;
    try {
        // First check for a numeric ip address.
        saObj = m_component;
    } catch (const std::exception&) {
        // No valid numeric IP address. Check for valid DNS name.
        if (!is_dns_name(m_component)) {
            throw std::invalid_argument(
                UPnPsdk_LOGEXCEPT(
                    "MSG1160") "URI with invalid host address or host name \"" +
                m_component + "\".\n");
        }
    }

    m_state = STATE::avail;
    return;
}


// CUri::CAuthority::CPort
// =======================
// Constructor
CUri::CAuthority::CPort::CPort(){
    TRACE2(this, " Construct CUri::CAuthority::CPort()") //
}

// Destructor
CUri::CAuthority::CPort::~CPort() {
    TRACE2(this, " Destruct CUri::CAuthority::CPort()") //
}

void CUri::CAuthority::CPort::construct_from(std::string_view a_authority_stv) {
    // This should be called only one time from a constructor.
    TRACE2(
        this,
        " Executing CUri::CAuthority::CPort::construct_from(a_authority_stv)")

    std::string_view authority_stv = a_authority_stv; // Working copy

    auto& npos = std::string_view::npos;
    size_t pos;
    // Extract port from authority string if present. I have to look for last
    // ocurance of the separator. The host may have an IPv6 address or a
    // userinfo.password that also have colons.
    // First strip userinfo from authority string if present.
    if ((pos = authority_stv.find_first_of('@')) != npos)
        authority_stv.remove_prefix(pos + 1);
    if ((pos = authority_stv.find_last_of("]:")) == npos ||
        authority_stv[pos] != ':')
        return; // No port found. It is undefined.

    // Remove host if any.
    authority_stv.remove_prefix(pos + 1);

    // Here we have the port string.
    if (authority_stv.empty()) {
        return;
    }

    // Check if the port string is valid.
    try {
        SSockaddr saObj;
        saObj = authority_stv; // Throws exception std::range_error
    } catch (const std::exception& ex) {
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT("MSG1164") "Catched next line...\n" + ex.what() +
            " Failed \"" + std::string(a_authority_stv) + "\".\n");
    }

    m_component = authority_stv;
    m_state = STATE::avail;
}


// CUri::CPath
// ===========
// Constructor
CUri::CPath::CPath(){
    TRACE2(this, " Construct CUri::CPath()") //
}

// Destructor
CUri::CPath::~CPath() {
    TRACE2(this, " Destruct CUri::CPath()") //
}

void CUri::CPath::construct_from(std::string_view a_uri_sv) {
    // This should be called only one time from a constructor.

    // If a URI contains an authority component, then the path component must
    // either be empty or begin with a slash ("/") character. The path is
    // terminated by the first question mark ("?") or number sign ("#")
    // character, or by the end of the URI (RFC3986_3.3.). For now I only
    // handle schemes "http", "https", and "file". They must always have an
    // authority component (RFC7230_2.7.1.).
    /// \todo manage relative URI path reference.
    // In addition, a URI reference may be a relative-path reference, in which
    // case the first path segment cannot contain a colon (":") character
    // (RFC3986_3.3.).
    TRACE2(this, " Executing CUri::CPath::construct_from(a_uri_sv)")

    // Remove prefix and suffix from the uri string_view that can contain
    // slashes do not belonging to the path.
    auto& npos = std::string_view::npos;
    size_t pos;
    if ((pos = a_uri_sv.find("//")) != npos)
        a_uri_sv.remove_prefix(pos + 2);
    if ((pos = a_uri_sv.find_first_of("?#")) != npos)
        a_uri_sv.remove_suffix(a_uri_sv.size() - pos);

    // Here we have a uri artifact that contains the path component with
    // slashes only belonging to it. Looking for it.
    if ((pos = a_uri_sv.find_first_of("/")) == npos) {
        // No '/' found. That means there is an empty path componnent.
        m_component.clear();
        m_state = STATE::empty;
        return;
    }
    // Strip all before path begin.
    a_uri_sv.remove_prefix(pos);
    if (a_uri_sv == "/") {
        m_component.clear();
        m_state = STATE::empty;
        return;
    }

    // There is nothing to strip behind the path component (query and/or
    // fragment component). That's already done above.
    m_component = a_uri_sv;
    // Normalize by removing dot segments in place.
    remove_dot_segments(m_component);

    m_state = STATE::avail;
}


// CUri::CQuery
// ===========
// Constructor
CUri::CQuery::CQuery(){
    TRACE2(this, " Construct CUri::CQuery()") //
}

// Destructor
CUri::CQuery::~CQuery() {
    TRACE2(this, " Destruct CUri::CQuery()") //
}

void CUri::CQuery::construct_from(std::string_view a_uri_stv) {
    // This should be called only one time from a constructor.

    // The query component is indicated by the first question mark ("?")
    // character and terminated by a number sign ("#") character or by the end
    // of the URI. The characters slash ("/") and question mark ("?") may
    // represent data within the query component (RFC3986_3.4.).
    TRACE2(this, " Executing CUri::CQuery::construct_from(a_uri_stv)")

    // Find begin of the query component.
    auto& npos = std::string_view::npos;
    size_t pos;
    if ((pos = a_uri_stv.find_first_of('?')) == npos)
        // No query component found. Leave it undefined.
        return;

    // Strip all before the query component.
    a_uri_stv.remove_prefix(pos + 1);
    if (a_uri_stv.empty() || a_uri_stv.front() == '#') {
        m_component.clear();
        m_state = STATE::empty;
        return;
    }
    // Strip all behind the query component.
    if ((pos = a_uri_stv.find_first_of('#')) != npos)
        a_uri_stv.remove_suffix(a_uri_stv.size() - pos);

    m_component = a_uri_stv;
    m_state = STATE::avail;
}


// CUri::CFragment
// ===============
// Constructor
CUri::CFragment::CFragment(){
    TRACE2(this, " Construct CUri::CFragment()") //
}

// Destructor
CUri::CFragment::~CFragment() {
    TRACE2(this, " Destruct CUri::CFragment()") //
}

void CUri::CFragment::construct_from(std::string_view a_uri_stv) {
    // This should be called only one time from a constructor.

    // A fragment identifier component is indicated by the presence of a number
    // sign ("#") character and terminated by the end of the URI.
    TRACE2(this, " Executing CUri::CFragment::construct_from(a_uri_stv)")

    // Find begin of the fragment component.
    auto& npos = std::string_view::npos;
    size_t pos;
    if ((pos = a_uri_stv.find_first_of('#')) == npos)
        // No fragment component found. Leave it undefined.
        return;

    // Strip all before the fragment component.
    a_uri_stv.remove_prefix(pos + 1);
    if (a_uri_stv.empty()) {
        m_component.clear();
        m_state = STATE::empty;
        return;
    }

    m_component = a_uri_stv;
    m_state = STATE::avail;
}


// CUri
// =====================================
// Constructor
CUri::CUri(){
    TRACE2(this, " Construct CUri()") //
}

// Constructor with setting the URI
CUri::CUri(std::string a_uri_str) {
    // It is important that the argument 'a_uri_str' is given by value. So
    // it is coppied to the stack and constant available for the live time
    // of the constructor and can be used as stable base for string_views.
    TRACE2(this, " Construct CUri(\"" + a_uri_str + "\")")

    // Normalize percent encoded character to upper case hex digits
    // (RFC3986_2.1.).
    auto it{a_uri_str.begin()};
    auto it_end{a_uri_str.end()};
    // clang-format off
    // First check begin and end of the URI for invalid encoded character.
    if (!a_uri_str.empty() &&
        ( /*begin*/ (a_uri_str.size() <  3 && *it == '%') ||
          /*end*/   (a_uri_str.size() >= 3 && (*(it_end - 2) == '%' || *(it_end - 1) == '%'))))
        goto except;
    // clang-format on

    // Then parse the URI to set upper case hex digits.
    for (; it < it_end; it++) {
        if (static_cast<unsigned char>(*it) == '%') {
            it++;
            if (!std::isxdigit(static_cast<unsigned char>(*it)))
                goto except;
            *it = static_cast<char>(
                std::toupper(static_cast<unsigned char>(*it)));
            it++;
            if (!std::isxdigit(static_cast<unsigned char>(*it)))
                goto except;
            *it = static_cast<char>(
                std::toupper(static_cast<unsigned char>(*it)));
        }
    }

    this->scheme.construct_from(a_uri_str);
    this->authority.construct_from(scheme.str(), a_uri_str);
    this->path.construct_from(a_uri_str);
    this->query.construct_from(a_uri_str);
    this->fragment.construct_from(a_uri_str);

    return;

except:
    throw std::invalid_argument(
        UPnPsdk_LOGEXCEPT(
            "MSG1165") "URI with invalid percent encoding, failed URI=\"" +
        a_uri_str + "\".\n");
}

// Destructor
CUri::~CUri(){
    TRACE2(this, " Destruct CUri()") //
}

// Getter
std::string CUri::str() const {
    return (scheme.str() + ':' +
            (authority.state() == STATE::undef ? "" : "//") +
            (authority.state() == STATE::avail ? authority.str() : "") +
            (path.state() == STATE::empty ? "/" : "") +
            (path.state() == STATE::avail ? path.str() : "") +
            (query.state() == STATE::undef ? "" : "?") +
            (query.state() == STATE::avail ? query.str() : "") +
            (fragment.state() == STATE::undef ? "" : "#") +
            (fragment.state() == STATE::avail ? fragment.str() : ""));
}

} // namespace UPnPsdk
