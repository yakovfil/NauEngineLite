// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <algorithm>
#include <type_traits>
#include <utility>

#include "nau/3d/dag_drv3d.h"

template <typename T>
class LockedBuffer
{
  Sbuffer *mLockedObject = nullptr;
  T *mData = nullptr;
  size_t mCount = 0;

public:
  LockedBuffer(Sbuffer *locked_object, uint32_t ofs_bytes, uint32_t count, int flags) : mLockedObject(locked_object)
  {
    NAU_ASSERT(locked_object);
    // TODO: It is necessary to prohibit having count == 0
    NAU_ASSERT(!count || ofs_bytes + sizeof(T) * count <= locked_object->ressize());
    uint32_t size_bytes = count ? sizeof(T) * count : mLockedObject->ressize() - ofs_bytes;
    NAU_ASSERT(size_bytes % sizeof(T) == 0);
    mCount = size_bytes / sizeof(T);
    if (!mLockedObject->lock(ofs_bytes, size_bytes, (void **)&mData, flags) || !mData)
    {
      mLockedObject = nullptr;
      mData = nullptr;
    }
  }
  LockedBuffer(const LockedBuffer &rhs) = delete;
  LockedBuffer(LockedBuffer &&rhs) noexcept
  {
      std::swap(mLockedObject, rhs.mLockedObject);
      std::swap(mData, rhs.mData);
      std::swap(mCount, rhs.mCount);
  }
  LockedBuffer operator=(const LockedBuffer &rhs) = delete;
  LockedBuffer &operator=(LockedBuffer &&rhs) noexcept
  {
      std::swap(mLockedObject, rhs.mLockedObject);
      std::swap(mData, rhs.mData);
      std::swap(mCount, rhs.mCount);
      return *this;
  }
  ~LockedBuffer()
  {
    if (mLockedObject)
      mLockedObject->unlock();
  }

  void close()
  {
      LockedBuffer tmp(std::move(*this));
  }

  explicit operator bool() const { return mLockedObject != nullptr; }

  // TODO: [[deprecated]]
  T *get() { return mData; }

  T &operator[](size_t index)
  {
    NAU_ASSERT(index < mCount);
    return mData[index];
  }

  template <typename InputIterator>
  void updateDataRange(size_t index, InputIterator data, size_t count_elements)
  {
    NAU_ASSERT(index + count_elements <= mCount);
    std::copy_n(data, count_elements, &mData[index]);
  }
};

template <typename T>
class LockedBufferWithOffset
{
  LockedBuffer<T> &mLockedBuffer;
  size_t mOffsetInElems;

public:
  LockedBufferWithOffset(LockedBuffer<T> &locked_buffer, size_t offset_in_elements = 0) :
    mLockedBuffer(locked_buffer), mOffsetInElems(offset_in_elements)
  {}

  T &operator[](size_t index) { return mLockedBuffer[mOffsetInElems + index]; }

  LockedBufferWithOffset &operator+=(size_t count)
  {
    mOffsetInElems += count;
    return *this;
  }
};

template <typename T>
LockedBuffer<T> lock_sbuffer(Sbuffer *buf, uint32_t ofs_bytes, uint32_t count, int flags)
{
  NAU_ASSERT(buf);
  if (std::is_const_v<T>)
      flags |= VBLOCK_READONLY;
  NAU_ASSERT(std::is_const_v<T> == !!(flags & VBLOCK_READONLY));
  return LockedBuffer<T>(buf, ofs_bytes, count, flags);
}

inline bool issue_readback_query(D3dEventQuery *query, Sbuffer *buf)
{
  if (buf->lock(0, 0, static_cast<void **>(nullptr), VBLOCK_READONLY))
    buf->unlock();
  return d3d::issue_event_query(query);
}
