/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (c) 2012 France Telecom All rights reserved.
 * Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
 * Redistribution only with this Copyright remark. Last modified: 2025-05-01
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
// Last compare with ./Pupnp source file on 2024-10-26, ver 1.14.20
/*!
 * \file
 * \ingroup threadutil
 * \brief Manage a threadpool (for internal use only).
 *
 * Because this is for internal use, parameters are NOT checked for validity.
 * The caller must ensure valid parameters.
 */

#include <ThreadPool.hpp>

#include <UPnPsdk/synclog.hpp>

/// \cond
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring> /* for memset()*/
/// \endcond

/*! Size of job free list. */
constexpr int JOBFREELISTSIZE{100};
/*! Infinite threads. */
constexpr int INFINITE_THREADS{-1};
/*! Error: maximun threads. */
#define EMAXTHREADS (-8 & 1 << 29)
/*! Invalid Policy */
#define INVALID_POLICY (-9 & 1 << 29)


namespace {
/*! \name Scope restricted to file
 * @{
 */

/*!
 * \brief Returns the difference in milliseconds between two timeval structures.
 *
 * \returns The difference in milliseconds, time1-time2.
 */
long DiffMillis(timeval* time1, timeval* time2) {
    double temp = 0.0;

    temp = static_cast<double>(time1->tv_sec - time2->tv_sec);
    /* convert to milliseconds */
    temp *= 1000.0;

    /* convert microseconds to milliseconds and add to temp */
    /* implicit flooring of unsigned long data type */
    temp += static_cast<double>(time1->tv_usec - time2->tv_usec) / 1000.0;

    return static_cast<long>(temp);
}

#if defined(STATS) || defined(DOXYGEN_RUN)
/*!
 * \brief Initializes the statistics structure.
 */
void StatsInit(
    /*! Valid non null stats structure. */
    ThreadPoolStats* stats) {
    stats->totalIdleTime = 0.0;
    stats->totalJobsHQ = 0;
    stats->totalJobsLQ = 0;
    stats->totalJobsMQ = 0;
    stats->totalTimeHQ = 0.0;
    stats->totalTimeMQ = 0.0;
    stats->totalTimeLQ = 0.0;
    stats->totalWorkTime = 0.0;
    stats->totalIdleTime = 0.0;
    stats->avgWaitHQ = 0.0;
    stats->avgWaitMQ = 0.0;
    stats->avgWaitLQ = 0.0;
    stats->workerThreads = 0;
    stats->idleThreads = 0;
    stats->persistentThreads = 0;
    stats->maxThreads = 0;
    stats->totalThreads = 0;
}

/*!
 * \brief StatsAccountLQ
 */
void StatsAccountLQ(
    /*! [in] Valid, non null, pointer to ThreadPool. */
    ThreadPool* tp,
    /*! . */
    long diffTime) {
    tp->stats.totalJobsLQ++;
    tp->stats.totalTimeLQ += static_cast<double>(diffTime);
}

/*!
 * \brief StatsAccountMQ
 */
void StatsAccountMQ(
    /*! [in] Valid, non null, pointer to ThreadPool. */
    ThreadPool* tp,
    /*! . */
    long diffTime) {
    tp->stats.totalJobsMQ++;
    tp->stats.totalTimeMQ += static_cast<double>(diffTime);
}

/*!
 * \brief StatsAccountHQ
 */
void StatsAccountHQ(
    /*! [in] Valid, non null, pointer to ThreadPool. */
    ThreadPool* tp,
    /*! . */
    long diffTime) {
    tp->stats.totalJobsHQ++;
    tp->stats.totalTimeHQ += static_cast<double>(diffTime);
}

/*!
 * \brief Calculates the time the job has been waiting at the specified
 * priority.
 *
 * Adds to the totalTime and totalJobs kept in the thread pool statistics
 * structure.
 */
void CalcWaitTime(
    /*! [in] Valid, non null, pointer to ThreadPool. */
    ThreadPool* tp,
    /*! [in] Thread priority. */
    ThreadPriority p,
    /*! [in] Valid thread pool job. */
    ThreadPoolJob* job) {
    struct timeval now;
    long diff;

    assert(tp != NULL);
    assert(job != NULL);

    gettimeofday(&now, NULL);
    diff = DiffMillis(&now, &job->requestTime);
    switch (p) {
    case LOW_PRIORITY:
        StatsAccountLQ(tp, diff);
        break;
    case MED_PRIORITY:
        StatsAccountMQ(tp, diff);
        break;
    case HIGH_PRIORITY:
        StatsAccountHQ(tp, diff);
        break;
    default:
        assert(0);
    }
}

/*!
 * \brief StatsTime
 */
time_t StatsTime(
    /*! . */
    time_t* t) {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    if (t)
        *t = tv.tv_sec;

    return tv.tv_sec;
}
#else  /* STATS */
inline void StatsInit(ThreadPoolStats* stats) {}
inline void StatsAccountLQ(ThreadPool* tp, long diffTime) {}
inline void StatsAccountMQ(ThreadPool* tp, long diffTime) {}
inline void StatsAccountHQ(ThreadPool* tp, long diffTime) {}
inline void CalcWaitTime(ThreadPool* tp, ThreadPriority p, ThreadPoolJob* job) {
}
inline time_t StatsTime(time_t* t) { return 0; }
#endif /* STATS */

/*!
 * \brief Compares thread pool jobs.
 */
int CmpThreadPoolJob(void* jobA, void* jobB) {
    ThreadPoolJob* a = (ThreadPoolJob*)jobA;
    ThreadPoolJob* b = (ThreadPoolJob*)jobB;

    return a->jobId == b->jobId;
}

/*!
 * \brief Deallocates a dynamically allocated ThreadPoolJob.
 */
void FreeThreadPoolJob(
    /*! [in] Valid, non null, pointer to ThreadPool. */
    ThreadPool* tp,
    /*! [in] Must be allocated with CreateThreadPoolJob. */
    ThreadPoolJob* tpj) {
    FreeListFree(&tp->jobFreeList, tpj);
}

/*!
 * \brief Sets the scheduling policy of the current process.
 *
 * \returns
 *  On success: **0**\n
 *  On error: result of GetLastError() on failure.
 */
int SetPolicyType(
    /*! . */
    [[maybe_unused]] PolicyType in) {
    int retVal = 0;
#ifdef __CYGWIN__
    /*! \todo not currently working... */
    (void)in;
    retVal = 0;
#elif defined(__APPLE__) || defined(__NetBSD__)
    (void)in;
    setpriority(PRIO_PROCESS, 0, 0);
    retVal = 0;
#elif defined(__PTW32_DLLPORT)
    retVal = sched_setscheduler(0, in);
#elif defined(_POSIX_PRIORITY_SCHEDULING) && _POSIX_PRIORITY_SCHEDULING > 0
    struct sched_param current;
    int sched_result;

    memset(&current, 0, sizeof(current));
    sched_getparam(0, &current);
    current.sched_priority = sched_get_priority_min(DEFAULT_POLICY);
    sched_result = sched_setscheduler(0, in, &current);
    retVal = (sched_result != -1 || errno == EPERM) ? 0 : errno;
#else
    retVal = 0;
#endif
    return retVal;
}

/*!
 * \brief Sets the priority of the currently running thread.
 *
 * \returns
 *  On success: **0**\n
 *  On error: EINVAL invalid priority or the result of GerLastError.
 */
int SetPriority(
    /*! [in] Thread priority */
    ThreadPriority priority) {
#if defined(_POSIX_PRIORITY_SCHEDULING) && _POSIX_PRIORITY_SCHEDULING > 0
    int retVal = 0;
    int currentPolicy;
    int minPriority = 0;
    int maxPriority = 0;
    int actPriority = 0;
    int midPriority = 0;
    struct sched_param newPriority;
    int sched_result;

    pthread_getschedparam(pthread_self(), &currentPolicy, &newPriority);
    minPriority = sched_get_priority_min(currentPolicy);
    maxPriority = sched_get_priority_max(currentPolicy);
    midPriority = (maxPriority - minPriority) / 2;
    switch (priority) {
    case LOW_PRIORITY:
        actPriority = minPriority;
        break;
    case MED_PRIORITY:
        actPriority = midPriority;
        break;
    case HIGH_PRIORITY:
        actPriority = maxPriority;
        break;
    default:
        retVal = EINVAL;
        goto exit_function;
    };

    newPriority.sched_priority = actPriority;

    sched_result =
        pthread_setschedparam(pthread_self(), currentPolicy, &newPriority);
    retVal = (sched_result == 0 || errno == EPERM) ? 0 : sched_result;
exit_function:
    return retVal;
#else
    (void)priority;
    return 0;
#endif
}

/*!
 * \brief Determines whether any jobs need to be bumped to a higher priority Q
 * and bumps them.
 *
 * tp->mutex must be locked.
 */
void BumpPriority(
    /*! [in] Valid, non null, pointer to ThreadPool. */
    ThreadPool* tp) {
    int done = 0;
    struct timeval now;
    long diffTime = 0;
    ThreadPoolJob* tempJob = NULL;

    gettimeofday(&now, NULL);
    while (!done) {
        if (tp->medJobQ.size) {
            tempJob = (ThreadPoolJob*)tp->medJobQ.head.next->item;
            diffTime = DiffMillis(&now, &tempJob->requestTime);
            if (diffTime >= tp->attr.starvationTime) {
                /* If job has waited longer than the starvation time, bump
                 * priority (add to higher priority Q) */
                StatsAccountMQ(tp, diffTime);
                ListDelNode(&tp->medJobQ, tp->medJobQ.head.next, 0);
                ListAddTail(&tp->highJobQ, tempJob);
                continue;
            }
        }
        if (tp->lowJobQ.size) {
            tempJob = (ThreadPoolJob*)tp->lowJobQ.head.next->item;
            diffTime = DiffMillis(&now, &tempJob->requestTime);
            if (diffTime >= tp->attr.maxIdleTime) {
                /* If job has waited longer than the starvation time, bump
                 * priority (add to higher priority Q) */
                StatsAccountLQ(tp, diffTime);
                ListDelNode(&tp->lowJobQ, tp->lowJobQ.head.next, 0);
                ListAddTail(&tp->medJobQ, tempJob);
                continue;
            }
        }
        done = 1;
    }
}

/*!
 * \brief Sets the fields of the passed in timespec to be relMillis
 * milliseconds in the future.
 */
void SetRelTimeout(
    /*! . */
    timespec* time,
    /*! milliseconds in the future. */
    int relMillis) {
    timeval now;
    int sec = relMillis / 1000;
    int milliSeconds = relMillis % 1000;

    gettimeofday(&now, NULL);
    time->tv_sec = now.tv_sec + sec;
    time->tv_nsec = (now.tv_usec / 1000 + milliSeconds) * 1000000;
}

/*!
 * \brief Sets seed for random number generator.
 *
 * Each thread sets the seed random number generator.
 *
 * \todo Solve problem with type casts.
 */
void SetSeed(void) {
    struct timeval t;

    gettimeofday(&t, NULL);
#if defined(__PTW32_DLLPORT) // pthreads4w on Microsoft Windows available.
    srand((unsigned int)t.tv_usec +
          (unsigned int)((unsigned long long)pthread_self().p));
#elif defined(BSD) || defined(__APPLE__) || defined(__FreeBSD_kernel__)
    srand((unsigned int)t.tv_usec +
          (unsigned int)((unsigned long)pthread_self()));
#elif defined(__linux__) || defined(__sun) || defined(__CYGWIN__) ||           \
    defined(__GLIBC__)
    srand((unsigned int)t.tv_usec + (unsigned int)pthread_self());
#else
    {
        volatile union {
            volatile pthread_t tid;
            volatile unsigned i;
        } idu;

        idu.tid = pthread_self();
        srand((unsigned int)t.tv_usec + idu.i);
    }
#endif
}

/*!
 * \brief Implements a thread pool worker.
 *
 * Worker waits for a job to become available. Worker picks up persistent jobs
 * first, high priority, med priority, then low priority. If worker remains
 * idle for more than specified max, the worker is released.
 */
void* WorkerThread(
    /*! arg -> is cast to (ThreadPool *). */
    void* arg) {
    time_t start = 0;

    ThreadPoolJob* job = NULL;
    ListNode* head = NULL;

    timespec timeout;
    int retCode = 0;
    int persistent = -1;
    ThreadPool* tp = (ThreadPool*)arg;

    UPnPsdk::initialize_thread();

    /* Increment total thread count */
    pthread_mutex_lock(&tp->mutex);
    tp->totalThreads++;
    tp->pendingWorkerThreadStart = 0;
    pthread_cond_broadcast(&tp->start_and_shutdown);
    pthread_mutex_unlock(&tp->mutex);

    SetSeed();
    StatsTime(&start);
    while (1) {
        pthread_mutex_lock(&tp->mutex);
        if (job) {
            tp->busyThreads--;
            FreeThreadPoolJob(tp, job);
            job = NULL;
        }
        retCode = 0;
        tp->stats.idleThreads++;
        tp->stats.totalWorkTime += (double)StatsTime(NULL) - (double)start;
        StatsTime(&start);
        if (persistent == 0) {
            tp->stats.workerThreads--;
        } else if (persistent == 1) {
            /* Persistent thread becomes a regular thread */
            tp->persistentThreads--;
        }

        /* Check for a job or shutdown */
        while (tp->lowJobQ.size == 0 && tp->medJobQ.size == 0 &&
               tp->highJobQ.size == 0 && !tp->persistentJob && !tp->shutdown) {
            /* If wait timed out and we currently have more than the
             * min threads, or if we have more than the max threads
             * (only possible if the attributes have been reset)
             * let this thread die. */
            if ((retCode == ETIMEDOUT &&
                 tp->totalThreads > tp->attr.minThreads) ||
                (tp->attr.maxThreads != -1 &&
                 tp->totalThreads > tp->attr.maxThreads)) {
                tp->stats.idleThreads--;
                goto exit_function;
            }
            SetRelTimeout(&timeout, tp->attr.maxIdleTime);

            /* wait for a job up to the specified max time */
            retCode =
                pthread_cond_timedwait(&tp->condition, &tp->mutex, &timeout);
        }
        tp->stats.idleThreads--;
        /* idle time */
        tp->stats.totalIdleTime += (double)StatsTime(NULL) - (double)start;
        /* work time */
        StatsTime(&start);
        /* bump priority of starved jobs */
        BumpPriority(tp);
        /* if shutdown then stop */
        if (tp->shutdown) {
            goto exit_function;
        } else {
            /* Pick up persistent job if available */
            if (tp->persistentJob) {
                job = tp->persistentJob;
                tp->persistentJob = NULL;
                tp->persistentThreads++;
                persistent = 1;
                pthread_cond_broadcast(&tp->start_and_shutdown);
            } else {
                tp->stats.workerThreads++;
                persistent = 0;
                /* Pick the highest priority job */
                if (tp->highJobQ.size > 0) {
                    head = ListHead(&tp->highJobQ);
                    if (head == NULL) {
                        tp->stats.workerThreads--;
                        goto exit_function;
                    }
                    job = (ThreadPoolJob*)head->item;
                    CalcWaitTime(tp, HIGH_PRIORITY, job);
                    ListDelNode(&tp->highJobQ, head, 0);
                } else if (tp->medJobQ.size > 0) {
                    head = ListHead(&tp->medJobQ);
                    if (head == NULL) {
                        tp->stats.workerThreads--;
                        goto exit_function;
                    }
                    job = (ThreadPoolJob*)head->item;
                    CalcWaitTime(tp, MED_PRIORITY, job);
                    ListDelNode(&tp->medJobQ, head, 0);
                } else if (tp->lowJobQ.size > 0) {
                    head = ListHead(&tp->lowJobQ);
                    if (head == NULL) {
                        tp->stats.workerThreads--;
                        goto exit_function;
                    }
                    job = (ThreadPoolJob*)head->item;
                    CalcWaitTime(tp, LOW_PRIORITY, job);
                    ListDelNode(&tp->lowJobQ, head, 0);
                } else {
                    /* Should never get here */
                    tp->stats.workerThreads--;
                    goto exit_function;
                }
            }
        }

        tp->busyThreads++;
        pthread_mutex_unlock(&tp->mutex);

        /* In the future can log info */
        if (SetPriority(job->priority) != 0) {
        } else {
        }
        /* run the job */
        job->func(job->arg);
        /* return to Normal */
        SetPriority(DEFAULT_PRIORITY);
    }

exit_function:
    tp->totalThreads--;
    pthread_cond_broadcast(&tp->start_and_shutdown);
    pthread_mutex_unlock(&tp->mutex);
    UPnPsdk::cleanup_thread();

    return NULL;
}

/*!
 * \brief Creates a Thread Pool Job. (Dynamically allocated)
 *
 * \returns
 *  On success: Pointer to a ThreadPoolJob\n
 *  On error: nullptr
 */
ThreadPoolJob* CreateThreadPoolJob(
    /*! job is copied. */
    ThreadPoolJob* job,
    /*! id of job. */
    int id,
    /*! [in] Valid, non null, pointer to ThreadPool. */
    ThreadPool* tp) {
    ThreadPoolJob* newJob{nullptr};

    newJob = (ThreadPoolJob*)FreeListAlloc(&tp->jobFreeList);
    if (newJob) {
        *newJob = *job;
        newJob->jobId = id;
        gettimeofday(&newJob->requestTime, NULL);
    }

    return newJob;
}

/*!
 * \brief Creates a worker thread, if the thread pool does not already have
 * max threads.
 *
 * \remark The ThreadPool object mutex must be locked prior to calling this
 * function.
 *
 * \returns
 *  On success: **0**\n
 *  On error: **< 0**
 *  - EMAXTHREADS if already max threads reached.
 *  - EAGAIN if system can not create thread.
 */
int CreateWorker(
    /*! [in] Valid, non null, pointer to ThreadPool. */
    ThreadPool* tp) {
    pthread_t temp;
    int rc = 0;
    pthread_attr_t attr;

    /* if a new worker is the process of starting, wait until it fully
     * starts */
    while (tp->pendingWorkerThreadStart) {
        pthread_cond_wait(&tp->start_and_shutdown, &tp->mutex);
    }

    if (tp->attr.maxThreads != INFINITE_THREADS &&
        tp->totalThreads + 1 > tp->attr.maxThreads) {
        return EMAXTHREADS;
    }
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, tp->attr.stackSize);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    rc = pthread_create(&temp, &attr, WorkerThread, tp);
    pthread_attr_destroy(&attr);
    if (rc == 0) {
        tp->pendingWorkerThreadStart = 1;
        /* wait until the new worker thread starts */
        while (tp->pendingWorkerThreadStart) {
            pthread_cond_wait(&tp->start_and_shutdown, &tp->mutex);
        }
    }
    if (tp->stats.maxThreads < tp->totalThreads) {
        tp->stats.maxThreads = tp->totalThreads;
    }

