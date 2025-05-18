/**************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (c) 2012 France Telecom All rights reserved.
 * Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
 * Redistribution only with this Copyright remark. Last modified: 2025-05-18
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
 **************************************************************************/
// Last compare with ./Pupnp source file on 2024-10-26, ver 1.14.20
/*!
 * \file
 * \brief Internal Web Server and functions to carry out operations of it.
 */

#include <webserver.hpp>
#include <UpnpExtraHeaders.hpp>
#include <UpnpIntTypes.hpp>
#include <httpreadwrite.hpp>
#include <ssdp_common.hpp>
#include <statcodes.hpp>
#include <upnpapi.hpp>

#include <UPnPsdk/synclog.hpp>
#include <UPnPsdk/port.hpp>
#include <UPnPsdk/webserver.hpp>

#include <umock/stdlib.hpp>
#include <umock/stdio.hpp>

#ifndef COMPA_INTERNAL_CONFIG_HPP
#error "No or wrong config.hpp header file included."
#endif

#ifndef COMPA_NET_HTTP_WEBSERVER_HPP
#error "No or wrong webserver.hpp header file included."
#endif

/// \cond
#include <cassert>
#include <sys/stat.h>
#include <iostream>
/// \endcond


namespace compa {
namespace {

/*! \brief Response Types. */
enum resp_type {
    RESP_UNSPEC,
    RESP_FILEDOC,
    RESP_XMLDOC,
    RESP_HEADERS,
    RESP_WEBDOC,
    RESP_POST
};

/// \brief Media types.
const char* gMediaTypes[] = {
    NULL,          ///< 0.
    "audio",       ///< 1.
    "video",       ///< 2.
    "image",       ///< 3.
    "application", ///< 4.
    "text"         ///< 5.
};

/// \brief index to get media type of application from the media types table.
constexpr size_t APPLICATION_INDEX{4};

/// \brief Number of elements for asctime_s on win32, means buffer size.
constexpr size_t ASCTIME_R_BUFFER_SIZE{26};

/*! \brief Alias directory structure on the webserver for an XML document. */
struct CXmlAlias {
  public:
    /// \cond
    // Constructor
    // -----------
    CXmlAlias(){TRACE2(this, " Construct CXmlAlias()")}

    /// \brief Copy constructor
    // ------------------------
    CXmlAlias(const CXmlAlias& that) {
        TRACE2(this, " Executing CXmlAlias copy constructor()")

        ::pthread_mutex_lock(&m_web_mutex);
        this->m_name = that.m_name;
        this->m_last_modified = that.m_last_modified;
        if (that.m_doc.data() == nullptr) {
            this->m_ct = that.m_ct;
            this->m_doc = that.m_doc;
        } else {
            this->m_ct = 1;
            const size_t doc_len{that.m_doc.length()};
            char* doc = static_cast<char*>(malloc(doc_len));
            memcpy(doc, that.m_doc.data(), doc_len);
            this->m_doc = std::string_view(doc, doc_len);
        }
        ::pthread_mutex_unlock(&m_web_mutex);
    }

    /// \brief Copy assignment operator
    // --------------------------------
    CXmlAlias& operator=(CXmlAlias that) {
        TRACE2(this, " Executing CXmlAlias assignment operator=")
        // The copy constructor has alread done its work to copy the current
        // object to the stack. No need to protect with a mutex. This swapping
        // from the stack is thread safe.
        std::swap(this->m_name, that.m_name);
        std::swap(this->m_doc, that.m_doc);
        std::swap(this->m_last_modified, that.m_last_modified);
        std::swap(this->m_ct, that.m_ct); // Set by the copy constructor.

        // by convention, always return *this
        return *this;
    }

    // Destructor
    // ----------
    ~CXmlAlias() {
        TRACE2(this, " Destruct CXmlAlias()")
        this->clear_private();
        if (::pthread_mutex_destroy(&m_web_mutex) != 0)
            UPnPsdk_LOGCRIT(
                "MSG1150") "fails with EBUSY. The mutex is currently locked.\n";
    }
    /// \endcond

    /*! \brief Set an XML Document
     * <!-- ------------------ -->
     * \copydetails web_server_set_alias()
     *
     * \note Due to taking over ownership:
     *  - Parameter **a_alias_content** is invalid (nullptr) after return. It
     * cannot be reused and must be new set.
     *  - Parameter **a_alias_content_length** is set to 0 after return and must
     * be new set. */
    int set(const char* a_alias_name, const char*& a_alias_content,
            size_t& a_alias_content_length,
            time_t a_last_modified = time(nullptr)) {
        TRACE2(this, " Executing CXmlAlias::set()")
        ::pthread_mutex_lock(&m_web_mutex);

        if (a_alias_name == nullptr) {
            /* don't serve aliased doc anymore */
            this->release_private();
            ::pthread_mutex_unlock(&m_web_mutex);
            return UPNP_E_SUCCESS;
        }
        if (a_alias_content == nullptr) {
            ::pthread_mutex_unlock(&m_web_mutex);
            return UPNP_E_INVALID_ARGUMENT;
        }
        this->release_private();

        /* insert leading /, if missing */
        if (*a_alias_name != '/')
            m_name = '/';
        m_name += a_alias_name;
        UPnPsdk_LOGINFO(
            "MSG1133") "attaching allocated document, take ownership.\n";
        m_doc = std::string_view(a_alias_content, a_alias_content_length);
        m_last_modified = a_last_modified;
        a_alias_content = nullptr;
        a_alias_content_length = 0;
        m_ct = 1;

        ::pthread_mutex_unlock(&m_web_mutex);
        return UPNP_E_SUCCESS;
    }

    /// \brief Get the name of the root udevice description XML document
    // -----------------------------------------------------------------
    std::string_view name() const {
        TRACE2(this, " Executing CXmlAlias::name()")
        ::pthread_mutex_lock(&m_web_mutex);
        std::string_view name = m_name;
        ::pthread_mutex_unlock(&m_web_mutex);
        return name;
    }

    /// \brief Get the root udevice description XML document
    // -----------------------------------------------------
    std::string_view doc() const {
        TRACE2(this, " Executing CXmlAlias::doc()")
        ::pthread_mutex_lock(&m_web_mutex);
        std::string_view doc = m_doc;
        ::pthread_mutex_unlock(&m_web_mutex);
        return doc;
    }

    /*! \brief Get last modified date of the root udevice description XML
     * document */
    // ------------------------------------------------------------------
    time_t last_modified() const {
        TRACE2(this, " Executing CXmlAlias::last_modified()")
        ::pthread_mutex_lock(&m_web_mutex);
        auto last_modified = m_last_modified;
        ::pthread_mutex_unlock(&m_web_mutex);
        return last_modified;
    }

    /// \brief Returns if the XML object contains a valid XML document
    // ---------------------------------------------------------------
    bool is_valid() const {
        TRACE2(this, " Executing CXmlAlias::is_valid()")
        ::pthread_mutex_lock(&m_web_mutex);
        auto doc = m_doc.data();
        ::pthread_mutex_unlock(&m_web_mutex);
        return doc != nullptr;
    }

