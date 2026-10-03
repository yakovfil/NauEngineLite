// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include <cstdlib>
#include <string>
#include <unordered_map>
#include "nau/memory/standard_allocator.h"
#include "nau/string/string_hash.h"
#include "ska_hash_map/flat_hash_map2.hpp"
void check(bool condition) { if (!condition) std::abort(); }
struct Resource
{
    std::unordered_map<void*, std::pair<size_t, size_t>> live;
    void* allocate(size_t bytes, size_t alignment)
    {
        void* p = ::operator new(bytes, std::align_val_t(alignment));
        live.emplace(p, std::pair(bytes, alignment)); return p;
    }
    void deallocate(void* p, size_t bytes, size_t alignment) noexcept
    {
        auto it = live.find(p); check(it != live.end());
        check(it->second == std::pair(bytes, alignment)); live.erase(it);
        ::operator delete(p, std::align_val_t(alignment));
    }
};
struct alignas(128) Owner
{
    static inline int alive = 0;
    std::string text;
    explicit Owner(std::string value = {}) : text(std::move(value)) { ++alive; }
    Owner(const Owner& v) : text(v.text) { ++alive; }
    Owner(Owner&& v) noexcept : text(std::move(v.text)) { ++alive; }
    Owner& operator=(const Owner&) = default;
    Owner& operator=(Owner&&) = default;
    ~Owner() { --alive; }
};
int main()
{
    Resource first, second;
    using Alloc = nau::ResourceAllocator<std::pair<std::string, Owner>, Resource>;
    using Map = ska::flat_hash_map<std::string, Owner, nau::StringHash, std::equal_to<>, Alloc>;
    {
        Map left{Alloc(first)}, right{Alloc(second)};
        for (int i = 0; i < 300; ++i) left.emplace(std::to_string(i), Owner(std::string(120, 'a') + std::to_string(i)));
        const char key[] = {'1', '2', 'x'};
        auto found = left.find_as(std::string_view(key, 2));
        check(found != left.end()); check(found->second.text.ends_with("12"));
        check(left.find_as("12") == found);
        ska::flat_hash_map<std::string, int> ordinary;
        ordinary.emplace("key", 42);
        check(ordinary.find_as("key")->second == 42);
        check(ordinary.find_as(std::string_view("key"))->second == 42);
        for (auto& [name, value] : left) check(reinterpret_cast<uintptr_t>(&value) % alignof(Owner) == 0);
        right.emplace("other", Owner("short"));
        left.swap(right);
        check(left.size() == 1 && right.size() == 300);
        check(&left.get_allocator().resource() == &first);
        check(&right.get_allocator().resource() == &second);
        Map transferred(std::move(right), Alloc(first));
        check(transferred.size() == 300 && right.empty());
        right = std::move(transferred);
        check(right.size() == 300);
        left = right;
        check(left.size() == 300);
        for (int i = 0; i < 300; i += 2) right.erase(std::to_string(i));
        check(right.size() == 150);
        left.clear(); right.clear();
    }
    check(Owner::alive == 0 && first.live.empty() && second.live.empty());
}
