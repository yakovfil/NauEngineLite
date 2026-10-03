// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <gtest/gtest.h>

#include <unordered_set>

#include "nau/serialization/json_utils.h"
#include "nau/string/hash_string.h"
#include "nau/string/string.h"

TEST(StandardMigration, SizedStringsAndFormatters)
{
    const char exact[] = {'a', 'b'};
    EXPECT_EQ(nau::string(std::string_view(exact, 2)).tostring(), "ab");
    EXPECT_EQ(nau::utils::format("{}", std::string_view(exact, 2)), "ab");
    const std::string bytes("a\0b", 3);
    const std::u8string utf8(u8"a\0b", 3);
    const nau::string value(bytes);
    EXPECT_EQ(value.tostring(), bytes);
    EXPECT_EQ(value.tou8string(), utf8);
    EXPECT_EQ(fmt::format(u8"{}", value), utf8);
    EXPECT_EQ(nau::strings::wstringToUtf8(nau::strings::utf8ToWString(utf8)), utf8);
    EXPECT_EQ(value.tou16string(), std::u16string(u"a\0b", 3));
    EXPECT_EQ(value.tou32string(), std::u32string(U"a\0b", 3));
    EXPECT_EQ(nau::string(std::u16string(u"a\0b", 3)).tostring(), bytes);
    EXPECT_EQ(nau::string(std::u32string(U"a\0b", 3)).tostring(), bytes);
    EXPECT_EQ(nau::utils::format("{}", value), bytes);
    EXPECT_EQ(nau::string::format("{}", value).tostring(), bytes);
    EXPECT_EQ(nau::utils::format("{}", std::u8string_view(utf8)), bytes);
    EXPECT_EQ(nau::diag_detail::makeFailureMessage("{}", std::string_view(bytes)), bytes);
    EXPECT_EQ(nau::diag_detail::makeFailureMessage(std::u8string_view(utf8)), bytes);
    EXPECT_EQ(nau::diag_detail::makeFailureMessage("{}", static_cast<const char*>(nullptr)), "NULLPTR");
    EXPECT_EQ(nau::utils::format("[{}]", std::string_view{}), "[]");
    const std::string pattern("{}\0{}", 5);
    EXPECT_EQ(nau::string::format(pattern, 'a', 'b').tostring(), bytes);
    EXPECT_EQ(nau::diag_detail::makeFailureMessage(std::string_view(pattern), 'a', 'b'), bytes);
    EXPECT_EQ(std::hash<nau::string>{}(value), std::hash<std::u8string_view>{}(utf8));
    const std::unordered_set<nau::string> distinct{value, nau::string("a")};
    EXPECT_EQ(distinct.size(), 2u);
}

TEST(StandardMigration, UnicodeConversions)
{
    const std::u8string utf8 = u8"A\u0416\U0001f680";
    const std::u16string utf16 = u"A\u0416\U0001f680";
    const std::u32string utf32 = U"A\u0416\U0001f680";
    for (const auto& value : {nau::string(utf8), nau::string(utf16), nau::string(utf32)})
    {
        EXPECT_EQ(value.tou8string(), utf8);
        EXPECT_EQ(fmt::format(u8"{}", value), utf8);
        EXPECT_EQ(nau::strings::wstringToUtf8(nau::strings::utf8ToWString(utf8)), utf8);
        EXPECT_EQ(value.tou16string(), utf16);
        EXPECT_EQ(value.tou32string(), utf32);
    }
}

TEST(StandardMigration, PersistentHashAndJsonFixtures)
{
    constexpr size_t expected = sizeof(size_t) == 8 ? static_cast<size_t>(0xe71fa2190541574bull) : size_t{0x1a47e90bu};
    EXPECT_EQ(nau::hash_string::constHash(u8"abc", 3), expected);
    EXPECT_EQ(std::hash<nau::hash_string>{}(nau::hash_string(nau::string("abc"))), expected);
    const std::vector<std::string> values{"alpha", std::string("a\0b", 3), "omega"};
    using nau::serialization::JsonUtils;
    const auto encoded = JsonUtils::stringify(values);
    const auto decoded = JsonUtils::parse<std::vector<std::string>>(encoded);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(*decoded, values);
    EXPECT_EQ(JsonUtils::stringify(std::vector<int>{1, 2, 3}), u8"[1,2,3]");
}

TEST(StdMigration, NullableCharacterPointersRemainEmpty)
{
    const char* narrow = nullptr;
    const char8_t* utf8 = nullptr;
    const char16_t* utf16 = nullptr;
    const char32_t* utf32 = nullptr;
    const wchar_t* wide = nullptr;
    EXPECT_TRUE(nau::string(narrow).empty());
    EXPECT_TRUE(nau::string(utf8).empty());
    EXPECT_TRUE(nau::string(utf16).empty());
    EXPECT_TRUE(nau::string(utf32).empty());
    EXPECT_TRUE(nau::string(wide).empty());
}
