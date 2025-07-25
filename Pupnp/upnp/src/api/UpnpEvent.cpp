// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-07-19
// Also Copyright by other contributor.

/*!
 * \file
 *
 * \brief Source file for UpnpEvent methods.
 * \author Marcelo Roberto Jimenez
 */
#include "config.hpp"

#include <stdlib.h> /* for calloc(), free() */
#include <string.h> /* for strlen(), strdup(), memset() */

#include "UpnpEvent.hpp"

struct s_UpnpEvent {
    int m_EventKey;
    IXML_Document* m_ChangedVariables;
    UpnpString* m_SID;
};

UpnpEvent* UpnpEvent_new(void) {
    struct s_UpnpEvent* p = (s_UpnpEvent*)calloc(1, sizeof(struct s_UpnpEvent));

    if (!p)
        return 0;

    /*p->m_EventKey = 0;*/
    /*p->m_ChangedVariables = 0;*/
    p->m_SID = UpnpString_new();

    return (UpnpEvent*)p;
}

void UpnpEvent_delete(UpnpEvent* q) {
    struct s_UpnpEvent* p = (struct s_UpnpEvent*)q;

    if (!p)
        return;

    UpnpString_delete(p->m_SID);
    p->m_SID = 0;
    p->m_ChangedVariables = 0;
    p->m_EventKey = 0;

    free(p);
}

int UpnpEvent_assign(UpnpEvent* p, const UpnpEvent* q) {
    int ok = 1;

    if (p != q) {
        ok = ok && UpnpEvent_set_EventKey(p, UpnpEvent_get_EventKey(q));
        ok = ok && UpnpEvent_set_ChangedVariables(
                       p, UpnpEvent_get_ChangedVariables(q));
        ok = ok && UpnpEvent_set_SID(p, UpnpEvent_get_SID(q));
    }

    return ok;
}

UpnpEvent* UpnpEvent_dup(const UpnpEvent* q) {
    UpnpEvent* p = UpnpEvent_new();

    if (!p)
        return 0;

    UpnpEvent_assign(p, q);

    return p;
}

int UpnpEvent_get_EventKey(const UpnpEvent* p) { return p->m_EventKey; }

int UpnpEvent_set_EventKey(UpnpEvent* p, int n) {
    p->m_EventKey = n;

    return 1;
}

IXML_Document* UpnpEvent_get_ChangedVariables(const UpnpEvent* p) {
    return p->m_ChangedVariables;
}

int UpnpEvent_set_ChangedVariables(UpnpEvent* p, IXML_Document* n) {
    p->m_ChangedVariables = n;

    return 1;
}

const UpnpString* UpnpEvent_get_SID(const UpnpEvent* p) { return p->m_SID; }

int UpnpEvent_set_SID(UpnpEvent* p, const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_SID, q);
}

size_t UpnpEvent_get_SID_Length(const UpnpEvent* p) {
    return UpnpString_get_Length(UpnpEvent_get_SID(p));
}

const char* UpnpEvent_get_SID_cstr(const UpnpEvent* p) {
    return UpnpString_get_String(UpnpEvent_get_SID(p));
}

int UpnpEvent_strcpy_SID(UpnpEvent* p, const char* s) {
    return UpnpString_set_String(p->m_SID, s);
}

int UpnpEvent_strncpy_SID(UpnpEvent* p, const char* s, size_t n) {
    return UpnpString_set_StringN(p->m_SID, s, n);
}

void UpnpEvent_clear_SID(UpnpEvent* p) { UpnpString_clear(p->m_SID); }
