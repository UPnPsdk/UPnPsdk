/* *****************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (c) 2012 France Telecom All rights reserved.
 * Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
 * Redistribution only with this Copyright remark. Last modified: 2025-05-29
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
// Last verify with this project pupnp source file on 2023-07-20, ver 1.14.17
/*!
 * \file
 * \brief Contains functions for scanner and parser for http messages.
 */

/// \cond
#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* For strcasestr() in string.h */
#endif
/// \endcond

#include <httpparser.hpp>
#include <statcodes.hpp>
#include <upnpdebug.hpp>

#include <UPnPsdk/synclog.hpp>

/// \cond
#include <cassert>
#include <cstdarg>
#include <limits.h>
/// \endcond

/* entity positions */


namespace { // anonymous namespace to keep scope local to file

/*! \brief Used to represent different types of tokens in input. */
enum token_type_t {
    TT_IDENTIFIER,
    TT_WHITESPACE,
    TT_CRLF,
    TT_CTRL,
    TT_SEPARATOR,
    TT_QUOTEDSTRING
};

/*! \brief Defines the HTTP methods. */
// This is prepared for binary search and must be sorted by method-name.
inline constexpr std::array<const UPnPsdk::str_int_entry, 11> Http_Method_Table{
    {{"DELETE", HTTPMETHOD_DELETE},
     {"GET", HTTPMETHOD_GET},
     {"HEAD", HTTPMETHOD_HEAD},
     {"M-POST", HTTPMETHOD_MPOST},
     {"M-SEARCH", HTTPMETHOD_MSEARCH},
     {"NOTIFY", HTTPMETHOD_NOTIFY},
     {"POST", HTTPMETHOD_POST},
     {"SUBSCRIBE", HTTPMETHOD_SUBSCRIBE},
     {"UNSUBSCRIBE", HTTPMETHOD_UNSUBSCRIBE},
     {"POST", SOAPMETHOD_POST},
     {"PUT", HTTPMETHOD_PUT}}};

/* *********************************************************************/
/* ***********                 scanner                     *************/
/* *********************************************************************/

/// \cond
constexpr char TOKCHAR_CR{0xD};
constexpr char TOKCHAR_LF{0xA};
/// \endcond

/*!
 * \brief Initialize scanner
 */
inline void scanner_init( //
    scanner_t* scanner,   ///< [out] Scanner Object to be initialized
    membuffer* bufptr     ///< [in]  Buffer to be copied
) {
    scanner->cursor = (size_t)0;
    scanner->msg = bufptr;
    scanner->entire_msg_loaded = 0;
}

/*!
 * \brief Determines if the passed value is a separator.
 */
inline int is_separator_char(
    int c ///< [in] Character to be tested against used separator values
) {
    return strchr(" \t()<>@,;:\\\"/[]?={}", c) != 0;
}

/*!
 * \brief Determines if the passed value is permissible in token.
 */
inline int is_identifier_char( //
    int c ///< [in] Character to be tested for separator values
) {
    return c >= 32 && c <= 126 && !is_separator_char(c);
}

/*!
 * \brief Determines if the passed value is a control character
 */
inline int is_control_char( //
    int c ///< [in] Character to be tested for a control character
) {
    return (c >= 0 && c <= 31) || c == 127;
}

/*!
 * \brief Determines if the passed value is permissible in qdtext
 */
inline int is_qdtext_char( //
    int c                  ///< [in] Character to be tested for CR/LF
) {
    /* we don't check for this; it's checked in get_token() */
    assert(c != '"');

    return (c >= 32 && c != 127) || c < 0 || c == TOKCHAR_CR ||
           c == TOKCHAR_LF || c == '\t';
}

/*!
 * \brief Reads next token from the input stream.
 *
 * 0 and is used as a marker, and will not be valid in a quote.
 *
 * \returns
 *  On success: PARSE_OK\n
 *  On error:
 *  - PARSE_INCOMPLETE - not enough chars to get a token
 *  - PARSE_FAILURE    - bad msg format
 */
parse_status_t scanner_get_token( //
    scanner_t* scanner,           ///< [in,out] Scanner Object
    memptr* token,                ///< [out] Token
    token_type_t* tok_type        ///< [out] Type of token
) {
    char* cursor;
    char* null_terminator; /* point to null-terminator in buffer */
    int c;
    token_type_t token_type;
    int got_end_quote;

    assert(scanner);
    assert(token);
    assert(tok_type);

    /* point to next char in buffer */
    cursor = scanner->msg->buf + scanner->cursor;
    null_terminator = scanner->msg->buf + scanner->msg->length;
    /* not enough chars in input to parse */
    if (cursor == null_terminator)
        return PARSE_INCOMPLETE;
    c = *cursor;
    if (is_identifier_char(c)) {
        /* scan identifier */
        token->buf = cursor++;
        token_type = TT_IDENTIFIER;
        while (cursor < null_terminator && is_identifier_char(*cursor))
            cursor++;
        if (!scanner->entire_msg_loaded && cursor == null_terminator)
            /* possibly more valid chars */
            return PARSE_INCOMPLETE;
        /* calc token length */
        token->length = (size_t)cursor - (size_t)token->buf;
    } else if (c == ' ' || c == '\t') {
        token->buf = cursor++;
        token_type = TT_WHITESPACE;
        while (cursor < null_terminator && (*cursor == ' ' || *cursor == '\t'))
            cursor++;
        if (!scanner->entire_msg_loaded && cursor == null_terminator)
            /* possibly more chars */
            return PARSE_INCOMPLETE;
        token->length = (size_t)cursor - (size_t)token->buf;
    } else if (c == TOKCHAR_CR) {
        /* scan CRLF */
        token->buf = cursor++;
        if (cursor == null_terminator)
            /* not enuf info to determine CRLF */
            return PARSE_INCOMPLETE;
        if (*cursor != TOKCHAR_LF) {
            /* couldn't match CRLF; match as CR */
            token_type = TT_CTRL; /* ctrl char */
            token->length = (size_t)1;
        } else {
            /* got CRLF */
            token->length = (size_t)2;
            token_type = TT_CRLF;
            cursor++;
        }
    } else if (c == TOKCHAR_LF) { /* accept \n as CRLF */
        token->buf = cursor++;
        token->length = (size_t)1;
        token_type = TT_CRLF;
    } else if (c == '"') {
        /* quoted text */
        token->buf = cursor++;
        token_type = TT_QUOTEDSTRING;
        got_end_quote = 0;
        while (cursor < null_terminator) {
            c = *cursor++;
            if (c == '"') {
                got_end_quote = 1;
                break;
            } else if (c == '\\') {
                if (cursor < null_terminator) {
                    /* c = *cursor++; */
                    cursor++;
                    /* the char after '\\' could be ANY
                     * octet */
                }
                /* else, while loop handles incomplete buf */
            } else if (is_qdtext_char(c)) {
                /* just accept char */
            } else
                /* bad quoted text */
                return PARSE_FAILURE;
        }
        if (got_end_quote)
            token->length = (size_t)cursor - (size_t)token->buf;
        else { /* incomplete */

            assert(cursor == null_terminator);
            return PARSE_INCOMPLETE;
        }
    } else if (is_separator_char(c)) {
        /* scan separator */
        token->buf = cursor++;
        token_type = TT_SEPARATOR;
        token->length = (size_t)1;
    } else if (is_control_char(c)) {
        /* scan ctrl char */
        token->buf = cursor++;
        token_type = TT_CTRL;
        token->length = (size_t)1;
    } else
        return PARSE_FAILURE;

    scanner->cursor += token->length; /* move to next token */
    *tok_type = token_type;
    return PARSE_OK;
}

/*!
 * \brief Get pointer to next character in string.
 *
 * \returns
 *  Pointer to next character in string
 */
inline char* scanner_get_str( //
    scanner_t* scanner        ///< [in] Scanner Object
) {
    return scanner->msg->buf + scanner->cursor;
}

/*!
 * \brief Compares name id in the http headers.
 *
 * \returns
 *  true - if name_id of header1 is equal name_id of header2\n
 *  false - otherwise
 */
int httpmsg_compare( //
    void* param1,    ///< [in] Pointer to a HTTP header
    void* param2     ///< [in] Pointer to a HTTP header
) {
    assert(param1 != NULL);
    assert(param2 != NULL);

    return ((http_header_t*)param1)->name_id ==
           ((http_header_t*)param2)->name_id;
}

/*!
 * \brief Free memory allocated for the http header.
 */
void httpheader_free( //
    void* msg         ///< [in] Pointer to HTTP header.
) {
    http_header_t* hdr = (http_header_t*)msg;

    membuffer_destroy(&hdr->name_buf);
    membuffer_destroy(&hdr->value);
    free(hdr);
}

/*!
 * \brief Skips blank lines at the start of a msg.
 *
 * \returns
 *  On success: PARSE_OK\n
 *  On error:
 *  - PARSE_INCOMPLETE - not enough chars to get a token
 *  - PARSE_FAILURE - bad msg format
 */
inline parse_status_t
skip_blank_lines(scanner_t* scanner ///< [in,out] Scanner Object
) {
    memptr token;
    token_type_t tok_type;
    parse_status_t status;

    /* skip ws, crlf */
    do {
        status = scanner_get_token(scanner, &token, &tok_type);
    } while (status == (parse_status_t)PARSE_OK &&
             (tok_type == (token_type_t)TT_WHITESPACE ||
              tok_type == (token_type_t)TT_CRLF));
    if (status == (parse_status_t)PARSE_OK) {
        /* pushback a non-whitespace token */
        scanner->cursor -= token.length;
    }

    return status;
}

/*!
 * \brief Skip linear whitespace.
 *
 * \returns
 *  On success: PARSE_OK - (LWS)* removed from input\n
 *  On error:
 *  - PARSE_FAILURE - bad input
 *  - PARSE_INCOMPLETE - incomplete input
 */
inline parse_status_t skip_lws(scanner_t* scanner ///< [in,out] Scanner Object
) {
    memptr token;
    token_type_t tok_type;
    parse_status_t status;
    size_t save_pos;
    int matched;

    do {
        save_pos = scanner->cursor;
        matched = 0;

        /* get CRLF or WS */
        status = scanner_get_token(scanner, &token, &tok_type);
        if (status == (parse_status_t)PARSE_OK) {
            if (tok_type == (token_type_t)TT_CRLF) {
                /* get WS */
                status = scanner_get_token(scanner, &token, &tok_type);
            }

            if (status == (parse_status_t)PARSE_OK &&
                tok_type == (token_type_t)TT_WHITESPACE) {
                matched = 1;
            } else {
                /* did not match LWS; pushback token(s) */
                scanner->cursor = save_pos;
            }
        }
    } while (matched);

    /* if entire msg is loaded, ignore an 'incomplete' warning */
    if (status == (parse_status_t)PARSE_INCOMPLETE &&
        scanner->entire_msg_loaded) {
        status = PARSE_OK;
    }

    return status;
}

/*!
 * \brief Match a string without whitespace or CRLF (%S)
 *
 * \returns
 *  On success: PARSE_OK\n
 *  On error:
 *  - PARSE_NO_MATCH
 *  - PARSE_FAILURE
 *  - PARSE_INCOMPLETE
 */
inline parse_status_t match_non_ws_string(
    scanner_t* scanner, ///< [in,out] Scanner Object.
    memptr* str         ///< [out] Buffer to get the scanner buffer contents.
) {
    memptr token{};
    token_type_t tok_type;
    parse_status_t status{};
    int done = 0;
    size_t save_cursor;

    save_cursor = scanner->cursor;

    str->length = (size_t)0;
    str->buf = scanner_get_str(scanner); /* point to next char in input */

    while (!done) {
        status = scanner_get_token(scanner, &token, &tok_type);
        if (status == (parse_status_t)PARSE_OK &&
            tok_type != (token_type_t)TT_WHITESPACE &&
            tok_type != (token_type_t)TT_CRLF) {
            /* append non-ws token */
            str->length += token.length;
        } else {
            done = 1;
        }
    }

    if (status == (parse_status_t)PARSE_OK) {
        /* last token was WS; push it back in */
        scanner->cursor -= token.length;
    }
    /* tolerate 'incomplete' msg */
    if (status == (parse_status_t)PARSE_OK ||
        (status == (parse_status_t)PARSE_INCOMPLETE &&
         scanner->entire_msg_loaded)) {
        if (str->length == (size_t)0) {
            /* no strings found */
            return PARSE_NO_MATCH;
        } else {
            return PARSE_OK;
        }
    } else {
        /* error -- pushback tokens */
        scanner->cursor = save_cursor;
        return status;
    }
}

/*!
 * \brief Matches a raw value in a the input.
 *
 * Value's length can be 0 or more. Whitespace after value is trimmed. On
 * success, scanner points the CRLF that ended the value.
 *
 * \returns
 *  On success: PARSE:_OK\n
 *  On error:
 *  - PARSE_INCOMPLETE
 *  - PARSE_FAILURE
 */
inline parse_status_t
match_raw_value(scanner_t* scanner, ///< [in,out] Scanner Object
                memptr* raw_value   ///< [out] Buffer to get the scanner buffer.
) {
    memptr token;
    token_type_t tok_type;
    parse_status_t status{};
    int done = 0;
    int saw_crlf = 0;
    size_t pos_at_crlf = (size_t)0;
    size_t save_pos;
    char c;

    save_pos = scanner->cursor;

    /* value points to start of input */
    raw_value->buf = scanner_get_str(scanner);
    raw_value->length = (size_t)0;

    while (!done) {
        status = scanner_get_token(scanner, &token, &tok_type);
        if (status == (parse_status_t)PARSE_OK) {
            if (!saw_crlf) {
                if (tok_type == (token_type_t)TT_CRLF) {
                    /* CRLF could end value */
                    saw_crlf = 1;

                    /* save input position at start of CRLF
                     */
                    pos_at_crlf = scanner->cursor - token.length;
                }
                /* keep appending value */
                raw_value->length += token.length;
            } else /* already seen CRLF */
            {
                if (tok_type == (token_type_t)TT_WHITESPACE) {
                    /* start again; forget CRLF */
                    saw_crlf = 0;
                    raw_value->length += token.length;
                } else {
                    /* non-ws means value ended just before
                     * CRLF */
                    done = 1;

                    /* point to the crlf which ended the
                     * value */
                    scanner->cursor = pos_at_crlf;
                }
            }
        } else {
            /* some kind of error; restore scanner position */
            scanner->cursor = save_pos;
            done = 1;
        }
    }

    if (status == (parse_status_t)PARSE_OK) {
        /* trim whitespace on right side of value */
        while (raw_value->length > (size_t)0) {
            /* get last char */
            c = raw_value->buf[raw_value->length - (size_t)1];

            if (c != ' ' && c != '\t' && c != TOKCHAR_CR && c != TOKCHAR_LF) {
                /* done; no more whitespace */
                break;
            }
            /* remove whitespace */
            raw_value->length--;
        }
    }

    return status;
}

/*!
 * \brief Matches an unsigned integer value in the input.
 *
 * The integer is returned in 'value'. Except for PARSE_OK result, the scanner's
 * cursor is moved back to its original position on error.
 *
 * \returns
 *  On success: PARSE_OK
 *  On error:
 *  - PARSE_NO_MATCH - got different kind of token
 *  - PARSE_FAILURE - bad input
 *  - PARSE_INCOMPLETE
 */
inline parse_status_t match_int(
    scanner_t* scanner, ///< [in,out] Scanner Object.
    int base,  ///< [in] Base of number in the string; valid values: 10 or 16.
    int* value ///< [out] Number stored here.
) {
    memptr token;
    token_type_t tok_type;
    parse_status_t status;
    long num;
    char* end_ptr;
    size_t save_pos;

    save_pos = scanner->cursor;
    status = scanner_get_token(scanner, &token, &tok_type);
    if (status == (parse_status_t)PARSE_OK) {
        if (tok_type == (token_type_t)TT_IDENTIFIER) {
            errno = 0;
            num = strtol(token.buf, &end_ptr, base);
            /* all and only those chars in token should be used for
             * num */
            if (num < 0 || end_ptr != token.buf + token.length ||
                ((num == LONG_MIN || num == LONG_MAX) && (errno == ERANGE))) {
                status = PARSE_NO_MATCH;
            }
            /* save result */
            *value = (int)num;
        } else {
            /* token must be an identifier */
            status = PARSE_NO_MATCH;
        }
    }
    if (status != (parse_status_t)PARSE_OK) {
        /* restore scanner position for bad values */
        scanner->cursor = save_pos;
    }

    return status;
}

/*!
 * \brief Reads data until end of line.
 *
 * The crlf at the end of line is not consumed. On error, scanner is not
 * restored. On success, **str** points to a string that runs until eol.
 *
 * \returns
 *  On success: PARSE_OK\n
 *  On error:
 *  - PARSE_FAILURE
 *  - PARSE_INCOMPLETE
 */
inline parse_status_t read_until_crlf(
    scanner_t* scanner, ///< [in,out] Scanner Object.
    memptr* str         ///< [out] Buffer to copy scanner buffer contents to.
) {
    memptr token;
    token_type_t tok_type;
    parse_status_t status;
    size_t start_pos;

    start_pos = scanner->cursor;
    str->buf = scanner_get_str(scanner);

    /* read until we hit a crlf */
    do {
        status = scanner_get_token(scanner, &token, &tok_type);
    } while (status == (parse_status_t)PARSE_OK &&
             tok_type != (token_type_t)TT_CRLF);

    if (status == (parse_status_t)PARSE_OK) {
        /* pushback crlf in stream */
        scanner->cursor -= token.length;

        /* str should include all strings except crlf at the end */
        str->length = scanner->cursor - start_pos;
    }

    return status;
}

/*!
 * \brief Compares a character to the next char in the scanner.
 *
 * On error, scanner chars are not restored.
 *
 * \returns
 *  On success: PARSE_OK\n
 *  On error:
 *  - PARSE_NO_MATCH
 *  - PARSE_INCOMPLETE
 */
inline parse_status_t
match_char(scanner_t* scanner, ///< [in,out] Scanner Object.
           char c,             ///< [in] Character to be compared with.
           int case_sensitive  /*!< [in] Flag indicating whether comparison
                                  should be  case sensitive. */
) {
    char scan_char;

    if (scanner->cursor >= scanner->msg->length) {
        return PARSE_INCOMPLETE;
    }
    /* read next char from scanner */
    scan_char = scanner->msg->buf[scanner->cursor++];

    if (case_sensitive) {
        return c == scan_char ? PARSE_OK : PARSE_NO_MATCH;
    } else {
        return tolower(c) == tolower(scan_char) ? PARSE_OK : PARSE_NO_MATCH;
    }
}

/*
 *
 *
 * args for ...
 *   %d,    int *     (31-bit positive integer)
 *   %x,    int *     (31-bit postive number encoded as hex)
 *   %s,    memptr*  (simple identifier)
 *   %q,    memptr*  (quoted string)
 *   %S,    memptr*  (non-whitespace string)
 *   %R,    memptr*  (raw value)
 *   %U,    uri_type* (url)
 *   %L,    memptr*  (string until end of line)
 *   %P,    int * (current index of the string being scanned)
 *
 * no args for
 *   ' '    LWS*
 *   \t     whitespace
 *   "%%"   matches '%'
 *   "% "   matches ' '
 *   %c     matches CRLF
 *   %i     ignore case in literal matching
 *   %n     case-sensitive matching in literals
 *   %w     optional whitespace; (similar to '\t',
 *                  except whitespace is optional)
 *   %0     (zero) match null-terminator char '\0'
 *              (can only be used as last char in fmt)
 *              use only in matchstr(), not match()
 *   other chars match literally
 *
 * returns:
 *   PARSE_OK
 *   PARSE_INCOMPLETE
 *   PARSE_FAILURE      -- bad input
 *   PARSE_NO_MATCH     -- input does not match pattern
 */

/*!
 * \brief Extracts variable parameters depending on the passed in format
 * parameter.
 *
 * Parses data also based on the passed in format parameter.
 *
 * \returns
 *  On success: PARSE_OK\n
 *  On error:
 *  - PARSE_INCOMPLETE
 *  - PARSE_FAILURE - bad input
 *  - PARSE_NO_MATCH - input does not match pattern
 */
parse_status_t vfmatch( //
    scanner_t* scanner, ///< [in,out] Scanner Object.
    const char* fmt,    ///< [in] Pattern Format.
    va_list argp        ///< [in] List of variable arguments.
) {
    char c;
    const char* fmt_ptr = fmt;
    parse_status_t status{};
    memptr* str_ptr;
    memptr temp_str;
    int* int_ptr;
    uri_type* uri_ptr;
    size_t save_pos;
    int stat;
    int case_sensitive = 1;
    memptr token;
    token_type_t tok_type;
    int base;

    assert(scanner != NULL);
    assert(fmt != NULL);

    /* save scanner pos; to aid error recovery */
    save_pos = scanner->cursor;

    status = PARSE_OK;
    while (((c = *fmt_ptr++) != 0) && (status == (parse_status_t)PARSE_OK)) {
        if (c == '%') {
            c = *fmt_ptr++;
            switch (c) {
            case 'R': /* raw value */
                str_ptr = va_arg(argp, memptr*);
                assert(str_ptr != NULL);
                status = match_raw_value(scanner, str_ptr);
                break;
            case 's': /* simple identifier */
                str_ptr = va_arg(argp, memptr*);
                assert(str_ptr != NULL);
                status = scanner_get_token(scanner, str_ptr, &tok_type);
                if (status == (parse_status_t)PARSE_OK &&
                    tok_type != (token_type_t)TT_IDENTIFIER) {
                    /* not an identifier */
                    status = PARSE_NO_MATCH;
                }
                break;
            case 'c': /* crlf */
                status = scanner_get_token(scanner, &token, &tok_type);
                if (status == (parse_status_t)PARSE_OK &&
                    tok_type != (token_type_t)TT_CRLF) {
                    /* not CRLF token */
                    status = PARSE_NO_MATCH;
                }
                break;
            case 'd': /* integer */
            case 'x': /* hex number */
                int_ptr = va_arg(argp, int*);
                assert(int_ptr != NULL);
                base = c == 'd' ? 10 : 16;
                status = match_int(scanner, base, int_ptr);
                break;
            case 'S': /* non-whitespace string */
            case 'U': /* uri */
                if (c == 'S') {
                    str_ptr = va_arg(argp, memptr*);
                } else {
                    str_ptr = &temp_str;
                }
                assert(str_ptr != NULL);
                status = match_non_ws_string(scanner, str_ptr);
                if (c == 'U' && status == (parse_status_t)PARSE_OK) {
                    uri_ptr = va_arg(argp, uri_type*);
                    assert(uri_ptr != NULL);
                    stat = parse_uri(str_ptr->buf, str_ptr->length, uri_ptr);
                    if (stat != HTTP_SUCCESS) {
                        status = PARSE_NO_MATCH;
                    }
                }
                break;
            case 'L': /* string till eol */
                str_ptr = va_arg(argp, memptr*);
                assert(str_ptr != NULL);
                status = read_until_crlf(scanner, str_ptr);
                break;
            case ' ': /* match space */
            case '%': /* match percentage symbol */
                status = match_char(scanner, c, case_sensitive);
                break;
            case 'n': /* case-sensitive match */
                case_sensitive = 1;
                break;
            case 'i': /* ignore case */
                case_sensitive = 0;
                break;
            case 'q': /* quoted string */
                str_ptr = (memptr*)va_arg(argp, memptr*);
                status = scanner_get_token(scanner, str_ptr, &tok_type);
                if (status == (parse_status_t)PARSE_OK &&
                    tok_type != (token_type_t)TT_QUOTEDSTRING) {
                    status = PARSE_NO_MATCH; /* not a quoted
                                    string */
                }
                break;
            case 'w':
                /* optional whitespace */
                status = scanner_get_token(scanner, &token, &tok_type);
                if (status == (parse_status_t)PARSE_OK &&
                    tok_type != (token_type_t)TT_WHITESPACE) {
                    /* restore non-whitespace token */
                    scanner->cursor -= token.length;
                }
                break;
            case 'P':
                /* current pos of scanner */
                int_ptr = va_arg(argp, int*);
                assert(int_ptr != NULL);
                *int_ptr = (int)scanner->cursor;
                break;
                /* valid only in matchstr() */
            case '0':
                /* end of msg? */
                /* check that we are 1 beyond last char */
                if (scanner->cursor == scanner->msg->length &&
                    scanner->msg->buf[scanner->cursor] == '\0') {
                    status = PARSE_OK;
                } else {
                    status = PARSE_NO_MATCH;
                }
                break;
            default:
                /* unknown option */
                assert(0);
            }
        } else {
            switch (c) {
            case ' ': /* LWS* */
                status = skip_lws(scanner);
                break;
            case '\t': /* Whitespace */
                status = scanner_get_token(scanner, &token, &tok_type);
                if (status == (parse_status_t)PARSE_OK &&
                    tok_type != (token_type_t)TT_WHITESPACE) {
                    /* not whitespace token */
                    status = PARSE_NO_MATCH;
                }
                break;
            default: /* match characters */
            {
                status = match_char(scanner, c, case_sensitive);
            }
            }
        }
    }
    if (status != (parse_status_t)PARSE_OK) {
        /* on error, restore original scanner pos */
        scanner->cursor = save_pos;
    }

    return status;
}

/*!
 * \brief Matches a variable parameter list and takes necessary actions based on
 * the data type specified.
 *
 * \returns
 *  On success: PARSE_OK\n
 *  On error:
 *  - PARSE_OK
 *  - PARSE_NO_MATCH
 *  - PARSE_INCOMPLETE
 *  - PARSE_FAILURE - bad input
 */
parse_status_t match(   //
    scanner_t* scanner, ///< [in,out] Scanner Object.
    const char* fmt,    ///< [in] Pattern format (like printf()).
    ...                 ///< [in] Variable arguments (like printf()).
) {
    parse_status_t ret_code;
    va_list args;

    va_start(args, fmt);
    ret_code = vfmatch(scanner, fmt, args);
    va_end(args);

    return ret_code;
}

/*!
 * \brief Initialize and allocate memory for http message.
 */
void httpmsg_init(      //
    http_message_t* msg ///< [in,out] HTTP Message Object
) {
    TRACE("Executing httpmsg_init()")
    msg->entity.buf = nullptr;
    msg->entity.length = (size_t)0;
    ListInit(&msg->headers, httpmsg_compare, httpheader_free);
    membuffer_init(&msg->msg);
    membuffer_init(&msg->status_msg);
    msg->urlbuf = nullptr;
    msg->initialized = 1;
}

/*!
 * \brief Initializes the parser object.
 */
inline void parser_init(  //
    http_parser_t* parser ///< [out] HTTP Parser Object.
) {
    if (parser == nullptr)
        return;

    memset(parser, 0, sizeof(http_parser_t));
    parser->http_error_code = HTTP_BAD_REQUEST; /* err msg by default */
    parser->ent_position = ENTREAD_DETERMINE_READ_METHOD;
    parser->valid_ssdp_notify_hack = 0;

    httpmsg_init(&parser->msg);
    scanner_init(&parser->scanner, &parser->msg.msg);
}

/*!
 * \brief Get HTTP Method, URL location and version information.
 *
 * \returns
 *  On success: PARSE_OK\n
 *  On error:
 *  - PARSE_SUCCESS
 *  - PARSE_FAILURE
 *  - PARSE_INCOMPLETE
 *  - PARSE_NO_MATCH
 */
parse_status_t parser_parse_requestline( //
    http_parser_t* parser                ///< [in,out] HTTP Parser Object.
) {
    parse_status_t status;
    http_message_t* hmsg = &parser->msg;
    memptr method_str;
    memptr version_str;
    size_t index;
    char save_char;
    int num_scanned;
    memptr url_str;

    assert(parser->position == POS_REQUEST_LINE);

    status = skip_blank_lines(&parser->scanner);
    if (status != (parse_status_t)PARSE_OK) {
        return status;
    }
    /* simple get http 0.9 as described in http 1.0 spec */

    status = match(&parser->scanner, "%s\t%S%w%c", &method_str, &url_str);

    UPnPsdk::CStrIntMap http_method_table(Http_Method_Table);
    if (status == (parse_status_t)PARSE_OK) {
        index = http_method_table.index_of(method_str.buf, true);
#if 0
        index =
            map_str_to_int(method_str.buf, method_str.length,
                           &Http_Method_Table[0], Http_Method_Table.size(), 1);
#endif
        if (index == http_method_table.npos) {
            /* error; method not found */
            parser->http_error_code = HTTP_NOT_IMPLEMENTED;
            return PARSE_FAILURE;
        }

        if (Http_Method_Table[index].id != HTTPMETHOD_GET) {
            parser->http_error_code = HTTP_BAD_REQUEST;
            return PARSE_FAILURE;
        }

        hmsg->method = HTTPMETHOD_SIMPLEGET;

        /* remove excessive leading slashes, keep one slash */
        while (url_str.length >= 2 && url_str.buf[0] == '/' &&
               url_str.buf[1] == '/') {
            url_str.buf++;
            url_str.length--;
        }
        /* store url */
        hmsg->urlbuf = str_alloc(url_str.buf, url_str.length);
        if (hmsg->urlbuf == NULL) {
            /* out of mem */
            parser->http_error_code = HTTP_INTERNAL_SERVER_ERROR;
            return PARSE_FAILURE;
        }
        if (parse_uri(hmsg->urlbuf, url_str.length, &hmsg->uri) !=
            HTTP_SUCCESS) {
            return PARSE_FAILURE;
        }

        parser->position = POS_COMPLETE; /* move to headers */

        return PARSE_SUCCESS;
    }

    status = match(&parser->scanner, "%s\t%S\t%ihttp%w/%w%L%c", &method_str,
                   &url_str, &version_str);
    if (status != (parse_status_t)PARSE_OK) {
        return status;
    }
    /* remove excessive leading slashes, keep one slash */
    while (url_str.length >= 2 && url_str.buf[0] == '/' &&
           url_str.buf[1] == '/') {
        url_str.buf++;
        url_str.length--;
    }
    /* store url */
    hmsg->urlbuf = str_alloc(url_str.buf, url_str.length);
    if (hmsg->urlbuf == NULL) {
        /* out of mem */
        parser->http_error_code = HTTP_INTERNAL_SERVER_ERROR;
        return PARSE_FAILURE;
    }
    if (parse_uri(hmsg->urlbuf, url_str.length, &hmsg->uri) != HTTP_SUCCESS) {
        return PARSE_FAILURE;
    }
#if 0
    index = map_str_to_int(method_str.buf, method_str.length, &Http_Method_Table[0],
                           Http_Method_Table.size(), 1);
#endif
    // Valid content of method_str.buf is not terminated with '\0' but only
    // specified by the length.
    const std::string tmpbuf = std::string(method_str.buf, method_str.length);
    index = http_method_table.index_of(tmpbuf.c_str(), true);

    if (index == http_method_table.npos) {
        /* error; method not found */
        parser->http_error_code = HTTP_NOT_IMPLEMENTED;
        return PARSE_FAILURE;
    }

    /* scan version */
    save_char = version_str.buf[version_str.length];
    version_str.buf[version_str.length] = '\0'; /* null-terminate */
#ifdef _WIN32
    num_scanned =
        sscanf_s(version_str.buf,
#else
    num_scanned = sscanf(version_str.buf,
#endif
                 "%d . %d", &hmsg->major_version, &hmsg->minor_version);
    version_str.buf[version_str.length] = save_char; /* restore */
    if (num_scanned != 2 ||
        /* HTTP version equals to 1.0 should fail for MSEARCH as
         * required by the UPnP certification tool */
        hmsg->major_version < 0 ||
        (hmsg->major_version == 1 && hmsg->minor_version < 1 &&
         Http_Method_Table[index].id == HTTPMETHOD_MSEARCH)) {
        parser->http_error_code = HTTP_HTTP_VERSION_NOT_SUPPORTED;
        /* error; bad http version */
        return PARSE_FAILURE;
    }

    hmsg->method = (http_method_t)Http_Method_Table[index].id;
    parser->position = POS_HEADERS; /* move to headers */

    return PARSE_OK;
}

/*!
 * \brief Reads entity using content-length.
 *
 * \returns
 *  On success: PARSE_SUCCESS\n
 *  On error: PARSE_INCOMPLETE
 */
inline parse_status_t parser_parse_entity_using_clen(
    http_parser_t* parser ///< [in,out] HTTP Parser Object.
) {
    /*int entity_length; */

    assert(parser->ent_position == ENTREAD_USING_CLEN);

    /* determine entity (i.e. body) length so far */
    parser->msg.entity.length = parser->msg.msg.length -
                                parser->entity_start_position +
                                parser->msg.amount_discarded;

    if (parser->msg.entity.length < parser->content_length) {
        /* more data to be read */
        return PARSE_INCOMPLETE;
    } else {
        if (parser->msg.entity.length > parser->content_length) {
            /* silently discard extra data */
            parser->msg.msg
                .buf[parser->entity_start_position + parser->content_length -
                     parser->msg.amount_discarded] = '\0';
        }
        /* save entity length */
        parser->msg.entity.length = parser->content_length;

        /* save entity start ptr; (the very last thing to do) */
        parser->msg.entity.buf =
            parser->msg.msg.buf + parser->entity_start_position;

        /* done reading entity */
        parser->position = POS_COMPLETE;
        return PARSE_SUCCESS;
    }
}

/*!
 * \brief Read data in the chunks.
 *
 * \returns
 *  On success: PARSE_CONTINUE_1\n
 *  On error:
 *  - PARSE_INCOMPLETE
 *  - PARSE_FAILURE
 *  - PARSE_NO_MATCH
 */
inline parse_status_t
parser_parse_chunky_body(http_parser_t* parser ///< [in,out] HTTP Parser Object.
) {
    parse_status_t status;
    size_t save_pos;

    /* if 'chunk_size' of bytes have been read; read next chunk */
    if ((parser->msg.msg.length - parser->scanner.cursor) >=
        parser->chunk_size) {
        /* move to next chunk */
        parser->scanner.cursor += parser->chunk_size;
        save_pos = parser->scanner.cursor;
        /* discard CRLF */
        status = match(&parser->scanner, "%c");
        if (status != (parse_status_t)PARSE_OK) {
            /*move back */
            parser->scanner.cursor -= parser->chunk_size;
            /*parser->scanner.cursor = save_pos; */
            return status;
        }
        membuffer_delete(&parser->msg.msg, save_pos,
                         (parser->scanner.cursor - save_pos));
        parser->scanner.cursor = save_pos;
        /*update temp  */
        parser->msg.entity.length += parser->chunk_size;
        parser->ent_position = ENTREAD_USING_CHUNKED;
        return PARSE_CONTINUE_1;
    } else
        /* need more data for chunk */
        return PARSE_INCOMPLETE;
}

/*!
 * \brief Read headers at the end of the chunked entity.
 *
 * \returns
 *  On success: PARSE_SUCCESS\n
 *  On error:
 *  - PARSE_NO_MATCH
 *  - PARSE_INCOMPLETE
 *  - PARSE_FAILURE
 */
inline parse_status_t parser_parse_chunky_headers(
    http_parser_t* parser ///< [in,out] HTTP Parser Object.
) {
    parse_status_t status;
    size_t save_pos;

    save_pos = parser->scanner.cursor;
    status = parser_parse_headers(parser);
    if (status == (parse_status_t)PARSE_OK) {
        /* finally, done with the whole msg */
        parser->position = POS_COMPLETE;

        membuffer_delete(&parser->msg.msg, save_pos,
                         (parser->scanner.cursor - save_pos));
        parser->scanner.cursor = save_pos;

        /* save entity start ptr as the very last thing to do */
        parser->msg.entity.buf =
            parser->msg.msg.buf + parser->entity_start_position;

        return PARSE_SUCCESS;
    } else {
        return status;
    }
}

/*!
 * \brief Read entity using chunked transfer encoding.
 *
 * \returns
 *  On success: PARSE_CONTINUE_1\n
 *  On error:
 *  - PARSE_INCOMPLETE
 *  - PARSE_FAILURE
 *  - PARSE_NO_MATCH
 */
inline parse_status_t parser_parse_chunky_entity(
    http_parser_t* parser ///< [in,out] HTTP Parser Object.
) {
    scanner_t* scanner = &parser->scanner;
    parse_status_t status;
    size_t save_pos;
    memptr dummy;

    assert(parser->ent_position == ENTREAD_USING_CHUNKED);

    save_pos = scanner->cursor;

    /* get size of chunk, discard extension, discard CRLF */
    status = match(scanner, "%x%L%c", &parser->chunk_size, &dummy);
    if (status != (parse_status_t)PARSE_OK) {
        scanner->cursor = save_pos;
        UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
                   "CHUNK COULD NOT BE PARSED\n");
        return status;
    }
    /* remove chunk info just matched; just retain data */
    membuffer_delete(&parser->msg.msg, save_pos, (scanner->cursor - save_pos));
    scanner->cursor = save_pos; /* adjust scanner too */

    if (parser->chunk_size == (size_t)0) {
        /* done reading entity; determine length of entity */
        parser->msg.entity.length = parser->scanner.cursor -
                                    parser->entity_start_position +
                                    parser->msg.amount_discarded;

        /* read entity headers */
        parser->ent_position = ENTREAD_CHUNKY_HEADERS;
    } else {
        /* read chunk body */
        parser->ent_position = ENTREAD_CHUNKY_BODY;
    }

    return PARSE_CONTINUE_1; /* continue to reading body */
}

/*!
 * \brief Keep reading entity until the connection is closed.
 *
 * \returns always PARSE_INCOMPLETE_ENTITY
 */
inline parse_status_t parser_parse_entity_until_close(
    http_parser_t* parser ///< [in,out] HTTP Parser Object.
) {
    size_t cursor;

    assert(parser->ent_position == ENTREAD_UNTIL_CLOSE);

    /* eat any and all data */
    cursor = parser->msg.msg.length;

    /* update entity length */
    parser->msg.entity.length =
        cursor - parser->entity_start_position + parser->msg.amount_discarded;

    /* update pointer */
    parser->msg.entity.buf =
        parser->msg.msg.buf + parser->entity_start_position;

    parser->scanner.cursor = cursor;

    return PARSE_INCOMPLETE_ENTITY; /* add anything */
}

} // anonymous namespace


