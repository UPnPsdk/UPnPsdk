// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-03-20

#include <pupnp/upnpdebug.hpp>

#include <UPnPsdk/synclog.hpp>
#include <stdexcept>

namespace pupnp {

CLogging::CLogging() = default;

void CLogging::enable(Upnp_LogLevel a_loglevel) {
    UpnpSetLogLevel(a_loglevel);
    if (UpnpInitLog() != UPNP_E_SUCCESS) {
        throw std::runtime_error(
            UPnPsdk_LOGEXCEPT("MSG1041") "Failed to initialize pupnp logging.");
    }
}

void CLogging::disable() { UpnpCloseLog(); }

CLogging::~CLogging() { UpnpCloseLog(); }

} // namespace pupnp
