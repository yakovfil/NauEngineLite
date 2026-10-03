// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <optional>

#include "nau/math/transform.h"
#include "nau/meta/class_info.h"
#include "nau/scene/components/component.h"
#include "nau/scene/components/component_attributes.h"
#include "nau/scene/components/internal/component_internal_attributes.h"
#include "nau/scene/transform_control.h"
#include "nau/utils/intrusive_list.h"

namespace nau::scene_internal
{
    /**
     */
    struct TransformListNode : nau::IntrusiveListNode
    {
    };
}  // namespace nau::scene_internal

namespace nau::scene
{

    /**
     */
    class NAU_CORESCENE_EXPORT SceneComponent : public Component,
                                                public virtual TransformControl,
                                                private scene_internal::TransformListNode
    {
        NAU_OBJECT(nau::scene::SceneComponent, Component, TransformControl)
        NAU_DECLARE_DYNAMIC_OBJECT

        NAU_CLASS_ATTRIBUTES(
            CLASS_ATTRIBUTE(SystemComponentAttrib, true),
            CLASS_ATTRIBUTE(ComponentDisplayNameAttrib, "Scene Component"),
            CLASS_ATTRIBUTE(ComponentDescriptionAttrib, "Scene Component (description)"))

        NAU_CLASS_FIELDS(
            CLASS_NAMED_FIELD(m_transform, "transform"))

    public:
        SceneComponent() = default;
        ~SceneComponent() override;

        SceneComponent(const SceneComponent&) = delete;
        SceneComponent& operator=(const SceneComponent&) = delete;

        const math::Transform& getWorldTransform() const final;
        void setWorldTransform(const math::Transform& transform) final;

        const math::Transform& getTransform() const final;
        void setTransform(const math::Transform& transform) final;

        /**
            @brief  Setting the rotation quaternion of the component relative to its parent
        */
        void setRotation(math::quat rotation) final;

        /**
            @brief  Setting the position of the component relative to its parent
        */
        void setTranslation(math::vec3 position) final;

        /**
            @brief  Setting the scale of the component relative to its parent
        */
        void setScale(math::vec3 scale) final;

        math::quat getRotation() const final;
        math::vec3 getTranslation() const final;
        math::vec3 getScale() const final;

    private:
        void appendTransformChild(SceneComponent& child);
        void removeTransformChild(SceneComponent& child);

    protected:
        virtual void notifyTransformChanged();
        virtual void notifyTransformChanged(const math::Transform& worldTransformCache);

    protected:
        math::Transform m_transform;
        mutable std::optional<math::Transform> m_worldTransformCache;

    private:
        SceneComponent* m_transformParent = nullptr;
        nau::IntrusiveList<scene_internal::TransformListNode> m_transformChildren;

        friend class SceneObject;
    };

}  // namespace nau::scene