    return rc;
}

/*!
 * \brief Determines whether or not a thread should be added based on the
 * jobsPerThread ratio.
 *
 * Adds a thread if appropriate.
 *
 * \remark The ThreadPool object mutex must be locked prior to calling this
 * function.
 */
void AddWorker(
    /*! [in] Valid, non null, pointer to ThreadPool. */
    ThreadPool* tp) {
    long jobs = 0;
    int threads = 0;

    jobs = tp->highJobQ.size + tp->lowJobQ.size + tp->medJobQ.size;
    threads = tp->totalThreads - tp->persistentThreads;
    while (threads == 0 || (jobs / threads) >= tp->attr.jobsPerThread ||
           (tp->totalThreads == tp->busyThreads)) {
        if (CreateWorker(tp) != 0) {
            return;
        }
        threads++;
    }
}

/// @} // Functions (scope restricted to file)
} // anonymous namespace


int ThreadPoolInit(ThreadPool* tp, ThreadPoolAttr* attr) {
    TRACE2("Executing ThreadPoolInit() for ThreadPool ", tp)
    int retCode = 0;
    int i = 0;

    if (!tp) {
        return EINVAL;
    }

    retCode += pthread_mutex_init(&tp->mutex, NULL);
    retCode += pthread_mutex_lock(&tp->mutex);

    retCode += pthread_cond_init(&tp->condition, NULL);
    retCode += pthread_cond_init(&tp->start_and_shutdown, NULL);
    if (retCode) {
        pthread_mutex_unlock(&tp->mutex);
        pthread_mutex_destroy(&tp->mutex);
        pthread_cond_destroy(&tp->condition);
        pthread_cond_destroy(&tp->start_and_shutdown);
        return EAGAIN;
    }
    if (attr) {
        tp->attr = *attr;
    } else {
        TPAttrInit(&tp->attr);
    }
    if (SetPolicyType(tp->attr.schedPolicy) != 0) {
        pthread_mutex_unlock(&tp->mutex);
        pthread_mutex_destroy(&tp->mutex);
        pthread_cond_destroy(&tp->condition);
        pthread_cond_destroy(&tp->start_and_shutdown);

        return INVALID_POLICY;
    }
    retCode +=
        FreeListInit(&tp->jobFreeList, sizeof(ThreadPoolJob), JOBFREELISTSIZE);
    StatsInit(&tp->stats);
    retCode += ListInit(&tp->highJobQ, CmpThreadPoolJob, NULL);
    retCode += ListInit(&tp->medJobQ, CmpThreadPoolJob, NULL);
    retCode += ListInit(&tp->lowJobQ, CmpThreadPoolJob, NULL);
    if (retCode) {
        retCode = EAGAIN;
    } else {
        tp->persistentJob = NULL;
        tp->lastJobId = 0;
        tp->shutdown = 0;
        tp->totalThreads = 0;
        tp->busyThreads = 0;
        tp->persistentThreads = 0;
        tp->pendingWorkerThreadStart = 0;
        for (i = 0; i < tp->attr.minThreads; ++i) {
            retCode = CreateWorker(tp);
            if (retCode) {
                break;
            }
        }
    }

    pthread_mutex_unlock(&tp->mutex);

    if (retCode) {
        /* clean up if the min threads could not be created */
        ThreadPoolShutdown(tp);
    }

    return retCode;
}

