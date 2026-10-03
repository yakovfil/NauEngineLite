// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <memory>
#include <span>
#include <utility>

template <typename T>
class DynamicArray
{
    std::unique_ptr<T[]> ptr;
    size_t count = 0;

public:
  DynamicArray() = default;
  DynamicArray(const DynamicArray &) = delete;
  DynamicArray(DynamicArray &&) = default;
  DynamicArray(T *p, size_t sz) : ptr{p}, count{sz} {}
  DynamicArray(std::unique_ptr<T[]>&& p, size_t sz) :
      ptr{std::forward<std::unique_ptr<T[]>>(p)},
      count{sz}
  {
  }
  explicit DynamicArray(size_t sz) :
      DynamicArray{std::make_unique<T[]>(sz), sz}
  {
  }
  DynamicArray &operator=(const DynamicArray &) = delete;
  DynamicArray &operator=(DynamicArray &&other)
  {
      std::swap(ptr, other.ptr);
      std::swap(count, other.count);
      return *this;
  }

  void adopt(T *new_ptr, size_t new_sz)
  {
      std::unique_ptr<T[]> newPtr{new_ptr};
      std::swap(ptr, newPtr);
      count = new_sz;
  }

  T *release()
  {
    count = 0;
    return ptr.release();
  }

  bool resize(size_t new_size)
  {
    if (count == new_size)
    {
      return false;
    }

    if (0 == new_size)
    {
      ptr.reset();
      count = 0;
      return true;
    }

    auto newBlock = std::make_unique<T[]>(new_size);
    if (!newBlock)
    {
      return false;
    }

    auto moveCount = Vectormath::min(count, new_size);
    for (uint32_t i = 0; i < moveCount; ++i)
    {
        newBlock[i] = std::move(ptr[i]);
    }
    std::swap(ptr, newBlock);
    count = new_size;
    return true;
  }

  T &operator[](size_t i) { return ptr[i]; }
  const T &operator[](size_t i) const { return ptr[i]; }
  size_t size() const { return count; }
  bool empty() const { return !ptr || 0 == count; }
  T *data() { return ptr.get(); }
  const T *data() const { return ptr.get(); }
  std::span<T> asSpan()
  {
      return {data(), size()};
  }
  std::span<const T> asSpan() const
  {
      return {data(), size()};
  }
  std::span<T> releaseAsSpan()
  {
    auto retValue = asSpan();
    release();
    return retValue;
  }
  static DynamicArray fromSpan(std::span<T> span)
  {
      return {span.data(), span.size()};
  }
  T *begin() { return ptr.get(); }
  const T *begin() const { return ptr.get(); }
  const T *cbegin() const { return begin(); }
  T *end() { return begin() + size(); }
  const T *end() const { return begin() + size(); }
  const T *cend() const { return end(); }
};