    /// \brief Release an XML document from the XML object
    // ---------------------------------------------------
    void release() {
        TRACE2(this, " Executing CXmlAlias::release()")
        ::pthread_mutex_lock(&m_web_mutex);
        this->release_private();
        ::pthread_mutex_unlock(&m_web_mutex);
    }

    /// \brief Clear the XML object
    // ----------------------------
    void clear() {
        TRACE2(this, " Executing CXmlAlias::clear()")
        ::pthread_mutex_lock(&m_web_mutex);
        this->clear_private();
        ::pthread_mutex_unlock(&m_web_mutex);
    }

  private:
    /*! \brief Mutex to protect this class */
    mutable ::pthread_mutex_t m_web_mutex = PTHREAD_MUTEX_INITIALIZER;

    /*! \brief name of DOC from root; e.g.: /foo/bar/mydesc.xml */
    std::string m_name;

    /*! \brief the XML document contents. */
    std::string_view m_doc{std::string_view(nullptr, 0)};

    /*! \brief Last modified time. */
    time_t m_last_modified{};

    /// cond;
    // Counter how often the document is requested and will be decremented with
    // this->release(). Only if m_cnt == 0 then this->clear_private() is called.
    int m_ct{};
    /// endcond;


    /// \brief Release an XML document from the XML object without mutex lock
    // ----------------------------------------------------------------------
    inline void release_private() {
        /* ignore invalid alias */
        if (m_ct > 0) {
            m_ct--;
            if (m_ct <= 0)
                this->clear_private();
        }
    }

