// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <EASTL/allocator.h>
#include <EASTL/map.h>
#include <EASTL/vector.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

TEST(EastlAllocator, AlignmentAndOffsets)
{
    eastl::allocator allocator;
    for (size_t alignment : {size_t{1}, size_t{16}, size_t{32}, size_t{64}, size_t{256}})
    {
        for (size_t offset : {size_t{0}, size_t{1}, size_t{7}, size_t{513}})
        {
            void* ptr = allocator.allocate(517, alignment, offset);
            ASSERT_NE(ptr, nullptr);
            // Match the bundled EASTL allocate_memory contract: ptr - offset is aligned.
            EXPECT_EQ((reinterpret_cast<uintptr_t>(ptr) - offset) % alignment, 0);
            std::memset(ptr, 0x5a, 517);
            allocator.deallocate(ptr, 517);
        }
    }
    void* ptr = allocator.allocate(3);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % alignof(std::max_align_t), 0);
    allocator.deallocate(ptr, 3);
    allocator.deallocate(nullptr, 0);
}

TEST(EastlAllocator, ReallocationPreservesDataAndAlignment)
{
    eastl::allocator allocator;
    auto* ptr = static_cast<unsigned char*>(allocator.allocate(31, 128, 7));
    ASSERT_NE(ptr, nullptr);
    std::memset(ptr, 0xa5, 31);
    for (size_t size : {size_t{4096}, size_t{17}})
    {
        auto* resized = static_cast<unsigned char*>(allocator.realloc(ptr, size));
        ASSERT_NE(resized, nullptr);
        ptr = resized;
        EXPECT_EQ((reinterpret_cast<uintptr_t>(ptr) - 7) % 128, 0);
        for (size_t i = 0; i < 17; ++i)
        {
            EXPECT_EQ(ptr[i], 0xa5);
        }
    }
    EXPECT_EQ(allocator.realloc(ptr, std::numeric_limits<size_t>::max()), nullptr);
    EXPECT_EQ(ptr[0], 0xa5);
    EXPECT_EQ(allocator.realloc(ptr, 0), nullptr);
    ptr = static_cast<unsigned char*>(allocator.realloc(nullptr, 19));
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % alignof(std::max_align_t), 0);
    allocator.deallocate(ptr, 19);
}

TEST(EastlAllocator, RejectsInvalidAndOverflowingRequests)
{
    eastl::allocator allocator;
    EXPECT_EQ(allocator.allocate(12, 0, 0), nullptr);
    EXPECT_EQ(allocator.allocate(12, 3, 0), nullptr);
    EXPECT_EQ(allocator.allocate(std::numeric_limits<size_t>::max(), 64, 0), nullptr);
}

TEST(EastlAllocator, OverAlignedContainers)
{
    struct alignas(128) Value
    {
        int number = 0;
    };
    eastl::map<int, Value> map;
    eastl::vector<Value> vector;
    for (int i = 0; i < 100; ++i)
    {
        map[i].number = i;
        vector.push_back(Value{i});
    }
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(reinterpret_cast<uintptr_t>(&map[i]) % alignof(Value), 0);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(&vector[i]) % alignof(Value), 0);
        EXPECT_EQ(map[i].number, i);
        EXPECT_EQ(vector[i].number, i);
    }
}
