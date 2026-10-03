// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include <memory>
#include <vector>

#include "gltf/gltf_file.h"
#include "nau/animation/data/frame.h"
#include "nau/assets/animation_asset_accessor.h"
#include "nau/assets/skeleton_asset_accessor.h"
#include "nau/io/file_system.h"
#include "nau/rtti/rtti_impl.h"

namespace nau
{
    class NanimStreamAssetContainer;

    class NanimAnimationAssetAccessor final : public IAnimationAssetAccessor, public ISkeletonAssetAccessor
    {
        NAU_CLASS_(nau::NanimAnimationAssetAccessor, IAnimationAssetAccessor, ISkeletonAssetAccessor)

        struct KeyFramesData;
        struct SkeletalTracksData;

   public:
        explicit NanimAnimationAssetAccessor(NanimStreamAssetContainer& container);
        NanimAnimationAssetAccessor(const io::FsPath& containerFilePath, size_t animationIndex, size_t channelIndex);
        ~NanimAnimationAssetAccessor();

        virtual AnimationDataDescriptor getDataDescriptor() const override;
        virtual async::Task<> copyVectors(const AnimationDataDescriptor& desc, TTimesContainer& times, TDataContainer<math::vec3>& data) const override;
        virtual async::Task<> copyRotations(const AnimationDataDescriptor& desc, TTimesContainer& times, TDataContainer<math::quat>& data) const override;
        virtual async::Task<> copyFramesData(const AnimationDataDescriptor& desc, DataBlock& data) const override;
        virtual nau::Ptr<ISkeletonAssetAccessor> getSkeletonAsset() const override;

        virtual SkeletonDataDescriptor getDescriptor() const override;
        virtual void copyInverseBindMatrices(std::vector<math::mat4>& data) const override;

    private:
        void fromBlk(const class DataBlock& blk);

    private:
        AnimationDataDescriptor m_descriptor;
        std::unique_ptr<KeyFramesData> m_data;
        std::unique_ptr<SkeletalTracksData> m_skeletalData;
    };

}  // namespace nau
