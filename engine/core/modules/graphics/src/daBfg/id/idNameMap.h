// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <limits>
#include <string_view>
#include <type_traits>

#include "nau/utils/dag_oaHashNameMap.h"
#include "nau/utils/enum_utils.h"

template <class EnumType>
struct IdNameMap : dag::OAHashNameMap<false>
{
  using Container = dag::OAHashNameMap<false>;
  using Container::Container;

  EnumType addNameId(std::string_view view)
  {
    const auto result = Container::addNameId(view.data(), view.size());
    NAU_FAST_ASSERT(result < std::numeric_limits<std::underlying_type_t<EnumType>>::max());
    return static_cast<EnumType>(result);
  }

  EnumType getNameId(std::string_view view) const
  {
    auto result = Container::getNameId(view.data(), view.size());
    return result < 0 ? EnumType::Invalid : static_cast<EnumType>(result);
  }

  std::string_view getName(EnumType id) const
  {
      return getNameCstr(id);
  }

  const char *getNameCstr(EnumType id) const
  {
      auto result = Container::getName(static_cast<int>(nau::to_underlying(id)));
      return result == nullptr ? "INVALID_NAME_ID" : result;
  }
};
