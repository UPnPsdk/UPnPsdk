// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-07-19

/*!
 * \file
 *
 * \brief Source file for UpnpSubscriptionRequest methods.
 * \author Marcelo Roberto Jimenez
 */
#include "config.hpp"

#include <stdlib.h> /* for calloc(), free() */
#include <string.h> /* for strlen(), strdup(), memset() */

#include "UpnpSubscriptionRequest.hpp"

struct s_UpnpSubscriptionRequest {
    UpnpString* m_ServiceId;
    UpnpString* m_UDN;
    UpnpString* m_SID;
};

UpnpSubscriptionRequest* UpnpSubscriptionRequest_new(void) {
    struct s_UpnpSubscriptionRequest* p = (s_UpnpSubscriptionRequest*)calloc(
        1, sizeof(struct s_UpnpSubscriptionRequest));

    if (!p)
        return 0;

    p->m_ServiceId = UpnpString_new();
    p->m_UDN = UpnpString_new();
    p->m_SID = UpnpString_new();

    return (UpnpSubscriptionRequest*)p;
}

void UpnpSubscriptionRequest_delete(UpnpSubscriptionRequest* q) {
    struct s_UpnpSubscriptionRequest* p = (struct s_UpnpSubscriptionRequest*)q;

    if (!p)
        return;

    UpnpString_delete(p->m_SID);
    p->m_SID = 0;
    UpnpString_delete(p->m_UDN);
    p->m_UDN = 0;
    UpnpString_delete(p->m_ServiceId);
    p->m_ServiceId = 0;

    free(p);
}

int UpnpSubscriptionRequest_assign(UpnpSubscriptionRequest* p,
                                   const UpnpSubscriptionRequest* q) {
    int ok = 1;

    if (p != q) {
        ok = ok && UpnpSubscriptionRequest_set_ServiceId(
                       p, UpnpSubscriptionRequest_get_ServiceId(q));
        ok = ok && UpnpSubscriptionRequest_set_UDN(
                       p, UpnpSubscriptionRequest_get_UDN(q));
        ok = ok && UpnpSubscriptionRequest_set_SID(
                       p, UpnpSubscriptionRequest_get_SID(q));
    }

    return ok;
}

UpnpSubscriptionRequest*
UpnpSubscriptionRequest_dup(const UpnpSubscriptionRequest* q) {
    UpnpSubscriptionRequest* p = UpnpSubscriptionRequest_new();

    if (!p)
        return 0;

    UpnpSubscriptionRequest_assign(p, q);

    return p;
}

const UpnpString*
UpnpSubscriptionRequest_get_ServiceId(const UpnpSubscriptionRequest* p) {
    return p->m_ServiceId;
}

int UpnpSubscriptionRequest_set_ServiceId(UpnpSubscriptionRequest* p,
                                          const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_ServiceId, q);
}

size_t
UpnpSubscriptionRequest_get_ServiceId_Length(const UpnpSubscriptionRequest* p) {
    return UpnpString_get_Length(UpnpSubscriptionRequest_get_ServiceId(p));
}

const char*
UpnpSubscriptionRequest_get_ServiceId_cstr(const UpnpSubscriptionRequest* p) {
    return UpnpString_get_String(UpnpSubscriptionRequest_get_ServiceId(p));
}

int UpnpSubscriptionRequest_strcpy_ServiceId(UpnpSubscriptionRequest* p,
                                             const char* s) {
    return UpnpString_set_String(p->m_ServiceId, s);
}

int UpnpSubscriptionRequest_strncpy_ServiceId(UpnpSubscriptionRequest* p,
                                              const char* s, size_t n) {
    return UpnpString_set_StringN(p->m_ServiceId, s, n);
}

void UpnpSubscriptionRequest_clear_ServiceId(UpnpSubscriptionRequest* p) {
    UpnpString_clear(p->m_ServiceId);
}

const UpnpString*
UpnpSubscriptionRequest_get_UDN(const UpnpSubscriptionRequest* p) {
    return p->m_UDN;
}

int UpnpSubscriptionRequest_set_UDN(UpnpSubscriptionRequest* p,
                                    const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_UDN, q);
}

size_t
UpnpSubscriptionRequest_get_UDN_Length(const UpnpSubscriptionRequest* p) {
    return UpnpString_get_Length(UpnpSubscriptionRequest_get_UDN(p));
}

const char*
UpnpSubscriptionRequest_get_UDN_cstr(const UpnpSubscriptionRequest* p) {
    return UpnpString_get_String(UpnpSubscriptionRequest_get_UDN(p));
}

int UpnpSubscriptionRequest_strcpy_UDN(UpnpSubscriptionRequest* p,
                                       const char* s) {
    return UpnpString_set_String(p->m_UDN, s);
}

int UpnpSubscriptionRequest_strncpy_UDN(UpnpSubscriptionRequest* p,
                                        const char* s, size_t n) {
    return UpnpString_set_StringN(p->m_UDN, s, n);
}

void UpnpSubscriptionRequest_clear_UDN(UpnpSubscriptionRequest* p) {
    UpnpString_clear(p->m_UDN);
}

const UpnpString*
UpnpSubscriptionRequest_get_SID(const UpnpSubscriptionRequest* p) {
    return p->m_SID;
}

int UpnpSubscriptionRequest_set_SID(UpnpSubscriptionRequest* p,
                                    const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_SID, q);
}

size_t
UpnpSubscriptionRequest_get_SID_Length(const UpnpSubscriptionRequest* p) {
    return UpnpString_get_Length(UpnpSubscriptionRequest_get_SID(p));
}

const char*
UpnpSubscriptionRequest_get_SID_cstr(const UpnpSubscriptionRequest* p) {
    return UpnpString_get_String(UpnpSubscriptionRequest_get_SID(p));
}

int UpnpSubscriptionRequest_strcpy_SID(UpnpSubscriptionRequest* p,
                                       const char* s) {
    return UpnpString_set_String(p->m_SID, s);
}

int UpnpSubscriptionRequest_strncpy_SID(UpnpSubscriptionRequest* p,
                                        const char* s, size_t n) {
    return UpnpString_set_StringN(p->m_SID, s, n);
}

void UpnpSubscriptionRequest_clear_SID(UpnpSubscriptionRequest* p) {
    UpnpString_clear(p->m_SID);
}
