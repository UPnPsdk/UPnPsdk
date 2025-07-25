// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-07-19

/*!
 * \file
 *
 * \brief Source file for UpnpActionRequest methods.
 * \author Marcelo Roberto Jimenez
 */
#include "config.hpp"

#include <stdlib.h> /* for calloc(), free() */
#include <string.h> /* for strlen(), strdup(), memset() */

#include "UpnpActionRequest.hpp"

struct s_UpnpActionRequest {
    int m_ErrCode;
    int m_Socket;
    UpnpString* m_ErrStr;
    UpnpString* m_ActionName;
    UpnpString* m_DevUDN;
    UpnpString* m_ServiceID;
    IXML_Document* m_ActionRequest;
    IXML_Document* m_ActionResult;
    IXML_Document* m_SoapHeader;
    struct sockaddr_storage m_CtrlPtIPAddr;
    UpnpString* m_Os;
};

UpnpActionRequest* UpnpActionRequest_new(void) {
    struct s_UpnpActionRequest* p =
        (s_UpnpActionRequest*)calloc(1, sizeof(struct s_UpnpActionRequest));

    if (!p)
        return 0;

    /*p->m_ErrCode = 0;*/
    /*p->m_Socket = 0;*/
    p->m_ErrStr = UpnpString_new();
    p->m_ActionName = UpnpString_new();
    p->m_DevUDN = UpnpString_new();
    p->m_ServiceID = UpnpString_new();
    /*p->m_ActionRequest = 0;*/
    /*p->m_ActionResult = 0;*/
    /*p->m_SoapHeader = 0;*/
    /* memset(&p->m_CtrlPtIPAddr, 0, sizeof (struct sockaddr_storage)); */
    p->m_Os = UpnpString_new();

    return (UpnpActionRequest*)p;
}

void UpnpActionRequest_delete(UpnpActionRequest* q) {
    struct s_UpnpActionRequest* p = (struct s_UpnpActionRequest*)q;

    if (!p)
        return;

    UpnpString_delete(p->m_Os);
    p->m_Os = 0;
    memset(&p->m_CtrlPtIPAddr, 0, sizeof(struct sockaddr_storage));
    p->m_SoapHeader = 0;
    p->m_ActionResult = 0;
    p->m_ActionRequest = 0;
    UpnpString_delete(p->m_ServiceID);
    p->m_ServiceID = 0;
    UpnpString_delete(p->m_DevUDN);
    p->m_DevUDN = 0;
    UpnpString_delete(p->m_ActionName);
    p->m_ActionName = 0;
    UpnpString_delete(p->m_ErrStr);
    p->m_ErrStr = 0;
    p->m_Socket = 0;
    p->m_ErrCode = 0;

    free(p);
}

int UpnpActionRequest_assign(UpnpActionRequest* p, const UpnpActionRequest* q) {
    int ok = 1;

    if (p != q) {
        ok = ok &&
             UpnpActionRequest_set_ErrCode(p, UpnpActionRequest_get_ErrCode(q));
        ok = ok &&
             UpnpActionRequest_set_Socket(p, UpnpActionRequest_get_Socket(q));
        ok = ok &&
             UpnpActionRequest_set_ErrStr(p, UpnpActionRequest_get_ErrStr(q));
        ok = ok && UpnpActionRequest_set_ActionName(
                       p, UpnpActionRequest_get_ActionName(q));
        ok = ok &&
             UpnpActionRequest_set_DevUDN(p, UpnpActionRequest_get_DevUDN(q));
        ok = ok && UpnpActionRequest_set_ServiceID(
                       p, UpnpActionRequest_get_ServiceID(q));
        ok = ok && UpnpActionRequest_set_ActionRequest(
                       p, UpnpActionRequest_get_ActionRequest(q));
        ok = ok && UpnpActionRequest_set_ActionResult(
                       p, UpnpActionRequest_get_ActionResult(q));
        ok = ok && UpnpActionRequest_set_SoapHeader(
                       p, UpnpActionRequest_get_SoapHeader(q));
        ok = ok && UpnpActionRequest_set_CtrlPtIPAddr(
                       p, UpnpActionRequest_get_CtrlPtIPAddr(q));
        ok = ok && UpnpActionRequest_set_Os(p, UpnpActionRequest_get_Os(q));
    }

    return ok;
}

