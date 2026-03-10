// Copyright (C) 2026+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// redistribution only with this copyright remark. last modified: 2026-03-13

// For more specific tests for old_code use 'Utest/compa/uri.d/test_uri.cpp'
// from the git commit. This 'test_uri2.cpp* is focused on Compa with IPv6,
// uncluding IPv4 mapped IPv6, compared to old_code.
//
// Helpful link for ip address structures:
// https://stackoverflow.com/a/16010670/5014688


// Include source code for testing. So we have also direct access to static
// functions which need to be tested.
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
#include <Pupnp/upnp/src/genlib/net/uri/uri.cpp>
// #include <Pupnp/upnp/src/gena/gena_device.cpp>
#else
#include <Compa/src/genlib/net/uri/uri.cpp>
#endif

#include <utest/utest.hpp>


namespace utest {

TEST(UriTestSuite, token_cmp) {
    // == 0, if string1 is identical to string2.
    token inull{nullptr, 0};
    EXPECT_EQ(::token_cmp(&inull, &inull), 0);

    token in0{"", 0};
    EXPECT_EQ(::token_cmp(&in0, &in0), 0);

    token in1{"some entry", 10};
    EXPECT_EQ(::token_cmp(&in1, &in1), 0);

    // < 0, if string1 is less than string2.
    token in2{"some longer entry", 17};

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": With string1 less than string2 it should return < 0 "
                     "not > 0.\n";
        EXPECT_GT(::token_cmp(&in1, &in2), 0); // Wrong!

    } else {

        EXPECT_LT(::token_cmp(&in1, &in2), 0)
            << "  # With string1 less than string2 it should return < 0 not > "
               "0.";
    }

    // > 0, if string1 is greater than string2.
    token in3{"entry", 5};
    EXPECT_GT(::token_cmp(&in1, &in3), 0);
}

TEST(UriDeathTest, token_string_casecmp) {
    // == 0, if string1 is identical to string2 case insensitive.
    token inull{nullptr, 0};
    constexpr char instr0[]{""};

    if (old_code) {
        std::cerr << CYEL "[    FIX   ] " CRES << __LINE__
                  << ": A nullptr in the token structure must not segfault on "
                     "MS Windows.\n";
#ifdef _MSC_VER
        // This expects segfault.
        EXPECT_DEATH(
            { ::token_string_casecmp(&inull, instr0); }, ".*"); // Wrong!
#endif
    } else {

        // This expects NO segfault.
        ASSERT_EXIT(
            {
                ::token_string_casecmp(&inull, instr0);
                exit(0);
            },
            ::testing::ExitedWithCode(0), ".*")
            << "  A nullptr in the token structure must not segfault.\n";

        EXPECT_EQ(::token_string_casecmp(&inull, instr0), 0);
        constexpr char instr1[]{"X"};
        EXPECT_EQ(::token_string_casecmp(&inull, instr1), -1);
    }

    token in0{"", 0};
    EXPECT_EQ(::token_string_casecmp(&in0, instr0), 0);

    token in1{"some entry", 10};
    constexpr char instr10[]{"SOME ENTRY"};
    EXPECT_EQ(::token_string_casecmp(&in1, instr10), 0);

    // < 0, if string1 is less than string2.
    constexpr char instr17[]{"some longer entry"};

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": With string1 less than string2 it should return < 0 "
                     "not > 0.\n";
        EXPECT_GT(::token_string_casecmp(&in1, instr17), 0); // Wrong!

    } else {

        EXPECT_LT(::token_string_casecmp(&in1, instr17), 0)
            << "  # With string1 less than string2 it should return < 0 not > "
               "0.";
    }

    // > 0, if string1 is greater than string2.
    constexpr char instr5[]{"entry"};
    EXPECT_GT(::token_string_casecmp(&in1, instr5), 0);
}


#ifdef UPnPsdk_WITH_NATIVE_PUPNP
// replace_escaped() function: tests from the uri module
// =====================================================

TEST(UriTestSuite, replace_escaped_ip4_check_buffer) {
    const char escstr[]{"%20"};
    size_t max = sizeof(escstr);
    char strbuf[sizeof(escstr)];
    memset(strbuf, 0xFF, max);
    EXPECT_EQ(strbuf[3], '\xFF');
    // Copies escstr with null terminator
    strcpy(strbuf, escstr);

    EXPECT_EQ(strbuf[0], '%');
    EXPECT_EQ(strbuf[1], '2');
    EXPECT_EQ(strbuf[2], '0');
    EXPECT_EQ(strbuf[3], '\0');
    EXPECT_EQ(strbuf[max - 1], '\0');

    // Test Unit; will fill trailing bytes with null
    ASSERT_EQ(::replace_escaped(strbuf, 0, &max), 1);
    EXPECT_EQ(strbuf[0], ' ');
    EXPECT_EQ(strbuf[1], '\0');
    EXPECT_EQ(strbuf[2], '\0');
    EXPECT_EQ(strbuf[3], '\0');
    EXPECT_EQ(max, sizeof(escstr) - 2);
}

