// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <algorithm>
#include <cstdlib>

#include "nau/memory/standard_allocation.h"

namespace nau
{
    // Byte storage for images and retained relocatable arrays. Allocating modules
    // remain loaded until their recorded release functions have been called.
    class HeapByteAllocator
    {
        struct Backend
        {
            void* allocate(std::size_t bytes)
            {
                return std::malloc(bytes);
            }
            void deallocate(void* pointer) noexcept
            {
                std::free(pointer);
            }
        };
        struct Header
        {
            void* storage;
            std::size_t bytes;
            std::size_t alignment;
        };
        static Header header(void* pointer) noexcept
        {
            Header result;
            std::memcpy(&result, static_cast<std::byte*>(pointer) - sizeof(Header), sizeof(Header));
            return result;
        }

    public:
        void* allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t))
        {
            constexpr auto limit = static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max());
            if (!alignment || (alignment & (alignment - 1)) || alignment - 1 > limit - sizeof(Header))
                return nullptr;
            const auto overhead = sizeof(Header) + alignment - 1;
            if (bytes > limit - overhead)
                return nullptr;
            static Backend backend;
            void* storage = memory_detail::allocateStandardStorage(backend, bytes + overhead, alignof(std::max_align_t));
            if (!storage)
                return nullptr;
            void* result = static_cast<std::byte*>(storage) + sizeof(Header);
            auto space = bytes + alignment - 1;
            std::align(alignment, bytes, result, space);
            const Header metadata{storage, bytes, alignment};
            std::memcpy(static_cast<std::byte*>(result) - sizeof(Header), &metadata, sizeof(Header));
            return result;
        }
        void* allocate(std::size_t bytes, std::size_t alignment, std::size_t offset, int)
        {
            return offset == 0 ? allocate(bytes, alignment) : nullptr;
        }
        void deallocate(void* pointer, std::size_t = 0) noexcept
        {
            if (pointer)
                memory_detail::releaseStandardStorage(header(pointer).storage);
        }
        void* realloc(void* pointer, std::size_t bytes)
        {
            if (!pointer)
                return allocate(bytes);
            if (!bytes)
            {
                deallocate(pointer);
                return nullptr;
            }
            const auto previous = header(pointer);
            void* replacement = allocate(bytes, previous.alignment);
            if (replacement)
            {
                std::memcpy(replacement, pointer, std::min(bytes, previous.bytes));
                deallocate(pointer);
            }
            return replacement;
        }
        bool operator==(const HeapByteAllocator&) const noexcept
        {
            return true;
        }
    };
}  // namespace nau
