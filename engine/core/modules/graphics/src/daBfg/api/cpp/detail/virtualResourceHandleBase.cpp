// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include "render/daBfg/detail/virtualResourceHandleBase.h"

#include <tuple>
#include <variant>

#include "dabfg/frontend/resourceProvider.h"

namespace dabfg::detail
{

template <class T>
T VirtualResourceHandleBase::getResourceView() const
{
  auto& storage = resUid.history ? provider->providedHistoryResources : provider->providedResources;
  if (auto it = storage.find(resUid.resId); it != storage.end())
  {
      return std::get<T>(it->second);
  }
  else
  {
      return {};
  }
}

template ManagedTexView VirtualResourceHandleBase::getResourceView<ManagedTexView>() const;
template ManagedBufView VirtualResourceHandleBase::getResourceView<ManagedBufView>() const;
template BlobView VirtualResourceHandleBase::getResourceView<BlobView>() const;

} // namespace dabfg::detail