void httpmsg_destroy(http_message_t* msg) {
    assert(msg != NULL);

    if (msg->initialized == 1) {
        ListDestroy(&msg->headers, 1);
        membuffer_destroy(&msg->msg);
        membuffer_destroy(&msg->status_msg);
        free(msg->urlbuf);
        msg->initialized = 0;
    }
}


http_header_t* httpmsg_find_hdr_str(http_message_t* msg,
                                    const char* header_name) {
    http_header_t* header;

    ListNode* node;

    node = ListHead(&msg->headers);
    while (node != NULL) {

        header = (http_header_t*)node->item;

        if (memptr_cmp_nocase(&header->name, header_name) == 0) {
            return header;
        }

        node = ListNext(&msg->headers, node);
    }
    return NULL;
}


http_header_t* httpmsg_find_hdr(http_message_t* msg, int header_name_id,
                                memptr* value) {
    http_header_t header; /* temp header for searching */
    ListNode* node;
    http_header_t* data;

    header.name_id = header_name_id;
    node = ListFind(&msg->headers, NULL, &header);
    if (node == NULL) {
        return NULL;
    }
    data = (http_header_t*)node->item;
    if (value != NULL) {
        value->buf = data->value.buf;
        value->length = data->value.length;
    }

    return data;
}


parse_status_t matchstr(char* str, size_t slen, const char* fmt, ...) {
    parse_status_t ret_code;
    char save_char;
    scanner_t scanner;
    membuffer buf;
    va_list arg_list;

    /* null terminate str */
    save_char = str[slen];
    str[slen] = '\0';

    membuffer_init(&buf);

    /* under no circumstances should this buffer be modifed because its
     * memory */
    /*  might have not come from malloc() */
    membuffer_attach(&buf, str, slen);

    scanner_init(&scanner, &buf);
    scanner.entire_msg_loaded = 1;

    va_start(arg_list, fmt);
    ret_code = vfmatch(&scanner, fmt, arg_list);
    va_end(arg_list);

    /* restore str */
    str[slen] = save_char;

    /* don't destroy buf */

    return ret_code;
}