    /// \brief Clear the XML object without mutex lock
    // -----------------------------------------------
    inline void clear_private() {
        m_name.clear();
        // std::string_view 'm_doc' was created from an (external) allocated
        // raw pointer that ownership has been overtaken with this. To be able
        // to free this raw pointer the const protection of the
        // std::string_view must be removed.
        if (m_doc.data() != nullptr) {
            UPnPsdk_LOGINFO("MSG1118") "freeing attached allocated document.\n";
            ::free(const_cast<char*>(m_doc.data()));
            m_doc = std::string_view(nullptr, 0);
        }
        m_last_modified = 0;
        m_ct = 0;
    }
};

/*! \brief Global XML document object. */
CXmlAlias gAliasDoc;


/*! \name Scope restricted to file
 * @{
 */

#if defined(_WIN32) || defined(DOXYGEN_RUN)
/*! \brief Multiplatform wrapper to make win32 asctime_s compatible to posix
 * asctime_r.
 * \details Only available on Microsoft Windows.*/
char* web_server_asctime_r(const struct tm* tm, char* buf) {
    if (tm == NULL || buf == NULL)
        return NULL;

    asctime_s(buf, ASCTIME_R_BUFFER_SIZE, tm);
    return buf;
}
#else
#define web_server_asctime_r asctime_r
#endif

/*!
 * \brief Based on the extension, returns the content type and content
 * subtype.
 *
 * For example:
 * Ext | type  | subtype
 * --- | ----- | -------
 * txt | text  | plain
 * htm | text  | html
 * xml | text  | xml
 * mp3 | audio | mpeg
 * The complete list you find at UPnPsdk::mediatype_list.
 *
 * \returns
 *  On Success: 0\n
 *  On Error: -1 - not found
 */
inline int search_extension(   //
    const char* a_extension,   ///< [in]
    const char** a_con_type,   ///< [out]
    const char** a_con_subtype ///< [out]
) {
    TRACE("Executing search_extension()")

    const UPnPsdk::Document_meta* filetype =
        UPnPsdk::select_filetype(std::string_view(a_extension));

    if (filetype == nullptr) {
        TRACE("  search_extension: return with -1")
        return -1;
    }
    *a_con_type = filetype->type.c_str();
    *a_con_subtype = filetype->subtype.c_str();

    return 0;
}

/*!
 * \brief Based on the extension of the \p filename, clones an XML string based
 * on type and content subtype.
 *
 * If content type and sub type are not found, unknown types are used.
 *
 * \returns
 *  On Success: 0.\n
 *  On Error:
 *  - UPNP_E_FILE_NOT_FOUND - on invalid filename.
 *  - UPNP_E_INVALID_ARGUMENT - on invalid fileInfo.
 *  - UPNP_E_OUTOF_MEMORY - on memory allocation failures.
 */
inline int get_content_type(
    const char* filename,  ///< [in] with extension, extension will be used.
    UpnpFileInfo* fileInfo ///< [out]
) {
    if (!filename)
        return UPNP_E_FILE_NOT_FOUND;
    if (!fileInfo)
        return UPNP_E_INVALID_ARGUMENT;

    const char* extension;
    const char* type{};
    const char* subtype{};
    int ctype_found = 0;
    char* temp = NULL;
    size_t length = 0;
    int rc = 0;

    UpnpFileInfo_set_ContentType(fileInfo, NULL);
    /* get ext */
    extension = strrchr(filename, '.');
    if (extension != NULL) {
        if (search_extension(extension + 1, &type, &subtype) == 0)
            ctype_found = 1;
    }
    if (!ctype_found) {
        /* unknown content type */
        type = gMediaTypes[APPLICATION_INDEX];
        subtype = "octet-stream";
    }
    length = strlen(type) + strlen("/") + strlen(subtype) + 1;
    temp = (char*)malloc(length);
    if (!temp)
        return UPNP_E_OUTOF_MEMORY;
    rc = snprintf(temp, length, "%s/%s", type, subtype);
    if (rc < 0 || (unsigned int)rc >= length) {
        umock::stdlib_h.free(temp);
        return UPNP_E_OUTOF_MEMORY;
    }
    UpnpFileInfo_set_ContentType(fileInfo, temp);
    umock::stdlib_h.free(temp);
    if (!UpnpFileInfo_get_ContentType(fileInfo))
        return UPNP_E_OUTOF_MEMORY;

    return 0;
}

/*!
 * \brief Get file information.
 *
 * \returns Integer.
 */
int get_file_info(
    /*! [in] Filename having the description document. */
    const char* filename,
    /*! [out] File information object having file attributes such as
     * filelength, when was the file last modified, whether a file or a
     * directory and whether the file or directory is readable. */
    UpnpFileInfo* info) {
    int code;
    struct stat s;
    FILE* fp;
    int fd;
    int rc = 0;
    time_t aux_LastModified;
    struct tm date;
    char buffer[ASCTIME_R_BUFFER_SIZE];

    UpnpFileInfo_set_ContentType(info, NULL);
#ifdef _WIN32
    umock::stdio_h.fopen_s(&fp, filename, "r");
#else
    fp = umock::stdio_h.fopen(filename, "r");
#endif
    /* check readable */
    UpnpFileInfo_set_IsReadable(info, fp != NULL);
    if (!fp) {
        rc = -1;
        goto exit_function;
    }
    fd = fileno(fp);
    if (fd == -1) {
        rc = -1;
        goto exit_function;
    }
    code = fstat(fd, &s);
    if (code == -1) {
        rc = -1;
        goto exit_function;
    }
    umock::stdio_h.fclose(fp);
    fp = NULL;
    if (S_ISDIR(s.st_mode)) {
        UpnpFileInfo_set_IsDirectory(info, 1);
    } else if (S_ISREG(s.st_mode)) {
        UpnpFileInfo_set_IsDirectory(info, 0);
    } else {
        rc = -1;
        goto exit_function;
    }
    UpnpFileInfo_set_FileLength(info, s.st_size);
    UpnpFileInfo_set_LastModified(info, s.st_mtime);
    rc = get_content_type(filename, info);
    aux_LastModified = UpnpFileInfo_get_LastModified(info);
    UPnPsdk_LOGINFO("MSG1108") "webserver file info=\""
        << filename << "\", length=" << UpnpFileInfo_get_FileLength(info)
        << ", last_mod=\""
        << web_server_asctime_r(http_gmtime_r(&aux_LastModified, &date), buffer)
        << "\", readable=" << UpnpFileInfo_get_IsReadable(info) << ".\n";

exit_function:
    if (fp) {
        umock::stdio_h.fclose(fp);
    }
    return rc;
}

/*!
 * \brief Compare file names.
 *
 * Compare the file names between the one on the XML alias and the one
 * passed in as the input parameter. If equal extract file information.
 *
 * \return
 * \li \c 1 - On Success
 * \li \c 0 if request is not an alias
 */
inline int get_alias(
    /*! [in] request file passed in to be compared with. */
    const char* request_file,
    /*! [out] xml alias object which has a file name stored. */
    CXmlAlias* alias,
    /*! [out] File information object which will be filled up if the file
     * comparison succeeds. */
    UpnpFileInfo* info) {
    int cmp = strcmp(alias->name().data(), request_file);
    if (cmp == 0) {
        UpnpFileInfo_set_FileLength(info,
                                    static_cast<off_t>(alias->doc().length()));
        UpnpFileInfo_set_IsDirectory(info, 0);
        UpnpFileInfo_set_IsReadable(info, 1);
        UpnpFileInfo_set_LastModified(info, alias->last_modified());
    }

    return cmp == 0;
}

/*!
 * \brief Compares filePath with paths from the list of virtual directory
 * lists.
 *
 * \return int.
 */
int isFileInVirtualDir(
    /*! [in] Directory path to be tested for virtual directory. */
    char* filePath,
    /*! [out] The cookie registered with this virtual directory, if matched.
     */
    const void** cookie) {
    virtualDirList* pCurVirtualDir;
    size_t webDirLen;

    pCurVirtualDir = pVirtualDirList;
    while (pCurVirtualDir != NULL) {
        webDirLen = strlen(pCurVirtualDir->dirName);
        if (webDirLen) {
            if (pCurVirtualDir->dirName[webDirLen - 1] == '/') {
                if (strncmp(pCurVirtualDir->dirName, filePath, webDirLen) ==
                    0) {
                    if (cookie != NULL)
                        *cookie = pCurVirtualDir->cookie;
                    return 1;
                }
            } else {
                if (strncmp(pCurVirtualDir->dirName, filePath, webDirLen) ==
                        0 &&
                    (filePath[webDirLen] == '/' ||
                     filePath[webDirLen] == '\0' ||
                     filePath[webDirLen] == '?')) {
                    if (cookie != NULL)
                        *cookie = pCurVirtualDir->cookie;
                    return 1;
                }
            }
        }
        pCurVirtualDir = pCurVirtualDir->next;
    }

    return 0;
}

/*!
 * \brief Converts C string in place to upper case.
 */
void ToUpperCase(
    /*! [in,out] string to be converted. */
    char* s) {
    while (*s) {
        *s = static_cast<char>(std::toupper(static_cast<unsigned char>(*s)));
        ++s;
    }
}

/*!
 * \brief Finds a substring from a string in a case insensitive way.
 *
 * \returns A pointer to the first occurence of s2 in s1.
 */
char* StrStr(
    /*! [in] Input string. */
    char* s1,
    /*! [in] Input sub-string. */
    const char* s2) {
    char* Str1;
    char* Str2;
    const char* Ptr;
    char* ret = nullptr;

    Str1 = strdup(s1);
    if (!Str1)
        goto error1;
    Str2 = strdup(s2);
    if (!Str2)
        goto error2;
    ToUpperCase(Str1);
    ToUpperCase(Str2);
    Ptr = strstr(Str1, Str2);
    if (!Ptr)
        ret = NULL;
    else
        ret = s1 + (Ptr - Str1);

    umock::stdlib_h.free(Str2);
error2:
    umock::stdlib_h.free(Str1);
error1:
    return ret;
}

/*!
 * \brief Finds next token in a string.
 *
 * \return Pointer to the next token.
 */
char* StrTok(
    /*! [in] String containing the token. */
    char** Src,
    /*! [in] Set of delimiter characters. */
    const char* Del) {
    char* TmpPtr;
    char* RetPtr;

    if (*Src != NULL) {
        RetPtr = *Src;
        TmpPtr = strstr(*Src, Del);
        if (TmpPtr != NULL) {
            *TmpPtr = '\0';
            *Src = TmpPtr + strlen(Del);
        } else
            *Src = NULL;

        return RetPtr;
    }

    return NULL;
}

/*!
 * \brief Returns a range of integers from a string.
 *
 * \returns Always returns 1.
 */
int GetNextRange(
    /*! string containing the token / range. */
    char** SrcRangeStr,
    /*! gets the first byte of the token. */
    off_t* FirstByte,
    /*! gets the last byte of the token. */
    off_t* LastByte) {
    char* Ptr;
    char* Tok;
    int i;
    int64_t F = -1;
    int64_t L = -1;
    int Is_Suffix_byte_Range = 1;

    if (*SrcRangeStr == NULL)
        return -1;
    Tok = StrTok(SrcRangeStr, ",");
    if ((Ptr = strstr(Tok, "-")) == NULL)
        return -1;
    *Ptr = ' ';
#ifdef _WIN32
    sscanf_s(Tok, "%" SCNd64 "%" SCNd64, &F, &L);
#else
    sscanf(Tok, "%" SCNd64 "%" SCNd64, &F, &L);
#endif
    if (F == -1 || L == -1) {
        *Ptr = '-';
        for (i = 0; i < (int)strlen(Tok); i++) {
            if (Tok[i] == '-') {
                break;
            } else if (isdigit(Tok[i])) {
                Is_Suffix_byte_Range = 0;
                break;
            }
        }
        if (Is_Suffix_byte_Range) {
            *FirstByte = (off_t)L;
            *LastByte = (off_t)F;
            return 1;
        }
    }
    *FirstByte = (off_t)F;
    *LastByte = (off_t)L;

    return 1;
}

/*!
 * \brief Fills in the Offset, read size and contents to send out as an HTTP
 * Range Response.
 *
 * \return
 * \li \c HTTP_BAD_REQUEST
 * \li \c HTTP_INTERNAL_SERVER_ERROR
 * \li \c HTTP_REQUEST_RANGE_NOT_SATISFIABLE
 * \li \c HTTP_OK
 */
int CreateHTTPRangeResponseHeader(
    /*! String containing the range. */
    char* ByteRangeSpecifier,
    /*! Length of the file. */
    off_t FileLength,
    /*! [out] SendInstruction object where the range operations will be
       stored. */
    struct SendInstruction* Instr) {
    off_t FirstByte, LastByte;
    char* RangeInput;
    char* Ptr;
    int rc = 0;

