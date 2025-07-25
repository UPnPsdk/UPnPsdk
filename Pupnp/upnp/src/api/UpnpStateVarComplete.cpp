// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-07-19

/*!
 * \file
 *
 * \brief Source file for UpnpStateVarComplete methods.
 * \author Marcelo Roberto Jimenez
 */
#include "config.hpp"

#include <stdlib.h> /* for calloc(), free() */
#include <string.h> /* for strlen(), strdup(), memset() */

#include "UpnpStateVarComplete.hpp"

struct s_UpnpStateVarComplete {
    int m_ErrCode;
    UpnpString* m_CtrlUrl;
    UpnpString* m_StateVarName;
    DOMString m_CurrentVal;
};

UpnpStateVarComplete* UpnpStateVarComplete_new(void) {
    struct s_UpnpStateVarComplete* p = (s_UpnpStateVarComplete*)calloc(
        1, sizeof(struct s_UpnpStateVarComplete));

    if (!p)
        return 0;

    /*p->m_ErrCode = 0;*/
    p->m_CtrlUrl = UpnpString_new();
    p->m_StateVarName = UpnpString_new();
    /*p->m_CurrentVal = 0;*/

    return (UpnpStateVarComplete*)p;
}

void UpnpStateVarComplete_delete(UpnpStateVarComplete* q) {
    struct s_UpnpStateVarComplete* p = (struct s_UpnpStateVarComplete*)q;

    if (!p)
        return;

    ixmlFreeDOMString(p->m_CurrentVal);
    p->m_CurrentVal = 0;
    UpnpString_delete(p->m_StateVarName);
    p->m_StateVarName = 0;
    UpnpString_delete(p->m_CtrlUrl);
    p->m_CtrlUrl = 0;
    p->m_ErrCode = 0;

    free(p);
}

int UpnpStateVarComplete_assign(UpnpStateVarComplete* p,
                                const UpnpStateVarComplete* q) {
    int ok = 1;

    if (p != q) {
        ok = ok && UpnpStateVarComplete_set_ErrCode(
                       p, UpnpStateVarComplete_get_ErrCode(q));
        ok = ok && UpnpStateVarComplete_set_CtrlUrl(
                       p, UpnpStateVarComplete_get_CtrlUrl(q));
        ok = ok && UpnpStateVarComplete_set_StateVarName(
                       p, UpnpStateVarComplete_get_StateVarName(q));
        ok = ok && UpnpStateVarComplete_set_CurrentVal(
                       p, UpnpStateVarComplete_get_CurrentVal(q));
    }

    return ok;
}

UpnpStateVarComplete* UpnpStateVarComplete_dup(const UpnpStateVarComplete* q) {
    UpnpStateVarComplete* p = UpnpStateVarComplete_new();

    if (!p)
        return 0;

    UpnpStateVarComplete_assign(p, q);

    return p;
}

int UpnpStateVarComplete_get_ErrCode(const UpnpStateVarComplete* p) {
    return p->m_ErrCode;
}

int UpnpStateVarComplete_set_ErrCode(UpnpStateVarComplete* p, int n) {
    p->m_ErrCode = n;

    return 1;
}

const UpnpString*
UpnpStateVarComplete_get_CtrlUrl(const UpnpStateVarComplete* p) {
    return p->m_CtrlUrl;
}

int UpnpStateVarComplete_set_CtrlUrl(UpnpStateVarComplete* p,
                                     const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_CtrlUrl, q);
}

size_t UpnpStateVarComplete_get_CtrlUrl_Length(const UpnpStateVarComplete* p) {
    return UpnpString_get_Length(UpnpStateVarComplete_get_CtrlUrl(p));
}

const char*
UpnpStateVarComplete_get_CtrlUrl_cstr(const UpnpStateVarComplete* p) {
    return UpnpString_get_String(UpnpStateVarComplete_get_CtrlUrl(p));
}

int UpnpStateVarComplete_strcpy_CtrlUrl(UpnpStateVarComplete* p,
                                        const char* s) {
    return UpnpString_set_String(p->m_CtrlUrl, s);
}

int UpnpStateVarComplete_strncpy_CtrlUrl(UpnpStateVarComplete* p, const char* s,
                                         size_t n) {
    return UpnpString_set_StringN(p->m_CtrlUrl, s, n);
}

void UpnpStateVarComplete_clear_CtrlUrl(UpnpStateVarComplete* p) {
    UpnpString_clear(p->m_CtrlUrl);
}

const UpnpString*
UpnpStateVarComplete_get_StateVarName(const UpnpStateVarComplete* p) {
    return p->m_StateVarName;
}

int UpnpStateVarComplete_set_StateVarName(UpnpStateVarComplete* p,
                                          const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_StateVarName, q);
}

size_t
UpnpStateVarComplete_get_StateVarName_Length(const UpnpStateVarComplete* p) {
    return UpnpString_get_Length(UpnpStateVarComplete_get_StateVarName(p));
}

const char*
UpnpStateVarComplete_get_StateVarName_cstr(const UpnpStateVarComplete* p) {
    return UpnpString_get_String(UpnpStateVarComplete_get_StateVarName(p));
}

int UpnpStateVarComplete_strcpy_StateVarName(UpnpStateVarComplete* p,
                                             const char* s) {
    return UpnpString_set_String(p->m_StateVarName, s);
}

int UpnpStateVarComplete_strncpy_StateVarName(UpnpStateVarComplete* p,
                                              const char* s, size_t n) {
    return UpnpString_set_StringN(p->m_StateVarName, s, n);
}

void UpnpStateVarComplete_clear_StateVarName(UpnpStateVarComplete* p) {
    UpnpString_clear(p->m_StateVarName);
}

const DOMString
UpnpStateVarComplete_get_CurrentVal(const UpnpStateVarComplete* p) {
    return p->m_CurrentVal;
}

int UpnpStateVarComplete_set_CurrentVal(UpnpStateVarComplete* p,
                                        const DOMString s) {
    DOMString q = ixmlCloneDOMString(s);
    if (!q)
        return 0;
    ixmlFreeDOMString(p->m_CurrentVal);
    p->m_CurrentVal = q;

    return 1;
}

const char*
UpnpStateVarComplete_get_CurrentVal_cstr(const UpnpStateVarComplete* p) {
    return (const char*)UpnpStateVarComplete_get_CurrentVal(p);
}