int ThreadPoolAddPersistent(ThreadPool* tp, ThreadPoolJob* job, int* jobId) {
    int ret = 0;
    int tempId = -1;
    ThreadPoolJob* temp = NULL;

    if (!tp || !job) {
        return EINVAL;
    }
    if (!jobId) {
        jobId = &tempId;
    }
    *jobId = INVALID_JOB_ID;

    pthread_mutex_lock(&tp->mutex);

    /* Create A worker if less than max threads running */
    if (tp->totalThreads < tp->attr.maxThreads) {
        CreateWorker(tp);
    } else {
        /* if there is more than one worker thread
         * available then schedule job, otherwise fail */
        if (tp->totalThreads - tp->persistentThreads - 1 == 0) {
            ret = EMAXTHREADS;
            goto exit_function;
        }
    }
    temp = CreateThreadPoolJob(job, tp->lastJobId, tp);
    if (!temp) {
        ret = EOUTOFMEM;
        goto exit_function;
    }
    tp->persistentJob = temp;

    /* Notify a waiting thread */
    pthread_cond_signal(&tp->condition);

    /* wait until long job has been picked up */
    while (tp->persistentJob)
        pthread_cond_wait(&tp->start_and_shutdown, &tp->mutex);
    *jobId = tp->lastJobId++;

exit_function:
    pthread_mutex_unlock(&tp->mutex);

    return ret;
}

