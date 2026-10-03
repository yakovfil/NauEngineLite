// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <vector>

#include "nau/assets/asset_ref.h"
#include "nau/math/transform.h"
#include "nau/scene/components/scene_component.h"

namespace nau::physics
{
    /**
     */
    struct ColliderBase
    {
        NAU_CLASS_FIELDS(
            CLASS_FIELD(isTrigger),
            CLASS_FIELD(materialAsset),
            CLASS_FIELD(localTransform))

        bool isTrigger = false;
        AssetRef<> materialAsset;
        math::Transform localTransform;
    };

    /**
     */
    struct SphereCollider : ColliderBase
    {
        NAU_CLASS_BASE(ColliderBase)
        NAU_CLASS_FIELDS(
            CLASS_FIELD(radius))

        float radius = 1.0f;
    };

    /**
     */
    struct BoxCollider : ColliderBase
    {
        NAU_CLASS_BASE(ColliderBase)
        NAU_CLASS_FIELDS(
            CLASS_FIELD(extent))

        math::vec3 extent = math::vec3::one();
    };

    /**
     */
    struct CapsuleCollider : ColliderBase
    {
        NAU_CLASS_BASE(ColliderBase)
        NAU_CLASS_FIELDS(
            CLASS_FIELD(height),
            CLASS_FIELD(radius))

        float height = 1.0f;
        float radius = 0.5f;
    };

    /**
     */
    struct CylinderCollider : ColliderBase
    {
        NAU_CLASS_BASE(ColliderBase)
        NAU_CLASS_FIELDS(
            CLASS_FIELD(height),
            CLASS_FIELD(radius))

        float height = 1.0f;
        float radius = 0.5f;
    };

    /**
     */
    struct CollisionDescription
    {
        std::vector<SphereCollider> spheres;
        std::vector<BoxCollider> boxes;
        std::vector<CapsuleCollider> capsules;
        std::vector<CylinderCollider> cylinders;

        NAU_CLASS_FIELDS(
            CLASS_FIELD(spheres),
            CLASS_FIELD(boxes),
            CLASS_FIELD(capsules),
            CLASS_FIELD(cylinders),
            CLASS_FIELD(cylinders))

        decltype(auto) addSphere(float r)
        {
            return spheres.emplace_back(SphereCollider{.radius = r});
        }

        decltype(auto) addBox(math::vec3 extent)
        {
            return boxes.emplace_back(BoxCollider{.extent = extent});
        }

        decltype(auto) addCapsule(float height, float radius)
        {
            return capsules.emplace_back(CapsuleCollider{.height = height, .radius = radius});
        }

        decltype(auto) addCylinder(float height, float radius)
        {
            return cylinders.emplace_back(CylinderCollider{.height = height, .radius = radius});
        }
    };
}  // namespace nau::physics