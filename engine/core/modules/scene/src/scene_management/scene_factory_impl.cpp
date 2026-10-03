// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "./scene_factory_impl.h"

#include <algorithm>
#include <span>
#include <vector>

#include "nau/diag/logging.h"
#include "nau/scene/components/internal/missing_component.h"
#include "nau/service/service_provider.h"
#include "scene_impl.h"
#include "scene_management/scene_builder.h"

namespace nau::scene
{
    IScene::Ptr SceneFactoryImpl::createEmptyScene()
    {
        return NauObject::classCreateInstance<SceneImpl>();
    }

    IScene::Ptr SceneFactoryImpl::createSceneFromAsset(const SceneAsset& sceneAsset, CreateSceneOptionFlag options)
    {
        NAU_ASSERT(sceneAsset.getSceneInfo().assetKind == SceneAssetKind::Scene);

        StackAllocatorUnnamed;
        ObjectUniquePtr scene = NauObject::classCreateInstance<SceneImpl>();

        SceneAssetVisitor sceneVisitor{*scene, options};
        sceneAsset.visitScene(sceneVisitor);
        sceneVisitor.finalizeConstruction(sceneAsset);

        return scene;
    }

    ObjectUniquePtr<SceneObject> SceneFactoryImpl::createSceneObjectFromAsset(const SceneAsset& sceneAsset)
    {
        return createSceneObjectFromAssetWithOptions(sceneAsset, CreateSceneOption::RecreateUid);
    }

    scene::SceneObject::Ptr SceneFactoryImpl::createSceneObjectFromAssetWithOptions(const SceneAsset& sceneAsset, scene::CreateSceneOptionFlag options)
    {
        NAU_ASSERT(sceneAsset.getSceneInfo().assetKind == SceneAssetKind::Prefab);

        StackAllocatorUnnamed;

        SceneAssetVisitor sceneVisitor{options};
        sceneAsset.visitScene(sceneVisitor);
        sceneVisitor.finalizeConstruction(sceneAsset);

        return sceneVisitor.getPrefabInstance();
    }

    ObjectUniquePtr<SceneObject> SceneFactoryImpl::createSceneObject(const rtti::TypeInfo* rootComponentType, const std::span<const rtti::TypeInfo*>)
    {
        ObjectUniquePtr<SceneComponent> rootComponent;
        if (rootComponentType)
        {
            rootComponent = createComponent(*rootComponentType);
            NAU_FATAL(rootComponent);
            NAU_FATAL(rootComponent->is<SceneComponent>(), "Root component MUST be SceneComponent");
        }
        else
        {
            rootComponent = NauObject::classCreateInstance<SceneComponent>(nullptr);
        }

        ObjectUniquePtr newObject = NauObject::classCreateInstance<SceneObject>(nullptr, std::move(rootComponent));
        if (IComponentEvents* const componentEvents = newObject->getRootComponent().as<IComponentEvents*>())
        {
            componentEvents->onComponentCreated();
        }

        return newObject;
    }

    ObjectUniquePtr<Component> SceneFactoryImpl::createComponent(const rtti::TypeInfo& type)
    {
        const IClassDescriptor::Ptr& componentClass = findComponentClass(type);
        if (!componentClass)
        {
            NAU_LOG_ERROR("Fail to create requested component. Type hash:({})", type.getHashCode());
            return createDefaultMissingComponent();
        }

        NAU_FATAL(componentClass->hasInterface<Component>(), "Broken internal logic. scene::Component api MUST be provided");
        if (!componentClass->hasInterface<Component>())
        {
            return nullptr;
        }

        NAU_FATAL(componentClass->getConstructor() != nullptr);

        IRttiObject* const component = *componentClass->getConstructor()->invoke(nullptr, {});
        NAU_FATAL(component);
        NAU_FATAL(component->is<Component>());

        return ObjectUniquePtr{component->as<Component*>()};
    }

    const std::vector<IClassDescriptor::Ptr>& SceneFactoryImpl::getComponentTypes()
    {
        if (!m_objectComponentTypes)
        {  // does not needed any thread sync. logic because scene system is single threaded.
            m_objectComponentTypes.emplace(getServiceProvider().findClasses<Component>());
            if (m_objectComponentTypes->empty())
            {
                NAU_LOG_WARNING("System doesn't provide any object component type");
            }
        }

        return *m_objectComponentTypes;
    }

    const IClassDescriptor::Ptr& SceneFactoryImpl::findComponentClass(const rtti::TypeInfo& type)
    {
        const auto& componentTypes = getComponentTypes();
        auto iter = std::find_if(componentTypes.begin(), componentTypes.end(), [&type](const IClassDescriptor::Ptr& classDesc)
        {
            return classDesc->getClassTypeInfo() == type;
        });

        return iter == componentTypes.end() ? m_emptyClassDescPtr : *iter;
    }

}  // namespace nau::scene
