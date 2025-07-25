// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-07-19
// Taken from authors who haven't made a note.

/*!
 * \file
 *
 * \brief Source file for UpnpExtraHeaders methods.
 * \author Marcelo Roberto Jimenez
 */
#include "config.hpp"

#include "UpnpExtraHeaders.hpp"

// #include <stdlib.h> /* for calloc(), free() */
// #include <string.h> /* for strlen(), strdup(), memset */

struct s_UpnpExtraHeaders {
    UpnpListHead m_node;
    UpnpString* m_name;
    UpnpString* m_value;
    DOMString m_resp;
};

UpnpExtraHeaders* UpnpExtraHeaders_new(void) {
    struct s_UpnpExtraHeaders* p =
        (s_UpnpExtraHeaders*)calloc(1, sizeof(struct s_UpnpExtraHeaders));

    if (!p)
        return 0;

    UpnpListInit(&p->m_node);
    p->m_name = UpnpString_new();
    p->m_value = UpnpString_new();
    /*p->m_resp = 0;*/

    return (UpnpExtraHeaders*)p;
}

void UpnpExtraHeaders_delete(UpnpExtraHeaders* q) {
    struct s_UpnpExtraHeaders* p = (struct s_UpnpExtraHeaders*)q;

    if (!p)
        return;

    ixmlFreeDOMString(p->m_resp);
    p->m_resp = 0;
    UpnpString_delete(p->m_value);
    p->m_value = 0;
    UpnpString_delete(p->m_name);
    p->m_name = 0;
    UpnpListInit(&p->m_node);

    free(p);
}

int UpnpExtraHeaders_assign(UpnpExtraHeaders* p, const UpnpExtraHeaders* q) {
    int ok = 1;

    if (p != q) {
        ok = ok && UpnpExtraHeaders_set_node(p, UpnpExtraHeaders_get_node(q));
        ok = ok && UpnpExtraHeaders_set_name(p, UpnpExtraHeaders_get_name(q));
        ok = ok && UpnpExtraHeaders_set_value(p, UpnpExtraHeaders_get_value(q));
        ok = ok && UpnpExtraHeaders_set_resp(p, UpnpExtraHeaders_get_resp(q));
    }

    return ok;
}

UpnpExtraHeaders* UpnpExtraHeaders_dup(const UpnpExtraHeaders* q) {
    UpnpExtraHeaders* p = UpnpExtraHeaders_new();

    if (!p)
        return 0;

    UpnpExtraHeaders_assign(p, q);

    return p;
}

const UpnpListHead* UpnpExtraHeaders_get_node(const UpnpExtraHeaders* p) {
    return &p->m_node;
}

int UpnpExtraHeaders_set_node(UpnpExtraHeaders* p, const UpnpListHead* q) {
    p->m_node = *q;

    return 1;
}

void UpnpExtraHeaders_add_to_list_node(UpnpExtraHeaders* p,
                                       struct UpnpListHead* head) {
    UpnpListHead* list = &p->m_node;
    UpnpListInsert(list, UpnpListEnd(list), head);
}

const UpnpString* UpnpExtraHeaders_get_name(const UpnpExtraHeaders* p) {
    return p->m_name;
}

int UpnpExtraHeaders_set_name(UpnpExtraHeaders* p, const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_name, q);
}

size_t UpnpExtraHeaders_get_name_Length(const UpnpExtraHeaders* p) {
    return UpnpString_get_Length(UpnpExtraHeaders_get_name(p));
}

const char* UpnpExtraHeaders_get_name_cstr(const UpnpExtraHeaders* p) {
    return UpnpString_get_String(UpnpExtraHeaders_get_name(p));
}

int UpnpExtraHeaders_strcpy_name(UpnpExtraHeaders* p, const char* s) {
    return UpnpString_set_String(p->m_name, s);
}

int UpnpExtraHeaders_strncpy_name(UpnpExtraHeaders* p, const char* s,
                                  size_t n) {
    return UpnpString_set_StringN(p->m_name, s, n);
}

void UpnpExtraHeaders_clear_name(UpnpExtraHeaders* p) {
    UpnpString_clear(p->m_name);
}

const UpnpString* UpnpExtraHeaders_get_value(const UpnpExtraHeaders* p) {
    return p->m_value;
}

int UpnpExtraHeaders_set_value(UpnpExtraHeaders* p, const UpnpString* s) {
    const char* q = UpnpString_get_String(s);

    return UpnpString_set_String(p->m_value, q);
}

size_t UpnpExtraHeaders_get_value_Length(const UpnpExtraHeaders* p) {
    return UpnpString_get_Length(UpnpExtraHeaders_get_value(p));
}

const char* UpnpExtraHeaders_get_value_cstr(const UpnpExtraHeaders* p) {
    return UpnpString_get_String(UpnpExtraHeaders_get_value(p));
}

int UpnpExtraHeaders_strcpy_value(UpnpExtraHeaders* p, const char* s) {
    return UpnpString_set_String(p->m_value, s);
}

int UpnpExtraHeaders_strncpy_value(UpnpExtraHeaders* p, const char* s,
                                   size_t n) {
    return UpnpString_set_StringN(p->m_value, s, n);
}

void UpnpExtraHeaders_clear_value(UpnpExtraHeaders* p) {
    UpnpString_clear(p->m_value);
}

const DOMString UpnpExtraHeaders_get_resp(const UpnpExtraHeaders* p) {
    return p->m_resp;
}

int UpnpExtraHeaders_set_resp(UpnpExtraHeaders* p, const DOMString s) {
    DOMString q = ixmlCloneDOMString(s);
    if (!q)
        return 0;
    ixmlFreeDOMString(p->m_resp);
    p->m_resp = q;

    return 1;
}

const char* UpnpExtraHeaders_get_resp_cstr(const UpnpExtraHeaders* p) {
    return (const char*)UpnpExtraHeaders_get_resp(p);
}
