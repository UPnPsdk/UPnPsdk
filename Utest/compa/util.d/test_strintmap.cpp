// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-10-20

#include <strintmap.hpp>
#include <httpparser.hpp> // for HTTPMETHOD* constants

#include <UPnPsdk/global.hpp>
#include <utest/utest.hpp>

namespace utest {

using ::testing::ExitedWithCode;


// testsuite for strintmap
//========================

constexpr size_t NUM_HTTP_METHODS = 10;
static str_int_entry Http_Method_Table[NUM_HTTP_METHODS] = {
    {"DELETE", HTTPMETHOD_DELETE},
    {"GET", HTTPMETHOD_GET},
    {"HEAD", HTTPMETHOD_HEAD},
    {"M-POST", HTTPMETHOD_MPOST},
    {"M-SEARCH", HTTPMETHOD_MSEARCH},
    {"NOTIFY", HTTPMETHOD_NOTIFY},
    {"POST", HTTPMETHOD_POST},
    {"PUT", HTTPMETHOD_PUT},
    {"SUBSCRIBE", HTTPMETHOD_SUBSCRIBE},
    {"UNSUBSCRIBE", HTTPMETHOD_UNSUBSCRIBE}};


TEST(StrintmapTestSuite, map_str_to_int_get_boundaries) {
    int idx =
        map_str_to_int("Delete", 6, Http_Method_Table, NUM_HTTP_METHODS, 0);
    EXPECT_EQ(idx, 0);

    idx = map_str_to_int("UNSUBSCRIBE", 11, Http_Method_Table, NUM_HTTP_METHODS,
                         1);
    EXPECT_EQ(idx, 9);
}

TEST(StrintmapDeathTest, map_str_to_int_with_nullptr_to_namestring) {
    std::cout << CYEL "[ BUGFIX   ]" CRES
              << " A nullptr to the namestring must not segfault.\n";
    // This expects segfault.
    EXPECT_DEATH(
        map_str_to_int(nullptr, 6, Http_Method_Table, NUM_HTTP_METHODS, 1),
        ".*");
}

TEST(StrintmapTestSuite, map_str_to_int_with_empty_namestring) {
    int idx{0};
    idx = map_str_to_int("\0", 0, Http_Method_Table, NUM_HTTP_METHODS, 0);
    EXPECT_EQ(idx, -1);
    idx = 0;
    idx = map_str_to_int("\0", 1, Http_Method_Table, NUM_HTTP_METHODS, 1);
    EXPECT_EQ(idx, -1);
}

TEST(StrintmapDeathTest, map_str_to_int_with_zero_namestring_length) {
    // This expects NO segfault.
    ASSERT_EXIT(
        (map_str_to_int("NOTIFY", 0, Http_Method_Table, NUM_HTTP_METHODS, 1),
         exit(0)),
        ExitedWithCode(0), ".*");
    int idx{};
    idx = map_str_to_int("NOTIFY", 0, Http_Method_Table, NUM_HTTP_METHODS, 1);
    EXPECT_EQ(idx, -1);
}

TEST(StrintmapDeathTest, map_str_to_int_with_nullptr_to_table) {
    std::cout << CYEL "[ BUGFIX   ]" CRES
              << " A nullptr to a table must not segfault.\n";
    // This expects segfault.
    EXPECT_DEATH(map_str_to_int("NOTIFY", 6, nullptr, NUM_HTTP_METHODS, 1),
                 ".*");
}

TEST(StrintmapDeathTest, map_str_to_int_with_zero_table_entries) {
    // This expects NO segfault.
    ASSERT_EXIT((map_str_to_int("NOTIFY", 6, Http_Method_Table, 0, 1), exit(0)),
                ExitedWithCode(0), ".*");
    int idx{};
    idx = map_str_to_int("NOTIFY", 6, Http_Method_Table, 0, 1);
    EXPECT_EQ(idx, -1);
}

TEST(StrintmapTestSuite, map_str_to_int_with_different_namestring_cases) {
    EXPECT_EQ(
        map_str_to_int("Notify", 6, Http_Method_Table, NUM_HTTP_METHODS, -1),
        -1);
    EXPECT_EQ(
        map_str_to_int("Notify", 6, Http_Method_Table, NUM_HTTP_METHODS, 0), 5);
}

TEST(StrintmapTestSuite, map_str_to_int_with_different_name_length) {
    EXPECT_EQ(
        map_str_to_int("M-Searc", 7, Http_Method_Table, NUM_HTTP_METHODS, 0),
        -1);
    EXPECT_EQ(map_str_to_int("M-SEARCHING", 11, Http_Method_Table,
                             NUM_HTTP_METHODS, 1),
              -1);
}

TEST(StrintmapTestSuite, map_int_to_str) {
    int idx = map_int_to_str(::HTTPMETHOD_NOTIFY, Http_Method_Table,
                             NUM_HTTP_METHODS);
    EXPECT_EQ(idx, 5);
}

TEST(StrintmapTestSuite, map_int_to_str_with_invalid_id) {
    int idx{};
    idx = map_int_to_str(65444, Http_Method_Table, NUM_HTTP_METHODS);
    EXPECT_EQ(idx, -1);
}

TEST(StrintmapDeathTest, map_int_to_str_with_nullptr_to_table) {
    std::cout << CYEL "[ BUGFIX   ]" CRES
              << " A nullptr to a table must not segfault.\n";
    // This expects segfault.
    EXPECT_DEATH(map_int_to_str(::HTTPMETHOD_NOTIFY, nullptr, NUM_HTTP_METHODS),
                 ".*");
}

TEST(StrintmapTestSuite, map_int_to_str_with_zero_table_entries) {
    int idx{};
    idx = map_int_to_str(::HTTPMETHOD_NOTIFY, Http_Method_Table, 0);
    EXPECT_EQ(idx, -1);
}

TEST(StrintmapTestSuite, map_int_to_str_with_oversized_table_entries) {
    int idx{};
    idx = map_int_to_str(65444, Http_Method_Table, NUM_HTTP_METHODS + 1);
    EXPECT_EQ(idx, -1);
}

} // namespace utest

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
