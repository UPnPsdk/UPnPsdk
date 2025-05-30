# Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2025-05-30

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(GTESTS_COMPA_THREADUTIL VERSION 0002
                  DESCRIPTION "Tests for the compa threadutil module"
                  HOMEPAGE_URL "https://github.com/UPnPsdk")


# FreeList
#=========
# Additional FreeList tests can be found until
# git commit 1baf9b7cb29aadae5408a42461c013ae4638d92e

add_executable(test_FreeList-psh
#-------------------------------
        ./test_FreeList.cpp
)
target_link_libraries(test_FreeList-psh
    PRIVATE pupnp_shared
    PRIVATE utest_shared
)
add_test(NAME ctest_FreeList-psh COMMAND test_FreeList-psh --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# LinkedList
#===========
add_executable(test_LinkedList-psh
        ./test_LinkedList.cpp
)
target_link_libraries(test_LinkedList-psh
    PRIVATE pupnp_shared
    PRIVATE utest_shared
)
add_test(NAME ctest_LinkedList-psh COMMAND test_LinkedList-psh --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)


# ThreadPool
#===========
add_executable(test_ThreadPool-pst
        ./test_ThreadPool.cpp
)
target_link_libraries(test_ThreadPool-pst
    PRIVATE pupnp_static
)
add_test(NAME ctest_ThreadPool-pst COMMAND test_ThreadPool-pst --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

if(FALSE)
add_executable(test_ThreadPool-cst
        ./test_ThreadPool.cpp
)
target_link_libraries(test_ThreadPool-cst
    PRIVATE compa_static
)
add_test(NAME ctest_ThreadPool-cst COMMAND test_ThreadPool-cst --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)
endif()


# TimerThread
#============
add_executable(test_TimerThread-pst
        ./test_TimerThread.cpp
)
target_link_libraries(test_TimerThread-pst
    PRIVATE pupnp_static
)
add_test(NAME ctest_TimerThread-pst COMMAND test_TimerThread-pst --gtest_shuffle
        WORKING_DIRECTORY ${UPnPsdk_RUNTIME_OUTPUT_DIRECTORY}
)

# I do not modify the ThreadPool management so there is no need to test for
# compatible modifications. I intend to replace threading with C++ standard
# library functions. --Ingo
