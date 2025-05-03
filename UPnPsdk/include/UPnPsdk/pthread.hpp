#ifndef UPnPsdk_PTHREAD_HPP
#define UPnPsdk_PTHREAD_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-05-01

/*!
 * \file
 * \brief Additional functions to manage Posix threads with C++ and for
 * Microsoft Windows.
 *
 * For internal use Only.
 */

#include "pthread.h" // To find pthreads4w don't use <pthread.h>

/// \cond

namespace UPnPsdk {
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


/****************************************************************************
 * Name: start_routine
 *
 *  Description:
 *      Thread start routine
 *      Internal Use Only.
 ***************************************************************************/
typedef void (*start_routine)(void* arg);

/// \endcond

class CPthread_scoped_mutex_lock {
    //
};

} // namespace UPnPsdk

#endif /* UPnPsdk_PTHREAD_HPP */
