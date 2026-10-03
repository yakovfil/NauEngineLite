// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <optional>
#include <string>

#include "nau/rtti/rtti_impl.h"
#include "nau/scene/components/internal/missing_component.h"
#include "nau/scene/scene_factory.h"
#include "nau/scene/scene_manager.h"

namespace nau::scene
{
    /**
     */
    class SceneAssetWrapper final : public SceneAsset
    {
        NAU_CLASS_(nau::scene::SceneAssetWrapper, SceneAsset)

    public:
        SceneAssetWrapper(IScene& scene);
        SceneAssetWrapper(SceneObject& rootObject);
        SceneAssetWrapper() = delete;
        SceneAssetWrapper(const SceneAssetWrapper&) = delete;

    private:
        static void applyComponentAsset(ComponentAsset& componentAsset, Component& component);

        void visitSceneObjectRecursive(SceneObject& sceneObject, ISceneAssetVisitor& visitor) const;

        SceneAssetInfo getSceneInfo() const override;

        std::optional<Vector<ReferenceField>> getReferencesInfo() const override;

        void visitScene(ISceneAssetVisitor& visitor) const override;

        mutable ObjectWeakRef<SceneObject> m_rootObjectRef;
        const bool m_prefabMode;
    };

    SceneAssetWrapper::SceneAssetWrapper(IScene& scene) :
        m_rootObjectRef(scene.getRoot()),
        m_prefabMode(false)
    {
    }

    SceneAssetWrapper::SceneAssetWrapper(SceneObject& rootObject) :
        m_rootObjectRef(rootObject),
        m_prefabMode(true)
    {
    }

    void SceneAssetWrapper::applyComponentAsset(ComponentAsset& componentAsset, Component& component)
    {
        if (component.is<IMissingComponent>())
        {
            component.as<IMissingComponent&>().fillComponentData(componentAsset);
            return;
        }

        // TODO: think what should return (non C++) script component ? (type of Script Component?)
        const rtti::TypeInfo& componentType = component.getClassDescriptor()->getClassTypeInfo();

        componentAsset.componentTypeId = componentType.getHashCode();
        componentAsset.uid = component.getUid();
        if (const SceneComponent* const sceneComponent = component.as<const SceneComponent*>())
        {
            componentAsset.transform = sceneComponent->getTransform();
        }
        else
        {
            componentAsset.transform = std::nullopt;
        }

        componentAsset.properties = nau::Ptr{component.as<RuntimeValue*>()};
    }

    void SceneAssetWrapper::visitSceneObjectRecursive(SceneObject& sceneObject, ISceneAssetVisitor& visitor) const
    {
        const auto children = sceneObject.getDirectChildObjects();
        const auto components = sceneObject.getDirectComponents();
        const Uid parentObjectUid = EXPR_Block->Uid
        {
            if (const SceneObject* parentObject = sceneObject.getParentObject())
            {
                if (m_prefabMode && &sceneObject == m_rootObjectRef.get())
                {
                    // Prefab mode: wrapped object can have parent, but when wrapping object (not scene) that parent must be ignored
                    // because this (parent) object will not reach the client through the visitor.
                    return NullUid;
                }

                return parentObject->getUid();
            }

            // for scene root must specify SceneVirtualRootUid as parent
            return m_prefabMode ? NullUid : SceneObjectAsset::SceneVirtualRootUid;
        };

        const Uid thisObjectUid = sceneObject.getUid();

        SceneObjectAsset objectAsset = {
            .uid = thisObjectUid,
            .name{std::string(sceneObject.getName())},
            .childCount = children.size(),
            .additionalComponentCount = components.size()};

        applyComponentAsset(objectAsset.rootComponent, sceneObject.getRootComponent());
        visitor.visitSceneObject(parentObjectUid, objectAsset);

        for (Component* const component : components)
        {
            if (component == &sceneObject.getRootComponent())
            {
                // always ignore root component
                continue;
            }

            NAU_FATAL(component);
            ComponentAsset componentAsset;
            applyComponentAsset(componentAsset, *component);
            visitor.visitSceneComponent(thisObjectUid, componentAsset);
        }

        for (SceneObject* const childObject : children)
        {
            NAU_FATAL(childObject);
            visitSceneObjectRecursive(*childObject, visitor);
        }
    }

    SceneAssetInfo SceneAssetWrapper::getSceneInfo() const
    {
        SceneAssetInfo info{
            .assetKind = m_prefabMode ? SceneAssetKind::Prefab : SceneAssetKind::Scene};

        info.name = "";

        return info;
    }

    std::optional<Vector<ReferenceField>> SceneAssetWrapper::getReferencesInfo() const
    {
        NAU_ASSERT(m_rootObjectRef);
        if (!m_rootObjectRef)
        {
            return {};
        }

        Vector<ReferenceField> allReferences;
        m_rootObjectRef->walkComponents([](Component& component, void* data)
        {
            // TODO: only direct fields currently are collected.
            // In the future this needs to be improved so that the traversal over fields is carried out recursively: also for of structure and collection values.
            auto& references = *reinterpret_cast<decltype(allReferences)*>(data);
            auto& dynObject = static_cast<DynamicObject&>(component);

            for (size_t i = 0, size = dynObject.getSize(); i < size; ++i)
            {
                auto [fieldName, fieldValue] = dynObject[i];

                if (fieldValue->is<scene::RuntimeObjectWeakRefValue>())
                {
                    auto& referenceFieldValue = fieldValue->as<scene::RuntimeObjectWeakRefValue&>();
                    if (ObjectWeakRef<> weakRef = referenceFieldValue.getObjectWeakRef())
                    {
                        ReferenceField& fieldData = references.emplace_back();
                        fieldData.fieldPath.assign(fieldName.data(), fieldName.size());
                        fieldData.componentUid = component.getUid();
                    }
                }
            }

            return true;
        }, &allReferences, true);

        return allReferences;
    }

    void SceneAssetWrapper::visitScene(ISceneAssetVisitor& visitor) const
    {
        NAU_ASSERT(m_rootObjectRef);
        if (!m_rootObjectRef)
        {
            return;
        }

        visitSceneObjectRecursive(*m_rootObjectRef, visitor);
    }

    SceneAsset::Ptr wrapSceneAsAsset(IScene::WeakRef sceneRef)
    {
        NAU_ASSERT(sceneRef);
        if (!sceneRef)
        {
            return nullptr;
        }

        return rtti::createInstance<SceneAssetWrapper>(*sceneRef);
    }

    SceneAsset::Ptr wrapSceneObjectAsAsset(ObjectWeakRef<SceneObject> sceneObjectRef)
    {
        NAU_ASSERT(sceneObjectRef);
        if (!sceneObjectRef)
        {
            return nullptr;
        }

        return rtti::createInstance<SceneAssetWrapper>(*sceneObjectRef);
    }
}  // namespace nau::scene
