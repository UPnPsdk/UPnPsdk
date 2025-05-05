// Copyright (C) 2025+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-05-05

/*!
 * \file
 * \brief Additional functions to manage Posix threads with C++ to be portable
 */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(DOXYGEN_RUN)
#include <UPnPsdk/pthread.hpp>
/// \cond
#include <algorithm> // for std::min()
/// \endcond

namespace UPnPsdk {

// Special converter for POSIX thread id from pthread_t to uint64_t for output.
uint64_t pthread_self() {
    pthread_t ptid = ::pthread_self();
    uint64_t threadId = 0;
    memcpy(&threadId, &ptid, std::min(sizeof(threadId), sizeof(ptid)));
    return threadId;
}

} // namespace UPnPsdk
#endif