int ThreadPoolAdd(ThreadPool* tp, ThreadPoolJob* job, int* jobId) {
    TRACE("Executing ThreadPoolAdd()")
    int rc = EOUTOFMEM;
    int tempId = -1;
    long totalJobs;
    ThreadPoolJob* temp = NULL;

    if (!tp || !job)
        return EINVAL;

    pthread_mutex_lock(&tp->mutex);

    totalJobs = tp->highJobQ.size + tp->lowJobQ.size + tp->medJobQ.size;
    if (totalJobs >= tp->attr.maxJobsTotal) {
        fprintf(stderr, "libupnp ThreadPoolAdd too many jobs: %ld\n",
                totalJobs);
        goto exit_function;
    }
    if (!jobId)
        jobId = &tempId;
    *jobId = INVALID_JOB_ID;
    temp = CreateThreadPoolJob(job, tp->lastJobId, tp);
    if (!temp)
        goto exit_function;
    switch (job->priority) {
    case HIGH_PRIORITY:
        if (ListAddTail(&tp->highJobQ, temp))
            rc = 0;
        break;
    case MED_PRIORITY:
        if (ListAddTail(&tp->medJobQ, temp))
            rc = 0;
        break;
    default:
        if (ListAddTail(&tp->lowJobQ, temp))
            rc = 0;
    }
    /* AddWorker if appropriate */
    AddWorker(tp);
    /* Notify a waiting thread */
    if (rc == 0)
        pthread_cond_signal(&tp->condition);
    else
        FreeThreadPoolJob(tp, temp);
    *jobId = tp->lastJobId++;

exit_function:
    pthread_mutex_unlock(&tp->mutex);

    return rc;
}

