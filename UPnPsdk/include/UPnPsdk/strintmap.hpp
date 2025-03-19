#ifndef UPnPdsk_STRINTMAP_HPP
#define UPnPdsk_STRINTMAP_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-03-20

/*!
 * \file
 * \brief String to integer and integer to string conversion functions.
 *
 * There are some constant mappings of a number to its string name for human
 * readability or for UPnP messages, e.g. error number, HTTP method etc. Because
 * these functions use very effective binary search, the map tables to seach
 * must be sorted by the name-string.
 */

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
 * \brief Table with C-strings mapped to an integer id.
 */
// Don't export class symbol; only used library intern.
template <typename T> //
class CStrIntMap {
  public:
    /*! \brief Value returned on error or when an index on a container is not
     * found.
     * \details Although the definition uses -1, size_t is an unsigned integer
     * type, and the value of npos is the largest \b positive value it can
     * hold, due to signed-to-unsigned implicit conversion. This is a portable
     * way to specify the largest value of any unsigned type. */
    static constexpr size_t npos = size_t(-1);

    /*! \brief Needs to know what table to use.
     * \details A valid table is a std::array() container with entries like
     * `{"GET", UPnPsdk::HTTPMETHOD_GET}` with (const char*)"GET" and
     * (int)HTTPMETHOD_GET. Any C-String and integer is possible. Due to use
     * of effective binary seach the entries must be sorted by the string
     * names. */
    CStrIntMap(T& a_table) : m_table(a_table) {}

    /*! \brief Match the given name with names from the entries in the table.
     * \returns
     *  - On success: Zero based index (position) on the table of entries.\n
     *  - On failure: CStrIntMap::npos means string not found */
    size_t index_of(
        /*! [in] String containing the name to be matched. */
        const char* a_name,
        /*! [in] Whether search should be case sensitive or not. Default is not
           case sensitive search. */
        bool a_case_sensitive = false);

    /*! \brief Returns the index from the table where the id matches the entry
     * from the table.
     * \returns
     *  - On success: Zero based index (position) on the table of entries.\n
     *  - On failure: CStrIntMap::npos means id not found */
    size_t index_of(
        /// [in] ID to be matched.
        const int a_id);

  private:
    T& m_table;
};

template <typename T>
size_t CStrIntMap<T>::index_of(const char* a_name, bool a_case_sensitive) {
    TRACE("Executing index_of(const char*)");
    if (a_name == nullptr || a_name[0] == '\0')
        return this->npos;

    // Select library compare function outside the loop with a function pointer
    // so we do not have to check on every loop for nothing.
    int (*str_cmp)(const char*, const char*);
    str_cmp = a_case_sensitive ? strcmp : strcasecmp;

    size_t top, mid, bot;
    int cmp;

    top = 0;
    bot = m_table.size() - 1;

    while (top <= bot) {
        mid = (top + bot) / 2;
        cmp = str_cmp(a_name, m_table[mid].name);
        if (cmp > 0) {
            top = mid + 1; /* look below mid */
        } else if (cmp < 0) {
            bot = mid - 1; /* look above mid */
        } else if (mid > INT_MAX) { // guard for the following type cast
            UPnPsdk_LOGCRIT("MSG1026") "Index mid="
                << mid
                << "exceeds integer limit. This program error MUST be fixed! "
                   "Program is stable but may have ignored runtime "
                   "conditions.\n";
            return this->npos;
        } else {
            return mid; /* match, return table index. */
        }
    }
    return this->npos;
}

template <typename T> //
size_t CStrIntMap<T>::index_of(int a_id) {
    TRACE("Executing index_of(int)");
    for (size_t i{0}; i < m_table.size(); i++) {
        if (m_table[i].id == a_id) {
            return i;
        }
    }
    return this->npos;
}

} // namespace UPnPsdk

#endif /* UPnPdsk_STRINTMAP_HPP */
