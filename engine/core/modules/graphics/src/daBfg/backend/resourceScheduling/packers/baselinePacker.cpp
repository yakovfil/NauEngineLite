// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include <vector>

#include "dabfg/backend/resourceScheduling/packer.h"

namespace dabfg
{

class BaselinePacker
{
public:
  PackerOutput operator()(PackerInput input)
  {
    uint64_t offset = 0;
    offsets.resize(input.resources.size(), PackerOutput::NOT_ALLOCATED);
    for (uint32_t i = 0; i < input.resources.size(); ++i)
    {
      const auto resSize = input.resources[i].sizeWithPadding(offset);
      if (NAU_LIKELY(resSize <= input.maxHeapSize && offset <= input.maxHeapSize - resSize))
      {
        offsets[i] = input.resources[i].doAlign(offset);
        offset += resSize;
      }
      else
      {
        offsets[i] = PackerOutput::NOT_SCHEDULED;
      }
    }
    PackerOutput output;
    output.offsets = offsets;
    output.heapSize = offset;
    return output;
  }

private:
    std::vector<uint64_t> offsets;  // std::vector
};

Packer make_baseline_packer() { return BaselinePacker(); }

} // namespace dabfg
