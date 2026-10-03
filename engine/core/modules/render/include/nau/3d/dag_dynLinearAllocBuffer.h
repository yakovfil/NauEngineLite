// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <string>
#include <utility>

#include "nau/3d/dag_resPtr.h"
//#include <debug/daNAU_ASSERT.h>

template <typename T>
struct DynLinearAllocBuffer
{
    DynLinearAllocBuffer(std::string name) :
        resName(std::move(name))
    {
    }

  DynLinearAllocBuffer(DynLinearAllocBuffer &&) = default;
  DynLinearAllocBuffer &operator=(DynLinearAllocBuffer &&) = default;

  DynLinearAllocBuffer(DynLinearAllocBuffer &) = delete;
  DynLinearAllocBuffer &operator=(DynLinearAllocBuffer &) = delete;

  uint32_t update(const T *data, uint32_t num_elems)
  {
    const uint32_t elemSize = sizeof(T);

    uint32_t currentFrame = ::dagor_frame_no();
    uint32_t updateFlags = VBLOCK_NOOVERWRITE;

    if (updateFrame != currentFrame)
    {
      elemOffset = 0;
      updateFrame = currentFrame;
      updateFlags = VBLOCK_DISCARD;
    }

    if (!handle || elemOffset + num_elems > allocatedElems)
    {
      if (handle)
        NAU_LOG_WARNING("Buffer {} was reallocated. Size {} is not enough.", resName.c_str(), allocatedElems);

      const uint32_t BUFFER_SIZE_MULTIPLIER = 100000;
      const uint32_t elemsToAllocate = (num_elems + BUFFER_SIZE_MULTIPLIER - 1) / BUFFER_SIZE_MULTIPLIER * BUFFER_SIZE_MULTIPLIER;

      handle.close();
      handle = dag::create_sbuffer(elemSize, elemsToAllocate, SBCF_UA_SR_STRUCTURED | SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE, 0,
        resName.c_str());

      elemOffset = 0;
      allocatedElems = elemsToAllocate;
      updateFlags = VBLOCK_DISCARD;
    }

    handle->updateData(elemOffset * elemSize, num_elems * elemSize, data, updateFlags);
    const uint32_t retOffset = elemOffset;
    elemOffset += num_elems;
    return retOffset;
  }

  void close()
  {
    handle.close();
    elemOffset = 0;
    allocatedElems = 0;
    updateFrame = 0;
  }

  Sbuffer *getBuffer() const { return handle.getBuf(); }

private:
    std::string resName;
    UniqueBufHolder handle;

    uint32_t elemOffset = 0;
    uint32_t allocatedElems = 0;

    uint32_t updateFrame = 0;
};