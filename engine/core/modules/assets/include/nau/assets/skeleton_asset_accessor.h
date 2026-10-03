// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <optional>
#include <span>
#include <string>
#include <vector>

#include "nau/assets/asset_accessor.h"
#include "nau/async/task.h"
#include "nau/math/math.h"

namespace nau
{
    struct SkeletonDataDescriptor
    {
        unsigned jointsCount = 0;
        std::string skeletonPath;
        std::string animationPath;
    };

    struct NAU_ABSTRACT_TYPE ISkeletonAssetAccessor : IAssetAccessor
    {
        NAU_INTERFACE(nau::ISkeletonAssetAccessor, IAssetAccessor)

        virtual SkeletonDataDescriptor getDescriptor() const = 0;

        virtual void copyInverseBindMatrices(std::vector<math::mat4>& data) const = 0;
    };

}  // namespace nau
