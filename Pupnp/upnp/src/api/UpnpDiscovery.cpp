// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-07-19

/*!
 * \file
 *
 * \brief Source file for UpnpDiscovery methods.
 *
 * \author Marcelo Roberto Jimenez
 */
#include "config.hpp"

#include <stdlib.h> /* for calloc(), free() */
#include <string.h> /* for strlen(), strdup(), memset() */

#include "UpnpDiscovery.hpp"

struct s_UpnpDiscovery {
    int m_ErrCode;
    int m_Expires;
    UpnpString* m_DeviceID;
    UpnpString* m_DeviceType;
    UpnpString* m_ServiceType;
    UpnpString* m_ServiceVer;
    UpnpString* m_Location;
    UpnpString* m_Os;
    UpnpString* m_Date;
    UpnpString* m_Ext;
    struct sockaddr_storage m_DestAddr;
};

UpnpDiscovery* UpnpDiscovery_new(void) {
    struct s_UpnpDiscovery* p =
        (s_UpnpDiscovery*)calloc(1, sizeof(struct s_UpnpDiscovery));

    if (!p)
        return 0;

    /*p->m_ErrCode = 0;*/
    /*p->m_Expires = 0;*/
    p->m_DeviceID = UpnpString_new();
    p->m_DeviceType = UpnpString_new();
    p->m_ServiceType = UpnpString_new();
    p->m_ServiceVer = UpnpString_new();
    p->m_Location = UpnpString_new();
    p->m_Os = UpnpString_new();
    p->m_Date = UpnpString_new();
    p->m_Ext = UpnpString_new();
    /* memset(&p->m_DestAddr, 0, sizeof (struct sockaddr_storage)); */

    return (UpnpDiscovery*)p;
}

void UpnpDiscovery_delete(UpnpDiscovery* q) {
    struct s_UpnpDiscovery* p = (struct s_UpnpDiscovery*)q;

    if (!p)
        return;

    memset(&p->m_DestAddr, 0, sizeof(struct sockaddr_storage));
    UpnpString_delete(p->m_Ext);
    p->m_Ext = 0;
    UpnpString_delete(p->m_Date);
    p->m_Date = 0;
    UpnpString_delete(p->m_Os);
    p->m_Os = 0;
    UpnpString_delete(p->m_Location);
    p->m_Location = 0;
    UpnpString_delete(p->m_ServiceVer);
    p->m_ServiceVer = 0;
    UpnpString_delete(p->m_ServiceType);
    p->m_ServiceType = 0;
    UpnpString_delete(p->m_DeviceType);
    p->m_DeviceType = 0;
    UpnpString_delete(p->m_DeviceID);
    p->m_DeviceID = 0;
    p->m_Expires = 0;
    p->m_ErrCode = 0;

    free(p);
}

int UpnpDiscovery_assign(UpnpDiscovery* p, const UpnpDiscovery* q) {
    int ok = 1;

    if (p != q) {
        ok = ok && UpnpDiscovery_set_ErrCode(p, UpnpDiscovery_get_ErrCode(q));
        ok = ok && UpnpDiscovery_set_Expires(p, UpnpDiscovery_get_Expires(q));
        ok = ok && UpnpDiscovery_set_DeviceID(p, UpnpDiscovery_get_DeviceID(q));
        ok = ok &&
             UpnpDiscovery_set_DeviceType(p, UpnpDiscovery_get_DeviceType(q));
        ok = ok &&
             UpnpDiscovery_set_ServiceType(p, UpnpDiscovery_get_ServiceType(q));
        ok = ok &&
             UpnpDiscovery_set_ServiceVer(p, UpnpDiscovery_get_ServiceVer(q));
        ok = ok && UpnpDiscovery_set_Location(p, UpnpDiscovery_get_Location(q));
        ok = ok && UpnpDiscovery_set_Os(p, UpnpDiscovery_get_Os(q));
        ok = ok && UpnpDiscovery_set_Date(p, UpnpDiscovery_get_Date(q));
        ok = ok && UpnpDiscovery_set_Ext(p, UpnpDiscovery_get_Ext(q));
        ok = ok && UpnpDiscovery_set_DestAddr(p, UpnpDiscovery_get_DestAddr(q));
    }

    return ok;
}

UpnpDiscovery* UpnpDiscovery_dup(const UpnpDiscovery* q) {
    UpnpDiscovery* p = UpnpDiscovery_new();

    if (!p)
        return 0;

    UpnpDiscovery_assign(p, q);

    return p;
}

int UpnpDiscovery_get_ErrCode(const UpnpDiscovery* p) { return p->m_ErrCode; }

int UpnpDiscovery_set_ErrCode(UpnpDiscovery* p, int n) {
    p->m_ErrCode = n;

    return 1;
}

