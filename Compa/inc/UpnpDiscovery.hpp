#ifndef COMPA_UPNPDISCOVERY_HPP
#define COMPA_UPNPDISCOVERY_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-05-21
// Last compare with ./Pupnp source file on 2025-05-23, ver 1.14.20
/*!
 * \file
 * \brief Header file for UpnpDiscovery methods.
 * \authors Marcelo Roberto Jimenez, Ingo Höft
 */

#include <UpnpString.hpp>

/*!
 * UpnpDiscovery
 */
typedef struct s_UpnpDiscovery UpnpDiscovery;

extern "C" {

/*! Constructor */
PUPNP_API UpnpDiscovery* UpnpDiscovery_new();
/*! Destructor */
PUPNP_API void UpnpDiscovery_delete(UpnpDiscovery* p);
/*! Copy Constructor */
PUPNP_API UpnpDiscovery* UpnpDiscovery_dup(const UpnpDiscovery* p);
/*! Assignment operator */
PUPNP_API int UpnpDiscovery_assign(UpnpDiscovery* p, const UpnpDiscovery* q);

/*! UpnpDiscovery_get_ErrCode */
PUPNP_API int UpnpDiscovery_get_ErrCode(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_ErrCode */
PUPNP_API int UpnpDiscovery_set_ErrCode(UpnpDiscovery* p, int n);

/*! UpnpDiscovery_get_Expires */
PUPNP_API int UpnpDiscovery_get_Expires(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_Expires */
PUPNP_API int UpnpDiscovery_set_Expires(UpnpDiscovery* p, int n);

/*! UpnpDiscovery_get_DeviceID */
PUPNP_API const UpnpString* UpnpDiscovery_get_DeviceID(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_DeviceID */
PUPNP_API int UpnpDiscovery_set_DeviceID(UpnpDiscovery* p, const UpnpString* s);
/*! UpnpDiscovery_get_DeviceID_Length */
PUPNP_API size_t UpnpDiscovery_get_DeviceID_Length(const UpnpDiscovery* p);
/*! UpnpDiscovery_get_DeviceID_cstr */
PUPNP_API const char* UpnpDiscovery_get_DeviceID_cstr(const UpnpDiscovery* p);
/*! UpnpDiscovery_strcpy_DeviceID */
PUPNP_API int UpnpDiscovery_strcpy_DeviceID(UpnpDiscovery* p, const char* s);
/*! UpnpDiscovery_strncpy_DeviceID */
PUPNP_API int UpnpDiscovery_strncpy_DeviceID(UpnpDiscovery* p, const char* s,
                                             size_t n);
/*! UpnpDiscovery_clear_DeviceID */
PUPNP_API void UpnpDiscovery_clear_DeviceID(UpnpDiscovery* p);

/*! UpnpDiscovery_get_DeviceType */
PUPNP_API const UpnpString*
UpnpDiscovery_get_DeviceType(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_DeviceType */
PUPNP_API int UpnpDiscovery_set_DeviceType(UpnpDiscovery* p,
                                           const UpnpString* s);
/*! UpnpDiscovery_get_DeviceType_Length */
PUPNP_API size_t UpnpDiscovery_get_DeviceType_Length(const UpnpDiscovery* p);
/*! UpnpDiscovery_get_DeviceType_cstr */
PUPNP_API const char* UpnpDiscovery_get_DeviceType_cstr(const UpnpDiscovery* p);
/*! UpnpDiscovery_strcpy_DeviceType */
PUPNP_API int UpnpDiscovery_strcpy_DeviceType(UpnpDiscovery* p, const char* s);
/*! UpnpDiscovery_strncpy_DeviceType */
PUPNP_API int UpnpDiscovery_strncpy_DeviceType(UpnpDiscovery* p, const char* s,
                                               size_t n);
/*! UpnpDiscovery_clear_DeviceType */
PUPNP_API void UpnpDiscovery_clear_DeviceType(UpnpDiscovery* p);

/*! UpnpDiscovery_get_ServiceType */
PUPNP_API const UpnpString*
UpnpDiscovery_get_ServiceType(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_ServiceType */
PUPNP_API int UpnpDiscovery_set_ServiceType(UpnpDiscovery* p,
                                            const UpnpString* s);
/*! UpnpDiscovery_get_ServiceType_Length */
PUPNP_API size_t UpnpDiscovery_get_ServiceType_Length(const UpnpDiscovery* p);
/*! UpnpDiscovery_get_ServiceType_cstr */
PUPNP_API const char*
UpnpDiscovery_get_ServiceType_cstr(const UpnpDiscovery* p);
/*! UpnpDiscovery_strcpy_ServiceType */
PUPNP_API int UpnpDiscovery_strcpy_ServiceType(UpnpDiscovery* p, const char* s);
/*! UpnpDiscovery_strncpy_ServiceType */
PUPNP_API int UpnpDiscovery_strncpy_ServiceType(UpnpDiscovery* p, const char* s,
                                                size_t n);
/*! UpnpDiscovery_clear_ServiceType */
PUPNP_API void UpnpDiscovery_clear_ServiceType(UpnpDiscovery* p);

/*! UpnpDiscovery_get_ServiceVer */
PUPNP_API const UpnpString*
UpnpDiscovery_get_ServiceVer(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_ServiceVer */
PUPNP_API int UpnpDiscovery_set_ServiceVer(UpnpDiscovery* p,
                                           const UpnpString* s);
/*! UpnpDiscovery_get_ServiceVer_Length */
PUPNP_API size_t UpnpDiscovery_get_ServiceVer_Length(const UpnpDiscovery* p);
/*! UpnpDiscovery_get_ServiceVer_cstr */
PUPNP_API const char* UpnpDiscovery_get_ServiceVer_cstr(const UpnpDiscovery* p);
/*! UpnpDiscovery_strcpy_ServiceVer */
PUPNP_API int UpnpDiscovery_strcpy_ServiceVer(UpnpDiscovery* p, const char* s);
/*! UpnpDiscovery_strncpy_ServiceVer */
PUPNP_API int UpnpDiscovery_strncpy_ServiceVer(UpnpDiscovery* p, const char* s,
                                               size_t n);
/*! UpnpDiscovery_clear_ServiceVer */
PUPNP_API void UpnpDiscovery_clear_ServiceVer(UpnpDiscovery* p);

/*! UpnpDiscovery_get_Location */
PUPNP_API const UpnpString* UpnpDiscovery_get_Location(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_Location */
PUPNP_API int UpnpDiscovery_set_Location(UpnpDiscovery* p, const UpnpString* s);
/*! UpnpDiscovery_get_Location_Length */
PUPNP_API size_t UpnpDiscovery_get_Location_Length(const UpnpDiscovery* p);
/*! UpnpDiscovery_get_Location_cstr */
PUPNP_API const char* UpnpDiscovery_get_Location_cstr(const UpnpDiscovery* p);
/*! UpnpDiscovery_strcpy_Location */
PUPNP_API int UpnpDiscovery_strcpy_Location(UpnpDiscovery* p, const char* s);
/*! UpnpDiscovery_strncpy_Location */
PUPNP_API int UpnpDiscovery_strncpy_Location(UpnpDiscovery* p, const char* s,
                                             size_t n);
/*! UpnpDiscovery_clear_Location */
PUPNP_API void UpnpDiscovery_clear_Location(UpnpDiscovery* p);

/*! UpnpDiscovery_get_Os */
PUPNP_API const UpnpString* UpnpDiscovery_get_Os(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_Os */
PUPNP_API int UpnpDiscovery_set_Os(UpnpDiscovery* p, const UpnpString* s);
/*! UpnpDiscovery_get_Os_Length */
PUPNP_API size_t UpnpDiscovery_get_Os_Length(const UpnpDiscovery* p);
/*! UpnpDiscovery_get_Os_cstr */
PUPNP_API const char* UpnpDiscovery_get_Os_cstr(const UpnpDiscovery* p);
/*! UpnpDiscovery_strcpy_Os */
PUPNP_API int UpnpDiscovery_strcpy_Os(UpnpDiscovery* p, const char* s);
/*! UpnpDiscovery_strncpy_Os */
PUPNP_API int UpnpDiscovery_strncpy_Os(UpnpDiscovery* p, const char* s,
                                       size_t n);
/*! UpnpDiscovery_clear_Os */
PUPNP_API void UpnpDiscovery_clear_Os(UpnpDiscovery* p);

/*! UpnpDiscovery_get_Date */
PUPNP_API const UpnpString* UpnpDiscovery_get_Date(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_Date */
PUPNP_API int UpnpDiscovery_set_Date(UpnpDiscovery* p, const UpnpString* s);
/*! UpnpDiscovery_get_Date_Length */
PUPNP_API size_t UpnpDiscovery_get_Date_Length(const UpnpDiscovery* p);
/*! UpnpDiscovery_get_Date_cstr */
PUPNP_API const char* UpnpDiscovery_get_Date_cstr(const UpnpDiscovery* p);
/*! UpnpDiscovery_strcpy_Date */
PUPNP_API int UpnpDiscovery_strcpy_Date(UpnpDiscovery* p, const char* s);
/*! UpnpDiscovery_strncpy_Date */
PUPNP_API int UpnpDiscovery_strncpy_Date(UpnpDiscovery* p, const char* s,
                                         size_t n);
/*! UpnpDiscovery_clear_Date */
PUPNP_API void UpnpDiscovery_clear_Date(UpnpDiscovery* p);

/*! UpnpDiscovery_get_Ext */
PUPNP_API const UpnpString* UpnpDiscovery_get_Ext(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_Ext */
PUPNP_API int UpnpDiscovery_set_Ext(UpnpDiscovery* p, const UpnpString* s);
/*! UpnpDiscovery_get_Ext_Length */
PUPNP_API size_t UpnpDiscovery_get_Ext_Length(const UpnpDiscovery* p);
/*! UpnpDiscovery_get_Ext_cstr */
PUPNP_API const char* UpnpDiscovery_get_Ext_cstr(const UpnpDiscovery* p);
/*! UpnpDiscovery_strcpy_Ext */
PUPNP_API int UpnpDiscovery_strcpy_Ext(UpnpDiscovery* p, const char* s);
/*! UpnpDiscovery_strncpy_Ext */
PUPNP_API int UpnpDiscovery_strncpy_Ext(UpnpDiscovery* p, const char* s,
                                        size_t n);
/*! UpnpDiscovery_clear_Ext */
PUPNP_API void UpnpDiscovery_clear_Ext(UpnpDiscovery* p);

/*! UpnpDiscovery_get_DestAddr */
PUPNP_API const struct sockaddr_storage*
UpnpDiscovery_get_DestAddr(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_DestAddr */
PUPNP_API int UpnpDiscovery_set_DestAddr(UpnpDiscovery* p,
                                         const struct sockaddr_storage* buf);
/*! UpnpDiscovery_clear_DestAddr */
PUPNP_API void UpnpDiscovery_clear_DestAddr(UpnpDiscovery* p);

} // extern "C"

#endif /* COMPA_UPNPDISCOVERY_HPP */
