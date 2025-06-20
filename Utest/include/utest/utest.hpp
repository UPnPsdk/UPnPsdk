#ifndef UPnPsdk_UTEST_HPP
#define UPnPsdk_UTEST_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-11

#include <UPnPsdk/visibility.hpp>
#include <UPnPsdk/port.hpp>
#include <UPnPsdk/synclog.hpp>

#include <regex>
#include <cstring>
#include <gmock/gmock.h>

// ANSI console colors
#define CRED "\033[38;5;203m" // red
#define CYEL "\033[38;5;227m" // yellow
#define CGRN "\033[38;5;83m"  // green
#define CRES "\033[0m"        // reset


namespace utest {

#ifdef UPnPsdk_WITH_NATIVE_PUPNP
constexpr bool old_code{true};
#else
constexpr bool old_code{false};
#endif
const bool github_actions{static_cast<bool>(std::getenv("GITHUB_ACTIONS"))};


// ###############################
//            Helper             #
// ###############################

// Capture output to stdout or stderr
// ----------------------------------
// class CaptureStdOutErr declaration
//
// Helper class to capture output to stdout or stderr into a buffer so we can
// compare it. We use a pipe that is opened non blocking if not on Microsoft
// Windows. That does not support unblocking on its _pipe(). I workaround it
// with always writing a '\0' to the pipe just before reading it. So it will
// never block. But we risk a deadlock when the internal pipe buffer is to small
// to capture all data. Write to a full buffer will be blocked. Then we do not
// have any chance to read the pipe to free the buffer. Maybe it could be
// managed with asynchronous mode on a pipe but that is far away from Posix
// compatible handling. This possible deadlock is only a problem on MS Windows.
//
// If you run into a deadlock on MS Windows then increase the
// 'm_pipebuffer_size'. --Ingo
// clang-format off
//
// Accessing the captured string will stop captureing.
// Typical usage is:
/*
    CaptureStdOutErr stderrObj; // Will use default UPnPsdk::log_fileno
    CaptureStdOutErr stderrObj(STDERR_FILENO); // or STDOUT_FILENO
    stderrObj.start();
    std::cerr << "Hello"; // or any other output from within functions
    EXPECT_EQ(stderrObj.str(), "Hello"));
    stderrObj.str() += " World";
    EXPECT_EQ(stderrObj.str(), "Hello World"));
*/
// Exception: Strong guarantee (no modifications)
//    throws: [std::logic_error] <- std::invalid_argument
//              Only stdout or stderr can be redirected.
//    throws: runtime_error
//              Failed to create a pipe.
//              Failed to duplicate a file descriptor.
//              Failed to read from pipe.
//              Read 0 byte from pipe.
// clang-format on

class UPnPsdk_VIS CaptureStdOutErr {
  public:
    CaptureStdOutErr(int a_fileno = UPnPsdk::log_fileno);
    virtual ~CaptureStdOutErr();
    void start();
    std::string& str();

  private:
    int out_pipe[2]{};
    static constexpr int m_pipebuffer_size{8192};
    static constexpr int m_chunk_size{512};
    bool m_capturing{false};
    SUPPRESS_MSVC_WARN_4251_NEXT_LINE
    std::string m_strbuffer{};

    // The original file descriptor STDOUT_FILENO or STDERR_FILENO that is
    // captured.
    int stdOutErrFd{};

    // This is the current stdout/stderr file descriptor that either points to
    // the system output or to the pipes write end.
    int current_stdOutErrFd;

