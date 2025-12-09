#ifndef UPnPsdk_URI_HPP
#define UPnPsdk_URI_HPP
// Copyright (C) 2025+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-12-07
/*!
 * \file
 * \brief Provide the uri class with its 5 components scheme, authority, path,
 * query, and fragment.
 */

#include <UPnPsdk/visibility.hpp>
#include <UPnPsdk/port.hpp>

/// \cond
#include <string>
/// \endcond

namespace UPnPsdk {

// Free functions
// ==============
/*!
 * \brief Removes ".", and ".." from a path.
 *
 * This function directly implements the "Remove Dot Segments" algorithm
 * described in <a
 * href="https://www.rfc-editor.org/rfc/rfc3986#section-5.2.4">RFC 3986 section
 * 5.2.4</a>. If it cannot find something to remove it just do nothing with
 * the path.
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
    /*! [in,out] String representing a path with '/' separators and possible
       containing ".." or "." segments. The path is modified in place. */
    std::string& a_path);


// CUri
/// \brief Class to manage a Uniform Resource Identifier
// =====================================================
class UPnPsdk_VIS CUri {
  public:
    /*! \brief Defines the possible states of a URI component.
     *
     * Quote RFC3986_5.3.:\n
     * "Note that we are careful to preserve the distinction between a
     * component that is undefined, meaning that its separator was not present
     * in the reference, and a component that is empty, meaning that the
     * separator was present and was immediately followed by the next component
     * separator or the end of the reference (RFC3986_5.3.)."
     * <!--
     * The state of a component in this class is not oriented on the URI input
     * reference, but on its output reference taking normalization and
     * comparison into account (RFC3986_6.). For example an input URI reference
     * "https://@[::1]:" will not set authority.userinfo.state() and
     * authority.port.state() to 'STATE::empty' but to 'STATE::undef', because
     * the expected URI output reference is "https://[::1]". --> */
    enum struct STATE {
        undef, ///< The component is undefined means its content is empty.
        empty, ///< The component is empty means it contains only its separator.
        avail /*!< The component is available means it has a valid content,
                 starting with its separator, except the scheme that has its
                 separator appended. */
    };

  private:
    // CComponents class
    // =============
    /*!
     * \brief Provides a Uniform Resource Identifier (URI) and its components
     *
     * This class is based on <a
     * href="https://www.rfc-editor.org/rfc/rfc3986">RFC 3986 - Uniform Resource
     * Identifier (URI): Generic Syntax</a>. For now it supports only scheme
     * 'http', 'https', and 'file' as used by this SDK but may be improved if
     * needed. All of the requirements for the "http" scheme are also
     * requirements for the "https" scheme, except that TCP port 443 is the
     * default instead of TCP port 80 (RFC7230 2.7.2.). The 'file' scheme
     * permits in addition an empty 'authority' component ("file:///") that
     * defaults to "localhost".
     *
     * \exception std::invalid_argument
     *  - if you try to read an undefined component string. Reading an empty
     *    component string is no problem.
     *  - if the URI string to set the object is ill formed. Then the state of
     * the URI object is undefined. You cannot expect valid content, not even
     * empty one.
     */
    class UPnPsdk_VIS CComponents {

      private: // to class CComponents
        // CComponents::CComponent
        // ===================
        class UPnPsdk_VIS CComponent {
          public:
            CComponent();
            ~CComponent();
            STATE state() const;
            std::string& str() const;

          protected:
            // This is only set by the constructor and then constant. That's why
            // I declare it mutable. Getter CComponent::str() (see above) for
            // extern access is declared const.
            STATE m_state{STATE::undef};
            SUPPRESS_MSVC_WARN_4251_NEXT_LINE
            mutable std::string m_component;
        };


