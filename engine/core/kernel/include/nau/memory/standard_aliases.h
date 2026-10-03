// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <functional>
#include <list>
#include <map>
#include <vector>

#include "nau/memory/scoped_standard_allocator.h"
#include "nau/memory/standard_domains.h"

namespace nau
{
    template <typename T>
    using Vector = std::vector<T, ArrayStdAllocator<T>>;
    template <typename T>
    using StackVector = std::vector<T, StackStdAllocator<T>>;
    template <typename T>
    using FrameVector = std::vector<T, FrameStdAllocator<T>>;

    template <typename Key, typename T, typename Compare = std::less<Key>>
    using Map = std::map<Key, T, Compare, PoolStdAllocator<std::pair<const Key, T>>>;
    template <typename Key, typename T, typename Compare = std::less<Key>>
    using StackMap = std::map<Key, T, Compare, StackStdAllocator<std::pair<const Key, T>>>;
    template <typename Key, typename T, typename Compare = std::less<Key>>
    using FrameMap = std::map<Key, T, Compare, FrameStdAllocator<std::pair<const Key, T>>>;
    template <typename T>
    using List = std::list<T, PoolStdAllocator<T>>;
}  // namespace nau
