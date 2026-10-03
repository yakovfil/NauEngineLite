// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <vector>

#include "nau/dataBlock/dag_dataBlock.h"

template <class MESSAGE_T>
std::vector<MESSAGE_T> get_ignored_validation_messages(const nau::DataBlock& blk)
{
    std::vector<MESSAGE_T> ignoreList;

    if (auto ignoreBlock = blk.getBlockByName("ignore"))
    {
        ignoreList.reserve(ignoreBlock->paramCount());
        for (int i = 0; i < ignoreBlock->paramCount(); i++)
        {
            if (auto ignoreId = ignoreBlock->getInt(i))
            {
                ignoreList.push_back((MESSAGE_T)ignoreId);
            }
        }
  }
  return ignoreList;
}
