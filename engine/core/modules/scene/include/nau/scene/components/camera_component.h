// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string>
#include <string_view>

#include "nau/assets/asset_descriptor.h"
#include "nau/scene/camera/camera.h"
#include "nau/scene/components/component_attributes.h"
#include "nau/scene/components/internal/component_internal_attributes.h"
#include "nau/scene/components/scene_component.h"

namespace nau::scene
{
    /**
     */
    class NAU_CORESCENE_EXPORT CameraComponent : public SceneComponent,
                                                 public CameraProperties,
                                                 public virtual ICameraControl
    {
        NAU_OBJECT(nau::scene::CameraComponent, SceneComponent, CameraProperties, ICameraControl)
        NAU_DECLARE_DYNAMIC_OBJECT
        NAU_CLASS_ATTRIBUTES(
            CLASS_ATTRIBUTE(SystemComponentAttrib, true),
            CLASS_ATTRIBUTE(ComponentDisplayNameAttrib, "Camera"),
            CLASS_ATTRIBUTE(ComponentDescriptionAttrib, "Camera (description)"))

        NAU_CLASS_FIELDS(
            CLASS_NAMED_FIELD(m_cameraName, "cameraName"))

    public:
        Uid getCameraUid() const final;
        Uid getWorldUid() const final;
        std::string_view getCameraName() const final;
        void setCameraName(std::string_view name) final;

        float getFov() const final;
        float getClipNearPlane() const final;
        float getClipFarPlane() const final;

        void setFov(float fov) final;
        void setClipNearPlane(float clipNearPlane) final;
        void setClipFarPlane(float clipFarPlane) final;

    private:
        std::string m_cameraName;
    };

}  // namespace nau::scene
