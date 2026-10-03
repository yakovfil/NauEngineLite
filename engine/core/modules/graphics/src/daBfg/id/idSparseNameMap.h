// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "nau/utils/dag_oaHashNameMap.h"

template <class EnumType>
struct IdSparseNameMap
{
  void add(const char *name, EnumType id)
  {
    auto idx = names.addNameId(name);
    if (idx >= ids.size())
      ids.emplace_back() = id;
  }

  EnumType get(const char *name) const
  {
    if (auto nameId = names.getNameId(name); nameId != -1)
      return ids[nameId];
    else
      return EnumType::Invalid;
  }

private:
  dag::OAHashNameMap<false> names;
  std::vector<EnumType> ids;
};
