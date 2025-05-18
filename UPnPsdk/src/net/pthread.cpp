// Copyright (C) 2025+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-05-19

/*!
 * \file
 * \brief Additional functions to manage Posix threads with C++ to be portable
 */

#include <UPnPsdk/pthread.hpp>
#include <UPnPsdk/synclog.hpp>

/// \cond
#include <algorithm> // for std::min()
/// \endcond

namespace UPnPsdk {

#if defined(_MSC_VER) || defined(__APPLE__) || defined(DOXYGEN_RUN)
// Special converter for POSIX thread id from pthread_t to uint64_t for output.
uint64_t pthread_self() {
    pthread_t ptid = ::pthread_self();
    uint64_t threadId = 0;
    memcpy(&threadId, &ptid, std::min(sizeof(threadId), sizeof(ptid)));
    return threadId;
}
#endif


CPthread_scoped_lock::CPthread_scoped_lock(::pthread_mutex_t& a_mutex)
    : m_mutex(a_mutex) {
    TRACE2(this, " Construct CPthread_scoped_lock (lock pthread mutex)")
    if (::pthread_mutex_lock(&m_mutex) != 0)
        throw std::runtime_error(UPnPsdk_LOGEXCEPT(
            "MSG1152") "The mutex has not been properly initialized.\n");
}

CPthread_scoped_lock::~CPthread_scoped_lock() {
    TRACE2(this, " Destruct CPthread_scoped_lock (unlock pthread mutex)")
    if (::pthread_mutex_unlock(&m_mutex) != 0)
        UPnPsdk_LOGCRIT(
            "MSG1153") "The mutex has not been properly initialized.\n";
}

} // namespace UPnPsdk