        // CComponents::CScheme
        // ================
        /*!
         * \brief Parse scheme component of a URI reference
         *
         * All of the requirements for the "http" scheme are also requirements
         * for the "https" scheme, except that TCP port 443 is the default
         * instead port 80 for "http" (RFC7230 2.7.2.).
         */
        class UPnPsdk_VIS CScheme : public CComponent {
          public:
            CScheme();
            ~CScheme();
            // Next should be called only one time from a constructor.
            void construct_from(std::string_view a_uri_str);
        };


        // CComponents::CAuthority
        // ===================
        class UPnPsdk_VIS CAuthority {

          private:
            // CComponents::CAuthority::CUserinfo
            // ==============================
            class UPnPsdk_VIS CUserinfo : public CComponent {
              public:
                CUserinfo();
                ~CUserinfo();
                // Next should be called only one time from a constructor.
                void construct_https_from(std::string_view a_authority);
            };


            // CComponents::CHost
            // ==============
            class UPnPsdk_VIS CHost : public CComponent {
              public:
                CHost();
                ~CHost();
                // Next should be called only one time from a constructor.
                void construct_https_from(std::string_view a_authority_sv);
            };


            // CComponents::CPort
            // ==============
            class UPnPsdk_VIS CPort : public CComponent {
              public:
                CPort();
                ~CPort();
                // Next should be called only one time from a constructor.
                void construct_https_from(std::string_view a_authority_sv);
                void construct_http_port();
            };


          public:
            CAuthority();
            ~CAuthority();
            // Next should be called only one time from a constructor.
            void construct_https_from(std::string_view a_uri_sv);
            void construct_file_from(std::string_view a_uri_sv);

            STATE state() const;
            std::string str() const;

            ///@{
            /// URI Subcomponent
            CUserinfo userinfo;
            CHost host;
            CPort port;
            ///@}
        };


        // CComponents::CPath
        // ==============
        class UPnPsdk_VIS CPath : public CComponent {
          public:
            CPath();
            ~CPath();
            // Next should be called only one time from a constructor.
            void construct_https_from(std::string_view a_uri_sv);
            void remove_dot_segments();
            void merge(const CComponents& a_base, const CComponents& a_rel);
        };


        // CComponents::CQuery
        // ===============
        class UPnPsdk_VIS CQuery : public CComponent {
          public:
            CQuery();
            ~CQuery();
            // Next should be called only one time from a constructor.
            void construct_https_from(std::string_view a_uri_sv);
        };


        // CComponents::CFragment
        // ==================
        class UPnPsdk_VIS CFragment : public CComponent {
          public:
            CFragment();
            ~CFragment();
            // Next should be called only one time from a constructor.
            void construct_https_from(std::string_view a_uri_sv);
        };


        // CComponents
        // =======
      public:
        // Constructor
        // -----------
        CComponents();

        // Destructor
        // ----------
        ~CComponents();

        // Getter
        // ------
        /// \brief Get the normalized URI string
        std::string str() const;

        ///@{
        /// URI Component
        CScheme scheme;
        CAuthority authority;
        CPath path;
        CQuery query;
        CFragment fragment;
        ///@}
    };


    // CUri
    // ========================================
  public:
    // Default constructor
    // -------------------
    CUri() = delete;

    /// \brief Constructor with setting the base URI
    // ---------------------------------------------
    CUri(
        /// [in] String with a possible absolute URI.
        std::string a_urief_str);


    // Destructor
    // ----------
    ~CUri();


    // Setter
    // ------
    /// \brief Set a relative resource reference
    void operator=(
        /*! [in] String with a possible relative reference to the Base URI from
         * the constructor. */
        std::string a_uriref_str);


    // Getter
    // ------
    /// \brief Get the normalized URI string
    std::string str() const;


    // member variable
    // ---------------
    /// \brief Base URI
    CComponents base;
    /// \brief Resulting URI of merged relative reference to the base URI
    CComponents target;
};

} // namespace UPnPsdk

#endif // UPnPsdk_URI_HPP
