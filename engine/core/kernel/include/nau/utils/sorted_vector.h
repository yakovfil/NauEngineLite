// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace nau
{
    namespace sorted_detail
    {
        struct Identity
        {
            template <typename T>
            const T& operator()(const T& value) const
            {
                return value;
            }
        };
        struct First
        {
            template <typename T>
            const auto& operator()(const T& value) const
            {
                return value.first;
            }
        };

        // Iterators have the underlying contiguous storage's invalidation rules.
        // Mutable access is retained for non-key fields; callers must not alter keys.
        // Storage assignment retains its allocation domain. Comparators must impose
        // the same strict weak ordering for keys and heterogeneous lookup arguments.
        template <typename Key, typename Value, typename Compare, typename Storage, typename KeyOf, bool Multi = false>
        class SortedVector
        {
        public:
            using key_type = Key;
            using value_type = Value;
            using base_type = Storage;
            using size_type = typename Storage::size_type;
            using iterator = typename Storage::iterator;
            using const_iterator = typename Storage::const_iterator;
            using allocator_type = typename Storage::allocator_type;
            using insert_result = std::conditional_t<Multi, iterator, std::pair<iterator, bool>>;

            SortedVector() = default;
            explicit SortedVector(const Compare& compare) :
                m_compare(compare)
            {
            }
            SortedVector(const Compare& compare, const allocator_type& allocator) :
                m_values(allocator),
                m_compare(compare)
            {
            }
            explicit SortedVector(const allocator_type& allocator) :
                m_values(allocator)
            {
            }
            SortedVector(std::initializer_list<Value> values, const Compare& compare = {}, const allocator_type& allocator = {}) :
                SortedVector(compare, allocator)
            {
                insert(values.begin(), values.end());
            }
            template <typename Input>
            SortedVector(Input first, Input last, const Compare& compare = {}, const allocator_type& allocator = {}) :
                SortedVector(compare, allocator)
            {
                insert(first, last);
            }
            iterator begin()
            {
                return m_values.begin();
            }
            const_iterator begin() const
            {
                return m_values.begin();
            }
            const_iterator cbegin() const
            {
                return m_values.cbegin();
            }
            iterator end()
            {
                return m_values.end();
            }
            const_iterator end() const
            {
                return m_values.end();
            }
            const_iterator cend() const
            {
                return m_values.cend();
            }
            auto rbegin()
            {
                return m_values.rbegin();
            }
            auto rbegin() const
            {
                return m_values.rbegin();
            }
            auto rend()
            {
                return m_values.rend();
            }
            auto rend() const
            {
                return m_values.rend();
            }
            Value* data()
            {
                return m_values.data();
            }
            const Value* data() const
            {
                return m_values.data();
            }
            size_type size() const
            {
                return m_values.size();
            }
            bool empty() const
            {
                return m_values.empty();
            }
            size_type capacity() const
            {
                return m_values.capacity();
            }
            void reserve(size_type count)
            {
                m_values.reserve(count);
            }
            void shrink_to_fit() requires requires(Storage& values) { values.shrink_to_fit(); }
            {
                m_values.shrink_to_fit();
            }
            friend bool operator==(const SortedVector& left, const SortedVector& right)
            {
                return left.m_values == right.m_values;
            }
            void clear()
            {
                m_values.clear();
            }
            Value& front()
            {
                return m_values.front();
            }
            const Value& front() const
            {
                return m_values.front();
            }
            Value& back()
            {
                return m_values.back();
            }
            const Value& back() const
            {
                return m_values.back();
            }
            Value& operator[](size_type index)
            {
                return m_values[index];
            }
            const Value& operator[](size_type index) const
            {
                return m_values[index];
            }
            void swap(SortedVector& other)
            {
                if (this == &other)
                    return;
                if (get_allocator() == other.get_allocator())
                    m_values.swap(other.m_values);
                else
                {
                    Storage left(get_allocator()), right(other.get_allocator());
                    left.reserve(other.size());
                    right.reserve(size());
                    for (auto& value : other.m_values)
                        left.emplace_back(std::move(value));
                    for (auto& value : m_values)
                        right.emplace_back(std::move(value));
                    m_values = std::move(left);
                    other.m_values = std::move(right);
                }
                using std::swap;
                swap(m_compare, other.m_compare);
            }
            allocator_type get_allocator() const
            {
                return m_values.get_allocator();
            }
            Compare key_comp() const
            {
                return m_compare;
            }

            template <typename K>
            iterator lower_bound(const K& key)
            {
                return std::lower_bound(begin(), end(), key, [this](const Value& value, const K& candidate)
                {
                    return m_compare(KeyOf{}(value), candidate);
                });
            }
            template <typename K>
            const_iterator lower_bound(const K& key) const
            {
                return std::lower_bound(begin(), end(), key, [this](const Value& value, const K& candidate)
                {
                    return m_compare(KeyOf{}(value), candidate);
                });
            }
            template <typename K>
            iterator upper_bound(const K& key)
            {
                return std::upper_bound(begin(), end(), key, [this](const K& candidate, const Value& value)
                {
                    return m_compare(candidate, KeyOf{}(value));
                });
            }
            template <typename K>
            const_iterator upper_bound(const K& key) const
            {
                return std::upper_bound(begin(), end(), key, [this](const K& candidate, const Value& value)
                {
                    return m_compare(candidate, KeyOf{}(value));
                });
            }
            template <typename K>
            auto equal_range(const K& key)
            {
                return std::pair(lower_bound(key), upper_bound(key));
            }
            template <typename K>
            auto equal_range(const K& key) const
            {
                return std::pair(lower_bound(key), upper_bound(key));
            }
            template <typename K>
            iterator find(const K& key)
            {
                auto found = lower_bound(key);
                return found != end() && !m_compare(key, KeyOf{}(*found)) ? found : end();
            }
            template <typename K>
            const_iterator find(const K& key) const
            {
                auto found = lower_bound(key);
                return found != end() && !m_compare(key, KeyOf{}(*found)) ? found : end();
            }
            iterator find(const Key& key)
            {
                return find<Key>(key);
            }
            const_iterator find(const Key& key) const
            {
                return find<Key>(key);
            }
            template <typename K>
            auto find_as(const K& key)
            {
                return find(key);
            }
            template <typename K>
            auto find_as(const K& key) const
            {
                return find(key);
            }
            template <typename K>
            bool contains(const K& key) const
            {
                return find(key) != end();
            }
            template <typename K>
            size_type count(const K& key) const
            {
                auto range = equal_range(key);
                return static_cast<size_type>(range.second - range.first);
            }

            template <typename... Args>
            insert_result emplace(Args&&... args)
            {
                Value value(std::forward<Args>(args)...);
                if constexpr (Multi)
                {
                    // Insertion at upper_bound preserves equivalent-key arrival order.
                    auto position = upper_bound(KeyOf{}(value));
                    return m_values.insert(position, std::move(value));
                }
                else
                {
                    auto position = lower_bound(KeyOf{}(value));
                    if (position != end() && !m_compare(KeyOf{}(value), KeyOf{}(*position)))
                        return {position, false};
                    return {m_values.insert(position, std::move(value)), true};
                }
            }
            template <typename... Args>
            iterator emplace_hint(const_iterator, Args&&... args)
            {
                if constexpr (Multi)
                    return emplace(std::forward<Args>(args)...);
                else
                    return emplace(std::forward<Args>(args)...).first;
            }
            insert_result insert(const Value& value)
            {
                return emplace(value);
            }
            insert_result insert(Value&& value)
            {
                return emplace(std::move(value));
            }
            iterator insert(const_iterator hint, const Value& value) { return emplace_hint(hint, value); }
            iterator insert(const_iterator hint, Value&& value) { return emplace_hint(hint, std::move(value)); }
            template <typename Input>
            void insert(Input first, Input last)
            {
                for (; first != last; ++first)
                    insert(*first);
            }
            iterator erase(const_iterator position)
            {
                return m_values.erase(position);
            }
            iterator erase(const_iterator first, const_iterator last)
            {
                return m_values.erase(first, last);
            }
            // Do not let the heterogeneous overload capture a mutable iterator.
            template <typename K>
            requires(!std::is_convertible_v<K, const_iterator>)
            size_type erase(const K& key)
            {
                auto range = equal_range(key);
                const auto removed = static_cast<size_type>(range.second - range.first);
                m_values.erase(range.first, range.second);
                return removed;
            }

        protected:
            Storage m_values;
            Compare m_compare{};
        };
    }  // namespace sorted_detail

    template <typename Key, typename Compare = std::less<>, typename Allocator = std::allocator<Key>, typename Storage = std::vector<Key, Allocator>>
    using SortedVectorSet = sorted_detail::SortedVector<Key, Key, Compare, Storage, sorted_detail::Identity>;

    template <typename Key, typename T, typename Compare = std::less<>, typename Allocator = std::allocator<std::pair<Key, T>>, typename Storage = std::vector<std::pair<Key, T>, Allocator>>
    class SortedVectorMap : public sorted_detail::SortedVector<Key, std::pair<Key, T>, Compare, Storage, sorted_detail::First>
    {
        using Base = sorted_detail::SortedVector<Key, std::pair<Key, T>, Compare, Storage, sorted_detail::First>;

    public:
        using Base::Base;
        using mapped_type = T;
        template <typename K, typename... Args>
        auto try_emplace(K&& key, Args&&... args)
        {
            auto position = this->lower_bound(key);
            if (position != this->end() && !this->m_compare(key, position->first))
                return std::pair(position, false);
            auto inserted = this->m_values.emplace(position, std::piecewise_construct,
                                                   std::forward_as_tuple(std::forward<K>(key)), std::forward_as_tuple(std::forward<Args>(args)...));
            return std::pair(inserted, true);
        }
        T& operator[](const Key& key)
        {
            return try_emplace(key).first->second;
        }
        T& operator[](Key&& key)
        {
            return try_emplace(std::move(key)).first->second;
        }
    };
    template <typename Key, typename Compare = std::less<>, typename Allocator = std::allocator<Key>, typename Storage = std::vector<Key, Allocator>>
    using SortedVectorMultiSet = sorted_detail::SortedVector<Key, Key, Compare, Storage, sorted_detail::Identity, true>;

    template <typename Key, typename T, typename Compare = std::less<>, typename Allocator = std::allocator<std::pair<Key, T>>, typename Storage = std::vector<std::pair<Key, T>, Allocator>>
    using SortedVectorMultiMap = sorted_detail::SortedVector<Key, std::pair<Key, T>, Compare, Storage, sorted_detail::First, true>;
}  // namespace nau
