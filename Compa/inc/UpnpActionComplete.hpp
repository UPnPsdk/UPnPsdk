#ifndef COMPA_UPNPACTIONCOMPLETE_HPP
#define COMPA_UPNPACTIONCOMPLETE_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-12
// Last compare with ./Pupnp source file on 2025-05-22, ver 1.14.20
/*!
 * \file
 *
 * \brief Header file for UpnpActionComplete methods.
 * \authors Marcelo Roberto Jimenez, Ingo Höft
 */

#include <UpnpString.hpp>
#include <ixml/ixml.hpp>

/*! UpnpActionComplete */
typedef struct s_UpnpActionComplete UpnpActionComplete;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*! Constructor */
PUPNP_Api UpnpActionComplete* UpnpActionComplete_new(void);
/*! Destructor */
PUPNP_Api void UpnpActionComplete_delete(UpnpActionComplete* p);
/*! Copy Constructor */
PUPNP_Api UpnpActionComplete*
UpnpActionComplete_dup(const UpnpActionComplete* p);
/*! Assignment operator */
PUPNP_Api int UpnpActionComplete_assign(UpnpActionComplete* p,
                                        const UpnpActionComplete* q);

/*! UpnpActionComplete_get_ErrCode */
PUPNP_Api int UpnpActionComplete_get_ErrCode(const UpnpActionComplete* p);
/*! UpnpActionComplete_set_ErrCode */
PUPNP_Api int UpnpActionComplete_set_ErrCode(UpnpActionComplete* p, int n);

/*! UpnpActionComplete_get_CtrlUrl */
PUPNP_Api const UpnpString*
UpnpActionComplete_get_CtrlUrl(const UpnpActionComplete* p);
/*! UpnpActionComplete_set_CtrlUrl */
PUPNP_Api int UpnpActionComplete_set_CtrlUrl(UpnpActionComplete* p,
                                             const UpnpString* s);
/*! UpnpActionComplete_get_CtrlUrl_Length */
PUPNP_Api size_t
UpnpActionComplete_get_CtrlUrl_Length(const UpnpActionComplete* p);
/*! UpnpActionComplete_get_CtrlUrl_cstr */
PUPNP_Api const char*
UpnpActionComplete_get_CtrlUrl_cstr(const UpnpActionComplete* p);
/*! UpnpActionComplete_strcpy_CtrlUrl */
PUPNP_Api int UpnpActionComplete_strcpy_CtrlUrl(UpnpActionComplete* p,
                                                const char* s);
/*! UpnpActionComplete_strncpy_CtrlUrl */
PUPNP_Api int UpnpActionComplete_strncpy_CtrlUrl(UpnpActionComplete* p,
                                                 const char* s, size_t n);
/*! UpnpActionComplete_clear_CtrlUrl */
PUPNP_Api void UpnpActionComplete_clear_CtrlUrl(UpnpActionComplete* p);

/*! UpnpActionComplete_get_ActionRequest */
PUPNP_Api IXML_Document*
UpnpActionComplete_get_ActionRequest(const UpnpActionComplete* p);
/*! UpnpActionComplete_set_ActionRequest */
PUPNP_Api int UpnpActionComplete_set_ActionRequest(UpnpActionComplete* p,
                                                   IXML_Document* n);

/*! UpnpActionComplete_get_ActionResult */
PUPNP_Api IXML_Document*
UpnpActionComplete_get_ActionResult(const UpnpActionComplete* p);
/*! UpnpActionComplete_set_ActionResult */
PUPNP_Api int UpnpActionComplete_set_ActionResult(UpnpActionComplete* p,
                                                  IXML_Document* n);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COMPA_UPNPACTIONCOMPLETE_HPP */
