// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "camera/internal_camera_properties.h"

#include <string_view>

namespace nau::scene
{
    Uid InternalCameraProperties::getCameraUid() const
    {
        return m_uid;
    }

    Uid InternalCameraProperties::getWorldUid() const
    {
        return m_worldUid;
    }

    std::string_view InternalCameraProperties::getCameraName() const
    {
        return m_name;
    }

    float InternalCameraProperties::getFov() const
    {
        return m_fieldOfView;
    }

    float InternalCameraProperties::getClipNearPlane() const
    {
        return m_clipNearPlane;
    }

    float InternalCameraProperties::getClipFarPlane() const
    {
        return m_clipFarPlane;
    }

    const math::Transform& InternalCameraProperties::getWorldTransform() const
    {
        return m_transform;
    }

    const math::Transform& InternalCameraProperties::getTransform() const
    {
        return m_transform;
    }

    math::quat InternalCameraProperties::getRotation() const
    {
        return m_transform.getRotation();
    }

    math::vec3 InternalCameraProperties::getTranslation() const
    {
        return m_transform.getTranslation();
    }

    math::vec3 InternalCameraProperties::getScale() const
    {
        return m_transform.getScale();
    }

}  // namespace nau::scene