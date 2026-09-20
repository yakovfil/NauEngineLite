// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#pragma once

#include <array>
#include <vector>

#include "nau/assets/asset_container.h"
#include "nau/assets/asset_view.h"
#include "nau/assets/asset_view_factory.h"
#include "nau/io/file_system.h"
#include "nau/rtti/rtti_impl.h"

namespace nau
{
    struct CpuAssetFile
    {
        std::string path;
        std::vector<std::byte> bytes;
    };

    // Owns immutable copies of explicitly supplied package files for mounting into Nau VFS.
    Result<io::IFileSystem::Ptr> createCpuAssetFileSystem(std::vector<CpuAssetFile> files);

    class CpuMeshView final : public IAssetView
    {
        NAU_CLASS_(nau::CpuMeshView, IAssetView)
    public:
        std::vector<std::array<float, 3>> positions;
        std::vector<std::array<float, 3>> normals;
        std::vector<uint16_t> indices;
    };

    class CpuMeshContainerLoader final : public IAssetContainerLoader
    {
        NAU_INTERFACE(nau::CpuMeshContainerLoader, IAssetContainerLoader)
    public:
        eastl::vector<eastl::string_view> getSupportedAssetKind() const override;
        async::Task<IAssetContainer::Ptr> loadFromStream(io::IStreamReader::Ptr, AssetContentInfo) override;
        RuntimeReadonlyDictionary::Ptr getDefaultImportSettings() const override
        {
            return nullptr;
        }
    };

    class CpuMeshViewFactory final : public IAssetViewFactory,
                                     public IRefCounted
    {
        NAU_CLASS_(nau::CpuMeshViewFactory, IAssetViewFactory, IRefCounted)
    public:
        eastl::vector<const rtti::TypeInfo*> getAssetViewTypes() const override;
        async::Task<IAssetView::Ptr> createAssetView(nau::Ptr<>, const rtti::TypeInfo&) override;
    };
}  // namespace nau
