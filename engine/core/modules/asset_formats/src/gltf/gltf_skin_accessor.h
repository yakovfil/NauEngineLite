// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include <string_view>
#include <vector>

#include "gltf/gltf_file.h"
#include "nau/assets/skeleton_asset_accessor.h"
#include "nau/io/file_system.h"
#include "nau/rtti/rtti_impl.h"

namespace nau
{
    class GltfSkinAssetAccessor final : public ISkeletonAssetAccessor
    {
        NAU_CLASS_(nau::GltfSkinAssetAccessor, ISkeletonAssetAccessor)

        struct BinaryAccessor
        {
            io::IFile::Ptr file;
            size_t offset;
            size_t size;
        };

        struct GltfSkinDataDescriptor
        {
            SkeletonDataDescriptor skeletonDesc;
            BinaryAccessor inverseBindMatricesAccessor;
        };

    public:
        GltfSkinAssetAccessor(const GltfFile& file, size_t skinIndex, std::string_view skeletonPath, const std::vector<io::IFile::Ptr>& bufferFiles);

        virtual SkeletonDataDescriptor getDescriptor() const override;
        virtual void copyInverseBindMatrices(std::vector<nau::math::Matrix4>& data) const override;

    private:
        GltfSkinDataDescriptor m_descriptor;
    };

}  // namespace nau