parse_status_t parser_parse_responseline(http_parser_t* parser) {
    parse_status_t status;
    http_message_t* hmsg = &parser->msg;
    memptr line;
    char save_char;
    int num_scanned;
    int i;
    size_t n;
    char* p;

    assert(parser->position == POS_RESPONSE_LINE);

    status = skip_blank_lines(&parser->scanner);
    if (status != (parse_status_t)PARSE_OK)
        return status;
    /* response line */
    /*status = match( &parser->scanner, "%ihttp%w/%w%d\t.\t%d\t%d\t%L%c", */
    /*  &hmsg->major_version, &hmsg->minor_version, */
    /*  &hmsg->status_code, &hmsg->status_msg ); */
    status = match(&parser->scanner, "%ihttp%w/%w%L%c", &line);
    if (status != (parse_status_t)PARSE_OK)
        return status;
    save_char = line.buf[line.length];
    line.buf[line.length] = '\0'; /* null-terminate */
    /* scan http version and ret code */
#ifdef _WIN32
    num_scanned = sscanf_s(line.buf,
#else
    num_scanned = sscanf(line.buf,
#endif
                           "%d . %d %d", &hmsg->major_version,
                           &hmsg->minor_version, &hmsg->status_code);
    line.buf[line.length] = save_char; /* restore */
    if (num_scanned != 3 || hmsg->major_version < 0 ||
        hmsg->minor_version < 0 || hmsg->status_code < 0)
        /* bad response line */
        return PARSE_FAILURE;
    /* point to status msg */
    p = line.buf;
    /* skip 3 ints */
    for (i = 0; i < 3; i++) {
        /* go to start of num */
        while (!isdigit(*p))
            p++;
        /* skip int */
        while (isdigit(*p))
            p++;
    }
    /* whitespace must exist after status code */
    if (*p != ' ' && *p != '\t')
        return PARSE_FAILURE;
    /* skip whitespace */
    while (*p == ' ' || *p == '\t')
        p++;
    /* now, p is at start of status msg */
    n = line.length - ((size_t)p - (size_t)line.buf);
    if (membuffer_assign(&hmsg->status_msg, p, n) != 0) {
        /* out of mem */
        parser->http_error_code = HTTP_INTERNAL_SERVER_ERROR;
        return PARSE_FAILURE;
    }
    parser->position = POS_HEADERS; /* move to headers */

    return PARSE_OK;
}


parse_status_t parser_parse_headers(http_parser_t* parser) {
    parse_status_t status;
    memptr token;
    memptr hdr_value;
    token_type_t tok_type;
    scanner_t* scanner = &parser->scanner;
    size_t save_pos;
    http_header_t* header;
    int header_id;
    int ret = 0;
    size_t index;
    http_header_t* orig_header;
    char save_char;
    int ret2;
    static char zero = 0;

    assert(parser->position == (parser_pos_t)POS_HEADERS ||
           parser->ent_position == ENTREAD_CHUNKY_HEADERS);

    while (1) {
        save_pos = scanner->cursor;
        /* check end of headers */
        status = scanner_get_token(scanner, &token, &tok_type);
        if (status != (parse_status_t)PARSE_OK) {
            /* pushback tokens; useful only on INCOMPLETE error */
            scanner->cursor = save_pos;
            return status;
        }
        switch (tok_type) {
        case TT_CRLF:
            /* end of headers */
            if ((parser->msg.is_request) &&
                (parser->msg.method == (http_method_t)HTTPMETHOD_POST)) {
                parser->position = POS_COMPLETE; /*post entity parsing */
                /*is handled separately  */
                return PARSE_SUCCESS;
            }
            parser->position = POS_ENTITY; /* read entity next */
            return PARSE_OK;
        case TT_IDENTIFIER:
            /* not end; read header */
            break;
        default:
            return PARSE_FAILURE; /* didn't see header name */
        }
        status = match(scanner, " : %R%c", &hdr_value);
        if (status != (parse_status_t)PARSE_OK) {
            /* pushback tokens; useful only on INCOMPLETE error */
            scanner->cursor = save_pos;
            return status;
        }
        /* add header */
        /* find header */

        UPnPsdk::CStrIntMap http_header_names_table(Http_Header_Names);
        index = http_header_names_table.index_of(
            std::string(token.buf, token.length).c_str());
        if (index != http_header_names_table.npos) {
            /*Check if it is a soap header */
            if (Http_Header_Names[index].id == HDR_SOAPACTION) {
                parser->msg.method = SOAPMETHOD_POST;
            }
            header_id = Http_Header_Names[index].id;
            orig_header = httpmsg_find_hdr(&parser->msg, header_id, NULL);
        } else {
            header_id = HDR_UNKNOWN;
            save_char = token.buf[token.length];
            token.buf[token.length] = '\0';
            orig_header = httpmsg_find_hdr_str(&parser->msg, token.buf);
            token.buf[token.length] = save_char; /* restore */
        }
        if (orig_header == NULL) {
            /* add new header */
            header = (http_header_t*)malloc(sizeof(http_header_t));
            if (header == NULL) {
                parser->http_error_code = HTTP_INTERNAL_SERVER_ERROR;
                return PARSE_FAILURE;
            }
            membuffer_init(&header->name_buf);
            membuffer_init(&header->value);
            /* value can be 0 length */
            if (hdr_value.length == (size_t)0) {
                hdr_value.buf = &zero;
                hdr_value.length = (size_t)1;
            }
            /* save in header in buffers */
            if (membuffer_assign(&header->name_buf, token.buf, token.length) ||
                membuffer_assign(&header->value, hdr_value.buf,
                                 hdr_value.length)) {
                /* not enough mem */
                membuffer_destroy(&header->value);
                membuffer_destroy(&header->name_buf);
                free(header);
                parser->http_error_code = HTTP_INTERNAL_SERVER_ERROR;
                return PARSE_FAILURE;
            }
            header->name.buf = header->name_buf.buf;
            header->name.length = header->name_buf.length;
            header->name_id = header_id;
            if (!ListAddTail(&parser->msg.headers, header)) {
                membuffer_destroy(&header->value);
                membuffer_destroy(&header->name_buf);
                free(header);
                parser->http_error_code = HTTP_INTERNAL_SERVER_ERROR;
                return PARSE_FAILURE;
            }
        } else if (hdr_value.length > (size_t)0) {
            /* append value to existing header */
            /* append space */
            ret = membuffer_append_str(&orig_header->value, ", ");
            /* append continuation of header value */
            ret2 = membuffer_append(&orig_header->value, hdr_value.buf,
                                    hdr_value.length);
            if (ret == UPNP_E_OUTOF_MEMORY || ret2 == UPNP_E_OUTOF_MEMORY) {
                /* not enuf mem */
                parser->http_error_code = HTTP_INTERNAL_SERVER_ERROR;
                return PARSE_FAILURE;
            }
        }
    } /* end while */
}


parse_status_t parser_get_entity_read_method(http_parser_t* parser) {
    http_message_t* hmsg = &parser->msg;
    int response_code;
    memptr hdr_value;

    assert(parser->ent_position == ENTREAD_DETERMINE_READ_METHOD);

    /* entity points to start of msg body */
    parser->msg.entity.buf = scanner_get_str(&parser->scanner);
    parser->msg.entity.length = (size_t)0;

    /* remember start of body */
    parser->entity_start_position = parser->scanner.cursor;

    /* std http rules for determining content length */

    /* * no body for 1xx, 204, 304 and HEAD, GET, */
    /*      SUBSCRIBE, UNSUBSCRIBE */
    if (hmsg->is_request) {
        switch (hmsg->method) {
        case HTTPMETHOD_HEAD:
        case HTTPMETHOD_GET:
            /*case HTTPMETHOD_POST: */
        case HTTPMETHOD_SUBSCRIBE:
        case HTTPMETHOD_UNSUBSCRIBE:
        case HTTPMETHOD_MSEARCH:
            /* no body; mark as done */
            parser->position = POS_COMPLETE;
            return PARSE_SUCCESS;
            break;

        default:; /* do nothing */
        }
    } else        /* response */
    {
        response_code = hmsg->status_code;

        if (response_code == 204 || response_code == 304 ||
            (response_code >= 100 && response_code <= 199) ||
            hmsg->request_method == (http_method_t)HTTPMETHOD_HEAD ||
            hmsg->request_method == (http_method_t)HTTPMETHOD_MSEARCH ||
            hmsg->request_method == (http_method_t)HTTPMETHOD_SUBSCRIBE ||
            hmsg->request_method == (http_method_t)HTTPMETHOD_UNSUBSCRIBE ||
            hmsg->request_method == (http_method_t)HTTPMETHOD_NOTIFY) {
            parser->position = POS_COMPLETE;
            return PARSE_SUCCESS;
        }
    }

    /* * transfer-encoding -- used to indicate chunked data */
    if (httpmsg_find_hdr(hmsg, HDR_TRANSFER_ENCODING, &hdr_value)) {
        if (raw_find_str(&hdr_value, "chunked") >= 0) {
            /* read method to use chunked transfer encoding */
            parser->ent_position = ENTREAD_USING_CHUNKED;
            UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
                       "Found Chunked Encoding ....\n");

            return PARSE_CONTINUE_1;
        }
    }
    /* * use content length */
    if (httpmsg_find_hdr(hmsg, HDR_CONTENT_LENGTH, &hdr_value)) {
        parser->content_length = (unsigned int)raw_to_int(&hdr_value, 10);
        parser->ent_position = ENTREAD_USING_CLEN;
        return PARSE_CONTINUE_1;
    }
    /* * multi-part/byteranges not supported (yet) */

    /* * read until connection is closed */
    if (hmsg->is_request) {
        /* set hack flag for NOTIFY methods; if set to 1 this is */
        /*  a valid SSDP notify msg */
        if (hmsg->method == (http_method_t)HTTPMETHOD_NOTIFY) {
            parser->valid_ssdp_notify_hack = 1;
        }

        parser->http_error_code = HTTP_LENGTH_REQUIRED;
        return PARSE_FAILURE;
    }

    parser->ent_position = ENTREAD_UNTIL_CLOSE;
    return PARSE_CONTINUE_1;
}


