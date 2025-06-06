#ifndef UPNPLIB_UPNPDISCOVERY_HPP
#define UPNPLIB_UPNPDISCOVERY_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-01
// Also Copyright by other contributor as noted below.
// Last compare with pupnp original source file on 2025-05-23, ver 1.14.20

/*!
 * \file
 *
 * \brief Header file for UpnpDiscovery methods.
 * \authors Marcelo Roberto Jimenez, Ingo Höft
 */
#include <stdlib.h>       /* for size_t */

#include "UpnpGlobal.hpp" /* for EXPORT_SPEC */

#include "UpnpString.hpp"
#include "UpnpInet.hpp"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*!
 * UpnpDiscovery
 */
typedef struct s_UpnpDiscovery UpnpDiscovery;

/*! Constructor */
EXPORT_SPEC UpnpDiscovery* UpnpDiscovery_new(void);
/*! Destructor */
EXPORT_SPEC void UpnpDiscovery_delete(UpnpDiscovery* p);
/*! Copy Constructor */
EXPORT_SPEC UpnpDiscovery* UpnpDiscovery_dup(const UpnpDiscovery* p);
/*! Assignment operator */
EXPORT_SPEC int UpnpDiscovery_assign(UpnpDiscovery* p, const UpnpDiscovery* q);

