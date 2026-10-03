// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include <array>
#include <cstdlib>
#include <string>
#define NAU_ASSERT(x, ...) \
    do                     \
    {                      \
        if (!(x))          \
            std::abort();  \
    } while (false)
#define NAU_FAST_ASSERT(x, ...) NAU_ASSERT(x)
#include "nau/utils/dag_oaHashNameMap.h"
static_assert(std::is_nothrow_move_constructible_v<dag::StringTableAllocator::StringPage>);
static_assert(alignof(dag::StringTableAllocator) >= alignof(std::vector<int>));
struct CollisionHasher
{
    static uint32_t hash_data(const char*, size_t)
    {
        return 7;
    }
};
int main()
{
    dag::OAHashNameMap<false, CollisionHasher> names;
    const char exact[] = {'a', 'b', 'c'};
    NAU_ASSERT(names.addNameId(exact, sizeof(exact)) == 0);
    const char* first = names.getName(0);
    NAU_ASSERT(std::string(first) == "abc");
    for (int i = 1; i < 400; ++i)
    {
        const auto name = "item-" + std::to_string(i) + std::string(30, 'x');
        NAU_ASSERT(names.addNameId(name.c_str()) == i);
    }
    NAU_ASSERT(names.getName(0) == first);
    NAU_ASSERT(names.getNameId("abc") == 0 && names.addNameId("abc") == 0);
    auto copy = names;
    NAU_ASSERT(copy.getName(0) != first && std::string(copy.getName(0)) == "abc");
    auto moved = std::move(names);
    NAU_ASSERT(moved.getName(0) == first);
    moved.erase(1);
    NAU_ASSERT(moved.nameCount() == 399 && moved.getNameId(("item-2" + std::string(30, 'x')).c_str()) == 1);
    copy.clear();
    NAU_ASSERT(copy.nameCount() == 0);
    dag::StringTableAllocator pages;
    auto emptyCopy = pages;
    const char payload[] = {'a', 0, 'b'};
    const auto offset = pages.addDataRaw(payload, sizeof(payload));
    auto pageCopy = pages;
    NAU_ASSERT(std::memcmp(pageCopy.getDataRawUnsafe(offset), payload, sizeof(payload)) == 0);
    dag::NameMapCI folded;
    NAU_ASSERT(folded.addNameId("Alpha") == 0);
    NAU_ASSERT(folded.addNameId("ALPHA") == 0 && folded.getNameId("alpha") == 0);
    dag::NameMap ordered;
    ordered.addNameId("zeta");
    ordered.addNameId("alpha");
    ordered.addNameId("beta");
    std::vector<uint32_t> ids;
    dag::gather_ids_in_lexical_order(ids, ordered);
    NAU_ASSERT((ids == std::vector<uint32_t>{1, 2, 0}));
}