parse_status_t parser_parse_entity(http_parser_t* parser) {
    parse_status_t status;

    assert(parser->position == POS_ENTITY);

    do {
        switch (parser->ent_position) {
        case ENTREAD_USING_CLEN:
            status = parser_parse_entity_using_clen(parser);
            break;

        case ENTREAD_USING_CHUNKED:
            status = parser_parse_chunky_entity(parser);
            break;

        case ENTREAD_CHUNKY_BODY:
            status = parser_parse_chunky_body(parser);
            break;

        case ENTREAD_CHUNKY_HEADERS:
            status = parser_parse_chunky_headers(parser);
            break;

        case ENTREAD_UNTIL_CLOSE:
            status = parser_parse_entity_until_close(parser);
            break;

        case ENTREAD_DETERMINE_READ_METHOD:
            status = parser_get_entity_read_method(parser);
            break;

        default:
            status = PARSE_FAILURE;
            assert(0);
        }

    } while (status == (parse_status_t)PARSE_CONTINUE_1);

    return status;
}


void parser_request_init(http_parser_t* parser) {
    parser_init(parser);
    parser->msg.is_request = 1;
    parser->position = POS_REQUEST_LINE;
}


void parser_response_init(http_parser_t* parser, http_method_t request_method) {
    parser_init(parser);
    parser->msg.is_request = 0;
    parser->msg.request_method = request_method;
    parser->msg.amount_discarded = (size_t)0;
    parser->position = POS_RESPONSE_LINE;
}


