// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <string>
#include <unordered_map>

#include "nau/string/hash_string.h"

using namespace std::string_literals;
using namespace nau::string_literals;  // Necessary for operator "s"

namespace nau::test
{
    TEST(NauHashString, baseConstructors)
    {
        std::string string1{nau::strings::toUtf8(u8"TEST: русские, 🤝  и ツ♫你好 symbols")};
        std::string string2{nau::strings::toUtf8(u8"TEST: русские, 🤝  и ツ♫你好 symbols.")};
        std::string string3{nau::strings::toUtf8(u8"TEST: русские, 🤝  и ツ♫你好 symb0ls.")};
        nau::hash_string hashed_string1 = string1;
        nau::hash_string hashed_string2 = string2;
        nau::hash_string hashed_string3 = string3;

        EXPECT_NE(hashed_string1, hashed_string2);
        EXPECT_NE(hashed_string1, hashed_string3);

        EXPECT_NE(hashed_string2, hashed_string1);
        EXPECT_NE(hashed_string2, hashed_string3);

        EXPECT_NE(hashed_string3, hashed_string1);
        EXPECT_NE(hashed_string3, hashed_string2);

        EXPECT_EQ(hashed_string1.toString(), string1);
        EXPECT_EQ(hashed_string2.toString(), string2);
        EXPECT_EQ(hashed_string3.toString(), string3);

        nau::hash_string hashed_string1c = string1;

        EXPECT_EQ(hashed_string1, hashed_string1c);

        switch (nau::hash_string(nau::strings::toUtf8(u8"TEST: русские, 🤝  и ツ♫你好 symb0ls.")))
        {
            case u8"TEST: русские, 🤝  и ツ♫你好 symbols"_sh:
            {
                ASSERT_TRUE(false);
                break;
            }
            case u8"TEST: русские, 🤝  и ツ♫你好 symbols."_sh:
            {
                ASSERT_TRUE(false);
                break;
            }
            case u8"TEST: русские, 🤝  и ツ♫你好 symb0ls."_sh:
            {
                ASSERT_TRUE(true);
                break;
            }
            default:
            {
                ASSERT_TRUE(false);
                break;
            }
        }
    }

    TEST(NauHashString, containers)
    {
        std::unordered_map<nau::hash_string, int> stlMap =
            {
                {      nau::strings::toUtf8(u8"T"), 1},
                {nau::strings::toUtf8(u8"русские"), 2},
                {     nau::strings::toUtf8(u8"🤝"), 3},
                {   nau::strings::toUtf8(u8"你好"), 4},
        };
        std::unordered_map<nau::hash_string, int> eastlMap =
            {
                {      nau::strings::toUtf8(u8"T"), 1},
                {nau::strings::toUtf8(u8"русские"), 2},
                {     nau::strings::toUtf8(u8"🤝"), 3},
                {   nau::strings::toUtf8(u8"你好"), 4},
        };

        stlMap.insert({nau::strings::toUtf8(u8"TEST: русские, 🤝  и ツ♫你好 symbols"), 5});
        eastlMap.insert({nau::strings::toUtf8(u8"TEST: русские, 🤝  и ツ♫你好 symbols"), 5});

        EXPECT_EQ(stlMap[u8"T"_sh], 1);
        EXPECT_EQ(stlMap[u8"русские"_sh], 2);
        EXPECT_EQ(stlMap[u8"🤝"_sh], 3);
        EXPECT_EQ(stlMap[u8"你好"_sh], 4);
        EXPECT_EQ(stlMap[u8"TEST: русские, 🤝  и ツ♫你好 symbols"_sh], 5);

        EXPECT_EQ(eastlMap[u8"T"_sh], 1);
        EXPECT_EQ(eastlMap[u8"русские"_sh], 2);
        EXPECT_EQ(eastlMap[u8"🤝"_sh], 3);
        EXPECT_EQ(eastlMap[u8"你好"_sh], 4);
        EXPECT_EQ(eastlMap[u8"TEST: русские, 🤝  и ツ♫你好 symbols"_sh], 5);
    }
}  // namespace nau::test