int ThreadPoolRemove(ThreadPool* tp, int jobId, ThreadPoolJob* out) {
    int ret = INVALID_JOB_ID;
    ThreadPoolJob* temp = NULL;
    ListNode* tempNode = NULL;
    ThreadPoolJob dummy;

    if (!tp)
        return EINVAL;
    if (!out)
        out = &dummy;
    dummy.jobId = jobId;

    pthread_mutex_lock(&tp->mutex);

    tempNode = ListFind(&tp->highJobQ, NULL, &dummy);
    if (tempNode) {
        temp = (ThreadPoolJob*)tempNode->item;
        *out = *temp;
        ListDelNode(&tp->highJobQ, tempNode, 0);
        FreeThreadPoolJob(tp, temp);
        ret = 0;
        goto exit_function;
    }

    tempNode = ListFind(&tp->medJobQ, NULL, &dummy);
    if (tempNode) {
        temp = (ThreadPoolJob*)tempNode->item;
        *out = *temp;
        ListDelNode(&tp->medJobQ, tempNode, 0);
        FreeThreadPoolJob(tp, temp);
        ret = 0;
        goto exit_function;
    }
    tempNode = ListFind(&tp->lowJobQ, NULL, &dummy);
    if (tempNode) {
        temp = (ThreadPoolJob*)tempNode->item;
        *out = *temp;
        ListDelNode(&tp->lowJobQ, tempNode, 0);
        FreeThreadPoolJob(tp, temp);
        ret = 0;
        goto exit_function;
    }
    if (tp->persistentJob && tp->persistentJob->jobId == jobId) {
        *out = *tp->persistentJob;
        FreeThreadPoolJob(tp, tp->persistentJob);
        tp->persistentJob = NULL;
        ret = 0;
        goto exit_function;
    }

exit_function:
    pthread_mutex_unlock(&tp->mutex);

    return ret;
}

