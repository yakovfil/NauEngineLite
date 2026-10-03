// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <string>
#include <vector>

#include "nau/string/string.h"

using namespace std::string_literals;
using namespace nau::string_literals;

namespace nau::test
{

    TEST(NauString, baseConstructors)
    {
        nau::string c_str8u{u8"TEST: русские, 🤝  и ツ♫你好 symbols"};
        nau::string c_str16u{u"TEST: русские, 🤝  и ツ♫你好 symbols"};
        nau::string c_str32u{U"TEST: русские, 🤝  и ツ♫你好 symbols"};

        nau::string cs_str8u{u8"TEST: русские, 🤝  и ツ♫你好 symbols"s};
        nau::string cs_str16u{u"TEST: русские, 🤝  и ツ♫你好 symbols"s};
        nau::string cs_str32u{U"TEST: русские, 🤝  и ツ♫你好 symbols"s};

        nau::string as_str8u = u8"TEST: русские, 🤝  и ツ♫你好 symbols"s;
        nau::string as_str16u = u"TEST: русские, 🤝  и ツ♫你好 symbols"s;
        nau::string as_str32u = U"TEST: русские, 🤝  и ツ♫你好 symbols"s;

        nau::string ces_str8u{std::u8string(u8"TEST: русские, 🤝  и ツ♫你好 symbols")};
        nau::string ces_str16u{std::u16string(u"TEST: русские, 🤝  и ツ♫你好 symbols")};
        nau::string ces_str32u{std::u32string(U"TEST: русские, 🤝  и ツ♫你好 symbols")};

        nau::string aes_str8u = std::u8string(u8"TEST: русские, 🤝  и ツ♫你好 symbols");
        nau::string aes_str16u = std::u16string(u"TEST: русские, 🤝  и ツ♫你好 symbols");
        nau::string aes_str32u = std::u32string(U"TEST: русские, 🤝  и ツ♫你好 symbols");

        nau::string sl_str8u = u8"TEST: русские, 🤝  и ツ♫你好 symbols"_ns;
        nau::string sl_str16u = u"TEST: русские, 🤝  и ツ♫你好 symbols"_ns;
        nau::string sl_str32u = U"TEST: русские, 🤝  и ツ♫你好 symbols"_ns;

        nau::string cp_str = c_str8u;
        ASSERT_NE(cp_str.c_str(), c_str8u.c_str());

        nau::string cp1_str = c_str8u;
        auto c1Ptr = cp1_str.c_str();
        nau::string cp2_str = c_str8u;
        auto c2Ptr = cp2_str.c_str();
        nau::string m1_str = std::move(cp1_str);
        nau::string m2_str{std::move(cp2_str)};
        ASSERT_EQ(c1Ptr, m1_str.c_str());
        ASSERT_EQ(c2Ptr, m2_str.c_str());

        std::vector<nau::string> test_strings = {
            c_str8u,
            c_str16u,
            c_str32u,

            cs_str8u,
            cs_str16u,
            cs_str32u,

            as_str8u,
            as_str16u,
            as_str32u,

            ces_str8u,
            ces_str16u,
            ces_str32u,

            aes_str8u,
            aes_str16u,
            aes_str32u,

            aes_str8u,
            aes_str16u,
            aes_str32u,

            m1_str,
            m2_str};

        for (auto& string : test_strings)
        {
            ASSERT_EQ(string.length(), 32);
            for (int i = 0; i < string.length(); i++)
            {
                EXPECT_EQ(string[i], U"TEST: русские, 🤝  и ツ♫你好 symbols"[i]);
                EXPECT_EQ(string.at(i), U"TEST: русские, 🤝  и ツ♫你好 symbols"[i]);
            }

            ASSERT_EQ(string.size(), 51);
            for (int i = 0; i < string.size(); i++)
            {
                EXPECT_EQ(string.data()[i], u8"TEST: русские, 🤝  и ツ♫你好 symbols"[i]);
                EXPECT_EQ(string.c_str()[i], u8"TEST: русские, 🤝  и ツ♫你好 symbols"[i]);
            }
        }
    }

