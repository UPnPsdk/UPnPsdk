#ifndef COMPA_UPNPSTRING_HPP
#define COMPA_UPNPSTRING_HPP
// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-05-22
// Also Copyright by other contributor as noted below.
// Last compare with ./Pupnp source file on 2025-05-22, ver 1.14.20
/*!
 * \file
 * \brief UpnpString object declaration.
 */

/*!
 * \defgroup UpnpString The UpnpString Class
 * \brief Implements string operations in the UPnP library.
 * \authors Marcelo Roberto Jimenez, Ingo Höft
 * \version 1.0
 * @{
 */

#include <UPnPsdk/visibility.hpp>
/// \cond
#include <cstddef> // For size_t
/// \endcond

extern "C" {

/*!
 * \brief Type of the string objects inside libupnp.
 */
// The typedef must be the same as in pupnp otherwise we cannot switch between
// pupnp gtest and compa gtest. Using the typedef in the header file but the
// definiton of the structure in the source file make the mmembers of the
// structure publicy invisible. That is intended but we will change it with
// using C++ private. --Ingo
typedef struct s_UpnpString UpnpString;

/*!
 * \brief Constructor.
 *
 * \return A pointer to a new allocated object.
 */
PUPNP_API UpnpString* UpnpString_new();

/*!
 * \brief Destructor.
 */
PUPNP_API void UpnpString_delete(
    /*! [in] The \em \b this pointer. */
    UpnpString* p);

/*!
 * \brief Copy Constructor.
 *
 * \return A pointer to a new allocated copy of the original object.
 */
PUPNP_API UpnpString* UpnpString_dup(
    /*! [in] The \em \b this pointer. */
    const UpnpString* p);

/*!
 * \brief Assignment operator.
 */
PUPNP_API void UpnpString_assign(
    /*! [in] The \em \b this pointer. */
    UpnpString* p,
    /*! [in] The \em \b that pointer. */
    const UpnpString* q);

/*!
 * \brief Returns the length of the string.
 *
 * \return The length of the string.
 * */
PUPNP_API size_t UpnpString_get_Length(
    /*! [in] The \em \b this pointer. */
    const UpnpString* p);

/*!
 * \brief Truncates the string to the specified lenght, or does nothing
 * if the current lenght is less than or equal to the requested length.
 * */
PUPNP_API void UpnpString_set_Length(
    /*! [in] The \em \b this pointer. */
    UpnpString* p,
    /*! [in] The requested length. */
    size_t n);

/*!
 * \brief Returns the pointer to char.
 *
 * \return The pointer to char.
 * \hidecallergraph
 */
PUPNP_API const char* UpnpString_get_String(
    /*! [in] The \em \b this pointer. */
    const UpnpString* p);

/*!
 * \brief Sets the string from a pointer to char.
 * \hidecallergraph
 */
PUPNP_API int UpnpString_set_String(
    /*! [in] The \em \b this pointer. */
    UpnpString* p,
    /*! [in] (char *) to copy from. */
    const char* s);

/*!
 * \brief Sets the string from a pointer to char using a maximum of N chars.
 */
PUPNP_API int UpnpString_set_StringN(
    /*! [in] The \em \b this pointer. */
    UpnpString* p,
    /*! [in] (char *) to copy from. */
    const char* s,
    /*! Maximum number of chars to copy.*/
    size_t n);

/*!
 * \brief Clears the string, sets its size to zero.
 */
PUPNP_API void UpnpString_clear(
    /*! [in] The \em \b this pointer. */
    UpnpString* p);

/*!
 * \brief Compares two strings for equality. Case matters.
 *
 * \return The result of strcmp().
 */
PUPNP_API int UpnpString_cmp(
    /*! [in] The \em \b the first string. */
    UpnpString* p,
    /*! [in] The \em \b the second string. */
    UpnpString* q);

/*!
 * \brief Compares two strings for equality. Case does not matter.
 *
 * \return The result of strcasecmp().
 */
PUPNP_API int UpnpString_casecmp(
    /*! [in] The \em \b the first string. */
    UpnpString* p,
    /*! [in] The \em \b the second string. */
    UpnpString* q);

} // extern "C"

/// @} UpnpString The UpnpString API

#endif // COMPA_UPNPSTRING_HPP
