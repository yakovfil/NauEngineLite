// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string>

#include "nau/assets/asset_ref.h"
#include "nau/scene/components/component_attributes.h"
#include "nau/scene/components/component_life_cycle.h"
#include "nau/scene/components/internal/component_internal_attributes.h"
#include "nau/scene/components/scene_component.h"

namespace nau
{
    /**
     * @brief Provides opportunity for child objects to be attached to (i.e. move together with) a particular bone of the skeleton.
     */
    class NAU_ANIMATION_EXPORT SkeletonSocketComponent : public scene::SceneComponent,
                                                         public scene::IComponentUpdate
    {
        NAU_OBJECT(nau::SkeletonSocketComponent,
                   scene::SceneComponent,
                   scene::IComponentUpdate)

        NAU_CLASS_ATTRIBUTES(
            CLASS_ATTRIBUTE(scene::SystemComponentAttrib, true),
            CLASS_ATTRIBUTE(scene::ComponentDisplayNameAttrib, "Skeleton Socket"),
            CLASS_ATTRIBUTE(scene::ComponentDescriptionAttrib, "Skeleton Socket (description)"))

    public:
        virtual void updateComponent(float dt) override;

        /**
         * @brief Attaches the socket to a particular bone.
         *
         * @param [in] boneName Name of the bone to attach the socket to.
         */
        void setBoneName(const std::string& boneName);

        /**
         * @brief Retrieves the name of the bone the socket is attached to.
         *
         * @return Name of the bone the socket is attached to.
         */
        const std::string& getBoneName() const;

        /**
         * @brief Applies the socket transform relative to the bone it is attached to.
         *
         * @param [in] transform Relative transfrom to apply.
         */
        void setRelativeToBoneOffset(const math::Transform& transform);

        /**
         * @brief Retrieves the transform of the socket relative to the bone it is attached to.
         *
         * @return Socket relative transform.
         */
        const math::Transform& getRelativeToBoneOffset() const;

    private:
        std::string m_boneName;
        math::Transform m_relativeToBoneOffset;
    };
}  // namespace nau
