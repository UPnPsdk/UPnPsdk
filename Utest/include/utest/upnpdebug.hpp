#ifndef UTEST_PUPNPDEBUG_HPP
#define UTEST_PUPNPDEBUG_HPP
// Copyright (C) 2025+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-07-28

#include <upnpdebug.hpp>
#include <UPnPsdk/synclog.hpp>
#include <exception>
#include <stdexcept>


namespace utest {

// Helper class for debug log messages in compatible code.
// -------------------------------------------------------
class CPupnplog { /*
 * Use it for example with:
    CPupnplog loggingObj; // Output only with build type DEBUG.
    loggingObj.enable(UPNP_ALL); // or other loglevel, e.g. UPNP_INFO.
    loggingObj.disable(); // optional
 */
  public:
    CPupnplog();
    virtual ~CPupnplog();
    /// -brief Enable debug logging messages.
    void enable(Upnp_LogLevel a_loglevel);
    /// -brief Disable debug logging messages.
    void disable();
};

CPupnplog::CPupnplog() = default;

void CPupnplog::enable(Upnp_LogLevel a_loglevel) {
    UpnpSetLogLevel(a_loglevel);
    if (UpnpInitLog() != UPNP_E_SUCCESS) {
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG1041") "Failed to initialize pupnp logging.");
    }
}

void CPupnplog::disable() { UpnpCloseLog(); }

CPupnplog::~CPupnplog() { UpnpCloseLog(); }

} // namespace utest

#endif // UTEST_PUPNPDEBUG_HPP
