// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// nau/utils/strings.h

#pragma once

#include <string>
#include <string_view>

#include "nau/kernel/kernel_config.h"

namespace nau::strings
{

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
