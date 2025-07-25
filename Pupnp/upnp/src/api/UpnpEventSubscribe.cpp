// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-07-19

/*!
 * \file
 *
 * \brief Source file for UpnpEventSubscribe methods.
 * \author Marcelo Roberto Jimenez
 */
#include "config.hpp"

#include <stdlib.h> /* for calloc(), free() */
#include <string.h> /* for strlen(), strdup(), memset() */

#include "UpnpEventSubscribe.hpp"

struct s_UpnpEventSubscribe {
    int m_ErrCode;
    int m_TimeOut;
    UpnpString* m_SID;
    UpnpString* m_PublisherUrl;
};

UpnpEventSubscribe* UpnpEventSubscribe_new(void) {
    struct s_UpnpEventSubscribe* p =
        (s_UpnpEventSubscribe*)calloc(1, sizeof(struct s_UpnpEventSubscribe));

    if (!p)
        return 0;

    /*p->m_ErrCode = 0;*/
    /*p->m_TimeOut = 0;*/
    p->m_SID = UpnpString_new();
    p->m_PublisherUrl = UpnpString_new();

    return (UpnpEventSubscribe*)p;
}

void UpnpEventSubscribe_delete(UpnpEventSubscribe* q) {
    struct s_UpnpEventSubscribe* p = (struct s_UpnpEventSubscribe*)q;

    if (!p)
        return;

    UpnpString_delete(p->m_PublisherUrl);
    p->m_PublisherUrl = 0;
    UpnpString_delete(p->m_SID);
    p->m_SID = 0;
    p->m_TimeOut = 0;
    p->m_ErrCode = 0;

    free(p);
}

int UpnpEventSubscribe_assign(UpnpEventSubscribe* p,
                              const UpnpEventSubscribe* q) {
    int ok = 1;

    if (p != q) {
        ok = ok && UpnpEventSubscribe_set_ErrCode(
                       p, UpnpEventSubscribe_get_ErrCode(q));
        ok = ok && UpnpEventSubscribe_set_TimeOut(
                       p, UpnpEventSubscribe_get_TimeOut(q));
        ok = ok && UpnpEventSubscribe_set_SID(p, UpnpEventSubscribe_get_SID(q));
        ok = ok && UpnpEventSubscribe_set_PublisherUrl(
                       p, UpnpEventSubscribe_get_PublisherUrl(q));
    }

    return ok;
}

UpnpEventSubscribe* UpnpEventSubscribe_dup(const UpnpEventSubscribe* q) {
    UpnpEventSubscribe* p = UpnpEventSubscribe_new();

    if (!p)
        return 0;

    UpnpEventSubscribe_assign(p, q);

    return p;
}

int UpnpEventSubscribe_get_ErrCode(const UpnpEventSubscribe* p) {
    return p->m_ErrCode;
}

int UpnpEventSubscribe_set_ErrCode(UpnpEventSubscribe* p, int n) {
    p->m_ErrCode = n;

    return 1;
}

int UpnpEventSubscribe_get_TimeOut(const UpnpEventSubscribe* p) {
    return p->m_TimeOut;
}

int UpnpEventSubscribe_set_TimeOut(UpnpEventSubscribe* p, int n) {
    p->m_TimeOut = n;

    return 1;
}

const UpnpString* UpnpEventSubscribe_get_SID(const UpnpEventSubscribe* p) {
    return p->m_SID;
}

int UpnpEventSubscribe_set_SID(UpnpEventSubscribe* p, const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_SID, q);
}

size_t UpnpEventSubscribe_get_SID_Length(const UpnpEventSubscribe* p) {
    return UpnpString_get_Length(UpnpEventSubscribe_get_SID(p));
}

const char* UpnpEventSubscribe_get_SID_cstr(const UpnpEventSubscribe* p) {
    return UpnpString_get_String(UpnpEventSubscribe_get_SID(p));
}

int UpnpEventSubscribe_strcpy_SID(UpnpEventSubscribe* p, const char* s) {
    return UpnpString_set_String(p->m_SID, s);
}

int UpnpEventSubscribe_strncpy_SID(UpnpEventSubscribe* p, const char* s,
                                   size_t n) {
    return UpnpString_set_StringN(p->m_SID, s, n);
}

void UpnpEventSubscribe_clear_SID(UpnpEventSubscribe* p) {
    UpnpString_clear(p->m_SID);
}

const UpnpString*
UpnpEventSubscribe_get_PublisherUrl(const UpnpEventSubscribe* p) {
    return p->m_PublisherUrl;
}

int UpnpEventSubscribe_set_PublisherUrl(UpnpEventSubscribe* p,
                                        const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_PublisherUrl, q);
}

size_t UpnpEventSubscribe_get_PublisherUrl_Length(const UpnpEventSubscribe* p) {
    return UpnpString_get_Length(UpnpEventSubscribe_get_PublisherUrl(p));
}

const char*
UpnpEventSubscribe_get_PublisherUrl_cstr(const UpnpEventSubscribe* p) {
    return UpnpString_get_String(UpnpEventSubscribe_get_PublisherUrl(p));
}

int UpnpEventSubscribe_strcpy_PublisherUrl(UpnpEventSubscribe* p,
                                           const char* s) {
    return UpnpString_set_String(p->m_PublisherUrl, s);
}

int UpnpEventSubscribe_strncpy_PublisherUrl(UpnpEventSubscribe* p,
                                            const char* s, size_t n) {
    return UpnpString_set_StringN(p->m_PublisherUrl, s, n);
}

void UpnpEventSubscribe_clear_PublisherUrl(UpnpEventSubscribe* p) {
    UpnpString_clear(p->m_PublisherUrl);
}