    // This is a dup of the original system stdout/stderr file descriptor so we
    // can restore it to the current_stdOutErrFd.
    int orig_stdOutErrFd{};
};


// function to get the modification time of a file
// -----------------------------------------------
//     using ::utest::file_mod_time;
UPnPsdk_VIS time_t file_mod_time(const std::string& a_pathname);


// function to test if file descriptors are closed
// -----------------------------------------------
//     using ::utest::check_closed_fds;
UPnPsdk_VIS void check_closed_fds(int a_from_fd, int a_to_fd);


// ###############################
//        Custom Matcher         #
// ###############################

// Matcher to use portable Regex from the C++ standard library
//------------------------------------------------------------
// This overcomes the mixed internal MatchesRegex() and ContainsRegex() from
// Googlemock. On Unix it uses Posix regex but on MS Windows it uses a limited
// gmock custom implementation. If using that to be portable we are limited to
// the gmock cripple regex for MS Windows.
// Reference: https://github.com/google/googletest/issues/1208
//
// ECMAScript syntax: https://cplusplus.com/reference/regex/ECMAScript/
//
/* Example:
    using ::utest::ContainsStdRegex;

    EXPECT_THAT(capturedStderr,
                ContainsStdRegex(" UPNP-MSER-1: .* invalid socket\\(-1\\) "));
*/
MATCHER_P(MatchesStdRegex, pattern, "") {
    static_cast<void>(*result_listener); // To suppress warning "unused param"
    std::regex regex(pattern);
    return std::regex_match(arg, regex);
}
MATCHER_P(ContainsStdRegex, pattern, "") {
    static_cast<void>(*result_listener); // To suppress warning "unused param"
    std::regex regex(pattern);
    return std::regex_search(arg, regex);
}

// Void pointer must be type casted
// --------------------------------
MATCHER_P(PointeeVoidToConstInt, expected, "") {
    static_cast<void>(*result_listener); // To suppress warning "unused param"
    return *static_cast<const int*>(arg) == expected;
}


// ###############################
//        Custom Actions         #
// ###############################

// Action for side effect to place a value at a location
// -----------------------------------------------------
// Generate function to set value refered to by n-th argument getsockopt(). This
// allows us to mock functions that pass in a pointer, expecting the result to
// be put into that location.
// Simple version:
// ACTION_P(SetArg3PtrIntValue, value) { *static_cast<int*>(arg3) = value; }
//
/* Example:
    using ::utest::SetArgPtrIntValue

    // Here the parameter count is zero based     0    1  2  3  4
    EXPECT_CALL(mock_sys_socketObj, getsockopt(sockfd, _, _, _, _))
        .WillOnce(DoAll(SetArgPtrIntValue<3>(1), Return(0)));
*/
ACTION_TEMPLATE(SetArgPtrIntValue, HAS_1_TEMPLATE_PARAMS(int, k),
                AND_1_VALUE_PARAMS(value)) {
    *static_cast<int*>(std::get<k>(args)) = value;
}

// Action to return a string literal
// ---------------------------------
// Coppy a string to an argument can also be done with
// SetArrayArgument<N>(first, last) but then we may get trouble with void*
// pointer to the Cstring. This is solved with type cast in this action.
//
// Reference: https://groups.google.com/g/googlemock/c/lQqCMW1ANQA
// simple version: ACTION_P(StrCpyToArg0, str) { strcpy(arg0, str); }
//
/* Example:
    using ::utest::StrCpyToArg;

    EXPECT_CALL( mocked_sys_socketObj, recvfrom(sockfd, _, _, _, _, _))
        .WillOnce(DoAll(StrCpyToArg<1>("ShutDown"), Return(8)));
*/
// Using type cast in case there is a 'void*' pointer used.
ACTION_TEMPLATE(StrCpyToArg, HAS_1_TEMPLATE_PARAMS(int, k),
                AND_1_VALUE_PARAMS(str)) {
    std::strcpy(static_cast<char*>(std::get<k>(args)), str);
}
ACTION_TEMPLATE(StrnCpyToArg, HAS_1_TEMPLATE_PARAMS(int, k),
                AND_2_VALUE_PARAMS(str, len)) {
    std::strncpy(static_cast<char*>(std::get<k>(args)), str,
                 static_cast<size_t>(len));
}

// Action for side effect to place or fill a structure at a location
// -----------------------------------------------------------------
// simple version:
// ACTION_P(StructCpyToArg0, src) { memcpy(arg0, src, sizeof(*arg0)); }
/* Example:
    using ::utest::StructCpyToArg;

    timeval timeout{1, 1000);
    EXPECT_CALL(m_sys_socketObj, select(_, _, _, _, _)
        .WillOnce(DoAll(StructCpyToArg<4>(&timeout), Return(1)));
*/
ACTION_TEMPLATE(StructCpyToArg, HAS_1_TEMPLATE_PARAMS(int, k),
                AND_2_VALUE_PARAMS(src, len)) {
    auto arg = std::get<k>(args);
    if (arg != nullptr && src != nullptr)
        std::memcpy(arg, src, len);
}

ACTION_TEMPLATE(SaddrCpyToArg, HAS_1_TEMPLATE_PARAMS(int, k),
                AND_1_VALUE_PARAMS(src)) {
    std::get<k>(args) = src;
}

ACTION_TEMPLATE(StructSetToArg, HAS_1_TEMPLATE_PARAMS(int, k),
                AND_1_VALUE_PARAMS(val)) {
    auto arg = std::get<k>(args);
    if (arg != nullptr)
        std::memset(arg, val, sizeof(*arg));
}

// Set Error portable means set 'errno' on Linux, 'WSASetLastError()' on win32
// ---------------------------------------------------------------------------
// 'errid' is portable mapped in UPnPsdk socket.hpp.
ACTION_P(SetErrPtblAndReturn, errid, ret) {
#ifdef _MSC_VER
    ::WSASetLastError(errid);
#else
    errno = errid;
#endif
    return (ret);
}

} // namespace utest

#endif // UPnPsdk_UTEST_HPP
