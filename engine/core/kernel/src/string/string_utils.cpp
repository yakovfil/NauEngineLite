// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// strings.cpp

#include "nau/string/string_utils.h"

#include <iterator>
#include <string_view>
#include <utility>

#include "nau/string/string_conv.h"

namespace nau::str_detail
{

    namespace
    {
        template <typename T>
        std::basic_string_view<T> splitNextImpl(std::basic_string_view<T> str, std::basic_string_view<T> current, std::basic_string_view<T> separators)
        {
            using StringView = std::basic_string_view<T>;

            if (str.empty())
            {
                return {};
            }

            size_t offset = current.empty() ? 0 : (current.data() - str.data()) + current.size();

            if (offset == str.size())
            {
                return {};
            }

            do
            {
                const auto pos = str.find_first_of(separators, offset);

                if (pos == StringView::npos)
                {
                    break;
                }

                if (pos != offset)
                {
                    return StringView(str.data() + offset, pos - offset);
                }

                ++offset;
            } while (offset < str.size());

            return offset == str.size() ? StringView{} : StringView(str.data() + offset, str.size() - offset);
        }

        template <typename T>
        std::basic_string_view<T> trimEndImpl(std::basic_string_view<T> str)
        {
            constexpr T Space = TYPED_STR(T, ' ');

            size_t spaces = 0;

            for (auto r = str.rbegin(); r != str.rend(); ++r)
            {
                if (*r != Space)
                {
                    break;
                }
                ++spaces;
            }

            return spaces == 0 ? str : std::basic_string_view<T>{str.data(), str.length() - spaces};
        }

        template <typename T>
        std::basic_string_view<T> trimStartImpl(std::basic_string_view<T> str)
        {
            constexpr T Space = TYPED_STR(T, ' ');

            auto iter = str.begin();

            for (; iter != str.end(); ++iter)
            {
                if (*iter != Space)
                {
                    break;
                }
            }

            const auto offset = std::distance(str.begin(), iter);

            // DEBUG_CHECK(offset >= 0)

            if (offset == str.length())
            {
                return {};
            }

            return offset == 0 ? str : std::basic_string_view<T>{str.data() + offset, (str.length() - offset)};
        }

        template <typename T>
        std::basic_string_view<T> trimIMpl(std::basic_string_view<T> str)
        {
            return trimStartImpl(trimEndImpl(str));
        }

        // template <typename T>
        // bool endWithImpl(std::basic_string_view<T> str, std::basic_string_view<T> value)
        // {
        //     const auto pos = str.rfind(value);
        //     return (pos != std::basic_string_view<T>::npos) && (str.length() - pos) == value.length();
        // }

        // template <typename T>
        // bool startWithImpl(std::basic_string_view<T> str, std::basic_string_view<T> value)
        // {
        //     return str.find(value) == 0;
        // }

    }  // namespace

    std::string_view splitNext(std::string_view str, std::string_view current, std::string_view separators)
    {
        return splitNextImpl(str, current, separators);
    }

    std::wstring_view splitNext(std::wstring_view str, std::wstring_view current, std::wstring_view separators)
    {
        return splitNextImpl(str, current, separators);
    }

    nau::string_view splitNext(nau::string_view str, nau::string_view current, nau::string_view separators)
    {
        if (str.empty() || separators.empty())
        {
            return {};
        }

        NAU_ASSERT(separators.size() == 1, "Currently multiply separators is not implemented");

        size_t offset = current.empty() ? 0 : (current.data() - str.data()) + current.size();

        if (offset == str.size())
        {
            return {};
        }

        const auto findSeparatorPos = [&, strSize = str.size(), separator = separators[0]](size_t offset)
        {
            for (size_t i = offset; i < strSize; ++i)
            {
                if (str[i] == separator)
                {
                    return i;
                }
            }

            return nau::string_view::npos;
        };

        do
        {
            const auto pos = findSeparatorPos(offset);

            if (pos == nau::string_view::npos)
            {
                break;
            }

            if (pos != offset)
            {
                return nau::string_view{str, pos - offset, offset};
            }

            ++offset;
        } while (offset < str.size());

        return offset == str.size() ? nau::string_view{} : nau::string_view{str, str.size() - offset, offset};
    }

}  // namespace nau::str_detail

namespace nau::strings
{

    std::pair<std::string_view, std::string_view> cut(std::string_view str, char separator)
    {
        if (str.size() < 2)
        {
            return {};
        }

        const auto index = str.find(separator);
        if (index == std::string_view::npos)
        {
            return {};
        }

        auto left = str.substr(0, index);
        auto right = str.substr(index + 1, str.size() - index - 1);

        return {left, right};
    }

    std::pair<std::u8string_view, std::u8string_view> cut(std::u8string_view str, char8_t separator)
    {
        auto [left, right] = strings::cut(toStringView(str), separator);
        return std::pair{toU8StringView(left), toU8StringView(right)};
    }

    std::string_view trimEnd(std::string_view str)
    {
        return str_detail::trimEndImpl(str);
    }

    std::wstring_view trimEnd(std::wstring_view str)
    {
        return str_detail::trimEndImpl(str);
    }

    std::string_view trimStart(std::string_view str)
    {
        return str_detail::trimStartImpl(str);
    }

    std::wstring_view trimStart(std::wstring_view str)
    {
        return str_detail::trimStartImpl(str);
    }

    std::string_view trim(std::string_view str)
    {
        return str_detail::trimIMpl(str);
    }

    std::wstring_view trim(std::wstring_view str)
    {
        return str_detail::trimIMpl(str);
    }

}  // namespace nau::strings
