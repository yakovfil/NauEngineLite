// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// nau/utils/strings.h

#pragma once

#include <string>
#include <string_view>

#include "nau/kernel/kernel_config.h"

namespace nau::strings
{

    // Checked conversions: malformed input reports a diagnostic and returns empty.
    NAU_KERNEL_EXPORT std::string toUtf8(std::string_view text);
    NAU_KERNEL_EXPORT std::string toUtf8(std::u8string_view text);
    NAU_KERNEL_EXPORT std::string toUtf8(std::u16string_view text);
    NAU_KERNEL_EXPORT std::string toUtf8(std::u32string_view text);
    NAU_KERNEL_EXPORT std::string toUtf8(std::wstring_view text);
    NAU_KERNEL_EXPORT std::u16string utf8ToUtf16(std::string_view text);
    NAU_KERNEL_EXPORT std::u32string utf8ToUtf32(std::string_view text);
    NAU_KERNEL_EXPORT bool isValidUtf8(std::string_view text);

    // Nullable external C strings must be handled before constructing a view.
    template <typename Char>
    std::string toUtf8(const Char* text)
    {
        return text ? toUtf8(std::basic_string_view<Char>{text}) : std::string{};
    }

    // Code-point units, not graphemes. Valid UTF-8 is required; invalid input
    // reports a diagnostic and returns zero/empty. Slicing accepts the end
    // position, clamps count, and rejects positions beyond the end.
    NAU_KERNEL_EXPORT size_t codePointCount(std::string_view text);
    NAU_KERNEL_EXPORT std::string codePointSlice(std::string_view text, size_t position, size_t count = std::string_view::npos);

    NAU_KERNEL_EXPORT
    std::wstring utf8ToWString(std::u8string_view text);

    NAU_KERNEL_EXPORT
    std::u8string wstringToUtf8(std::wstring_view text);

    inline std::wstring utf8ToWString(std::string_view text)
    {
        return utf8ToWString(std::u8string_view{reinterpret_cast<const char8_t*>(text.data()), text.size()});
    }

    inline std::string_view toStringView(const std::u8string_view str)
    {
        static_assert(sizeof(char) == sizeof(char8_t));
        if (str.empty())
        {
            return {};
        }

        const char* const charPtr = reinterpret_cast<const char*>(str.data());
        return {charPtr, str.size()};
    }

    inline std::string_view toStringView(const std::string_view str)
    {
        static_assert(sizeof(char) == sizeof(char8_t));
        if (str.empty())
        {
            return {};
        }

        return {str.data(), str.size()};
    }

    inline std::wstring_view toStringView(const std::wstring_view str)
    {
        if (str.empty())
        {
            return {};
        }

        return {str.data(), str.size()};
    }

    inline std::u8string_view toU8StringView(const std::string_view str)
    {
        static_assert(sizeof(char) == sizeof(char8_t));
        if (str.empty())
        {
            return {};
        }

        return std::u8string_view{reinterpret_cast<const char8_t*>(str.data()), str.size()};
    }

}  // namespace nau::strings
