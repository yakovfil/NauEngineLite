// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>

#include "nau/utils/inline_vector.h"
#include "nau/utils/sorted_vector.h"

void check(bool value)
{
    if (!value)
        std::abort();
}
struct Order
{
    bool descending = false;
    bool operator()(int a, int b) const
    {
        return descending ? a > b : a < b;
    }
};
int main()
{
#if defined(_MSC_VER)
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    nau::SortedVectorMap<int, std::string> map{
        {{3, "c"}, {1, "a"}, {3, "ignored"}, {2, "b"}}
    };
    check(map.size() == 3 && map.begin()->first == 1 && map.find(3)->second == "c");
    check(!map.emplace(2, "ignored").second && map.find(2)->second == "b");
    check(map.emplace_hint(map.begin(), 4, "d")->first == 4);
    check(map.data() + 3 == &*(map.end() - 1));
    check(map.erase(map.begin())->first == 2);
    check(map.erase(9) == 0 && map.erase(3) == 1);
    map[7] = "g";
    check(map[7] == "g" && map.count(7) == 1);
    const auto& constant = map;
    check(constant.lower_bound(3)->first == 4 && constant.upper_bound(4)->first == 7);
    check(constant.equal_range(4).second - constant.equal_range(4).first == 1);
    check(constant.find(100) == constant.end());
    map.erase(map.begin(), map.end());
    check(map.empty());

    nau::SortedVectorMap<std::string, int, std::less<>> names;
    names.emplace("two", 2);
    names.emplace("one", 1);
    check(names.find(std::string_view("two"))->second == 2);
    check(names.find_as("one")->second == 1 && names.contains(std::string_view("one")));
    check(names.erase(std::string_view("one")) == 1);
    nau::SortedVectorMap<int, std::unique_ptr<int>> owners;
    owners.try_emplace(2, std::make_unique<int>(22));
    auto value = std::make_unique<int>(99);
    check(!owners.try_emplace(2, std::move(value)).second && value && *value == 99);
    owners.try_emplace(1, std::make_unique<int>(11));
    auto moved = std::move(owners);
    check(*moved.begin()->second == 11 && *moved.find(2)->second == 22);

    using Storage = nau::InlineVector<int, 4>;
    nau::SortedVectorSet<int, Order, std::allocator<int>, Storage> set(Order{true});
    for (int key : {2, 1, 3, 2})
        set.insert(key);
    check(set.size() == 3 && set.capacity() == 4 && *set.begin() == 3 && *set.rbegin() == 1);
    check(*set.lower_bound(2) == 2 && *set.upper_bound(2) == 1);
    auto copy = set;
    check(copy.key_comp().descending && *copy.begin() == 3);
    copy.erase(copy.begin());
    check(*copy.begin() == 2 && set.size() == 3);
    for (int key : {4, 5, 6})
        set.emplace(key);
    check(set.size() == 6 && *set.begin() == 6);

    nau::SortedVectorSet<int> actual;
    std::set<int> expected;
    unsigned random = 71;
    for (int step = 0; step < 3000; ++step)
    {
        random = random * 1664525u + 1013904223u;
        const int key = (random >> 8) % 100;
        if (random & 1)
            check(actual.insert(key).second == expected.insert(key).second);
        else
            check(actual.erase(key) == expected.erase(key));
        check(actual.size() == expected.size() && std::equal(actual.begin(), actual.end(), expected.begin()));
        check(actual.count(key) == expected.count(key));
        auto range = actual.equal_range(key);
        check(range.second - range.first == static_cast<std::ptrdiff_t>(expected.count(key)));
    }
    nau::SortedVectorMultiMap<int, int> duplicates{
        {{1, 10}, {2, 30}, {1, 20}}
    };
    auto equivalent = duplicates.equal_range(1);
    check(equivalent.second - equivalent.first == 2);
    check(equivalent.first->second == 10 && (equivalent.first + 1)->second == 20);
    duplicates.erase(equivalent.first);
    check(duplicates.find(1)->second == 20);
    duplicates.emplace_hint(duplicates.begin(), 1, 40);
    check(duplicates.find(1)->second == 20 && (duplicates.find(1) + 1)->second == 40);
    check(duplicates.erase(1) == 2 && duplicates.size() == 1);
    nau::SortedVectorMultiMap<int, int> swapped{{{5, 50}}};
    swapped.swap(duplicates);
    check(swapped.begin()->first == 2 && duplicates.begin()->first == 5);

    struct Ack
    {
        int sequence, missing, timeout;
        bool operator<(const Ack& other) const
        {
            return timeout < other.timeout;
        }
    };
    nau::SortedVectorMultiSet<Ack> acknowledgements;
    acknowledgements.insert({1, 0, 100});
    acknowledgements.insert({2, 0, 100});
    acknowledgements.insert({3, 0, 90});
    acknowledgements.insert({4, 0, 100});
    check(acknowledgements.size() == 4 && acknowledgements.front().sequence == 3);
    auto acks = acknowledgements.equal_range(Ack{0, 0, 100});
    check(acks.second - acks.first == 3 && acks.first->sequence == 1 && (acks.first + 2)->sequence == 4);
    ++(acks.first + 1)->missing;
    acknowledgements.erase(acks.first + 1);
    check(acknowledgements.count(Ack{0, 0, 100}) == 2);
    acknowledgements.erase(acknowledgements.begin(), acknowledgements.lower_bound(Ack{0, 0, 100}));
    check(acknowledgements.size() == 2 && acknowledgements.front().sequence == 1);

    // Graphics packing compares indexes through a separate height column.
    const int heights[] = {10, 5, 10, 5, 20};
    auto byHeight = [&heights](int a, int b)
    {
        return heights[a] > heights[b];
    };
    nau::SortedVectorMultiSet<int, decltype(byHeight)> roots(byHeight);
    for (int i = 0; i < 5; ++i)
        roots.emplace(i);
    check(roots.front() == 4 && roots.back() == 3);
    auto smallest = roots.equal_range(roots.back());
    check(smallest.second - smallest.first == 2 && *smallest.first == 1);
    roots.erase(smallest.first, smallest.second);
    check(roots.size() == 3 && roots[1] == 0 && roots[2] == 2);
    roots.emplace(1);
    check(roots.back() == 1);
    std::puts("sorted vectors: order, lookup, duplicates, erasure, inline storage and ownership passed");
}
