// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <array>
#include <concepts>
#include <span>

#include "nau/assets/scene_asset.h"
#include "nau/rtti/ptr.h"
#include "nau/scene/scene.h"
#include "nau/scene/scene_object.h"
#include "nau/utils/typed_flag.h"

namespace nau::scene
{
    enum class CreateSceneOption
    {
        RecreateUid = NauFlag(1)
    };

    NAU_DEFINE_TYPED_FLAG(CreateSceneOption)

    /**
     */
    struct NAU_ABSTRACT_TYPE ISceneFactory
    {
        NAU_TYPEID(nau::scene::ISceneFactory)

        virtual ~ISceneFactory() = default;

        virtual IScene::Ptr createEmptyScene() = 0;

        virtual IScene::Ptr createSceneFromAsset(const SceneAsset& sceneAsset, CreateSceneOptionFlag options = {}) = 0;

        virtual SceneObject::Ptr createSceneObjectFromAsset(const SceneAsset& sceneAsset) = 0;

        virtual SceneObject::Ptr createSceneObject(const rtti::TypeInfo* rootComponentType = nullptr, const std::span<const rtti::TypeInfo*> components = {}) = 0;

        template <std::derived_from<SceneComponent> ComponentType, std::derived_from<Component>... MoreComponents>
        ObjectUniquePtr<SceneObject> createSceneObject()
        {
            static_assert(sizeof...(MoreComponents) == 0, "Not implemented");

            auto obj = createSceneObject(&rtti::getTypeInfo<ComponentType>());
            return obj;
        }
    };

    /**
     */
    NAU_CORESCENE_EXPORT SceneAsset::Ptr wrapSceneAsAsset(IScene::WeakRef sceneRef);

    /**
     */
    NAU_CORESCENE_EXPORT SceneAsset::Ptr wrapSceneObjectAsAsset(ObjectWeakRef<SceneObject> sceneObjectRef);

}  // namespace nau::scene

namespace nau::scene_internal
{
    /**
        @brief The API is solely intended for use in test projects only.
     */
    struct NAU_ABSTRACT_TYPE ISceneFactoryInternal
    {
        NAU_TYPEID(nau::scene_internal::ISceneFactoryInternal)

        virtual ~ISceneFactoryInternal() = default;

        /**
            @brief same as ISceneFactory::createSceneObjectFromAsset, but allow to specify option flags (to test object copying with no uid overriding)
         */
        virtual scene::SceneObject::Ptr createSceneObjectFromAssetWithOptions(const SceneAsset& sceneAsset, scene::CreateSceneOptionFlag options) = 0;
    };

}  // namespace nau::scene_internal
