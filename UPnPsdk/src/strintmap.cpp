/* *****************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
 * Redistribution only with this Copyright remark. Last modified: 2024-10-11
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * - Neither name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ****************************************************************************/
/*!
 * \file
 * \brief String to integer and integer to string conversion functions.
 */

#include <UPnPsdk/strintmap.hpp>
#include <UPnPsdk/synclog.hpp>
/// \cond
#include <cstring>
/// \endcond


namespace UPnPsdk {

int str_to_int(const char* name, size_t name_len, const str_int_entry* table,
               int num_entries, int case_sensitive) {
    TRACE("Executing str_to_int()");
    if (name == nullptr || name_len == 0 || table == nullptr)
        return -1;

    int top, mid, bot;
    int cmp;

    top = 0;
    bot = num_entries - 1;

    while (top <= bot) {
        mid = (top + bot) / 2;
        if (case_sensitive) {
            cmp = strncmp(name, table[mid].name, name_len);
        } else {
            cmp = strncasecmp(name, table[mid].name, name_len);
        }

        if (cmp > 0) {
            top = mid + 1; /* look below mid */
        } else if (cmp < 0) {
            bot = mid - 1; /* look above mid */
            /* cmp == 0 */
        } else if (strlen(table[mid].name) != name_len) {
            return -1; // matched substring of table entry
        } else {
            return mid; /* match; return table index */
        }
    }

    return -1; /* header name not found */
}

int int_to_str(int id, const str_int_entry* table, int num_entries) {
    TRACE("Executing int_to_str()");
    if (table == nullptr)
        return -1;

    for (int i = 0; i < num_entries; i++) {
        if (table[i].id == id) {
            return i;
        }
    }
    return -1;
}

} // namespace UPnPsdk
