// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "fmt/chrono.h"
#include "nau/string/string.h"

using namespace std::string_literals;  // Necessary for operator "s"

namespace nau::test
{
    TEST(NauString, comparison_operators)
    {
#define TEST_STRINGS(test_strings)                               \
    {                                                            \
        for (int i = 0; i < 10; i++)                             \
        {                                                        \
            EXPECT_EQ(test_strings[i], test_strings[i]);         \
            auto copy = test_strings[i];                         \
            EXPECT_EQ(test_strings[i], copy);                    \
            if (i != 9)                                          \
            {                                                    \
                ASSERT_NE(test_strings[i], test_strings[i + 1]); \
                ASSERT_LT(test_strings[i], test_strings[i + 1]); \
                ASSERT_LE(test_strings[i], test_strings[i + 1]); \
                                                                 \
                ASSERT_GT(test_strings[i + 1], test_strings[i]); \
                ASSERT_GE(test_strings[i + 1], test_strings[i]); \
            }                                                    \
        }                                                        \
    };

        nau::string s00 = u8"";
        nau::string s01 = u8"1";
        nau::string s02 = u8"2";
        nau::string s03 = u8"a";
        nau::string s04 = u8"b";

        nau::string s05 = u8"б";
        nau::string s06 = u8"в";

        nau::string s07 = u8"你";
        nau::string s08 = u8"好";

        nau::string s09 = u8"🤝";

        std::vector<nau::string> test_strings1 = {s00, s01, s02, s03, s04, s05, s06, s07, s08, s09};
        TEST_STRINGS(test_strings1);

        nau::string s10 = u8"a";
        nau::string s11 = u8"a1";
        nau::string s12 = u8"a2";
        nau::string s13 = u8"aa";
        nau::string s14 = u8"ab";

        nau::string s15 = u8"aб";
        nau::string s16 = u8"aв";

        nau::string s17 = u8"a你";
        nau::string s18 = u8"a好";

        nau::string s19 = u8"a🤝";

        std::vector<nau::string> test_strings2 = {s10, s11, s12, s13, s14, s15, s16, s17, s18, s19};
        TEST_STRINGS(test_strings2);

        nau::string s20 = u8"🤝aм好ツ";
        nau::string s21 = u8"🤝aм好ツ1";
        nau::string s22 = u8"🤝aм好ツ2";
        nau::string s23 = u8"🤝aм好ツa";
        nau::string s24 = u8"🤝aм好ツb";

        nau::string s25 = u8"🤝aм好ツб";
        nau::string s26 = u8"🤝aм好ツв";

        nau::string s27 = u8"🤝aм好ツ你";
        nau::string s28 = u8"🤝aм好ツ好";

        nau::string s29 = u8"🤝aм好ツ🤝";

        std::vector<nau::string> test_strings3 = {s20, s21, s22, s23, s24, s25, s26, s27, s28, s29};
        TEST_STRINGS(test_strings3);
    }

    TEST(NauString, subscript_operators)
    {
        nau::string str = u8"TEST: pусские, 🤝  и ツ♫你好 symbols";

        for (int i = 0; i < str.length(); i++)
        {
            EXPECT_EQ(str[i], U"TEST: pусские, 🤝  и ツ♫你好 symbols"[i]);
        }

        str[0] = U'a';
        EXPECT_EQ(str, u8"aEST: pусские, 🤝  и ツ♫你好 symbols");
        str[1] = U'🤝';
        EXPECT_EQ(str, u8"a🤝ST: pусские, 🤝  и ツ♫你好 symbols");
        str[4] = U'ツ';
        EXPECT_EQ(str, u8"a🤝STツ pусские, 🤝  и ツ♫你好 symbols");
        str[22] = U'y';
        EXPECT_EQ(str, u8"a🤝STツ pусские, 🤝  и ツ♫y好 symbols");
        str[23] = U'б';
        EXPECT_EQ(str, u8"a🤝STツ pусские, 🤝  и ツ♫yб symbols");
    }

    TEST(NauString, substring)
    {
        nau::string str = u8"TEST: pусские, 🤝  и ツ♫你好 symbols";

        EXPECT_EQ(str.substr(0, 10), u8"TEST: pусс");
        EXPECT_EQ(str.substr(10, 22), u8"кие, 🤝  и ツ♫你好 symbols");
        str[0] = U'a';
        EXPECT_EQ(str.substr(0, 10), u8"aEST: pусс");
        EXPECT_EQ(str.substr(10, 22), u8"кие, 🤝  и ツ♫你好 symbols");
        str[1] = U'🤝';
        EXPECT_EQ(str.substr(0, 10), u8"a🤝ST: pусс");
        EXPECT_EQ(str.substr(10, 22), u8"кие, 🤝  и ツ♫你好 symbols");
        str[4] = U'ツ';
        EXPECT_EQ(str.substr(0, 10), u8"a🤝STツ pусс");
        EXPECT_EQ(str.substr(10, 22), u8"кие, 🤝  и ツ♫你好 symbols");
        str[22] = U'y';
        str[23] = U'б';
        EXPECT_EQ(str.substr(0, 10), u8"a🤝STツ pусс");
        EXPECT_EQ(str.substr(10, 22), u8"кие, 🤝  и ツ♫yб symbols");
    }

