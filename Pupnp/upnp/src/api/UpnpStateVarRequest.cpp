// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-07-19

/*!
 * \file
 *
 * \brief Source file for UpnpStateVarRequest methods.
 * \author Marcelo Roberto Jimenez
 */
#include "config.hpp"

#include <stdlib.h> /* for calloc(), free() */
#include <string.h> /* for strlen(), strdup(), memset() */

#include "UpnpStateVarRequest.hpp"

struct s_UpnpStateVarRequest {
    int m_ErrCode;
    int m_Socket;
    UpnpString* m_ErrStr;
    UpnpString* m_DevUDN;
    UpnpString* m_ServiceID;
    UpnpString* m_StateVarName;
    struct sockaddr_storage m_CtrlPtIPAddr;
    DOMString m_CurrentVal;
};

UpnpStateVarRequest* UpnpStateVarRequest_new(void) {
    struct s_UpnpStateVarRequest* p =
        (s_UpnpStateVarRequest*)calloc(1, sizeof(struct s_UpnpStateVarRequest));

    if (!p)
        return 0;

    /*p->m_ErrCode = 0;*/
    /*p->m_Socket = 0;*/
    p->m_ErrStr = UpnpString_new();
    p->m_DevUDN = UpnpString_new();
    p->m_ServiceID = UpnpString_new();
    p->m_StateVarName = UpnpString_new();
    /* memset(&p->m_CtrlPtIPAddr, 0, sizeof (struct sockaddr_storage)); */
    /*p->m_CurrentVal = 0;*/

    return (UpnpStateVarRequest*)p;
}

void UpnpStateVarRequest_delete(UpnpStateVarRequest* q) {
    struct s_UpnpStateVarRequest* p = (struct s_UpnpStateVarRequest*)q;

    if (!p)
        return;

    ixmlFreeDOMString(p->m_CurrentVal);
    p->m_CurrentVal = 0;
    memset(&p->m_CtrlPtIPAddr, 0, sizeof(struct sockaddr_storage));
    UpnpString_delete(p->m_StateVarName);
    p->m_StateVarName = 0;
    UpnpString_delete(p->m_ServiceID);
    p->m_ServiceID = 0;
    UpnpString_delete(p->m_DevUDN);
    p->m_DevUDN = 0;
    UpnpString_delete(p->m_ErrStr);
    p->m_ErrStr = 0;
    p->m_Socket = 0;
    p->m_ErrCode = 0;

    free(p);
}

int UpnpStateVarRequest_assign(UpnpStateVarRequest* p,
                               const UpnpStateVarRequest* q) {
    int ok = 1;

    if (p != q) {
        ok = ok && UpnpStateVarRequest_set_ErrCode(
                       p, UpnpStateVarRequest_get_ErrCode(q));
        ok = ok && UpnpStateVarRequest_set_Socket(
                       p, UpnpStateVarRequest_get_Socket(q));
        ok = ok && UpnpStateVarRequest_set_ErrStr(
                       p, UpnpStateVarRequest_get_ErrStr(q));
        ok = ok && UpnpStateVarRequest_set_DevUDN(
                       p, UpnpStateVarRequest_get_DevUDN(q));
        ok = ok && UpnpStateVarRequest_set_ServiceID(
                       p, UpnpStateVarRequest_get_ServiceID(q));
        ok = ok && UpnpStateVarRequest_set_StateVarName(
                       p, UpnpStateVarRequest_get_StateVarName(q));
        ok = ok && UpnpStateVarRequest_set_CtrlPtIPAddr(
                       p, UpnpStateVarRequest_get_CtrlPtIPAddr(q));
        ok = ok && UpnpStateVarRequest_set_CurrentVal(
                       p, UpnpStateVarRequest_get_CurrentVal(q));
    }

    return ok;
}

UpnpStateVarRequest* UpnpStateVarRequest_dup(const UpnpStateVarRequest* q) {
    UpnpStateVarRequest* p = UpnpStateVarRequest_new();

    if (!p)
        return 0;

    UpnpStateVarRequest_assign(p, q);

    return p;
}

int UpnpStateVarRequest_get_ErrCode(const UpnpStateVarRequest* p) {
    return p->m_ErrCode;
}

int UpnpStateVarRequest_set_ErrCode(UpnpStateVarRequest* p, int n) {
    p->m_ErrCode = n;

    return 1;
}

int UpnpStateVarRequest_get_Socket(const UpnpStateVarRequest* p) {
    return p->m_Socket;
}

int UpnpStateVarRequest_set_Socket(UpnpStateVarRequest* p, int n) {
    p->m_Socket = n;

    return 1;
}

const UpnpString* UpnpStateVarRequest_get_ErrStr(const UpnpStateVarRequest* p) {
    return p->m_ErrStr;
}

int UpnpStateVarRequest_set_ErrStr(UpnpStateVarRequest* p,
                                   const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_ErrStr, q);
}

size_t UpnpStateVarRequest_get_ErrStr_Length(const UpnpStateVarRequest* p) {
    return UpnpString_get_Length(UpnpStateVarRequest_get_ErrStr(p));
}

const char* UpnpStateVarRequest_get_ErrStr_cstr(const UpnpStateVarRequest* p) {
    return UpnpString_get_String(UpnpStateVarRequest_get_ErrStr(p));
}

int UpnpStateVarRequest_strcpy_ErrStr(UpnpStateVarRequest* p, const char* s) {
    return UpnpString_set_String(p->m_ErrStr, s);
}