int ThreadPoolGetAttr(ThreadPool* tp, ThreadPoolAttr* out) {
    if (!tp || !out)
        return EINVAL;
    if (!tp->shutdown)
        pthread_mutex_lock(&tp->mutex);
    *out = tp->attr;
    if (!tp->shutdown)
        pthread_mutex_unlock(&tp->mutex);

    return 0;
}

int ThreadPoolSetAttr(ThreadPool* tp, ThreadPoolAttr* attr) {
    int retCode = 0;
    ThreadPoolAttr temp;
    int i = 0;

    if (!tp)
        return EINVAL;

    pthread_mutex_lock(&tp->mutex);

    if (attr)
        temp = *attr;
    else
        TPAttrInit(&temp);
    if (SetPolicyType(temp.schedPolicy) != 0) {
        pthread_mutex_unlock(&tp->mutex);
        return INVALID_POLICY;
    }
    tp->attr = temp;
    /* add threads */
    if (tp->totalThreads < tp->attr.minThreads) {
        for (i = tp->totalThreads; i < tp->attr.minThreads; i++) {
            retCode = CreateWorker(tp);
            if (retCode != 0) {
                break;
            }
        }
    }
    /* signal changes */
    pthread_cond_signal(&tp->condition);

    pthread_mutex_unlock(&tp->mutex);

    if (retCode != 0)
        /* clean up if the min threads could not be created */
        ThreadPoolShutdown(tp);

    return retCode;
}

