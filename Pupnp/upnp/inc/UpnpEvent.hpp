#ifndef UPNPLIB_UPNPEVENT_HPP
#define UPNPLIB_UPNPEVENT_HPP
// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-01
// Also Copyright by other contributor as noted below.
//
// Last compare with pupnp original source file on 2025-05-22, ver 1.14.20

/*!
 * \file
 *
 * \brief Header file for UpnpEvent methods.
 * \authors Marcelo Roberto Jimenez, Ingo Höft
 */
#include <stdlib.h>       /* for size_t */

#include "UpnpGlobal.hpp" /* for EXPORT_SPEC */

#include "ixml.hpp"
#include "UpnpString.hpp"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*!
 * UpnpEvent
 */
typedef struct s_UpnpEvent UpnpEvent;

/*! Constructor */
EXPORT_SPEC UpnpEvent* UpnpEvent_new(void);
/*! Destructor */
EXPORT_SPEC void UpnpEvent_delete(UpnpEvent* p);
/*! Copy Constructor */
EXPORT_SPEC UpnpEvent* UpnpEvent_dup(const UpnpEvent* p);
/*! Assignment operator */
EXPORT_SPEC int UpnpEvent_assign(UpnpEvent* p, const UpnpEvent* q);

/*! UpnpEvent_get_EventKey */
EXPORT_SPEC int UpnpEvent_get_EventKey(const UpnpEvent* p);
/*! UpnpEvent_set_EventKey */
EXPORT_SPEC int UpnpEvent_set_EventKey(UpnpEvent* p, int n);

/*! UpnpEvent_get_ChangedVariables */
EXPORT_SPEC IXML_Document* UpnpEvent_get_ChangedVariables(const UpnpEvent* p);
/*! UpnpEvent_set_ChangedVariables */
EXPORT_SPEC int UpnpEvent_set_ChangedVariables(UpnpEvent* p, IXML_Document* n);

/*! UpnpEvent_get_SID */
EXPORT_SPEC const UpnpString* UpnpEvent_get_SID(const UpnpEvent* p);
/*! UpnpEvent_set_SID */
EXPORT_SPEC int UpnpEvent_set_SID(UpnpEvent* p, const UpnpString* s);
/*! UpnpEvent_get_SID_Length */
EXPORT_SPEC size_t UpnpEvent_get_SID_Length(const UpnpEvent* p);
/*! UpnpEvent_get_SID_cstr */
EXPORT_SPEC const char* UpnpEvent_get_SID_cstr(const UpnpEvent* p);
/*! UpnpEvent_strcpy_SID */
EXPORT_SPEC int UpnpEvent_strcpy_SID(UpnpEvent* p, const char* s);
/*! UpnpEvent_strncpy_SID */
EXPORT_SPEC int UpnpEvent_strncpy_SID(UpnpEvent* p, const char* s, size_t n);
/*! UpnpEvent_clear_SID */
EXPORT_SPEC void UpnpEvent_clear_SID(UpnpEvent* p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* UPNPLIB_UPNPEVENT_HPP */
