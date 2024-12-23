// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-12-25

#include <UPnPsdk/upnp_device.hpp>
#include <utest/utest.hpp>

namespace utest {

using ::UPnPsdk::CRootdevice;


TEST(RootDeviceTestSuite, bind) {
    CRootdevice rootdev;
    rootdev.bind("lo");
#ifdef __unix__
    EXPECT_EQ(rootdev.ifname(), "lo");
#endif
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include "utest/utest_main.inc"
    return gtest_return_code; // managed in compa/gtest_main.inc
}
