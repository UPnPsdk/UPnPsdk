#ifndef COMPA_UPNPEVENTSUBSCRIBE_HPP
#define COMPA_UPNPEVENTSUBSCRIBE_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-09
// Last compare with ./Pupnp source file on 2025-05-22, ver 1.14.20
/*!
 * \file
 * \brief Header file for UpnpEventSubscribe methods.
 * \authors Marcelo Roberto Jimenez, Ingo Höft
 */

#include <UpnpString.hpp>

/*!
 * UpnpEventSubscribe
 */
typedef struct s_UpnpEventSubscribe UpnpEventSubscribe;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*! Constructor */
PUPNP_API UpnpEventSubscribe* UpnpEventSubscribe_new(void);
/*! Destructor */
PUPNP_API void UpnpEventSubscribe_delete(UpnpEventSubscribe* p);
/*! Copy Constructor */
PUPNP_API UpnpEventSubscribe*
UpnpEventSubscribe_dup(const UpnpEventSubscribe* p);
/*! Assignment operator */
PUPNP_API int UpnpEventSubscribe_assign(UpnpEventSubscribe* p,
                                        const UpnpEventSubscribe* q);

/*! UpnpEventSubscribe_get_ErrCode */
PUPNP_API int UpnpEventSubscribe_get_ErrCode(const UpnpEventSubscribe* p);
/*! UpnpEventSubscribe_set_ErrCode */
PUPNP_API int UpnpEventSubscribe_set_ErrCode(UpnpEventSubscribe* p, int n);

/*! UpnpEventSubscribe_get_TimeOut */
PUPNP_API int UpnpEventSubscribe_get_TimeOut(const UpnpEventSubscribe* p);
/*! UpnpEventSubscribe_set_TimeOut */
PUPNP_API int UpnpEventSubscribe_set_TimeOut(UpnpEventSubscribe* p, int n);

/*! UpnpEventSubscribe_get_SID */
PUPNP_API const UpnpString*
UpnpEventSubscribe_get_SID(const UpnpEventSubscribe* p);
/*! UpnpEventSubscribe_set_SID */
PUPNP_API int UpnpEventSubscribe_set_SID(UpnpEventSubscribe* p,
                                         const UpnpString* s);
/*! UpnpEventSubscribe_get_SID_Length */
PUPNP_API size_t UpnpEventSubscribe_get_SID_Length(const UpnpEventSubscribe* p);
/*! UpnpEventSubscribe_get_SID_cstr */
PUPNP_API const char*
UpnpEventSubscribe_get_SID_cstr(const UpnpEventSubscribe* p);
/*! UpnpEventSubscribe_strcpy_SID */
PUPNP_API int UpnpEventSubscribe_strcpy_SID(UpnpEventSubscribe* p,
                                            const char* s);
/*! UpnpEventSubscribe_strncpy_SID */
PUPNP_API int UpnpEventSubscribe_strncpy_SID(UpnpEventSubscribe* p,
                                             const char* s, size_t n);
/*! UpnpEventSubscribe_clear_SID */
PUPNP_API void UpnpEventSubscribe_clear_SID(UpnpEventSubscribe* p);

/*! UpnpEventSubscribe_get_PublisherUrl */
PUPNP_API const UpnpString*
UpnpEventSubscribe_get_PublisherUrl(const UpnpEventSubscribe* p);
/*! UpnpEventSubscribe_set_PublisherUrl */
PUPNP_API int UpnpEventSubscribe_set_PublisherUrl(UpnpEventSubscribe* p,
                                                  const UpnpString* s);
/*! UpnpEventSubscribe_get_PublisherUrl_Length */
PUPNP_API size_t
UpnpEventSubscribe_get_PublisherUrl_Length(const UpnpEventSubscribe* p);
/*! UpnpEventSubscribe_get_PublisherUrl_cstr */
PUPNP_API const char*
UpnpEventSubscribe_get_PublisherUrl_cstr(const UpnpEventSubscribe* p);
/*! UpnpEventSubscribe_strcpy_PublisherUrl */
PUPNP_API int UpnpEventSubscribe_strcpy_PublisherUrl(UpnpEventSubscribe* p,
                                                     const char* s);
/*! UpnpEventSubscribe_strncpy_PublisherUrl */
PUPNP_API int UpnpEventSubscribe_strncpy_PublisherUrl(UpnpEventSubscribe* p,
                                                      const char* s, size_t n);
/*! UpnpEventSubscribe_clear_PublisherUrl */
PUPNP_API void UpnpEventSubscribe_clear_PublisherUrl(UpnpEventSubscribe* p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COMPA_UPNPEVENTSUBSCRIBE_HPP*/