parse_status_t parser_parse(http_parser_t* parser) {
    parse_status_t status;

    /*takes an http_parser_t with memory already allocated  */
    /*in the message  */
    assert(parser != nullptr);

    do {
        switch (parser->position) {
        case POS_ENTITY:
            status = parser_parse_entity(parser);

            break;

        case POS_HEADERS:
            status = parser_parse_headers(parser);

            break;

        case POS_REQUEST_LINE:
            status = parser_parse_requestline(parser);

            break;

        case POS_RESPONSE_LINE:
            status = parser_parse_responseline(parser);

            break;

        default: {
            status = PARSE_FAILURE;
            assert(0);
        }
        }

    } while (status == (parse_status_t)PARSE_OK);

    return status;
}


parse_status_t parser_append(http_parser_t* parser, const char* buf,
                             size_t buf_length) {
    assert(parser != nullptr);
    assert(buf != nullptr);

    /* append data to buffer */
    int ret_code = membuffer_append(&parser->msg.msg, buf, buf_length);
    if (ret_code != 0) {
        /* set failure status */
        parser->http_error_code = HTTP_INTERNAL_SERVER_ERROR;
        return PARSE_FAILURE;
    }

    return parser_parse(parser);
}


int raw_to_int(memptr* raw_value, int base) {
    long num;
    char* end_ptr;

    if (raw_value->length == (size_t)0)
        return -1;
    errno = 0;
    num = strtol(raw_value->buf, &end_ptr, base);
    if ((num < 0)
        /* all and only those chars in token should be used for num */
        || (end_ptr != raw_value->buf + raw_value->length) ||
        ((num == LONG_MIN || num == LONG_MAX) && (errno == ERANGE))) {
        return -1;
    }
    return (int)num;
}


