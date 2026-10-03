// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <optional>
#include <string>
#include <vector>

#include "nau/assets/scene_asset.h"

namespace nau
{
    /**
     */
    struct ObjectsBlockInfo
    {
        size_t offset;
        size_t size;

        NAU_CLASS_FIELDS(
            CLASS_FIELD(offset),
            CLASS_FIELD(size))
    };

    struct SerializedSceneObject : public SceneObjectAsset
    {
        unsigned parentLocalId = 0;
        unsigned localId = 0;

        // At the moment it is assumed that there is always only one root in the scene. This is a field for checking the validity of the data
        bool isSceneRoot = false;
        std::vector<ComponentAsset> additionalComponents;
        std::vector<unsigned> childLocalIds;

        NAU_CLASS_BASE(SceneObjectAsset)

        NAU_CLASS_FIELDS(
            CLASS_FIELD(parentLocalId),
            CLASS_FIELD(localId),
            CLASS_FIELD(isSceneRoot),
            CLASS_FIELD(additionalComponents),
            CLASS_FIELD(childLocalIds))
    };

    struct SceneHeader
    {
        SceneAssetKind assetKind = SceneAssetKind::Undefined;
        std::string name;
        std::string version;
        std::string objectsContentFormat = "application/json";
        std::vector<ObjectsBlockInfo> objects;
        std::vector<unsigned> topLevelObjectIds;
        std::optional<Vector<ReferenceField>> referencesInfo;

        NAU_CLASS_FIELDS(
            CLASS_FIELD(assetKind),
            CLASS_FIELD(name),
            CLASS_FIELD(version),
            CLASS_FIELD(objectsContentFormat),
            CLASS_FIELD(objects),
            CLASS_FIELD(topLevelObjectIds),
            CLASS_FIELD(referencesInfo))
    };

}  // namespace nau