    TEST(NauString, locale_char)
    {
        nau::string c_str = "TEST: русские, 🤝  и ツ♫你好 symbols";
        nau::string w_str = L"TEST: русские, 🤝  и ツ♫你好 symbols";

        nau::string s_c_str = "TEST: русские, 🤝  и ツ♫你好 symbols"s;
        nau::string s_w_str = L"TEST: русские, 🤝  и ツ♫你好 symbols"s;

        nau::string es_c_str = std::string("TEST: русские, 🤝  и ツ♫你好 symbols");
        nau::string es_w_str = std::wstring(L"TEST: русские, 🤝  и ツ♫你好 symbols");

        std::vector<nau::string> test_strings = {
            c_str,
            w_str,

            s_c_str,
            s_w_str,

            es_c_str,
            es_w_str};

        for (auto& string : test_strings)
        {
            ASSERT_EQ(string.length(), 32);
            for (int i = 0; i < string.length(); i++)
            {
                EXPECT_EQ(string[i], U"TEST: русские, 🤝  и ツ♫你好 symbols"[i]);
            }
        }
    }

    TEST(NauString, iterrators)
    {
        nau::string test_str{u8"TEST: русские, 🤝  и ツ♫你好 symbols"};

        int i = 0;
        for (auto it = test_str.begin(); it < test_str.end(); i++, it++)
        {
            EXPECT_EQ(*it, U"TEST: русские, 🤝  и ツ♫你好 symbols"[i]);
        }
        EXPECT_EQ(i, test_str.length());
        i = 0;
        for (auto it = test_str.cbegin(); it < test_str.cend(); i++, it++)
        {
            EXPECT_EQ(*it, U"TEST: русские, 🤝  и ツ♫你好 symbols"[i]);
        }
        EXPECT_EQ(i, test_str.length());
        i = test_str.length();
        for (auto it = test_str.rbegin(); it < test_str.rend(); i--, it++)
        {
            EXPECT_EQ(*it, U"TEST: русские, 🤝  и ツ♫你好 symbols"[i - 1]);
        }
        EXPECT_EQ(i, 0);
        i = test_str.length();
        for (auto it = test_str.crbegin(); it < test_str.crend(); i--, it++)
        {
            EXPECT_EQ(*it, U"TEST: русские, 🤝  и ツ♫你好 symbols"[i - 1]);
        }
        EXPECT_EQ(i, 0);

        nau::string test_str2 = test_str;

        i = 0;
        for (auto it = test_str2.begin(); it < test_str2.end(); i++, it++)
        {
            EXPECT_EQ(*it, U"TEST: русские, 🤝  и ツ♫你好 symbols"[i]);
        }
        EXPECT_EQ(i, test_str.length());

        nau::string test_str3 = std::move(test_str);
        ASSERT_TRUE(test_str.empty());

        i = 0;
        for (auto it = test_str3.begin(); it < test_str3.end(); i++, it++)
        {
            EXPECT_EQ(*it, U"TEST: русские, 🤝  и ツ♫你好 symbols"[i]);
        }
        EXPECT_EQ(i, test_str3.length());

        nau::string sub_test_str = test_str3.substr(0, 0);
        ASSERT_TRUE(sub_test_str.empty());
        sub_test_str = test_str3.substr(1, 0);
        ASSERT_TRUE(sub_test_str.empty());
        sub_test_str = test_str3.substr(1, 1);
        sub_test_str = sub_test_str.substr(0, 0);
        ASSERT_TRUE(sub_test_str.empty());

        test_str = test_str3;

        sub_test_str = test_str.substr(7, 15);

        i = 0;
        for (auto it = sub_test_str.begin(); it < sub_test_str.end(); i++, it++)
        {
            EXPECT_EQ(*it, U"усские, 🤝  и ツ♫"[i]);
        }
        EXPECT_EQ(i, 15);
        i = 0;
        for (auto it = sub_test_str.cbegin(); it < sub_test_str.cend(); i++, it++)
        {
            EXPECT_EQ(*it, U"усские, 🤝  и ツ♫"[i]);
        }
        i = sub_test_str.length();
        for (auto it = sub_test_str.rbegin(); it < sub_test_str.rend(); i--, it++)
        {
            EXPECT_EQ(*it, U"усские, 🤝  и ツ♫"[i - 1]);
        }
        EXPECT_EQ(i, 0);
        i = sub_test_str.length();
        for (auto it = sub_test_str.crbegin(); it < sub_test_str.crend(); i--, it++)
        {
            EXPECT_EQ(*it, U"усские, 🤝  и ツ♫"[i - 1]);
        }
        EXPECT_EQ(i, 0);

        test_str2 = sub_test_str;

        i = 0;
        for (auto it = test_str2.begin(); it < test_str2.end(); i++, it++)
        {
            EXPECT_EQ(*it, U"усские, 🤝  и ツ♫"[i]);
        }
    }
}  // namespace nau::test