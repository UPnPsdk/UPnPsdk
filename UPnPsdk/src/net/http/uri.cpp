// Copyright (C) 2025+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-10-03
/*!
 * \file
 * \brief Provide the uri class and manage urls.
 */

#include <UPnPsdk/uri.hpp>
#include <UPnPsdk/synclog.hpp>

namespace UPnPsdk {

// Constructor
CUri::CUri(){
    TRACE2(this, " Construct CUri()") //
}

// Destructor
CUri::~CUri() {
    TRACE2(this, " Destruct CUri()")
}

// Assignment operator to set a uri
// --------------------------------
void CUri::operator=(const std::string a_uri_str) {
    TRACE2(this, " Executing CUri::operator=(\"" + a_uri_str + "\")")

    [[maybe_unused]] std::string::const_iterator finger =
        this->parse_scheme(a_uri_str.begin(), a_uri_str);
}

// Get the scheme
// --------------
std::string_view CUri::scheme() const { return m_scheme; }

// Private methods
// ===============
// parse_scheme
// ------------
std::string::const_iterator
CUri::parse_scheme(std::string::const_iterator a_scheme_start,
                   const std::string& a_uri) {
    TRACE2(this, " Executing CUri::parse_scheme()")

    std::string::const_iterator scheme_end;
    for (scheme_end = a_scheme_start;
         (*scheme_end != ':') && (scheme_end != a_uri.end()); scheme_end++) //
    {
        if (!(std::isalnum(static_cast<unsigned char>(*scheme_end)) ||
              (*scheme_end == '-') || (*scheme_end == '+') ||
              (*scheme_end == '.')))
            throw std::invalid_argument(
                UPnPsdk_LOGEXCEPT(
                    "MSG1157") "Malformed URI, scheme with invalid char: \"" +
                a_uri + "\"\n");
    }

    if (a_scheme_start == scheme_end)
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT(
                "MSG1046") "Malformed URI, scheme with zero-length: \"" +
            a_uri + "\"\n");
    if (scheme_end == a_uri.end())
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT("MSG1154") "Malformed URI, no scheme found: \"" +
            a_uri + "\"\n");
    if (!std::isalpha(static_cast<unsigned char>(*a_scheme_start)))
        throw std::invalid_argument(
            UPnPsdk_LOGEXCEPT("MSG1155") "Malformed URI, scheme first char "
                                         "must be alpha: \"" +
            a_uri + "\"\n");

    m_scheme = std::string(a_scheme_start, scheme_end);
    return scheme_end + 1; // Points now behind scheme delimiter ':'.
}

} // namespace UPnPsdk
