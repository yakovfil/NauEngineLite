// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#pragma once
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <type_traits>
#include <utility>

namespace nau
{
    namespace fixed_map_detail
    {
        [[noreturn]] inline void fail(const char* message)
        {
            std::fputs(message, stderr);
            std::abort();
        }
        // No implementation node names are used. Rebinding checks the actual node
        // against this inline envelope at compile time. New STL layouts must pass
        // these checks and the allocation-count probes before being supported.
        template <typename Value, size_t Capacity, bool Overflow, typename OverflowAllocator>
        struct Resource
        {
            static constexpr size_t alignment = alignof(Value) > alignof(std::max_align_t) ? alignof(Value) : alignof(std::max_align_t);
            static constexpr size_t bytes = ((sizeof(Value) + 4 * sizeof(void*) + alignment - 1) / alignment) * alignment;
            struct alignas(alignment) Slot
            {
                std::byte data[bytes];
                bool used = false;
            };
            std::array<Slot, Capacity> nodes{};
            std::array<Slot, 2> controls{};
            OverflowAllocator overflow;
            bool initializing = true;
            explicit Resource(const OverflowAllocator& allocator) :
                overflow(allocator)
            {
            }
            template <typename T>
            T* allocate(size_t count)
            {
                static_assert(sizeof(T) <= bytes && alignof(T) <= alignment, "FixedMap node envelope must be updated for this STL layout");
                if (count != 1)
                    fail("Nau FixedMap requires single-node allocation\n");
                if (initializing)
                {
                    for (auto& slot : controls)
                        if (!slot.used)
                        {
                            slot.used = true;
                            return reinterpret_cast<T*>(slot.data);
                        }
                    fail("Nau FixedMap control storage exceeded\n");
                }
                for (auto& slot : nodes)
                    if (!slot.used)
                    {
                        slot.used = true;
                        return reinterpret_cast<T*>(slot.data);
                    }
                if constexpr (Overflow)
                {
                    using A = typename std::allocator_traits<OverflowAllocator>::template rebind_alloc<T>;
                    A allocator(overflow);
                    return std::allocator_traits<A>::allocate(allocator, count);
                }
                else
                    fail("Nau FixedMap capacity exceeded\n");
            }
            template <typename T>
            void deallocate(T* pointer, size_t count)
            {
                for (auto& slot : controls)
                    if (static_cast<void*>(pointer) == slot.data)
                    {
                        slot.used = false;
                        return;
                    }
                for (auto& slot : nodes)
                    if (static_cast<void*>(pointer) == slot.data)
                    {
                        slot.used = false;
                        return;
                    }
                if constexpr (Overflow)
                {
                    using A = typename std::allocator_traits<OverflowAllocator>::template rebind_alloc<T>;
                    A allocator(overflow);
                    std::allocator_traits<A>::deallocate(allocator, pointer, count);
                }
                else
                    fail("Nau FixedMap invalid release\n");
            }
        };
        template <typename T, typename R>
        struct Allocator
        {
            using value_type = T;
            using is_always_equal = std::false_type;
            using propagate_on_container_move_assignment = std::false_type;
            using propagate_on_container_copy_assignment = std::false_type;
            using propagate_on_container_swap = std::false_type;
            template <typename U>
            struct rebind
            {
                using other = Allocator<U, R>;
            };
            R* resource;
            explicit Allocator(R& r) :
                resource(&r)
            {
            }
            template <typename U>
            Allocator(const Allocator<U, R>& other) :
                resource(other.resource)
            {
            }
            T* allocate(size_t n)
            {
                return resource->template allocate<T>(n);
            }
            void deallocate(T* p, size_t n)
            {
                resource->deallocate(p, n);
            }
            template <typename U>
            bool operator==(const Allocator<U, R>& other) const
            {
                return resource == other.resource;
            }
        };
    }  // namespace fixed_map_detail
    // Nodes never leave their containing resource. Copies/moves rebuild nodes;
    // insertion preserves references, while assignment/swap invalidate them.
    template <typename Key, typename T, size_t Capacity, bool Overflow = true, typename Compare = std::less<Key>, typename Allocator = std::allocator<std::pair<const Key, T>>>
    class FixedMap
    {
        using R = fixed_map_detail::Resource<std::pair<const Key, T>, Capacity, Overflow, Allocator>;
        using A = fixed_map_detail::Allocator<std::pair<const Key, T>, R>;
        using Map = std::map<Key, T, Compare, A>;
        R m_resource;
        Map m_map;

