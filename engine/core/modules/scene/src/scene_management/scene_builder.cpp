// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include "./scene_builder.h"

#include <functional>
#include <list>
#include <optional>
#include <string>
#include <unordered_map>

#include "nau/memory/scoped_standard_allocator.h"
#include "nau/memory/standard_aliases.h"
#include "nau/scene/components/internal/missing_component.h"
#include "nau/scene/scene_manager.h"
#include "nau/serialization/json.h"
#include "nau/serialization/json_utils.h"
#include "nau/service/service_provider.h"

namespace nau::scene
{
    namespace
    {
        class SceneObjectsFinalizer
        {
            template <typename T>
            using StackMap = std::unordered_map<Uid, T, std::hash<Uid>, std::equal_to<Uid>, StackStdAllocator<std::pair<const Uid, T>>>;

        public:
            SceneObjectsFinalizer(SceneObject& root, CreateSceneOptionFlag options, const std::optional<Vector<ReferenceField>>& referenceInfo) :
                m_needRecreatedUids(options.has(CreateSceneOption::RecreateUid)),
                m_needResolveReferenceFields(referenceInfo && !referenceInfo->empty())
            {
                const auto visitObject = [](SceneObject& sceneObject, void* data)
                {
                    auto& self = *reinterpret_cast<SceneObjectsFinalizer*>(data);

                    if (self.m_needResolveReferenceFields)
                    {
                        // must executed before uid re-generation
                        self.m_objects.emplace(sceneObject.getUid(), &sceneObject);
                    }

                    if (self.m_needRecreatedUids)
                    {
                        sceneObject.setUid(Uid::generate());
                    }

                    sceneObject.walkComponents([](Component& component, void* data)
                    {
                        auto& self = *reinterpret_cast<SceneObjectsFinalizer*>(data);

                        if (self.m_needResolveReferenceFields)
                        {
                            // must executed before uid re-generation
                            self.m_components.emplace(component.getUid(), &component);
                        }
                        if (self.m_needRecreatedUids)
                        {
                            component.setUid(Uid::generate());
                        }

                        if (IComponentEvents* const componentEvents = component.as<IComponentEvents*>())
                        {
                            self.m_allComponentEvents.push_back(componentEvents);
                        }

                        return true;
                    }, data, false);

                    return true;
                };

                visitObject(root, this);
                root.walkChildObjects(visitObject, this, true);

                if (m_needResolveReferenceFields)
                {
                    resolveReferenceFields(*referenceInfo);
                }

                for (IComponentEvents* componentEvents : m_allComponentEvents)
                {
                    componentEvents->onAfterComponentRestored();
                }
            }

