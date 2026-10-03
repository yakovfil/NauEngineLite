// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include <cstring>
#include <unordered_map>

#include "nau/memory/standard_allocator.h"
#include "nau/utils/fixed_map.h"
void check(bool value)
{
    if (!value)
        std::abort();
}
struct Resource
{
    int allocations = 0;
    std::unordered_map<void*, size_t> live;
    void* allocate(size_t bytes, size_t alignment)
    {
        auto* p = ::operator new(bytes, std::align_val_t(alignment));
        live.emplace(p, alignment);
        ++allocations;
        return p;
    }
    void deallocate(void* p, size_t, size_t alignment)
    {
        check(live.at(p) == alignment);
        live.erase(p);
        ::operator delete(p, std::align_val_t(alignment));
    }
};
struct alignas(128) Value
{
    static inline int alive = 0;
    int number;
    Value(int n = 0) :
        number(n)
    {
        ++alive;
    }
    Value(const Value& other) :
        Value(other.number)
    {
    }
    Value(Value&& other) noexcept :
        Value(other.number)
    {
    }
    Value& operator=(const Value&) = default;
    ~Value()
    {
        --alive;
    }
};
int main(int argc, char** argv)
{
#if defined(_MSC_VER)
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    if (argc > 1 && std::strcmp(argv[1], "--overflow-disabled") == 0)
    {
        nau::FixedMap<int, int, 1, false> values;
        values.emplace(1, 1);
        values.emplace(2, 2);
        return 1;
    }
    Resource first, second;
    using Pair = std::pair<const int, Value>;
    using A = nau::ResourceAllocator<Pair, Resource>;
    using Map = nau::FixedMap<int, Value, 3, true, std::less<int>, A>;
    {
        Map values(std::less<int>{}, A(first));
        check(first.allocations == 0);
        for (int i = 0; i < 3; ++i)
            values.try_emplace(i, i);
        check(first.allocations == 0 && Value::alive == 3);
        auto* stable = &values.find(0)->second;
        check(reinterpret_cast<uintptr_t>(stable) % 128 == 0);
        check(!values.try_emplace(0, 99).second && first.allocations == 0);
        values.erase(1);
        values.try_emplace(5, 5);
        check(first.allocations == 0 && &values.find(0)->second == stable);
        values.try_emplace(9, 9);
        check(first.allocations > 0 && &values.find(0)->second == stable);
        Map copied(values);
        check(copied.size() == 4 && copied.find(5)->second.number == 5);
        Map destination(std::less<int>{}, A(second));
        destination = std::move(copied);
        check(copied.empty() && &destination.get_allocator().resource() == &second);
        values.swap(destination);
        check(&values.get_allocator().resource() == &first && &destination.get_allocator().resource() == &second);
        destination = values;
        check(destination.size() == 4);
    }
    check(Value::alive == 0 && first.live.empty() && second.live.empty());
    nau::FixedMap<int, std::unique_ptr<int>, 2> destination;
    {
        nau::FixedMap<int, std::unique_ptr<int>, 2> source;
        source.try_emplace(1, std::make_unique<int>(42));
        destination = std::move(source);
    }
    check(*destination.find(1)->second == 42);
    nau::FixedMap<int, int, 2, false> bounded;
    bounded.emplace(2, 20);
    bounded.emplace(1, 10);
    check(!bounded.emplace(1, 30).second && bounded.begin()->second == 10);
    auto copy = bounded;
    auto moved = std::move(copy);
    check(copy.empty() && moved.size() == 2);
    std::puts("fixed map: inline nodes, alignment, reuse, overflow, domains and source destruction passed");
}
