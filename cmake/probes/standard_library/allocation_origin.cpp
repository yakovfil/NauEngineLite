// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <cstdlib>
#include <mutex>
#include <unordered_set>

#include "nau/memory/standard_allocation.h"

namespace
{
    struct Backend
    {
        std::mutex mutex;
        std::unordered_set<void*> live;
        unsigned allocations = 0;
        unsigned releases = 0;
        void* allocate(size_t bytes)
        {
            void* pointer = std::malloc(bytes);
            if (pointer)
            {
                std::lock_guard lock(mutex);
                live.insert(pointer);
                ++allocations;
            }
            return pointer;
        }
        void deallocate(void* pointer)
        {
            std::lock_guard lock(mutex);
            if (live.erase(pointer) != 1)
                std::abort();
            ++releases;
            std::free(pointer);
        }
    } backend;
}  // namespace

extern "C" __declspec(dllexport) void* probe_allocate(size_t bytes, size_t alignment)
{
    return nau::memory_detail::allocateStandardStorage(backend, bytes, alignment);
}
extern "C" __declspec(dllexport) unsigned probe_allocations()
{
    std::lock_guard lock(backend.mutex);
    return backend.allocations;
}
extern "C" __declspec(dllexport) unsigned probe_releases()
{
    std::lock_guard lock(backend.mutex);
    return backend.releases;
}
