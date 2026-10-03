// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include <optional>
#include <string>

#include "nau/assets/asset_view.h"
#include "nau/math/transform.h"
#include "nau/memory/standard_aliases.h"
#include "nau/meta/class_info.h"
#include "nau/rtti/type_info.h"
#include "nau/scene/scene_query.h"
#include "nau/serialization/runtime_value.h"
#include "nau/utils/enum/enum_reflection.h"
#include "nau/utils/functor.h"
#include "nau/utils/uid.h"

namespace nau
{
    NAU_DEFINE_ENUM_(SceneAssetKind,
                     Undefined,
                     Scene,
                     Prefab);

    /**
        @brief Reference information
     */
    struct ReferenceField
    {
        Uid componentUid;
        std::string fieldPath;

        NAU_CLASS_FIELDS(
            CLASS_FIELD(componentUid),
            CLASS_FIELD(fieldPath)
        )
    };

    /**
        @brief Scene Asset info
     */
    struct SceneAssetInfo
    {
        SceneAssetKind assetKind;
        std::string name;

        NAU_CLASS_FIELDS(
            CLASS_FIELD(assetKind),
            CLASS_FIELD(name))
    };

    /**
     * @brief Encapsulates a component of a game object.
     */
    struct ComponentAsset
    {
        /**
         * @brief Component type id.
         */
        size_t componentTypeId = 0;

        /**
         * @brief Component identifier.
         */
        Uid uid = NullUid;

        /**
         * @brief Component transform within the active scene.
         */
        std::optional<math::Transform> transform;

        /**
         * @brief Properties of the component.
         */
        RuntimeValue::Ptr properties;

        NAU_CLASS_FIELDS(
            CLASS_FIELD(componentTypeId),
            CLASS_FIELD(uid),
            CLASS_FIELD(transform),
            CLASS_FIELD(properties))

        const rtti::TypeInfo getComponentType() const
        {
            return rtti::makeTypeInfoFromId(componentTypeId);
        }

        template <rtti::WithTypeInfo T>
        void setComponentType()
        {
            componentTypeId = rtti::getTypeInfo<T>().getHashCode();
        }

        void setComponentType(const rtti::TypeInfo& type)
        {
            componentTypeId = type.getHashCode();
        }
    };

    /**
        @brief
     */
    struct SceneObjectAsset
    {
        static inline Uid SceneVirtualRootUid = *Uid::parseString("00000000-0000-0000-0000-000000000001");

        /**
         * @brief Object identifier.
         */
        Uid uid;

        /**
         * @brief Name of the object.
         */
        std::string name;

        /**
         * @brief Number of object's children.
         */
        size_t childCount;

        /**
         * @brief Number of object components (excluding its root component).
         */
        size_t additionalComponentCount;

        /**
         * @brief Object's root component, i.e. its transform provider and its component hierarchy base.
         */
        ComponentAsset rootComponent;

        NAU_CLASS_FIELDS(
            CLASS_FIELD(uid),
            CLASS_FIELD(name),
            CLASS_FIELD(childCount),
            CLASS_FIELD(additionalComponentCount),
            CLASS_FIELD(rootComponent))
    };

    /**
     * @brief provides an interface for visiting scene objects and their components.
     * 
     * @note visitSceneObject and visitSceneComponent methods are to be overriden in implementations.
     */
    struct NAU_ABSTRACT_TYPE ISceneAssetVisitor
    {
        /**
         * @brief Destructor.
         */
        virtual ~ISceneAssetVisitor() = default;

        /**
            @brief Provide information about scene object
            @param parentObjectUid
                Specifies the parent object identifier. The system will guarantee that the object with this uid is visited early.
                If parentObjectUid is NullUid, then visitSceneObject called for topmost scene object (i.e. parent is scene root)
                If parentObjectUid is SceneObjectAsset::SceneVirtualRootUid then visited object is scene root itself
         */
        virtual bool visitSceneObject(Uid parentObjectUid, const SceneObjectAsset& childObject) = 0;

        /**
            @brief Provide information about object's component.
            @param parentObjectUid
                Specifies the components's owner object identifier. The system will guarantee that the object with this uid is visited early.
                If parentObjectUid is NullUid, then visitSceneObject then visited object is scene root.
         */
        virtual bool visitSceneComponent(Uid parentObjectUid, const ComponentAsset& component) = 0;
    };

    /**
     * @brief Provides an interface for a scene asset.
     */
    struct NAU_ABSTRACT_TYPE SceneAsset : IAssetView
    {
        NAU_INTERFACE(nau::SceneAsset, IAssetView)

        using Ptr = nau::Ptr<SceneAsset>;

        virtual SceneAssetInfo getSceneInfo() const = 0;

        virtual std::optional<Vector<ReferenceField>> getReferencesInfo() const = 0;

        /**
         * @brief Visits each scene object and their components recursively.
         * 
         * @param [in] visitor  Object visitor providing callback for each object and component visit.
         */
        virtual void visitScene(ISceneAssetVisitor& visitor) const = 0;
    };
}  // namespace nau
