#ifndef UPNPLIB_UPNPACTIONREQUEST_HPP
#define UPNPLIB_UPNPACTIONREQUEST_HPP
// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-01
// Also Copyright by other contributor as noted below.
// Last compare with pupnp original source file on 2025-05-22, ver 1.14.20

/*!
 * \file
 *
 * \brief Header file for UpnpActionRequest methods.
 * \authors Marcelo Roberto Jimenez, Ingo Höft
 */
// #include <stdlib.h> /* for size_t */

// #include "UpnpGlobal.hpp" /* for EXPORT_SPEC */

#include "UpnpString.hpp"
#include "ixml.hpp"
#include "UpnpInet.hpp"
// #include "list.hpp"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*!
 * UpnpActionRequest
 */
typedef struct s_UpnpActionRequest UpnpActionRequest;

/*! Constructor */
EXPORT_SPEC UpnpActionRequest* UpnpActionRequest_new(void);
/*! Destructor */
EXPORT_SPEC void UpnpActionRequest_delete(UpnpActionRequest* p);
/*! Copy Constructor */
EXPORT_SPEC UpnpActionRequest*
UpnpActionRequest_dup(const UpnpActionRequest* p);
/*! Assignment operator */
EXPORT_SPEC int UpnpActionRequest_assign(UpnpActionRequest* p,
                                         const UpnpActionRequest* q);

