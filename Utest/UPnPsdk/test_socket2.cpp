// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-01-09

#include <UPnPsdk/socket.hpp>
#include <utest/utest.hpp>

namespace utest {

using ::testing::HasSubstr;

using ::UPnPsdk::CSocket;
using ::UPnPsdk::CSocket_basic;
using ::UPnPsdk::SSockaddr;


TEST(SockTestSuite, instantiate_unbind_socket) {
    CSocket sockObj(SOCK_STREAM);
    EXPECT_EQ(static_cast<SOCKET>(sockObj), INVALID_SOCKET);
    SSockaddr saddr;
    // EXPECT_THROW(sockObj.sockaddr(saddr), std::runtime_error);
    sockObj.sockaddr(saddr);
    EXPECT_EQ(saddr.netaddrp(), ":0");
    // EXPECT_EQ(sockObj.get_socktype(), SOCK_STREAM);
}

TEST(SocketBindTestSuite, bind2_with_address_successful) {
    SSockaddr saddr;
    saddr = "[::1]:50001";

    CSocket sockObj;
    ASSERT_NO_THROW(sockObj.bind2(SOCK_STREAM, &saddr));

    EXPECT_TRUE(sockObj.is_bound());
    EXPECT_EQ(sockObj.get_socktype(), SOCK_STREAM);
    SSockaddr sa_sockObj;
    sockObj.sockaddr(sa_sockObj);
    EXPECT_TRUE(saddr == sa_sockObj);
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
