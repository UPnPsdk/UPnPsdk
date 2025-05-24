#ifndef COMPA_UPNPACTIONCOMPLETE_HPP
#define COMPA_UPNPACTIONCOMPLETE_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-05-21
// Last compare with ./Pupnp source file on 2025-05-22, ver 1.14.20
/*!
 * \file
 *
 * \brief Header file for UpnpActionComplete methods.
 * \authors Marcelo Roberto Jimenez, Ingo Höft
 */

#include <UpnpString.hpp>
#include <ixml.hpp>

/*! UpnpActionComplete */
typedef struct s_UpnpActionComplete UpnpActionComplete;

extern "C" {

/*! Constructor */
PUPNP_API UpnpActionComplete* UpnpActionComplete_new();
/*! Destructor */
PUPNP_API void UpnpActionComplete_delete(UpnpActionComplete* p);
/*! Copy Constructor */
PUPNP_API UpnpActionComplete*
UpnpActionComplete_dup(const UpnpActionComplete* p);
/*! Assignment operator */
PUPNP_API int UpnpActionComplete_assign(UpnpActionComplete* p,
                                        const UpnpActionComplete* q);

/*! UpnpActionComplete_get_ErrCode */
PUPNP_API int UpnpActionComplete_get_ErrCode(const UpnpActionComplete* p);
/*! UpnpActionComplete_set_ErrCode */
PUPNP_API int UpnpActionComplete_set_ErrCode(UpnpActionComplete* p, int n);

/*! UpnpActionComplete_get_CtrlUrl */
PUPNP_API const UpnpString*
UpnpActionComplete_get_CtrlUrl(const UpnpActionComplete* p);
/*! UpnpActionComplete_set_CtrlUrl */
PUPNP_API int UpnpActionComplete_set_CtrlUrl(UpnpActionComplete* p,
                                             const UpnpString* s);
/*! UpnpActionComplete_get_CtrlUrl_Length */
PUPNP_API size_t
UpnpActionComplete_get_CtrlUrl_Length(const UpnpActionComplete* p);
/*! UpnpActionComplete_get_CtrlUrl_cstr */
PUPNP_API const char*
UpnpActionComplete_get_CtrlUrl_cstr(const UpnpActionComplete* p);
/*! UpnpActionComplete_strcpy_CtrlUrl */
PUPNP_API int UpnpActionComplete_strcpy_CtrlUrl(UpnpActionComplete* p,
                                                const char* s);
/*! UpnpActionComplete_strncpy_CtrlUrl */
PUPNP_API int UpnpActionComplete_strncpy_CtrlUrl(UpnpActionComplete* p,
                                                 const char* s, size_t n);
/*! UpnpActionComplete_clear_CtrlUrl */
PUPNP_API void UpnpActionComplete_clear_CtrlUrl(UpnpActionComplete* p);

/*! UpnpActionComplete_get_ActionRequest */
PUPNP_API IXML_Document*
UpnpActionComplete_get_ActionRequest(const UpnpActionComplete* p);
/*! UpnpActionComplete_set_ActionRequest */
PUPNP_API int UpnpActionComplete_set_ActionRequest(UpnpActionComplete* p,
                                                   IXML_Document* n);

/*! UpnpActionComplete_get_ActionResult */
PUPNP_API IXML_Document*
UpnpActionComplete_get_ActionResult(const UpnpActionComplete* p);
/*! UpnpActionComplete_set_ActionResult */
PUPNP_API int UpnpActionComplete_set_ActionResult(UpnpActionComplete* p,
                                                  IXML_Document* n);
} // extern "C"

#endif /* COMPA_UPNPACTIONCOMPLETE_HPP */
