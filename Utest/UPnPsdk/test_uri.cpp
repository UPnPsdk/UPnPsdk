// Copyright (C) 2025+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-10-03

#include <UPnPsdk/uri.hpp>
#include <utest/utest.hpp>

namespace utest {

using ::UPnPsdk::CUri;


TEST(CUriTestSuite, uri_get_empty) {
    CUri uriObj;
    EXPECT_TRUE(uriObj.scheme().empty());
}

TEST(CUriTestSuite, uri_scheme) {
    CUri uriObj;
    EXPECT_TRUE(uriObj.scheme().empty());

    uriObj = "http-s:";
    EXPECT_EQ(uriObj.scheme(), "http-s");
    EXPECT_THROW({ uriObj = ""; }, std::invalid_argument);
    EXPECT_THROW({ uriObj = ":"; }, std::invalid_argument);
    EXPECT_THROW({ uriObj = "https"; }, std::invalid_argument);
    EXPECT_THROW({ uriObj = "0https:"; }, std::invalid_argument);
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv); // preferred
#include <utest/utest_main.inc>
    return gtest_return_code;               // managed in utest/utest_main.inc
}
