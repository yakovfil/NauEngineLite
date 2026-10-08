// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/string/format.h"
#include "nau/string/string_conv.h"

namespace nau::test
{
    TEST(NauString, ExplicitEncodingConstruction)
    {
        const auto bytes = strings::toUtf8(u8"A\u0416\u4f60\U0001f680");
        EXPECT_EQ(strings::toUtf8(u"A\u0416\u4f60\U0001f680"), bytes);
        EXPECT_EQ(strings::toUtf8(U"A\u0416\u4f60\U0001f680"), bytes);
        EXPECT_EQ(strings::toUtf8(strings::utf8ToWString(bytes)), bytes);
        EXPECT_EQ(strings::toUtf8(strings::utf8ToUtf16(bytes)), bytes);
        EXPECT_EQ(strings::toUtf8(strings::utf8ToUtf32(bytes)), bytes);
        const std::string embedded("a\0b", 3);
        EXPECT_EQ(strings::toUtf8(strings::utf8ToUtf16(embedded)), embedded);
        EXPECT_TRUE(strings::toUtf8(static_cast<const char*>(nullptr)).empty());
    }
}  // namespace nau::test
