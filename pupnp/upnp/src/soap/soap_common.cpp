// Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2022-02-19

#include "config.h"
#if EXCLUDE_SOAP == 0

#include "httpparser.hpp"
#include "sock.hpp"
#include "soaplib.hpp"

const char* ContentTypeHeader = "CONTENT-TYPE: text/xml; charset=\"utf-8\"\r\n";

#endif /* EXCLUDE_SOAP */