int raw_find_str(memptr* raw_value, const char* str) {
    char c;
    char* ptr;
    int i = 0;

    /* save */
    c = raw_value->buf[raw_value->length];

    /* Make it lowercase */
    for (i = 0; raw_value->buf[i]; ++i) {
        raw_value->buf[i] = (char)tolower(raw_value->buf[i]);
    }

    /* null-terminate */
    raw_value->buf[raw_value->length] = 0;

    /* Find the substring position */
    ptr = strstr(raw_value->buf, str);

    /* restore the "length" byte */
    raw_value->buf[raw_value->length] = c;

    if (ptr == 0) {
        return -1;
    }

    /* return index */
    return (int)(ptr - raw_value->buf);
}

const char* method_to_str(http_method_t method) {
#if 0
    int index =
        map_int_to_str(method, &Http_Method_Table[0], Http_Method_Table.size());
#endif
    UPnPsdk::CStrIntMap http_method_table(Http_Method_Table);
    size_t index = http_method_table.index_of(method);

    assert(index != http_method_table.npos);

    return index == http_method_table.npos ? NULL
                                           : Http_Method_Table[index].name;
}

void print_http_headers(std::string_view log_msg, http_message_t* hmsg) {
    if (!UPnPsdk::g_dbug)
        return;
    UPnPsdk_LOGINFO(log_msg) "...\n";

    ListNode* node;
    /* NNS:  dlist_node *node; */
    http_header_t* header;

    /* print start line */
    if (hmsg->is_request) {
        std::cerr << "    method=" << hmsg->method
                  << ", version=" << hmsg->major_version << "."
                  << hmsg->minor_version << ", url=\""
                  << std::string(hmsg->uri.pathquery.buff,
                                 hmsg->uri.pathquery.size)
                  << "\".\n";
    } else {
        std::cerr << "    resp_status=" << hmsg->status_code
                  << ", version=" << hmsg->major_version << "."
                  << hmsg->minor_version << ", status_msg=\""
                  << std::string(hmsg->status_msg.buf, hmsg->status_msg.length)
                  << "\".\n";
    }

    /* print headers */
    node = ListHead(&hmsg->headers);
    /* NNS: node = dlist_first_node( &hmsg->headers ); */
    while (node != NULL) {
        header = (http_header_t*)node->item;
        /* NNS: header = (http_header_t *)node->data; */
        std::cerr << "    name_id=" << header->name_id << ", hdr_name=\""
                  << std::string(header->name.buf, header->name.length)
                  << "\", value=\""
                  << std::string(header->value.buf, header->value.length)
                  << "\".\n";

        node = ListNext(&hmsg->headers, node);
        /* NNS: node = dlist_next( &hmsg->headers, node ); */
    }
}
