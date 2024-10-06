#ifndef UPnPsdk_GLOBAL_HPP
#define UPnPsdk_GLOBAL_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-10-05
/*!
 * \file
 * \brief Global used constants and variables.
 */

// Due to the global nature of this header file additional #include statements
// should be taken with great care. They are included in nearly all other
// compile units.
#include <UPnPsdk/global.ipp>

namespace UPnPsdk {

// Global variables
// ================
/*!
 * \brief Switch to enable verbose (debug) output.
 *
 * This flag is only modified by user intervention, e.g. on the command line or
 * with an environment variable but never modified by the production code. Only
 * Unit tests may toggle the switch under test.
 */
UPnPsdk_EXTERN bool g_dbug;

} // namespace UPnPsdk

#endif // UPnPsdk_GLOBAL_HPP
