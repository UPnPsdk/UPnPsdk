#ifndef UPnPdsk_STRINTMAP_HPP
#define UPnPdsk_STRINTMAP_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-10-18

/*!
 * \file
 * \brief String to integer and integer to string conversion functions.
 *
 * There are some constant mappings of a number to its string name for human
 * readability or for UPnP messages, e.g. error number, HTTP method etc. Because
 * these functions use very effective binary search, the map tables to seach
 * must be sorted by the name-string.
 */

#include <UPnPsdk/visibility.hpp>
#include <UPnPsdk/synclog.hpp>
#include <UPnPsdk/port.hpp>
/// \cond
#include <cstddef> // for size_t
#include <cstring>
#include <climits>
/// \endcond


namespace UPnPsdk {

/// String to integer map entry.
struct str_int_entry {
    const char* name; ///< A value in string form.
    int id; ///< Same value in integer form.
};

/*!
 * \brief Match the given name with names from the entries in the table.
 * \returns
 * On success: Zero based index (position) on the table of entries.\n
 * On failure: \b -1 means string not found
 */
// Don't export function symbol; only used library intern.
template <typename T>
int str_to_int(
    const char* name, ///< [in] String containing the name to be matched.
    const T& table, ///< [in] Table of entries that need to be matched.
    bool case_sensitive =
        false /*!< [in] Whether search should be case sensitive or not. Default
                 is not case sensitive search. */
) {
    TRACE("Executing str_to_int()");
    if (name == nullptr || name[0] == '\0')
        return -1;

    size_t top, mid, bot;
    int cmp;

    top = 0;
    bot = table.size() - 1;

    while (top <= bot) {
        mid = (top + bot) / 2;
        if (case_sensitive) {
            cmp = strcmp(name, table[mid].name);
        } else {
            cmp = strcasecmp(name, table[mid].name);
        }

        if (cmp > 0) {
            top = mid + 1; /* look below mid */
        } else if (cmp < 0) {
            bot = mid - 1; /* look above mid */
        } else if (mid > INT_MAX) {
            UPNPLIB_LOGCRIT "MSG1026: Index mid="
                << mid
                << "exceeds integer limit. This program error MUST be fixed! "
                   "Program is stable but may have ignored runtime "
                   "conditions.\n";
            return -1; // guard for the following type cast
        } else {
            return static_cast<int>(mid); /* match, return table index.*/
        }
    }

    return -1; /* header name not found */
}

/*!
 * \brief Returns the index from the table where the id matches the entry from
 * the table.
 * \returns
 * On success: Zero based index (position) on the table of entries.\n
 * On failure: \b -1 means id not found
 */
// Don't export function symbol; only used library intern.
template <typename T>
int int_to_str( //
    int id, ///< [in] ID to be matched.
    const T& table ///< [in] Table of entries that need to be matched.
) {
    TRACE("Executing int_to_str()");
    for (size_t i{0}; i < table.size(); i++) {
        if (table[i].id == id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

} // namespace UPnPsdk

#endif /* UPnPdsk_STRINTMAP_HPP */
