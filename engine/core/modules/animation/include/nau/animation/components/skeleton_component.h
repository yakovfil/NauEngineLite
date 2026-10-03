// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/maths/vec_float.h>

#include <map>
#include <string>
#include <vector>

#include "nau/animation/assets/skeleton_asset.h"
#include "nau/animation/playback/animation.h"
#include "nau/assets/asset_ref.h"
#include "nau/scene/components/component_attributes.h"
#include "nau/scene/components/component_life_cycle.h"
#include "nau/scene/components/internal/component_internal_attributes.h"
#include "nau/scene/components/scene_component.h"

namespace nau
{
    struct ISkeletonAnimatableKeyFrameType
    {
    };

    struct SkeletalTrackData final
    {
        float weight;
        animation::AnimationBlendMethod blendMethod;

        ozz::animation::SamplingJob::Context animSamplingContext;
        ozz::vector<ozz::math::SoaTransform> locals;
    };

    struct SkeletalAnimRuntimeData final
    {
        std::map<nau::string, SkeletalTrackData> tracks;

        // Buffer of local transforms after blending is performed
        ozz::vector<ozz::math::SoaTransform> locals;
    };

    class NAU_ANIMATION_EXPORT SkeletonComponent : public scene::SceneComponent,
                                                   public nau::animation::IAnimatable
    {
        NAU_OBJECT(nau::SkeletonComponent, scene::SceneComponent, nau::animation::IAnimatable)
        NAU_DECLARE_DYNAMIC_OBJECT


        NAU_CLASS_ATTRIBUTES(
            CLASS_ATTRIBUTE(scene::SystemComponentAttrib, true),
            CLASS_ATTRIBUTE(scene::ComponentDisplayNameAttrib, "Skeleton"),
            CLASS_ATTRIBUTE(scene::ComponentDescriptionAttrib, "Skeleton (description)"))

        NAU_CLASS_FIELDS(
            CLASS_NAMED_FIELD(m_skeletonAsset, "skeletonAsset"), )

    public:
        SkeletonComponent();

        const std::string& getName() const;

        SkeletonAssetRef& getSkeletonAsset() const;
        nau::Ptr<SkeletonAssetView> getSkeletonAssetView() const noexcept;
        void setSkeletonAsset(SkeletonAssetRef assetRef);

        void setSkeletonAssetView(nau::Ptr<SkeletonAssetView> assetView);

        void setSkeletonToDefaultPose();

        const ozz::animation::Skeleton& getSkeleton() const;
        const std::vector<SkeletonJoint>& getJoints() const;
        const std::vector<nau::math::Matrix4>& getInverseBindTransforms() const;

        unsigned getBonesCount() const;

        const ozz::vector<ozz::math::Float4x4>& getModelSpaceJointMatrices() const;

        ozz::vector<ozz::math::Float4x4>& getModelSpaceJointMatricesMutable();

        SkeletalAnimRuntimeData& getAnimRuntimeDataMutable();

        static bool drawDebugSkeletons;  // todo: DevOptions (?)

    private:
        mutable SkeletonAssetRef m_skeletonAsset;  // for serialization

        nau::Ptr<SkeletonAssetView> m_skeletonAssetView;

        SkeletalAnimRuntimeData animRuntimeData;

        // Buffer of model space matrices.
        ozz::vector<ozz::math::Float4x4> models;

        std::string m_name;
    };
}  // namespace nau
