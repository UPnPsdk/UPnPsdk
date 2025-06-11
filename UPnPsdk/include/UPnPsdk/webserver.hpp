#ifndef UPNPLIB_WEBSERVER_HPP
#define UPNPLIB_WEBSERVER_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-11
/*!
 * \file
 * \brief Declarations to manage the builtin Webserver
 */

#include <UPnPsdk/visibility.hpp>
/// \cond
#include <string>
/// \endcond

namespace UPnPsdk {

/*! \brief Mapping of file extension to content-type of document */
struct Document_meta {
    std::string extension; ///< extension of a filename
    std::string type; ///< file type
    std::string subtype; ///< file subtype
};

/*!
 * \brief Based on the extension, returns the content type and content subtype.
 *
 *  **Example**
 *  \code
 *  #include <UPnPsdk/webserver.hpp>
 *  #include <iostream>
 *  const Document_meta* doc_meta = select_filetype("mp3");
 *  if (doc_meta != nullptr) {
 *      std::cout << "type = " << doc_meta->type
 *                << ", subtype = " << doc_meta->subtype << "\n";
 *  }
 * \endcode
 * \return
 * - \c pointer to a content type structure for a known file extension
 * - \c nullptr if file extension was not known
 */
UPnPsdk_VIS const Document_meta* select_filetype( //
    std::string_view a_extension ///< [in] file extension
);
} // namespace UPnPsdk

#endif // UPNPLIB_WEBSERVER_HPP
