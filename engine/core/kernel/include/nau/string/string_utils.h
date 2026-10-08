// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// nau/utils/strings.h

#pragma once

#include <ctype.h>

#include <algorithm>
#include <charconv>
#include <concepts>
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "nau/diag/assertion.h"
#include "nau/kernel/kernel_config.h"
#include "nau/string/format.h"
#include "nau/string/string_conv.h"

namespace nau::str_detail
{

    template <typename T>
    constexpr inline auto choose([[maybe_unused]] const wchar_t* wstr, [[maybe_unused]] const char* str)
    {
        if constexpr (std::is_same_v<T, wchar_t>)
        {
            return wstr;
        }
        else
        {
            return str;
        }
    }

    template <typename T>
    constexpr inline auto choose([[maybe_unused]] wchar_t wchr, [[maybe_unused]] char chr)
    {
        if constexpr (std::is_same_v<T, wchar_t>)
        {
            return wchr;
        }
        else
        {
            return chr;
        }
    }


    NAU_KERNEL_EXPORT
    std::wstring_view splitNext(std::wstring_view str, std::wstring_view current, std::wstring_view separators);

    NAU_KERNEL_EXPORT
    std::string_view splitNext(std::string_view str, std::string_view current, std::string_view separators);

    // template <typename C>
    // auto splitImpl(std::basic_string_view<C> text, std::basic_string_view<C> separators)
    // {
    //     return SplitSequence<C>(text, separators);
    // }

}  // namespace nau::str_detail

namespace nau::strings
{
    template <typename T>
    concept KnownStringView =
        std::is_same_v<std::string_view, T> ||
        std::is_same_v<std::wstring_view, T>;

    template <KnownStringView StringView>
    struct SplitSequence
    {
        class iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = StringView;

            iterator() = default;

            iterator(StringView str, StringView separators, StringView current = {}) :
                m_str(str),
                m_separators(separators),
                m_current(current)
            {
            }

            bool operator==(const iterator& other) const
            {
                if (m_current.empty() || other.m_current.empty())
                {
                    return m_current.empty() && other.m_current.empty();
                }

                return m_current.data() == other.m_current.data();
            }

            bool operator!=(const iterator& other) const
            {
                return !this->operator==(other);
            }

            iterator& operator++()
            {
                if (m_current = str_detail::splitNext(m_str, m_current, m_separators); m_current.empty())
                {
                    m_str = {};
                    m_separators = {};
                }

                return *this;
            }

            iterator operator++(int)
            {
                auto iter = *this;
                this->operator++();

                return iter;
            }

            StringView operator*() const
            {
                NAU_ASSERT(!m_current.empty(), "Iterator is not dereferenceable");
                return m_current;
            }

            StringView* operator->() const
            {
                NAU_ASSERT(!m_current.empty(), "Iterator is not dereferenceable");
                return &m_current;
            }

        private:
            StringView m_str;
            StringView m_separators;
            StringView m_current;
        };

        StringView str;
        StringView separators;

        SplitSequence() = default;

        SplitSequence(StringView inStr, StringView inSeparators) :
            str(inStr),
            separators(inSeparators)
        {
        }

        iterator begin() const
        {
            StringView next = str_detail::splitNext(str, {}, separators);
            return next.empty() ? end() : iterator{str, separators, next};
        }

