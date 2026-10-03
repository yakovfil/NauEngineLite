// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <string>
#include <unordered_map>

#include "nau/string/hash_string.h"

using namespace nau::string_literals;  // Necessary for operator "s"

namespace nau::test
{
    TEST(NauHashString, baseConstructors)
    {
        nau::string string1{u8"TEST: русские, 🤝  и ツ♫你好 symbols"};
        nau::string string2{u8"TEST: русские, 🤝  и ツ♫你好 symbols."};
        nau::string string3{u8"TEST: русские, 🤝  и ツ♫你好 symb0ls."};
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

        switch (nau::hash_string(u8"TEST: русские, 🤝  и ツ♫你好 symb0ls."))
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
                {      u8"T"_ns, 1},
                {u8"русские"_ns, 2},
                {     u8"🤝"_ns, 3},
                {   u8"你好"_ns, 4},
        };
        std::unordered_map<nau::hash_string, int> eastlMap =
            {
                {      u8"T"_ns, 1},
                {u8"русские"_ns, 2},
                {     u8"🤝"_ns, 3},
                {   u8"你好"_ns, 4},
        };

        stlMap.insert({u8"TEST: русские, 🤝  и ツ♫你好 symbols"_ns, 5});
        eastlMap.insert({u8"TEST: русские, 🤝  и ツ♫你好 symbols"_ns, 5});

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