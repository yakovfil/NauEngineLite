// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "camera/detached_camera.h"

#include <string_view>

namespace nau::scene
{
    DetachedCamera::DetachedCamera(Uid worldUid) :
        InternalCameraProperties{}
    {
        m_uid = Uid::generate();
        m_worldUid = worldUid;
    }

    void DetachedCamera::setCameraName(std::string_view name)
    {
        lock_(m_mutex);
        m_name = name;
    }

    void DetachedCamera::setWorldTransform(const math::Transform& transform)
    {
        lock_(m_mutex);
        m_transform = transform;
    }

    void DetachedCamera::setFov(float fov)
    {
        lock_(m_mutex);
        m_fieldOfView = fov;
    }

    void DetachedCamera::setClipNearPlane(float clipNearPlane)
    {
        lock_(m_mutex);
        m_clipNearPlane = clipNearPlane;
    }

    void DetachedCamera::setClipFarPlane(float clipFarPlane)
    {
        lock_(m_mutex);
        m_clipFarPlane = clipFarPlane;
    }

    void DetachedCamera::setTransform(const math::Transform& transform)
    {
        lock_(m_mutex);
        m_transform = transform;
    }

    void DetachedCamera::setRotation(math::quat rotation)
    {
        lock_(m_mutex);
        m_transform.setRotation(rotation);
    }

    void DetachedCamera::setTranslation(math::vec3 position)
    {
        lock_(m_mutex);
        m_transform.setTranslation(position);
    }

    void DetachedCamera::setScale(math::vec3 scale)
    {
        lock_(m_mutex);
        NAU_LOG_WARNING("Really need to setting scale for the camera ?");
        m_transform.setScale(scale);
    }
}  // namespace nau::scene
