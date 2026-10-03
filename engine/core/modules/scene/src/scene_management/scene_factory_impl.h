// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <span>
#include <vector>

#include "nau/dispatch/class_descriptor.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/scene/internal/component_factory.h"
#include "nau/scene/scene_factory.h"

namespace nau::scene
{
    class SceneFactoryImpl final : public ISceneFactory,
                                   public scene_internal::ISceneFactoryInternal,
                                   public IComponentFactory
    {
        NAU_CLASS_BASE(ISceneFactory, scene_internal::ISceneFactoryInternal,IComponentFactory)
        NAU_TYPEID(nau::scene::SceneFactoryImpl)

    private:
        IScene::Ptr createEmptyScene() override;

        IScene::Ptr createSceneFromAsset(const SceneAsset& sceneAsset, CreateSceneOptionFlag options) override;

        ObjectUniquePtr<SceneObject> createSceneObjectFromAsset(const SceneAsset& sceneAsset) override;

        scene::SceneObject::Ptr createSceneObjectFromAssetWithOptions(const SceneAsset& sceneAsset, scene::CreateSceneOptionFlag options) override;

        scene::ObjectUniquePtr<SceneObject> createSceneObject(const rtti::TypeInfo* rootComponentType, const std::span<const rtti::TypeInfo*>) override;

        scene::ObjectUniquePtr<Component> createComponent(const rtti::TypeInfo& type) override;

        const std::vector<IClassDescriptor::Ptr>& getComponentTypes();

        const IClassDescriptor::Ptr& findComponentClass(const rtti::TypeInfo& type);

        std::optional<std::vector<IClassDescriptor::Ptr>> m_objectComponentTypes;
        const IClassDescriptor::Ptr m_emptyClassDescPtr = {};
    };

}  // namespace nau::scene