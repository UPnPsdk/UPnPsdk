// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-11-19

#include <UPnPsdk/global.hpp>
#include <UPnPsdk/netadapter.hpp>
#include <utest/utest.hpp>


namespace utest {

using ::testing::AnyOf;


TEST(NetadapterTestSuite, get_adapters_info_successful) {
#ifdef _WIN32
    // This object is only an empty skeleton that will be coded and
    // improved.
    UPnPsdk::CNetadapter netadapterObj;
    UPnPsdk::INetadapter& nadObj{netadapterObj};

    EXPECT_NO_THROW(nadObj.load());
    EXPECT_FALSE(nadObj.get_next());
    UPnPsdk::SSockaddr saObj = nadObj.sockaddr();
    EXPECT_TRUE(saObj.netaddrp().empty());
    saObj = nadObj.socknetmask();
    EXPECT_TRUE(saObj.netaddrp().empty());

#else

    UPnPsdk::CNetadapter netadapterObj;
    UPnPsdk::INetadapter& nadObj{netadapterObj};

    char addrStr[INET6_ADDRSTRLEN];
    char nmskStr[INET6_ADDRSTRLEN];
    char servStr[NI_MAXSERV];
    UPnPsdk::SSockaddr saddrObj;
    UPnPsdk::SSockaddr snmskObj;

    nadObj.load();

    do {
        ASSERT_FALSE(nadObj.name().empty());
        saddrObj = nadObj.sockaddr();
        ASSERT_THAT(saddrObj.ss.ss_family, AnyOf(AF_INET6, AF_INET));
        // Check valid ip address
        ASSERT_EQ(getnameinfo(&saddrObj.sa, sizeof(saddrObj.ss), addrStr,
                              sizeof(addrStr), servStr, sizeof(servStr),
                              NI_NUMERICHOST),
                  0);
        snmskObj = nadObj.socknetmask();
        // Check valid netmask
        ASSERT_EQ(getnameinfo(&snmskObj.sa, sizeof(snmskObj.ss), nmskStr,
                              sizeof(nmskStr), nullptr, 0, NI_NUMERICHOST),
                  0);
#if 0
        // To show resolved iface names set first NI_NUMERICHOST above to 0.
        std::cout << "DEBUG! \"" << nadObj.name() << "\" address = " << addrStr
                  << "(" << saddrObj.netaddr() << "), netmask = " << nmskStr
                  << "(" << snmskObj.netaddr() << "), service = " << servStr
                  << '\n';
#endif
    } while (nadObj.get_next());

#endif // _WIN32
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