    Instr->IsRangeActive = 1;
    Instr->ReadSendSize = FileLength;
    if (!ByteRangeSpecifier)
        return HTTP_BAD_REQUEST;
    RangeInput = strdup(ByteRangeSpecifier);
    if (!RangeInput)
        return HTTP_INTERNAL_SERVER_ERROR;
    /* CONTENT-RANGE: bytes 222-3333/4000  HTTP_PARTIAL_CONTENT */
    if (StrStr(RangeInput, "bytes") == NULL ||
        (Ptr = StrStr(RangeInput, "=")) == NULL) {
        umock::stdlib_h.free(RangeInput);
        Instr->IsRangeActive = 0;
        return HTTP_BAD_REQUEST;
    }
    /* Jump = */
    Ptr = Ptr + 1;
    if (FileLength < 0) {
        int ret = HTTP_REQUEST_RANGE_NOT_SATISFIABLE;
        if ((*Ptr == '0') && (*(Ptr + 1) == '-') && (*(Ptr + 2) == '\0')) {
            Instr->IsRangeActive = 0;
            ret = HTTP_OK;
        }
        umock::stdlib_h.free(RangeInput);
        return ret;
    }
    if (GetNextRange(&Ptr, &FirstByte, &LastByte) != -1) {
        if (FileLength < FirstByte) {
            umock::stdlib_h.free(RangeInput);
            return HTTP_REQUEST_RANGE_NOT_SATISFIABLE;
        }
        if (FirstByte >= 0 && LastByte >= 0 && LastByte >= FirstByte) {
            if (LastByte >= FileLength)
                LastByte = FileLength - 1;
            Instr->RangeOffset = FirstByte;
            Instr->ReadSendSize = LastByte - FirstByte + 1;
            /* Data between two range. */
            rc = snprintf(
                Instr->RangeHeader, sizeof(Instr->RangeHeader),
                "CONTENT-RANGE: bytes %" PRId64 "-%" PRId64 "/%" PRId64 "\r\n",
                (int64_t)FirstByte, (int64_t)LastByte, (int64_t)FileLength);
            if (rc < 0 || (unsigned int)rc >= sizeof(Instr->RangeHeader)) {
                umock::stdlib_h.free(RangeInput);
                return HTTP_INTERNAL_SERVER_ERROR;
            }
        } else if (FirstByte >= 0 && LastByte == -1 && FirstByte < FileLength) {
            Instr->RangeOffset = FirstByte;
            Instr->ReadSendSize = FileLength - FirstByte;
            rc = snprintf(Instr->RangeHeader, sizeof(Instr->RangeHeader),
                          "CONTENT-RANGE: bytes %" PRId64 "-%" PRId64
                          "/%" PRId64 "\r\n",
                          (int64_t)FirstByte, (int64_t)(FileLength - 1),
                          (int64_t)FileLength);
            if (rc < 0 || (unsigned int)rc >= sizeof(Instr->RangeHeader)) {
                umock::stdlib_h.free(RangeInput);
                return HTTP_INTERNAL_SERVER_ERROR;
            }
        } else if (FirstByte == -1 && LastByte > 0) {
            if (LastByte >= FileLength) {
                Instr->RangeOffset = 0;
                Instr->ReadSendSize = FileLength;
                rc = snprintf(Instr->RangeHeader, sizeof(Instr->RangeHeader),
                              "CONTENT-RANGE: bytes 0-%" PRId64 "/%" PRId64
                              "\r\n",
                              (int64_t)(FileLength - 1), (int64_t)FileLength);
            } else {
                Instr->RangeOffset = FileLength - LastByte;
                Instr->ReadSendSize = LastByte;
                rc = snprintf(Instr->RangeHeader, sizeof(Instr->RangeHeader),
                              "CONTENT-RANGE: bytes %" PRId64 "-%" PRId64
                              "/%" PRId64 "\r\n",
                              (int64_t)(FileLength - LastByte),
                              (int64_t)FileLength - 1, (int64_t)FileLength);
            }
            if (rc < 0 || (unsigned int)rc >= sizeof(Instr->RangeHeader)) {
                umock::stdlib_h.free(RangeInput);
                return HTTP_INTERNAL_SERVER_ERROR;
            }
        } else {
            umock::stdlib_h.free(RangeInput);
            return HTTP_REQUEST_RANGE_NOT_SATISFIABLE;
        }
    } else {
        umock::stdlib_h.free(RangeInput);
        return HTTP_REQUEST_RANGE_NOT_SATISFIABLE;
    }

