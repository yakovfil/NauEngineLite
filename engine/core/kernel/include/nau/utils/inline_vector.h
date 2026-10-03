// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace nau
{
    // Contiguous inline objects with an explicit typed overflow allocator.
    // Growth invalidates pointers; copies and unequal-resource moves rebuild
    // objects in destination storage. Assignment retains the destination domain.
    template <typename T, std::size_t Capacity, bool Overflow = true, typename Allocator = std::allocator<T>>
    class InlineVector
    {
    public:
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = T&;
        using const_reference = const T&;
        using iterator = T*;
        using const_iterator = const T*;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using allocator_type = Allocator;

        InlineVector() = default;
        explicit InlineVector(const Allocator& allocator) :
            m_allocator(allocator)
        {
        }
        explicit InlineVector(size_type count, const Allocator& allocator = {}) :
            InlineVector(allocator)
        {
            resize(count);
        }
        InlineVector(size_type count, const T& value, const Allocator& allocator = {}) :
            InlineVector(allocator)
        {
            resize(count, value);
        }
        template <typename Input>
        requires(!std::is_integral_v<Input>)
        InlineVector(Input first, Input last, const Allocator& allocator = {}) :
            InlineVector(allocator)
        {
            for (; first != last; ++first)
                emplace_back(*first);
        }
        InlineVector(std::initializer_list<T> values, const Allocator& allocator = {}) :
            InlineVector(values.begin(), values.end(), allocator)
        {
        }
        InlineVector(const InlineVector& other)
        requires std::is_copy_constructible_v<T>
            :
            InlineVector(other.begin(), other.end(), std::allocator_traits<Allocator>::select_on_container_copy_construction(other.get_allocator()))
        {
        }
        InlineVector(InlineVector&& other) noexcept(std::is_nothrow_copy_constructible_v<Allocator> && std::is_nothrow_move_constructible_v<Allocator> && std::is_nothrow_move_constructible_v<T> && noexcept(std::declval<const Allocator&>() == std::declval<const Allocator&>())) :
            InlineVector(other.get_allocator())
        {
            *this = std::move(other);
        }
        InlineVector(InlineVector&& other, const Allocator& allocator) :
            InlineVector(allocator)
        {
            *this = std::move(other);
        }
        ~InlineVector()
        {
            clearInline();
        }
        InlineVector& operator=(const InlineVector& other)
        requires std::is_copy_constructible_v<T>
        {
            if (this != &other)
                assign(other.begin(), other.end());
            return *this;
        }
        InlineVector& operator=(InlineVector&& other)
        {
            if (this == &other)
                return *this;
            if (other.m_spilled && get_allocator() == other.get_allocator())
            {
                clearInline();
                m_spilled = false;
                m_overflow.reset();
                m_overflow.emplace(std::move(*other.m_overflow));
                m_spilled = true;
                other.m_overflow.reset();
                other.m_spilled = false;
            }
            else
            {
                clear();
                reserve(other.size());
                for (auto& value : other)
                    emplace_back(std::move(value));
                other.clear();
            }
            return *this;
        }
        InlineVector& operator=(std::initializer_list<T> values)
        {
            assign(values.begin(), values.end());
            return *this;
        }
        Allocator get_allocator() const
        {
            return m_allocator;
        }
        size_type size() const noexcept
        {
            return m_spilled ? m_overflow->size() : m_inlineSize;
        }
        size_type capacity() const noexcept
        {
            return m_spilled ? m_overflow->capacity() : Capacity;
        }
        bool empty() const noexcept
        {
            return size() == 0;
        }
        T* data() noexcept
        {
            return m_spilled ? m_overflow->data() : inlineData();
        }
        const T* data() const noexcept
        {
            return m_spilled ? m_overflow->data() : inlineData();
        }
        iterator begin() noexcept
        {
            return data();
        }
        const_iterator begin() const noexcept
        {
            return data();
        }
        const_iterator cbegin() const noexcept
        {
            return begin();
        }
        iterator end() noexcept
        {
            return data() + size();
        }
        const_iterator end() const noexcept
        {
            return data() + size();
        }
        const_iterator cend() const noexcept
        {
            return end();
        }
        auto rbegin() noexcept
        {
            return std::reverse_iterator(end());
        }
        auto rend() noexcept
        {
            return std::reverse_iterator(begin());
        }
        auto rbegin() const noexcept
        {
            return std::reverse_iterator(end());
        }
        auto rend() const noexcept
        {
            return std::reverse_iterator(begin());
        }
        T& operator[](size_type index)
        {
            return data()[index];
        }
        const T& operator[](size_type index) const
        {
            return data()[index];
        }
        T& front()
        {
            return *begin();
        }
        const T& front() const
        {
            return *begin();
        }
        T& back()
        {
            return *(end() - 1);
        }
        const T& back() const
        {
            return *(end() - 1);
        }
        void reserve(size_type count)
        {
            if (count <= capacity())
                return;
            if constexpr (!Overflow)
                fail();
            else if (m_spilled)
                m_overflow->reserve(count);
            else
            {
                std::vector<T, Allocator> replacement(get_allocator());
                replacement.reserve(count);
                for (size_type i = 0; i < m_inlineSize; ++i)
                    replacement.emplace_back(std::move_if_noexcept(inlineData()[i]));
                m_overflow.emplace(std::move(replacement));
                clearInline();
                m_spilled = true;
            }
        }
        template <typename... Args>
        T& emplace_back(Args&&... args)
        {
            if (m_spilled)
                return m_overflow->emplace_back(std::forward<Args>(args)...);
            if (m_inlineSize < Capacity)
            {
                T* result = std::construct_at(inlineData() + m_inlineSize, std::forward<Args>(args)...);
                ++m_inlineSize;
                return *result;
            }
            if constexpr (!Overflow)
                fail();
            else
            {
                // Materialize aliases into inline storage before relocation.
                T value(std::forward<Args>(args)...);
                reserve(Capacity < 2 ? 2 : Capacity * 2);
                return m_overflow->emplace_back(std::move_if_noexcept(value));
            }
        }
        T& push_back()
        {
            return emplace_back();
        }
        void push_back(const T& value)
        {
            emplace_back(value);
        }
        void push_back(T&& value)
        {
            emplace_back(std::move(value));
        }
        void pop_back()
        {
            if (m_spilled)
                m_overflow->pop_back();
            else
                std::destroy_at(inlineData() + --m_inlineSize);
        }
        void clear() noexcept
        {
            if (m_spilled)
                m_overflow->clear();
            else
                clearInline();
        }
        void resize(size_type count)
        {
            while (size() > count)
                pop_back();
            reserve(count);
            while (size() < count)
                emplace_back();
        }
        void resize(size_type count, const T& value)
        {
            if (count <= size())
            {
                while (size() > count)
                    pop_back();
                return;
            }
            T copy(value);
            reserve(count);
            while (size() < count)
                emplace_back(copy);
        }
        template <typename Input>
        requires(!std::is_integral_v<Input>)
        void assign(Input first, Input last)
        {
            InlineVector replacement(first, last, get_allocator());
            *this = std::move(replacement);
        }
        template <typename... Args>
        iterator emplace(const_iterator position, Args&&... args)
        {
            const auto offset = position - begin();
            T value(std::forward<Args>(args)...);
            if (position == end())
            {
                emplace_back(std::move(value));
                return begin() + offset;
            }
            if (size() == capacity())
                reserve(size() + 1);
            if (m_spilled)
                m_overflow->emplace(m_overflow->begin() + offset, std::move(value));
            else
            {
                const auto previous = size();
                emplace_back(std::move(back()));
                std::move_backward(begin() + offset, begin() + previous - 1, begin() + previous);
                (*this)[offset] = std::move(value);
            }
            return begin() + offset;
        }
        iterator insert(const_iterator position, const T& value)
        {
            return emplace(position, value);
        }
        iterator insert(const_iterator position, T&& value)
        {
            return emplace(position, std::move(value));
        }
        template <std::input_iterator Input>
        iterator insert(const_iterator position, Input first, Input last)
        {
            const auto offset = position - begin();
            auto next = offset;
            for (; first != last; ++first, ++next)
                emplace(begin() + next, *first);
            return begin() + offset;
        }
        iterator erase(const_iterator first, const_iterator last)
        {
            const auto offset = first - begin();
            const auto count = last - first;
            if (count)
            {
                std::move(begin() + offset + count, end(), begin() + offset);
                for (difference_type i = 0; i < count; ++i)
                    pop_back();
            }
            return begin() + offset;
        }
        iterator erase(const_iterator position)
        {
            return erase(position, position + 1);
        }
        iterator erase_unsorted(const_iterator position)
        {
            const auto offset = position - begin();
            if (position != end() - 1)
                (*this)[offset] = std::move(back());
            pop_back();
            return begin() + offset;
        }
        void swap(InlineVector& other)
        {
            if (this == &other)
                return;
            InlineVector temporary(std::move(*this));
            *this = std::move(other);
            other = std::move(temporary);
        }

    private:
        T* inlineData() noexcept
        {
            return reinterpret_cast<T*>(m_storage);
        }
        const T* inlineData() const noexcept
        {
            return reinterpret_cast<const T*>(m_storage);
        }
        void clearInline() noexcept
        {
            while (m_inlineSize)
                std::destroy_at(inlineData() + --m_inlineSize);
        }
        [[noreturn]] static void fail()
        {
            std::fputs("Nau InlineVector capacity exceeded\n", stderr);
            std::abort();
        }
        Allocator m_allocator;
        std::optional<std::vector<T, Allocator>> m_overflow;
        size_type m_inlineSize = 0;
        bool m_spilled = false;
        alignas(T) std::byte m_storage[sizeof(T) * (Capacity ? Capacity : 1)];
    };
}  // namespace nau
