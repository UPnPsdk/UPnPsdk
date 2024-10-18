// Copyright (C) 2024+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-10-18

#include <UPnPsdk/strintmap.hpp>
#include <UPnPsdk/httpparser.hpp> // for HTTPMETHOD* constants
#include <UPnPsdk/global.hpp>
#include <utest/utest.hpp>
#include <array> // MS Windows needs this

namespace utest {

// using ::testing::ExitedWithCode;


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
    int idx = UPnPsdk::str_to_int("Delete", Http_Method_Table);
    EXPECT_EQ(idx, 0);

    idx = UPnPsdk::str_to_int("UNSUBSCRIBE", Http_Method_Table, true);
    EXPECT_EQ(idx, 9);
}

#if 0
TEST(StrintmapDeathTest, map_str_to_int_with_nullptr_to_namestring) {
    Cstrintmap mapObj;

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ]" CRES
                  << " A nullptr to the namestring must not segfault.\n";
        // This expects segfault.
        EXPECT_DEATH(mapObj.map_str_to_int(nullptr, 6, Http_Method_Table,
                                           NUM_HTTP_METHODS, 1),
                     ".*");

    } else {

        // This expects NO segfault.
        ASSERT_EXIT((mapObj.map_str_to_int(nullptr, 6, Http_Method_Table,
                                           NUM_HTTP_METHODS, 1),
                     exit(0)),
                    ExitedWithCode(0), ".*");
        int idx{};
        idx = mapObj.map_str_to_int(nullptr, 6, Http_Method_Table,
                                    NUM_HTTP_METHODS, 1);
        EXPECT_EQ(idx, -1);
    }
}

TEST(StrintmapDeathTest, map_str_to_int_with_zero_namestring_length) {
    Cstrintmap mapObj;

    // This expects NO segfault.
    ASSERT_EXIT((mapObj.map_str_to_int("NOTIFY", 0, Http_Method_Table,
                                       NUM_HTTP_METHODS, 1),
                 exit(0)),
                ExitedWithCode(0), ".*");
    int idx{};
    idx = mapObj.map_str_to_int("NOTIFY", 0, Http_Method_Table,
                                NUM_HTTP_METHODS, 1);
    EXPECT_EQ(idx, -1);
}

TEST(StrintmapDeathTest, map_str_to_int_with_nullptr_to_table) {
    Cstrintmap mapObj;

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ]" CRES
                  << " A nullptr to a table must not segfault.\n";
        // This expects segfault.
        EXPECT_DEATH(
            mapObj.map_str_to_int("NOTIFY", 6, nullptr, NUM_HTTP_METHODS, 1),
            ".*");

    } else {

        // This expects NO segfault.
        ASSERT_EXIT(
            (mapObj.map_str_to_int("NOTIFY", 6, nullptr, NUM_HTTP_METHODS, 1),
             exit(0)),
            ExitedWithCode(0), ".*");
        int idx{};
        idx = mapObj.map_str_to_int("NOTIFY", 6, nullptr, NUM_HTTP_METHODS, 1);
        EXPECT_EQ(idx, -1);
    }
}

TEST(StrintmapDeathTest, map_str_to_int_with_zero_table_entries) {
    Cstrintmap mapObj;

    // This expects NO segfault.
    ASSERT_EXIT(
        (mapObj.map_str_to_int("NOTIFY", 6, Http_Method_Table, 0, 1), exit(0)),
        ExitedWithCode(0), ".*");
    int idx{};
    idx = mapObj.map_str_to_int("NOTIFY", 6, Http_Method_Table, 0, 1);
    EXPECT_EQ(idx, -1);
}

TEST(StrintmapTestSuite, map_str_to_int_with_different_namestring_cases) {
    Cstrintmap mapObj;

    EXPECT_EQ(mapObj.map_str_to_int("Notify", 6, Http_Method_Table,
                                    NUM_HTTP_METHODS, -1),
              -1);
    EXPECT_EQ(mapObj.map_str_to_int("Notify", 6, Http_Method_Table,
                                    NUM_HTTP_METHODS, 0),
              5);
}

TEST(StrintmapTestSuite, map_str_to_int_with_different_name_length) {
    Cstrintmap mapObj;

    EXPECT_EQ(mapObj.map_str_to_int("M-Searc", 7, Http_Method_Table,
                                    NUM_HTTP_METHODS, 0),
              -1);
    EXPECT_EQ(mapObj.map_str_to_int("M-SEARCHING", 11, Http_Method_Table,
                                    NUM_HTTP_METHODS, 1),
              -1);
}
#endif

TEST(StrintmapTestSuite, int_to_str) {
    int idx =
        UPnPsdk::int_to_str(UPnPsdk::HTTPMETHOD_UNSUBSCRIBE, Http_Method_Table);
    EXPECT_EQ(idx, 9);
}

#if 0
TEST(StrintmapTestSuite, map_int_to_str_with_invalid_id) {
    Cstrintmap mapObj;

    int idx{};
    idx = mapObj.map_int_to_str(65444, Http_Method_Table, NUM_HTTP_METHODS);
    EXPECT_EQ(idx, -1);
}

TEST(StrintmapDeathTest, map_int_to_str_with_nullptr_to_table) {
    Cstrintmap mapObj;

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ]" CRES
                  << " A nullptr to a table must not segfault.\n";
        // This expects segfault.
        EXPECT_DEATH(mapObj.map_int_to_str(::HTTPMETHOD_NOTIFY, nullptr,
                                           NUM_HTTP_METHODS),
                     ".*");

    } else {

        // This expects NO segfault.
        ASSERT_EXIT((mapObj.map_int_to_str(::HTTPMETHOD_NOTIFY, nullptr,
                                           NUM_HTTP_METHODS),
                     exit(0)),
                    ExitedWithCode(0), ".*");
        int idx{};
        idx = mapObj.map_int_to_str(::HTTPMETHOD_NOTIFY, nullptr,
                                    NUM_HTTP_METHODS);
        EXPECT_EQ(idx, -1);
    }
}

TEST(StrintmapTestSuite, map_int_to_str_with_zero_table_entiries) {
    Cstrintmap mapObj;

    int idx{};
    idx = mapObj.map_int_to_str(::HTTPMETHOD_NOTIFY, Http_Method_Table, 0);
    EXPECT_EQ(idx, -1);
}

TEST(StrintmapTestSuite, map_int_to_str_with_oversized_table_entiries) {
    Cstrintmap mapObj;

    int idx{};
    idx = mapObj.map_int_to_str(65444, Http_Method_Table, NUM_HTTP_METHODS + 1);
    EXPECT_EQ(idx, -1);
}
#endif

} // namespace utest

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