    umock::stdlib_h.free(RangeInput);
    return HTTP_OK;
}

/*!
 * \brief Get header id from the request parameter.
 *
 * Get header id from the request parameter and take appropriate action based
 * on the ids as an HTTP Range Response.
 *
 * \returns
 * \li \c HTTP_BAD_REQUEST
 * \li \c HTTP_INTERNAL_SERVER_ERROR
 * \li \c HTTP_REQUEST_RANGE_NOT_SATISFIABLE
 * \li \c HTTP_OK
 */
int CheckOtherHTTPHeaders(
    /*! [in] HTTP Request message. */
    http_message_t* Req,
    /*! [out] Send Instruction object to data for the response. */
    struct SendInstruction* RespInstr,
    /*! Size of the file containing the request document. */
    off_t FileSize) {
    http_header_t* header;
    ListNode* node;
    /*NNS: dlist_node* node; */
    size_t index;
    int RetCode = HTTP_OK;
    char* TmpBuf;
    size_t TmpBufSize = LINE_SIZE;

    TmpBuf = (char*)malloc(TmpBufSize);
    if (!TmpBuf)
        return HTTP_INTERNAL_SERVER_ERROR;
    node = ListHead(&Req->headers);
    while (node != NULL) {
        header = (http_header_t*)node->item;
        /* find header type. */
#if 0
        index =
            map_str_to_int((const char*)header->name.buf, header->name.length,
                           &Http_Header_Names[0], Http_Header_Names.size(), 0);
#endif
        UPnPsdk::CStrIntMap http_header_names_table(Http_Header_Names);
        index = http_header_names_table.index_of(header->name.buf);

        if (header->value.length >= TmpBufSize) {
            umock::stdlib_h.free(TmpBuf);
            TmpBufSize = header->value.length + 1;
            TmpBuf = (char*)malloc(TmpBufSize);
            if (!TmpBuf)
                return HTTP_INTERNAL_SERVER_ERROR;
        }
        memcpy(TmpBuf, header->value.buf, header->value.length);
        TmpBuf[header->value.length] = '\0';
        if (index != http_header_names_table.npos) {
            switch (Http_Header_Names[index].id) {
            case HDR_TE: {
                /* Request */
                RespInstr->IsChunkActive = 1;

                if (strlen(TmpBuf) > strlen("gzip")) {
                    /* means client will accept trailer. */
                    if (StrStr(TmpBuf, "trailers") != NULL) {
                        RespInstr->IsTrailers = 1;
                    }
                }
                break;
            }
            case HDR_CONTENT_LENGTH:
                RespInstr->RecvWriteSize = atoi(TmpBuf);
                break;
            case HDR_RANGE:
                RetCode =
                    CreateHTTPRangeResponseHeader(TmpBuf, FileSize, RespInstr);
                if (RetCode != HTTP_OK) {
                    umock::stdlib_h.free(TmpBuf);
                    return RetCode;
                }
                break;
            case HDR_ACCEPT_LANGUAGE:
                if (header->value.length + 1 >
                    sizeof(RespInstr->AcceptLanguageHeader)) {
                    size_t length = sizeof(RespInstr->AcceptLanguageHeader) - 1;
                    memcpy(RespInstr->AcceptLanguageHeader, TmpBuf, length);
                    RespInstr->AcceptLanguageHeader[length] = '\0';
                } else {
                    memcpy(RespInstr->AcceptLanguageHeader, TmpBuf,
                           header->value.length + 1);
                }
                break;
            default:
                /*
                   TODO
                 */
                /*
                   header.value is the value.
                 */
                /*
                   case HDR_CONTENT_TYPE: return 1;
                   case HDR_CONTENT_LANGUAGE:return 1;
                   case HDR_LOCATION: return 1;
                   case HDR_CONTENT_LOCATION:return 1;
                   case HDR_ACCEPT: return 1;
                   case HDR_ACCEPT_CHARSET: return 1;
                   case HDR_USER_AGENT: return 1;
                 */

                /*Header check for encoding */
                /*
                   case HDR_ACCEPT_RANGE:
                   case HDR_CONTENT_RANGE:
                   case HDR_IF_RANGE:
                 */

                /*Header check for encoding */
                /*
                   case HDR_ACCEPT_ENCODING:
                   if(StrStr(TmpBuf, "identity"))
                   {
                   break;
                   }
                   else return -1;
                   case HDR_CONTENT_ENCODING:
                   case HDR_TRANSFER_ENCODING:
                 */
                break;
            }
        }
        node = ListNext(&Req->headers, node);
    }
    umock::stdlib_h.free(TmpBuf);

    return RetCode;
}

/*!
 * \brief Free extra HTTP headers.
 */
void FreeExtraHTTPHeaders(
    /*! [in] extra HTTP headers to free. */
    [[maybe_unused]] UpnpListHead* extraHeadersList) {
    UpnpListIter pos;
    UpnpExtraHeaders* extra;

    for (pos = UpnpListBegin(extraHeadersList);
         pos != UpnpListEnd(extraHeadersList);) {
        extra = (UpnpExtraHeaders*)pos;
        pos = UpnpListErase(extraHeadersList, pos);
        UpnpExtraHeaders_delete(extra);
    }
}

/*!
 * \brief Build an array of unrecognized headers.
 *
 * \returns
 *  On success: HTTP_OK\n
 *  On error: HTTP_INTERNAL_SERVER_ERROR
 */
int ExtraHTTPHeaders(
    /*! [in] HTTP Request message. */
    [[maybe_unused]] http_message_t* Req,
    /*! [in] Extra header list. */
    [[maybe_unused]] UpnpListHead* extraHeadersList) {
    http_header_t* header;
    ListNode* node;
    size_t index;
    UpnpExtraHeaders* extraHeader;
    UpnpListHead* extraHeaderNode;

    node = ListHead(&Req->headers);
    while (node != NULL) {
        header = (http_header_t*)node->item;
        /* find header type. */
#if 0
        index = map_str_to_int((const char*)header->name.buf,
                               header->name.length, &Http_Header_Names[0],
                               Http_Header_Names.size(), 0);
#endif
        UPnPsdk::CStrIntMap http_header_names_table(Http_Header_Names);
        index = http_header_names_table.index_of(header->name.buf);

        if (index == http_header_names_table.npos) {
            extraHeader = UpnpExtraHeaders_new();
            if (!extraHeader) {
                FreeExtraHTTPHeaders(extraHeadersList);
                return HTTP_INTERNAL_SERVER_ERROR;
            }
            extraHeaderNode =
                (UpnpListHead*)UpnpExtraHeaders_get_node(extraHeader);
            UpnpListInsert(extraHeadersList, UpnpListEnd(extraHeadersList),
                           extraHeaderNode);
            UpnpExtraHeaders_strncpy_name(extraHeader, header->name.buf,
                                          header->name.length);
            UpnpExtraHeaders_strncpy_value(extraHeader, header->value.buf,
                                           header->value.length);
        }
        node = ListNext(&Req->headers, node);
    }

    return HTTP_OK;
}

/*!
 * \brief Process a remote request and return the result.
 *
 * Creates the different kind of header depending on the type of request.
 *
 * \returns
 *  On success: HTTP_OK\n
 *  On error:
 *  - HTTP_BAD_REQUEST
 *  - HTTP_INTERNAL_SERVER_ERROR
 *  - HTTP_REQUEST_RANGE_NOT_SATISFIABLE
 *  - HTTP_FORBIDDEN
 *  - HTTP_NOT_FOUND
 *  - HTTP_NOT_ACCEPTABLE
 */
int process_request_in(
    /*! [in] Socket info. */
    SOCKINFO* info,
    /*! [in] HTTP Request message. */
    http_message_t* req,
    /*! [out] Tpye of response. */
    enum resp_type* rtype,
    /*! [out] Headers. */
    membuffer* headers,
    /*! [out] Get filename from request document. */
    membuffer* filename,
    /*! [out] Xml alias document from the request document. */
    CXmlAlias* a_alias,
    /*! [out] Send Instruction object where the response is set up. */
    SendInstruction* RespInstr) {
    int code;
    int err_code;

    char* request_doc{nullptr};
    UpnpFileInfo* finfo;
    time_t aux_LastModified;
    bool alias_grabbed{false};
    int using_alias{0};
    int using_virtual_dir{0};
    uri_type* url;
    const char* temp_str;
    int resp_major;
    int resp_minor;
    size_t dummy;
    memptr hdr_value;

    url = &req->uri;
    assert(req->method == HTTPMETHOD_GET || req->method == HTTPMETHOD_HEAD ||
           req->method == HTTPMETHOD_POST ||
           req->method == HTTPMETHOD_SIMPLEGET);
    /* init */
    memset(&finfo, 0, sizeof(finfo));
    finfo = UpnpFileInfo_new();
    err_code = HTTP_INTERNAL_SERVER_ERROR; /* default error */

    http_CalcResponseVersion(req->major_version, req->minor_version,
                             &resp_major, &resp_minor);
    /* */
    /* remove dots */
    /* */
    request_doc = (char*)malloc(url->pathquery.size + 1);
    if (request_doc == NULL) {
        goto error_handler; /* out of mem */
    }
    memcpy(request_doc, url->pathquery.buff, url->pathquery.size);
    request_doc[url->pathquery.size] = '\0';
    dummy = url->pathquery.size;
    remove_escaped_chars(request_doc, &dummy);
    code = remove_dots(request_doc, url->pathquery.size);
    if (code != 0) {
        err_code = HTTP_FORBIDDEN;
        goto error_handler;
    }
    if (*request_doc != '/') {
        /* no slash */
        err_code = HTTP_BAD_REQUEST;
        goto error_handler;
    }
    if (isFileInVirtualDir(request_doc, &RespInstr->Cookie)) {
        using_virtual_dir = 1;
        RespInstr->IsVirtualFile = 1;
        if (membuffer_assign_str(filename, request_doc) != 0) {
            goto error_handler;
        }
    } else {
        /* try using alias */
        if (a_alias != nullptr && gAliasDoc.is_valid()) {
            *a_alias = gAliasDoc;
            alias_grabbed = true;
            using_alias = get_alias(request_doc, a_alias, finfo);
            if (using_alias == 1) {
                UpnpFileInfo_set_ContentType(finfo,
                                             "text/xml; charset=\"utf-8\"");
                if (UpnpFileInfo_get_ContentType(finfo) == NULL) {
                    goto error_handler;
                }
            }
        }
    }
    if (using_virtual_dir) {
        if (req->method != HTTPMETHOD_POST) {
            if ((code = ExtraHTTPHeaders(
                     req, (UpnpListHead*)UpnpFileInfo_get_ExtraHeadersList(
                              finfo))) != HTTP_OK) {
                err_code = code;
                goto error_handler;
            }

            UpnpFileInfo_set_CtrlPtIPAddr(finfo, &info->foreign_sockaddr);

            if (httpmsg_find_hdr(req, HDR_USER_AGENT, &hdr_value) != NULL) {
                UpnpFileInfo_strncpy_Os(finfo, hdr_value.buf, hdr_value.length);
            }

            /* get file info */
            if (virtualDirCallback.get_info(filename->buf, finfo,
                                            RespInstr->Cookie,
                                            &RespInstr->RequestCookie) != 0) {
                err_code = HTTP_NOT_FOUND;
                goto error_handler;
            }
            /* try index.html if req is a dir */
            if (UpnpFileInfo_get_IsDirectory(finfo)) {
                if (filename->buf[filename->length - 1] == '/') {
                    temp_str = "index.html";
                } else {
                    temp_str = "/index.html";
                }
                if (membuffer_append_str(filename, temp_str) != 0) {
                    goto error_handler;
                }
                /* get info */
                if (virtualDirCallback.get_info(
                        filename->buf, finfo, RespInstr->Cookie,
                        &RespInstr->RequestCookie) != UPNP_E_SUCCESS ||
                    UpnpFileInfo_get_IsDirectory(finfo)) {
                    err_code = HTTP_NOT_FOUND;
                    goto error_handler;
                }
            }
            /* not readable */
            if (!UpnpFileInfo_get_IsReadable(finfo)) {
                err_code = HTTP_FORBIDDEN;
                goto error_handler;
            }
            /* finally, get content type */
            /* if ( get_content_type(filename->buf, &content_type)
             * != 0 ) */
            /*{ */
            /*  goto error_handler; */
            /* } */
        }
    } else if (!using_alias) {
        if (gDocumentRootDir.length == 0) {
            goto error_handler;
        }
        /* */
        /* get file name */
        /* */

        /* filename str */
        if (membuffer_assign_str(filename, gDocumentRootDir.buf) != 0 ||
            membuffer_append_str(filename, request_doc) != 0) {
            goto error_handler; /* out of mem */
        }
        /* remove trailing slashes */
        while (filename->length > 0 &&
               filename->buf[filename->length - 1] == '/') {
            membuffer_delete(filename, filename->length - 1, 1);
        }
        if (req->method != HTTPMETHOD_POST) {
            /* get info on file */
            if (get_file_info(filename->buf, finfo) != 0) {
                err_code = HTTP_NOT_FOUND;
                goto error_handler;
            }
            /* try index.html if req is a dir */
            if (UpnpFileInfo_get_IsDirectory(finfo)) {
                if (filename->buf[filename->length - 1] == '/') {
                    temp_str = "index.html";
                } else {
                    temp_str = "/index.html";
                }
                if (membuffer_append_str(filename, temp_str) != 0) {
                    goto error_handler;
                }
                /* get info */
                if (get_file_info(filename->buf, finfo) != 0 ||
                    UpnpFileInfo_get_IsDirectory(finfo)) {
                    err_code = HTTP_NOT_FOUND;
                    goto error_handler;
                }
            }
            /* not readable */
            if (!UpnpFileInfo_get_IsReadable(finfo)) {
                err_code = HTTP_FORBIDDEN;
                goto error_handler;
            }
        }
        /* finally, get content type */
        /*      if ( get_content_type(filename->buf, &content_type) != 0
         * ) */
        /*      { */
        /*          goto error_handler; */
        /*      } */
    }
    RespInstr->CorsHeader =
        (gWebserverCorsString.length > 0) ? gWebserverCorsString.buf : NULL;
    RespInstr->ReadSendSize = UpnpFileInfo_get_FileLength(finfo);
    /* Check other header field. */
    code = CheckOtherHTTPHeaders(req, RespInstr,
                                 UpnpFileInfo_get_FileLength(finfo));
    if (code != HTTP_OK) {
        err_code = code;
        goto error_handler;
    }
    if (req->method == HTTPMETHOD_POST) {
        *rtype = RESP_POST;
        err_code = HTTP_OK;
        goto error_handler;
    }

    /* Check if chunked encoding should be used. */
    if (using_virtual_dir &&
        UpnpFileInfo_get_FileLength(finfo) == UPNP_USING_CHUNKED) {
        /* Chunked encoding is only supported by HTTP 1.1 clients */
        if (resp_major == 1 && resp_minor == 1) {
            RespInstr->IsChunkActive = 1;
        } else {
            /* The virtual callback indicates that we should use
             * chunked encoding however the client doesn't support
             * it. Return with an internal server error. */
            err_code = HTTP_NOT_ACCEPTABLE;
            goto error_handler;
        }
    }

    aux_LastModified = UpnpFileInfo_get_LastModified(finfo);
    if (RespInstr->IsRangeActive && RespInstr->IsChunkActive) {
        /* Content-Range: bytes 222-3333/4000  HTTP_PARTIAL_CONTENT */
        /* Transfer-Encoding: chunked */
        if (http_MakeMessage(
                headers, resp_major, resp_minor,
                "R"
                "T"
                "GKLAD"
                "s"
                "tcS"
                "Xc"
                "ECc",
                HTTP_PARTIAL_CONTENT,                /* status code */
                UpnpFileInfo_get_ContentType(finfo), /* content type */
                RespInstr,                           /* range info */
                RespInstr,                           /* language info */
                RespInstr, /* Access-Control-Allow-Origin */
                "LAST-MODIFIED: ", &aux_LastModified, X_USER_AGENT,
                UpnpFileInfo_get_ExtraHeadersList(finfo)) != 0) {
            goto error_handler;
        }
    } else if (RespInstr->IsRangeActive && !RespInstr->IsChunkActive) {
        /* Content-Range: bytes 222-3333/4000  HTTP_PARTIAL_CONTENT */
        if (http_MakeMessage(
                headers, resp_major, resp_minor,
                "R"
                "N"
                "T"
                "GLAD"
                "s"
                "tcS"
                "Xc"
                "ECc",
                HTTP_PARTIAL_CONTENT,                /* status code */
                RespInstr->ReadSendSize,             /* content length */
                UpnpFileInfo_get_ContentType(finfo), /* content type */
                RespInstr,                           /* range info */
                RespInstr,                           /* language info */
                RespInstr, /* Access-Control-Allow-Origin */
                "LAST-MODIFIED: ", &aux_LastModified, X_USER_AGENT,
                UpnpFileInfo_get_ExtraHeadersList(finfo)) != 0) {
            goto error_handler;
        }
    } else if (!RespInstr->IsRangeActive && RespInstr->IsChunkActive) {
        /* Transfer-Encoding: chunked */
        if (http_MakeMessage(
                headers, resp_major, resp_minor,
                "RK"
                "TLAD"
                "s"
                "tcS"
                "Xc"
                "ECc",
                HTTP_OK,                             /* status code */
                UpnpFileInfo_get_ContentType(finfo), /* content type */
                RespInstr,                           /* language info */
                RespInstr, /* Access-Control-Allow-Origin */
                "LAST-MODIFIED: ", &aux_LastModified, X_USER_AGENT,
                UpnpFileInfo_get_ExtraHeadersList(finfo)) != 0) {
            goto error_handler;
        }
    } else {
        /* !RespInstr->IsRangeActive && !RespInstr->IsChunkActive */
        if (RespInstr->ReadSendSize >= 0) {
            if (http_MakeMessage(
                    headers, resp_major, resp_minor,
                    "R"
                    "N"
                    "TLAD"
                    "s"
                    "tcS"
                    "Xc"
                    "ECc",
                    HTTP_OK,                             /* status code */
                    RespInstr->ReadSendSize,             /* content length */
                    UpnpFileInfo_get_ContentType(finfo), /* content type */
                    RespInstr,                           /* language info */
                    RespInstr, /* Access-Control-Allow-Origin */
                    "LAST-MODIFIED: ", &aux_LastModified, X_USER_AGENT,
                    UpnpFileInfo_get_ExtraHeadersList(finfo)) != 0) {
                goto error_handler;
            }
        } else {
            if (http_MakeMessage(
                    headers, resp_major, resp_minor,
                    "R"
                    "TLAD"
                    "s"
                    "tcS"
                    "Xc"
                    "ECc",
                    HTTP_OK,                             /* status code */
                    UpnpFileInfo_get_ContentType(finfo), /* content type */
                    RespInstr,                           /* language info */
                    RespInstr, /* Access-Control-Allow-Origin */
                    "LAST-MODIFIED: ", &aux_LastModified, X_USER_AGENT,
                    UpnpFileInfo_get_ExtraHeadersList(finfo)) != 0) {
                goto error_handler;
            }
        }
    }
    if (req->method == HTTPMETHOD_HEAD) {
        *rtype = RESP_HEADERS;
    } else if (using_alias) {
        /* GET xml */
        *rtype = RESP_XMLDOC;
    } else if (using_virtual_dir) {
        *rtype = RESP_WEBDOC;
    } else {
        /* GET filename */
        *rtype = RESP_FILEDOC;
    }
    /* simple get http 0.9 as specified in http 1.0 */
    /* don't send headers */
    if (req->method == HTTPMETHOD_SIMPLEGET) {
        membuffer_destroy(headers);
    }
    err_code = HTTP_OK;

error_handler:
    umock::stdlib_h.free(request_doc);
    FreeExtraHTTPHeaders(
        (UpnpListHead*)UpnpFileInfo_get_ExtraHeadersList(finfo));
    UpnpFileInfo_delete(finfo);
    if (err_code != HTTP_OK && alias_grabbed) {
        a_alias->release();
    }

    return err_code;
}

/*!
 * \brief Receives the HTTP post message.
 *
 * \returns
 *  On success: HTTP_OK\n
 *  On error:
 *  - HTTP_INTERNAL_SERVER_ERROR
 *  - HTTP_UNAUTHORIZED
 *  - HTTP_BAD_REQUEST
 *  - HTTP_SERVICE_UNAVAILABLE
 */
int http_RecvPostMessage(
    /*! HTTP Parser object. */
    http_parser_t* parser,
    /*! [in] Socket Information object. */
    SOCKINFO* info,
    /*! File where received data is copied to. */
    char* filename,
    /*! Send Instruction object which gives information whether the file
     * is a virtual file or not. */
    struct SendInstruction* Instr) {
    size_t Data_Buf_Size = 1024;
    char Buf[1024];
    int Timeout = -1;
    FILE* Fp;
    parse_status_t status = PARSE_OK;
    int ok_on_close = 0;
    size_t entity_offset = 0;
    int num_read = 0;
    int ret_code = HTTP_OK;