    public:
        using value_type = typename Map::value_type;
        using key_type = Key;
        using mapped_type = T;
        using size_type = typename Map::size_type;
        using iterator = typename Map::iterator;
        using const_iterator = typename Map::const_iterator;
        FixedMap() :
            FixedMap(Compare{}, Allocator{})
        {
        }
        explicit FixedMap(const Compare& compare, const Allocator& allocator = {}) :
            m_resource(allocator),
            m_map(compare, A(m_resource))
        {
            m_resource.initializing = false;
        }
        template <typename Input>
        FixedMap(Input first, Input last, const Compare& compare = {}, const Allocator& allocator = {}) :
            FixedMap(compare, allocator)
        {
            insert(first, last);
        }
        FixedMap(const FixedMap& other) :
            FixedMap(other.key_comp(), std::allocator_traits<Allocator>::select_on_container_copy_construction(other.get_allocator()))
        {
            for (const auto& value : other)
                m_map.insert(value);
        }
        FixedMap(FixedMap&& other) :
            FixedMap(other.key_comp(), other.get_allocator())
        {
            for (auto& value : other)
                m_map.try_emplace(value.first, std::move(value.second));
            other.clear();
        }
        FixedMap& operator=(const FixedMap& other)
        {
            if (this != &other)
            {
                m_map = other.m_map;
            }
            return *this;
        }
        FixedMap& operator=(FixedMap&& other)
        {
            if (this != &other)
            {
                m_map = std::move(other.m_map);
                other.clear();
            }
            return *this;
        }
        auto begin()
        {
            return m_map.begin();
        }
        auto begin() const
        {
            return m_map.begin();
        }
        auto end()
        {
            return m_map.end();
        }
        auto end() const
        {
            return m_map.end();
        }
        auto cbegin() const
        {
            return m_map.cbegin();
        }
        auto cend() const
        {
            return m_map.cend();
        }
        auto size() const
        {
            return m_map.size();
        }
        bool empty() const
        {
            return m_map.empty();
        }
        void clear()
        {
            m_map.clear();
        }
        auto key_comp() const
        {
            return m_map.key_comp();
        }
        Allocator get_allocator() const
        {
            return m_resource.overflow;
        }
        template <typename K>
        auto find(const K& key)
        {
            return m_map.find(key);
        }
        template <typename K>
        auto find(const K& key) const
        {
            return m_map.find(key);
        }
        template <typename K>
        auto lower_bound(const K& key)
        {
            return m_map.lower_bound(key);
        }
        template <typename K>
        auto lower_bound(const K& key) const
        {
            return m_map.lower_bound(key);
        }
        template <typename K>
        auto upper_bound(const K& key)
        {
            return m_map.upper_bound(key);
        }
        template <typename K>
        auto upper_bound(const K& key) const
        {
            return m_map.upper_bound(key);
        }
        template <typename K>
        auto equal_range(const K& key)
        {
            return m_map.equal_range(key);
        }
        template <typename K>
        auto equal_range(const K& key) const
        {
            return m_map.equal_range(key);
        }
        template <typename K>
        auto count(const K& key) const
        {
            return m_map.count(key);
        }
        T& operator[](const Key& key)
        {
            return m_map[key];
        }
        T& operator[](Key&& key)
        {
            return m_map[std::move(key)];
        }
        template <typename... Args>
        auto try_emplace(Args&&... args)
        {
            return m_map.try_emplace(std::forward<Args>(args)...);
        }
        template <typename... Args>
        auto emplace(Args&&... args)
        {
            value_type value(std::forward<Args>(args)...);
            return m_map.try_emplace(value.first, std::move(value.second));
        }
        auto insert(const value_type& value)
        {
            return m_map.try_emplace(value.first, value.second);
        }
        auto insert(value_type&& value)
        {
            return m_map.try_emplace(value.first, std::move(value.second));
        }
        template <typename Input>
        void insert(Input first, Input last)
        {
            for (; first != last; ++first)
                emplace(first->first, first->second);
        }
        auto erase(const_iterator position)
        {
            return m_map.erase(position);
        }
        auto erase(iterator position)
        {
            return m_map.erase(position);
        }
        auto erase(const_iterator first, const_iterator last)
        {
            return m_map.erase(first, last);
        }
        size_type erase(const Key& key)
        {
            return m_map.erase(key);
        }
        void swap(FixedMap& other)
        {
            if (this == &other)
                return;
            FixedMap temporary(std::move(*this));
            *this = std::move(other);
            other = std::move(temporary);
        }
    };
}  // namespace nau
