#ifndef UTEST_THREADPOOL_INIT_HPP
#define UTEST_THREADPOOL_INIT_HPP
// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-05-31

namespace utest {

// Class to initialize and shutdown ThreadPools
// --------------------------------------------
// Due to linking with different libraries (pupnp, or compa, or UPnPsdk) this
// class must be included with its definition into the test executable.
// Otherwise it will not find the right subroutines.
//
// This is used on tests that need ThreadPools. With the constructor you can
// specify relevant test parameter, which are the shutdown flag and the maximal
// value of possible jobs.
// a_shutdown: if this flag is set then the threadpool management does not add
//             new jobs to a threadpool. This can be used when the isolated
//             test needs threadpools to be initialized but will have timing
//             problems when running in a thread.
// a_maxjobstotal: for tests this is mostly set to 1 so there is only one job in
//                one thread running. With setting it to 0 it can be tested if
//                the Unit under test will run in a thread. You will get an
//                error message "to much jobs: 0".
class CThreadPoolInit {
  public:
    CThreadPoolInit(::ThreadPool& a_threadpool, const bool a_shutdown = 0,
                    const int a_maxJobsTotal = DEFAULT_MAX_JOBS_TOTAL)
        : m_threadpool(a_threadpool) {
        TRACE2(this, " Construct CThreadPoolInit()")
        // Initialize the given Threadpool. nullptr means to use default
        // attributes.
        int ret = ThreadPoolInit(&m_threadpool, nullptr);
        if (ret != 0)
            throw std::runtime_error(
                "UPnPsdk MSG1045 EXCEPT[" + ::std::string(__func__) +
                "()] Initializing ThreadPool fails with return number " +
                std::to_string(ret));

        if (a_shutdown) {
            TRACE("constructor CThreadPoolInit(): set shutdown flag.")
            // Prevent to add jobs without error message if set, I test jobs
            // isolated.
            m_threadpool.shutdown = 1;
        } else {
            // or allow number of threads. Use 0 with check of error message
            // to stderr to test if threads are used.
            ret = TPAttrSetMaxJobsTotal(&m_threadpool.attr, a_maxJobsTotal);
            if (ret != 0)
                throw std::runtime_error("UPnPsdk MSG1046 EXCEPT[" +
                                         ::std::string(__func__) +
                                         "()] Setting maximal jobs number "
                                         "fails with return number " +
                                         std::to_string(ret));
        }
    }

    virtual ~CThreadPoolInit() {
        TRACE2(this, " Destruct CThreadPoolInit()")
        // Shutdown the threadpool.
        ThreadPoolShutdown(&m_threadpool);
    }

  private:
    ThreadPool& m_threadpool;
};

} // namespace utest

#endif // UTEST_THREADPOOL_INIT_HPP
