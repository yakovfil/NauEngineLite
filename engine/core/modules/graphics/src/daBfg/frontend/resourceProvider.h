// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <variant>

#include "dabfg/id/idIndexedMapping.h"
#include "nau/3d/dag_resPtr.h"
#include "nau/math/math.h"
#include "nau/utils/sorted_vector.h"
#include "render/daBfg/detail/autoResTypeNameId.h"
#include "render/daBfg/detail/blob.h"
#include "render/daBfg/detail/resNameId.h"

namespace dabfg
{

    using ProvidedResource = std::variant<ManagedTexView, ManagedBufView, dabfg::BlobView>;

    struct ResourceProvider
    {
        nau::SortedVectorMap<ResNameId, ProvidedResource> providedResources;
        nau::SortedVectorMap<ResNameId, ProvidedResource> providedHistoryResources;

        IdIndexedMapping<AutoResTypeNameId, nau::math::IVector2> resolutions;

        void clear()
        {
            providedResources.clear();
            providedHistoryResources.clear();
        }
};

} // namespace dabfg