int ThreadPoolShutdown(ThreadPool* tp) {
    TRACE2("Executing ThreadPoolShutdown() for ThreadPool ", tp)
    ListNode* head = NULL;
    ThreadPoolJob* temp = NULL;

    if (!tp)
        return EINVAL;
    pthread_mutex_lock(&tp->mutex);
    /* clean up high priority jobs */
    while (tp->highJobQ.size) {
        head = ListHead(&tp->highJobQ);
        if (head == NULL) {
            pthread_mutex_unlock(&tp->mutex);
            return EINVAL;
        }
        temp = (ThreadPoolJob*)head->item;
        if (temp->free_func)
            temp->free_func(temp->arg);
        FreeThreadPoolJob(tp, temp);
        ListDelNode(&tp->highJobQ, head, 0);
    }
    ListDestroy(&tp->highJobQ, 0);
    /* clean up med priority jobs */
    while (tp->medJobQ.size) {
        head = ListHead(&tp->medJobQ);
        if (head == NULL) {
            pthread_mutex_unlock(&tp->mutex);
            return EINVAL;
        }
        temp = (ThreadPoolJob*)head->item;
        if (temp->free_func)
            temp->free_func(temp->arg);
        FreeThreadPoolJob(tp, temp);
        ListDelNode(&tp->medJobQ, head, 0);
    }
    ListDestroy(&tp->medJobQ, 0);
    /* clean up low priority jobs */
    while (tp->lowJobQ.size) {
        head = ListHead(&tp->lowJobQ);
        if (head == NULL) {
            pthread_mutex_unlock(&tp->mutex);
            return EINVAL;
        }
        temp = (ThreadPoolJob*)head->item;
        if (temp->free_func)
            temp->free_func(temp->arg);
        FreeThreadPoolJob(tp, temp);
        ListDelNode(&tp->lowJobQ, head, 0);
    }
    ListDestroy(&tp->lowJobQ, 0);
    /* clean up long term job */
    if (tp->persistentJob) {
        temp = tp->persistentJob;
        if (temp->free_func)
            temp->free_func(temp->arg);
        FreeThreadPoolJob(tp, temp);
        tp->persistentJob = NULL;
    }
    /* signal shutdown */
    tp->shutdown = 1;
    pthread_cond_broadcast(&tp->condition);
    /* wait for all threads to finish */
    while (tp->totalThreads > 0)
        pthread_cond_wait(&tp->start_and_shutdown, &tp->mutex);
    /* destroy condition */
    while (pthread_cond_destroy(&tp->condition) != 0) {
    }
    while (pthread_cond_destroy(&tp->start_and_shutdown) != 0) {
    }
    FreeListDestroy(&tp->jobFreeList);

    pthread_mutex_unlock(&tp->mutex);

    /* destroy mutex */
    while (pthread_mutex_destroy(&tp->mutex) != 0) {
    }

    return 0;
}

int maxJobsTotal = DEFAULT_MAX_JOBS_TOTAL;

void TPSetMaxJobsTotal(int mjt) { maxJobsTotal = mjt; }

int TPAttrInit(ThreadPoolAttr* attr) {
    if (!attr)
        return EINVAL;
    attr->jobsPerThread = DEFAULT_JOBS_PER_THREAD;
    attr->maxIdleTime = DEFAULT_IDLE_TIME;
    attr->maxThreads = DEFAULT_MAX_THREADS;
    attr->minThreads = DEFAULT_MIN_THREADS;
    attr->stackSize = DEFAULT_STACK_SIZE;
    attr->schedPolicy = DEFAULT_POLICY;
    attr->starvationTime = DEFAULT_STARVATION_TIME;
    attr->maxJobsTotal = maxJobsTotal;

    return 0;
}

int TPJobInit(ThreadPoolJob* job, UPnPsdk::start_routine func, void* arg) {
    if (!job || !func)
        return EINVAL;
    job->func = func;
    job->arg = arg;
    job->priority = DEFAULT_PRIORITY;
    job->free_func = DEFAULT_FREE_ROUTINE;

    return 0;
}

int TPJobSetPriority(ThreadPoolJob* job, ThreadPriority priority) {
    if (!job)
        return EINVAL;
    switch (priority) {
    case LOW_PRIORITY:
    case MED_PRIORITY:
    case HIGH_PRIORITY:
        job->priority = priority;
        return 0;
    default:
        return EINVAL;
    }
}

int TPJobSetFreeFunction(ThreadPoolJob* job, free_routine func) {
    if (!job)
        return EINVAL;
    job->free_func = func;

    return 0;
}

int TPAttrSetMaxThreads(ThreadPoolAttr* attr, int maxThreads) {
    if (!attr)
        return EINVAL;
    attr->maxThreads = maxThreads;

    return 0;
}

int TPAttrSetMinThreads(ThreadPoolAttr* attr, int minThreads) {
    if (!attr)
        return EINVAL;
    attr->minThreads = minThreads;

    return 0;
}

int TPAttrSetStackSize(ThreadPoolAttr* attr, size_t stackSize) {
    if (!attr)
        return EINVAL;
    attr->stackSize = stackSize;

    return 0;
}

int TPAttrSetIdleTime(ThreadPoolAttr* attr, int idleTime) {
    if (!attr)
        return EINVAL;
    attr->maxIdleTime = idleTime;

    return 0;
}

int TPAttrSetJobsPerThread(ThreadPoolAttr* attr, int jobsPerThread) {
    if (!attr)
        return EINVAL;
    attr->jobsPerThread = jobsPerThread;

    return 0;
}

