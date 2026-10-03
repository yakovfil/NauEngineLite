// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include <span>
#include <vector>

#include "gltf/gltf_file.h"
#include "nau/assets/mesh_asset_accessor.h"
#include "nau/io/file_system.h"
#include "nau/rtti/rtti_impl.h"

namespace nau
{
    class GltfMeshAssetAccessor final : public IMeshAssetAccessor
    {
        NAU_CLASS_(nau::GltfMeshAssetAccessor, IMeshAssetAccessor)
    public:
        GltfMeshAssetAccessor(const GltfFile& file, size_t meshIndex, const std::vector<io::IFile::Ptr>& buffers);

        ElementFormatFlag getSupportedIndexTypes() const override;

        MeshDescription getDescription() const override;

        std::vector<VertAttribDescription> getVertAttribDescriptions() const override;

        Result<> copyVertAttribs(std::span<OutputVertAttribDescription>) const override;

        Result<> copyIndices(void* outputBuffer, size_t outputBufferSize, ElementFormat outputIndexFormat) const override;

    private:

        struct BinaryAccessor
        {
            VertAttribDescription* attrib;
            size_t offset;
            size_t size;
            void* memory;
            io::IFile::Ptr file;
        };

        MeshDescription m_meshDescription;
        std::vector<VertAttribDescription> m_vertAttributes;
        std::vector<BinaryAccessor> m_binaryAccessors;
    };

}  // namespace nau
