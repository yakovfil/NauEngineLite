// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <daECS/core/internal/dataComponentManager.h>

#include <algorithm>
#include <limits>
#include <type_traits>
#include <utility>

#include "specialized_memcpy.h"

namespace ecs
{

__forceinline void DataComponentManager::addToChunk(Chunk &chunk, const uint32_t id_in_chunk, uint32_t entity_size,
  const uint8_t *__restrict data, const uint16_t *__restrict component_sz, const uint16_t *__restrict component_ofs)
{
  NAU_FAST_ASSERT(chunk.data);
  const uint32_t shift = chunk.getCapacityBits();
  for (uint8_t *__restrict compStream = chunk.getData(), *__restrict end = compStream + (entity_size << shift); compStream != end;
       component_sz++, component_ofs++)
  {
    const uint16_t csz = *component_sz;
    // optimize for typical sizes
    SPECIALIZE_MEMCPY_TO_SOA(csz, compStream, id_in_chunk, data + *component_ofs);
    compStream += csz << shift;
  }
}

inline uint32_t DataComponentManager::allocateChunk(const uint32_t entity_size, uint8_t capacity_bits)
{
    capacity_bits = std::clamp(capacity_bits, initialBits, (uint8_t)MAX_CAPACITY_BITS);
    currentCapacityBits = std::max(currentCapacityBits, capacity_bits);
    Chunk chunk(getAllocateSize(capacity_bits, entity_size), capacity_bits);
    totalEntitiesCapacity += chunk.getCapacity();
    if (!aliasedChunks.isArray() && aliasedChunks.getSingleChunk().data == NULL)
    {
        aliasedChunks.getSingleChunk().swap(std::move(chunk));
        return 0;
    }
  else
  {
    auto &chunksArray = aliasedChunks.getArray();
    chunksArray.emplace_back(std::move(chunk));
    return (uint32_t)chunksArray.size() - 1;
  }
}

inline void DataComponentManager::removeChunk(uint32_t c)
{
  if (c >= getChunksCount())
    return;
  NAU_ASSERT(totalEntitiesCapacity >= getChunk(c).getCapacity());
  if (NAU_UNLIKELY(getChunkUsed(c)))
  {
    NAU_ASSERT(0, "removing used chunk={}(out of {})! {} used", c, getChunksCount(), getChunkUsed(c));
    return;
  }
  else
      totalEntitiesCapacity = std::max((int)0, (int)totalEntitiesCapacity - (int)getChunk(c).getCapacity());
  aliasedChunks.eraseChunk(c);
  workingChunk = 0;
}

inline DataComponentManager::Chunk &DataComponentManager::allocateEmpty(chunk_type_t &chunkId, uint32_t &id, uint32_t entity_size)
{
  // select suitable chunk
  Chunk *__restrict chunk = &getChunk(workingChunk);
#if NAU_DEBUG
  NAU_ASSERT(chunk->getUsed() < std::numeric_limits<std::remove_reference<decltype(id)>::type>::max());
#endif
  const uint32_t cUsed = chunk->getUsed();
  if (NAU_LIKELY(cUsed < chunk->getCapacity() && isUnlocked()))  // current chunk has no sufficient memory
  {
    // do the allocation
    id = cUsed; // we always add to the end
    chunkId = workingChunk;
    return *chunk;
  }
  return allocateEmptyInNewChunk(chunkId, id, entity_size);
}

inline void DataComponentManager::allocated(chunk_type_t chunkId)
{
  unlock();
  totalEntitiesUsed++;
  getChunk(chunkId).entitiesUsed++; // re-read memory
}

inline void DataComponentManager::allocate(chunk_type_t &chunkId, uint32_t &id, uint32_t entity_size, const uint8_t *__restrict data,
  const uint16_t *__restrict component_sz, const uint16_t *__restrict component_ofs)
{
  // select suitable chunk
  Chunk &destChunk = allocateEmpty(chunkId, id, entity_size);
  addToChunk(destChunk, id, entity_size, data, component_sz, component_ofs);
  totalEntitiesUsed++;
  destChunk.entitiesUsed++;
}

inline bool DataComponentManager::removeFromChunk(chunk_type_t chunkId, uint32_t index, uint32_t entity_size,
  const uint16_t *__restrict component_sz,
  uint32_t &moved_index) // moved_index has became index
{
  DAECS_EXT_ASSERTF_RETURN(chunkId < getChunksCount(), false, "{} chunk < {}", chunkId, getChunksCount());
  Chunk &chunk = getChunksPtr()[chunkId];

  DAECS_EXT_ASSERTF_RETURN(index < chunk.getUsed(), false, "{}>={} chunk {}", index, chunk.getUsed(), chunkId);

  --totalEntitiesUsed;
  --chunk.entitiesUsed;
  if (index == chunk.entitiesUsed)
    return false;

  const uint32_t shift = chunk.getCapacityBits();
  moved_index = chunk.entitiesUsed;
  for (uint8_t *compStream = chunk.getData(), *end = compStream + (entity_size << shift); compStream < end; component_sz++)
  {
    uint32_t csz = *component_sz;
    if (csz)
    {
      // optimize for typical sizes
      SPECIALIZE_MEMCPY_IN_SOA(csz, compStream, index, moved_index);
      compStream += csz << shift;
    }
  }
  return true;
}

}; // namespace ecs