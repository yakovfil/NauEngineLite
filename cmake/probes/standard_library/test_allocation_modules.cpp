// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#define NOMINMAX
#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <thread>
#include <vector>

#include "nau/memory/standard_allocation.h"
#include "nau/memory/standard_allocator.h"

void check(bool condition)
{
    if (!condition)
        std::abort();
}
struct Module
{
    HMODULE handle;
    using Allocate = void* (*)(size_t, size_t);
    using Count = unsigned (*)();
    Allocate allocateRaw;
    Count allocations;
    Count releases;
    explicit Module(const char* path) :
        handle(LoadLibraryA(path))
    {
        check(handle != nullptr);
        allocateRaw = reinterpret_cast<Allocate>(GetProcAddress(handle, "probe_allocate"));
        allocations = reinterpret_cast<Count>(GetProcAddress(handle, "probe_allocations"));
        releases = reinterpret_cast<Count>(GetProcAddress(handle, "probe_releases"));
        check(allocateRaw && allocations && releases);
    }
    ~Module()
    {
        check(FreeLibrary(handle));
    }
    void* allocate(size_t bytes, size_t alignment)
    {
        return allocateRaw(bytes, alignment);
    }
    void deallocate(void* pointer, size_t, size_t)
    {
        // The EXE invokes the release function recorded inside the owning DLL.
        nau::memory_detail::releaseStandardStorage(pointer);
    }
};
struct alignas(256) Value
{
    std::unique_ptr<int> number;
    explicit Value(int value) :
        number(std::make_unique<int>(value))
    {
    }
    Value(Value&&) = default;
    Value& operator=(Value&&) = default;
};

int main(int argc, char** argv)
{
    check(argc == 3);
    Module first(argv[1]), second(argv[2]);
    using Allocator = nau::ResourceAllocator<Value, Module>;
    using Values = std::vector<Value, Allocator>;
    {
        Values values{Allocator(first)};
        for (int i = 0; i < 65; ++i)
            values.emplace_back(i);
        for (int i = 0; i < 65; ++i)
        {
            check(*values[i].number == i);
            check(reinterpret_cast<uintptr_t>(&values[i]) % alignof(Value) == 0);
        }
        Values moved{Allocator(second)};
        moved = std::move(values);
        check(&moved.get_allocator().resource() == &second);
        check(*moved.back().number == 64);
        std::thread destroy([owned = std::move(moved)]() mutable
        {
            owned.clear();
            owned.shrink_to_fit();
        });
        destroy.join();
        using Pair = std::pair<const int, Value>;
        using NodeAllocator = nau::ResourceAllocator<Pair, Module>;
        std::map<int, Value, std::less<int>, NodeAllocator> nodes{NodeAllocator(first)};
        nodes.try_emplace(7, 42);
        check(*nodes.at(7).number == 42);
        check(reinterpret_cast<uintptr_t>(&nodes.at(7)) % alignof(Value) == 0);
    }
    check(first.allocations() > 0 && second.allocations() > 0);
    check(first.allocations() == first.releases());
    check(second.allocations() == second.releases());
    check(first.allocateRaw(SIZE_MAX, 128) == nullptr);
    check(second.allocateRaw(8, 3) == nullptr);
    std::puts("DLL allocation: two origins, alignment, rebind, unequal-resource move and cross-thread release passed");
}