int UpnpDiscovery_get_Expires(const UpnpDiscovery* p) { return p->m_Expires; }

int UpnpDiscovery_set_Expires(UpnpDiscovery* p, int n) {
    p->m_Expires = n;

    return 1;
}

const UpnpString* UpnpDiscovery_get_DeviceID(const UpnpDiscovery* p) {
    return p->m_DeviceID;
}

int UpnpDiscovery_set_DeviceID(UpnpDiscovery* p, const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_DeviceID, q);
}

size_t UpnpDiscovery_get_DeviceID_Length(const UpnpDiscovery* p) {
    return UpnpString_get_Length(UpnpDiscovery_get_DeviceID(p));
}

const char* UpnpDiscovery_get_DeviceID_cstr(const UpnpDiscovery* p) {
    return UpnpString_get_String(UpnpDiscovery_get_DeviceID(p));
}

int UpnpDiscovery_strcpy_DeviceID(UpnpDiscovery* p, const char* s) {
    return UpnpString_set_String(p->m_DeviceID, s);
}

int UpnpDiscovery_strncpy_DeviceID(UpnpDiscovery* p, const char* s, size_t n) {
    return UpnpString_set_StringN(p->m_DeviceID, s, n);
}

void UpnpDiscovery_clear_DeviceID(UpnpDiscovery* p) {
    UpnpString_clear(p->m_DeviceID);
}

const UpnpString* UpnpDiscovery_get_DeviceType(const UpnpDiscovery* p) {
    return p->m_DeviceType;
}

int UpnpDiscovery_set_DeviceType(UpnpDiscovery* p, const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_DeviceType, q);
}

size_t UpnpDiscovery_get_DeviceType_Length(const UpnpDiscovery* p) {
    return UpnpString_get_Length(UpnpDiscovery_get_DeviceType(p));
}

const char* UpnpDiscovery_get_DeviceType_cstr(const UpnpDiscovery* p) {
    return UpnpString_get_String(UpnpDiscovery_get_DeviceType(p));
}

int UpnpDiscovery_strcpy_DeviceType(UpnpDiscovery* p, const char* s) {
    return UpnpString_set_String(p->m_DeviceType, s);
}

int UpnpDiscovery_strncpy_DeviceType(UpnpDiscovery* p, const char* s,
                                     size_t n) {
    return UpnpString_set_StringN(p->m_DeviceType, s, n);
}

void UpnpDiscovery_clear_DeviceType(UpnpDiscovery* p) {
    UpnpString_clear(p->m_DeviceType);
}

const UpnpString* UpnpDiscovery_get_ServiceType(const UpnpDiscovery* p) {
    return p->m_ServiceType;
}

int UpnpDiscovery_set_ServiceType(UpnpDiscovery* p, const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_ServiceType, q);
}

size_t UpnpDiscovery_get_ServiceType_Length(const UpnpDiscovery* p) {
    return UpnpString_get_Length(UpnpDiscovery_get_ServiceType(p));
}

const char* UpnpDiscovery_get_ServiceType_cstr(const UpnpDiscovery* p) {
    return UpnpString_get_String(UpnpDiscovery_get_ServiceType(p));
}

int UpnpDiscovery_strcpy_ServiceType(UpnpDiscovery* p, const char* s) {
    return UpnpString_set_String(p->m_ServiceType, s);
}

int UpnpDiscovery_strncpy_ServiceType(UpnpDiscovery* p, const char* s,
                                      size_t n) {
    return UpnpString_set_StringN(p->m_ServiceType, s, n);
}

void UpnpDiscovery_clear_ServiceType(UpnpDiscovery* p) {
    UpnpString_clear(p->m_ServiceType);
}

const UpnpString* UpnpDiscovery_get_ServiceVer(const UpnpDiscovery* p) {
    return p->m_ServiceVer;
}

int UpnpDiscovery_set_ServiceVer(UpnpDiscovery* p, const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_ServiceVer, q);
}

size_t UpnpDiscovery_get_ServiceVer_Length(const UpnpDiscovery* p) {
    return UpnpString_get_Length(UpnpDiscovery_get_ServiceVer(p));
}

const char* UpnpDiscovery_get_ServiceVer_cstr(const UpnpDiscovery* p) {
    return UpnpString_get_String(UpnpDiscovery_get_ServiceVer(p));
}

int UpnpDiscovery_strcpy_ServiceVer(UpnpDiscovery* p, const char* s) {
    return UpnpString_set_String(p->m_ServiceVer, s);
}

int UpnpDiscovery_strncpy_ServiceVer(UpnpDiscovery* p, const char* s,
                                     size_t n) {
    return UpnpString_set_StringN(p->m_ServiceVer, s, n);
}

