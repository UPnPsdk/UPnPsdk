// Copyright (C) 2025+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-03-15

#include <UPnPsdk/netadapter2.hpp>
#include <UPnPsdk/port_sock.hpp>
#include <utest/utest.hpp>

namespace utest {

// Coppied from <UPnPsdk/socket.hpp>.
// To be portable with BSD socket error number constants I have to
// define and use these macros with appended 'P' for portable.
#ifdef _MSC_VER
#define EBADFP WSAENOTSOCK
#define ENOTCONNP WSAENOTCONN
#define EINTRP WSAEINTR
#define EFAULTP WSAEFAULT
#define ENOMEMP WSA_NOT_ENOUGH_MEMORY
#define EINVALP WSAEINVAL
#define EACCESP WSAEACCES
#else
#define EBADFP EBADF
#define ENOTCONNP ENOTCONN
#define EINTRP EINTR
#define EFAULTP EFAULT
#define ENOMEMP ENOMEM
#define EINVALP EINVAL
#define EACCESP EACCES
#endif
/// \endcond

using ::testing::Return;


TEST(SocketErrorTestSuite, get_socket_error_successful) {
    // Create a di-client object with default di-service.
    UPnPsdk::CSocketErr2 socket_errObj;

    // Test Unit
    // This returns a real error of an invalid socket file descriptor number.
    char so_opt;
    socklen_t optlen{sizeof(so_opt)};
    int ret_getsockopt =
        ::getsockopt(55555, SOL_SOCKET, SO_ERROR, &so_opt, &optlen);
    socket_errObj.catch_error();
    EXPECT_NE(ret_getsockopt, 0);

    EXPECT_EQ(static_cast<int>(socket_errObj), EBADFP);
    // Don't know what exact message is given. It depends on the platform.
    EXPECT_GE(socket_errObj.get_error_str().size(), 10);
    std::cout << socket_errObj.get_error_str() << "\n";
}


class CSocketErrMock : public UPnPsdk::ISocketErr {
  public:
    CSocketErrMock() = default;
    virtual ~CSocketErrMock() override = default;
    MOCK_METHOD(int, sock_err_ref, ());
    virtual operator const int&() override {
        m_errno = sock_err_ref();
        return m_errno;
    }
    MOCK_METHOD(void, catch_error, (), (override));
    MOCK_METHOD(std::string, get_error_str, (), (const, override));

  private:
    int m_errno{};
};

TEST(SocketErrorTestSuite, mock_sockerr_successful) {
    // Create mocking di-service object and get the smart pointer to it.
    auto socket_err_mockPtr = std::make_shared<CSocketErrMock>();

    // Mock the di-service.
    EXPECT_CALL(*socket_err_mockPtr, sock_err_ref()).WillOnce(Return(123));
    EXPECT_CALL(*socket_err_mockPtr, catch_error()).Times(1);
    EXPECT_CALL(*socket_err_mockPtr, get_error_str())
        .WillOnce(Return("Mocked error string."));

    // Create a di-client object and inject the mock di-service.
    UPnPsdk::CSocketErr2 socketerrObj(socket_err_mockPtr);

    EXPECT_EQ(static_cast<int>(socketerrObj), 123);
    socketerrObj.catch_error();
    EXPECT_EQ(socketerrObj.get_error_str(), "Mocked error string.");
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
