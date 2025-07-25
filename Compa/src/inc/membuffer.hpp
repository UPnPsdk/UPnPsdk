#ifndef COMPA_GENLIB_UTIL_MEMBUFFER_HPP
#define COMPA_GENLIB_UTIL_MEMBUFFER_HPP
/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (c) 2012 France Telecom All rights reserved.
 * Copyright (C) 2021+ GPL 3 and higher by Ingo Höft,  Ingo@Hoeft-online.de>
 * Redistribution only with this Copyright remark. Last modified: 2025-06-12
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
 ******************************************************************************/
// Last compare with pupnp original source file on 2024-02-14, ver 1.14.18
/*!
 * \file
 * \brief Manage blocks of dynamically allocated memory.
 *
 * membuffer is only used internal within the library and linked with the source
 * file so there are no exports EXPORT_SPEC for external use.
 */

/// \cond
#include <cstddef>
/// \endcond

/*! \brief pointer to a chunk of memory. */
struct memptr {
    /*! \brief start of memory (read/write). */
    char* buf;
    /*! \brief length of memory (read-only). */
    size_t length;
};

/*! \brief Maintains a block of dynamically allocated memory.
 * \note Total length/capacity should not exceed MAX_INT. It is always a
 * terminating null byte ('\0') appended but not reflected in length and
 * capacity */
struct membuffer {
    /// \brief mem buffer; must not write beyond buf[length-1] (read/write).
    char* buf;
    /*! \brief length of buffer without terminating null byte (read-only). */
    size_t length;
    /// \brief total allocated memory without terminating null byte (read-only).
    size_t capacity;
    /*! \brief used to increase size; MUST be > 0; (read/write). */
    size_t size_inc;
    /*! \brief default value of size_inc. */
};
/// \cond
inline constexpr size_t MEMBUF_DEF_SIZE_INC{5};
/// \endcond

/*!
 * \brief Allocate memory and copy information from the input string to the
 * newly allocated memory.
 *
 * \return Pointer to the newly allocated memory.
 * NULL if memory cannot be allocated.
 */
// Don't export function symbol; only used library intern.
char* str_alloc(
    /*! [in] Input string object. */
    const char* str,
    /*! [in] Input string length. */
    size_t str_len);

/*!
 * \brief Compares characters of strings passed for number of bytes.
 * If equal for the number of bytes, the length of the bytes determines
 * which buffer is shorter.
 *
 * \return
 * \li <  0 string1 substring less than string2 substring
 * \li == 0 string1 substring identical to string2 substring
 * \li >  0 string1 substring greater than string2 substring
 */
// Don't export function symbol; only used library intern.
int memptr_cmp(
    /*! [in] Input memory object. */
    memptr* m,
    /*! [in] Constatnt string for the memory object to be compared with. */
    const char* s);

/*!
 * \brief Compares characters of 2 strings irrespective of the case for a
 * specific count of bytes.
 *
 * If the character comparison is the same the length of the 2 srings
 * determines the shorter of the 2 strings.
 *
 * \return
 * \li <  0 string1 substring less than string2 substring
 * \li == 0 string1 substring identical to string2 substring
 * \li >  0 string1 substring greater than string2 substring
 */
// Don't export function symbol; only used library intern.
int memptr_cmp_nocase(
    /*! [in] Input memory object. */
    memptr* m,
    /*! [in] Constant string for the memory object to be compared with. */
    const char* s);

/*!
 * \brief Increases or decreases buffer cap so that at least 'new_length'
 * bytes can be stored.
 *
 * \return
 * \li UPNP_E_SUCCESS - On Success
 * \li UPNP_E_OUTOF_MEMORY - On failure to allocate memory.
 */
// Don't export function symbol; only used library intern.
int membuffer_set_size(
    /*! [in,out] buffer whose size is to be modified. */
    membuffer* m,
    /*! [in] new size to which the buffer will be modified. */
    size_t new_length);

/*!
 * \brief Wrapper to membuffer_initialize().
 *
 * Set the size of the buffer to MEMBUF_DEF_SIZE_INC and Initializes
 * m->buf to NULL, length = 0.
 */
