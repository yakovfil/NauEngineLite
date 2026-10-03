// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/animation/components/skeleton_component.h"

#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/base/span.h>

#include <string>
#include <vector>

#include "nau/math/math.h"
#include "nau/scene/scene_object.h"

namespace nau
{
    NAU_IMPLEMENT_DYNAMIC_OBJECT(SkeletonComponent)

    bool SkeletonComponent::drawDebugSkeletons = false;

    SkeletonComponent::SkeletonComponent()
    {
        m_name = "Skeleton Component";
    }

    const std::string& SkeletonComponent::getName() const
    {
        return m_name;
    }

    SkeletonAssetRef& SkeletonComponent::getSkeletonAsset() const
    {
        return m_skeletonAsset;
    }

    nau::Ptr<SkeletonAssetView> SkeletonComponent::getSkeletonAssetView() const noexcept
    {
        return m_skeletonAssetView;
    }

    void SkeletonComponent::setSkeletonAsset(SkeletonAssetRef asset)
    {
        m_skeletonAsset = std::move(asset);
    }

    void SkeletonComponent::setSkeletonAssetView(nau::Ptr<SkeletonAssetView> assetView)
    {
        m_skeletonAssetView = assetView;

        models.resize(getSkeleton().num_joints());

        animRuntimeData.locals.resize(getSkeleton().num_soa_joints());

        setSkeletonToDefaultPose();
    }

    void SkeletonComponent::setSkeletonToDefaultPose()
    {
        const std::vector<math::Matrix4>& bindPose = m_skeletonAssetView->getDefaultPoseTransforms();

        static_assert(sizeof(decltype(*models.data())) == sizeof(decltype(*bindPose.data())));
        std::memcpy(models.data(), bindPose.data(), getSkeleton().num_joints() * sizeof(decltype(*models.data())));
    }

    const ozz::animation::Skeleton& SkeletonComponent::getSkeleton() const
    {
        return m_skeletonAssetView->getSkeleton();
    }

    const std::vector<SkeletonJoint>& SkeletonComponent::getJoints() const
    {
        return m_skeletonAssetView->getJoints();
    }

    const std::vector<nau::math::Matrix4>& SkeletonComponent::getInverseBindTransforms() const
    {
        return m_skeletonAssetView->getInverseBindTransforms();
    }

    const ozz::vector<ozz::math::Float4x4>& SkeletonComponent::getModelSpaceJointMatrices() const
    {
        return models;
    }

    ozz::vector<ozz::math::Float4x4>& SkeletonComponent::getModelSpaceJointMatricesMutable()
    {
        return models;
    }

    SkeletalAnimRuntimeData& SkeletonComponent::getAnimRuntimeDataMutable()
    {
        return animRuntimeData;
    }

    unsigned SkeletonComponent::getBonesCount() const
    {
        return models.size();
    }

}  // namespace nau
