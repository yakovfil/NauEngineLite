// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <cstring>
#include <stdexcept>
#include <unordered_map>

#include "nau/memory/standard_allocator.h"
#include "nau/utils/inline_vector.h"
#if defined(_MSC_VER) && defined(_DEBUG)
    #include <crtdbg.h>
#endif

void check(bool condition)
{
    if (!condition)
        std::abort();
}
struct Resource
{
    int allocations = 0;
    std::unordered_map<void*, size_t> live;
    void* allocate(size_t bytes, size_t alignment)
    {
        ++allocations;
        void* pointer = ::operator new(bytes, std::align_val_t(alignment));
        live.emplace(pointer, alignment);
        return pointer;
    }
    void deallocate(void* pointer, size_t, size_t alignment)
    {
        check(live.at(pointer) == alignment);
        live.erase(pointer);
        ::operator delete(pointer, std::align_val_t(alignment));
    }
};
struct alignas(128) Value
{
    static inline int alive = 0;
    int number;
    explicit Value(int value = 0) :
        number(value)
    {
        ++alive;
    }
    Value(const Value& other) :
        number(other.number)
    {
        ++alive;
    }
    Value(Value&& other) noexcept :
        number(other.number)
    {
        ++alive;
    }
    Value& operator=(const Value&) = default;
    Value& operator=(Value&&) = default;
    ~Value()
    {
        --alive;
    }
};

#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
struct ThrowingCopy
{
    static inline int alive = 0;
    static inline int remaining = 0;
    ThrowingCopy()
    {
        ++alive;
    }
    ThrowingCopy(const ThrowingCopy&)
    {
        if (--remaining == 0)
            throw std::runtime_error("copy fixture");
        ++alive;
    }
    ~ThrowingCopy()
    {
        --alive;
    }
};
#endif

int main(int argc, char** argv)
{
#if defined(_MSC_VER)
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    #if defined(_DEBUG)
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    #endif
#endif
    if (argc == 2 && std::strcmp(argv[1], "--overflow-disabled") == 0)
    {
        nau::InlineVector<int, 2, false> values{1, 2};
        values.push_back(3);
        return 1;
    }
    Resource first, second;
    using Allocator = nau::ResourceAllocator<Value, Resource>;
    using Values = nau::InlineVector<Value, 2, true, Allocator>;
    {
        Values values{Allocator(first)};
        check(first.allocations == 0 && Value::alive == 0);
        values.emplace_back(10);
        check(first.allocations == 0 && Value::alive == 1);
        values.emplace_back(20);
        check(first.allocations == 0 && Value::alive == 2);
        check(reinterpret_cast<uintptr_t>(values.data()) % 128 == 0);
        values.push_back(values.front());
        check(first.allocations > 0 && values.size() == 3 && Value::alive == 3);
        check(values.back().number == 10);
        check(reinterpret_cast<uintptr_t>(values.data()) % 128 == 0);
        Values destination{Allocator(second)};
        destination = std::move(values);
        check(values.empty() && destination.size() == 3);
        check(&destination.get_allocator().resource() == &second);
        values.emplace_back(42);
        values.swap(destination);
        check(values.size() == 3 && destination.front().number == 42);
        values.erase(values.begin() + 1);
        values.emplace(values.begin() + 1, 99);
        check(values[1].number == 99 && values.size() == 3);
        auto copy = values;
        check(copy.size() == 3 && copy[1].number == 99);
        copy.assign(copy.begin(), copy.end());
        copy.erase_unsorted(copy.begin());
        check(copy.size() == 2);
    }
    check(Value::alive == 0 && first.live.empty() && second.live.empty());
    {
        nau::InlineVector<std::unique_ptr<int>, 2> values;
        values.emplace_back(std::make_unique<int>(7));
        values.emplace(values.begin(), std::make_unique<int>(8));
        values.emplace_back(std::make_unique<int>(9));
        auto moved = std::move(values);
        check(values.empty() && *moved[0] == 8 && *moved[2] == 9);
    }
    {
        nau::InlineVector<int, 3> range{1, 4};
        int middle[] = {2, 3};
        auto inserted = range.insert(range.begin() + 1, std::begin(middle), std::end(middle));
        check(inserted == range.begin() + 1 && range.size() == 4);
        for (int i = 0; i < 4; ++i)
            check(range[i] == i + 1);
        nau::InlineVector<int, 3>::reverse_iterator reverse = range.rbegin();
        check(*reverse == 4);
        check(range.insert(range.end(), middle, middle) == range.end());
    }
    nau::InlineVector<int, 0> zero;
    zero.push_back(7);
    check(zero.front() == 7);
    nau::InlineVector<int, 3, false> bounded{1, 2};
    bounded.insert(bounded.begin(), 0);
    check(bounded.size() == 3 && bounded[2] == 2);
    bounded.erase(bounded.begin(), bounded.end());
    check(bounded.empty());
    std::vector<nau::InlineVector<std::unique_ptr<int>, 1>> nested;
    for (int i = 0; i < 20; ++i)
        nested.emplace_back().emplace_back(std::make_unique<int>(i));
    for (int i = 0; i < 20; ++i)
        check(*nested[i][0] == i);
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    {
        nau::InlineVector<ThrowingCopy, 3> original(2);
        ThrowingCopy::remaining = 2;
        bool caught = false;
        try
        {
            auto copy = original;
        }
        catch (const std::runtime_error&)
        {
            caught = true;
        }
        check(caught && ThrowingCopy::alive == 2 && original.size() == 2);
    }
    check(ThrowingCopy::alive == 0);
#endif
    std::puts("inline vector: no inline allocations, aligned growth, lifetimes and domains passed");
}
