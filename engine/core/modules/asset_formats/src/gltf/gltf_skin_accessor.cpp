// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "gltf_skin_accessor.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace nau
{
    GltfSkinAssetAccessor::GltfSkinAssetAccessor(const GltfFile& file, size_t skinIndex, std::string_view skeletonPath, const std::vector<io::IFile::Ptr>& bufferFiles)
    {
        NAU_ASSERT(skinIndex < file.skins.size());

        const GltfSkin& skinData = file.skins[skinIndex];

        const GltfAccessor& dataAccessor = file.accessors[skinData.inverseBindMatrices];
        const GltfBufferView& dataBufferView = file.bufferViews[dataAccessor.bufferView];

        m_descriptor.skeletonDesc = SkeletonDataDescriptor{static_cast<unsigned>(skinData.joints.size()), std::string(skeletonPath)};

        auto& invBindMatricesAccessor = m_descriptor.inverseBindMatricesAccessor;
        invBindMatricesAccessor.file = bufferFiles[dataBufferView.buffer];
        invBindMatricesAccessor.offset = dataBufferView.byteOffset;
        invBindMatricesAccessor.size = dataBufferView.byteLength;
    }

    SkeletonDataDescriptor GltfSkinAssetAccessor::getDescriptor() const
    {
        return m_descriptor.skeletonDesc;
    }

    void GltfSkinAssetAccessor::copyInverseBindMatrices(std::vector<nau::math::Matrix4>& data) const
    {
        auto inputStream = m_descriptor.inverseBindMatricesAccessor.file->createStream(io::AccessMode::Read);
        inputStream->setPosition(io::OffsetOrigin::Begin, m_descriptor.inverseBindMatricesAccessor.offset);

        auto* reader = inputStream->as<io::IStreamReader*>();
        NAU_ASSERT(reader);

        if (reader)
        {
            data.resize(m_descriptor.skeletonDesc.jointsCount);
            io::copyFromStream(data.data(), m_descriptor.inverseBindMatricesAccessor.size, *reader).ignore();
        }

        return;
    }

}  // namespace nau
