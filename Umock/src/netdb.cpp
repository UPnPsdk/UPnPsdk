// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-12-11

#include <umock/netdb.hpp>
#include <UPnPsdk/port.hpp>

namespace umock {

NetdbInterface::NetdbInterface() = default;
NetdbInterface::~NetdbInterface() = default;

NetdbReal::NetdbReal() = default;
NetdbReal::~NetdbReal() = default;
int NetdbReal::getaddrinfo(const char* node, const char* service,
                           const struct addrinfo* hints,
                           struct addrinfo** res) {
    return ::getaddrinfo(node, service, hints, res);
}
void NetdbReal::freeaddrinfo(struct addrinfo* res) {
    return ::freeaddrinfo(res);
}
#ifndef _MSC_VER
servent* NetdbReal::getservent() { return ::getservent(); }

servent* NetdbReal::getservbyname(const char* name, const char* proto) {
    return ::getservbyname(name, proto);
}

servent* NetdbReal::getservbyport(int port, const char* proto) {
    return ::getservbyport(port, proto);
}

void NetdbReal::setservent(int stayopen) { return ::setservent(stayopen); }

void NetdbReal::endservent() { return ::endservent(); }
#endif

// This constructor is used to inject the pointer to the real function.
Netdb::Netdb(NetdbReal* a_ptr_realObj) {
    m_ptr_workerObj = (NetdbInterface*)a_ptr_realObj;
}

// This constructor is used to inject the pointer to the mocking function.
Netdb::Netdb(NetdbInterface* a_ptr_mockObj) {
    m_ptr_oldObj = m_ptr_workerObj;
    m_ptr_workerObj = a_ptr_mockObj;
}

// The destructor is ussed to restore the old pointer.
Netdb::~Netdb() { m_ptr_workerObj = m_ptr_oldObj; }

// Methods
int Netdb::getaddrinfo(const char* node, const char* service,
                       const struct addrinfo* hints, struct addrinfo** res) {
    return m_ptr_workerObj->getaddrinfo(node, service, hints, res);
}

void Netdb::freeaddrinfo(struct addrinfo* res) {
    return m_ptr_workerObj->freeaddrinfo(res);
}

#ifndef _MSC_VER
servent* Netdb::getservent() { return m_ptr_workerObj->getservent(); }

servent* Netdb::getservbyname(const char* name, const char* proto) {
    return m_ptr_workerObj->getservbyname(name, proto);
}

servent* Netdb::getservbyport(int port, const char* proto) {
    return m_ptr_workerObj->getservbyport(port, proto);
}

void Netdb::setservent(int stayopen) {
    return m_ptr_workerObj->setservent(stayopen);
}

void Netdb::endservent() { return m_ptr_workerObj->endservent(); }
#endif

// On program start create an object and inject pointer to the real function.
// This will exist until program end.
NetdbReal netdb_realObj;
SUPPRESS_MSVC_WARN_4273_NEXT_LINE
UPNPLIB_API Netdb netdb_h(&netdb_realObj);

} // namespace umock
