#ifndef COMPA_UPNPFILEINFO_HPP
#define COMPA_UPNPFILEINFO_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-05-21
// Also Copyright by other contributor as noted below.
// Last compare with pupnp original source file on 2023-04-25, ver 1.14.15
/*!
 * \file
 * \brief Header file for UpnpFileInfo methods.
 * \authors Marcelo Roberto Jimenez, Ingo Höft
 */

#include <UpnpString.hpp>
#include <ixml.hpp>
#include <list.hpp>
#include <UPnPsdk/port_sock.hpp>

/// \cond
#ifdef _WIN32
#include <sys/types.h> // needed for off_t
#endif                 // _WIN32
/// \endcond

extern "C" {

/*!
 * UpnpFileInfo
 */
// The typedef must be the same as in pupnp otherwise we cannot switch between
// pupnp utest and compa utest. Using the typedef in the header file but the
// definiton of the structure in the source file make the mmembers of the
// structure publicy invisible. That is intended but we will change it with
// using C++ private. --Ingo
typedef struct s_UpnpFileInfo UpnpFileInfo;

/*! Constructor */
UPnPsdk_API UpnpFileInfo* UpnpFileInfo_new();
/*! Destructor */
UPnPsdk_API void UpnpFileInfo_delete(UpnpFileInfo* p);
/*! Copy Constructor */
UPnPsdk_API UpnpFileInfo* UpnpFileInfo_dup(const UpnpFileInfo* p);
/*! Assignment operator */
UPnPsdk_API int UpnpFileInfo_assign(UpnpFileInfo* p, const UpnpFileInfo* q);

/*! UpnpFileInfo_get_FileLength */
UPnPsdk_API off_t UpnpFileInfo_get_FileLength(const UpnpFileInfo* p);
/*! UpnpFileInfo_set_FileLength */
UPnPsdk_API int UpnpFileInfo_set_FileLength(UpnpFileInfo* p, off_t n);

/*! UpnpFileInfo_get_LastModified */
UPnPsdk_API time_t UpnpFileInfo_get_LastModified(const UpnpFileInfo* p);
/*! UpnpFileInfo_set_LastModified */
UPnPsdk_API int UpnpFileInfo_set_LastModified(UpnpFileInfo* p, time_t n);

/*! UpnpFileInfo_get_IsDirectory */
UPnPsdk_API int UpnpFileInfo_get_IsDirectory(const UpnpFileInfo* p);
/*! UpnpFileInfo_set_IsDirectory */
UPnPsdk_API int UpnpFileInfo_set_IsDirectory(UpnpFileInfo* p, int n);

/*! UpnpFileInfo_get_IsReadable */
UPnPsdk_API int UpnpFileInfo_get_IsReadable(const UpnpFileInfo* p);
/*! UpnpFileInfo_set_IsReadable */
UPnPsdk_API int UpnpFileInfo_set_IsReadable(UpnpFileInfo* p, int n);

/*! UpnpFileInfo_get_ContentType */
UPnPsdk_API const DOMString UpnpFileInfo_get_ContentType(const UpnpFileInfo* p);
/*! UpnpFileInfo_set_ContentType */
UPnPsdk_API int UpnpFileInfo_set_ContentType(UpnpFileInfo* p,
                                             const DOMString s);
/*! UpnpFileInfo_get_ContentType_cstr */
UPnPsdk_API const char*
UpnpFileInfo_get_ContentType_cstr(const UpnpFileInfo* p);

/*! UpnpFileInfo_get_ExtraHeadersList */
UPnPsdk_API const UpnpListHead*
UpnpFileInfo_get_ExtraHeadersList(const UpnpFileInfo* p);
/*! UpnpFileInfo_set_ExtraHeadersList */
UPnPsdk_API int UpnpFileInfo_set_ExtraHeadersList(UpnpFileInfo* p,
                                                  const UpnpListHead* q);
/*! UpnpFileInfo_add_to_list_ExtraHeadersList */
UPnPsdk_API void UpnpFileInfo_add_to_list_ExtraHeadersList(UpnpFileInfo* p,
                                                           UpnpListHead* head);

/*! UpnpFileInfo_get_CtrlPtIPAddr */
UPnPsdk_API const sockaddr_storage*
UpnpFileInfo_get_CtrlPtIPAddr(const UpnpFileInfo* p);
/*! UpnpFileInfo_set_CtrlPtIPAddr */
UPnPsdk_API int UpnpFileInfo_set_CtrlPtIPAddr(UpnpFileInfo* p,
                                              const sockaddr_storage* buf);
/*! UpnpFileInfo_clear_CtrlPtIPAddr */
UPnPsdk_API void UpnpFileInfo_clear_CtrlPtIPAddr(UpnpFileInfo* p);

/*! UpnpFileInfo_get_Os */
UPnPsdk_API const UpnpString* UpnpFileInfo_get_Os(const UpnpFileInfo* p);
/*! UpnpFileInfo_set_Os */
UPnPsdk_API int UpnpFileInfo_set_Os(UpnpFileInfo* p, const UpnpString* s);
/*! UpnpFileInfo_get_Os_Length */
UPnPsdk_API size_t UpnpFileInfo_get_Os_Length(const UpnpFileInfo* p);
/*! UpnpFileInfo_get_Os_cstr */
UPnPsdk_API const char* UpnpFileInfo_get_Os_cstr(const UpnpFileInfo* p);
/*! UpnpFileInfo_strcpy_Os */
UPnPsdk_API int UpnpFileInfo_strcpy_Os(UpnpFileInfo* p, const char* s);
/*! UpnpFileInfo_strncpy_Os */
UPnPsdk_API int UpnpFileInfo_strncpy_Os(UpnpFileInfo* p, const char* s,
                                        size_t n);
/*! UpnpFileInfo_clear_Os */
UPnPsdk_API void UpnpFileInfo_clear_Os(UpnpFileInfo* p);

} // extern "C"

#endif /* COMPA_UPNPFILEINFO_HPP */