/*! UpnpActionRequest_get_ErrCode */
EXPORT_SPEC int UpnpActionRequest_get_ErrCode(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_ErrCode */
EXPORT_SPEC int UpnpActionRequest_set_ErrCode(UpnpActionRequest* p, int n);

/*! UpnpActionRequest_get_Socket */
EXPORT_SPEC int UpnpActionRequest_get_Socket(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_Socket */
EXPORT_SPEC int UpnpActionRequest_set_Socket(UpnpActionRequest* p, int n);

/*! UpnpActionRequest_get_ErrStr */
EXPORT_SPEC const UpnpString*
UpnpActionRequest_get_ErrStr(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_ErrStr */
EXPORT_SPEC int UpnpActionRequest_set_ErrStr(UpnpActionRequest* p,
                                             const UpnpString* s);
/*! UpnpActionRequest_get_ErrStr_Length */
EXPORT_SPEC size_t
UpnpActionRequest_get_ErrStr_Length(const UpnpActionRequest* p);
/*! UpnpActionRequest_get_ErrStr_cstr */
EXPORT_SPEC const char*
UpnpActionRequest_get_ErrStr_cstr(const UpnpActionRequest* p);
/*! UpnpActionRequest_strcpy_ErrStr */
EXPORT_SPEC int UpnpActionRequest_strcpy_ErrStr(UpnpActionRequest* p,
                                                const char* s);
/*! UpnpActionRequest_strncpy_ErrStr */
EXPORT_SPEC int UpnpActionRequest_strncpy_ErrStr(UpnpActionRequest* p,
                                                 const char* s, size_t n);
/*! UpnpActionRequest_clear_ErrStr */
EXPORT_SPEC void UpnpActionRequest_clear_ErrStr(UpnpActionRequest* p);

/*! UpnpActionRequest_get_ActionName */
EXPORT_SPEC const UpnpString*
UpnpActionRequest_get_ActionName(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_ActionName */
EXPORT_SPEC int UpnpActionRequest_set_ActionName(UpnpActionRequest* p,
                                                 const UpnpString* s);
/*! UpnpActionRequest_get_ActionName_Length */
EXPORT_SPEC size_t
UpnpActionRequest_get_ActionName_Length(const UpnpActionRequest* p);
/*! UpnpActionRequest_get_ActionName_cstr */
EXPORT_SPEC const char*
UpnpActionRequest_get_ActionName_cstr(const UpnpActionRequest* p);
/*! UpnpActionRequest_strcpy_ActionName */
EXPORT_SPEC int UpnpActionRequest_strcpy_ActionName(UpnpActionRequest* p,
                                                    const char* s);
/*! UpnpActionRequest_strncpy_ActionName */
EXPORT_SPEC int UpnpActionRequest_strncpy_ActionName(UpnpActionRequest* p,
                                                     const char* s, size_t n);
/*! UpnpActionRequest_clear_ActionName */
EXPORT_SPEC void UpnpActionRequest_clear_ActionName(UpnpActionRequest* p);

/*! UpnpActionRequest_get_DevUDN */
EXPORT_SPEC const UpnpString*
UpnpActionRequest_get_DevUDN(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_DevUDN */
EXPORT_SPEC int UpnpActionRequest_set_DevUDN(UpnpActionRequest* p,
                                             const UpnpString* s);
/*! UpnpActionRequest_get_DevUDN_Length */
EXPORT_SPEC size_t
UpnpActionRequest_get_DevUDN_Length(const UpnpActionRequest* p);
/*! UpnpActionRequest_get_DevUDN_cstr */
EXPORT_SPEC const char*
UpnpActionRequest_get_DevUDN_cstr(const UpnpActionRequest* p);
/*! UpnpActionRequest_strcpy_DevUDN */
EXPORT_SPEC int UpnpActionRequest_strcpy_DevUDN(UpnpActionRequest* p,
                                                const char* s);
/*! UpnpActionRequest_strncpy_DevUDN */
EXPORT_SPEC int UpnpActionRequest_strncpy_DevUDN(UpnpActionRequest* p,
                                                 const char* s, size_t n);
/*! UpnpActionRequest_clear_DevUDN */
EXPORT_SPEC void UpnpActionRequest_clear_DevUDN(UpnpActionRequest* p);

/*! UpnpActionRequest_get_ServiceID */
EXPORT_SPEC const UpnpString*
UpnpActionRequest_get_ServiceID(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_ServiceID */
EXPORT_SPEC int UpnpActionRequest_set_ServiceID(UpnpActionRequest* p,
                                                const UpnpString* s);
/*! UpnpActionRequest_get_ServiceID_Length */
EXPORT_SPEC size_t
UpnpActionRequest_get_ServiceID_Length(const UpnpActionRequest* p);
/*! UpnpActionRequest_get_ServiceID_cstr */
EXPORT_SPEC const char*
UpnpActionRequest_get_ServiceID_cstr(const UpnpActionRequest* p);
/*! UpnpActionRequest_strcpy_ServiceID */
EXPORT_SPEC int UpnpActionRequest_strcpy_ServiceID(UpnpActionRequest* p,
                                                   const char* s);
/*! UpnpActionRequest_strncpy_ServiceID */
EXPORT_SPEC int UpnpActionRequest_strncpy_ServiceID(UpnpActionRequest* p,
                                                    const char* s, size_t n);
/*! UpnpActionRequest_clear_ServiceID */
EXPORT_SPEC void UpnpActionRequest_clear_ServiceID(UpnpActionRequest* p);

/*! UpnpActionRequest_get_ActionRequest */
EXPORT_SPEC IXML_Document*
UpnpActionRequest_get_ActionRequest(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_ActionRequest */
EXPORT_SPEC int UpnpActionRequest_set_ActionRequest(UpnpActionRequest* p,
                                                    IXML_Document* n);

/*! UpnpActionRequest_get_ActionResult */
EXPORT_SPEC IXML_Document*
UpnpActionRequest_get_ActionResult(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_ActionResult */
EXPORT_SPEC int UpnpActionRequest_set_ActionResult(UpnpActionRequest* p,
                                                   IXML_Document* n);

/*! UpnpActionRequest_get_SoapHeader */
EXPORT_SPEC IXML_Document*
UpnpActionRequest_get_SoapHeader(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_SoapHeader */
EXPORT_SPEC int UpnpActionRequest_set_SoapHeader(UpnpActionRequest* p,
                                                 IXML_Document* n);

/*! UpnpActionRequest_get_CtrlPtIPAddr */
EXPORT_SPEC const struct sockaddr_storage*
UpnpActionRequest_get_CtrlPtIPAddr(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_CtrlPtIPAddr */
EXPORT_SPEC int
UpnpActionRequest_set_CtrlPtIPAddr(UpnpActionRequest* p,
                                   const struct sockaddr_storage* buf);
/*! UpnpActionRequest_clear_CtrlPtIPAddr */
EXPORT_SPEC void UpnpActionRequest_clear_CtrlPtIPAddr(UpnpActionRequest* p);

/*! UpnpActionRequest_get_Os */
EXPORT_SPEC const UpnpString*
UpnpActionRequest_get_Os(const UpnpActionRequest* p);
/*! UpnpActionRequest_set_Os */
EXPORT_SPEC int UpnpActionRequest_set_Os(UpnpActionRequest* p,
                                         const UpnpString* s);
/*! UpnpActionRequest_get_Os_Length */
EXPORT_SPEC size_t UpnpActionRequest_get_Os_Length(const UpnpActionRequest* p);
/*! UpnpActionRequest_get_Os_cstr */
EXPORT_SPEC const char*
UpnpActionRequest_get_Os_cstr(const UpnpActionRequest* p);
/*! UpnpActionRequest_strcpy_Os */
EXPORT_SPEC int UpnpActionRequest_strcpy_Os(UpnpActionRequest* p,
                                            const char* s);
/*! UpnpActionRequest_strncpy_Os */
EXPORT_SPEC int UpnpActionRequest_strncpy_Os(UpnpActionRequest* p,
                                             const char* s, size_t n);
/*! UpnpActionRequest_clear_Os */
EXPORT_SPEC void UpnpActionRequest_clear_Os(UpnpActionRequest* p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* UPNPLIB_UPNPACTIONREQUEST_HPP */
