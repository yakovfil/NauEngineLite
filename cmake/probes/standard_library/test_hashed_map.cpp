// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include <cstdint>
#include <cstdlib>
#include <string>
#include <unordered_map>
#define NAU_ASSERT(x, ...) \
    do                     \
    {                      \
        if (!(x))          \
            std::abort();  \
    } while (false)
#define NAU_FAST_ASSERT(x, ...) NAU_ASSERT(x)
#include "nau/utils/dag_hashedKeyMap.h"
#if NAU_HASH_REJECT == 1
dag::HashedKeyMap<unsigned, std::string> rejected;
#elif NAU_HASH_REJECT == 2
dag::FixedCapacityHashedKeyMap<unsigned, std::string> rejected;
#endif
struct alignas(128) Value
{
    unsigned number;
};
struct Resource
{
    std::unordered_map<void*, size_t> live;
    int remaining = -1;
};
Resource defaultResource;
template <class T>
struct Allocator
{
    using value_type = T;
    Resource* owner = &defaultResource;
    Allocator() = default;
    explicit Allocator(Resource& resource) :
        owner(&resource)
    {
    }
    template <class U>
    Allocator(const Allocator<U>& other) :
        owner(other.owner)
    {
    }
    T* allocate(size_t n)
    {
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
        if (owner->remaining == 0)
            throw std::bad_alloc();
        if (owner->remaining > 0)
            --owner->remaining;
#endif
        auto* p = std::allocator<T>{}.allocate(n);
        NAU_ASSERT(owner->live.emplace(p, n * sizeof(T)).second);
        return p;
    }
    void deallocate(T* p, size_t n)
    {
        NAU_ASSERT(owner->live.at(p) == n * sizeof(T));
        owner->live.erase(p);
        std::allocator<T>{}.deallocate(p, n);
    }
    bool operator==(const Allocator& other) const
    {
        return owner == other.owner;
    }
};
int main()
{
    for (int run = 0; run < 100; ++run)
    {
        dag::HashedKeyMap<unsigned, Value, 99999> values;
        for (unsigned i = 0; i < 150; ++i)
            values.emplace(i, Value{i});
        auto copy = values;
        auto moved = std::move(values);
        for (unsigned i = 0; i < 150; ++i)
        {
            NAU_ASSERT(moved.findVal(i)->number == i && copy.findVal(i)->number == i);
            NAU_ASSERT(reinterpret_cast<uintptr_t>(moved.findVal(i)) % 128 == 0);
        }
        for (unsigned i = 0; i < 75; ++i)
            moved.erase(i);
        moved.swap(copy);
        NAU_ASSERT(moved.size() == 150 && copy.size() == 75);
        values = copy;
        NAU_ASSERT(values.size() == 75);
        values.clear();
        NAU_ASSERT(values.empty() && !values.has_key(1));
        dag::HashedKeySet<unsigned, 99999> set(0);
        NAU_ASSERT(set.insert(0) && !set.insert(0));
        for (unsigned i = 1; i < 80; ++i)
            set.insert(i);
        auto other = std::move(set);
        NAU_ASSERT(other.has_key(0) && !set.has_key(0));
        other.erase(0);
        NAU_ASSERT(!other.has_key(0));
    }
    Resource first, second;
    {
        using Map = dag::HashedKeyMap<unsigned, Value, 0, oa_hashmap_util::NoHash<unsigned>, Allocator<std::byte>>;
        Map destination{Allocator<std::byte>(second)};
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
        {
            Map failure{Allocator<std::byte>(first)};
            first.remaining = 1;
            bool caught = false;
            try
            {
                failure.emplace(1, Value{1});
            }
            catch (const std::bad_alloc&)
            {
                caught = true;
            }
            NAU_ASSERT(caught && failure.empty() && first.live.empty());
            first.remaining = -1;
        }
#endif
        {
            Map source{Allocator<std::byte>(first)};
            for (unsigned i = 1; i < 90; ++i)
                source.emplace(i, Value{i});
            destination = source;
            destination.swap(source);
            destination = std::move(source);
        }
        NAU_ASSERT(destination.findVal(89)->number == 89);
    }
    NAU_ASSERT(first.live.empty() && second.live.empty() && defaultResource.live.empty());
    unsigned keys[4] = {}, vals[4] = {};
    dag::FixedCapacityHashedKeyMap<unsigned, unsigned> fixed{3, 0, keys, vals};
    NAU_ASSERT(fixed.emplace_if_missing(1, 10));
    NAU_ASSERT(!fixed.emplace_if_missing(1, 20));
    NAU_ASSERT(fixed.findOr(1, 0) == 20 && fixed.size() == 1);
}
