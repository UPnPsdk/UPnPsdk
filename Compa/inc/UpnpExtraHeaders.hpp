#ifndef COMPA_UPNPEXTRAHEADERS_HPP
#define COMPA_UPNPEXTRAHEADERS_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-02
// Last compare with ./Pupnp source file on 2025-05-22, ver 1.14.20
/*!
 * \file
 * \brief Header file for UpnpExtraHeaders methods.
 * \authors Marcelo Roberto Jimenez, Ingo Höft
 */

#include <UpnpString.hpp>
#include <ixml/ixml.hpp>
#include <list.hpp>

/*!
 * UpnpExtraHeaders
 */
typedef struct s_UpnpExtraHeaders UpnpExtraHeaders;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*! Constructor */
PUPNP_API UpnpExtraHeaders* UpnpExtraHeaders_new();
/*! Destructor */
PUPNP_API void UpnpExtraHeaders_delete(UpnpExtraHeaders* p);
/*! Copy Constructor */
PUPNP_API UpnpExtraHeaders* UpnpExtraHeaders_dup(const UpnpExtraHeaders* p);
/*! Assignment operator */
PUPNP_API int UpnpExtraHeaders_assign(UpnpExtraHeaders* p,
                                      const UpnpExtraHeaders* q);

/*! UpnpExtraHeaders_get_node */
PUPNP_API const UpnpListHead*
UpnpExtraHeaders_get_node(const UpnpExtraHeaders* p);
/*! UpnpExtraHeaders_set_node */
PUPNP_API int UpnpExtraHeaders_set_node(UpnpExtraHeaders* p,
                                        const UpnpListHead* q);
/*! UpnpExtraHeaders_add_to_list_node */
PUPNP_API void UpnpExtraHeaders_add_to_list_node(UpnpExtraHeaders* p,
                                                 UpnpListHead* head);

/*! UpnpExtraHeaders_get_name */
PUPNP_API const UpnpString*
UpnpExtraHeaders_get_name(const UpnpExtraHeaders* p);
/*! UpnpExtraHeaders_set_name */
PUPNP_API int UpnpExtraHeaders_set_name(UpnpExtraHeaders* p,
                                        const UpnpString* s);
/*! UpnpExtraHeaders_get_name_Length */
PUPNP_API size_t UpnpExtraHeaders_get_name_Length(const UpnpExtraHeaders* p);
/*! UpnpExtraHeaders_get_name_cstr */
PUPNP_API const char* UpnpExtraHeaders_get_name_cstr(const UpnpExtraHeaders* p);
/*! UpnpExtraHeaders_strcpy_name */
PUPNP_API int UpnpExtraHeaders_strcpy_name(UpnpExtraHeaders* p, const char* s);
/*! UpnpExtraHeaders_strncpy_name */
PUPNP_API int UpnpExtraHeaders_strncpy_name(UpnpExtraHeaders* p, const char* s,
                                            size_t n);
/*! UpnpExtraHeaders_clear_name */
PUPNP_API void UpnpExtraHeaders_clear_name(UpnpExtraHeaders* p);

/*! UpnpExtraHeaders_get_value */
PUPNP_API const UpnpString*
UpnpExtraHeaders_get_value(const UpnpExtraHeaders* p);
/*! UpnpExtraHeaders_set_value */
PUPNP_API int UpnpExtraHeaders_set_value(UpnpExtraHeaders* p,
                                         const UpnpString* s);
/*! UpnpExtraHeaders_get_value_Length */
PUPNP_API size_t UpnpExtraHeaders_get_value_Length(const UpnpExtraHeaders* p);
/*! UpnpExtraHeaders_get_value_cstr */
PUPNP_API const char*
UpnpExtraHeaders_get_value_cstr(const UpnpExtraHeaders* p);
/*! UpnpExtraHeaders_strcpy_value */
PUPNP_API int UpnpExtraHeaders_strcpy_value(UpnpExtraHeaders* p, const char* s);
/*! UpnpExtraHeaders_strncpy_value */
PUPNP_API int UpnpExtraHeaders_strncpy_value(UpnpExtraHeaders* p, const char* s,
                                             size_t n);
/*! UpnpExtraHeaders_clear_value */
PUPNP_API void UpnpExtraHeaders_clear_value(UpnpExtraHeaders* p);

/*! UpnpExtraHeaders_get_resp */
PUPNP_API const DOMString UpnpExtraHeaders_get_resp(const UpnpExtraHeaders* p);
/*! UpnpExtraHeaders_set_resp */
PUPNP_API int UpnpExtraHeaders_set_resp(UpnpExtraHeaders* p, const DOMString s);
/*! UpnpExtraHeaders_get_resp_cstr */
PUPNP_API const char* UpnpExtraHeaders_get_resp_cstr(const UpnpExtraHeaders* p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COMPA_UPNPEXTRAHEADERS_HPP */