    if (Instr && Instr->IsVirtualFile) {
        Fp = (FILE*)(virtualDirCallback.open)(
            filename, UPNP_WRITE, Instr->Cookie, Instr->RequestCookie);
        if (Fp == NULL)
            return HTTP_INTERNAL_SERVER_ERROR;
    } else {
#ifdef UPNP_ENABLE_POST_WRITE
        Fp = umock::stdio_h.fopen(filename, "wb");
        if (Fp == NULL)
            return HTTP_UNAUTHORIZED;
#else
        return HTTP_NOT_FOUND;
#endif
    }
    parser->position = POS_ENTITY;
    do {
        /* first parse what has already been gotten */
        if (parser->position != POS_COMPLETE)
            status = parser_parse_entity(parser);
        if (status == PARSE_INCOMPLETE_ENTITY) {
            /* read until close */
            ok_on_close = 1;
        } else if ((status != PARSE_SUCCESS) && (status != PARSE_CONTINUE_1) &&
                   (status != PARSE_INCOMPLETE)) {
            /* error */
            ret_code = HTTP_BAD_REQUEST;
            goto ExitFunction;
        }
        /* read more if necessary entity */
        while (entity_offset + Data_Buf_Size > parser->msg.entity.length &&
               parser->position != POS_COMPLETE) {
            num_read = sock_read(info, Buf, sizeof(Buf), &Timeout);
            if (num_read > 0) {
                /* append data to buffer */
                if (membuffer_append(&parser->msg.msg, Buf, (size_t)num_read) !=
                    0) {
                    /* set failure status */
                    parser->http_error_code = HTTP_INTERNAL_SERVER_ERROR;
                    ret_code = HTTP_INTERNAL_SERVER_ERROR;
                    goto ExitFunction;
                }
                status = parser_parse_entity(parser);
                if (status == PARSE_INCOMPLETE_ENTITY) {
                    /* read until close */
                    ok_on_close = 1;
                } else if ((status != PARSE_SUCCESS) &&
                           (status != PARSE_CONTINUE_1) &&
                           (status != PARSE_INCOMPLETE)) {
                    ret_code = HTTP_BAD_REQUEST;
                    goto ExitFunction;
                }
            } else if (num_read == 0) {
                if (ok_on_close) {
                    UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
                               "<<< (RECVD) "
                               "<<<\n%s\n-----------------\n",
                               parser->msg.msg.buf);
                    parser->position = POS_COMPLETE;
                } else {
                    /* partial msg or response */
                    parser->http_error_code = HTTP_BAD_REQUEST;
                    ret_code = HTTP_BAD_REQUEST;
                    goto ExitFunction;
                }
            } else {
                ret_code = HTTP_SERVICE_UNAVAILABLE;
                goto ExitFunction;
            }
        }
        if ((entity_offset + Data_Buf_Size) > parser->msg.entity.length) {
            Data_Buf_Size = parser->msg.entity.length - entity_offset;
        }
        memcpy(
            Buf,
            &parser->msg.msg.buf[parser->entity_start_position + entity_offset],
            Data_Buf_Size);
        entity_offset += Data_Buf_Size;
        if (Instr && Instr->IsVirtualFile) {
            int n = virtualDirCallback.write(
                Fp, Buf, Data_Buf_Size, Instr->Cookie, Instr->RequestCookie);
            if (n < 0) {
                ret_code = HTTP_INTERNAL_SERVER_ERROR;
                goto ExitFunction;
            }
        } else {
            size_t n = umock::stdio_h.fwrite(Buf, 1, Data_Buf_Size, Fp);
            if (n != Data_Buf_Size) {
                ret_code = HTTP_INTERNAL_SERVER_ERROR;
                goto ExitFunction;
            }
        }
    } while (parser->position != POS_COMPLETE ||
             entity_offset != parser->msg.entity.length);
ExitFunction:
    if (Instr && Instr->IsVirtualFile) {
        virtualDirCallback.close(Fp, Instr->Cookie, Instr->RequestCookie);
    } else {
        umock::stdio_h.fclose(Fp);
    }

