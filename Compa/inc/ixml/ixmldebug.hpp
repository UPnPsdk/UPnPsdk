#ifndef UPNPLIB_IXMLDEBUG_HPP
#define UPNPLIB_IXMLDEBUG_HPP
// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-05-29
// Taken from authors who haven't made a note.
// Last compare with pupnp original source file on 2025-05-23, ver 1.14.20

#include <ixml/ixml.hpp>

/*!
 * \file
 *
 * \brief Auxiliar routines to aid debugging.
 */

/*!
 * \brief Prints the debug statement either on the standard output or log file
 * along with the information from where this debug statement is coming.
 */
#ifdef DEBUG
void IxmlPrintf(
    /*! [in] The file name, usually __FILE__. */
    const char* DbgFileName,
    /*! [in] The line number, usually __LINE__ or a variable that got the
     * __LINE__ at the appropriate place. */
    int DbgLineNo,
    /*! [in] The function name. */
    const char* FunctionName,
    /*! [in] Printf like format specification. */
    const char* FmtStr,
    /*! [in] Printf like Variable number of arguments that will go in the debug
     * statement. */
    ...)
#if (__GNUC__ >= 3)
    /* This enables printf like format checking by the compiler */
    __attribute__((format(__printf__, 4, 5)))
#endif
    ;
#else  /* DEBUG */
static inline void IxmlPrintf([[maybe_unused]] const char* FmtStr, ...) {}
#endif /* DEBUG */

/*!
 * \brief Print the node names and values of a XML tree.
 */
#ifdef DEBUG
void printNodes(
    /*! [in] The root of the tree to print. */
    IXML_Node* tmpRoot,
    /*! [in] The depth to print. */
    int depth);
#else
static inline void printNodes([[maybe_unused]] IXML_Node* tmpRoot,
                              [[maybe_unused]] int depth) {}
#endif

#endif /* UPNPLIB_IXMLDEBUG_HPP */
