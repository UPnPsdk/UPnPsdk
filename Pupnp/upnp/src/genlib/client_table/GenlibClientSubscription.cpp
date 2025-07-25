// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-07-19
// Also Copyright by other contributor.

/*!
 * \file
 *
 * \brief Source file for GenlibClientSubscription methods.
 * \author Marcelo Roberto Jimenez
 */
#include "config.hpp"

#include "GenlibClientSubscription.hpp"
#include "UpnpString.hpp"

#include <stdlib.h> /* for calloc(), free() */
#include <string.h> /* for strlen(), strdup() memset() */

struct s_GenlibClientSubscription {
    int m_RenewEventId;
    UpnpString* m_SID;
    UpnpString* m_ActualSID;
    UpnpString* m_EventURL;
    GenlibClientSubscription* m_Next;
};

GenlibClientSubscription* GenlibClientSubscription_new(void) {
    struct s_GenlibClientSubscription* p = (s_GenlibClientSubscription*)calloc(
        1, sizeof(struct s_GenlibClientSubscription));

    if (!p)
        return 0;

    /*p->m_RenewEventId = 0;*/
    p->m_SID = UpnpString_new();
    p->m_ActualSID = UpnpString_new();
    p->m_EventURL = UpnpString_new();
    /*p->m_Next = 0;*/

    return (GenlibClientSubscription*)p;
}

void GenlibClientSubscription_delete(GenlibClientSubscription* q) {
    struct s_GenlibClientSubscription* p =
        (struct s_GenlibClientSubscription*)q;

    if (!p)
        return;

    p->m_Next = 0;
    UpnpString_delete(p->m_EventURL);
    p->m_EventURL = 0;
    UpnpString_delete(p->m_ActualSID);
    p->m_ActualSID = 0;
    UpnpString_delete(p->m_SID);
    p->m_SID = 0;
    p->m_RenewEventId = 0;

    free(p);
}

int GenlibClientSubscription_assign(GenlibClientSubscription* p,
                                    const GenlibClientSubscription* q) {
    int ok = 1;

    if (p != q) {
        ok = ok && GenlibClientSubscription_set_RenewEventId(
                       p, GenlibClientSubscription_get_RenewEventId(q));
        ok = ok && GenlibClientSubscription_set_SID(
                       p, GenlibClientSubscription_get_SID(q));
        ok = ok && GenlibClientSubscription_set_ActualSID(
                       p, GenlibClientSubscription_get_ActualSID(q));
        ok = ok && GenlibClientSubscription_set_EventURL(
                       p, GenlibClientSubscription_get_EventURL(q));
        ok = ok && GenlibClientSubscription_set_Next(
                       p, GenlibClientSubscription_get_Next(q));
    }

    return ok;
}

GenlibClientSubscription*
GenlibClientSubscription_dup(const GenlibClientSubscription* q) {
    GenlibClientSubscription* p = GenlibClientSubscription_new();

    if (!p)
        return 0;

    GenlibClientSubscription_assign(p, q);

    return p;
}

int GenlibClientSubscription_get_RenewEventId(
    const GenlibClientSubscription* p) {
    return p->m_RenewEventId;
}

int GenlibClientSubscription_set_RenewEventId(GenlibClientSubscription* p,
                                              int n) {
    p->m_RenewEventId = n;

    return 1;
}

const UpnpString*
GenlibClientSubscription_get_SID(const GenlibClientSubscription* p) {
    return p->m_SID;
}

int GenlibClientSubscription_set_SID(GenlibClientSubscription* p,
                                     const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_SID, q);
}

size_t
GenlibClientSubscription_get_SID_Length(const GenlibClientSubscription* p) {
    return UpnpString_get_Length(GenlibClientSubscription_get_SID(p));
}

const char*
GenlibClientSubscription_get_SID_cstr(const GenlibClientSubscription* p) {
    return UpnpString_get_String(GenlibClientSubscription_get_SID(p));
}

int GenlibClientSubscription_strcpy_SID(GenlibClientSubscription* p,
                                        const char* s) {
    return UpnpString_set_String(p->m_SID, s);
}

int GenlibClientSubscription_strncpy_SID(GenlibClientSubscription* p,
                                         const char* s, size_t n) {
    return UpnpString_set_StringN(p->m_SID, s, n);
}

void GenlibClientSubscription_clear_SID(GenlibClientSubscription* p) {
    UpnpString_clear(p->m_SID);
}

const UpnpString*
GenlibClientSubscription_get_ActualSID(const GenlibClientSubscription* p) {
    return p->m_ActualSID;
}

int GenlibClientSubscription_set_ActualSID(GenlibClientSubscription* p,
                                           const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_ActualSID, q);
}

size_t GenlibClientSubscription_get_ActualSID_Length(
    const GenlibClientSubscription* p) {
    return UpnpString_get_Length(GenlibClientSubscription_get_ActualSID(p));
}

const char*
GenlibClientSubscription_get_ActualSID_cstr(const GenlibClientSubscription* p) {
    return UpnpString_get_String(GenlibClientSubscription_get_ActualSID(p));
}

int GenlibClientSubscription_strcpy_ActualSID(GenlibClientSubscription* p,
                                              const char* s) {
    return UpnpString_set_String(p->m_ActualSID, s);
}

int GenlibClientSubscription_strncpy_ActualSID(GenlibClientSubscription* p,
                                               const char* s, size_t n) {
    return UpnpString_set_StringN(p->m_ActualSID, s, n);
}

void GenlibClientSubscription_clear_ActualSID(GenlibClientSubscription* p) {
    UpnpString_clear(p->m_ActualSID);
}

const UpnpString*
GenlibClientSubscription_get_EventURL(const GenlibClientSubscription* p) {
    return p->m_EventURL;
}

int GenlibClientSubscription_set_EventURL(GenlibClientSubscription* p,
                                          const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_EventURL, q);
}

size_t GenlibClientSubscription_get_EventURL_Length(
    const GenlibClientSubscription* p) {
    return UpnpString_get_Length(GenlibClientSubscription_get_EventURL(p));
}

const char*
GenlibClientSubscription_get_EventURL_cstr(const GenlibClientSubscription* p) {
    return UpnpString_get_String(GenlibClientSubscription_get_EventURL(p));
}

int GenlibClientSubscription_strcpy_EventURL(GenlibClientSubscription* p,
                                             const char* s) {
    return UpnpString_set_String(p->m_EventURL, s);
}

int GenlibClientSubscription_strncpy_EventURL(GenlibClientSubscription* p,
                                              const char* s, size_t n) {
    return UpnpString_set_StringN(p->m_EventURL, s, n);
}

void GenlibClientSubscription_clear_EventURL(GenlibClientSubscription* p) {
    UpnpString_clear(p->m_EventURL);
}

GenlibClientSubscription*
GenlibClientSubscription_get_Next(const GenlibClientSubscription* p) {
    return p->m_Next;
}

int GenlibClientSubscription_set_Next(GenlibClientSubscription* p,
                                      GenlibClientSubscription* n) {
    p->m_Next = n;

    return 1;
}
