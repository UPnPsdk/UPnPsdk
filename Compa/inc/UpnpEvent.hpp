#ifndef COMPA_UPNPEVENT_HPP
#define COMPA_UPNPEVENT_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-12
// Also Copyright by other contributor as noted below.
//
// Last compare with ./Pupnp source file on 2025-05-22, ver 1.14.20

/*!
 * \file
 * \brief Header file for UpnpEvent methods.
 * \authors Marcelo Roberto Jimenez, Ingo Höft
 */

#include <ixml/ixml.hpp>
#include <UpnpString.hpp>

/*!
 * UpnpEvent
 */
typedef struct s_UpnpEvent UpnpEvent;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*! Constructor */
PUPNP_Api UpnpEvent* UpnpEvent_new(void);
/*! Destructor */
PUPNP_Api void UpnpEvent_delete(UpnpEvent* p);
/*! Copy Constructor */
PUPNP_Api UpnpEvent* UpnpEvent_dup(const UpnpEvent* p);
/*! Assignment operator */
PUPNP_Api int UpnpEvent_assign(UpnpEvent* p, const UpnpEvent* q);

/*! UpnpEvent_get_EventKey */
PUPNP_Api int UpnpEvent_get_EventKey(const UpnpEvent* p);
/*! UpnpEvent_set_EventKey */
PUPNP_Api int UpnpEvent_set_EventKey(UpnpEvent* p, int n);

/*! UpnpEvent_get_ChangedVariables */
PUPNP_Api IXML_Document* UpnpEvent_get_ChangedVariables(const UpnpEvent* p);
/*! UpnpEvent_set_ChangedVariables */
PUPNP_Api int UpnpEvent_set_ChangedVariables(UpnpEvent* p, IXML_Document* n);

/*! UpnpEvent_get_SID */
PUPNP_Api const UpnpString* UpnpEvent_get_SID(const UpnpEvent* p);
/*! UpnpEvent_set_SID */
PUPNP_Api int UpnpEvent_set_SID(UpnpEvent* p, const UpnpString* s);
/*! UpnpEvent_get_SID_Length */
PUPNP_Api size_t UpnpEvent_get_SID_Length(const UpnpEvent* p);
/*! UpnpEvent_get_SID_cstr */
PUPNP_Api const char* UpnpEvent_get_SID_cstr(const UpnpEvent* p);
/*! UpnpEvent_strcpy_SID */
PUPNP_Api int UpnpEvent_strcpy_SID(UpnpEvent* p, const char* s);
/*! UpnpEvent_strncpy_SID */
PUPNP_Api int UpnpEvent_strncpy_SID(UpnpEvent* p, const char* s, size_t n);
/*! UpnpEvent_clear_SID */
PUPNP_Api void UpnpEvent_clear_SID(UpnpEvent* p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COMPA_UPNPEVENT_HPP */