    return ret_code;
}

/// @} // Scope restricted to file
} // anonymous namespace
} // namespace compa


void web_server_init() {
    if (bWebServerState == WEB_SERVER_DISABLED) {
        membuffer_init(&gDocumentRootDir);
        membuffer_init(&gWebserverCorsString);
        compa::gAliasDoc.clear();
        pVirtualDirList = nullptr;

        /* Initialize callbacks */
        virtualDirCallback.get_info = nullptr;
        virtualDirCallback.open = nullptr;
        virtualDirCallback.read = nullptr;
        virtualDirCallback.write = nullptr;
        virtualDirCallback.seek = nullptr;
        virtualDirCallback.close = nullptr;

        bWebServerState = WEB_SERVER_ENABLED;
        SetHTTPGetCallback(web_server_callback);
    }
}

int web_server_set_alias(const char* a_alias_name, const char* a_alias_content,
                         size_t a_alias_content_length,
                         time_t a_last_modified) {
    TRACE("Executing web_server_set_alias()")
    return compa::gAliasDoc.set(a_alias_name, a_alias_content,
                                a_alias_content_length, a_last_modified);
}

int web_server_set_root_dir(const char* root_dir) {
    TRACE("Executing web_server_set_root_dir()")
    if (root_dir == nullptr)
        return UPNP_E_INVALID_ARGUMENT;

    int ret = membuffer_assign_str(&gDocumentRootDir, root_dir);
    if (ret != 0)
        return ret;
    /* remove trailing '/', if any */
    if (gDocumentRootDir.length > 0) {
        size_t index = gDocumentRootDir.length - 1; /* last char */
        if (gDocumentRootDir.buf[index] == '/')
            membuffer_delete(&gDocumentRootDir, index, 1);
    }

    return 0;
}