TEST(UriTestSuite, replace_escaped_ip4) {
    const char escstr[]{"Hello%20%0AWorld%G0!%0x"};
    size_t max = sizeof(escstr);
    char strbuf[sizeof(escstr)];
    memset(strbuf, 0xFF, max);
    strcpy(strbuf, escstr);
    EXPECT_EQ(strbuf[max - 1], '\0');

    // Test Unit
    // The function converts only one escaped character and the index must
    // exactly point to its '%'.
    ASSERT_EQ(::replace_escaped(strbuf, 5, &max), 1);
    EXPECT_STREQ(strbuf, "Hello %0AWorld%G0!%0x");
    // max buffer length is reduced by two characters (redudce '%xx' to ' ').
    EXPECT_EQ(max, sizeof(escstr) - 2);
    EXPECT_EQ(max, strlen(strbuf) + 1);
    // The two trailing free characters are filled with '\0'.
    EXPECT_EQ(strbuf[max - 2], 'x');
    EXPECT_EQ(strbuf[max - 1], '\0'); // regular new delimiter
    EXPECT_EQ(strbuf[max], '\0');     // filled
    EXPECT_EQ(strbuf[max + 1], '\0'); // filled
    EXPECT_EQ(max + 2, sizeof(strbuf));

    // Not pointing to an escaped character
    EXPECT_EQ(::replace_escaped(strbuf, 0, &max), 0);
    // No hex values after escape character
    EXPECT_EQ(::replace_escaped(strbuf, 16, &max), 0);
    EXPECT_EQ(::replace_escaped(strbuf, 20, &max), 0);
    // Failures should not modify output.
    EXPECT_STREQ(strbuf, "Hello %0AWorld%G0!%0x");
    EXPECT_EQ(max, sizeof(escstr) - 2);
}


// remove_escaped_chars() function: tests from the uri module
// ==========================================================
class RemoveEscCharsPTestSuite
    : public ::testing::TestWithParam<
          //           escstr,      resultstr,   resultsize
          ::std::tuple<const char*, const char*, size_t>> {};

TEST_P(RemoveEscCharsPTestSuite, remove_escaped_chars) {
    // Get parameter
    ::std::tuple params = GetParam();
    const char* escstr = ::std::get<0>(params);
    size_t size{strlen(escstr)}; // without '\0'

    // string buffer; we have to set it with a constant because we cannot get
    // the string size from pointer escstr. But there is a guard (ASSERT_GT).
    // strbuf must be one byte greater for '\0' than the strlen of escstr.
    char strbuf[32];
    memset(strbuf, 0xAA, sizeof(strbuf)); // Fill with garbage.
    ASSERT_GT(sizeof(strbuf), size)
        << "Error: string buffer too small for testing. You must increase it "
           "in this test.\n";
    strcpy(strbuf, escstr); // with '\0'

    // Test Unit.
    EXPECT_EQ(::remove_escaped_chars(strbuf, &size), UPNP_E_SUCCESS);
    EXPECT_STREQ(strbuf, ::std::get<1>(params)); // new result string
    EXPECT_EQ(size, ::std::get<2>(params));      // new result size
}

INSTANTIATE_TEST_SUITE_P(
    uri, RemoveEscCharsPTestSuite,
    ::testing::Values(
        // clang-format off
        // Using reserved generic delimiters (RFC3986 2.2.), execpt %20.
        //                 escstr,                   resultstr,       resultsize
        ::std::make_tuple("%5BHello%20world%21%5D", "[Hello world!]", 14),
        ::std::make_tuple("", "", 0),
        ::std::make_tuple("%20", " ", 1),
        ::std::make_tuple("%3A%2F", ":/", 2),
        ::std::make_tuple("hello", "hello", 5)
        // clang-format on
        ));


TEST(UriDeathTest, remove_escaped_chars_edge_conditions) {
#if defined(__APPLE__) && !defined(DEBUG)
    // With this "Release" build there is a curious situation on MacOS. The
    // normal test with EXPECT_EQ(), as shown below, fails with a
    // "SIGTRAP***Exception". The EXPECT_DEATH() test exits with "failed to
    // die." The EXPECT_EXIT() test fails with "Signal(11)". Huh? Seems we have
    // a Schroedinger thing: the test itself modifies the test. Maybe it has
    // something to do with the 'assert()' debug function that is disabled
    // here? Anyway, the new code is fixed.

    //     EXPECT_EQ(::remove_escaped_chars(nullptr, nullptr),
    //               UPNP_E_SUCCESS);
    std::cout << CYEL "[    FIX   ] " CRES << __LINE__
              << ": Calling Unit with nullptr should not segfault.\n";

#elif defined(__APPLE__) && defined(DEBUG)
    EXPECT_DEATH({ ::remove_escaped_chars(nullptr, nullptr); }, ".*"); // Wrong!

#elif defined(__linux__) || defined(_MSC_VER)
    std::cout << CYEL "[    FIX   ] " CRES << __LINE__
              << ": Calling Unit with nullptr should not segfault.\n";
    EXPECT_DEATH({ ::remove_escaped_chars(nullptr, nullptr); }, ".*"); // Wrong!
#endif

#ifndef __APPLE__
    char strbuf[]{"hello"};
    size_t size{sizeof(strbuf) - 1};
    EXPECT_DEATH({ ::remove_escaped_chars(nullptr, &size); }, ".*");  // Wrong!
    EXPECT_DEATH({ ::remove_escaped_chars(strbuf, nullptr); }, ".*"); // Wrong!
#endif
}
#endif // UPnPsdk_WITH_NATIVE_PUPNP

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // Managed in gtest_main.inc
}
