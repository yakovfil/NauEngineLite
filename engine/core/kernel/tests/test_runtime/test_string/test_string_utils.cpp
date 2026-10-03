// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// test_strings.cpp

#include <vector>

#include "nau/string/string_utils.h"

namespace nau::test
{
    using namespace ::testing;

    TEST(TestStringUtils, Cut)
    {
        using StrPair = std::pair<std::string_view, std::string_view>;

        ASSERT_THAT(strings::cut(std::string_view{"one/two"}, '/'), Eq(StrPair{"one", "two"}));
        ASSERT_THAT(strings::cut(std::string_view{"one/two/three"}, '/'), Eq(StrPair{"one", "two/three"}));
        ASSERT_THAT(strings::cut(std::string_view{"one"}, '/'), Eq(StrPair{"", ""}));
        ASSERT_THAT(strings::cut(std::string_view{}, '/'), Eq(StrPair{"", ""}));
        ASSERT_THAT(strings::cut(std::string_view{"a,"}, ','), Eq(StrPair{"a", ""}));
        ASSERT_THAT(strings::cut(std::string_view{",b"}, ','), Eq(StrPair{"", "b"}));
    }

    TEST(TestStringUtils, Split)
    {
        nau::string str(u8"one;two;;three;");
        nau::string sep(u8";");

        std::vector<nau::string> strings;

        for (auto elem : strings::split(nau::string_view{str}, nau::string_view{sep}))
        {
            strings.emplace_back(elem);
        }

        ASSERT_EQ(strings.size(), 3);
        ASSERT_EQ(strings[0], u8"one");
        ASSERT_EQ(strings[1], u8"two");
        ASSERT_EQ(strings[2], u8"three");
    }

}  // namespace nau::test