int web_server_set_cors(const char* cors_string) {
    int ret;

    ret = membuffer_assign_str(&gWebserverCorsString, cors_string);
    if (ret != 0)
        return ret;

    return 0;
}

void web_server_callback(http_parser_t* a_parser,
                         /* INOUT */ http_message_t* a_req, SOCKINFO* a_info) {
    int ret;
    int timeout = -1;
    compa::resp_type rtype{compa::RESP_UNSPEC};
    membuffer headers;
    membuffer filename;
    compa::CXmlAlias xmldoc;
    SendInstruction RespInstr;

    /* init */
    memset(&RespInstr, 0, sizeof(RespInstr));
    membuffer_init(&headers);
    membuffer_init(&filename);

    /* Process request should create the different kind of header depending on
     * the type of request. */
    ret = compa::process_request_in(a_info, a_req, &rtype, &headers, &filename,
                                    &xmldoc, &RespInstr);
    if (ret != HTTP_OK) {
        /* send error code */
        http_SendStatusResponse(a_info, ret, a_req->major_version,
                                a_req->minor_version);
    } else {
        /* send response */
        switch (rtype) {
        case compa::RESP_FILEDOC:
            http_SendMessage(a_info, &timeout, "Ibf", &RespInstr, headers.buf,
                             headers.length, filename.buf);
            break;
        case compa::RESP_XMLDOC:
            http_SendMessage(a_info, &timeout, "Ibb", &RespInstr, headers.buf,
                             headers.length, xmldoc.doc().data(),
                             xmldoc.doc().length());
            xmldoc.release();
            break;
        case compa::RESP_WEBDOC:
            /*http_SendVirtualDirDoc(a_info, &timeout, "Ibf",
                &RespInstr,
                headers.buf, headers.length,
                filename.buf);*/
            http_SendMessage(a_info, &timeout, "Ibf", &RespInstr, headers.buf,
                             headers.length, filename.buf);
            break;
        case compa::RESP_HEADERS:
            /* headers only */
            http_SendMessage(a_info, &timeout, "b", headers.buf,
                             headers.length);
            break;
        case compa::RESP_POST:
            /* headers only */
            ret = compa::http_RecvPostMessage(a_parser, a_info, filename.buf,
                                              &RespInstr);
            /* Send response. */
            http_MakeMessage(&headers, 1, 1, "RTLSXcCc", ret, "text/html",
                             &RespInstr, X_USER_AGENT);
            http_SendMessage(a_info, &timeout, "b", headers.buf,
                             headers.length);
            break;
        default:
            UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
                       "webserver: Invalid response type received.\n");
            assert(0);
        }
    }
    UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
               "webserver: request processed...\n");
    membuffer_destroy(&headers);
    membuffer_destroy(&filename);
}

void SetHTTPGetCallback(MiniServerCallback callback) {
    TRACE("Executing SetHTTPGetCallback()")
    gGetCallback = callback;
}

void web_server_destroy() {
    if (bWebServerState == WEB_SERVER_ENABLED) {
        membuffer_destroy(&gDocumentRootDir);
        membuffer_destroy(&gWebserverCorsString);
        compa::gAliasDoc.clear();
        bWebServerState = WEB_SERVER_DISABLED;
        SetHTTPGetCallback(nullptr);
    }
}
