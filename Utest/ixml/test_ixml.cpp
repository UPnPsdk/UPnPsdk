// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-10-09

#include <gmock/gmock.h>
#include <UPnPsdk/global.hpp>
#include <ixml.hpp>

namespace utest {

TEST(IxmlTestSuite, load_document_ex) {
    IXML_Document* doc{};

    int rc = ixmlLoadDocumentEx(IXML_TESTDATA_DIR "/empty_attribute.xml", &doc);
    EXPECT_EQ(rc, IXML_SUCCESS);

    DOMString s = ixmlPrintDocument(doc);
    EXPECT_TRUE((s != nullptr) && (s[0] != '\0'));
    if (s == nullptr || s[0] == '\0') {
        ixmlDocument_free(doc);
        GTEST_FAIL();
    }

    // Skip trailing spaces
    char* p = s + strlen(s) - 1;
    while (isspace(*p) && p > s)
        p--;
    EXPECT_TRUE(*s == '<' && *p == '>');

    ixmlFreeDOMString(s);
    ixmlDocument_free(doc);
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in utest/utest_main.inc
}
