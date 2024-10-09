// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-10-09

#include <gmock/gmock.h>

namespace utest {

TEST(EmptyTestSuite, empty_gtest) {
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
    #include <utest/utest_main.inc>
    return gtest_return_code; // managed in utest/utest_main.inc
}
