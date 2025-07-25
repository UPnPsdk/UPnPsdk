// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-07-19

/*!
 * \file
 *
 * \brief Source file for UpnpActionComplete methods.
 * \author Marcelo Roberto Jimenez
 */
#include "config.hpp"

#include <stdlib.h> /* for calloc(), free() */
#include <string.h> /* for strlen(), strdup(), memset() */

#include "UpnpActionComplete.hpp"

struct s_UpnpActionComplete {
    int m_ErrCode;
    UpnpString* m_CtrlUrl;
    IXML_Document* m_ActionRequest;
    IXML_Document* m_ActionResult;
};

UpnpActionComplete* UpnpActionComplete_new(void) {
    struct s_UpnpActionComplete* p =
        (s_UpnpActionComplete*)calloc(1, sizeof(struct s_UpnpActionComplete));

    if (!p)
        return 0;

    /*p->m_ErrCode = 0;*/
    p->m_CtrlUrl = UpnpString_new();
    /*p->m_ActionRequest = 0;*/
    /*p->m_ActionResult = 0;*/

    return (UpnpActionComplete*)p;
}

void UpnpActionComplete_delete(UpnpActionComplete* q) {
    struct s_UpnpActionComplete* p = (struct s_UpnpActionComplete*)q;

    if (!p)
        return;

    p->m_ActionResult = 0;
    p->m_ActionRequest = 0;
    UpnpString_delete(p->m_CtrlUrl);
    p->m_CtrlUrl = 0;
    p->m_ErrCode = 0;

    free(p);
}

int UpnpActionComplete_assign(UpnpActionComplete* p,
                              const UpnpActionComplete* q) {
    int ok = 1;

    if (p != q) {
        ok = ok && UpnpActionComplete_set_ErrCode(
                       p, UpnpActionComplete_get_ErrCode(q));
        ok = ok && UpnpActionComplete_set_CtrlUrl(
                       p, UpnpActionComplete_get_CtrlUrl(q));
        ok = ok && UpnpActionComplete_set_ActionRequest(
                       p, UpnpActionComplete_get_ActionRequest(q));
        ok = ok && UpnpActionComplete_set_ActionResult(
                       p, UpnpActionComplete_get_ActionResult(q));
    }

    return ok;
}

UpnpActionComplete* UpnpActionComplete_dup(const UpnpActionComplete* q) {
    UpnpActionComplete* p = UpnpActionComplete_new();

    if (!p)
        return 0;

    UpnpActionComplete_assign(p, q);

    return p;
}

int UpnpActionComplete_get_ErrCode(const UpnpActionComplete* p) {
    return p->m_ErrCode;
}

int UpnpActionComplete_set_ErrCode(UpnpActionComplete* p, int n) {
    p->m_ErrCode = n;

    return 1;
}

const UpnpString* UpnpActionComplete_get_CtrlUrl(const UpnpActionComplete* p) {
    return p->m_CtrlUrl;
}

int UpnpActionComplete_set_CtrlUrl(UpnpActionComplete* p, const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_CtrlUrl, q);
}

size_t UpnpActionComplete_get_CtrlUrl_Length(const UpnpActionComplete* p) {
    return UpnpString_get_Length(UpnpActionComplete_get_CtrlUrl(p));
}

const char* UpnpActionComplete_get_CtrlUrl_cstr(const UpnpActionComplete* p) {
    return UpnpString_get_String(UpnpActionComplete_get_CtrlUrl(p));
}

int UpnpActionComplete_strcpy_CtrlUrl(UpnpActionComplete* p, const char* s) {
    return UpnpString_set_String(p->m_CtrlUrl, s);
}

int UpnpActionComplete_strncpy_CtrlUrl(UpnpActionComplete* p, const char* s,
                                       size_t n) {
    return UpnpString_set_StringN(p->m_CtrlUrl, s, n);
}

void UpnpActionComplete_clear_CtrlUrl(UpnpActionComplete* p) {
    UpnpString_clear(p->m_CtrlUrl);
}

IXML_Document*
UpnpActionComplete_get_ActionRequest(const UpnpActionComplete* p) {
    return p->m_ActionRequest;
}

int UpnpActionComplete_set_ActionRequest(UpnpActionComplete* p,
                                         IXML_Document* n) {
    p->m_ActionRequest = n;

    return 1;
}

IXML_Document*
UpnpActionComplete_get_ActionResult(const UpnpActionComplete* p) {
    return p->m_ActionResult;
}

int UpnpActionComplete_set_ActionResult(UpnpActionComplete* p,
                                        IXML_Document* n) {
    p->m_ActionResult = n;

    return 1;
}
