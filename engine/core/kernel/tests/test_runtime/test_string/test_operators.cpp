// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/string/format.h"
#include "nau/string/string_conv.h"

namespace nau::test
{
    TEST(NauString, ExplicitCharacterMutationAndSlicing)
    {
        const auto bytes = strings::toUtf8(u8"A\u0416\u4f60\U0001f680");
        auto codePoints = strings::utf8ToUtf32(bytes);
        EXPECT_EQ(codePoints, U"A\u0416\u4f60\U0001f680");
        codePoints[0] = U'\U0001f600';
        codePoints[3] = U'B';
        const auto changed = strings::toUtf8(codePoints);
        EXPECT_EQ(changed, strings::toUtf8(u8"\U0001f600\u0416\u4f60"
                                           "B"));
        EXPECT_EQ(strings::codePointSlice(changed, 1, 2), strings::toUtf8(u8"\u0416\u4f60"));
        EXPECT_EQ(strings::codePointSlice(changed, 2, 100), strings::toUtf8(u8"\u4f60"
                                                                            "B"));
    }
    TEST(NauString, StandardFormatting)
    {
        const std::string value("a\0b", 3);
        EXPECT_EQ(utils::format("{}", value), value);
        EXPECT_EQ(utils::format(std::string("{}\0{}", 5), 'a', 'b'), value);
        EXPECT_EQ(utils::format("{}", strings::toUtf8(u8"\u4f60\u597d")), strings::toUtf8(u8"\u4f60\u597d"));
    }
}  // namespace nau::test