// Don't export function symbol; only used library intern.
void membuffer_init(
    /*! [in,out] Buffer to be initialized. */
    membuffer* m);

/*!
 * \brief Free's memory allocated for membuffer* m.
 */
// Don't export function symbol; only used library intern.
void membuffer_destroy(
    /*! [in,out] Buffer to be destroyed. */
    membuffer* m);

/*!
 * \brief Allocate memory to membuffer *m and copy the contents of the in
 * parameter const void *buf.
 *
 * \return
 * \li UPNP_E_SUCCESS
 * \li UPNP_E_OUTOF_MEMORY
 */
// Don't export function symbol; only used library intern.
int membuffer_assign(
    /*! [in,out] Buffer whose memory is to be allocated and assigned. */
    membuffer* m,
    /*! [in] Source buffer whose contents will be copied. */
    const void* buf,
    /*! [in] Length of the source buffer. */
    size_t buf_len);

/*!
 * \brief Wrapper function for membuffer_assign().
 *
 * \return
 * \li UPNP_E_SUCCESS
 * \li UPNP_E_OUTOF_MEMORY
 */
// Don't export function symbol; only used library intern.
int membuffer_assign_str(
    /*! [in,out] Buffer to be allocated and assigned. */
    membuffer* m,
    /*! [in] Source buffer whose contents will be copied. */
    const char* c_str);

/*!
 * \brief Invokes function to appends data from a constant buffer to the buffer.
 *
 * \return int.
 */
// Don't export function symbol; only used library intern.
int membuffer_append(
    /*! [in,out] Buffer whose memory is to be appended. */
    membuffer* m,
    /*! [in] Source buffer whose contents will be copied. */
    const void* buf,
    /*! [in] Length of the source buffer. */
    size_t buf_len);

/*!
 * \brief Invokes function to appends data from a constant string to the buffer.
 *
 * \return int.
 */
// Don't export function symbol; only used library intern.
int membuffer_append_str(
    /*! [in,out] Buffer whose memory is to be appended. */
    membuffer* m,
    /*! [in] Source buffer whose contents will be copied. */
    const char* c_str);

/*!
 * \brief Allocates memory for the new data to be inserted. Does
 * memory management by moving the data from the existing memory to
 * the newly allocated memory and then appending the new data.
 *
 * \return 0 if successful, error code if error.
 */
// Don't export function symbol; only used library intern.
int membuffer_insert(
    /*! [in,out] Buffer whose memory size is to be increased and appended.
     */
    membuffer* m,
    /*! [in] source buffer whose contents will be copied. */
    const void* buf,
    /*! [in] size of the source buffer. */
    size_t buf_len,
    /*! [in] index to determine the bounds while movinf the data. */
    size_t index);

/*!
 * \brief Shrink the size of the buffer depending on the current size of the
 * buffer and the input parameters. Move contents from the old buffer to the
 * new sized buffer.
 */
// Don't export function symbol; only used library intern.
void membuffer_delete(
    /*! [in,out] Buffer whose memory size is to be decreased and copied
     * to the modified location. */
    membuffer* m,
    /*! [in] Index to determine bounds while moving data. */
    size_t index,
    /*! [in] Number of bytes that the data needs to shrink by. */
    size_t num_bytes);

/*!
 * \brief Detaches current buffer and returns it. The caller must free the
 * returned buffer using free(). After this call, length becomes 0.
 *
 * \return A pointer to the current buffer.
 */
// Don't export function symbol; only used library intern.
char* membuffer_detach(
    /*! [in,out] Buffer to be returned and updated. */
    membuffer* m);

/*!
 * \brief Free existing memory in membuffer and assign the new buffer in its
 * place.
 *
 * \note 'new_buf' must be allocted using malloc or realloc so that it can be
 * freed using free().
 */
// Don't export function symbol; only used library intern.
void membuffer_attach(
    /*! [in,out] Buffer to be updated. */
    membuffer* m,
    /*! [in] Source buffer which will be assigned to the buffer to be
     * updated. */
    char* new_buf,
    /*! [in] Length of the source buffer. */
    size_t buf_len);

#endif /* COMPA_GENLIB_UTIL_MEMBUFFER_HPP */
