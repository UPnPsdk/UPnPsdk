/* *****************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
 * Redistribution only with this Copyright remark. Last modified: 2024-10-18
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

#include <strintmap.hpp>
#include <UPnPsdk/synclog.hpp>
#include <array>

/// \brief Size of the temporary std::array() container that can hold all tables
constexpr size_t container_size{33};

/// \todo This complex and expensive interface wrapper must be removed.
int map_str_to_int(const char* name, size_t name_len,
                   const str_int_entry* table, int num_entries,
                   int case_sensitive) {
    TRACE("Executing map_str_to_int()");
    // num_entries must not be negative due to unsigned type cast later.
    if (name_len <= 0 || table == nullptr || num_entries <= 0)
        return -1;

    // Create a std::array container from plain C array.
    std::array<str_int_entry, container_size> tbl{};
    if (tbl.size() < static_cast<size_t>(num_entries)) {
        UPNPLIB_LOGCRIT "MSG1029: std::array container["
            << tbl.size() << "] is to small, requested " << num_entries
            << " entries. This program error MUST be fixed! Program is stable "
               "but may have ignored runtime conditions.\n";
        return -1;
    }
    size_t i{0};
    for (; i < static_cast<size_t>(num_entries); i++) {
        tbl[i] = table[i];
    }
    // Fill remaining entries with dummy values needed for binary search.
    constexpr char str_zzzz[]{"ZZZZ"};
    for (; i < tbl.size(); i++) {
        tbl[i].name = str_zzzz;
        tbl[i].id = INT_MIN + 8;
    }

    return UPnPsdk::str_to_int(name, tbl, case_sensitive);
}


/// \todo This complex and expensive interface wrapper must be removed.
int map_int_to_str(int id, const str_int_entry* table, int num_entries) {
    TRACE("Executing map_int_to_str()");
    // num_entries must not be negative due to unsigned type cast later.
    if (table == nullptr || num_entries <= 0)
        return -1;

    // Create a std::array container from plain C array.
    std::array<str_int_entry, container_size> tbl{};
    if (tbl.size() < static_cast<size_t>(num_entries)) {
        UPNPLIB_LOGCRIT "MSG1032: std::array container["
            << tbl.size() << "] is to small, requested " << num_entries
            << " entries. This program error MUST be fixed! Program is stable "
               "but may have ignored runtime conditions.\n";
        return -1;
    }
    size_t i{0};
    for (; i < static_cast<size_t>(num_entries); i++) {
        tbl[i] = table[i];
    }
    // Fill remaining entries with dummy values needed for binary search.
    constexpr char str_zzzz[]{"ZZZZ"};
    for (; i < tbl.size(); i++) {
        tbl[i].name = str_zzzz;
        tbl[i].id = INT_MIN + 8;
    }

    return UPnPsdk::int_to_str(id, tbl);
}
