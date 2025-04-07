// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-04-07

#include <UPnPsdk/strintmap.hpp>
#include <UPnPsdk/httpparser.hpp> // for HTTPMETHOD* constants
#include <utest/utest.hpp>
#include <array>                  // MS Windows needs this

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

UPnPsdk::CStrIntMap table(Http_Method_Table);


// testsuite for strintmap
//========================

TEST(StrintmapTestSuite, index_of_boundaries) {
    size_t idx = table.index_of("Delete");
    EXPECT_EQ(idx, 0);

    idx = table.index_of("UNSUBSCRIBE", true);
    EXPECT_EQ(idx, 9);
}

TEST(StrintmapTestSuite, index_of_outer_boundaries) {
    size_t idx = table.index_of("aaa");
    EXPECT_EQ(idx, table.npos);

    idx = table.index_of("ZZZ", true);
    EXPECT_EQ(idx, table.npos);
}

TEST(StrintmapTestSuite, nullptr_to_namestring) {
    ASSERT_EXIT((table.index_of(nullptr, true), exit(0)), ExitedWithCode(0),
                ".*");
    size_t idx{};
    idx = table.index_of(nullptr, true);
    EXPECT_EQ(idx, table.npos);
}

TEST(StrintmapTestSuite, empty_namestring) {
    size_t idx{};
    idx = table.index_of("\0");
    EXPECT_EQ(idx, table.npos);
    idx = 0;
    idx = table.index_of("\0", true);
    EXPECT_EQ(idx, table.npos);
}

TEST(StrintmapTestSuite, different_namestring_cases) {
    EXPECT_EQ(table.index_of("Notify", true), table.npos);
    EXPECT_EQ(table.index_of("Notify", false), 5);
}

TEST(StrintmapTestSuite, different_name_length) {
    EXPECT_EQ(table.index_of("M-Searc"), table.npos);
    EXPECT_EQ(table.index_of("M-SEARCHING", true), table.npos);
}

TEST(StrintmapTestSuite, int_to_str) {
    size_t idx = table.index_of(UPnPsdk::HTTPMETHOD_UNSUBSCRIBE);
    EXPECT_EQ(idx, 9);
}

TEST(StrintmapTestSuite, int_to_str_with_invalid_id) {
    size_t idx{};
    idx = table.index_of(INT_MAX);
    EXPECT_EQ(idx, table.npos);
    idx = 0;
    idx = table.index_of(INT_MIN);
    EXPECT_EQ(idx, table.npos);
}

} // namespace utest

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
