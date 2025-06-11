#ifndef COMPA_UPNPACTIONREQUEST_HPP
#define COMPA_UPNPACTIONREQUEST_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-12
// Last compare with ./Pupnp source file on 2025-05-22, ver 1.14.20
/*!
 * \file
 * \brief Header file for UpnpActionRequest methods.
 * \authors Marcelo Roberto Jimenez, Ingo Höft
 */

#include <UpnpString.hpp>
#include <ixml/ixml.hpp>

/*!
 * UpnpActionRequest
 */
typedef struct s_UpnpActionRequest UpnpActionRequest;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*! Constructor */
PUPNP_Api UpnpActionRequest* UpnpActionRequest_new(void);
/*! Destructor */
PUPNP_Api void UpnpActionRequest_delete(UpnpActionRequest* p);
/*! Copy Constructor */
PUPNP_Api UpnpActionRequest* UpnpActionRequest_dup(const UpnpActionRequest* p);
/*! Assignment operator */
PUPNP_Api int UpnpActionRequest_assign(UpnpActionRequest* p,
                                       const UpnpActionRequest* q);

/*! UpnpActionRequest_get_ErrCode */
PUPNP_Api int UpnpActionRequest_get_ErrCode(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_ErrCode */
PUPNP_Api int UpnpActionRequest_set_ErrCode(UpnpActionRequest* p, int n);

/*! UpnpActionRequest_get_Socket */
PUPNP_Api int UpnpActionRequest_get_Socket(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_Socket */
PUPNP_Api int UpnpActionRequest_set_Socket(UpnpActionRequest* p, int n);

/*! UpnpActionRequest_get_ErrStr */
PUPNP_Api const UpnpString*
UpnpActionRequest_get_ErrStr(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_ErrStr */
PUPNP_Api int UpnpActionRequest_set_ErrStr(UpnpActionRequest* p,
                                           const UpnpString* s);
/*! UpnpActionRequest_get_ErrStr_Length */
PUPNP_Api size_t
UpnpActionRequest_get_ErrStr_Length(const UpnpActionRequest* p);
/*! UpnpActionRequest_get_ErrStr_cstr */
PUPNP_Api const char*
UpnpActionRequest_get_ErrStr_cstr(const UpnpActionRequest* p);
/*! UpnpActionRequest_strcpy_ErrStr */
PUPNP_Api int UpnpActionRequest_strcpy_ErrStr(UpnpActionRequest* p,
                                              const char* s);
/*! UpnpActionRequest_strncpy_ErrStr */
PUPNP_Api int UpnpActionRequest_strncpy_ErrStr(UpnpActionRequest* p,
                                               const char* s, size_t n);
/*! UpnpActionRequest_clear_ErrStr */
PUPNP_Api void UpnpActionRequest_clear_ErrStr(UpnpActionRequest* p);

/*! UpnpActionRequest_get_ActionName */
PUPNP_Api const UpnpString*
UpnpActionRequest_get_ActionName(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_ActionName */
PUPNP_Api int UpnpActionRequest_set_ActionName(UpnpActionRequest* p,
                                               const UpnpString* s);
/*! UpnpActionRequest_get_ActionName_Length */
PUPNP_Api size_t
UpnpActionRequest_get_ActionName_Length(const UpnpActionRequest* p);
/*! UpnpActionRequest_get_ActionName_cstr */
PUPNP_Api const char*
UpnpActionRequest_get_ActionName_cstr(const UpnpActionRequest* p);
/*! UpnpActionRequest_strcpy_ActionName */
PUPNP_Api int UpnpActionRequest_strcpy_ActionName(UpnpActionRequest* p,
                                                  const char* s);
/*! UpnpActionRequest_strncpy_ActionName */
PUPNP_Api int UpnpActionRequest_strncpy_ActionName(UpnpActionRequest* p,
                                                   const char* s, size_t n);
/*! UpnpActionRequest_clear_ActionName */
PUPNP_Api void UpnpActionRequest_clear_ActionName(UpnpActionRequest* p);

/*! UpnpActionRequest_get_DevUDN */
PUPNP_Api const UpnpString*
UpnpActionRequest_get_DevUDN(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_DevUDN */
PUPNP_Api int UpnpActionRequest_set_DevUDN(UpnpActionRequest* p,
                                           const UpnpString* s);
/*! UpnpActionRequest_get_DevUDN_Length */
PUPNP_Api size_t
UpnpActionRequest_get_DevUDN_Length(const UpnpActionRequest* p);
/*! UpnpActionRequest_get_DevUDN_cstr */
PUPNP_Api const char*
UpnpActionRequest_get_DevUDN_cstr(const UpnpActionRequest* p);
/*! UpnpActionRequest_strcpy_DevUDN */
PUPNP_Api int UpnpActionRequest_strcpy_DevUDN(UpnpActionRequest* p,
                                              const char* s);
/*! UpnpActionRequest_strncpy_DevUDN */
PUPNP_Api int UpnpActionRequest_strncpy_DevUDN(UpnpActionRequest* p,
                                               const char* s, size_t n);
/*! UpnpActionRequest_clear_DevUDN */
PUPNP_Api void UpnpActionRequest_clear_DevUDN(UpnpActionRequest* p);

/*! UpnpActionRequest_get_ServiceID */
PUPNP_Api const UpnpString*
UpnpActionRequest_get_ServiceID(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_ServiceID */
PUPNP_Api int UpnpActionRequest_set_ServiceID(UpnpActionRequest* p,
                                              const UpnpString* s);
/*! UpnpActionRequest_get_ServiceID_Length */
PUPNP_Api size_t
UpnpActionRequest_get_ServiceID_Length(const UpnpActionRequest* p);
/*! UpnpActionRequest_get_ServiceID_cstr */
PUPNP_Api const char*
UpnpActionRequest_get_ServiceID_cstr(const UpnpActionRequest* p);
/*! UpnpActionRequest_strcpy_ServiceID */
PUPNP_Api int UpnpActionRequest_strcpy_ServiceID(UpnpActionRequest* p,
                                                 const char* s);
/*! UpnpActionRequest_strncpy_ServiceID */
PUPNP_Api int UpnpActionRequest_strncpy_ServiceID(UpnpActionRequest* p,
                                                  const char* s, size_t n);
/*! UpnpActionRequest_clear_ServiceID */
PUPNP_Api void UpnpActionRequest_clear_ServiceID(UpnpActionRequest* p);

/*! UpnpActionRequest_get_ActionRequest */
PUPNP_Api IXML_Document*
UpnpActionRequest_get_ActionRequest(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_ActionRequest */
PUPNP_Api int UpnpActionRequest_set_ActionRequest(UpnpActionRequest* p,
                                                  IXML_Document* n);

/*! UpnpActionRequest_get_ActionResult */
PUPNP_Api IXML_Document*
UpnpActionRequest_get_ActionResult(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_ActionResult */
PUPNP_Api int UpnpActionRequest_set_ActionResult(UpnpActionRequest* p,
                                                 IXML_Document* n);

/*! UpnpActionRequest_get_SoapHeader */
PUPNP_Api IXML_Document*
UpnpActionRequest_get_SoapHeader(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_SoapHeader */
PUPNP_Api int UpnpActionRequest_set_SoapHeader(UpnpActionRequest* p,
                                               IXML_Document* n);

/*! UpnpActionRequest_get_CtrlPtIPAddr */
PUPNP_Api const struct sockaddr_storage*
UpnpActionRequest_get_CtrlPtIPAddr(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_CtrlPtIPAddr */
PUPNP_Api int
UpnpActionRequest_set_CtrlPtIPAddr(UpnpActionRequest* p,
                                   const struct sockaddr_storage* buf);
/*! UpnpActionRequest_clear_CtrlPtIPAddr */
PUPNP_Api void UpnpActionRequest_clear_CtrlPtIPAddr(UpnpActionRequest* p);

/*! UpnpActionRequest_get_Os */
PUPNP_Api const UpnpString*
UpnpActionRequest_get_Os(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_Os */
PUPNP_Api int UpnpActionRequest_set_Os(UpnpActionRequest* p,
                                       const UpnpString* s);
/*! UpnpActionRequest_get_Os_Length */
PUPNP_Api size_t UpnpActionRequest_get_Os_Length(const UpnpActionRequest* p);
/*! UpnpActionRequest_get_Os_cstr */
PUPNP_Api const char* UpnpActionRequest_get_Os_cstr(const UpnpActionRequest* p);
/*! UpnpActionRequest_strcpy_Os */
PUPNP_Api int UpnpActionRequest_strcpy_Os(UpnpActionRequest* p, const char* s);
/*! UpnpActionRequest_strncpy_Os */
PUPNP_Api int UpnpActionRequest_strncpy_Os(UpnpActionRequest* p, const char* s,
                                           size_t n);
/*! UpnpActionRequest_clear_Os */
PUPNP_Api void UpnpActionRequest_clear_Os(UpnpActionRequest* p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COMPA_UPNPACTIONREQUEST_HPP */
