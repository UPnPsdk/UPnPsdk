#ifndef UPNPSTATEVARREQUEST_HPP
#define UPNPSTATEVARREQUEST_HPP
// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-01
// Also Copyright by other contributor as noted below.
// Last compare with pupnp original source file on 2025-05-23, ver 1.14.20

/*!
 * \file
 *
 * \brief Header file for UpnpStateVarRequest methods.
 * \authors Marcelo Roberto Jimenez, Ingo Höft
 */

#include <stdlib.h>       /* for size_t */

#include "UpnpGlobal.hpp" /* for EXPORT_SPEC */

#include "UpnpString.hpp"
#include "UpnpInet.hpp"
#include "ixml.hpp"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*!
 * UpnpStateVarRequest
 */
typedef struct s_UpnpStateVarRequest UpnpStateVarRequest;

/*! Constructor */
EXPORT_SPEC UpnpStateVarRequest* UpnpStateVarRequest_new(void);
/*! Destructor */
EXPORT_SPEC void UpnpStateVarRequest_delete(UpnpStateVarRequest* p);
/*! Copy Constructor */
EXPORT_SPEC UpnpStateVarRequest*
UpnpStateVarRequest_dup(const UpnpStateVarRequest* p);
/*! Assignment operator */
EXPORT_SPEC int UpnpStateVarRequest_assign(UpnpStateVarRequest* p,
                                           const UpnpStateVarRequest* q);