        private:
            /**
                @brief resolve all reference fields, specified by referenceInfo
                    First, will try to find a component/object in the current structure: i.e. references between local reference objects.
                    If there is no object with the requested uid, it will try to find a global object (in active scenes)
             */
            void resolveReferenceFields(const Vector<ReferenceField>& referenceInfo)
            {
                if (referenceInfo.empty())
                {
                    return;
                }

                // uid: actually is original uid at this moment all objects uids are regenerated,
                // but m_allObjects and m_allComponents keeps references by old uids.
                const auto findObjectByUid = [this](const Uid& uid) -> ObjectWeakRef<>
                {
                    auto object = m_objects.find(uid);
                    return object != m_objects.end() ? ObjectWeakRef<>{*object->second} : ObjectWeakRef<>{};
                };

                const auto findComponentByUid = [this](const Uid& uid) -> ObjectWeakRef<>
                {
                    auto component = m_components.find(uid);
                    return component != m_components.end() ? ObjectWeakRef<>{*component->second} : ObjectWeakRef{};
                };

                const auto resolveQuery = [&](const SceneQuery& query) -> ObjectWeakRef<>
                {
                    if (query.uid == NullUid)
                    {
                        return nullptr;
                    }

                    // First try to find the object (or component) in the current object hierarchy (i.e. the object we are currently building):
                    //  - if category is specified then look in corresponding object collection
                    ObjectWeakRef<> weakRef;
                    if (query.category)
                    {
                        weakRef = (*query.category == QueryObjectCategory::Object) ? findObjectByUid(query.uid) : findComponentByUid(query.uid);
                    }
                    //  - if category is not specified then look both scene objects and components
                    else if (weakRef = findObjectByUid(query.uid); !weakRef)
                    {
                        weakRef = findComponentByUid(query.uid);
                    }

                    // Finally, if the reference is not resolved from the local hierarchy, then  will try to resolve it from global objects (i.e. active scene)
                    return weakRef ? weakRef : getServiceProvider().get<ISceneManager>().querySingleObject(query);
                };

                for (const ReferenceField& field : referenceInfo)
                {
                    auto componentEntry = m_components.find(field.componentUid);
                    if (componentEntry == m_components.end())
                    {
                        continue;
                    }

                    Component& component = *componentEntry->second;
                    RuntimeValue::Ptr fieldValue = component.getValue(strings::toStringView(field.fieldPath));
                    if (!fieldValue)
                    {
                        NAU_LOG_WARNING("The object has no field:({})", field.fieldPath);
                        continue;
                    }

                    RuntimeObjectWeakRefValue* const weakRefField = fieldValue->as<RuntimeObjectWeakRefValue*>();
                    if (!weakRefField)
                    {
                        NAU_LOG_WARNING("The field:({}) is expected to be a reference value.", field.fieldPath);
                        continue;
                    }

                    const SceneQuery objectQuery = weakRefField->getObjectQuery();
                    const ObjectWeakRef<> weakRef = resolveQuery(objectQuery);
                    weakRefField->setObjectWeakRef(weakRef);
                }
            }

            const bool m_needRecreatedUids;
            const bool m_needResolveReferenceFields;
            StackMap<SceneObject*> m_objects;
            StackMap<Component*> m_components;
            std::list<IComponentEvents*, StackStdAllocator<IComponentEvents*>> m_allComponentEvents;
        };

    }  // namespace

    SceneAssetVisitor::SceneAssetVisitor(CreateSceneOptionFlag options) :
        m_sceneFactory(getServiceProvider().get<ISceneFactory>()),
        m_options(options),
        m_buildPrefabMode(true)
    {
    }

    SceneAssetVisitor::SceneAssetVisitor(IScene& targetScene, CreateSceneOptionFlag options) :
        m_sceneFactory(getServiceProvider().get<ISceneFactory>()),
        m_rootObjectRef(targetScene.getRoot()),
        m_options(options),
        m_buildPrefabMode(false)
    {
    }

    void SceneAssetVisitor::finalizeConstruction(const SceneAsset& sceneAsset)
    {
        SceneObjectsFinalizer{*m_rootObjectRef, m_options, sceneAsset.getReferencesInfo()};
    }

    ObjectUniquePtr<SceneObject> SceneAssetVisitor::getPrefabInstance()
    {
        NAU_FATAL(m_buildPrefabMode, "getPrefabInstance should only be used for prefab build mode.");
        return std::move(m_prefabObject);
    }

    bool SceneAssetVisitor::visitSceneObject(Uid parentObjectUid, const SceneObjectAsset& objectAsset)
    {
        if (!m_rootObjectRef)
        {
            NAU_FATAL(m_buildPrefabMode);
            NAU_ASSERT(parentObjectUid == NullUid);
            NAU_ASSERT(!m_prefabObject, "Only one root object is expected when building prefab asset.");

            m_prefabObject = createObject(objectAsset);
            m_rootObjectRef = *m_prefabObject;
        }
        else if (parentObjectUid == SceneObjectAsset::SceneVirtualRootUid)
        {
            NAU_FATAL(!m_buildPrefabMode);
            SceneObject& root = getObject(NullUid);
            m_allObjects.emplace(root.getUid(), &root);

            root.setName(objectAsset.name);
            root.setUid(objectAsset.uid);
            fillComponent(root.getRootComponent(), objectAsset.rootComponent);
        }
        else
        {
            SceneObject& parent = getObject(parentObjectUid);
            ObjectUniquePtr child = createObject(objectAsset);
            parent.attachChild(std::move(child));
        }

        return true;
    }