int TPAttrSetStarvationTime(ThreadPoolAttr* attr, int starvationTime) {
    if (!attr)
        return EINVAL;
    attr->starvationTime = starvationTime;

    return 0;
}

int TPAttrSetSchedPolicy(ThreadPoolAttr* attr, PolicyType schedPolicy) {
    if (!attr)
        return EINVAL;
    attr->schedPolicy = schedPolicy;

    return 0;
}

int TPAttrSetMaxJobsTotal(ThreadPoolAttr* attr, int totalMaxJobs) {
    if (!attr)
        return EINVAL;
    attr->maxJobsTotal = totalMaxJobs;

    return 0;
}

#if defined(STATS) || defined(DOXYGEN_RUN)
void ThreadPoolPrintStats(ThreadPoolStats* stats) {
    if (!stats)
        return;
    /* some OSses time_t length may depending on platform, promote it to
     * long for safety */
    fprintf(stderr, "ThreadPoolStats at Time: %ld\n", (long)StatsTime(NULL));
    fprintf(stderr, "High Jobs pending: %d\n", stats->currentJobsHQ);
    fprintf(stderr, "Med Jobs Pending: %d\n", stats->currentJobsMQ);
    fprintf(stderr, "Low Jobs Pending: %d\n", stats->currentJobsLQ);
    fprintf(stderr, "Average Wait in High Priority Q in milliseconds: %f\n",
            stats->avgWaitHQ);
    fprintf(stderr, "Average Wait in Med Priority Q in milliseconds: %f\n",
            stats->avgWaitMQ);
    fprintf(stderr, "Averate Wait in Low Priority Q in milliseconds: %f\n",
            stats->avgWaitLQ);
    fprintf(stderr, "Max Threads Active: %d\n", stats->maxThreads);
    fprintf(stderr, "Current Worker Threads: %d\n", stats->workerThreads);
    fprintf(stderr, "Current Persistent Threads: %d\n",
            stats->persistentThreads);
    fprintf(stderr, "Current Idle Threads: %d\n", stats->idleThreads);
    fprintf(stderr, "Total Threads : %d\n", stats->totalThreads);
    fprintf(stderr, "Total Time spent Working in seconds: %f\n",
            stats->totalWorkTime);
    fprintf(stderr, "Total Time spent Idle in seconds : %f\n",
            stats->totalIdleTime);
}

int ThreadPoolGetStats(ThreadPool* tp, ThreadPoolStats* stats) {
    if (tp == NULL || stats == NULL)
        return EINVAL;
    /* if not shutdown then acquire mutex */
    if (!tp->shutdown)
        pthread_mutex_lock(&tp->mutex);

    *stats = tp->stats;
    if (stats->totalJobsHQ > 0)
        stats->avgWaitHQ = stats->totalTimeHQ / (double)stats->totalJobsHQ;
    else
        stats->avgWaitHQ = 0.0;
    if (stats->totalJobsMQ > 0)
        stats->avgWaitMQ = stats->totalTimeMQ / (double)stats->totalJobsMQ;
    else
        stats->avgWaitMQ = 0.0;
    if (stats->totalJobsLQ > 0)
        stats->avgWaitLQ = stats->totalTimeLQ / (double)stats->totalJobsLQ;
    else
        stats->avgWaitLQ = 0.0;
    stats->totalThreads = tp->totalThreads;
    stats->persistentThreads = tp->persistentThreads;
    stats->currentJobsHQ = (int)ListSize(&tp->highJobQ);
    stats->currentJobsLQ = (int)ListSize(&tp->lowJobQ);
    stats->currentJobsMQ = (int)ListSize(&tp->medJobQ);

    /* if not shutdown then release mutex */
    if (!tp->shutdown)
        pthread_mutex_unlock(&tp->mutex);

    return 0;
}
#endif /* STATS */

#ifdef _WIN32
/// \cond
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS 11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS 11644473600000000ULL
#endif
/// \endcond

int gettimeofday(struct timeval* tv, struct timezone* tz) {
    FILETIME ft;
    unsigned __int64 tmpres = 0;
    static int tzflag;

    if (tv) {
        GetSystemTimeAsFileTime(&ft);

        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;

        /*converting file time to unix epoch*/
        tmpres /= 10; /*convert into microseconds*/
        tmpres -= DELTA_EPOCH_IN_MICROSECS;
        tv->tv_sec = (long)(tmpres / 1000000UL);
        tv->tv_usec = (long)(tmpres % 1000000UL);
    }
    if (tz) {
        if (!tzflag) {
            _tzset();
            tzflag++;
        }
#ifdef _UCRT
        long itz = 0;
        _get_timezone(&itz);
        tz->tz_minuteswest = (int)(itz / 60);
        _get_daylight(&tz->tz_dsttime);
#else
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
#endif
    }

    return 0;
}
#endif /* _WIN32 */
