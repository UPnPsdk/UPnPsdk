#ifdef COMPA_HAVE_CTRLPT_SSDP

#ifndef COMPA_GENLIB_CLIENTSUBSCRIPTION_HPP
#define COMPA_GENLIB_CLIENTSUBSCRIPTION_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-05-21
/*!
 * \file
 * \brief Header file for GenlibClientSubscription methods.
 * \authors Marcelo Roberto Jimenez, Ingo Höft
 */

#include <UpnpString.hpp>

/*!
 * GenlibClientSubscription
 */
typedef struct s_GenlibClientSubscription GenlibClientSubscription;

/*! Constructor */
PUPNP_EXP GenlibClientSubscription* GenlibClientSubscription_new();
/*! Destructor */
PUPNP_EXP void GenlibClientSubscription_delete(GenlibClientSubscription* p);
/*! Copy Constructor */
PUPNP_EXP GenlibClientSubscription*
GenlibClientSubscription_dup(const GenlibClientSubscription* p);
/*! Assignment operator */
PUPNP_EXP int
GenlibClientSubscription_assign(GenlibClientSubscription* p,
                                const GenlibClientSubscription* q);

/*! GenlibClientSubscription_get_RenewEventId */
PUPNP_EXP int
GenlibClientSubscription_get_RenewEventId(const GenlibClientSubscription* p);
/*! GenlibClientSubscription_set_RenewEventId */
PUPNP_EXP int
GenlibClientSubscription_set_RenewEventId(GenlibClientSubscription* p, int n);

/*! GenlibClientSubscription_get_SID */
PUPNP_EXP const UpnpString*
GenlibClientSubscription_get_SID(const GenlibClientSubscription* p);
/*! GenlibClientSubscription_set_SID */
PUPNP_EXP int GenlibClientSubscription_set_SID(GenlibClientSubscription* p,
                                               const UpnpString* s);
/*! GenlibClientSubscription_get_SID_Length */
PUPNP_EXP size_t
GenlibClientSubscription_get_SID_Length(const GenlibClientSubscription* p);
/*! GenlibClientSubscription_get_SID_cstr */
PUPNP_EXP const char*
GenlibClientSubscription_get_SID_cstr(const GenlibClientSubscription* p);
/*! GenlibClientSubscription_strcpy_SID */
PUPNP_EXP int GenlibClientSubscription_strcpy_SID(GenlibClientSubscription* p,
                                                  const char* s);
/*! GenlibClientSubscription_strncpy_SID */
PUPNP_EXP int GenlibClientSubscription_strncpy_SID(GenlibClientSubscription* p,
                                                   const char* s, size_t n);
/*! GenlibClientSubscription_clear_SID */
PUPNP_EXP void GenlibClientSubscription_clear_SID(GenlibClientSubscription* p);

/*! GenlibClientSubscription_get_ActualSID */
PUPNP_EXP const UpnpString*
GenlibClientSubscription_get_ActualSID(const GenlibClientSubscription* p);
/*! GenlibClientSubscription_set_ActualSID */
PUPNP_EXP int
GenlibClientSubscription_set_ActualSID(GenlibClientSubscription* p,
                                       const UpnpString* s);
/*! GenlibClientSubscription_get_ActualSID_Length */
PUPNP_EXP size_t GenlibClientSubscription_get_ActualSID_Length(
    const GenlibClientSubscription* p);
/*! GenlibClientSubscription_get_ActualSID_cstr */
PUPNP_EXP const char*
GenlibClientSubscription_get_ActualSID_cstr(const GenlibClientSubscription* p);
/*! GenlibClientSubscription_strcpy_ActualSID */
PUPNP_EXP int
GenlibClientSubscription_strcpy_ActualSID(GenlibClientSubscription* p,
                                          const char* s);
/*! GenlibClientSubscription_strncpy_ActualSID */
PUPNP_EXP int
GenlibClientSubscription_strncpy_ActualSID(GenlibClientSubscription* p,
                                           const char* s, size_t n);
/*! GenlibClientSubscription_clear_ActualSID */
PUPNP_EXP void
GenlibClientSubscription_clear_ActualSID(GenlibClientSubscription* p);

/*! GenlibClientSubscription_get_EventURL */
PUPNP_EXP const UpnpString*
GenlibClientSubscription_get_EventURL(const GenlibClientSubscription* p);
/*! GenlibClientSubscription_set_EventURL */
PUPNP_EXP int GenlibClientSubscription_set_EventURL(GenlibClientSubscription* p,
                                                    const UpnpString* s);
/*! GenlibClientSubscription_get_EventURL_Length */
PUPNP_EXP size_t
GenlibClientSubscription_get_EventURL_Length(const GenlibClientSubscription* p);
/*! GenlibClientSubscription_get_EventURL_cstr */
PUPNP_EXP const char*
GenlibClientSubscription_get_EventURL_cstr(const GenlibClientSubscription* p);
/*! GenlibClientSubscription_strcpy_EventURL */
PUPNP_EXP int
GenlibClientSubscription_strcpy_EventURL(GenlibClientSubscription* p,
                                         const char* s);
/*! GenlibClientSubscription_strncpy_EventURL */
PUPNP_EXP int
GenlibClientSubscription_strncpy_EventURL(GenlibClientSubscription* p,
                                          const char* s, size_t n);
/*! GenlibClientSubscription_clear_EventURL */
PUPNP_EXP void
GenlibClientSubscription_clear_EventURL(GenlibClientSubscription* p);

/*! GenlibClientSubscription_get_Next */
PUPNP_EXP GenlibClientSubscription*
GenlibClientSubscription_get_Next(const GenlibClientSubscription* p);
/*! GenlibClientSubscription_set_Next */
PUPNP_EXP int GenlibClientSubscription_set_Next(GenlibClientSubscription* p,
                                                GenlibClientSubscription* n);

#endif // COMPA_GENLIB_CLIENTSUBSCRIPTION_HPP
#endif // COMPA_HAVE_CTRLPT_SSDP