    bool SceneAssetVisitor::visitSceneComponent(Uid parentObjectUid, const ComponentAsset& componentAsset)
    {
        SceneObject& object = getObject(parentObjectUid == SceneObjectAsset::SceneVirtualRootUid ? NullUid : parentObjectUid);

        const rtti::TypeInfo componentType = componentAsset.getComponentType();
        NAU_ASSERT(componentType);
        if (!componentType)
        {
            return true;
        }

        Component& component = object.addComponent(componentType);
        if (component.is<IMissingComponent>()) [[unlikely]]
        {
            component.as<IMissingComponent&>().setComponentData(componentAsset);
        }

        fillComponent(component, componentAsset);

        return true;
    }

    ObjectUniquePtr<SceneObject> SceneAssetVisitor::createObject(const SceneObjectAsset& objectAsset)
    {
        NAU_ASSERT(objectAsset.uid != NullUid);
        const rtti::TypeInfo componentType = objectAsset.rootComponent.getComponentType();

        ObjectUniquePtr object = componentType ? m_sceneFactory.createSceneObject(&componentType) : m_sceneFactory.createSceneObject();
        NAU_FATAL(object);

        if (object->getRootComponent().is<IMissingComponent>())
        {
            object->getRootComponent().as<IMissingComponent&>().setComponentData(objectAsset.rootComponent);
        }


        object->setUid(objectAsset.uid);
        object->setName(objectAsset.name);

        fillComponent(object->getRootComponent(), objectAsset.rootComponent);
        m_allObjects.emplace(object->getUid(), object.get());

        return object;
    }

    void SceneAssetVisitor::fillComponent(Component& component, const ComponentAsset& componentAsset)
    {
        if (componentAsset.properties)
        {
#if 0  // Debug dump component properties 
            std::basic_string<char> buffer;
            io::InplaceStringWriter<char> writer{buffer};
            serialization::jsonWrite(writer, makeValueRef(componentAsset.properties, getDefaultAllocator()), serialization::JsonSettings{.pretty = true}).ignore();
            std::cout << buffer.c_str() << std::endl;
#endif
            RuntimeValue::Ptr propsTarget = rtti::staticCast<RuntimeValue*>(&component);
            auto result = RuntimeValue::assign(propsTarget, componentAsset.properties);
            if (!result)
            {
                NAU_LOG_ERROR("Fail to restore component ():({})", result.getError()->getMessage());
            }
        }

        if (component.getUid() == NullUid)
        {
            // in general uid can be obtained directly from properties
            // if not - it will be take from asset or just generated
            const Uid uid = componentAsset.uid != NullUid ? componentAsset.uid : Uid::generate();
            component.setUid(uid);
        }

        if (componentAsset.transform)
        {
            SceneComponent* const sceneComponent = component.as<SceneComponent*>();
            if (sceneComponent)
            {
                sceneComponent->setTransform(*componentAsset.transform);
            }
        }
    }

    SceneObject& SceneAssetVisitor::getObject(Uid uid)
    {
        if (uid == NullUid || (m_rootObjectRef && m_rootObjectRef->getUid() == uid))
        {
            return *m_rootObjectRef;
        }

        auto iter = m_allObjects.find(uid);
        NAU_FATAL(iter != m_allObjects.end(), "Scene Visitor broken logic. Object not found ({})", toString(uid));

        return *iter->second;
    }

}  // namespace nau::scene
