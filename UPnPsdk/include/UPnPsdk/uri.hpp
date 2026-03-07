#ifndef UPnPsdk_URI_HPP
#define UPnPsdk_URI_HPP
// Copyright (C) 2025+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-03-08
/*!
 * \file
 * \brief Manage Uniform Resource Identifier (URI) as specified with <a
 * href="https://www.rfc-editor.org/rfc/rfc3986">RFC 3986</a>.
 */

#include <UPnPsdk/visibility.hpp>
#include <UPnPsdk/port.hpp>
#include <UPnPsdk/port_sock.hpp>

/// \cond
#include <string>
/// \endcond


/// Yet another success code.
inline constexpr int HTTP_SUCCESS{1};

/// Type of the URI.
// Must not use ABSOLUTE, RELATIVE; already defined in Win32 for other meaning.
enum struct uriType {
    Absolute, ///< The URI is absolute, means it has a 'scheme' component.
    Relative ///< The URI is relative, means it hasn't a 'scheme' component.
};

/// Type of the 'path' part of the URI.
enum struct pathType {
    ABS_PATH, ///< The 'path' component begins with a '/'.
    REL_PATH, ///< The 'path' component doesn't begin with a '/'.
    OPAQUE_PART /*! A URI is opaque if, and only if, it is absolute and its
                   'scheme'-specific part does not begin with a slash character
                   ('/'). An opaque URI has a 'scheme', a 'scheme'-specific
                   part, and possibly a 'fragment'; all other components are
                   undefined. A typical example of an opaque uri is a mail to
                   url "mailto:a@b.com". (<a href="
                   http://docs.oracle.com/javase/8/docs/api/java/net/URI.html#isOpaque--">source</a>)
                 */
};

/*!
 * \brief Buffer used in parsing http messages, urls, etc. Generally this
 * simply holds a pointer into a larger array.
 */
struct token {
    const char* buff; ///< Buffer
    size_t size; ///< Size of the buffer
};

/*!
 * \brief Represents a host port, e.g. "[::1]:443".
 */
struct hostport_type {
    token text; ///< Pointing to the full host:port string representation.
    sockaddr_storage IPaddress; ///< Network socket address.
};

/*!
 * \brief Represents a URI used in parse_uri and elsewhere.
 */
struct uri_type {
    /// @{
    /// \brief Member variable
    uriType type;
    token scheme;
    pathType path_type;
    token pathquery;
    token fragment;
    hostport_type hostport;
    /// @}
};

namespace UPnPsdk {

// Free functions
// ==============
/*!
 * \brief Remove dot segments from a path
 * \ingroup upnpsdk-uri
 *
 * This function directly implements the "Remove Dot Segments" algorithm
 * described in <a
 * href="https://www.rfc-editor.org/rfc/rfc3986#section-5.2.4">RFC 3986 section
 * 5.2.4</a>. The path is modified in place. If it cannot find something to
 * remove it just do nothing with the path.
 *
 * Examples:
\code
remove_dot_segments("/foo/./bar"); // results to "/foo/bar"
remove_dot_segments("/foo/../bar"); // results to "/bar"
remove_dot_segments("../bar"); // results to "bar"
remove_dot_segments("./bar"); // results to "bar"
remove_dot_segments(".../bar"); // results to ".../bar" (do nothing)
remove_dot_segments("/./hello/foo/../bar"); // results to "/hello/bar"
\endcode
 */
UPnPsdk_VIS void remove_dot_segments(
    /*! [in,out] Reference of a string representing a path with '/' separators
       and possible containing ".." or "." segments. */
    std::string& a_path);


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
    /// Current state of the component.
    STATE m_state{STATE::undef};
    /// \cond
    DISABLE_MSVC_WARN_4251
    /// \endcond
    /// Name of the component.
    std::string m_component;
    /// \cond
    ENABLE_MSVC_WARN
    /// \endcond
};


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
     * \exception std::invalid_argument if [URI reference](\ref glossary_URIref)
     * with invalid host address or host name pattern is detected. No DNS lookup
     * is performed. */
    CHost(std::string_view a_uri_sv ///< [in] Input URI string
    );
};


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


// Class CPrepUriStr
// =================
/*!
 * \brief Internal class to prepare the input URI string, Not publicly usable
 * \ingroup upnpsdk-uri
 */
class CPrepUriStr {
  public:
    /*! \brief Initialize the helper class
     *
     * Normalize percent encoded characters of a URI reference string to
     * upper case hex digits (<a
     * href="https://www.rfc-editor.org/rfc/rfc3986#section-2.1">RFC3986_2.1.</a>)
     * \ingroup upnpsdk-uri
     *
     * Corrections are made in place. The size of the corrected string doesn't
     * change.
     *
     * \exception std::invalid_argument if invalid percent encoding is detected.
     */
    CPrepUriStr(std::string& a_uriref_str ///< [in] Input URI string
    );
};


// Class CUriRef
// =============
/*!
 * \brief This is a [URI reference](\ref glossary_URIref).
 * \ingroup upnpsdk-uri
 */
class CUriRef {
  private:
    using STATE = CComponent::STATE;

    // member classes
    // --------------
    // Next attribute must be declared first due to correct initialization of
    // the following five component member classes.
    /// \brief Prepare input URI string
    CPrepUriStr prepare_uri_str;

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
     *  - if port number is invalid.
     */
    // It is important that the argument 'a_uriref_str' is given by value. So
    // it is coppied to the stack and available for the lifetime of the
    // constructor and can be used as stable base for string_views.
    CUriRef(std::string a_uriref_str ///< [in] Input URI string
    );

    /// Get state of the URI reference
    CComponent::STATE state() const;

    /// Get URI reference string
    std::string str() const;
};

} // namespace UPnPsdk


/*!
 * \brief Parses a uri as defined in <a
 * href="https://www.rfc-editor.org/rfc/rfc3986"> RFC 3986 (Uniform Resource
 * Identifier)</a>.
 * \ingroup upnpsdk-uri
 *
 * Handles absolute, relative, and opaque uris. Parses into the following
 * pieces: scheme, hostport, pathquery, fragment (host with port and path with
 * query are treated as one token). Strings in output uri_type are treated as
 * token with character chain and size. They are not null ('\0') terminated.
 *
 * Caller should check for the pieces they require.
 *
 * \returns
 *  On success: HTTP_SUCCESS\n
 *  On error: UPNP_E_INVALID_URL, accessing \b out (arg3) then, is undefined
 *            behavior.
 */
UPnPsdk_VIS int parse_uri(
    /*! [in] Character string containing uri information to be parsed. It is
     * not expected to be terminated with zero ('\0'), but may have. */
    const char* in,
    /*! [in] Number of characters (strlen()) of the input string. */
    size_t max,
    /*! [out] Output parameter which will have the parsed uri information.
     */
    uri_type* out);

#endif // UPnPsdk_URI_HPP
