#ifndef UPnPsdk_URI_HPP
#define UPnPsdk_URI_HPP
// Copyright (C) 2025+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2026-02-23
/*!
 * \file
 * \brief Manage Uniform Resource Identifier (URI) as specified with <a
 * href="https://www.rfc-editor.org/rfc/rfc3986">RFC 3986</a>.
 */

#include <UPnPsdk/visibility.hpp>

/// \cond
#include <string>
/// \endcond

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

} // namespace UPnPsdk

#endif // UPnPsdk_URI_HPP
