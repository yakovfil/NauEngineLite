// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <vector>

#include "nau/generic/dag_bitset.h"
#include "nau/generic/dag_objectPool.h"
#include "nau/memory/heap_byte_allocator.h"

TEST(RendererContainers, AlignedRawGrowthRetainsBytesAndFailureRetainsOwnership)
{
    nau::HeapByteAllocator allocator;
    for (const size_t alignment : {size_t{1}, size_t{16}, size_t{64}, size_t{256}})
    {
        auto* data = static_cast<unsigned char*>(allocator.allocate(37, alignment));
        ASSERT_NE(data, nullptr);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(data) % alignment, 0u);
        for (size_t i = 0; i < 37; ++i)
            data[i] = static_cast<unsigned char>(i);
        data = static_cast<unsigned char*>(allocator.realloc(data, 1024));
        ASSERT_NE(data, nullptr);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(data) % alignment, 0u);
        for (size_t i = 0; i < 37; ++i)
            EXPECT_EQ(data[i], i);
        EXPECT_EQ(allocator.realloc(data, std::numeric_limits<size_t>::max()), nullptr);
        EXPECT_EQ(data[36], 36);
        data = static_cast<unsigned char*>(allocator.realloc(data, 8));
        ASSERT_NE(data, nullptr);
        for (size_t i = 0; i < 8; ++i)
            EXPECT_EQ(data[i], i);
        EXPECT_EQ(allocator.realloc(data, 0), nullptr);
    }
    EXPECT_EQ(allocator.allocate(12, 3), nullptr);
    EXPECT_EQ(allocator.allocate(12, 16, 1, 0), nullptr);
    allocator.deallocate(nullptr);
}

TEST(RendererContainers, FixedMaskScansEmptyWordBoundariesAndTail)
{
    Bitset<130> bits;
    EXPECT_EQ(bits.find_first(), 130u);
    EXPECT_EQ(bits.find_last(), 130u);
    EXPECT_EQ(bits.find_first_and_reset(), 130u);
    EXPECT_EQ(bits.begin(), bits.end());
    for (auto i : {0, 31, 32, 63, 64, 129})
        bits.set(i);
    EXPECT_EQ((std::vector<size_t>(bits.begin(), bits.end())), (std::vector<size_t>{0, 31, 32, 63, 64, 129}));
    EXPECT_EQ(bits.find_last(), 129u);
    for (auto i : {0, 31, 32, 63, 64, 129})
        EXPECT_EQ(bits.find_first_and_reset(), i);
    EXPECT_TRUE(bits.none());
    bits.set();
    EXPECT_EQ(bits.count(), 130u);
    EXPECT_EQ((~bits).count(), 0u);
    EXPECT_EQ(Bitset<0>{}.find_first_and_reset(), 0u);
}

namespace
{
    struct alignas(128) PoolOwner
    {
        static inline int alive = 0;
        int value;
        explicit PoolOwner(int number) :
            value(number)
        {
            ++alive;
        }
        ~PoolOwner()
        {
            --alive;
        }
    };
}  // namespace
TEST(RendererContainers, PoolGrowthRetainsAddressesAndReusesReleasedSlots)
{
    ASSERT_EQ(PoolOwner::alive, 0);
    {
        ObjectPool<PoolOwner, 65> pool;
        std::vector<PoolOwner*> owners;
        for (int i = 0; i < 140; ++i)
        {
            owners.push_back(pool.allocate(i));
            EXPECT_EQ(reinterpret_cast<uintptr_t>(owners.back()) % alignof(PoolOwner), 0u);
        }
        EXPECT_EQ(pool.size(), 140u);
        for (int i = 0; i < 140; ++i)
            EXPECT_EQ(owners[i]->value, i);
        auto* released = owners[64];
        pool.free(released);
        EXPECT_EQ(PoolOwner::alive, 139);
        EXPECT_EQ(pool.size(), 139u);
        EXPECT_EQ(pool.allocate(999), released);
        EXPECT_EQ(released->value, 999);
        pool.freeAll();
        EXPECT_EQ(PoolOwner::alive, 0);
        EXPECT_EQ(pool.size(), 0u);
        pool.allocate(7);
    }
    EXPECT_EQ(PoolOwner::alive, 0);
}