UpnpActionRequest* UpnpActionRequest_dup(const UpnpActionRequest* q) {
    UpnpActionRequest* p = UpnpActionRequest_new();

    if (!p)
        return 0;

    UpnpActionRequest_assign(p, q);

    return p;
}

int UpnpActionRequest_get_ErrCode(const UpnpActionRequest* p) {
    return p->m_ErrCode;
}

int UpnpActionRequest_set_ErrCode(UpnpActionRequest* p, int n) {
    p->m_ErrCode = n;

    return 1;
}

int UpnpActionRequest_get_Socket(const UpnpActionRequest* p) {
    return p->m_Socket;
}

int UpnpActionRequest_set_Socket(UpnpActionRequest* p, int n) {
    p->m_Socket = n;

    return 1;
}

const UpnpString* UpnpActionRequest_get_ErrStr(const UpnpActionRequest* p) {
    return p->m_ErrStr;
}

int UpnpActionRequest_set_ErrStr(UpnpActionRequest* p, const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_ErrStr, q);
}

size_t UpnpActionRequest_get_ErrStr_Length(const UpnpActionRequest* p) {
    return UpnpString_get_Length(UpnpActionRequest_get_ErrStr(p));
}

const char* UpnpActionRequest_get_ErrStr_cstr(const UpnpActionRequest* p) {
    return UpnpString_get_String(UpnpActionRequest_get_ErrStr(p));
}

int UpnpActionRequest_strcpy_ErrStr(UpnpActionRequest* p, const char* s) {
    return UpnpString_set_String(p->m_ErrStr, s);
}

int UpnpActionRequest_strncpy_ErrStr(UpnpActionRequest* p, const char* s,
                                     size_t n) {
    return UpnpString_set_StringN(p->m_ErrStr, s, n);
}

void UpnpActionRequest_clear_ErrStr(UpnpActionRequest* p) {
    UpnpString_clear(p->m_ErrStr);
}

const UpnpString* UpnpActionRequest_get_ActionName(const UpnpActionRequest* p) {
    return p->m_ActionName;
}

int UpnpActionRequest_set_ActionName(UpnpActionRequest* p,
                                     const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_ActionName, q);
}

size_t UpnpActionRequest_get_ActionName_Length(const UpnpActionRequest* p) {
    return UpnpString_get_Length(UpnpActionRequest_get_ActionName(p));
}

const char* UpnpActionRequest_get_ActionName_cstr(const UpnpActionRequest* p) {
    return UpnpString_get_String(UpnpActionRequest_get_ActionName(p));
}

int UpnpActionRequest_strcpy_ActionName(UpnpActionRequest* p, const char* s) {
    return UpnpString_set_String(p->m_ActionName, s);
}

int UpnpActionRequest_strncpy_ActionName(UpnpActionRequest* p, const char* s,
                                         size_t n) {
    return UpnpString_set_StringN(p->m_ActionName, s, n);
}

void UpnpActionRequest_clear_ActionName(UpnpActionRequest* p) {
    UpnpString_clear(p->m_ActionName);
}

const UpnpString* UpnpActionRequest_get_DevUDN(const UpnpActionRequest* p) {
    return p->m_DevUDN;
}

int UpnpActionRequest_set_DevUDN(UpnpActionRequest* p, const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_DevUDN, q);
}

size_t UpnpActionRequest_get_DevUDN_Length(const UpnpActionRequest* p) {
    return UpnpString_get_Length(UpnpActionRequest_get_DevUDN(p));
}

const char* UpnpActionRequest_get_DevUDN_cstr(const UpnpActionRequest* p) {
    return UpnpString_get_String(UpnpActionRequest_get_DevUDN(p));
}

int UpnpActionRequest_strcpy_DevUDN(UpnpActionRequest* p, const char* s) {
    return UpnpString_set_String(p->m_DevUDN, s);
}

