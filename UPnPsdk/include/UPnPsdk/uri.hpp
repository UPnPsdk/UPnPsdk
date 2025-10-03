#ifndef UPnPsdk_URI_HPP
#define UPnPsdk_URI_HPP
// Copyright (C) 2025+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-10-03
/*!
 * \file
 * \brief Provide the uri class and manage urls.
 */

#include <UPnPsdk/visibility.hpp>

/// \cond
#include <string>
/// \endcond

namespace UPnPsdk {

class UPnPsdk_VIS CUri {
  public:
    // Constructor
    // -----------
    CUri();

    // Destructor
    // ----------
    virtual ~CUri();

    /*! \name Setter
     * *************
     * @{ */
    // Assignment operator to set a uri
    // --------------------------------
    /*! \brief Set uri
     */
    void operator=(
        /// [in] String with a possible uri
        const std::string a_uri_str);
    /// @} Setter


    /*! \name Getter
     * *************
     * @{ */
    // Get uri string
    // --------------
    /*! \brief Get uri string
     */
    std::string_view scheme() const;
    /// @} Getter

  private:
    std::string m_scheme;

    std::string::const_iterator
    parse_scheme(std::string::const_iterator a_scheme_start,
                 const std::string& a_uri);
};

} // namespace UPnPsdk

#endif // UPnPsdk_URI_HPP
