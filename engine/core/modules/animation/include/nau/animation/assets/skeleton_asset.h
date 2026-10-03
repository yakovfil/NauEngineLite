// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <ozz/animation/runtime/skeleton.h>

#include <string>
#include <vector>

#include "nau/assets/asset_view.h"
#include "nau/async/task.h"
#include "nau/math/math.h"
#include "nau/rtti/rtti_impl.h"

namespace nau
{
    struct SkeletonJoint
    {
        unsigned parentIndex;
        std::string jointName;
    };

    class NAU_ANIMATION_EXPORT SkeletonAssetView : public IAssetView
    {
        NAU_CLASS_(nau::SkeletonAssetView, IAssetView)
    public:
        static async::Task<nau::Ptr<SkeletonAssetView>> createFromAssetAccessor(nau::Ptr<> accessor);

        const std::vector<math::Matrix4>& getInverseBindTransforms() const;
        const std::vector<math::Matrix4>& getDefaultPoseTransforms() const;

        const ozz::animation::Skeleton& getSkeleton() const;
        const std::vector<SkeletonJoint>& getJoints() const;

    private:
        std::vector<SkeletonJoint> joints;
        std::vector<math::Matrix4> inverseBindTransforms;
        std::vector<math::Matrix4> defaultPoseTransforms;

        ozz::animation::Skeleton skeleton;
    };

}  // namespace nau