/*! UpnpStateVarRequest_get_ErrCode */
EXPORT_SPEC int UpnpStateVarRequest_get_ErrCode(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_set_ErrCode */
EXPORT_SPEC int UpnpStateVarRequest_set_ErrCode(UpnpStateVarRequest* p, int n);

/*! UpnpStateVarRequest_get_Socket */
EXPORT_SPEC int UpnpStateVarRequest_get_Socket(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_set_Socket */
EXPORT_SPEC int UpnpStateVarRequest_set_Socket(UpnpStateVarRequest* p, int n);

/*! UpnpStateVarRequest_get_ErrStr */
EXPORT_SPEC const UpnpString*
UpnpStateVarRequest_get_ErrStr(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_set_ErrStr */
EXPORT_SPEC int UpnpStateVarRequest_set_ErrStr(UpnpStateVarRequest* p,
                                               const UpnpString* s);
/*! UpnpStateVarRequest_get_ErrStr_Length */
EXPORT_SPEC size_t
UpnpStateVarRequest_get_ErrStr_Length(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_get_ErrStr_cstr */
EXPORT_SPEC const char*
UpnpStateVarRequest_get_ErrStr_cstr(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_strcpy_ErrStr */
EXPORT_SPEC int UpnpStateVarRequest_strcpy_ErrStr(UpnpStateVarRequest* p,
                                                  const char* s);
/*! UpnpStateVarRequest_strncpy_ErrStr */
EXPORT_SPEC int UpnpStateVarRequest_strncpy_ErrStr(UpnpStateVarRequest* p,
                                                   const char* s, size_t n);
/*! UpnpStateVarRequest_clear_ErrStr */
EXPORT_SPEC void UpnpStateVarRequest_clear_ErrStr(UpnpStateVarRequest* p);

/*! UpnpStateVarRequest_get_DevUDN */
EXPORT_SPEC const UpnpString*
UpnpStateVarRequest_get_DevUDN(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_set_DevUDN */
EXPORT_SPEC int UpnpStateVarRequest_set_DevUDN(UpnpStateVarRequest* p,
                                               const UpnpString* s);
/*! UpnpStateVarRequest_get_DevUDN_Length */
EXPORT_SPEC size_t
UpnpStateVarRequest_get_DevUDN_Length(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_get_DevUDN_cstr */
EXPORT_SPEC const char*
UpnpStateVarRequest_get_DevUDN_cstr(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_strcpy_DevUDN */
EXPORT_SPEC int UpnpStateVarRequest_strcpy_DevUDN(UpnpStateVarRequest* p,
                                                  const char* s);
/*! UpnpStateVarRequest_strncpy_DevUDN */
EXPORT_SPEC int UpnpStateVarRequest_strncpy_DevUDN(UpnpStateVarRequest* p,
                                                   const char* s, size_t n);
/*! UpnpStateVarRequest_clear_DevUDN */
EXPORT_SPEC void UpnpStateVarRequest_clear_DevUDN(UpnpStateVarRequest* p);

/*! UpnpStateVarRequest_get_ServiceID */
EXPORT_SPEC const UpnpString*
UpnpStateVarRequest_get_ServiceID(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_set_ServiceID */
EXPORT_SPEC int UpnpStateVarRequest_set_ServiceID(UpnpStateVarRequest* p,
                                                  const UpnpString* s);
/*! UpnpStateVarRequest_get_ServiceID_Length */
EXPORT_SPEC size_t
UpnpStateVarRequest_get_ServiceID_Length(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_get_ServiceID_cstr */
EXPORT_SPEC const char*
UpnpStateVarRequest_get_ServiceID_cstr(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_strcpy_ServiceID */
EXPORT_SPEC int UpnpStateVarRequest_strcpy_ServiceID(UpnpStateVarRequest* p,
                                                     const char* s);
/*! UpnpStateVarRequest_strncpy_ServiceID */
EXPORT_SPEC int UpnpStateVarRequest_strncpy_ServiceID(UpnpStateVarRequest* p,
                                                      const char* s, size_t n);
/*! UpnpStateVarRequest_clear_ServiceID */
EXPORT_SPEC void UpnpStateVarRequest_clear_ServiceID(UpnpStateVarRequest* p);

/*! UpnpStateVarRequest_get_StateVarName */
EXPORT_SPEC const UpnpString*
UpnpStateVarRequest_get_StateVarName(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_set_StateVarName */
EXPORT_SPEC int UpnpStateVarRequest_set_StateVarName(UpnpStateVarRequest* p,
                                                     const UpnpString* s);
/*! UpnpStateVarRequest_get_StateVarName_Length */
EXPORT_SPEC size_t
UpnpStateVarRequest_get_StateVarName_Length(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_get_StateVarName_cstr */
EXPORT_SPEC const char*
UpnpStateVarRequest_get_StateVarName_cstr(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_strcpy_StateVarName */
EXPORT_SPEC int UpnpStateVarRequest_strcpy_StateVarName(UpnpStateVarRequest* p,
                                                        const char* s);
/*! UpnpStateVarRequest_strncpy_StateVarName */
EXPORT_SPEC int UpnpStateVarRequest_strncpy_StateVarName(UpnpStateVarRequest* p,
                                                         const char* s,
                                                         size_t n);
/*! UpnpStateVarRequest_clear_StateVarName */
EXPORT_SPEC void UpnpStateVarRequest_clear_StateVarName(UpnpStateVarRequest* p);

/*! UpnpStateVarRequest_get_CtrlPtIPAddr */
EXPORT_SPEC const struct sockaddr_storage*
UpnpStateVarRequest_get_CtrlPtIPAddr(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_set_CtrlPtIPAddr */
EXPORT_SPEC int
UpnpStateVarRequest_set_CtrlPtIPAddr(UpnpStateVarRequest* p,
                                     const struct sockaddr_storage* buf);
/*! UpnpStateVarRequest_clear_CtrlPtIPAddr */
EXPORT_SPEC void UpnpStateVarRequest_clear_CtrlPtIPAddr(UpnpStateVarRequest* p);

/*! UpnpStateVarRequest_get_CurrentVal */
EXPORT_SPEC const DOMString
UpnpStateVarRequest_get_CurrentVal(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_set_CurrentVal */
EXPORT_SPEC int UpnpStateVarRequest_set_CurrentVal(UpnpStateVarRequest* p,
                                                   const DOMString s);
/*! UpnpStateVarRequest_get_CurrentVal_cstr */
EXPORT_SPEC const char*
UpnpStateVarRequest_get_CurrentVal_cstr(const UpnpStateVarRequest* p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* UPNPSTATEVARREQUEST_HPP */
