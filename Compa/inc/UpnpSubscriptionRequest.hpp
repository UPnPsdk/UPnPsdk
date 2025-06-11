#ifndef COMPA_UPNPSUBSCRIPTIONREQUEST_HPP
#define COMPA_UPNPSUBSCRIPTIONREQUEST_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-12
// Last compare with ./Pupnp source file on 2025-05-23, ver 1.14.20
/*!
 * \file
 * \brief Header file for UpnpSubscriptionRequest methods.
 * \authors Marcelo Roberto Jimenez, Ingo Höft
 */

#include "UpnpString.hpp"

/*!
 * UpnpSubscriptionRequest
 */
typedef struct s_UpnpSubscriptionRequest UpnpSubscriptionRequest;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*! Constructor */
PUPNP_Api UpnpSubscriptionRequest* UpnpSubscriptionRequest_new(void);
/*! Destructor */
PUPNP_Api void UpnpSubscriptionRequest_delete(UpnpSubscriptionRequest* p);
/*! Copy Constructor */
PUPNP_Api UpnpSubscriptionRequest*
UpnpSubscriptionRequest_dup(const UpnpSubscriptionRequest* p);
/*! Assignment operator */
PUPNP_Api int UpnpSubscriptionRequest_assign(UpnpSubscriptionRequest* p,
                                             const UpnpSubscriptionRequest* q);

/*! UpnpSubscriptionRequest_get_ServiceId */
PUPNP_Api const UpnpString*
UpnpSubscriptionRequest_get_ServiceId(const UpnpSubscriptionRequest* p);
/*! UpnpSubscriptionRequest_set_ServiceId */
PUPNP_Api int UpnpSubscriptionRequest_set_ServiceId(UpnpSubscriptionRequest* p,
                                                    const UpnpString* s);
/*! UpnpSubscriptionRequest_get_ServiceId_Length */
PUPNP_Api size_t
UpnpSubscriptionRequest_get_ServiceId_Length(const UpnpSubscriptionRequest* p);
/*! UpnpSubscriptionRequest_get_ServiceId_cstr */
PUPNP_Api const char*
UpnpSubscriptionRequest_get_ServiceId_cstr(const UpnpSubscriptionRequest* p);
/*! UpnpSubscriptionRequest_strcpy_ServiceId */
PUPNP_Api int
UpnpSubscriptionRequest_strcpy_ServiceId(UpnpSubscriptionRequest* p,
                                         const char* s);
/*! UpnpSubscriptionRequest_strncpy_ServiceId */
PUPNP_Api int
UpnpSubscriptionRequest_strncpy_ServiceId(UpnpSubscriptionRequest* p,
                                          const char* s, size_t n);
/*! UpnpSubscriptionRequest_clear_ServiceId */
PUPNP_Api void
UpnpSubscriptionRequest_clear_ServiceId(UpnpSubscriptionRequest* p);

/*! UpnpSubscriptionRequest_get_UDN */
PUPNP_Api const UpnpString*
UpnpSubscriptionRequest_get_UDN(const UpnpSubscriptionRequest* p);
/*! UpnpSubscriptionRequest_set_UDN */
PUPNP_Api int UpnpSubscriptionRequest_set_UDN(UpnpSubscriptionRequest* p,
                                              const UpnpString* s);
/*! UpnpSubscriptionRequest_get_UDN_Length */
PUPNP_Api size_t
UpnpSubscriptionRequest_get_UDN_Length(const UpnpSubscriptionRequest* p);
/*! UpnpSubscriptionRequest_get_UDN_cstr */
PUPNP_Api const char*
UpnpSubscriptionRequest_get_UDN_cstr(const UpnpSubscriptionRequest* p);
/*! UpnpSubscriptionRequest_strcpy_UDN */
PUPNP_Api int UpnpSubscriptionRequest_strcpy_UDN(UpnpSubscriptionRequest* p,
                                                 const char* s);
/*! UpnpSubscriptionRequest_strncpy_UDN */
PUPNP_Api int UpnpSubscriptionRequest_strncpy_UDN(UpnpSubscriptionRequest* p,
                                                  const char* s, size_t n);
/*! UpnpSubscriptionRequest_clear_UDN */
PUPNP_Api void UpnpSubscriptionRequest_clear_UDN(UpnpSubscriptionRequest* p);

/*! UpnpSubscriptionRequest_get_SID */
PUPNP_Api const UpnpString*
UpnpSubscriptionRequest_get_SID(const UpnpSubscriptionRequest* p);
/*! UpnpSubscriptionRequest_set_SID */
PUPNP_Api int UpnpSubscriptionRequest_set_SID(UpnpSubscriptionRequest* p,
                                              const UpnpString* s);
/*! UpnpSubscriptionRequest_get_SID_Length */
PUPNP_Api size_t
UpnpSubscriptionRequest_get_SID_Length(const UpnpSubscriptionRequest* p);
/*! UpnpSubscriptionRequest_get_SID_cstr */
PUPNP_Api const char*
UpnpSubscriptionRequest_get_SID_cstr(const UpnpSubscriptionRequest* p);
/*! UpnpSubscriptionRequest_strcpy_SID */
PUPNP_Api int UpnpSubscriptionRequest_strcpy_SID(UpnpSubscriptionRequest* p,
                                                 const char* s);
/*! UpnpSubscriptionRequest_strncpy_SID */
PUPNP_Api int UpnpSubscriptionRequest_strncpy_SID(UpnpSubscriptionRequest* p,
                                                  const char* s, size_t n);
/*! UpnpSubscriptionRequest_clear_SID */
PUPNP_Api void UpnpSubscriptionRequest_clear_SID(UpnpSubscriptionRequest* p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COMPA_UPNPSUBSCRIPTIONREQUEST_HPP */
