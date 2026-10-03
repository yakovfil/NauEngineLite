// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <gtest/gtest.h>

#include <list>
#include <map>
#include <memory>
#include <vector>

#include "nau/memory/scoped_standard_allocator.h"
#include "nau/memory/standard_domains.h"
#include "nau/utils/dynamic_bit_vector.h"

namespace
{
    struct alignas(128) Value
    {
        int number = 0;
    };
}  // namespace

TEST(StandardAllocator, EngineArrayAndNodeDomains)
{
    std::vector<Value, nau::ArrayStdAllocator<Value>> values;
    using Pair = std::pair<const int, Value>;
    std::map<int, Value, std::less<int>, nau::PoolStdAllocator<Pair>> nodes;
    std::list<Value, nau::PoolStdAllocator<Value>> list;
    for (int i = 0; i < 100; ++i)
    {
        values.push_back(Value{i});
        nodes.emplace(i, Value{i});
        list.push_back(Value{i});
    }
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(values[i].number, i);
        EXPECT_EQ(nodes.at(i).number, i);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(&values[i]) % 128, 0);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(&nodes.at(i)) % 128, 0);
    }
    for (const auto& value : list)
    {
        EXPECT_EQ(reinterpret_cast<uintptr_t>(&value) % 128, 0);
    }
    auto moved = std::move(nodes);
    EXPECT_EQ(moved.size(), 100);
    auto extracted = moved.extract(7);
    nodes.insert(std::move(extracted));
    EXPECT_EQ(nodes.at(7).number, 7);
}

TEST(StandardAllocator, InvalidRawRequestsDoNotReachBackend)
{
    struct Backend
    {
        int calls = 0;
        void* allocate(size_t)
        {
            ++calls;
            return nullptr;
        }
        void deallocate(void*)
        {
        }
    } backend;
    EXPECT_EQ(nau::memory_detail::allocateStandardStorage(backend, 12, 0), nullptr);
    EXPECT_EQ(nau::memory_detail::allocateStandardStorage(backend, 12, 3), nullptr);
    EXPECT_EQ(nau::memory_detail::allocateStandardStorage(backend, SIZE_MAX, 128), nullptr);
    EXPECT_EQ(backend.calls, 0);
}

namespace
{
    template <typename Allocator, typename Backend>
    void checkScopedTransfers(Backend& first, Backend& second)
    {
        using Values = std::vector<Value, Allocator>;
        Values source{Allocator(first)};
        source.push_back(Value{7});
        auto copy = source;
        EXPECT_EQ(copy.get_allocator().resource(), &first);
        Values destination{Allocator(second)};
        destination = copy;
        EXPECT_EQ(destination.get_allocator().resource(), &second);
        source.push_back(Value{9});
        destination = std::move(source);
        EXPECT_EQ(destination.get_allocator().resource(), &second);
        EXPECT_EQ(destination.at(1).number, 9);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(destination.data()) % alignof(Value), 0);
        nau::swapResourceContainers(copy, destination);
        EXPECT_EQ(copy.size(), 2);
        EXPECT_EQ(destination.size(), 1);
        EXPECT_EQ(copy.get_allocator().resource(), &first);
        EXPECT_EQ(destination.get_allocator().resource(), &second);
        Values equal{Allocator(first)};
        equal.push_back(Value{11});
        const Value* stable = equal.data();
        nau::swapResourceContainers(copy, equal);
        EXPECT_EQ(copy.data(), stable);
    }
}  // namespace

TEST(StandardAllocator, FrameResourceTransfersBeforeReset)
{
    nau::FrameAllocator first, second;
    checkScopedTransfers<nau::FrameStdAllocator<Value>>(first, second);
    {
        using Allocator = nau::FrameStdAllocator<nau::BitVectorWord>;
        nau::DynamicBitVector<Allocator> left{Allocator(first)}, right{Allocator(second)};
        left.set(65, true);
        right.set(200, true);
        left.swap(right);
        EXPECT_EQ(left.get_allocator().resource(), &first);
        EXPECT_EQ(right.get_allocator().resource(), &second);
        EXPECT_EQ(left.size(), 201);
        EXPECT_TRUE(left.test(200));
        EXPECT_TRUE(right.test(65));
        right = std::move(left);
        EXPECT_TRUE(left.empty());
        EXPECT_TRUE(right.test(200));
        EXPECT_EQ(right.get_allocator().resource(), &second);
    }
    EXPECT_TRUE(first.prepareFrame());
    EXPECT_TRUE(second.prepareFrame());
}

TEST(StandardAllocator, StackResourceTransfersBeforeDestruction)
{
    nau::StackAllocator<16384> first, second;
    checkScopedTransfers<nau::StackStdAllocator<Value>>(first, second);
    EXPECT_TRUE(first.isClear());
    EXPECT_TRUE(second.isClear());
}
