#ifndef UPnPsdk_PTHREAD_HPP
#define UPnPsdk_PTHREAD_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-04-30

/*!
 * \file
 * \brief Additional functions to manage Posix threads with C++ and for
 * Microsoft Windows.
 *
 * For internal use Only.
 */

#include "pthread.h" // To find pthreads4w don't use <pthread.h>

/// \cond

#if defined(BSD) && !defined(__GNU__)
#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#endif

#if defined(PTHREAD_MUTEX_RECURSIVE) || defined(__DragonFly__)
/* This system has SuS2-compliant mutex attributes.
 * E.g. on Cygwin, where we don't have the old nonportable (NP) symbols
 */
#define ITHREAD_MUTEX_FAST_NP PTHREAD_MUTEX_NORMAL
#define ITHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#define ITHREAD_MUTEX_ERRORCHECK_NP PTHREAD_MUTEX_ERRORCHECK
#else /* PTHREAD_MUTEX_RECURSIVE */
#define ITHREAD_MUTEX_FAST_NP PTHREAD_MUTEX_FAST_NP
#define ITHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE_NP
#define ITHREAD_MUTEX_ERRORCHECK_NP PTHREAD_MUTEX_ERRORCHECK_NP
#endif /* PTHREAD_MUTEX_RECURSIVE */

#define ITHREAD_PROCESS_PRIVATE PTHREAD_PROCESS_PRIVATE
#define ITHREAD_PROCESS_SHARED PTHREAD_PROCESS_SHARED

#define ITHREAD_CANCELED PTHREAD_CANCELED

#define ITHREAD_STACK_MIN PTHREAD_STACK_MIN
#define ITHREAD_CREATE_DETACHED PTHREAD_CREATE_DETACHED
#define ITHREAD_CREATE_JOINABLE PTHREAD_CREATE_JOINABLE

namespace UPnPsdk {

/****************************************************************************
 * Name: start_routine
 *
 *  Description:
 *      Thread start routine
 *      Internal Use Only.
 ***************************************************************************/
typedef void (*start_routine)(void* arg);

namespace {
/****************************************************************************
 * Function: initialize_thread
 *
 *  Description:
 *      Initializes the thread. Does nothing in all implementations, except
 *      when statically linked for WIN32. For details have a look at
 *      build/_deps/pthreads4w-src/README.NONPORTABLE.
 *  Parameters:
 *      none.
 *  Returns:
 *      always 0 (pthread_win32_thread_attach_np() always returns true).
 ***************************************************************************/
inline int initialize_thread() {
#if defined(_WIN32) && defined(PTW32_STATIC_LIB)
    return !pthread_win32_thread_attach_np();
#else
    return 0;
#endif
}

/****************************************************************************
 * Function: cleanup_thread
 *
 *  Description:
 *      Clean up thread resources. Does nothing in all implementations, except
 *      when statically linked for WIN32. For details have a look at
 *      build/_deps/pthreads4w-src/README.NONPORTABLE.
 *  Parameters:
 *      none.
 *  Returns:
 *      always 0 (pthread_win32_thread_detach_np() always returns true).
 ***************************************************************************/
inline int cleanup_thread() {
#if defined(_WIN32) && defined(PTW32_STATIC_LIB)
    return !pthread_win32_thread_detach_np();
#else
    return 0;
#endif
}

} // namespace
} // namespace UPnPsdk

/// \endcond

#endif /* UPnPsdk_PTHREAD_HPP */