        iterator end() const
        {
            return {};
        }
    };

    template <typename C, typename... A>
    SplitSequence(std::basic_string_view<C, A...>, std::basic_string_view<C, A...>) -> SplitSequence<std::basic_string_view<C, A...>>;

    SplitSequence(std::string_view, std::string_view) -> SplitSequence<std::string_view>;

    inline char lower(char ch)
    {
        const auto res = tolower(static_cast<unsigned char>(ch));
        return static_cast<char>(res);
    }

    inline char upper(char ch)
    {
        const auto res = toupper(static_cast<unsigned char>(ch));
        return static_cast<char>(res);
    }

    inline bool isUpper(char ch)
    {
        return isupper(static_cast<int>(ch)) != 0;
    }

    inline bool isLower(char ch)
    {
        return islower(static_cast<int>(ch)) != 0;
    }

    inline wchar_t lower(wchar_t wch)
    {
        const auto res = towlower(static_cast<unsigned short>(wch));
        return static_cast<wchar_t>(res);
    }

    inline wchar_t upper(wchar_t wch)
    {
        const auto res = towupper(static_cast<unsigned short>(wch));
        return static_cast<wchar_t>(res);
    }

    inline bool isUpper(wchar_t ch)
    {
        return iswupper(ch) != 0;
    }

    inline bool isLower(wchar_t ch)
    {
        return iswlower(ch) != 0;
    }

    template <typename C, typename... A>
    auto split(std::basic_string_view<C, A...> str, std::basic_string_view<C, A...> separators)
    {
        // auto inStr = std::basic_string_view{text};
        // auto inSeparators = std::basic_string_view{separators};

        return SplitSequence{str, separators};
    }

    inline auto split(std::string_view str, std::string_view separators)
    {
        return SplitSequence{str, separators};
    }

    NAU_KERNEL_EXPORT
    std::pair<std::string_view, std::string_view> cut(std::string_view str, char separator);

    NAU_KERNEL_EXPORT
    std::pair<std::u8string_view, std::u8string_view> cut(std::u8string_view str, char8_t separator);

    NAU_KERNEL_EXPORT
    std::string_view trimEnd(std::string_view str);

    NAU_KERNEL_EXPORT
    std::wstring_view trimEnd(std::wstring_view str);

    NAU_KERNEL_EXPORT
    std::string_view trimStart(std::string_view str);

    NAU_KERNEL_EXPORT
    std::wstring_view trimStart(std::wstring_view str);

    NAU_KERNEL_EXPORT
    std::string_view trim(std::string_view str);

    NAU_KERNEL_EXPORT
    std::wstring_view trim(std::wstring_view str);

    // bool endWith(std::string_view str, std::string_view);

    // bool endWith(std::wstring_view str, std::wstring_view);

    // bool startWith(std::string_view str, std::string_view);

    // bool startWith(std::wstring_view str, std::wstring_view);

    // template<size_t N, typename T, typename Chars>
    // auto splitToArray(std::basic_string_view<T> str, Chars separators) requires std::is_constructible_v<std::basic_string_view<T>, Chars>
    //{
    //	static_assert(N > 0);
    //
    //	std::array<std::basic_string_view<T>, N> result;
    //	std::basic_string_view<T> current;
    //
    //	for (size_t i = 0; i < result.size(); ++i)
    //	{
    //		current = splitNext(str, current, separators);
    //		if (current.empty())
    //		{
    //			break;
    //		}
    //
    //		result[i] = current;
    //	}
    //
    //	return result;
    // }

    /// <summary>
    /// Perform 'basic' case insensitive string comparison.
    /// </summary>
    template <typename It1, typename It2>
    inline bool icaseEqual(It1 begin1, It1 end1, It2 begin2, It2 end2) noexcept
    {
        const auto len1 = std::distance(begin1, end1);
        const auto len2 = std::distance(begin2, end2);

        if (len1 != len2)
        {
            return false;
        }

        auto iter1 = begin1;
        auto iter2 = begin2;

        while (iter1 != end1)
        {
            const auto ch1 = *iter1;
            const auto ch2 = *iter2;

            if (lower(ch1) != lower(ch2))
            {
                return false;
            }

            ++iter1;
            ++iter2;
        }

        return true;
    }

    template <typename It1, typename It2>
    inline int icaseCompare(It1 begin1, It1 end1, It2 begin2, It2 end2) noexcept
    {
        static_assert(std::is_same_v<decltype(*begin1), decltype(*begin2)>, "String type mismatch");

        auto p = std::mismatch(begin1, end1, begin2, end2, [](auto c1, auto c2)
        {
            return lower(c1) == lower(c2);
        });

        if (p.first == end1)
        {
            return p.second == end2 ? 0 : -1;
        }
        else if (p.second == end2)
        {
            return 1;
        }

        const auto ch1 = lower(*p.first);
        const auto ch2 = lower(*p.second);

        //	DEBUG_CHECK(ch1 != ch2)

        return ch1 < ch2 ? -1 : 1;
    }

    inline bool icaseEqual(std::string_view str1, std::string_view str2)
    {
        return icaseEqual(std::begin(str1), std::end(str1), std::begin(str2), std::end(str2));
    }

    inline bool icaseEqual(std::wstring_view str1, std::wstring_view str2)
    {
        return icaseEqual(std::begin(str1), std::end(str1), std::begin(str2), std::end(str2));
    }

    template <typename Char>
    int icaseCompare(std::basic_string_view<Char> str1, std::basic_string_view<Char> str2) noexcept
    {
        return icaseCompare(std::begin(str1), std::end(str1), std::begin(str2), std::end(str2));
    }

    template <typename String = std::string_view>
    struct CiStringComparer
    {
        using is_transparent = int;

        template <typename Left, typename Right>
        requires(std::is_constructible_v<String, Left> && std::is_constructible_v<String, Right>)
        bool operator()(const Left& str1, const Right& str2) const noexcept
        {
            return icaseCompare(String{str1}, String{str2}) < 0;
        }
    };

    template <typename Number>
    Number lexicalCast(std::string_view str)
    requires(std::is_arithmetic_v<Number>)
    {
        Number number{};

        const auto [ptr, err] = std::from_chars(str.data(), str.data() + str.size(), number);
        if (err == std::errc::invalid_argument)
        {
            return 0;
        }

        [[maybe_unused]]
        const bool parseAll = ptr == str.data() + str.size();
        return number;
    }

    template <typename Number, typename C>
    requires(sizeof(C) == sizeof(char) && std::is_arithmetic_v<Number>)
    Number lexicalCast(std::basic_string_view<C> str)
    {
        return strings::lexicalCast<Number>(std::string_view{reinterpret_cast<const char*>(str.data()), str.size()});
    }

    template <>
    inline bool lexicalCast<bool>(std::string_view str)
    {
        if (icaseEqual(str, "true"))
        {
            return true;
        }
        else if (icaseEqual(str, "false"))
        {
            return false;
        }

        return false;
    }

    template <typename Number>
    std::string lexicalCast(const Number number)
    requires(std::is_arithmetic_v<Number>)
    {
        return std::to_string(number);
    }

    inline std::string lexicalCast(const bool value)
    {
        return value ? "true" : "false";
    }

}  // namespace nau::strings

#define TYPED_STR(T, text) nau::str_detail::choose<T>(L##text, text)
#define TYPED_CHR(T, chr) nau::str_detail::choose<T>(L##chr, chr)