/*! UpnpDiscovery_get_ErrCode */
EXPORT_SPEC int UpnpDiscovery_get_ErrCode(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_ErrCode */
EXPORT_SPEC int UpnpDiscovery_set_ErrCode(UpnpDiscovery* p, int n);

/*! UpnpDiscovery_get_Expires */
EXPORT_SPEC int UpnpDiscovery_get_Expires(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_Expires */
EXPORT_SPEC int UpnpDiscovery_set_Expires(UpnpDiscovery* p, int n);

/*! UpnpDiscovery_get_DeviceID */
EXPORT_SPEC const UpnpString*
UpnpDiscovery_get_DeviceID(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_DeviceID */
EXPORT_SPEC int UpnpDiscovery_set_DeviceID(UpnpDiscovery* p,
                                           const UpnpString* s);
/*! UpnpDiscovery_get_DeviceID_Length */
EXPORT_SPEC size_t UpnpDiscovery_get_DeviceID_Length(const UpnpDiscovery* p);
/*! UpnpDiscovery_get_DeviceID_cstr */
EXPORT_SPEC const char* UpnpDiscovery_get_DeviceID_cstr(const UpnpDiscovery* p);
/*! UpnpDiscovery_strcpy_DeviceID */
EXPORT_SPEC int UpnpDiscovery_strcpy_DeviceID(UpnpDiscovery* p, const char* s);
/*! UpnpDiscovery_strncpy_DeviceID */
EXPORT_SPEC int UpnpDiscovery_strncpy_DeviceID(UpnpDiscovery* p, const char* s,
                                               size_t n);
/*! UpnpDiscovery_clear_DeviceID */
EXPORT_SPEC void UpnpDiscovery_clear_DeviceID(UpnpDiscovery* p);

/*! UpnpDiscovery_get_DeviceType */
EXPORT_SPEC const UpnpString*
UpnpDiscovery_get_DeviceType(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_DeviceType */
EXPORT_SPEC int UpnpDiscovery_set_DeviceType(UpnpDiscovery* p,
                                             const UpnpString* s);
/*! UpnpDiscovery_get_DeviceType_Length */
EXPORT_SPEC size_t UpnpDiscovery_get_DeviceType_Length(const UpnpDiscovery* p);
/*! UpnpDiscovery_get_DeviceType_cstr */
EXPORT_SPEC const char*
UpnpDiscovery_get_DeviceType_cstr(const UpnpDiscovery* p);
/*! UpnpDiscovery_strcpy_DeviceType */
EXPORT_SPEC int UpnpDiscovery_strcpy_DeviceType(UpnpDiscovery* p,
                                                const char* s);
/*! UpnpDiscovery_strncpy_DeviceType */
EXPORT_SPEC int UpnpDiscovery_strncpy_DeviceType(UpnpDiscovery* p,
                                                 const char* s, size_t n);
/*! UpnpDiscovery_clear_DeviceType */
EXPORT_SPEC void UpnpDiscovery_clear_DeviceType(UpnpDiscovery* p);

/*! UpnpDiscovery_get_ServiceType */
EXPORT_SPEC const UpnpString*
UpnpDiscovery_get_ServiceType(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_ServiceType */
EXPORT_SPEC int UpnpDiscovery_set_ServiceType(UpnpDiscovery* p,
                                              const UpnpString* s);
/*! UpnpDiscovery_get_ServiceType_Length */
EXPORT_SPEC size_t UpnpDiscovery_get_ServiceType_Length(const UpnpDiscovery* p);
/*! UpnpDiscovery_get_ServiceType_cstr */
EXPORT_SPEC const char*
UpnpDiscovery_get_ServiceType_cstr(const UpnpDiscovery* p);
/*! UpnpDiscovery_strcpy_ServiceType */
EXPORT_SPEC int UpnpDiscovery_strcpy_ServiceType(UpnpDiscovery* p,
                                                 const char* s);
/*! UpnpDiscovery_strncpy_ServiceType */
EXPORT_SPEC int UpnpDiscovery_strncpy_ServiceType(UpnpDiscovery* p,
                                                  const char* s, size_t n);
/*! UpnpDiscovery_clear_ServiceType */
EXPORT_SPEC void UpnpDiscovery_clear_ServiceType(UpnpDiscovery* p);

/*! UpnpDiscovery_get_ServiceVer */
EXPORT_SPEC const UpnpString*
UpnpDiscovery_get_ServiceVer(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_ServiceVer */
EXPORT_SPEC int UpnpDiscovery_set_ServiceVer(UpnpDiscovery* p,
                                             const UpnpString* s);
/*! UpnpDiscovery_get_ServiceVer_Length */
EXPORT_SPEC size_t UpnpDiscovery_get_ServiceVer_Length(const UpnpDiscovery* p);
/*! UpnpDiscovery_get_ServiceVer_cstr */
EXPORT_SPEC const char*
UpnpDiscovery_get_ServiceVer_cstr(const UpnpDiscovery* p);
/*! UpnpDiscovery_strcpy_ServiceVer */
EXPORT_SPEC int UpnpDiscovery_strcpy_ServiceVer(UpnpDiscovery* p,
                                                const char* s);
/*! UpnpDiscovery_strncpy_ServiceVer */
EXPORT_SPEC int UpnpDiscovery_strncpy_ServiceVer(UpnpDiscovery* p,
                                                 const char* s, size_t n);
/*! UpnpDiscovery_clear_ServiceVer */
EXPORT_SPEC void UpnpDiscovery_clear_ServiceVer(UpnpDiscovery* p);

/*! UpnpDiscovery_get_Location */
EXPORT_SPEC const UpnpString*
UpnpDiscovery_get_Location(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_Location */
EXPORT_SPEC int UpnpDiscovery_set_Location(UpnpDiscovery* p,
                                           const UpnpString* s);
/*! UpnpDiscovery_get_Location_Length */
EXPORT_SPEC size_t UpnpDiscovery_get_Location_Length(const UpnpDiscovery* p);
/*! UpnpDiscovery_get_Location_cstr */
EXPORT_SPEC const char* UpnpDiscovery_get_Location_cstr(const UpnpDiscovery* p);
/*! UpnpDiscovery_strcpy_Location */
EXPORT_SPEC int UpnpDiscovery_strcpy_Location(UpnpDiscovery* p, const char* s);
/*! UpnpDiscovery_strncpy_Location */
EXPORT_SPEC int UpnpDiscovery_strncpy_Location(UpnpDiscovery* p, const char* s,
                                               size_t n);
/*! UpnpDiscovery_clear_Location */
EXPORT_SPEC void UpnpDiscovery_clear_Location(UpnpDiscovery* p);

/*! UpnpDiscovery_get_Os */
EXPORT_SPEC const UpnpString* UpnpDiscovery_get_Os(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_Os */
EXPORT_SPEC int UpnpDiscovery_set_Os(UpnpDiscovery* p, const UpnpString* s);
/*! UpnpDiscovery_get_Os_Length */
EXPORT_SPEC size_t UpnpDiscovery_get_Os_Length(const UpnpDiscovery* p);
/*! UpnpDiscovery_get_Os_cstr */
EXPORT_SPEC const char* UpnpDiscovery_get_Os_cstr(const UpnpDiscovery* p);
/*! UpnpDiscovery_strcpy_Os */
EXPORT_SPEC int UpnpDiscovery_strcpy_Os(UpnpDiscovery* p, const char* s);
/*! UpnpDiscovery_strncpy_Os */
EXPORT_SPEC int UpnpDiscovery_strncpy_Os(UpnpDiscovery* p, const char* s,
                                         size_t n);
/*! UpnpDiscovery_clear_Os */
EXPORT_SPEC void UpnpDiscovery_clear_Os(UpnpDiscovery* p);

/*! UpnpDiscovery_get_Date */
EXPORT_SPEC const UpnpString* UpnpDiscovery_get_Date(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_Date */
EXPORT_SPEC int UpnpDiscovery_set_Date(UpnpDiscovery* p, const UpnpString* s);
/*! UpnpDiscovery_get_Date_Length */
EXPORT_SPEC size_t UpnpDiscovery_get_Date_Length(const UpnpDiscovery* p);
/*! UpnpDiscovery_get_Date_cstr */
EXPORT_SPEC const char* UpnpDiscovery_get_Date_cstr(const UpnpDiscovery* p);
/*! UpnpDiscovery_strcpy_Date */
EXPORT_SPEC int UpnpDiscovery_strcpy_Date(UpnpDiscovery* p, const char* s);
/*! UpnpDiscovery_strncpy_Date */
EXPORT_SPEC int UpnpDiscovery_strncpy_Date(UpnpDiscovery* p, const char* s,
                                           size_t n);
/*! UpnpDiscovery_clear_Date */
EXPORT_SPEC void UpnpDiscovery_clear_Date(UpnpDiscovery* p);

/*! UpnpDiscovery_get_Ext */
EXPORT_SPEC const UpnpString* UpnpDiscovery_get_Ext(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_Ext */
EXPORT_SPEC int UpnpDiscovery_set_Ext(UpnpDiscovery* p, const UpnpString* s);
/*! UpnpDiscovery_get_Ext_Length */
EXPORT_SPEC size_t UpnpDiscovery_get_Ext_Length(const UpnpDiscovery* p);
/*! UpnpDiscovery_get_Ext_cstr */
EXPORT_SPEC const char* UpnpDiscovery_get_Ext_cstr(const UpnpDiscovery* p);
/*! UpnpDiscovery_strcpy_Ext */
EXPORT_SPEC int UpnpDiscovery_strcpy_Ext(UpnpDiscovery* p, const char* s);
/*! UpnpDiscovery_strncpy_Ext */
EXPORT_SPEC int UpnpDiscovery_strncpy_Ext(UpnpDiscovery* p, const char* s,
                                          size_t n);
/*! UpnpDiscovery_clear_Ext */
EXPORT_SPEC void UpnpDiscovery_clear_Ext(UpnpDiscovery* p);

/*! UpnpDiscovery_get_DestAddr */
EXPORT_SPEC const struct sockaddr_storage*
UpnpDiscovery_get_DestAddr(const UpnpDiscovery* p);
/*! UpnpDiscovery_set_DestAddr */
EXPORT_SPEC int UpnpDiscovery_set_DestAddr(UpnpDiscovery* p,
                                           const struct sockaddr_storage* buf);
/*! UpnpDiscovery_clear_DestAddr */
EXPORT_SPEC void UpnpDiscovery_clear_DestAddr(UpnpDiscovery* p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* UPNPLIB_UPNPDISCOVERY_HPP */
