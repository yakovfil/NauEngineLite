// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <cstdio>
#include <cstdlib>
#include <limits>
#include <type_traits>

#include "nau/memory/array_allocator.h"
#include "nau/memory/fixed_blocks.h"
#include "nau/memory/standard_allocation.h"

namespace nau
{
    /** Stateless domain adapter. Each allocation records the originating backend
     * and release operation, including when allocated in a different module.
     * Modules providing backends must remain loaded until their storage is freed.
     */
    template <typename T, bool UsePool>
    class DomainAllocator
    {
    public:
        using value_type = T;
        using propagate_on_container_copy_assignment = std::false_type;
        using propagate_on_container_move_assignment = std::false_type;
        using propagate_on_container_swap = std::false_type;
        // Any instance can release storage through its recorded origin.
        using is_always_equal = std::true_type;
        template <typename U>
        struct rebind
        {
            using other = DomainAllocator<U, UsePool>;
        };

        DomainAllocator() noexcept = default;
        template <typename U>
        DomainAllocator(const DomainAllocator<U, UsePool>&) noexcept
        {
        }

        [[nodiscard]] T* allocate(std::size_t count)
        {
            if (count == 0)
            {
                return nullptr;
            }
            if (count > std::numeric_limits<std::size_t>::max() / sizeof(T))
            {
                fail();
            }
            void* result = nullptr;
            if constexpr (UsePool)
            {
                // Standard node containers rebind to their actual node type.
                // Pool blocks include alignment slack and ownership metadata.
                constexpr auto rawSize = memory_detail::standardAllocationSize(sizeof(T), alignof(T));
                constexpr auto blockSize = (rawSize + alignof(std::max_align_t) - 1) & ~(alignof(std::max_align_t) - 1);
                static_assert(blockSize <= static_cast<std::size_t>(std::numeric_limits<int>::max()));
                if (count != 1)
                {
                    fail();  // This domain is for node containers, not arrays.
                }
                result = memory_detail::allocateStandardStorage(FixedBlocksAllocator<static_cast<int>(blockSize)>::instance(), sizeof(T), alignof(T));
            }
            else
            {
                result = memory_detail::allocateStandardStorage(ArrayAllocator<1024 * 1024>::instance(), count * sizeof(T), alignof(T));
            }
            if (!result)
            {
                fail();
            }
            return static_cast<T*>(result);
        }

        void deallocate(T* pointer, std::size_t) noexcept
        {
            memory_detail::releaseStandardStorage(pointer);
        }

        template <typename U>
        bool operator==(const DomainAllocator<U, UsePool>&) const noexcept
        {
            return true;
        }

    private:
        [[noreturn]] static void fail() noexcept
        {
            std::fputs("Nau container allocation failed\n", stderr);
            std::abort();
        }
    };

    template <typename T>
    using ArrayStdAllocator = DomainAllocator<T, false>;
    template <typename T>
    using PoolStdAllocator = DomainAllocator<T, true>;
}  // namespace nau
