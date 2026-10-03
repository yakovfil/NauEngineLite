// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>

namespace nau
{
    struct IntrusiveListNode
    {
        IntrusiveListNode* next = nullptr;
        IntrusiveListNode* previous = nullptr;
    };

    // Non-owning membership. A node belongs to at most one list, must be
    // unlinked before destruction, and must not move while linked.
    template <typename T>
    class IntrusiveList
    {
        template <bool Constant>
        class Iterator
        {
            friend class IntrusiveList;
            template <bool>
            friend class Iterator;
            IntrusiveListNode* m_node = nullptr;
            explicit Iterator(IntrusiveListNode* node) noexcept :
                m_node(node)
            {
            }

        public:
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::bidirectional_iterator_tag;
            using iterator_concept = std::bidirectional_iterator_tag;
            using reference = std::conditional_t<Constant, const T&, T&>;
            using pointer = std::conditional_t<Constant, const T*, T*>;
            Iterator() noexcept = default;
            template <bool Other>
            requires(Constant && !Other)
            Iterator(const Iterator<Other>& other) noexcept :
                m_node(other.m_node)
            {
            }
            reference operator*() const noexcept
            {
                return *static_cast<pointer>(m_node);
            }
            pointer operator->() const noexcept
            {
                return std::addressof(operator*());
            }
            Iterator& operator++() noexcept
            {
                m_node = m_node->next;
                return *this;
            }
            Iterator operator++(int) noexcept
            {
                auto old = *this;
                ++*this;
                return old;
            }
            Iterator& operator--() noexcept
            {
                m_node = m_node->previous;
                return *this;
            }
            Iterator operator--(int) noexcept
            {
                auto old = *this;
                --*this;
                return old;
            }
            template <bool Other>
            bool operator==(const Iterator<Other>& other) const noexcept
            {
                return m_node == other.m_node;
            }
        };

    public:
        using iterator = Iterator<false>;
        using const_iterator = Iterator<true>;
        IntrusiveList() noexcept :
            m_root{&m_root, &m_root}
        {
        }
        IntrusiveList(const IntrusiveList&) = delete;
        IntrusiveList& operator=(const IntrusiveList&) = delete;
        ~IntrusiveList()
        {
            clear();
        }
        iterator begin() noexcept
        {
            return iterator(m_root.next);
        }
        iterator end() noexcept
        {
            return iterator(&m_root);
        }
        const_iterator begin() const noexcept
        {
            return const_iterator(m_root.next);
        }
        const_iterator end() const noexcept
        {
            return const_iterator(const_cast<IntrusiveListNode*>(&m_root));
        }
        bool empty() const noexcept
        {
            return m_root.next == &m_root;
        }
        std::size_t size() const noexcept
        {
            return static_cast<std::size_t>(std::distance(begin(), end()));
        }
        T& front() noexcept
        {
            return *begin();
        }
        const T& front() const noexcept
        {
            return *begin();
        }
        T& back() noexcept
        {
            return *--end();
        }
        const T& back() const noexcept
        {
            return *--end();
        }
        iterator insert(const_iterator position, T& value) noexcept
        {
            auto* node = static_cast<IntrusiveListNode*>(&value);
            auto* after = position.m_node;
            node->next = after;
            node->previous = after->previous;
            after->previous->next = node;
            after->previous = node;
            return iterator(node);
        }
        void push_back(T& value) noexcept
        {
            insert(end(), value);
        }
        void push_front(T& value) noexcept
        {
            insert(begin(), value);
        }
        iterator erase(const_iterator position) noexcept
        {
            auto* node = position.m_node;
            auto* after = node->next;
            node->previous->next = after;
            after->previous = node->previous;
            node->next = node->previous = nullptr;
            return iterator(after);
        }
        void clear() noexcept
        {
            while (!empty())
                erase(begin());
        }
        iterator locate(T& value) noexcept
        {
            for (auto it = begin(); it != end(); ++it)
                if (std::addressof(*it) == std::addressof(value))
                    return it;
            return end();
        }
        bool contains(const T& value) const noexcept
        {
            for (const T& item : *this)
                if (std::addressof(item) == std::addressof(value))
                    return true;
            return false;
        }
        void remove(T& value) noexcept
        {
            if (auto it = locate(value); it != end())
                erase(it);
        }

    private:
        IntrusiveListNode m_root;
    };
}  // namespace nau
