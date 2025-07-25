#ifdef UPNP_HAVE_CLIENT

#ifndef GENLIBCLIENTSUBSCRIPTION_HPP
#define GENLIBCLIENTSUBSCRIPTION_HPP
// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-07-19
// Based on source code from author Marcelo Roberto Jimenez.

/*!
 * \file
 *
 * \brief Header file for GenlibClientSubscription methods.
 *
 * \author Marcelo Roberto Jimenez
 */
#include <stdlib.h>       /* for size_t */

#include "UpnpGlobal.hpp" /* for EXPORT_SPEC */

#include "UpnpString.hpp"

/*!
 * GenlibClientSubscription
 */
typedef struct s_GenlibClientSubscription GenlibClientSubscription;

/*! Constructor */
EXPORT_SPEC GenlibClientSubscription* GenlibClientSubscription_new(void);
/*! Destructor */
EXPORT_SPEC void GenlibClientSubscription_delete(GenlibClientSubscription* p);
/*! Copy Constructor */
EXPORT_SPEC GenlibClientSubscription*
GenlibClientSubscription_dup(const GenlibClientSubscription* p);
/*! Assignment operator */
EXPORT_SPEC int
GenlibClientSubscription_assign(GenlibClientSubscription* p,
                                const GenlibClientSubscription* q);

/*! GenlibClientSubscription_get_RenewEventId */
EXPORT_SPEC int
GenlibClientSubscription_get_RenewEventId(const GenlibClientSubscription* p);
/*! GenlibClientSubscription_set_RenewEventId */
EXPORT_SPEC int
GenlibClientSubscription_set_RenewEventId(GenlibClientSubscription* p, int n);

/*! GenlibClientSubscription_get_SID */
EXPORT_SPEC const UpnpString*
GenlibClientSubscription_get_SID(const GenlibClientSubscription* p);
/*! GenlibClientSubscription_set_SID */
EXPORT_SPEC int GenlibClientSubscription_set_SID(GenlibClientSubscription* p,
                                                 const UpnpString* s);
/*! GenlibClientSubscription_get_SID_Length */
EXPORT_SPEC size_t
GenlibClientSubscription_get_SID_Length(const GenlibClientSubscription* p);
/*! GenlibClientSubscription_get_SID_cstr */
EXPORT_SPEC const char*
GenlibClientSubscription_get_SID_cstr(const GenlibClientSubscription* p);
/*! GenlibClientSubscription_strcpy_SID */
EXPORT_SPEC int GenlibClientSubscription_strcpy_SID(GenlibClientSubscription* p,
                                                    const char* s);
/*! GenlibClientSubscription_strncpy_SID */
EXPORT_SPEC int
GenlibClientSubscription_strncpy_SID(GenlibClientSubscription* p, const char* s,
                                     size_t n);
/*! GenlibClientSubscription_clear_SID */
EXPORT_SPEC void
GenlibClientSubscription_clear_SID(GenlibClientSubscription* p);

/*! GenlibClientSubscription_get_ActualSID */
EXPORT_SPEC const UpnpString*
GenlibClientSubscription_get_ActualSID(const GenlibClientSubscription* p);
/*! GenlibClientSubscription_set_ActualSID */
EXPORT_SPEC int
GenlibClientSubscription_set_ActualSID(GenlibClientSubscription* p,
                                       const UpnpString* s);
/*! GenlibClientSubscription_get_ActualSID_Length */
EXPORT_SPEC size_t GenlibClientSubscription_get_ActualSID_Length(
    const GenlibClientSubscription* p);
/*! GenlibClientSubscription_get_ActualSID_cstr */
EXPORT_SPEC const char*
GenlibClientSubscription_get_ActualSID_cstr(const GenlibClientSubscription* p);
/*! GenlibClientSubscription_strcpy_ActualSID */
EXPORT_SPEC int
GenlibClientSubscription_strcpy_ActualSID(GenlibClientSubscription* p,
                                          const char* s);
/*! GenlibClientSubscription_strncpy_ActualSID */
EXPORT_SPEC int
GenlibClientSubscription_strncpy_ActualSID(GenlibClientSubscription* p,
                                           const char* s, size_t n);
/*! GenlibClientSubscription_clear_ActualSID */
EXPORT_SPEC void
GenlibClientSubscription_clear_ActualSID(GenlibClientSubscription* p);

/*! GenlibClientSubscription_get_EventURL */
EXPORT_SPEC const UpnpString*
GenlibClientSubscription_get_EventURL(const GenlibClientSubscription* p);
/*! GenlibClientSubscription_set_EventURL */
EXPORT_SPEC int
GenlibClientSubscription_set_EventURL(GenlibClientSubscription* p,
                                      const UpnpString* s);
/*! GenlibClientSubscription_get_EventURL_Length */
EXPORT_SPEC size_t
GenlibClientSubscription_get_EventURL_Length(const GenlibClientSubscription* p);
/*! GenlibClientSubscription_get_EventURL_cstr */
EXPORT_SPEC const char*
GenlibClientSubscription_get_EventURL_cstr(const GenlibClientSubscription* p);
/*! GenlibClientSubscription_strcpy_EventURL */
EXPORT_SPEC int
GenlibClientSubscription_strcpy_EventURL(GenlibClientSubscription* p,
                                         const char* s);
/*! GenlibClientSubscription_strncpy_EventURL */
EXPORT_SPEC int
GenlibClientSubscription_strncpy_EventURL(GenlibClientSubscription* p,
                                          const char* s, size_t n);
/*! GenlibClientSubscription_clear_EventURL */
EXPORT_SPEC void
GenlibClientSubscription_clear_EventURL(GenlibClientSubscription* p);

/*! GenlibClientSubscription_get_Next */
EXPORT_SPEC GenlibClientSubscription*
GenlibClientSubscription_get_Next(const GenlibClientSubscription* p);
/*! GenlibClientSubscription_set_Next */
EXPORT_SPEC int GenlibClientSubscription_set_Next(GenlibClientSubscription* p,
                                                  GenlibClientSubscription* n);

#endif /* GENLIBCLIENTSUBSCRIPTION_HPP */

#endif // UPNP_HAVE_CLIENT
