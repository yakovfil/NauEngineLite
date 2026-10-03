// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <list>
#include <optional>
#include <unordered_map>
#include <utility>

namespace nau
{
    /** Externally synchronized cache. References survive promotion and unrelated
     * insertion, but not replacement, eviction, erase, clear or destruction.
     * Callbacks must not reenter this cache; release callbacks must not throw.
     */
    template <typename Key, typename Value, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>>
    class LruCache
    {
        using Entries = std::list<std::pair<Key, Value>>;
        using Iterator = typename Entries::iterator;

    public:
        using Create = std::function<Value(const Key&)>;
        using Release = std::function<void(const Value&)>;

        explicit LruCache(std::size_t capacity, Create create = {}, Release release = {}) :
            m_capacity(capacity),
            m_create(std::move(create)),
            m_release(std::move(release))
        {
        }

        LruCache(const LruCache&) = delete;
        LruCache& operator=(const LruCache&) = delete;
        LruCache(LruCache&&) = delete;
        LruCache& operator=(LruCache&&) = delete;

        ~LruCache()
        {
            clear();
        }

        bool empty() const noexcept
        {
            return m_entries.empty();
        }
        std::size_t size() const noexcept
        {
            return m_entries.size();
        }
        std::size_t capacity() const noexcept
        {
            return m_capacity;
        }
        bool contains(const Key& key) const
        {
            return m_index.find(key) != m_index.end();
        }

        std::optional<Value> at(const Key& key) const
        {
            const auto found = m_index.find(key);
            return found == m_index.end() ? std::nullopt : std::optional<Value>(found->second->second);
        }

        Value& get(const Key& key)
        {
            requireCapacity();
            const auto found = m_index.find(key);
            if (found != m_index.end())
            {
                promote(found->second);
                return found->second->second;
            }
            Value value = m_create ? m_create(key) : Value{};
            insert(key, std::move(value));
            return m_entries.front().second;
        }

        Value& operator[](const Key& key)
        {
            return get(key);
        }

        template <typename V>
        bool insert(const Key& key, V&& value)
        {
            requireCapacity();
            if (contains(key))
            {
                return false;  // Duplicate insertion does not promote an entry.
            }
            // Materialize first: value may refer to the entry about to be evicted.
            Entries pending;
            pending.emplace_front(key, std::forward<V>(value));
            if (size() == m_capacity)
            {
                erase_oldest();
            }
            // On an index insertion exception pending destroys its value and
            // the existing list remains consistent. Splice cannot throw.
            m_index.emplace(pending.front().first, pending.begin());
            m_entries.splice(m_entries.begin(), pending, pending.begin());
            return true;
        }

        template <typename V>
        bool assign(const Key& key, V&& value)
        {
            requireCapacity();
            const auto found = m_index.find(key);
            if (found == m_index.end())
            {
                return false;
            }
            Entries replacement;
            replacement.emplace_front(key, std::forward<V>(value));
            const auto old = found->second;
            found->second = replacement.begin();
            m_entries.splice(m_entries.begin(), replacement, replacement.begin());
            if (m_release)
            {
                m_release(old->second);
            }
            m_entries.erase(old);
            return true;
        }

        template <typename V>
        void insert_or_assign(const Key& key, V&& value)
        {
            if (!assign(key, std::forward<V>(value)))
            {
                insert(key, std::forward<V>(value));
            }
        }

        bool touch(const Key& key)
        {
            const auto found = m_index.find(key);
            if (found == m_index.end())
            {
                return false;
            }
            promote(found->second);
            return true;
        }

        bool erase(const Key& key)
        {
            const auto found = m_index.find(key);
            if (found == m_index.end())
            {
                return false;
            }
            const auto entry = found->second;
            m_index.erase(found);
            if (m_release)
            {
                m_release(entry->second);
            }
            m_entries.erase(entry);
            return true;
        }

        void erase_oldest()
        {
            if (empty())
            {
                fail("Nau LRU cache cannot evict an empty cache");
            }
            erase(m_entries.back().first);
        }

        void clear() noexcept
        {
            while (!empty())
            {
                erase_oldest();
            }
        }

        void resize(std::size_t capacity)
        {
            m_capacity = capacity;
            while (size() > capacity)
            {
                erase_oldest();
            }
        }

    private:
        void promote(Iterator entry) noexcept
        {
            m_entries.splice(m_entries.begin(), m_entries, entry);
        }

        [[noreturn]] static void fail(const char* message) noexcept
        {
            std::fputs(message, stderr);
            std::fputc('\n', stderr);
            std::abort();
        }

        void requireCapacity() const
        {
            if (m_capacity == 0)
            {
                fail("Nau LRU cache access requires nonzero capacity");
            }
        }

        std::size_t m_capacity;
        Entries m_entries;
        std::unordered_map<Key, Iterator, Hash, Equal> m_index;
        Create m_create;
        Release m_release;
    };
}  // namespace nau
