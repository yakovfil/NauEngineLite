// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#pragma once
#include <algorithm>
#include <compare>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace nau
{
    // Each column is contiguous. Growth can invalidate every column pointer.
    // Row append/resize roll back partially constructed fields on failure.
    template <typename... Ts>
    class TupleVector
    {
        static_assert(sizeof...(Ts) > 0);
        std::tuple<std::vector<Ts>...> m_columns;
        struct Rollback
        {
            TupleVector& owner;
            size_t rows;
            bool committed = false;
            ~Rollback()
            {
                if (!committed)
                    std::apply([this](auto&... columns)
                    {
                        (([&]
                        {
                            while (columns.size() > rows)
                                columns.pop_back();
                        }()),
                         ...);
                    }, owner.m_columns);
            }
        };
        template <typename Args, size_t... I>
        void append(Args&& args, std::index_sequence<I...>)
        {
            Rollback rollback{*this, size()};
            (std::get<I>(m_columns).emplace_back(std::get<I>(std::forward<Args>(args))), ...);
            rollback.committed = true;
        }

    public:
        using size_type = size_t;
        using value_type = std::tuple<Ts...>;
        using value_tuple = value_type;
        TupleVector() = default;
        TupleVector(const TupleVector&)
        requires(std::is_copy_constructible_v<Ts> && ...)
        = default;
        TupleVector(TupleVector&& other) noexcept :
            m_columns(std::move(other.m_columns))
        {
            other.clear();
        }
        TupleVector& operator=(const TupleVector& other)
        requires(std::is_copy_constructible_v<Ts> && ...)
        {
            if (this != &other)
            {
                TupleVector copy(other);
                swap(copy);
            }
            return *this;
        }
        TupleVector& operator=(TupleVector&& other) noexcept
        {
            if (this != &other)
            {
                m_columns = std::move(other.m_columns);
                other.clear();
            }
            return *this;
        }
        size_t size() const
        {
            return std::get<0>(m_columns).size();
        }
        bool empty() const
        {
            return size() == 0;
        }
        size_t capacity() const
        {
            return std::apply([](const auto&... columns)
            {
                return std::min({columns.capacity()...});
            }, m_columns);
        }
        template <size_t I>
        auto* get()
        {
            return std::get<I>(m_columns).data();
        }
        template <size_t I>
        const auto* get() const
        {
            return std::get<I>(m_columns).data();
        }
        template <typename T>
        T* get()
        {
            return std::get<std::vector<T>>(m_columns).data();
        }
        template <typename T>
        const T* get() const
        {
            return std::get<std::vector<T>>(m_columns).data();
        }
        void reserve(size_t n)
        {
            std::apply([n](auto&... columns)
            {
                (columns.reserve(n), ...);
            }, m_columns);
        }
        void shrink_to_fit()
        {
            std::apply([](auto&... columns)
            {
                (columns.shrink_to_fit(), ...);
            }, m_columns);
        }
        void clear() noexcept
        {
            std::apply([](auto&... columns)
            {
                (columns.clear(), ...);
            }, m_columns);
        }
        void pop_back()
        {
            std::apply([](auto&... columns)
            {
                (columns.pop_back(), ...);
            }, m_columns);
        }
        void resize(size_t n)
        {
            if (n < size())
            {
                while (size() > n)
                    pop_back();
                return;
            }
            Rollback rollback{*this, size()};
            std::apply([n](auto&... columns)
            {
                (columns.resize(n), ...);
            }, m_columns);
            rollback.committed = true;
        }
        template <typename... Args>
        void emplace_back(Args&&... args)
        {
            static_assert(sizeof...(Args) == sizeof...(Ts));
            append(std::forward_as_tuple(std::forward<Args>(args)...), std::index_sequence_for<Ts...>{});
        }
        template <typename... Args>
        void push_back(Args&&... args)
        {
            emplace_back(std::forward<Args>(args)...);
        }
        auto operator[](size_t index)
        {
            return std::apply([index](auto&... columns)
            {
                return std::tie(columns[index]...);
            }, m_columns);
        }
        auto operator[](size_t index) const
        {
            return std::apply([index](const auto&... columns)
            {
                return std::tie(columns[index]...);
            }, m_columns);
        }
        void swap(TupleVector& other) noexcept
        {
            m_columns.swap(other.m_columns);
        }
        // Row positions intentionally carry an owner, rather than a column pointer.
        class iterator
        {
            friend class TupleVector;
            TupleVector* owner = nullptr;
            size_t index = 0;
            iterator(TupleVector* o, size_t i) :
                owner(o),
                index(i)
            {
            }

        public:
            using difference_type = std::ptrdiff_t;
            using value_type = std::tuple<Ts...>;
            using value_tuple = value_type;
            using iterator_category = std::random_access_iterator_tag;
            using reference = std::tuple<Ts&...>;
            using pointer = void;
            iterator() = default;
            auto operator*() const
            {
                return (*owner)[index];
            }
            iterator& operator++()
            {
                ++index;
                return *this;
            }
            iterator operator++(int)
            {
                auto old = *this;
                ++*this;
                return old;
            }
            iterator& operator--()
            {
                --index;
                return *this;
            }
            iterator operator--(int)
            {
                auto old = *this;
                --*this;
                return old;
            }
            auto operator[](difference_type n) const
            {
                return *(*this + n);
            }
            friend iterator operator+(difference_type n, iterator value)
            {
                return value += n;
            }
            friend auto operator<=>(iterator first, iterator last)
            {
                return first.index <=> last.index;
            }
            iterator& operator+=(difference_type n)
            {
                index += n;
                return *this;
            }
            iterator& operator-=(difference_type n)
            {
                index -= n;
                return *this;
            }
            friend iterator operator+(iterator i, difference_type n)
            {
                return i += n;
            }
            friend iterator operator-(iterator i, difference_type n)
            {
                return i -= n;
            }
            friend difference_type operator-(iterator a, iterator b)
            {
                return static_cast<difference_type>(a.index) - static_cast<difference_type>(b.index);
            }
            friend bool operator==(iterator a, iterator b)
            {
                return a.owner == b.owner && a.index == b.index;
            }
        };
        iterator begin()
        {
            return iterator(this, 0);
        }
        iterator end()
        {
            return iterator(this, size());
        }
        iterator erase(iterator first, iterator last)
        {
            // Rebuild retained rows: a failed field construction cannot leave
            // partially erased columns with different row counts.
            TupleVector replacement;
            replacement.reserve(size() - (last.index - first.index));
            for (size_t i = 0; i < size(); ++i)
            {
                if (i >= first.index && i < last.index)
                    continue;
                if constexpr ((std::is_copy_constructible_v<Ts> && ...))
                    std::apply([&](const auto&... fields)
                    {
                        replacement.emplace_back(fields...);
                    }, (*this)[i]);
                else
                    std::apply([&](auto&... fields)
                    {
                        replacement.emplace_back(std::move(fields)...);
                    }, (*this)[i]);
            }
            swap(replacement);
            return iterator(this, first.index);
        }
        iterator erase(iterator position)
        {
            return erase(position, position + 1);
        }
    };
}  // namespace nau
