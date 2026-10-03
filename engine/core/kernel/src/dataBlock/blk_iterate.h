// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <nau/dataBlock/dag_dataBlock.h>

#include <deque>

namespace nau
{

    template <typename Cb, typename Alloc = std::allocator<const DataBlock*>>
    static inline void iterate_blocks_bfs(const DataBlock& db, Cb cb)
    {
        std::deque<const DataBlock*, Alloc> queue;
        queue.push_back(&db);
        while (!queue.empty())
        {
            auto node = queue.front();
            queue.pop_front();
            cb(*node);
            for (uint32_t i = 0, e = node->blockCount(); i < e; ++i)
                if (auto b = node->getBlock(i))
                    queue.push_back(b);
        }
    }
}  // namespace nau