// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <limits>
#include <string_view>

#include "nau/utils/inline_vector.h"

namespace nau
{
    // Capacity includes the terminator, matching the migrated fixed-string
    // call sites. A moved-from string remains empty and null-terminated.
    template <typename Char, std::size_t Capacity, bool Overflow = true, typename Allocator = std::allocator<Char>>
    class InlineString
    {
        static_assert(Capacity > 0);
        static_assert(std::is_trivial_v<Char>);
        using Characters = InlineVector<Char, Capacity, Overflow, Allocator>;

    public:
        using value_type = Char;
        using size_type = std::size_t;
        using iterator = Char*;
        using const_iterator = const Char*;
        using View = std::basic_string_view<Char>;
        InlineString()
        {
            m_characters.emplace_back(Char{});
        }
        explicit InlineString(const Allocator& allocator) :
            m_characters(allocator)
        {
            m_characters.emplace_back(Char{});
        }
        InlineString(const Char* value, const Allocator& allocator = {}) :
            InlineString(View(value), allocator)
        {
        }
        InlineString(const Char* value, size_type length, const Allocator& allocator = {}) :
            InlineString(View(value, length), allocator)
        {
        }
        InlineString(const Char* first, const Char* last, const Allocator& allocator = {}) :
            InlineString(first, static_cast<size_type>(last - first), allocator)
        {
        }
        InlineString(View value, const Allocator& allocator = {}) :
            InlineString(allocator)
        {
            append(value);
        }
        InlineString(const InlineString&) = default;
        InlineString& operator=(const InlineString&) = default;
        InlineString(InlineString&& other) noexcept(std::is_nothrow_move_constructible_v<Characters>) :
            m_characters(std::move(other.m_characters))
        {
            other.m_characters.emplace_back(Char{});
        }
        InlineString& operator=(InlineString&& other)
        {
            if (this != &other)
            {
                if (get_allocator() == other.get_allocator())
                {
                    m_characters = std::move(other.m_characters);
                    other.m_characters.emplace_back(Char{});
                }
                else
                {
                    InlineString replacement(View(other), get_allocator());
                    m_characters = std::move(replacement.m_characters);
                    other.clear();
                }
            }
            return *this;
        }
        InlineString& operator=(View value)
        {
            InlineString replacement(value, get_allocator());
            *this = std::move(replacement);
            return *this;
        }
        InlineString& operator=(const Char* value)
        {
            return *this = View(value);
        }
        Allocator get_allocator() const
        {
            return m_characters.get_allocator();
        }
        size_type size() const noexcept
        {
            return m_characters.size() - 1;
        }
        size_type length() const noexcept
        {
            return size();
        }
        bool empty() const noexcept
        {
            return size() == 0;
        }
        Char* data() noexcept
        {
            return m_characters.data();
        }
        const Char* data() const noexcept
        {
            return m_characters.data();
        }
        const Char* c_str() const noexcept
        {
            return data();
        }
        iterator begin() noexcept
        {
            return data();
        }
        const_iterator begin() const noexcept
        {
            return data();
        }
        iterator end() noexcept
        {
            return data() + size();
        }
        const_iterator end() const noexcept
        {
            return data() + size();
        }
        Char& operator[](size_type index)
        {
            return data()[index];
        }
        const Char& operator[](size_type index) const
        {
            return data()[index];
        }
        operator View() const noexcept
        {
            return View(data(), size());
        }
        void reserve(size_type count)
        {
            checkLength(count);
            m_characters.reserve(count + 1);
        }
        void clear()
        {
            m_characters.clear();
            m_characters.emplace_back(Char{});
        }
        void resize(size_type count, Char fill = Char{})
        {
            checkLength(count);
            const auto previous = size();
            m_characters.resize(count + 1);
            if (count > previous)
                std::fill(data() + previous, data() + count, fill);
            data()[count] = Char{};
        }
        InlineString& append(View value)
        {
            if (value.empty())
                return *this;
            if (value.size() > std::numeric_limits<size_type>::max() - size() - 1)
                fail();
            // Snapshot before growth: value may be a substring of this string,
            // including its terminator. Inline input stays inline here too.
            Characters source(value.begin(), value.end(), get_allocator());
            reserve(size() + source.size());
            m_characters.pop_back();
            for (Char item : source)
                m_characters.emplace_back(item);
            m_characters.emplace_back(Char{});
            return *this;
        }
        InlineString& append(const Char* value)
        {
            return append(View(value));
        }
        InlineString& append(const Char* value, size_type length)
        {
            return append(View(value, length));
        }
        InlineString& operator+=(View value)
        {
            return append(value);
        }
        InlineString& operator+=(const Char* value)
        {
            return append(value);
        }
        InlineString& operator+=(Char value)
        {
            return append(View(&value, 1));
        }
        void push_back(Char value)
        {
            *this += value;
        }
        InlineString& replace(const_iterator first, const_iterator last, const Char* replacement, size_type length)
        {
            const auto prefix = static_cast<size_type>(first - begin());
            const auto suffix = static_cast<size_type>(last - begin());
            InlineString result(get_allocator());
            result.append(data(), prefix);
            result.append(replacement, length);
            result.append(data() + suffix, size() - suffix);
            *this = std::move(result);
            return *this;
        }
        friend InlineString operator+(const InlineString& first, View second)
        {
            InlineString result(first);
            result += second;
            return result;
        }
        friend bool operator==(const InlineString& first, View second)
        {
            return View(first) == second;
        }
        friend bool operator==(const InlineString& first, const InlineString& second)
        {
            return View(first) == View(second);
        }

    private:
        static void checkLength(size_type length)
        {
            if (length == std::numeric_limits<size_type>::max())
                fail();
        }
        [[noreturn]] static void fail()
        {
            std::fputs("Nau InlineString size overflow\n", stderr);
            std::abort();
        }
        Characters m_characters;
    };
}  // namespace nau
