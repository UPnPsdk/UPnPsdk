#ifdef COMPA_HAVE_DEVICE_SOAP

#ifndef COMPA_UPNPSTATEVARREQUEST_HPP
#define COMPA_UPNPSTATEVARREQUEST_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-12
// Last compare with ./Pupnp source file on 2025-05-23, ver 1.14.20
/*!
 * \file
 * \brief Header file for UpnpStateVarRequest methods.
 * \authors Marcelo Roberto Jimenez, Ingo Höft
 */

#include <UpnpString.hpp>
#include <ixml/ixml.hpp>

/*!
 * UpnpStateVarRequest
 */
typedef struct s_UpnpStateVarRequest UpnpStateVarRequest;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*! Constructor */
PUPNP_Api UpnpStateVarRequest* UpnpStateVarRequest_new(void);
/*! Destructor */
PUPNP_Api void UpnpStateVarRequest_delete(UpnpStateVarRequest* p);
/*! Copy Constructor */
PUPNP_Api UpnpStateVarRequest*
UpnpStateVarRequest_dup(const UpnpStateVarRequest* p);
/*! Assignment operator */
PUPNP_Api int UpnpStateVarRequest_assign(UpnpStateVarRequest* p,
                                         const UpnpStateVarRequest* q);

/*! UpnpStateVarRequest_get_ErrCode */
PUPNP_Api int UpnpStateVarRequest_get_ErrCode(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_set_ErrCode */
PUPNP_Api int UpnpStateVarRequest_set_ErrCode(UpnpStateVarRequest* p, int n);

/*! UpnpStateVarRequest_get_Socket */
PUPNP_Api int UpnpStateVarRequest_get_Socket(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_set_Socket */
PUPNP_Api int UpnpStateVarRequest_set_Socket(UpnpStateVarRequest* p, int n);

/*! UpnpStateVarRequest_get_ErrStr */
PUPNP_Api const UpnpString*
UpnpStateVarRequest_get_ErrStr(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_set_ErrStr */
PUPNP_Api int UpnpStateVarRequest_set_ErrStr(UpnpStateVarRequest* p,
                                             const UpnpString* s);
/*! UpnpStateVarRequest_get_ErrStr_Length */
PUPNP_Api size_t
UpnpStateVarRequest_get_ErrStr_Length(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_get_ErrStr_cstr */
PUPNP_Api const char*
UpnpStateVarRequest_get_ErrStr_cstr(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_strcpy_ErrStr */
PUPNP_Api int UpnpStateVarRequest_strcpy_ErrStr(UpnpStateVarRequest* p,
                                                const char* s);
/*! UpnpStateVarRequest_strncpy_ErrStr */
PUPNP_Api int UpnpStateVarRequest_strncpy_ErrStr(UpnpStateVarRequest* p,
                                                 const char* s, size_t n);
/*! UpnpStateVarRequest_clear_ErrStr */
PUPNP_Api void UpnpStateVarRequest_clear_ErrStr(UpnpStateVarRequest* p);

/*! UpnpStateVarRequest_get_DevUDN */
PUPNP_Api const UpnpString*
UpnpStateVarRequest_get_DevUDN(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_set_DevUDN */
PUPNP_Api int UpnpStateVarRequest_set_DevUDN(UpnpStateVarRequest* p,
                                             const UpnpString* s);
/*! UpnpStateVarRequest_get_DevUDN_Length */
PUPNP_Api size_t
UpnpStateVarRequest_get_DevUDN_Length(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_get_DevUDN_cstr */
PUPNP_Api const char*
UpnpStateVarRequest_get_DevUDN_cstr(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_strcpy_DevUDN */
PUPNP_Api int UpnpStateVarRequest_strcpy_DevUDN(UpnpStateVarRequest* p,
                                                const char* s);
/*! UpnpStateVarRequest_strncpy_DevUDN */
PUPNP_Api int UpnpStateVarRequest_strncpy_DevUDN(UpnpStateVarRequest* p,
                                                 const char* s, size_t n);
/*! UpnpStateVarRequest_clear_DevUDN */
PUPNP_Api void UpnpStateVarRequest_clear_DevUDN(UpnpStateVarRequest* p);

/*! UpnpStateVarRequest_get_ServiceID */
PUPNP_Api const UpnpString*
UpnpStateVarRequest_get_ServiceID(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_set_ServiceID */
PUPNP_Api int UpnpStateVarRequest_set_ServiceID(UpnpStateVarRequest* p,
                                                const UpnpString* s);
/*! UpnpStateVarRequest_get_ServiceID_Length */
PUPNP_Api size_t
UpnpStateVarRequest_get_ServiceID_Length(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_get_ServiceID_cstr */
PUPNP_Api const char*
UpnpStateVarRequest_get_ServiceID_cstr(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_strcpy_ServiceID */
PUPNP_Api int UpnpStateVarRequest_strcpy_ServiceID(UpnpStateVarRequest* p,
                                                   const char* s);
/*! UpnpStateVarRequest_strncpy_ServiceID */
PUPNP_Api int UpnpStateVarRequest_strncpy_ServiceID(UpnpStateVarRequest* p,
                                                    const char* s, size_t n);
/*! UpnpStateVarRequest_clear_ServiceID */
PUPNP_Api void UpnpStateVarRequest_clear_ServiceID(UpnpStateVarRequest* p);

/*! UpnpStateVarRequest_get_StateVarName */
PUPNP_Api const UpnpString*
UpnpStateVarRequest_get_StateVarName(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_set_StateVarName */
PUPNP_Api int UpnpStateVarRequest_set_StateVarName(UpnpStateVarRequest* p,
                                                   const UpnpString* s);
/*! UpnpStateVarRequest_get_StateVarName_Length */
PUPNP_Api size_t
UpnpStateVarRequest_get_StateVarName_Length(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_get_StateVarName_cstr */
PUPNP_Api const char*
UpnpStateVarRequest_get_StateVarName_cstr(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_strcpy_StateVarName */
PUPNP_Api int UpnpStateVarRequest_strcpy_StateVarName(UpnpStateVarRequest* p,
                                                      const char* s);
/*! UpnpStateVarRequest_strncpy_StateVarName */
PUPNP_Api int UpnpStateVarRequest_strncpy_StateVarName(UpnpStateVarRequest* p,
                                                       const char* s, size_t n);
/*! UpnpStateVarRequest_clear_StateVarName */
PUPNP_Api void UpnpStateVarRequest_clear_StateVarName(UpnpStateVarRequest* p);

/*! UpnpStateVarRequest_get_CtrlPtIPAddr */
PUPNP_Api const struct sockaddr_storage*
UpnpStateVarRequest_get_CtrlPtIPAddr(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_set_CtrlPtIPAddr */
PUPNP_Api int
UpnpStateVarRequest_set_CtrlPtIPAddr(UpnpStateVarRequest* p,
                                     const struct sockaddr_storage* buf);
/*! UpnpStateVarRequest_clear_CtrlPtIPAddr */
PUPNP_Api void UpnpStateVarRequest_clear_CtrlPtIPAddr(UpnpStateVarRequest* p);

/*! UpnpStateVarRequest_get_CurrentVal */
PUPNP_Api const DOMString
UpnpStateVarRequest_get_CurrentVal(const UpnpStateVarRequest* p);
/*! UpnpStateVarRequest_set_CurrentVal */
PUPNP_Api int UpnpStateVarRequest_set_CurrentVal(UpnpStateVarRequest* p,
                                                 const DOMString s);
/*! UpnpStateVarRequest_get_CurrentVal_cstr */
PUPNP_Api const char*
UpnpStateVarRequest_get_CurrentVal_cstr(const UpnpStateVarRequest* p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // COMPA_UPNPSTATEVARREQUEST_HPP
#endif // COMPA_HAVE_DEVICE_SOAP
