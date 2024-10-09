// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-08-17

#include <umock/stdlib.hpp>
#include <UPnPsdk/port.hpp>

namespace umock {

StdlibInterface::StdlibInterface() = default;
StdlibInterface::~StdlibInterface() = default;

StdlibReal::StdlibReal() = default;
StdlibReal::~StdlibReal() = default;
void* StdlibReal::malloc(size_t size) { return ::malloc(size); }
void* StdlibReal::calloc(size_t nmemb, size_t size) {
    return ::calloc(nmemb, size);
}
void* StdlibReal::realloc(void* ptr, size_t size) {
    return ::realloc(ptr, size);
}
void StdlibReal::free(void* ptr) { return ::free(ptr); }

// This constructor is used to inject the pointer to the real function.
Stdlib::Stdlib(StdlibReal* a_ptr_realObj) {
    m_ptr_workerObj = (StdlibInterface*)a_ptr_realObj;
}

// This constructor is used to inject the pointer to the mocking function.
Stdlib::Stdlib(StdlibInterface* a_ptr_mockObj) {
    m_ptr_oldObj = m_ptr_workerObj;
    m_ptr_workerObj = a_ptr_mockObj;
}

// The destructor is ussed to restore the old pointer.
Stdlib::~Stdlib() { m_ptr_workerObj = m_ptr_oldObj; }

// Methods
void* Stdlib::malloc(size_t size) { return m_ptr_workerObj->malloc(size); }
void* Stdlib::calloc(size_t nmemb, size_t size) {
    return m_ptr_workerObj->calloc(nmemb, size);
}

void* Stdlib::realloc(void* ptr, size_t size) {
    return m_ptr_workerObj->realloc(ptr, size);
}

void Stdlib::free(void* ptr) { return m_ptr_workerObj->free(ptr); }

// On program start create an object and inject pointer to the real functions.
// This will exist until program end.
StdlibReal stdlib_realObj;
SUPPRESS_MSVC_WARN_4273_NEXT_LINE
UPnPsdk_API Stdlib stdlib_h(&stdlib_realObj);

} // namespace umock
