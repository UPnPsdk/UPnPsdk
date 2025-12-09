// Copyright (C) 2025+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-12-07
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
 * \brief Check if string is a valid IPv4 address
 *
 * This checks that the address consists of four octets, each ranging from 0 to
 * 255, separated by dots.
 */
bool is_ipv4_addr(const std::string& ip) {
    TRACE("Executing is_ipv4_addr()")
    std::regex ipv4_pattern(
        R"(^((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])\.){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])$)");
    return std::regex_match(ip, ipv4_pattern);
}

/*!
 * \brief Check if a string conforms to a DNS name
 */
bool is_dns_name(const std::string& label) {
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
 * upper case hex digits (RFC3986_2.1.).
 *
 * Possible corrections are made in place. The size of the corrected string
 * doesn't change.
 */
void parse_percent_encoded_chars(std::string& a_uriref_str) {
    TRACE("Executing parse_percent_encoded_char()")

    auto it{a_uriref_str.begin()};
    auto it_end{a_uriref_str.end()};
    // clang-format off
    // First check begin and end of the URI for invalid encoded character.
    if (!a_uriref_str.empty() &&
        ( /*begin*/ (a_uriref_str.size() <  3 && *it == '%') ||
          /*end*/   (a_uriref_str.size() >= 3 && (*(it_end - 2) == '%' || *(it_end - 1) == '%'))))
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
    return;

except:
    throw std::invalid_argument(
        UPnPsdk_LOGEXCEPT(
            "MSG1165") "URI with invalid percent encoding, failed URI=\"" +
        a_uriref_str + "\".\n");
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


// CComponents::CComponent
// =======================
// Constructor
CUri::CComponents::CComponent::CComponent(){
    TRACE2(this, " Construct CUri::CComponents::CComponent()") //
}

// Destructor
CUri::CComponents::CComponent::~CComponent(){
    TRACE2(this, " Destruct CUri::CComponents::CComponent()") //
}

CUri::STATE CUri::CComponents::CComponent::state() const {
    TRACE2(this, " Executing CUri::CComponents::CComponent::state()")
    return m_state;
}

std::string& CUri::CComponents::CComponent::str() const {
    TRACE2(this, " Executing CUri::CComponents::CComponent::str()")
    if (m_state == STATE::undef)
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT("MSG1154") "Reading an undefined URI component "
                                         "is not possible.\n");

    return m_component;
}


// CUri::CComponents::CScheme
// ==========================
// Constructor
CUri::CComponents::CScheme::CScheme(){
    TRACE2(this, " Construct CUri::CComponents::CScheme()") //
}

// Destructor
CUri::CComponents::CScheme::~CScheme() {
    TRACE2(this, " Destruct CUri::CComponents::CScheme()") //
}

void CUri::CComponents::CScheme::construct_from(std::string_view a_uri_sv) {
    // A scheme, if any, must begin with a letter, must have alphanum
    // characters, or '-', or '+', or '.', and ends with ':'.
    TRACE2(this,
           " Executing CUri::CComponents::CScheme::construct_from(a_uri_sv)")

    m_state = STATE::undef;

    size_t pos;
    if ((pos = a_uri_sv.find_first_of(':')) == std::string_view::npos) {
        // No separator found means the scheme is undefined. The URI-reference
        // may be a relative reference (RFC3986_4.1). That will be checked
        // later.
        return;
    }
    if (pos == 0) {
        // ':' at the first position means the scheme is empty. This is invalid
        // but will be checked later.
        m_component.clear();
        m_state = STATE::empty;
        return;
    }

    // Strip the view to only have the scheme. If pos > size(), the behavior is
    // undefined. But that is guarded above.
    a_uri_sv.remove_suffix(a_uri_sv.size() - pos);

    // Check if the scheme has valid character and normalize it to lower case
    // character (RFC3986_3.1., RFC3986_6.2.2.1).
    m_component = a_uri_sv;
    if (!std::isalpha(static_cast<unsigned char>(a_uri_sv.front()))) {
        // First character is not alpha.
        goto exception;
    } else {
        unsigned char ch;
        for (auto it{m_component.begin()}; it < m_component.end(); it++) {
            ch = static_cast<unsigned char>(*it);
            if (!(std::isalnum(ch) || (ch == '-') || (ch == '+') ||
                  (ch == '.'))) {
                goto exception;
            }
            *it = static_cast<char>(std::tolower(ch));
        }
    }

    m_state = STATE::avail;
    return;

exception:
    throw std::invalid_argument(
        UPnPsdk_LOGEXCEPT("MSG1157") "Invalid or unsupported scheme=\"" +
        std::string(a_uri_sv) + "\" given.\n");
}


// CUri::CComponents::CAuthority
// =============================
// Constructor
CUri::CComponents::CAuthority::CAuthority(){
    TRACE2(this, " Construct CUri::CComponents::CAuthority()") //
}

// Destructor
CUri::CComponents::CAuthority::~CAuthority() {
    TRACE2(this, " Destruct CUri::CComponents::CAuthority()") //
}

void CUri::CComponents::CAuthority::construct_https_from(
    std::string_view a_uri_sv) {
    // It constructs URI for scheme "https" and "http".

    // The authority component is preceded by a double slash ("//") and is
    // terminated by the next slash ('/'), question mark ('?'), or number sign
    // ('#') character, or by the end of the URI (RFC3986 3.2.). A sender MUST
    // NOT generate an "http" URI with an empty host identifier. A recipient
    // that processes such a URI reference MUST reject it as invalid (RFC7230
    // 2.7.1.). This means an authority component is mandatory.
    TRACE2(this,
           " Executing "
           "CUri::CComponents::CAuthority::construct_https_from(a_uri_sv)")

    std::string_view authority_sv;

    // Extract the authority component with its separator "//".
    auto& npos = std::string_view::npos;
    size_t pos;
    if ((pos = a_uri_sv.find("//")) != npos) {
        authority_sv = a_uri_sv;
        authority_sv.remove_prefix(pos);
        // Find end of the authority and remove the rest to the end.
        if ((pos = authority_sv.find_first_of("/?#", 2)) != npos)
            authority_sv.remove_suffix(authority_sv.size() - pos - 1);
    }

    // Here we have the authority_sv string with separator "//" and end
    // separator (one of '/', '?', '#', '')  or an empty one representing an
    // undefined authority. Having authority_sv only "//" means authority is
    // empty. Construct subcomponents.
    UPnPsdk_LOGINFO("MSG1159") "authority_sv=\"" << authority_sv << "\"\n";
    this->userinfo.construct_https_from(authority_sv);
    this->host.construct_https_from(authority_sv);
    this->port.construct_https_from(authority_sv);
}

void CUri::CComponents::CAuthority::construct_file_from(
    std::string_view a_uri_sv) {
    // It constructs URI for scheme "file".
    //
    // For scheme "file" an empty authority "file:///" is accepted because it
    // has implicit a default authority.host "localhost" defined
    // (RFC3986_3.2.2.). When a scheme defines a default for authority and a
    // URI reference to that default is desired, the reference should be
    // normalized to an empty authority (RFC3986_6.2.3.).
    TRACE2(this, " Executing "
                 "CUri::CComponents::CAuthority::construct_https_scheme_file_"
                 "from(a_uri_sv)")

    std::string_view authority_sv;

    // Extract the authority component with its separator "//".
    auto& npos = std::string_view::npos;
    size_t pos;
    if ((pos = a_uri_sv.find("//")) != npos && pos + 2 < a_uri_sv.size())
        // If there is at least one more any character (e.g. "//x") then an
        // empty host is used that defaults to "localhost". Otherwise an
        // undefined host is detected that should fail the URI.
        authority_sv = "///";

    UPnPsdk_LOGINFO("MSG1159") "authority_sv=\"" << authority_sv << "\"\n";
    this->userinfo.construct_https_from(authority_sv);
    this->host.construct_https_from(authority_sv);
    this->port.construct_https_from(authority_sv);
}

CUri::STATE CUri::CComponents::CAuthority::state() const {
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

std::string CUri::CComponents::CAuthority::str() const {
    return (userinfo.state() == STATE::avail ? userinfo.str() : "") +
           (userinfo.state() == STATE::avail ? "@" : "") +
           (host.state() == STATE::avail ? host.str() : "") +
           (port.state() == STATE::avail ? ":" : "") +
           (port.state() == STATE::avail ? port.str() : "");
}


// CUri::CComponents::CAuthority::CUserinfo
// ========================================
// Constructor
CUri::CComponents::CAuthority::CUserinfo::CUserinfo(){
    TRACE2(this, " Construct CUri::CComponents::CAuthority::CUserinfo()") //
}

// Destructor
CUri::CComponents::CAuthority::CUserinfo::~CUserinfo() {
    TRACE2(this, " Destruct CUri::CComponents::CAuthority::CUserinfo()") //
}

void CUri::CComponents::CAuthority::CUserinfo::construct_https_from(
    std::string_view a_authority_sv) {
    // The user information, if present, is followed by a commercial at-sign
    // ("@") that delimits it from the host (RFC3986 3.2.1.). The userinfo may
    // be undefined (no "@" found within the authority). I also take an empty
    // userinfo into account (first character of the authority is "@").
    // Applications should not render as clear text any password data after the
    // first colon (:) found within a userinfo subcomponent unless the data
    // after the colon is the empty string (indicating no password).
    TRACE2(this,
           " Executing "
           "CUri::CComponents::CAuthority::CUserinfo::construct_https_from(a_"
           "authority_sv)")

    m_state = STATE::undef;

    if (a_authority_sv.empty())
        return;

    auto& npos = std::string_view::npos;
    size_t pos;
    // remove authority separator if any.
    if (a_authority_sv.starts_with("//"))
        a_authority_sv.remove_prefix(2);
    if ((pos = a_authority_sv.find_first_of("/?#")) != npos)
        a_authority_sv.remove_suffix(a_authority_sv.size() - pos);

    // Check if there is a separator, or if it is on the first position (no
    // content preceeding).
    if ((pos = a_authority_sv.find_first_of('@')) == npos)
        return; // No userinfo subcomponent available.

    if (pos == 0 || a_authority_sv[0] == ':') {
        // Separator '@' or ':' (for password) at first position meeans userinfo
        // is empty.
        m_component.clear();
        m_state = STATE::empty;
        return;
    }

    // Extract userinfo
    a_authority_sv.remove_suffix(a_authority_sv.size() - pos);

    // Check special case with clear text password.
    if ((pos = a_authority_sv.find_first_of(':')) != npos) {
        // Here we have found a username with clear text password appended.
        // That will be stripped incl. ':'.
        if (pos == 0) {
            return;
        }
        // Strip deprecated clear text password (RFC3986 3.3.1.).
        a_authority_sv.remove_suffix(a_authority_sv.size() - pos);
    }

    m_component = a_authority_sv;
    m_state = STATE::avail;
    return;
}


// CUri::CComponents::CAuthority::CHost
// ====================================
// Constructor
CUri::CComponents::CAuthority::CHost::CHost(){
    TRACE2(this, " Construct CUri::CComponents::CAuthority::CHost()") //
}

// Destructor
CUri::CComponents::CAuthority::CHost::~CHost() {
    TRACE2(this, " Destruct CUri::CComponents::CAuthority::CHost()") //
}

void CUri::CComponents::CAuthority::CHost::construct_https_from(
    std::string_view a_authority_sv) {
    TRACE2(this, " Executing "
                 "CUri::CComponents::CAuthority::CHost::construct_https_from(a_"
                 "authority_sv)")

    m_state = STATE::undef;

    if (a_authority_sv.empty() || a_authority_sv == "//")
        return;

    if (a_authority_sv == "///") {
        m_component.clear();
        m_state = STATE::empty;
        return;
    }

    auto& npos = std::string_view::npos;
    size_t pos;
    // remove authority separator if any.
    if (a_authority_sv.starts_with("//"))
        a_authority_sv.remove_prefix(2);
    if ((pos = a_authority_sv.find_first_of("/?#")) != npos)
        a_authority_sv.remove_suffix(a_authority_sv.size() - pos);

    // Strip userinfo from authority string if present.
    if ((pos = a_authority_sv.find_first_of('@')) != npos)
        a_authority_sv.remove_prefix(pos + 1);
    // Strip port from authority string if present. I have to look for last
    // ocurance of the separator. The host may have an IPv6 address that also
    // have colons.
    if ((pos = a_authority_sv.find_last_of("]:")) != npos &&
        a_authority_sv[pos] == ':')
        a_authority_sv.remove_suffix(a_authority_sv.size() - pos);

    // Here we have the extracted host string.
    if (a_authority_sv.empty()) {
        m_component.clear();
        m_state = STATE::empty;
        return;
    }

    m_component = a_authority_sv;
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
    // Check IPv4 address and DNS name.
    // The syntax rule for host is ambiguous because it does not completely
    // distinguish between an IPv4address and a reg-name. In order to
    // disambiguate the syntax, we apply the "first-match-wins" algorithm:
    // If host matches the rule for IPv4address, then it should be
    // considered an IPv4 address literal and not a reg-name.
    // (RFC3986_3.2.2.)
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


// CUri::CComponents::CAuthority::CPort
// ====================================
// Constructor
CUri::CComponents::CAuthority::CPort::CPort(){
    TRACE2(this, " Construct CUri::CComponents::CAuthority::CPort()") //
}

// Destructor
CUri::CComponents::CAuthority::CPort::~CPort() {
    TRACE2(this, " Destruct CUri::CComponents::CAuthority::CPort()") //
}

void CUri::CComponents::CAuthority::CPort::construct_https_from(
    std::string_view a_authority_sv) {
    TRACE2(this, " Executing "
                 "CUri::CComponents::CAuthority::CPort::construct_https_from(a_"
                 "authority_sv)")

    m_state = STATE::undef;

    if (a_authority_sv.empty())
        return; // Port is undefined.

    std::string_view authority_sv = a_authority_sv; // Working copy

    auto& npos = std::string_view::npos;
    size_t pos;
    // remove authority separator if any.
    if (authority_sv.starts_with("//"))
        authority_sv.remove_prefix(2);
    // Remove path, query, fragment if any.
    if ((pos = authority_sv.find_first_of("/?#")) != npos)
        authority_sv.remove_suffix(authority_sv.size() - pos);

    // Extract port from authority string if present. I have to look for last
    // ocurance of the separator. The host may have an IPv6 address or a
    // userinfo.password that also have colons.
    // First strip userinfo from authority string if present.
    if ((pos = authority_sv.find_first_of('@')) != npos)
        authority_sv.remove_prefix(pos + 1);
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
    } else if (authority_sv == "443") {
        // Default Port 443 is normalized to undefined port.
        m_state = STATE::undef;
        return;
    }

    // Check if the port string is valid.
    try {
        SSockaddr saObj;
        saObj = authority_sv; // Throws exception std::range_error
    } catch (const std::exception& ex) {
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT("MSG1164") "Catched next line...\n" + ex.what() +
            " Failed \"" + std::string(a_authority_sv) + "\".\n");
    }

    m_component = authority_sv;
    m_state = STATE::avail;
}

void CUri::CComponents::CAuthority::CPort::construct_http_port() {
    // Default Port 80 is normalized to undefined port.
    if (m_state == STATE::avail && m_component == "80") {
        m_state = STATE::undef;
    }
}


// CUri::CComponents::CPath
// ========================
// Constructor
CUri::CComponents::CPath::CPath(){
    TRACE2(this, " Construct CUri::CComponents::CPath()") //
}

// Destructor
CUri::CComponents::CPath::~CPath() {
    TRACE2(this, " Destruct CUri::CComponents::CPath()") //
}

void CUri::CComponents::CPath::construct_https_from(std::string_view a_uri_sv) {
    // If a URI contains an authority component, then the path component must
    // either be empty or begin with a slash ("/") character. The path is
    // terminated by the first question mark ("?") or number sign ("#")
    // character, or by the end of the URI (RFC3986_3.3.). For now I only
    // handle schemes "http", "https", and "file". They must always have an
    // authority component (RFC7230_2.7.1.).
    // In addition, a URI reference may be a relative-path reference, in which
    // case the first path segment cannot contain a colon (":") character
    // (RFC3986_3.3.).
    TRACE2(
        this,
        " Executing CUri::CComponents::CPath::construct_https_from(a_uri_sv)")

    m_state = STATE::undef;

    auto& npos = std::string_view::npos;
    size_t pos;
    // Remove possible scheme.
    if ((pos = a_uri_sv.find_first_of(':')) != npos) {
        a_uri_sv.remove_prefix(pos + 1);
    }
    // Remove possible authority.
    if ((pos = a_uri_sv.find("//")) != npos) {
        // Find start of the path, but ignore empty path '/'.
        if (((pos = a_uri_sv.find_first_of("/?#", pos + 2)) != npos) &&
            (a_uri_sv[pos] == '/') && (pos + 1 < a_uri_sv.size())) {
            a_uri_sv.remove_prefix(pos);
        } else {
            // There is an empty path.
            m_component = '/';
            m_state = STATE::empty;
            return;
        }
    }
    // Remove possible query and/or fragment.
    if ((pos = a_uri_sv.find_first_of("?#")) != npos)
        a_uri_sv.remove_suffix(a_uri_sv.size() - pos);

    if (a_uri_sv.empty() || a_uri_sv == "/") {
        m_component = '/';
        m_state = STATE::empty;
        return;
    }

    m_component = a_uri_sv;
    m_state = STATE::avail;
}

void CUri::CComponents::CPath::remove_dot_segments() {
    // Normalize by removing dot segments in place.
    UPnPsdk::remove_dot_segments(m_component);
}

void CUri::CComponents::CPath::merge(const CComponents& a_base,
                                     const CComponents& a_rel) {
    if (a_base.authority.state() != STATE::undef &&
        a_base.path.state() == STATE::empty) {
        m_component = '/' + a_rel.path.str();
        m_state = STATE::avail;
    } else {
        size_t pos;
        if ((pos = a_base.path.str().find_last_of('/')) == std::string::npos)
            m_component.clear();
        else
            m_component = a_base.path.str().substr(0, pos);
        m_component += '/' + a_rel.path.str();
        m_state = STATE::avail;
    }
}


// CComponents::CQuery
// ===================
// Constructor
CUri::CComponents::CQuery::CQuery(){
    TRACE2(this, " Construct CUri::CComponents::CQuery()") //
}

// Destructor
CUri::CComponents::CQuery::~CQuery() {
    TRACE2(this, " Destruct CUri::CComponents::CQuery()") //
}

void CUri::CComponents::CQuery::construct_https_from(
    std::string_view a_uri_sv) {
    // The query component is indicated by the first question mark ("?")
    // character and terminated by a number sign ("#") character or by the end
    // of the URI. The characters slash ("/") and question mark ("?") may
    // represent data within the query component (RFC3986_3.4.).
    TRACE2(
        this,
        " Executing CUri::CComponents::CQuery::construct_https_from(a_uri_sv)")

    m_state = STATE::undef;

    // Find begin of the query component.
    auto& npos = std::string_view::npos;
    size_t pos;
    if ((pos = a_uri_sv.find_first_of('?')) == npos)
        // No query component found. Leave it undefined.
        return;

    // Strip all before the query component.
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


// CUri::CComponents::CFragment
// ============================
// Constructor
CUri::CComponents::CFragment::CFragment(){
    TRACE2(this, " Construct CUri::CComponents::CFragment()") //
}

// Destructor
CUri::CComponents::CFragment::~CFragment() {
    TRACE2(this, " Destruct CUri::CComponents::CFragment()") //
}

void CUri::CComponents::CFragment::construct_https_from(
    std::string_view a_uri_sv) {
    // A fragment identifier component is indicated by the presence of a number
    // sign ("#") character and terminated by the end of the URI.
    TRACE2(this, " Executing "
                 "CUri::CComponents::CFragment::construct_https_from(a_uri_sv)")

    m_state = STATE::undef;

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


// CUri::CComponents class
// =======================
// Constructor
CUri::CComponents::CComponents(){
    TRACE2(this, " Construct CUri::CComponents()") //
}

// Destructor
CUri::CComponents::~CComponents(){
    TRACE2(this, " Destruct CUri::CComponents()") //
}

// Getter
std::string CUri::CComponents::str() const {
    if (scheme.state() == STATE::avail && scheme.str() == "file")
        return "file:///";
    else
        return (scheme.state() == STATE::avail ? scheme.str() : "") +
               (scheme.state() == STATE::undef ? "" : ":") +
               (authority.state() == STATE::undef ? "" : "//") +
               (authority.state() == STATE::avail ? authority.str() : "") +
               (path.state() == STATE::empty ? "/" : "") +
               (path.state() == STATE::avail ? path.str() : "") +
               (query.state() == STATE::undef ? "" : "?") +
               (query.state() == STATE::avail ? query.str() : "") +
               (fragment.state() == STATE::undef ? "" : "#") +
               (fragment.state() == STATE::avail ? fragment.str() : "");
}


// CUri class
// =====================================
// Constructor
CUri::CUri(std::string a_uriref_str) {
    // It is important that the argument 'a_uriref_str' is given by value. So
    // it is coppied to the stack and constant available for the live time of
    // the constructor and can be used as stable base for string_views.
    TRACE2(this, " Construct CUri(a_uriabs_str)")
    UPnPsdk_LOGINFO("MSG1167") "Construct CUri::CUri(" + a_uriref_str + ")\n";

    // Normalize percent encoded character in place to uppercase.
    parse_percent_encoded_chars(a_uriref_str);

    // Split components
    // ~~~~~~~~~~~~~~~~
    this->base.scheme.construct_from(a_uriref_str);
    const std::string& scheme = this->base.scheme.state() == STATE::avail
                                    ? this->base.scheme.str()
                                    : "";

    if (scheme == "https" || scheme == "http") {
        this->base.authority.construct_https_from(a_uriref_str);
        if (scheme == "http")
            this->base.authority.port.construct_http_port();
    } else if (scheme == "file") {
        this->base.authority.construct_file_from(a_uriref_str);
    } else {
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT(
                "MSG1046") "Ill formed base URI, Empty or unsupported scheme "
                           "specified. Failed \"" +
            a_uriref_str + "\"\n");
    }
    this->base.path.construct_https_from(a_uriref_str);
    this->base.path.remove_dot_segments();
    this->base.query.construct_https_from(a_uriref_str);
    this->base.fragment.construct_https_from(a_uriref_str);

    // Check dependencies of the components
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Manage scheme "https" and "http".
    if (scheme == "https" || scheme == "http") {
        // Scheme "https" and "http" must have a non empty host component.
        if (this->base.authority.host.state() != STATE::avail)
            throw std::invalid_argument(
                UPnPsdk_LOGEXCEPT("MSG1155") "Ill formed base URI. Scheme '" +
                scheme + "' must have a host identifier. Failed \"" +
                a_uriref_str + "\".\n");
    }
    // Manage scheme "file".
    else if (scheme == "file:") {
        // Scheme "file" must have at least an empty host component.
        if (this->base.authority.host.state() == STATE::undef)
            throw std::invalid_argument(
                UPnPsdk_LOGEXCEPT("MSG1163") "Ill formed base URI. Scheme '" +
                scheme + "' must have a host identifier. Failed \"" +
                a_uriref_str + "\".\n");
    }
}


// Destructor
CUri::~CUri() {
    TRACE2(this, " Destruct CUri()") //
}


void CUri::operator=(std::string a_uriref_str) {
    // It is important that the argument 'a_uriref_str' is given by value. So
    // it is coppied to the stack and constant available for the live time of
    // the method and can be used as stable base for string_views.
    TRACE2(this, " Executing CUri::operator=(a_uriref_str)")
    UPnPsdk_LOGINFO("MSG1167") "Executing CUri::operator=(\"" + a_uriref_str +
        "\")\n";

    // Normalize percent encoded character in place to uppercase.
    parse_percent_encoded_chars(a_uriref_str);

    CComponents rel;

    // Split components
    // ~~~~~~~~~~~~~~~~
    rel.scheme.construct_from(a_uriref_str);
    if (this->base.scheme.str() == "file")
        rel.authority.construct_file_from(a_uriref_str);
    else
        rel.authority.construct_https_from(a_uriref_str);
    rel.path.construct_https_from(a_uriref_str);
    rel.query.construct_https_from(a_uriref_str);
    rel.fragment.construct_https_from(a_uriref_str);

    // Transform References
    // --------------------
    // For an overview of the following algorithm have a look to the pseudo code
    // at RFC3986_5.2.2.
    auto& baseObj = this->base;
    auto& targetObj = this->target;
    if (rel.scheme.state() != STATE::undef) {
        targetObj.scheme = rel.scheme;
        targetObj.authority = rel.authority;
        targetObj.path = rel.path;
        targetObj.path.remove_dot_segments();
        targetObj.query = rel.query;
    } else {
        if (rel.authority.state() != STATE::undef) {
            targetObj.authority = rel.authority;
            targetObj.path = rel.path;
            targetObj.path.remove_dot_segments();
            targetObj.query = rel.query;
        } else {
            if (rel.path.state() == STATE::empty) {
                targetObj.path = baseObj.path;
                targetObj.path.remove_dot_segments();
                if (rel.query.state() != STATE::undef)
                    targetObj.query = rel.query;
                else
                    targetObj.query = baseObj.query;
            } else {
                if (rel.path.str().front() == '/') {
                    targetObj.path = rel.path;
                    targetObj.path.remove_dot_segments();
                } else {
                    targetObj.path.merge(baseObj, rel);
                    targetObj.path.remove_dot_segments();
                }
                targetObj.query = rel.query;
            }
            targetObj.authority = baseObj.authority;
        }
        targetObj.scheme = baseObj.scheme;
    }
    targetObj.fragment = rel.fragment;
}


std::string CUri::str() const {
    if (target.scheme.state() == STATE::undef &&
        target.authority.state() == STATE::undef &&
        target.path.state() == STATE::undef &&
        target.query.state() == STATE::undef &&
        target.fragment.state() == STATE::undef)
        return base.str();
    else
        return target.str();
}

} // namespace UPnPsdk
