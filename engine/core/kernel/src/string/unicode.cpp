// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/string/string_conv.h"
#include "nau/string/utf8_noexcept.h"

namespace nau::strings
{
    bool isValidUtf8(std::string_view text)
    {
        return text.empty() || utf8::is_valid(text.begin(), text.end());
    }

    std::string toUtf8(std::string_view text)
    {
        if (!isValidUtf8(text))
        {
            NAU_FAILURE_ALWAYS("Invalid UTF-8 input string");
            return {};
        }
        return std::string{text};
    }

    std::string toUtf8(std::u8string_view text)
    {
        return toUtf8(toStringView(text));
    }

    std::u32string utf8ToUtf32(std::string_view text)
    {
        std::u32string result;
        if (!text.empty())
        {
            nau_utf8::utf8to32(text.begin(), text.end(), std::back_inserter(result));
        }
        return result;
    }

    std::u16string utf8ToUtf16(std::string_view text)
    {
        std::u16string result;
        if (!text.empty())
        {
            nau_utf8::utf8to16(text.begin(), text.end(), std::back_inserter(result));
        }
        return result;
    }

    std::string toUtf8(std::u32string_view text)
    {
        for (char32_t cp : text)
        {
            if (!utf8::internal::is_code_point_valid(static_cast<uint32_t>(cp)))
            {
                NAU_FAILURE_ALWAYS("Invalid Unicode scalar input string");
                return {};
            }
        }
        std::string result;
        for (char32_t cp : text)
        {
            utf8::internal::append(static_cast<uint32_t>(cp), std::back_inserter(result));
        }
        return result;
    }

    std::string toUtf8(std::u16string_view text)
    {
        std::u32string decoded;
        nau_utf8::utf16to32(text.begin(), text.end(), std::back_inserter(decoded));
        return toUtf8(std::u32string_view{decoded});
    }

    std::string toUtf8(std::wstring_view text)
    {
        std::u32string decoded;
        nau_utf8::utf16to32(text.begin(), text.end(), std::back_inserter(decoded));
        return toUtf8(std::u32string_view{decoded});
    }

    size_t codePointCount(std::string_view text)
    {
        return utf8ToUtf32(text).size();
    }

    std::wstring utf8ToWString(std::u8string_view text)
    {
        const auto bytes = toStringView(text);
        std::wstring result;
        if constexpr (sizeof(wchar_t) == 2)
        {
            for (char16_t unit : utf8ToUtf16(bytes))
            {
                result.push_back(static_cast<wchar_t>(unit));
            }
        }
        else
        {
            for (char32_t cp : utf8ToUtf32(bytes))
            {
                result.push_back(static_cast<wchar_t>(cp));
            }
        }
        return result;
    }

    std::u8string wstringToUtf8(std::wstring_view text)
    {
        const auto bytes = toUtf8(text);
        const auto view = toU8StringView(bytes);
        return std::u8string{view};
    }

    std::string codePointSlice(std::string_view text, size_t position, size_t count)
    {
        const auto decoded = utf8ToUtf32(text);
        if (position > decoded.size())
        {
            NAU_FAILURE_ALWAYS("Code-point position is out of range");
            return {};
        }
        return toUtf8(std::u32string_view{decoded}.substr(position, count));
    }
}  // namespace nau::strings
