// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/scene/components/camera_component.h"

#include <string_view>

#include "nau/scene/scene_object.h"
#include "nau/scene/world.h"

namespace nau::scene
{
    NAU_IMPLEMENT_DYNAMIC_OBJECT(CameraComponent)

    Uid CameraComponent::getCameraUid() const
    {
        return SceneComponent::getUid();
    }

    Uid CameraComponent::getWorldUid() const
    {
        const IScene* const scene = getParentObject().getScene();
        const IWorld* const world = scene ? scene->getWorld() : nullptr;

        return world ? world->getUid() : NullUid;
    }

    std::string_view CameraComponent::getCameraName() const
    {
        return m_cameraName.empty() ? getParentObject().getName() : m_cameraName;
    }

    void CameraComponent::setCameraName(std::string_view name)
    {
        value_changes_scope;
        m_cameraName = name;
    }

    float CameraComponent::getFov() const
    {
        return m_fieldOfView;
    }

    float CameraComponent::getClipNearPlane() const
    {
        return m_clipNearPlane;
    }

    float CameraComponent::getClipFarPlane() const
    {
        return m_clipFarPlane;
    }

    void CameraComponent::setFov(float fov)
    {
        value_changes_scope;
        m_fieldOfView = fov;
    }

    void CameraComponent::setClipNearPlane(float clipNearPlane)
    {
        value_changes_scope;
        m_clipNearPlane = clipNearPlane;
    }

    void CameraComponent::setClipFarPlane(float clipFarPlane)
    {
        value_changes_scope;
        m_clipFarPlane = clipFarPlane;
    }

}  // namespace nau::scene
