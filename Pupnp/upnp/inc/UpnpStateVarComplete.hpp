#ifndef UPNPLIB_UPNPSTATEVARCOMPLETE_HPP
#define UPNPLIB_UPNPSTATEVARCOMPLETE_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-01
// Also Copyright by other contributor as noted below.
// Last compare with pupnp original source file on 2025-05-23, ver 1.14.20

/*!
 * \file
 *
 * \brief Header file for UpnpStateVarComplete methods.
 * \authors Marcelo Roberto Jimenez, Ingo Höft
 */
#include <stdlib.h>       /* for size_t */

#include "UpnpGlobal.hpp" /* for EXPORT_SPEC */

#include "UpnpString.hpp"
#include "ixml.hpp"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*!
 * UpnpStateVarComplete
 */
typedef struct s_UpnpStateVarComplete UpnpStateVarComplete;

/*! Constructor */
EXPORT_SPEC UpnpStateVarComplete* UpnpStateVarComplete_new(void);
/*! Destructor */
EXPORT_SPEC void UpnpStateVarComplete_delete(UpnpStateVarComplete* p);
/*! Copy Constructor */
EXPORT_SPEC UpnpStateVarComplete*
UpnpStateVarComplete_dup(const UpnpStateVarComplete* p);
/*! Assignment operator */
EXPORT_SPEC int UpnpStateVarComplete_assign(UpnpStateVarComplete* p,
                                            const UpnpStateVarComplete* q);

/*! UpnpStateVarComplete_get_ErrCode */
EXPORT_SPEC int UpnpStateVarComplete_get_ErrCode(const UpnpStateVarComplete* p);
/*! UpnpStateVarComplete_set_ErrCode */
EXPORT_SPEC int UpnpStateVarComplete_set_ErrCode(UpnpStateVarComplete* p,
                                                 int n);

/*! UpnpStateVarComplete_get_CtrlUrl */
EXPORT_SPEC const UpnpString*
UpnpStateVarComplete_get_CtrlUrl(const UpnpStateVarComplete* p);
/*! UpnpStateVarComplete_set_CtrlUrl */
EXPORT_SPEC int UpnpStateVarComplete_set_CtrlUrl(UpnpStateVarComplete* p,
                                                 const UpnpString* s);
/*! UpnpStateVarComplete_get_CtrlUrl_Length */
EXPORT_SPEC size_t
UpnpStateVarComplete_get_CtrlUrl_Length(const UpnpStateVarComplete* p);
/*! UpnpStateVarComplete_get_CtrlUrl_cstr */
EXPORT_SPEC const char*
UpnpStateVarComplete_get_CtrlUrl_cstr(const UpnpStateVarComplete* p);
/*! UpnpStateVarComplete_strcpy_CtrlUrl */
EXPORT_SPEC int UpnpStateVarComplete_strcpy_CtrlUrl(UpnpStateVarComplete* p,
                                                    const char* s);
/*! UpnpStateVarComplete_strncpy_CtrlUrl */
EXPORT_SPEC int UpnpStateVarComplete_strncpy_CtrlUrl(UpnpStateVarComplete* p,
                                                     const char* s, size_t n);
/*! UpnpStateVarComplete_clear_CtrlUrl */
EXPORT_SPEC void UpnpStateVarComplete_clear_CtrlUrl(UpnpStateVarComplete* p);

/*! UpnpStateVarComplete_get_StateVarName */
EXPORT_SPEC const UpnpString*
UpnpStateVarComplete_get_StateVarName(const UpnpStateVarComplete* p);
/*! UpnpStateVarComplete_set_StateVarName */
EXPORT_SPEC int UpnpStateVarComplete_set_StateVarName(UpnpStateVarComplete* p,
                                                      const UpnpString* s);
/*! UpnpStateVarComplete_get_StateVarName_Length */
EXPORT_SPEC size_t
UpnpStateVarComplete_get_StateVarName_Length(const UpnpStateVarComplete* p);
/*! UpnpStateVarComplete_get_StateVarName_cstr */
EXPORT_SPEC const char*
UpnpStateVarComplete_get_StateVarName_cstr(const UpnpStateVarComplete* p);
/*! UpnpStateVarComplete_strcpy_StateVarName */
EXPORT_SPEC int
UpnpStateVarComplete_strcpy_StateVarName(UpnpStateVarComplete* p,
                                         const char* s);
/*! UpnpStateVarComplete_strncpy_StateVarName */
EXPORT_SPEC int
UpnpStateVarComplete_strncpy_StateVarName(UpnpStateVarComplete* p,
                                          const char* s, size_t n);
/*! UpnpStateVarComplete_clear_StateVarName */
EXPORT_SPEC void
UpnpStateVarComplete_clear_StateVarName(UpnpStateVarComplete* p);

/*! UpnpStateVarComplete_get_CurrentVal */
EXPORT_SPEC const DOMString
UpnpStateVarComplete_get_CurrentVal(const UpnpStateVarComplete* p);
/*! UpnpStateVarComplete_set_CurrentVal */
EXPORT_SPEC int UpnpStateVarComplete_set_CurrentVal(UpnpStateVarComplete* p,
                                                    const DOMString s);
/*! UpnpStateVarComplete_get_CurrentVal_cstr */
EXPORT_SPEC const char*
UpnpStateVarComplete_get_CurrentVal_cstr(const UpnpStateVarComplete* p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* UPNPLIB_UPNPSTATEVARCOMPLETE_HPP */
