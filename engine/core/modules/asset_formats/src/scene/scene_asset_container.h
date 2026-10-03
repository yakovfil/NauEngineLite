// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "nau/assets/asset_container.h"
#include "nau/rtti/rtti_impl.h"
#include "scene_serialization.h"

namespace nau
{
    /**
     */
    class SceneAssetContainer : public IAssetContainer
    {
        NAU_CLASS_(nau::SceneAssetContainer, IAssetContainer)

    public:
        using ObjectsMap = std::unordered_map<unsigned, SerializedSceneObject>;

        SceneAssetContainer(SceneHeader&& sceneHeader, ObjectsMap&& sceneObjects);

        const SceneHeader& getSceneHeader() const;

        const ObjectsMap& getSceneObjects() const;

    private:
        nau::Ptr<> getAsset(std::string_view path) override;

        std::vector<std::string> getContent() const override;

        const SceneHeader m_sceneHeader;
        const ObjectsMap m_sceneObjects;
    };

    /**
     */
    class SceneAssetLoader final : public IAssetContainerLoader,
                                   public IRttiObject
    {
        NAU_RTTI_CLASS(nau::SceneAssetLoader, IAssetContainerLoader, IRttiObject)

    private:
        std::vector<std::string_view> getSupportedAssetKind() const override;

        async::Task<IAssetContainer::Ptr> loadFromStream(io::IStreamReader::Ptr, AssetContentInfo info) override;

        RuntimeReadonlyDictionary::Ptr getDefaultImportSettings() const override;
    };

}  // namespace nau