void UpnpDiscovery_clear_ServiceVer(UpnpDiscovery* p) {
    UpnpString_clear(p->m_ServiceVer);
}

const UpnpString* UpnpDiscovery_get_Location(const UpnpDiscovery* p) {
    return p->m_Location;
}

int UpnpDiscovery_set_Location(UpnpDiscovery* p, const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_Location, q);
}

size_t UpnpDiscovery_get_Location_Length(const UpnpDiscovery* p) {
    return UpnpString_get_Length(UpnpDiscovery_get_Location(p));
}

const char* UpnpDiscovery_get_Location_cstr(const UpnpDiscovery* p) {
    return UpnpString_get_String(UpnpDiscovery_get_Location(p));
}

int UpnpDiscovery_strcpy_Location(UpnpDiscovery* p, const char* s) {
    return UpnpString_set_String(p->m_Location, s);
}

int UpnpDiscovery_strncpy_Location(UpnpDiscovery* p, const char* s, size_t n) {
    return UpnpString_set_StringN(p->m_Location, s, n);
}

void UpnpDiscovery_clear_Location(UpnpDiscovery* p) {
    UpnpString_clear(p->m_Location);
}

const UpnpString* UpnpDiscovery_get_Os(const UpnpDiscovery* p) {
    return p->m_Os;
}

int UpnpDiscovery_set_Os(UpnpDiscovery* p, const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_Os, q);
}

size_t UpnpDiscovery_get_Os_Length(const UpnpDiscovery* p) {
    return UpnpString_get_Length(UpnpDiscovery_get_Os(p));
}

const char* UpnpDiscovery_get_Os_cstr(const UpnpDiscovery* p) {
    return UpnpString_get_String(UpnpDiscovery_get_Os(p));
}

int UpnpDiscovery_strcpy_Os(UpnpDiscovery* p, const char* s) {
    return UpnpString_set_String(p->m_Os, s);
}

int UpnpDiscovery_strncpy_Os(UpnpDiscovery* p, const char* s, size_t n) {
    return UpnpString_set_StringN(p->m_Os, s, n);
}

void UpnpDiscovery_clear_Os(UpnpDiscovery* p) { UpnpString_clear(p->m_Os); }

const UpnpString* UpnpDiscovery_get_Date(const UpnpDiscovery* p) {
    return p->m_Date;
}

int UpnpDiscovery_set_Date(UpnpDiscovery* p, const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_Date, q);
}

size_t UpnpDiscovery_get_Date_Length(const UpnpDiscovery* p) {
    return UpnpString_get_Length(UpnpDiscovery_get_Date(p));
}

const char* UpnpDiscovery_get_Date_cstr(const UpnpDiscovery* p) {
    return UpnpString_get_String(UpnpDiscovery_get_Date(p));
}

int UpnpDiscovery_strcpy_Date(UpnpDiscovery* p, const char* s) {
    return UpnpString_set_String(p->m_Date, s);
}

int UpnpDiscovery_strncpy_Date(UpnpDiscovery* p, const char* s, size_t n) {
    return UpnpString_set_StringN(p->m_Date, s, n);
}

void UpnpDiscovery_clear_Date(UpnpDiscovery* p) { UpnpString_clear(p->m_Date); }

const UpnpString* UpnpDiscovery_get_Ext(const UpnpDiscovery* p) {
    return p->m_Ext;
}

int UpnpDiscovery_set_Ext(UpnpDiscovery* p, const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_Ext, q);
}

size_t UpnpDiscovery_get_Ext_Length(const UpnpDiscovery* p) {
    return UpnpString_get_Length(UpnpDiscovery_get_Ext(p));
}

const char* UpnpDiscovery_get_Ext_cstr(const UpnpDiscovery* p) {
    return UpnpString_get_String(UpnpDiscovery_get_Ext(p));
}

int UpnpDiscovery_strcpy_Ext(UpnpDiscovery* p, const char* s) {
    return UpnpString_set_String(p->m_Ext, s);
}

int UpnpDiscovery_strncpy_Ext(UpnpDiscovery* p, const char* s, size_t n) {
    return UpnpString_set_StringN(p->m_Ext, s, n);
}

void UpnpDiscovery_clear_Ext(UpnpDiscovery* p) { UpnpString_clear(p->m_Ext); }

const struct sockaddr_storage*
UpnpDiscovery_get_DestAddr(const UpnpDiscovery* p) {
    return &p->m_DestAddr;
}

int UpnpDiscovery_set_DestAddr(UpnpDiscovery* p,
                               const struct sockaddr_storage* buf) {
    p->m_DestAddr = *buf;

    return 1;
}

void UpnpDiscovery_clear_DestAddr(UpnpDiscovery* p) {
    memset(&p->m_DestAddr, 0, sizeof(struct sockaddr_storage));
}