    TEST(NauString, hashEastl)
    {
        using EastlU8String = std::basic_string<char8_t>;
        const auto text = u8"text_1";

        const size_t hash1 = std::hash<nau::string>{}(nau::string{text});
        const size_t hash2 = std::hash<nau::string>{}(nau::string{text});
        const size_t hash3 = std::hash<EastlU8String>{}(EastlU8String{text});

        ASSERT_EQ(hash1, hash2);
        ASSERT_EQ(hash1, hash3);
    }

    TEST(NauString, hashEastl_2)
    {
        std::unordered_map<nau::string, unsigned> values;

        values[nau::string{u8"one"}] = 11;
        values[nau::string{u8"two"}] = 22;

        ASSERT_EQ(values[nau::string{u8"one"}], 11);
        ASSERT_EQ(values[nau::string{u8"two"}], 22);
    }

    TEST(NauString, hashStd)
    {
        const auto text = u8"text_1";

        const size_t hash1 = std::hash<nau::string>{}(nau::string{text});
        const size_t hash2 = std::hash<nau::string>{}(nau::string{text});

        ASSERT_EQ(hash1, hash2);
    }

    TEST(NauString, hashStd_2)
    {
        const nau::string str1{u8"text_1"};
        const std::string str2{"text_1"};

        std::unordered_map<nau::string, unsigned> values;

        values[nau::string{u8"one"}] = 11;
        values[nau::string{u8"two"}] = 22;

        ASSERT_EQ(values[nau::string{u8"one"}], 11);
        ASSERT_EQ(values[nau::string{u8"two"}], 22);
    }

    std::tm make_tm(int year, int mon, int mday, int hour, int min, int sec)
    {
        auto tm = std::tm();
        tm.tm_sec = sec;
        tm.tm_min = min;
        tm.tm_hour = hour;
        tm.tm_mday = mday;
        tm.tm_mon = mon - 1;
        tm.tm_year = year - 1900;
        return tm;
    }

    TEST(NauString, format)
    {
        nau::string buffer;
        nau::string::format_to(buffer, u8"{}", 12345);
        EXPECT_EQ(buffer, u8"12345");

        // static_assert(fmt::is_formattable<nau::nau_string>::value);
        buffer = nau::string(u8"{}🤝{}");
        auto str1 = nau::string(u8"123");
        auto str2 = nau::string(u8"45");
        buffer.format_inline(str1, str2);
        EXPECT_EQ(buffer, u8"123🤝45");

        buffer = nau::string(u8"{} {}");
        buffer.format_inline(u8"123", 45);
        EXPECT_EQ(buffer, u8"123 45");

        const auto tm = make_tm(1970, 1, 1, 1, 2, 3);

        EXPECT_EQ(nau::string(u8"{:%I,%H,%M,%S}").format(tm), u8"01,01,02,03");
        EXPECT_EQ(nau::string::format(nau::string(u8"{:%0I,%0H,%0M,%0S}"), tm), u8"01,01,02,03");
        EXPECT_EQ(nau::string::format(nau::string(u8"{:%_I,%_H,%_M,%_S}"), tm), u8" 1, 1, 2, 3");
        EXPECT_EQ(nau::string::format(nau::string(u8"{:%-I,%-H,%-M,%-S}"), tm), u8"1,1,2,3");

        EXPECT_EQ(nau::string::format(nau::string(u8"{:%OI,%OH,%OM,%OS}"), tm), u8"01,01,02,03");
        EXPECT_EQ(nau::string::format(nau::string(u8"{:%0OI,%0OH,%0OM,%0OS}"), tm), u8"01,01,02,03");
        EXPECT_EQ(nau::string::format(nau::string(u8"{:%_OI,%_OH,%_OM,%_OS}"), tm), u8" 1, 1, 2, 3");
        EXPECT_EQ(nau::string::format(nau::string(u8"{:%-OI,%-OH,%-OM,%-OS}"), tm), u8"1,1,2,3");
    }

    TEST(NauString, appendFormat)
    {
        nau::string buffer;
        buffer.append_format(u8"{}", 12345);
        EXPECT_EQ(buffer, u8"12345");
        buffer.append_format(u8"{}{}", "*", buffer);
        EXPECT_EQ(buffer, u8"12345*12345");
    }
}  // namespace nau::test