// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <cstdio>
#include <cstdlib>
#include <limits>
#include <type_traits>
#include <utility>

#include "nau/memory/frame_allocator.h"
#include "nau/memory/stack_allocator.h"
#include "nau/memory/standard_allocation.h"

namespace nau
{
    /** Captures the frame/stack backend, never the mutable current-resource slot.
     * Containers (including retained capacity) must die before reset or resource
     * destruction. Copy construction retains the source resource; assignment
     * retains the destination resource. Direct container swap requires equality.
     */
    template <typename T, typename Backend>
    class ScopedStdAllocator
    {
    public:
        using value_type = T;
        using propagate_on_container_copy_assignment = std::false_type;
        using propagate_on_container_move_assignment = std::false_type;
        using propagate_on_container_swap = std::false_type;
        using is_always_equal = std::false_type;
        template <typename U>
        struct rebind
        {
            using other = ScopedStdAllocator<U, Backend>;
        };

        ScopedStdAllocator() noexcept :
            m_backend(current())
        {
        }
        explicit ScopedStdAllocator(Backend& backend) noexcept :
            m_backend(&backend)
        {
        }
        template <typename U>
        ScopedStdAllocator(const ScopedStdAllocator<U, Backend>& other) noexcept :
            m_backend(other.resource())
        {
        }

        [[nodiscard]] T* allocate(std::size_t count)
        {
            if (!count)
                return nullptr;
            if (!m_backend || count > std::numeric_limits<std::size_t>::max() / sizeof(T))
                fail();
            void* result = memory_detail::allocateStandardStorage(*m_backend, count * sizeof(T), alignof(T));
            if (!result)
                fail();
            return static_cast<T*>(result);
        }
        void deallocate(T* pointer, std::size_t) noexcept
        {
            memory_detail::releaseStandardStorage(pointer);
        }
        Backend* resource() const noexcept
        {
            return m_backend;
        }
        template <typename U>
        bool operator==(const ScopedStdAllocator<U, Backend>& other) const noexcept
        {
            return m_backend == other.resource();
        }

    private:
        static Backend* current() noexcept
        {
            if constexpr (std::is_same_v<Backend, IFrameAllocator>)
                return IFrameAllocator::getFrameAllocator();
            else
            {
                static_assert(std::is_same_v<Backend, IStackAllocator>);
                return IStackAllocator::getStackAllocator();
            }
        }
        [[noreturn]] static void fail() noexcept
        {
            std::fputs("Nau scoped allocation failed\n", stderr);
            std::abort();
        }
        Backend* m_backend;
    };

    template <typename T>
    using FrameStdAllocator = ScopedStdAllocator<T, IFrameAllocator>;
    template <typename T>
    using StackStdAllocator = ScopedStdAllocator<T, IStackAllocator>;

    // Unequal-resource transfer rebuilds values in the destination domains.
    // Callers must not expect iterators/references to survive this operation.
    template <typename Container>
    void swapResourceContainers(Container& first, Container& second)
    {
        if (&first == &second)
            return;
        if (first.get_allocator() == second.get_allocator())
        {
            first.swap(second);
            return;
        }
        Container temporary(std::move(first), first.get_allocator());
        first = std::move(second);
        second = std::move(temporary);
    }
}  // namespace nau