int UpnpStateVarRequest_strncpy_ErrStr(UpnpStateVarRequest* p, const char* s,
                                       size_t n) {
    return UpnpString_set_StringN(p->m_ErrStr, s, n);
}

void UpnpStateVarRequest_clear_ErrStr(UpnpStateVarRequest* p) {
    UpnpString_clear(p->m_ErrStr);
}

const UpnpString* UpnpStateVarRequest_get_DevUDN(const UpnpStateVarRequest* p) {
    return p->m_DevUDN;
}

int UpnpStateVarRequest_set_DevUDN(UpnpStateVarRequest* p,
                                   const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_DevUDN, q);
}

size_t UpnpStateVarRequest_get_DevUDN_Length(const UpnpStateVarRequest* p) {
    return UpnpString_get_Length(UpnpStateVarRequest_get_DevUDN(p));
}

const char* UpnpStateVarRequest_get_DevUDN_cstr(const UpnpStateVarRequest* p) {
    return UpnpString_get_String(UpnpStateVarRequest_get_DevUDN(p));
}

int UpnpStateVarRequest_strcpy_DevUDN(UpnpStateVarRequest* p, const char* s) {
    return UpnpString_set_String(p->m_DevUDN, s);
}

int UpnpStateVarRequest_strncpy_DevUDN(UpnpStateVarRequest* p, const char* s,
                                       size_t n) {
    return UpnpString_set_StringN(p->m_DevUDN, s, n);
}

void UpnpStateVarRequest_clear_DevUDN(UpnpStateVarRequest* p) {
    UpnpString_clear(p->m_DevUDN);
}

const UpnpString*
UpnpStateVarRequest_get_ServiceID(const UpnpStateVarRequest* p) {
    return p->m_ServiceID;
}

int UpnpStateVarRequest_set_ServiceID(UpnpStateVarRequest* p,
                                      const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_ServiceID, q);
}

size_t UpnpStateVarRequest_get_ServiceID_Length(const UpnpStateVarRequest* p) {
    return UpnpString_get_Length(UpnpStateVarRequest_get_ServiceID(p));
}

const char*
UpnpStateVarRequest_get_ServiceID_cstr(const UpnpStateVarRequest* p) {
    return UpnpString_get_String(UpnpStateVarRequest_get_ServiceID(p));
}

int UpnpStateVarRequest_strcpy_ServiceID(UpnpStateVarRequest* p,
                                         const char* s) {
    return UpnpString_set_String(p->m_ServiceID, s);
}

int UpnpStateVarRequest_strncpy_ServiceID(UpnpStateVarRequest* p, const char* s,
                                          size_t n) {
    return UpnpString_set_StringN(p->m_ServiceID, s, n);
}

void UpnpStateVarRequest_clear_ServiceID(UpnpStateVarRequest* p) {
    UpnpString_clear(p->m_ServiceID);
}

const UpnpString*
UpnpStateVarRequest_get_StateVarName(const UpnpStateVarRequest* p) {
    return p->m_StateVarName;
}

int UpnpStateVarRequest_set_StateVarName(UpnpStateVarRequest* p,
                                         const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_StateVarName, q);
}

size_t
UpnpStateVarRequest_get_StateVarName_Length(const UpnpStateVarRequest* p) {
    return UpnpString_get_Length(UpnpStateVarRequest_get_StateVarName(p));
}

const char*
UpnpStateVarRequest_get_StateVarName_cstr(const UpnpStateVarRequest* p) {
    return UpnpString_get_String(UpnpStateVarRequest_get_StateVarName(p));
}

int UpnpStateVarRequest_strcpy_StateVarName(UpnpStateVarRequest* p,
                                            const char* s) {
    return UpnpString_set_String(p->m_StateVarName, s);
}

int UpnpStateVarRequest_strncpy_StateVarName(UpnpStateVarRequest* p,
                                             const char* s, size_t n) {
    return UpnpString_set_StringN(p->m_StateVarName, s, n);
}

void UpnpStateVarRequest_clear_StateVarName(UpnpStateVarRequest* p) {
    UpnpString_clear(p->m_StateVarName);
}

const struct sockaddr_storage*
UpnpStateVarRequest_get_CtrlPtIPAddr(const UpnpStateVarRequest* p) {
    return &p->m_CtrlPtIPAddr;
}

int UpnpStateVarRequest_set_CtrlPtIPAddr(UpnpStateVarRequest* p,
                                         const struct sockaddr_storage* buf) {
    p->m_CtrlPtIPAddr = *buf;

    return 1;
}

void UpnpStateVarRequest_clear_CtrlPtIPAddr(UpnpStateVarRequest* p) {
    memset(&p->m_CtrlPtIPAddr, 0, sizeof(struct sockaddr_storage));
}

const DOMString
UpnpStateVarRequest_get_CurrentVal(const UpnpStateVarRequest* p) {
    return p->m_CurrentVal;
}

int UpnpStateVarRequest_set_CurrentVal(UpnpStateVarRequest* p,
                                       const DOMString s) {
    DOMString q = ixmlCloneDOMString(s);
    if (!q)
        return 0;
    ixmlFreeDOMString(p->m_CurrentVal);
    p->m_CurrentVal = q;

    return 1;
}

const char*
UpnpStateVarRequest_get_CurrentVal_cstr(const UpnpStateVarRequest* p) {
    return (const char*)UpnpStateVarRequest_get_CurrentVal(p);
}