int UpnpActionRequest_strncpy_DevUDN(UpnpActionRequest* p, const char* s,
                                     size_t n) {
    return UpnpString_set_StringN(p->m_DevUDN, s, n);
}

void UpnpActionRequest_clear_DevUDN(UpnpActionRequest* p) {
    UpnpString_clear(p->m_DevUDN);
}

const UpnpString* UpnpActionRequest_get_ServiceID(const UpnpActionRequest* p) {
    return p->m_ServiceID;
}

int UpnpActionRequest_set_ServiceID(UpnpActionRequest* p, const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_ServiceID, q);
}

size_t UpnpActionRequest_get_ServiceID_Length(const UpnpActionRequest* p) {
    return UpnpString_get_Length(UpnpActionRequest_get_ServiceID(p));
}

const char* UpnpActionRequest_get_ServiceID_cstr(const UpnpActionRequest* p) {
    return UpnpString_get_String(UpnpActionRequest_get_ServiceID(p));
}

int UpnpActionRequest_strcpy_ServiceID(UpnpActionRequest* p, const char* s) {
    return UpnpString_set_String(p->m_ServiceID, s);
}

int UpnpActionRequest_strncpy_ServiceID(UpnpActionRequest* p, const char* s,
                                        size_t n) {
    return UpnpString_set_StringN(p->m_ServiceID, s, n);
}

void UpnpActionRequest_clear_ServiceID(UpnpActionRequest* p) {
    UpnpString_clear(p->m_ServiceID);
}

IXML_Document* UpnpActionRequest_get_ActionRequest(const UpnpActionRequest* p) {
    return p->m_ActionRequest;
}

int UpnpActionRequest_set_ActionRequest(UpnpActionRequest* p,
                                        IXML_Document* n) {
    p->m_ActionRequest = n;

    return 1;
}

IXML_Document* UpnpActionRequest_get_ActionResult(const UpnpActionRequest* p) {
    return p->m_ActionResult;
}

int UpnpActionRequest_set_ActionResult(UpnpActionRequest* p, IXML_Document* n) {
    p->m_ActionResult = n;

    return 1;
}

IXML_Document* UpnpActionRequest_get_SoapHeader(const UpnpActionRequest* p) {
    return p->m_SoapHeader;
}

int UpnpActionRequest_set_SoapHeader(UpnpActionRequest* p, IXML_Document* n) {
    p->m_SoapHeader = n;

    return 1;
}

const struct sockaddr_storage*
UpnpActionRequest_get_CtrlPtIPAddr(const UpnpActionRequest* p) {
    return &p->m_CtrlPtIPAddr;
}

int UpnpActionRequest_set_CtrlPtIPAddr(UpnpActionRequest* p,
                                       const struct sockaddr_storage* buf) {
    p->m_CtrlPtIPAddr = *buf;

    return 1;
}

void UpnpActionRequest_clear_CtrlPtIPAddr(UpnpActionRequest* p) {
    memset(&p->m_CtrlPtIPAddr, 0, sizeof(struct sockaddr_storage));
}

const UpnpString* UpnpActionRequest_get_Os(const UpnpActionRequest* p) {
    return p->m_Os;
}

int UpnpActionRequest_set_Os(UpnpActionRequest* p, const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_Os, q);
}

size_t UpnpActionRequest_get_Os_Length(const UpnpActionRequest* p) {
    return UpnpString_get_Length(UpnpActionRequest_get_Os(p));
}

const char* UpnpActionRequest_get_Os_cstr(const UpnpActionRequest* p) {
    return UpnpString_get_String(UpnpActionRequest_get_Os(p));
}

int UpnpActionRequest_strcpy_Os(UpnpActionRequest* p, const char* s) {
    return UpnpString_set_String(p->m_Os, s);
}

int UpnpActionRequest_strncpy_Os(UpnpActionRequest* p, const char* s,
                                 size_t n) {
    return UpnpString_set_StringN(p->m_Os, s, n);
}

void UpnpActionRequest_clear_Os(UpnpActionRequest* p) {
    UpnpString_clear(p->m_Os);
}
