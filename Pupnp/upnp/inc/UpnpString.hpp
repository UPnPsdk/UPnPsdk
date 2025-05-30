#ifndef PUPNP_UPNPSTRING_HPP
#define PUPNP_UPNPSTRING_HPP
// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-05-22
// Also Copyright by other contributor as noted below.
// Last compare with pupnp original source file on 2025-05-22, ver 1.14.20

/*!
 * \defgroup UpnpString The UpnpString Class
 * \brief Implements string operations in the UPnP library.
 * \authors Marcelo Roberto Jimenez, Ingo Höft
 * \version 1.0
 *
 * @{
 *
 * \file
 *
 * \brief UpnpString object declaration.
 */

#include "UpnpGlobal.hpp" /* for EXPORT_SPEC */

#include <stdlib.h>       /* for size_t */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*!
 * \brief Type of the string objects inside libupnp.
 */
typedef struct s_UpnpString UpnpString;

/*!
 * \brief Constructor.
 *
 * \return A pointer to a new allocated object.
 */
EXPORT_SPEC UpnpString* UpnpString_new(void);

/*!
 * \brief Destructor.
 */
EXPORT_SPEC void UpnpString_delete(
    /*! [in] The \em \b this pointer. */
    UpnpString* p);

/*!
 * \brief Copy Constructor.
 *
 * \return A pointer to a new allocated copy of the original object.
 */
EXPORT_SPEC UpnpString* UpnpString_dup(
    /*! [in] The \em \b this pointer. */
    const UpnpString* p);

/*!
 * \brief Assignment operator.
 */
EXPORT_SPEC void UpnpString_assign(
    /*! [in] The \em \b this pointer. */
    UpnpString* p,
    /*! [in] The \em \b that pointer. */
    const UpnpString* q);

/*!
 * \brief Returns the length of the string.
 *
 * \return The length of the string.
 * */
EXPORT_SPEC size_t UpnpString_get_Length(
    /*! [in] The \em \b this pointer. */
    const UpnpString* p);

/*!
 * \brief Truncates the string to the specified lenght, or does nothing
 * if the current lenght is less than or equal to the requested length.
 * */
EXPORT_SPEC void UpnpString_set_Length(
    /*! [in] The \em \b this pointer. */
    UpnpString* p,
    /*! [in] The requested length. */
    size_t n);

/*!
 * \brief Returns the pointer to char.
 *
 * \return The pointer to char.
 */
EXPORT_SPEC const char* UpnpString_get_String(
    /*! [in] The \em \b this pointer. */
    const UpnpString* p);

/*!
 * \brief Sets the string from a pointer to char.
 */
EXPORT_SPEC int UpnpString_set_String(
    /*! [in] The \em \b this pointer. */
    UpnpString* p,
    /*! [in] (char *) to copy from. */
    const char* s);

/*!
 * \brief Sets the string from a pointer to char using a maximum of N chars.
 */
EXPORT_SPEC int UpnpString_set_StringN(
    /*! [in] The \em \b this pointer. */
    UpnpString* p,
    /*! [in] (char *) to copy from. */
    const char* s,
    /*! Maximum number of chars to copy.*/
    size_t n);

/*!
 * \brief Clears the string, sets its size to zero.
 */
EXPORT_SPEC void UpnpString_clear(
    /*! [in] The \em \b this pointer. */
    UpnpString* p);

/*!
 * \brief Compares two strings for equality. Case matters.
 *
 * \return The result of strcmp().
 */
EXPORT_SPEC int UpnpString_cmp(
    /*! [in] The \em \b the first string. */
    UpnpString* p,
    /*! [in] The \em \b the second string. */
    UpnpString* q);

/*!
 * \brief Compares two strings for equality. Case does not matter.
 *
 * \return The result of strcasecmp().
 */
EXPORT_SPEC int UpnpString_casecmp(
    /*! [in] The \em \b the first string. */
    UpnpString* p,
    /*! [in] The \em \b the second string. */
    UpnpString* q);

/* @} UpnpString The UpnpString API */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PUPNP_UPNPSTRING_HPP */
