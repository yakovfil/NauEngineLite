// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/string/format.h"
#include "nau/string/string_conv.h"

namespace nau::test
{
    TEST(NauStringView, BorrowedBytesAndExplicitCodePoints)
    {
        const auto owner = strings::toUtf8(u8"A\u0416\u4f60\U0001f680");
        std::string_view view{owner};
        EXPECT_EQ(view.data(), owner.data());
        EXPECT_EQ(view.size(), 10u);
        EXPECT_EQ(strings::codePointCount(view), 4u);
        EXPECT_EQ(strings::codePointSlice(view, 1, 2), strings::toUtf8(u8"\u0416\u4f60"));
        view.remove_prefix(1);
        EXPECT_EQ(view.data(), owner.data() + 1);
        EXPECT_EQ(strings::codePointCount(view), 3u);
        view.remove_suffix(4);
        EXPECT_EQ(strings::utf8ToUtf32(view), U"\u0416\u4f60");
        EXPECT_TRUE(strings::codePointSlice(view, 2).empty());
    }
}  // namespace nau::test
