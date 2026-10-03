// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <new>
#include <unordered_map>
#include <vector>
#if defined(_MSC_VER) && defined(_DEBUG)
    #include <crtdbg.h>
#endif

#include "nau/memory/standard_allocation.h"
#include "nau/memory/standard_allocator.h"

void check(bool value)
{
    if (!value)
    {
        std::abort();
    }
}

struct Resource
{
    struct Allocation
    {
        size_t bytes;
        size_t alignment;
    };
    std::unordered_map<void*, Allocation> live;
    int allocations = 0;
    int releases = 0;

    void* allocate(size_t bytes, size_t alignment)
    {
        void* pointer = ::operator new(bytes, std::align_val_t(alignment));
        live.emplace(pointer, Allocation{bytes, alignment});
        ++allocations;
        return pointer;
    }

    void deallocate(void* pointer, size_t bytes, size_t alignment) noexcept
    {
        const auto it = live.find(pointer);
        check(it != live.end());
        check(it->second.bytes == bytes && it->second.alignment == alignment);
        live.erase(it);
        ++releases;
        ::operator delete(pointer, std::align_val_t(alignment));
    }
};

struct alignas(128) Value
{
    std::unique_ptr<int> value;
    explicit Value(int number) :
        value(std::make_unique<int>(number))
    {
    }
    Value(Value&&) = default;
    Value& operator=(Value&&) = default;
};

struct RawResource
{
    std::unordered_map<void*, size_t> live;
    int releases = 0;
    void* allocate(size_t bytes)
    {
        void* pointer = std::malloc(bytes);
        if (pointer)
            live.emplace(pointer, bytes);
        return pointer;
    }
    void deallocate(void* pointer)
    {
        check(live.erase(pointer) == 1);
        ++releases;
        std::free(pointer);
    }
};

int main(int argc, char** argv)
{
#if defined(_MSC_VER)
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    #if defined(_DEBUG)
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    #endif
#endif
    Resource first, second;
    using Allocator = nau::ResourceAllocator<Value, Resource>;
    Allocator allocator(first);
    if (argc == 2 && std::strcmp(argv[1], "--overflow") == 0)
    {
        (void)allocator.allocate(Allocator::max_size() + 1);
        return 1;
    }
    static_assert(!std::allocator_traits<Allocator>::is_always_equal::value);
    static_assert(!std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value);
    {
        std::vector<Value, Allocator> values(allocator);
        for (int i = 0; i < 100; ++i)
        {
            values.emplace_back(i);
        }
        for (int i = 0; i < 100; ++i)
        {
            check(*values[i].value == i);
            check(reinterpret_cast<uintptr_t>(&values[i]) % alignof(Value) == 0);
        }
        std::vector<Value, Allocator> destination{Allocator(second)};
        destination = std::move(values);
        check(&destination.get_allocator().resource() == &second);
        check(*destination[99].value == 99);
        destination.erase(destination.begin() + 17, destination.end());
        destination.shrink_to_fit();
        for (int i = 0; i < 17; ++i)
        {
            check(*destination[i].value == i);
            check(reinterpret_cast<uintptr_t>(&destination[i]) % alignof(Value) == 0);
        }

        using Pair = std::pair<const int, Value>;
        using NodeAllocator = nau::ResourceAllocator<Pair, Resource>;
        std::map<int, Value, std::less<int>, NodeAllocator> nodes{NodeAllocator(first)};
        nodes.try_emplace(7, 42);
        check(*nodes.at(7).value == 42);
        check(reinterpret_cast<uintptr_t>(&nodes.at(7)) % alignof(Value) == 0);
        auto node = nodes.extract(7);
        auto inserted = nodes.insert(std::move(node));
        check(inserted.inserted);
    }
    check(first.live.empty() && second.live.empty());
    check(first.allocations == first.releases && second.allocations == second.releases);
    check(allocator.allocate(0) == nullptr);
    RawResource rawFirst, rawSecond;
    for (size_t alignment : {size_t(1), size_t(8), size_t(16), size_t(32), size_t(64), size_t(128), size_t(256), size_t(4096)})
    {
        void* a = nau::memory_detail::allocateStandardStorage(rawFirst, 37, alignment);
        void* b = nau::memory_detail::allocateStandardStorage(rawSecond, 71, alignment);
        check(a && b);
        check(reinterpret_cast<uintptr_t>(a) % alignment == 0);
        check(reinterpret_cast<uintptr_t>(b) % alignment == 0);
        std::memset(a, 0xAB, 37);
        std::memset(b, 0xCD, 71);
        // Release takes no current-resource argument: the stored origin owns it.
        nau::memory_detail::releaseStandardStorage(b);
        nau::memory_detail::releaseStandardStorage(a);
    }
    check(rawFirst.live.empty() && rawSecond.live.empty());
    check(rawFirst.releases == 8 && rawSecond.releases == 8);
    check(nau::memory_detail::allocateStandardStorage(rawFirst, SIZE_MAX, 128) == nullptr);
    check(nau::memory_detail::allocateStandardStorage(rawFirst, 8, 3) == nullptr);
    check(nau::memory_detail::allocateStandardStorage(rawFirst, 8, 0) == nullptr);
    nau::memory_detail::releaseStandardStorage(nullptr);
    std::puts("resource allocator: alignment, rebinding, ownership and move checks passed");
}
