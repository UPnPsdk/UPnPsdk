#ifndef UPnPsdk_UPNPTOOLS_HPP
#define UPnPsdk_UPNPTOOLS_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-11
/*!
 * \file
 * \brief General usable free function tools and helper.
 */

#include <UPnPsdk/visibility.hpp>
#include <string>

namespace UPnPsdk {

/*! \brief Get error name string.
 * \return Name string of the error */
UPnPsdk_VIS const std::string errStr( //
    int error ///< [in] Error number
);

/*! \brief Get extended error name string.
 * \return Error message with hint what should be correct */
UPnPsdk_VIS const std::string errStrEx( //
    const int error, /*!< [in] Error number */
    const int success /*!< [in] Message number that should be given instead of
                         the error */
);

} // namespace UPnPsdk

#endif // UPnPsdk_UPNPTOOLS_HPP
