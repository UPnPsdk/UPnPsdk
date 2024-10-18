// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-10-18

#include <UPnPsdk/strintmap.hpp>
#include <UPnPsdk/httpparser.hpp> // for HTTPMETHOD* constants
#include <UPnPsdk/global.hpp>
#include <utest/utest.hpp>
#include <array> // MS Windows needs this

namespace utest {

using ::testing::ExitedWithCode;


/*! \brief Defines the HTTP methods. */
constexpr std::array<UPnPsdk::str_int_entry, 10> Http_Method_Table{
    {{"DELETE", UPnPsdk::HTTPMETHOD_DELETE},
     {"GET", UPnPsdk::HTTPMETHOD_GET},
     {"HEAD", UPnPsdk::HTTPMETHOD_HEAD},
     {"M-POST", UPnPsdk::HTTPMETHOD_MPOST},
     {"M-SEARCH", UPnPsdk::HTTPMETHOD_MSEARCH},
     {"NOTIFY", UPnPsdk::HTTPMETHOD_NOTIFY},
     {"POST", UPnPsdk::HTTPMETHOD_POST},
     {"PUT", UPnPsdk::HTTPMETHOD_PUT},
     {"SUBSCRIBE", UPnPsdk::HTTPMETHOD_SUBSCRIBE},
     {"UNSUBSCRIBE", UPnPsdk::HTTPMETHOD_UNSUBSCRIBE}}};


// testsuite for strintmap
//========================
TEST(StrintmapTestSuite, str_to_int_get_boundaries) {
    int idx = str_to_int("Delete", Http_Method_Table);
    EXPECT_EQ(idx, 0);

    idx = str_to_int("UNSUBSCRIBE", Http_Method_Table, true);
    EXPECT_EQ(idx, 9);
}

TEST(StrintmapTestSuite, str_to_int_with_nullptr_to_namestring) {
    ASSERT_EXIT((str_to_int(nullptr, Http_Method_Table, true), exit(0)),
                ExitedWithCode(0), ".*");
    int idx{};
    idx = str_to_int(nullptr, Http_Method_Table, true);
    EXPECT_EQ(idx, -1);
}

TEST(StrintmapTestSuite, str_to_int_with_empty_namestring) {
    int idx{};
    idx = str_to_int("\0", Http_Method_Table);
    EXPECT_EQ(idx, -1);
    idx = 0;
    idx = str_to_int("\0", Http_Method_Table, true);
    EXPECT_EQ(idx, -1);
}

TEST(StrintmapTestSuite, str_to_int_with_different_namestring_cases) {
    EXPECT_EQ(str_to_int("Notify", Http_Method_Table, true), -1);
    EXPECT_EQ(str_to_int("Notify", Http_Method_Table, false), 5);
}

TEST(StrintmapTestSuite, str_to_int_with_different_name_length) {
    EXPECT_EQ(str_to_int("M-Searc", Http_Method_Table), -1);
    EXPECT_EQ(str_to_int("M-SEARCHING", Http_Method_Table, true), -1);
}

TEST(StrintmapTestSuite, int_to_str) {
    int idx = int_to_str(UPnPsdk::HTTPMETHOD_UNSUBSCRIBE, Http_Method_Table);
    EXPECT_EQ(idx, 9);
}

TEST(StrintmapTestSuite, int_to_str_with_invalid_id) {
    int idx{};
    idx = int_to_str(INT_MAX, Http_Method_Table);
    EXPECT_EQ(idx, -1);
    idx = 0;
    idx = int_to_str(INT_MIN, Http_Method_Table);
    EXPECT_EQ(idx, -1);
}

} // namespace utest

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
