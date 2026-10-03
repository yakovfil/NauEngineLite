// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include <vector>

#include "gltf/gltf_file.h"
#include "nau/animation/data/frame.h"
#include "nau/assets/animation_asset_accessor.h"
#include "nau/io/file_system.h"
#include "nau/rtti/rtti_impl.h"

namespace nau
{
    class GltfAnimationAssetAccessor final : public IAnimationAssetAccessor
    {
        NAU_CLASS_(nau::GltfAnimationAssetAccessor, IAnimationAssetAccessor)

        struct BinaryAccessor
        {
            io::IFile::Ptr file;
            size_t offset;
            size_t size;
        };

        struct GltfAnimDataDescriptor
        {
            AnimationDataDescriptor animationDesc;
            BinaryAccessor timesAccessor;
            BinaryAccessor dataAccessor;
        };

    public:
        GltfAnimationAssetAccessor(const GltfFile& file, size_t animationIndex, size_t channelIndex, const std::vector<io::IFile::Ptr>& bufferFiles);

        virtual AnimationDataDescriptor getDataDescriptor() const override;
        virtual async::Task<> copyVectors(const AnimationDataDescriptor& desc, TTimesContainer& times, TDataContainer<math::vec3>& data) const override;
        virtual async::Task<> copyRotations(const AnimationDataDescriptor& desc, TTimesContainer& times, TDataContainer<math::quat>& data) const override;
        virtual async::Task<> copyFramesData(const AnimationDataDescriptor& desc, DataBlock& data) const override;
        virtual nau::Ptr<struct ISkeletonAssetAccessor> getSkeletonAsset() const override;

    private:
        GltfAnimDataDescriptor m_descriptor;

        template <typename TData>
        friend void readBinaryData(const BinaryAccessor&, std::vector<TData>&);

        template <typename TData>
        void copyTrackData(const AnimationDataDescriptor& desc, TTimesContainer&, TDataContainer<TData>&) const;
    };

}  // namespace nau
