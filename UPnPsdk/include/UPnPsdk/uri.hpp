#ifndef UPnPsdk_URI_HPP
#define UPnPsdk_URI_HPP
// Copyright (C) 2025+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-10-29
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

/*!
 * \brief Provides a Uniform Resource Identifier (URI) and its components
 * \exception std::invalid_argument if the URI string to set the object is ill
 * formed. Then the state of the URI object is undefined. You cannot expect
 * valid content, not even empty one.
 */
class UPnPsdk_VIS CUri {

  public:
    /*! \brief Defines the possible states of a URI component.
     *
     * Note that we (RFC3986) are careful to preserve the distinction between a
     * component that is undefined, meaning that its separator was not present
     * in the reference, and a component that is empty, meaning that the
     * separator was present and was immediately followed by the next component
     * separator or the end of the reference (RFC3986_5.3.). The state of a
     * component in this class is not oriented on the URI input reference, but
     * on its output reference taking normalization and comparison into account
     * (RFC3986_6.). For example an input URI reference "https://@[::1]:" will
     * not set authority.userinfo.state() and authority.port.state() to
     * 'STATE::empty' but to 'STATE::undef', because the expected URI output
     * reference is "https://[::1]/". */
    enum struct STATE {
        undef, /*!< The component is undefined. It may have any uninitialized
                  content. */
        empty, ///< The component is empty. Its content is empty.
        avail ///< The component is available. It has a valid content.
    };

  private: // to class CUri
    // CUri::CComponent
    // ================
    class UPnPsdk_VIS CComponent {
      public:
        CComponent();
        virtual ~CComponent();
        STATE state() const;
        std::string& str() const;

      protected:
        // m_state is STATE::undef: m_component may have any undefined content.
        //            STATE::empty: m_component.empty() == true.
        //            STATE::avail: m_component is available (valid content).
        // This is only set by the constructor and then constant. That's why I
        // declare it mutable. Getter CComponent::str() (see above) for extern
        // access is declared const.
        STATE m_state{STATE::undef};
        SUPPRESS_MSVC_WARN_4251_NEXT_LINE
        mutable std::string m_component;
    };


    // CUri::CScheme
    // =============
    class UPnPsdk_VIS CScheme : public CComponent {
      public:
        CScheme();
        virtual ~CScheme();
        // Next should be called only one time from a constructor.
        void construct_from(std::string_view a_uri_str);
    };


    // CUri::CAuthority
    // ================
    class UPnPsdk_VIS CAuthority {

      private:
        // CUri::CAuthority::CUserinfo
        // ===========================
        class UPnPsdk_VIS CUserinfo : public CComponent {
          public:
            CUserinfo();
            virtual ~CUserinfo();
            void construct_from(std::string_view a_authority);
        };


        // CUri::CHost
        // ===========
        class UPnPsdk_VIS CHost : public CComponent {
          public:
            CHost();
            virtual ~CHost();
            void construct_from(std::string_view a_scheme_stv,
                                std::string_view a_authority_stv);
        };


        // CUri::CPort
        // ===========
        class UPnPsdk_VIS CPort : public CComponent {
          public:
            CPort();
            virtual ~CPort();
            void construct_from(std::string_view a_authority_stv);
        };


      public:
        CAuthority();
        virtual ~CAuthority();
        // Next should be called only one time from a constructor.
        void construct_from(std::string_view a_scheme_stv,
                            std::string_view a_uri_stv);
        STATE state() const;
        std::string str() const;

        ///@{
        /// URI Subcomponent
        CUserinfo userinfo;
        CHost host;
        CPort port;
        ///@}
    };


    // CUri::CPath
    // ===========
    class UPnPsdk_VIS CPath : public CComponent {
      public:
        CPath();
        virtual ~CPath();
        // Next should be called only one time from a constructor.
        void construct_from(std::string_view a_uri_stv);
    };


    // CUri::CQuery
    // ============
    class UPnPsdk_VIS CQuery : public CComponent {
      public:
        CQuery();
        virtual ~CQuery();
        // Next should be called only one time from a constructor.
        void construct_from(std::string_view a_uri_stv);
    };


    // CUri::CFragment
    // ===============
    class UPnPsdk_VIS CFragment : public CComponent {
      public:
        CFragment();
        virtual ~CFragment();
        // Next should be called only one time from a constructor.
        void construct_from(std::string_view a_uri_stv);
    };


    // CUri
    // =====================================
  public:
    // Constructor
    // -----------
    CUri();

    /// \brief Constructor with setting the URI
    // ----------------------------------------
    CUri(
        /// [in] String with a possible URI
        // Getting this string by value to have it on the stack to be more
        // thread safe and usable with string_views.
        const std::string a_uri_str);

    // Destructor
    // ----------
    virtual ~CUri();

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

} // namespace UPnPsdk

#endif // UPnPsdk_URI_HPP
