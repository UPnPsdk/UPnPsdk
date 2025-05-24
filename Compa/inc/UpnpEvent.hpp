#ifndef COMPA_UPNPEVENT_HPP
#define COMPA_UPNPEVENT_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-05-22
// Also Copyright by other contributor as noted below.
//
// Last compare with ./Pupnp source file on 2025-05-22, ver 1.14.20

/*!
 * \file
 * \brief Header file for UpnpEvent methods.
 * \authors Marcelo Roberto Jimenez, Ingo Höft
 */

#include <ixml.hpp>
#include <UpnpString.hpp>

/*!
 * UpnpEvent
 */
typedef struct s_UpnpEvent UpnpEvent;

extern "C" {

/*! Constructor */
PUPNP_API UpnpEvent* UpnpEvent_new();
/*! Destructor */
PUPNP_API void UpnpEvent_delete(UpnpEvent* p);
/*! Copy Constructor */
PUPNP_API UpnpEvent* UpnpEvent_dup(const UpnpEvent* p);
/*! Assignment operator */
PUPNP_API int UpnpEvent_assign(UpnpEvent* p, const UpnpEvent* q);

/*! UpnpEvent_get_EventKey */
PUPNP_API int UpnpEvent_get_EventKey(const UpnpEvent* p);
/*! UpnpEvent_set_EventKey */
PUPNP_API int UpnpEvent_set_EventKey(UpnpEvent* p, int n);

/*! UpnpEvent_get_ChangedVariables */
PUPNP_API IXML_Document* UpnpEvent_get_ChangedVariables(const UpnpEvent* p);
/*! UpnpEvent_set_ChangedVariables */
PUPNP_API int UpnpEvent_set_ChangedVariables(UpnpEvent* p, IXML_Document* n);

/*! UpnpEvent_get_SID */
PUPNP_API const UpnpString* UpnpEvent_get_SID(const UpnpEvent* p);
/*! UpnpEvent_set_SID */
PUPNP_API int UpnpEvent_set_SID(UpnpEvent* p, const UpnpString* s);
/*! UpnpEvent_get_SID_Length */
PUPNP_API size_t UpnpEvent_get_SID_Length(const UpnpEvent* p);
/*! UpnpEvent_get_SID_cstr */
PUPNP_API const char* UpnpEvent_get_SID_cstr(const UpnpEvent* p);
/*! UpnpEvent_strcpy_SID */
PUPNP_API int UpnpEvent_strcpy_SID(UpnpEvent* p, const char* s);
/*! UpnpEvent_strncpy_SID */
PUPNP_API int UpnpEvent_strncpy_SID(UpnpEvent* p, const char* s, size_t n);
/*! UpnpEvent_clear_SID */
PUPNP_API void UpnpEvent_clear_SID(UpnpEvent* p);

} // extern "C"

#endif /* COMPA_UPNPEVENT_HPP */
