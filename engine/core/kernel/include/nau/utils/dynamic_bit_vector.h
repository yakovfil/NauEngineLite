// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <algorithm>
#include <compare>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace nau
{
    using BitVectorWord = std::uintptr_t;

    // Bits are indexed least-significant-first within each word. Unused tail
    // bits are always zero. Allocators are standard typed word allocators.
    template <typename Allocator = std::allocator<BitVectorWord>>
    class DynamicBitVector
    {
        static constexpr std::size_t WordBits = std::numeric_limits<BitVectorWord>::digits;
        static constexpr auto AllBits = ~BitVectorWord{0};

    public:
        using value_type = bool;
        using size_type = std::size_t;
        using allocator_type = Allocator;
        using const_reference = bool;
        class reference
        {
        public:
            reference(BitVectorWord& word, size_type bit) :
                m_word(&word),
                m_mask(BitVectorWord{1} << bit)
            {
            }
            operator bool() const noexcept
            {
                return (*m_word & m_mask) != 0;
            }
            reference& operator=(bool value) noexcept
            {
                if (value)
                    *m_word |= m_mask;
                else
                    *m_word &= ~m_mask;
                return *this;
            }
            reference& operator=(const reference& other) noexcept
            {
                return *this = static_cast<bool>(other);
            }

        private:
            BitVectorWord* m_word;
            BitVectorWord m_mask;
        };

    private:
        template <bool Constant>
        class Iterator
        {
            friend class DynamicBitVector;
            template <bool>
            friend class Iterator;
            using Owner = std::conditional_t<Constant, const DynamicBitVector, DynamicBitVector>;
            Owner* m_owner = nullptr;
            size_type m_index = 0;
            Iterator(Owner* owner, size_type index) :
                m_owner(owner),
                m_index(index)
            {
            }

        public:
            using value_type = bool;
            using difference_type = std::ptrdiff_t;
            using reference = std::conditional_t<Constant, bool, typename DynamicBitVector::reference>;
            using pointer = void;
            using iterator_category = std::random_access_iterator_tag;
            using iterator_concept = std::random_access_iterator_tag;
            Iterator() = default;
            template <bool Other>
            requires(Constant && !Other)
            Iterator(const Iterator<Other>& other) :
                m_owner(other.m_owner),
                m_index(other.m_index)
            {
            }
            reference operator*() const
            {
                return (*m_owner)[m_index];
            }
            reference operator[](difference_type offset) const
            {
                return *(*this + offset);
            }
            Iterator& operator++()
            {
                ++m_index;
                return *this;
            }
            Iterator operator++(int)
            {
                auto previous = *this;
                ++*this;
                return previous;
            }
            Iterator& operator--()
            {
                --m_index;
                return *this;
            }
            Iterator operator--(int)
            {
                auto previous = *this;
                --*this;
                return previous;
            }
            Iterator& operator+=(difference_type offset)
            {
                m_index += offset;
                return *this;
            }
            Iterator& operator-=(difference_type offset)
            {
                m_index -= offset;
                return *this;
            }
            Iterator operator+(difference_type offset) const
            {
                auto copy = *this;
                return copy += offset;
            }
            Iterator operator-(difference_type offset) const
            {
                auto copy = *this;
                return copy -= offset;
            }
            friend Iterator operator+(difference_type offset, Iterator value)
            {
                return value += offset;
            }
            template <bool Other>
            difference_type operator-(const Iterator<Other>& other) const
            {
                return static_cast<difference_type>(m_index) - static_cast<difference_type>(other.m_index);
            }
            template <bool Other>
            bool operator==(const Iterator<Other>& other) const
            {
                return m_owner == other.m_owner && m_index == other.m_index;
            }
            template <bool Other>
            auto operator<=>(const Iterator<Other>& other) const
            {
                return m_index <=> other.m_index;
            }
        };

    public:
        using iterator = Iterator<false>;
        using const_iterator = Iterator<true>;
        DynamicBitVector() = default;
        explicit DynamicBitVector(const Allocator& allocator) :
            m_words(allocator)
        {
        }
        explicit DynamicBitVector(size_type count, const Allocator& allocator = {}) :
            m_words(allocator)
        {
            resize(count);
        }
        DynamicBitVector(size_type count, bool value, const Allocator& allocator = {}) :
            m_words(allocator)
        {
            resize(count, value);
        }
        DynamicBitVector(const DynamicBitVector&) = default;
        DynamicBitVector& operator=(const DynamicBitVector&) = default;
        DynamicBitVector(DynamicBitVector&& other) noexcept(std::is_nothrow_move_constructible_v<decltype(m_words)>) :
            m_words(std::move(other.m_words)),
            m_size(std::exchange(other.m_size, 0))
        {
        }
        DynamicBitVector& operator=(DynamicBitVector&& other)
        {
            if (this != &other)
            {
                m_words = std::move(other.m_words);
                m_size = std::exchange(other.m_size, 0);
                other.m_words.clear();
            }
            return *this;
        }
        size_type size() const noexcept
        {
            return m_size;
        }
        bool empty() const noexcept
        {
            return m_size == 0;
        }
        Allocator get_allocator() const
        {
            return m_words.get_allocator();
        }
        iterator begin()
        {
            return iterator(this, 0);
        }
        iterator end()
        {
            return iterator(this, m_size);
        }
        const_iterator begin() const
        {
            return const_iterator(this, 0);
        }
        const_iterator end() const
        {
            return const_iterator(this, m_size);
        }
        const_iterator cbegin() const
        {
            return begin();
        }
        const_iterator cend() const
        {
            return end();
        }
        auto rbegin()
        {
            return std::reverse_iterator(end());
        }
        auto rend()
        {
            return std::reverse_iterator(begin());
        }
        auto rbegin() const
        {
            return std::reverse_iterator(end());
        }
        auto rend() const
        {
            return std::reverse_iterator(begin());
        }
        reference operator[](size_type index)
        {
            return reference(m_words[index / WordBits], index % WordBits);
        }
        bool operator[](size_type index) const
        {
            return (m_words[index / WordBits] & (BitVectorWord{1} << (index % WordBits))) != 0;
        }
        bool test(size_type index, bool defaultValue = false) const
        {
            return index < m_size ? (*this)[index] : defaultValue;
        }
        reference at(size_type index)
        {
            checkIndex(index);
            return (*this)[index];
        }
        bool at(size_type index) const
        {
            checkIndex(index);
            return (*this)[index];
        }
        void set(size_type index, bool value)
        {
            if (index == std::numeric_limits<size_type>::max())
                fail();
            if (index >= m_size)
                resize(index + 1);
            (*this)[index] = value;
        }
        void resize(size_type count, bool value = false)
        {
            const auto previous = m_size;
            m_words.resize(wordCount(count), 0);
            m_size = count;
            if (count > previous && value)
                fillRange(previous, count, true);
            maskTail();
        }
        void reserve(size_type count)
        {
            m_words.reserve(wordCount(count));
        }
        void clear() noexcept
        {
            m_words.clear();
            m_size = 0;
        }
        void fill(bool value)
        {
            std::fill(m_words.begin(), m_words.end(), value ? AllBits : 0);
            maskTail();
        }
        void push_back(bool value = false)
        {
            set(m_size, value);
        }
        void pop_back()
        {
            resize(m_size - 1);
        }
        reference front()
        {
            return (*this)[0];
        }
        bool front() const
        {
            return (*this)[0];
        }
        reference back()
        {
            return (*this)[m_size - 1];
        }
        bool back() const
        {
            return (*this)[m_size - 1];
        }
        const BitVectorWord* data() const noexcept
        {
            return m_words.data();
        }
        void swap(DynamicBitVector& other)
        {
            if (get_allocator() == other.get_allocator())
                m_words.swap(other.m_words);
            else
            {
                std::vector<BitVectorWord, Allocator> left(other.m_words.begin(), other.m_words.end(), get_allocator());
                std::vector<BitVectorWord, Allocator> right(m_words.begin(), m_words.end(), other.get_allocator());
                m_words.swap(left);
                other.m_words.swap(right);
            }
            std::swap(m_size, other.m_size);
        }

    private:
        static size_type wordCount(size_type bits)
        {
            return bits / WordBits + (bits % WordBits != 0);
        }
        void maskTail()
        {
            if (m_size % WordBits)
                m_words.back() &= AllBits >> (WordBits - m_size % WordBits);
        }
        void fillRange(size_type first, size_type last, bool value)
        {
            while (first < last)
            {
                const auto start = first % WordBits;
                const auto length = std::min(last - first, WordBits - start);
                const auto mask = (AllBits >> (WordBits - length)) << start;
                auto& word = m_words[first / WordBits];
                if (value)
                    word |= mask;
                else
                    word &= ~mask;
                first += length;
            }
        }
        void checkIndex(size_type index) const
        {
            if (index < m_size)
                return;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
            throw std::out_of_range("Nau DynamicBitVector index");
#else
            fail();
#endif
        }
        [[noreturn]] static void fail()
        {
            std::fputs("Nau DynamicBitVector invalid size or index\n", stderr);
            std::abort();
        }
        std::vector<BitVectorWord, Allocator> m_words;
        size_type m_size = 0;
    };
}  // namespace nau
