// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once
#include <bitset>
#include <cstddef>
#include <iterator>

// Fixed-size standard bit storage with iteration over set indices. No caller
// depends on a library-specific word layout; serialized masks use explicit integers.
template <std::size_t N>
class Bitset : public std::bitset<N>
{
public:
    using base_type = std::bitset<N>;
    using size_type = std::size_t;
    using base_type::base_type;
    Bitset() = default;
    Bitset(const base_type& value) :
        base_type(value)
    {
    }
    size_type find_first() const
    {
        return find_next_from(0);
    }
    size_type find_last() const
    {
        for (size_type i = N; i > 0; --i)
            if (this->test(i - 1))
                return i - 1;
        return N;
    }
    size_type find_first_and_reset()
    {
        const auto index = find_first();
        if (index != N)
            this->reset(index);
        return index;
    }
    class Iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = size_type;
        using difference_type = std::ptrdiff_t;
        using reference = size_type;
        using pointer = void;
        Iterator() = default;
        Iterator(const Bitset* owner, size_type index) :
            m_owner(owner),
            m_index(index)
        {
        }
        value_type operator*() const
        {
            return m_index;
        }
        Iterator& operator++()
        {
            m_index = m_owner->find_next_from(m_index + 1);
            return *this;
        }
        Iterator operator++(int)
        {
            auto previous = *this;
            ++*this;
            return previous;
        }
        bool operator==(const Iterator&) const = default;

    private:
        const Bitset* m_owner = nullptr;
        size_type m_index = N;
    };
    Iterator begin() const
    {
        return {this, find_first()};
    }
    Iterator end() const
    {
        return {this, N};
    }

private:
    size_type find_next_from(size_type start) const
    {
        for (size_type i = start; i < N; ++i)
            if (this->test(i))
                return i;
        return N;
    }
};
