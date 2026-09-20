// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <iterator>
#include <string>

#include "nau/diag/assertion.h"
#include "utf8/core.h"

namespace nau_utf8
{
    inline bool is_valid(const char8_t* text)
    {
        return utf8::is_valid(text, text + std::char_traits<char8_t>::length(text));
    }

    template <typename Input, typename Output>
    void utf16to32(Input first, Input last, Output output)
    {
        std::u32string decoded;
        bool valid = true;
        while (first != last)
        {
            uint32_t cp = static_cast<uint32_t>(*first++);
            if constexpr (sizeof(typename std::iterator_traits<Input>::value_type) == 2)
            {
                if (cp >= 0xd800 && cp <= 0xdbff)
                {
                    if (first == last)
                    {
                        valid = false;
                        break;
                    }
                    const uint32_t low = static_cast<uint32_t>(*first++);
                    if (low < 0xdc00 || low > 0xdfff)
                    {
                        valid = false;
                        break;
                    }
                    cp = 0x10000 + ((cp - 0xd800) << 10) + (low - 0xdc00);
                }
            }
            if (!utf8::internal::is_code_point_valid(cp))
            {
                valid = false;
                break;
            }
            decoded.push_back(static_cast<char32_t>(cp));
        }
        if (!valid)
        {
            NAU_FAILURE_ALWAYS("Invalid UTF input string");
            return;
        }
        for (char32_t cp : decoded)
        {
            *output++ = cp;
        }
    }

    template <typename Input, typename Output>
    void utf8to32(Input first, Input last, Output output)
    {
        if (!utf8::is_valid(first, last))
        {
            NAU_FAILURE_ALWAYS("Invalid UTF-8 input string");
            return;
        }
        while (first != last)
        {
            utf8::utfchar32_t cp;
            utf8::internal::validate_next(first, last, cp);
            *output++ = static_cast<char32_t>(cp);
        }
    }

    template <typename Input, typename Output>
    void utf8to16(Input first, Input last, Output output)
    {
        std::u32string decoded;
        utf8to32(first, last, std::back_inserter(decoded));
        for (uint32_t cp : decoded)
        {
            if (cp <= 0xffff)
            {
                *output++ = static_cast<char16_t>(cp);
            }
            else
            {
                cp -= 0x10000;
                *output++ = static_cast<char16_t>(0xd800 + (cp >> 10));
                *output++ = static_cast<char16_t>(0xdc00 + (cp & 0x3ff));
            }
        }
    }
}  // namespace nau_utf8
