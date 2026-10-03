// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <span>

#include "nau/hash/sha1.h"
#include "nau/util/dag_strUtil.h"

struct ShaderHashValue
{
  typedef uint8_t ValueType[20];
  ValueType value;

  friend bool operator==(const ShaderHashValue &l, const ShaderHashValue &r)
  {
    static_assert(sizeof(ShaderHashValue) == sizeof(ValueType));
    static_assert(20 == sizeof(ValueType));
    return memcmp(l.value, r.value, sizeof(ValueType)) == 0;
  }

  friend bool operator!=(const ShaderHashValue &l, const ShaderHashValue &r) { return !(l == r); }

  template <typename T>
  static ShaderHashValue calculate(std::span<const T> data)
  {
    ShaderHashValue value;
    sha1_csum(reinterpret_cast<const unsigned char *>(data.data()), static_cast<int>(data_size(data)),
      reinterpret_cast<unsigned char *>(value.value));
    return value;
  }

  template <typename T>
  static ShaderHashValue calculate(const T *data, size_t count)
  {
    ShaderHashValue value;
    sha1_csum(reinterpret_cast<const unsigned char *>(data), static_cast<int>(count * sizeof(T)),
      reinterpret_cast<unsigned char *>(value.value));
    return value;
  }

  static ShaderHashValue fromString(const char *str, int len = -1)
  {
    ShaderHashValue value;
    str_hex_to_data_buf(value.value, sizeof(value.value), str, nullptr, len);
    return value;
  }

  void convertToString(char *buffer, size_t size) const
  {
    NAU_ASSERT(size > (sizeof(ShaderHashValue) * 2));
    data_to_str_hex_buf(buffer, size, value, sizeof(value));
  }
};
