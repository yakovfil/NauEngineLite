// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <cstddef>
#include <cstring>
#include <limits>
#include <memory>

namespace nau::memory_detail
{
    struct StandardAllocationHeader
    {
        void* owner;
        void* base;
        void (*release)(void*, void*) noexcept;
    };

    constexpr std::size_t standardAllocationSize(std::size_t bytes, std::size_t alignment) noexcept
    {
        // Bound raw requests to PTRDIFF_MAX as well as checking every addition.
        // This leaves room for the existing backends' small internal headers.
        constexpr auto limit = static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max());
        if (alignment == 0 || (alignment & (alignment - 1)) != 0 ||
            alignment - 1 > limit - sizeof(StandardAllocationHeader))
        {
            return 0;
        }
        const auto overhead = sizeof(StandardAllocationHeader) + alignment - 1;
        return bytes > limit - overhead ? 0 : bytes + overhead;
    }

    template <typename Backend>
    void* allocateStandardStorage(Backend& backend, std::size_t bytes, std::size_t alignment)
    {
        const auto reserved = standardAllocationSize(bytes, alignment);
        if (!reserved)
        {
            return nullptr;
        }
        void* base = backend.allocate(reserved);
        if (!base)
        {
            return nullptr;
        }
        void* result = static_cast<std::byte*>(base) + sizeof(StandardAllocationHeader);
        auto space = reserved - sizeof(StandardAllocationHeader);
        std::align(alignment, bytes, result, space);
        const StandardAllocationHeader header{
            &backend, base, [](void* owner, void* pointer) noexcept
        {
            static_cast<Backend*>(owner)->deallocate(pointer);
        }};
        std::memcpy(static_cast<std::byte*>(result) - sizeof(header), &header, sizeof(header));
        return result;
    }

    inline void releaseStandardStorage(void* pointer) noexcept
    {
        if (pointer)
        {
            StandardAllocationHeader header;
            std::memcpy(&header, static_cast<std::byte*>(pointer) - sizeof(header), sizeof(header));
            header.release(header.owner, header.base);
        }
    }
}  // namespace nau::memory_detail
