// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <deque>
#include <utility>
#include <vector>

#include "nau/memory/mem_allocator.h"
#include "nau/util/dag_generationRefId.h"
#include "nau/utils/dynamic_bit_vector.h"

// template class containing referenced by weak reference with generation data
// only available functions doesReferenceExist, destroyReference, allocateOne
//  ReferenceType has to be GenerationRefId or has GENERATION_BITS, static make, .generation(), .index(), and default constructor

template <class ReferenceType, class DataType, class GenerationType = typename std::conditional<ReferenceType::GENERATION_BITS <= 8, uint8_t, uint16_t>::type, int MinimumFreeIndices = 16>
class GenerationReferencedData
{
public:
  typedef GenerationType generation_type;
  typedef DataType data_type;
  const int minimum_free_indices = MinimumFreeIndices;

  GenerationReferencedData() : dataList(nullptr) {}
  ~GenerationReferencedData()
  {
    clear();
    if (dataList)
    {
        nau::getDefaultAllocator()->deallocate(dataList);
    }
  }

  GenerationReferencedData(GenerationReferencedData &&r) : dataList(nullptr) { swap(r); }

  GenerationReferencedData &operator=(GenerationReferencedData &&r)
  {
    swap(r);
    return *this;
  }

  GenerationReferencedData(const GenerationReferencedData &) = delete;
  GenerationReferencedData &operator=(const GenerationReferencedData &) = delete;

  inline bool doesReferenceExist(ReferenceType e) const
  {
    const unsigned idx = e.index();
    bool r = idx < genList.size() && genList[idx] == e.generation();
    NAU_ASSERT(!r || aliveList.test(idx, false));
    return r;
  }
  inline bool destroyReference(ReferenceType e)
  {
    const unsigned idx = e.index();
    if (!aliveList.test(idx, false))
      return false;
    generation_type &gen = genList[idx];
    if (gen != e.generation())
      return false;

    dataList[idx].~DataType();
    aliveList.set(idx, false);
    gen++;
    freeIndices.push_back(idx);
    return true;
  }
  template <class... Args>
  inline ReferenceType emplaceOne(Args &&...args)
  {
    unsigned idx;
    if (freeIndices.size() > minimum_free_indices)
    {
      idx = freeIndices.front();
      freeIndices.pop_front();
      new(dataList + idx /*, _NEW_INPLACE*/) DataType(std::forward<Args>(args)...);
      aliveList.set(idx, true);
      return ReferenceType::make(idx, genList[idx]);
    }
    idx = (uint32_t)genList.size();
    NAU_ASSERT_RETURN(idx < (1 << ReferenceType::INDEX_BITS), ReferenceType());
    const generation_type initialGen = 0;

    unsigned oldSize = genList.size();
    unsigned oldCapacity = genList.capacity();
    genList.emplace_back(initialGen);

    if (oldCapacity < genList.capacity())
      growCapacity(oldSize, genList.capacity());

    new(dataList + idx /*, _NEW_INPLACE*/) DataType(std::forward<Args>(args)...);
    aliveList.set(idx, true);
    return ReferenceType::make(idx, initialGen);
  }
  inline ReferenceType allocateOne() { return emplaceOne(); }
  const DataType *cget(ReferenceType ref) const
  {
    const unsigned idx = ref.index();
    if (idx < genList.size() && genList[idx] == ref.generation())
    {
      NAU_ASSERT(aliveList.test(idx, false));
      return dataList + idx;
    }
    return nullptr;
  }
  const DataType *get(ReferenceType ref) const { return cget(ref); }
  DataType *get(ReferenceType ref) { return const_cast<DataType *>(cget(ref)); }

  unsigned totalSize() const { return genList.size(); }
  unsigned freeIndicesSize() const { return freeIndices.size(); }
  ReferenceType createReferenceFromIdx(unsigned idx)
  {
    return aliveList.test(idx, false) ? ReferenceType::make(idx, genList[idx]) : ReferenceType();
  }
  ReferenceType getRefByIdx(uint32_t idx) const
  {
    return aliveList.test(idx, false) ? ReferenceType::make(idx, genList[idx]) : ReferenceType::INVALID;
  }
  const DataType *cgetByIdx(unsigned idx) const { return aliveList.test(idx, false) ? dataList + idx : nullptr; }
  const DataType *getByIdx(unsigned idx) const { return cgetByIdx(idx); }
  DataType *getByIdx(unsigned idx) { return const_cast<DataType *>(cgetByIdx(idx)); }

  bool empty() const { return genList.size() == freeIndices.size(); }

  void clear()
  {
    for (int i = 0; i < genList.size(); ++i)
    {
      if (aliveList.test(i, false))
        dataList[i].~DataType();
    }

    genList.clear();
    aliveList.clear();
    freeIndices.clear();
  }

  void swap(GenerationReferencedData &r)
  {
      std::swap(dataList, r.dataList);
      std::swap(genList, r.genList);
      std::swap(aliveList, r.aliveList);
      std::swap(freeIndices, r.freeIndices);
  }

protected:
  void growCapacity(unsigned old_size, unsigned new_capacity)
  {
    DataType* newData = static_cast<DataType*>(nau::getDefaultAllocator()->allocate(size_t(new_capacity) * sizeof(DataType)));
    for (unsigned i = 0; i < old_size; ++i)
    {
      if (aliveList.test(i, false))
      {
          new(newData + i /*, _NEW_INPLACE*/) DataType(std::move(dataList[i]));
          dataList[i].~DataType();  //-V595
      }
    }

    if (dataList)
    {
        nau::getDefaultAllocator()->deallocate(dataList);
    }
    dataList = newData;
  }
  __forceinline bool doesReferenceExist(ReferenceType e, unsigned &idx) const
  {
    idx = e.index();
    bool r = idx < genList.size() && genList[idx] == e.generation();
    NAU_ASSERT(!r || aliveList.test(idx, false));
    return r;
  }

  DataType *dataList;
  std::vector<generation_type> genList;
  nau::DynamicBitVector<> aliveList;
  std::deque<uint32_t> freeIndices;
};
