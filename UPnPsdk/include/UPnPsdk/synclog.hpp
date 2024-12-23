#ifndef UPnPsdk_SYNCLOG_HPP
#define UPnPsdk_SYNCLOG_HPP
// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-12-23
/*!
 * \file
 * \brief Define macro for synced logging to the console for detailed info and
 * debug.
 */

#include <cmake_vars.hpp>
#include <UPnPsdk/global.hpp>
/// \cond
#include <string>
#include <iostream>
#ifndef __APPLE__
#include <syncstream>
#endif

namespace UPnPsdk {

// clang-format off

// Usage: SYNC(std::cout) << "Message\n";
// Usage: SYNC(std::cerr) << "Error\n";
#ifdef __APPLE__
  #define SYNC(s) (s)
#else
  #define SYNC(s) std::osyncstream((s))
#endif


// Trace messages
// --------------
#ifdef UPnPsdk_WITH_TRACE
  #define TRACE(s) SYNC(std::cerr)<<"TRACE["<<(static_cast<const char*>(__FILE__) + CMAKE_SOURCE_PATH_LENGTH)<<":"<<__LINE__<<"] "<<(s)<<"\n";
  #define TRACE2(a, b) SYNC(std::cerr)<<"TRACE["<<(static_cast<const char*>(__FILE__) + CMAKE_SOURCE_PATH_LENGTH)<<":"<<__LINE__<<"] "<<(a)<<(b)<<"\n";
#else // no UPnPsdk_WITH_TRACE
  #define TRACE(s)
  #define TRACE2(a, b)
#endif


// Debug output messages with some that can be enabled during runtime.
// -------------------------------------------------------------------
// __PRETTY_FUNCTION__ is defined for POSIX so we have it there for the
// function signature to output for information. On MSC_VER it is named
// __FUNCTION__.
#ifdef _MSC_VER
#define __PRETTY_FUNCTION__ __FUNCTION__
// or more verbose: #define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

// This is intended to be used as:
// throw(UPnPsdk_LOGEXCEPT + "MSG1nnn: exception message.\n");
#define UPnPsdk_LOGEXCEPT "UPnPsdk ["+::std::string(__PRETTY_FUNCTION__)+"] EXCEPTION "
#define UPnPsdk_LOGWHAT "UPnPsdk ["+::std::string(__PRETTY_FUNCTION__)+"] WHAT "

// Next line mainly used for Unit Tests to catch the right output channel.
inline constexpr int log_fileno{2}; // 1 = stdout, 2 = stderr, conforming to next line
#define UPnPsdk_LOG SYNC(std::cerr)<<"UPnPsdk ["<<__PRETTY_FUNCTION__
// Critical messages are always output.
#define UPnPsdk_LOGCRIT UPnPsdk_LOG<<"] CRITICAL "
#define UPnPsdk_LOGERR if(UPnPsdk::g_dbug) UPnPsdk_LOG<<"] ERROR "
#define UPnPsdk_LOGCATCH if(UPnPsdk::g_dbug) UPnPsdk_LOG<<"] CATCH "
#define UPnPsdk_LOGINFO if(UPnPsdk::g_dbug) UPnPsdk_LOG<<"] INFO "

// clang-format on
} // namespace UPnPsdk
/// \endcond

#endif // UPnPsdk_SYNCLOG_HPP
