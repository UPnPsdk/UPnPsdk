#ifndef COMPA_UPNPSTATEVARCOMPLETE_HPP
#define COMPA_UPNPSTATEVARCOMPLETE_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-12
// Last compare with ./Pupnp source file on 2025-05-23, ver 1.14.20
/*!
 * \file
 * \brief Header file for UpnpStateVarComplete methods.
 * \authors Marcelo Roberto Jimenez, Ingo Höft
 */

#include <UpnpString.hpp>
#include <ixml/ixml.hpp>

/// \cond
#include <stdlib.h> /* for size_t */
/// \endcond

/*!
 * UpnpStateVarComplete
 */
typedef struct s_UpnpStateVarComplete UpnpStateVarComplete;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*! Constructor */
PUPNP_Api UpnpStateVarComplete* UpnpStateVarComplete_new(void);
/*! Destructor */
PUPNP_Api void UpnpStateVarComplete_delete(UpnpStateVarComplete* p);
/*! Copy Constructor */
PUPNP_Api UpnpStateVarComplete*
UpnpStateVarComplete_dup(const UpnpStateVarComplete* p);
/*! Assignment operator */
PUPNP_Api int UpnpStateVarComplete_assign(UpnpStateVarComplete* p,
                                          const UpnpStateVarComplete* q);

/*! UpnpStateVarComplete_get_ErrCode */
PUPNP_Api int UpnpStateVarComplete_get_ErrCode(const UpnpStateVarComplete* p);
/*! UpnpStateVarComplete_set_ErrCode */
PUPNP_Api int UpnpStateVarComplete_set_ErrCode(UpnpStateVarComplete* p, int n);

/*! UpnpStateVarComplete_get_CtrlUrl */
PUPNP_Api const UpnpString*
UpnpStateVarComplete_get_CtrlUrl(const UpnpStateVarComplete* p);
/*! UpnpStateVarComplete_set_CtrlUrl */
PUPNP_Api int UpnpStateVarComplete_set_CtrlUrl(UpnpStateVarComplete* p,
                                               const UpnpString* s);
/*! UpnpStateVarComplete_get_CtrlUrl_Length */
PUPNP_Api size_t
UpnpStateVarComplete_get_CtrlUrl_Length(const UpnpStateVarComplete* p);
/*! UpnpStateVarComplete_get_CtrlUrl_cstr */
PUPNP_Api const char*
UpnpStateVarComplete_get_CtrlUrl_cstr(const UpnpStateVarComplete* p);
/*! UpnpStateVarComplete_strcpy_CtrlUrl */
PUPNP_Api int UpnpStateVarComplete_strcpy_CtrlUrl(UpnpStateVarComplete* p,
                                                  const char* s);
/*! UpnpStateVarComplete_strncpy_CtrlUrl */
PUPNP_Api int UpnpStateVarComplete_strncpy_CtrlUrl(UpnpStateVarComplete* p,
                                                   const char* s, size_t n);
/*! UpnpStateVarComplete_clear_CtrlUrl */
PUPNP_Api void UpnpStateVarComplete_clear_CtrlUrl(UpnpStateVarComplete* p);

/*! UpnpStateVarComplete_get_StateVarName */
PUPNP_Api const UpnpString*
UpnpStateVarComplete_get_StateVarName(const UpnpStateVarComplete* p);
/*! UpnpStateVarComplete_set_StateVarName */
PUPNP_Api int UpnpStateVarComplete_set_StateVarName(UpnpStateVarComplete* p,
                                                    const UpnpString* s);
/*! UpnpStateVarComplete_get_StateVarName_Length */
PUPNP_Api size_t
UpnpStateVarComplete_get_StateVarName_Length(const UpnpStateVarComplete* p);
/*! UpnpStateVarComplete_get_StateVarName_cstr */
PUPNP_Api const char*
UpnpStateVarComplete_get_StateVarName_cstr(const UpnpStateVarComplete* p);
/*! UpnpStateVarComplete_strcpy_StateVarName */
PUPNP_Api int UpnpStateVarComplete_strcpy_StateVarName(UpnpStateVarComplete* p,
                                                       const char* s);
/*! UpnpStateVarComplete_strncpy_StateVarName */
PUPNP_Api int UpnpStateVarComplete_strncpy_StateVarName(UpnpStateVarComplete* p,
                                                        const char* s,
                                                        size_t n);
/*! UpnpStateVarComplete_clear_StateVarName */
PUPNP_Api void UpnpStateVarComplete_clear_StateVarName(UpnpStateVarComplete* p);

/*! UpnpStateVarComplete_get_CurrentVal */
PUPNP_Api const DOMString
UpnpStateVarComplete_get_CurrentVal(const UpnpStateVarComplete* p);
/*! UpnpStateVarComplete_set_CurrentVal */
PUPNP_Api int UpnpStateVarComplete_set_CurrentVal(UpnpStateVarComplete* p,
                                                  const DOMString s);
/*! UpnpStateVarComplete_get_CurrentVal_cstr */
PUPNP_Api const char*
UpnpStateVarComplete_get_CurrentVal_cstr(const UpnpStateVarComplete* p);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* COMPA_UPNPSTATEVARCOMPLETE_HPP */
