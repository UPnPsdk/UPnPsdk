#ifndef UPnPsdk_PTHREAD_HPP
#define UPnPsdk_PTHREAD_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-11

/*!
 * \file
 * \brief Additional functions to manage Posix threads with C++ to be portable
 */

#include <UPnPsdk/visibility.hpp> // needed for some platforms
/// \cond
#include "pthread.h" // To find pthreads4w don't use <pthread.h>
#include <cstdint>
/// \endcond

namespace UPnPsdk {
/// \cond
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


#if defined(_MSC_VER) || defined(__APPLE__) || defined(DOXYGEN_RUN)
/*!
 * \brief Get pthread thread id as unsigned integer
 *
 * Actually ::%pthread_self() returns pthread_t and not an integer thread id you
 * can work with. The following helper function will get you that in a portable
 * way across different POSIX systems.\n
 * Reference: <!--REF:--><a
 * href="https://stackoverflow.com/a/18709692/5014688">How do I get a thread ID
 * from an arbitrary pthread_t?</a>
 * \note This function is only available for macOS and Microsoft Windows.
 * Unix/Linux with GCC compiler can do this authomatically.
 */
UPnPsdk_VIS uint64_t pthread_self();
#endif


/*!
 * \brief Scoped POSIX thread mutex lock is valid for the current scope of the
 * object
 *
 * The constructor locks the given mutex and the destructor unlocks it. This way
 * the locked mutex is authomatically unlocked when leaving the current scope,
 * for example on return or with an exception.
 *
 * \exception std::runtime_error The mutex has not been properly initialized.
 */
class CPthread_scoped_lock {
  public:
    /*! \brief Locks given POSIX thread mutex. */
    CPthread_scoped_lock(
        ::pthread_mutex_t& a_mutex /*!< [in] Mutex used to lock. */);
    /*! \brief Unlock the mutex that was locked by the constructor */
    ~CPthread_scoped_lock();

  private:
    ::pthread_mutex_t& m_mutex;
};

} // namespace UPnPsdk

#endif /* UPnPsdk_PTHREAD_HPP */
