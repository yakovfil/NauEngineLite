// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <unordered_map>

#include "nau/assets/scene_asset.h"
#include "nau/memory/standard_aliases.h"
#include "nau/scene/components/component_life_cycle.h"
#include "nau/scene/scene.h"
#include "nau/scene/scene_factory.h"

namespace nau::scene
{
    /**
     */
    class SceneAssetVisitor final : public ISceneAssetVisitor
    {
    public:
        SceneAssetVisitor(CreateSceneOptionFlag options);
        SceneAssetVisitor(IScene& targetScene, CreateSceneOptionFlag options);

        void finalizeConstruction(const SceneAsset& sceneAsset);

        ObjectUniquePtr<SceneObject> getPrefabInstance();

    private:
        bool visitSceneObject(Uid parentObjectUid, const SceneObjectAsset& childObject) override;

        bool visitSceneComponent(Uid parentObjectUid, const ComponentAsset& component) override;

        ObjectUniquePtr<SceneObject> createObject(const SceneObjectAsset& objectAsset);

        void fillComponent(Component& component, const ComponentAsset& componentAsset);

        SceneObject& getObject(Uid uid);

        ISceneFactory& m_sceneFactory;
        ObjectWeakRef<SceneObject> m_rootObjectRef;
        ObjectUniquePtr<SceneObject> m_prefabObject;
        const CreateSceneOptionFlag m_options;
        const bool m_buildPrefabMode;
        std::unordered_map<Uid, SceneObject*> m_allObjects;
        //
    };
}  // namespace nau::scene
