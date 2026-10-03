// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <string_view>

#include "camera/internal_camera_properties.h"
#include "nau/math/transform.h"
#include "nau/scene/camera/camera.h"
#include "nau/scene/nau_object.h"
#include "nau/threading/spin_lock.h"

namespace nau::scene
{
    /**
     */
    class DetachedCamera final : public InternalCameraProperties, public virtual ICameraControl
    {
        NAU_CLASS_(nau::scene::DetachedCamera, InternalCameraProperties, ICameraControl)


    public:
        DetachedCamera(const DetachedCamera&) = delete;
        DetachedCamera(Uid worldUid);

        void setCameraName(std::string_view name) override;

        void setWorldTransform(const math::Transform& transform) override;
        void setTransform(const math::Transform& transform) override;

        void setRotation(math::quat rotation) override;
        void setTranslation(math::vec3 position) override;
        void setScale(math::vec3 scale) override;

        void setFov(float fov) override;
        void setClipNearPlane(float clipNearPlane) override;
        void setClipFarPlane(float clipFarPlane) override;

    private:
        mutable threading::SpinLock m_mutex;

        friend class ReadonlyCamera;
    };

}  // namespace nau::scene