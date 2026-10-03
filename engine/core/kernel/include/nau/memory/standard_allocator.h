// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <type_traits>

namespace nau
{
    /**
     * Standard typed allocator bound to a non-owning resource. The resource
     * provides allocate(bytes, alignment) and deallocate(pointer, bytes,
     * alignment), and must outlive every container using it. Resource equality
     * means identity; allocation never silently changes domains.
     */
    template <typename T, typename Resource>
    class ResourceAllocator
    {
    public:
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using propagate_on_container_copy_assignment = std::false_type;
        using propagate_on_container_move_assignment = std::false_type;
        using propagate_on_container_swap = std::false_type;
        using is_always_equal = std::false_type;

        template <typename U>
        struct rebind
        {
            using other = ResourceAllocator<U, Resource>;
        };

        explicit ResourceAllocator(Resource& resource) noexcept :
            m_resource(&resource)
        {
        }

        template <typename U>
        ResourceAllocator(const ResourceAllocator<U, Resource>& other) noexcept :
            m_resource(&other.resource())
        {
        }

        [[nodiscard]] T* allocate(size_type count)
        {
            if (count > max_size())
            {
                allocationFailure();
            }
            if (count == 0)
            {
                return nullptr;
            }
            auto* pointer = static_cast<T*>(m_resource->allocate(count * sizeof(T), alignof(T)));
            if (!pointer)
            {
                allocationFailure();
            }
            return pointer;
        }

        void deallocate(T* pointer, size_type count) noexcept
        {
            if (pointer)
            {
                m_resource->deallocate(pointer, count * sizeof(T), alignof(T));
            }
        }

        [[nodiscard]] static constexpr size_type max_size() noexcept
        {
            return std::numeric_limits<size_type>::max() / sizeof(T);
        }

        [[nodiscard]] Resource& resource() const noexcept
        {
            return *m_resource;
        }

        template <typename U>
        bool operator==(const ResourceAllocator<U, Resource>& other) const noexcept
        {
            return m_resource == &other.resource();
        }

    private:
        [[noreturn]] static void allocationFailure() noexcept
        {
            // This low-level header must work before engine diagnostics exist,
            // including exception-free Web builds. Never return null as a
            // successful nonzero allocation to a standard container.
            std::fputs("Nau resource allocation failed\n", stderr);
            std::abort();
        }

        Resource* m_resource;
    };
}  // namespace nau
