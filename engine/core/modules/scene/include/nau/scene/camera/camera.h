// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <string_view>

#include "nau/math/transform.h"
#include "nau/meta/class_info.h"
#include "nau/rtti/rtti_object.h"
#include "nau/scene/transform_control.h"
#include "nau/utils/uid.h"


namespace nau::scene
{
    /**
     */
    struct NAU_ABSTRACT_TYPE ICameraProperties : virtual TransformProperties,
                                                 virtual IRttiObject
    {
        NAU_INTERFACE(nau::scene::ICameraProperties, TransformProperties, IRttiObject)

        virtual Uid getCameraUid() const = 0;
        virtual Uid getWorldUid() const = 0;
        virtual std::string_view getCameraName() const = 0;
        virtual float getFov() const = 0;
        virtual float getClipNearPlane() const = 0;
        virtual float getClipFarPlane() const = 0;
    };

    /**
     */
    struct NAU_ABSTRACT_TYPE ICameraControl : virtual ICameraProperties,
                                              virtual TransformControl
    {
        NAU_INTERFACE(nau::scene::ICameraControl, ICameraProperties, TransformControl)

        virtual void setCameraName(std::string_view cameraName) = 0;
        virtual void setFov(float fov) = 0;
        virtual void setClipNearPlane(float clipNearPlane) = 0;
        virtual void setClipFarPlane(float clipFarPlane) = 0;
    };

    /**
     */
    struct NAU_ABSTRACT_TYPE CameraProperties : public virtual ICameraProperties
    {
        NAU_INTERFACE(nau::scene::CameraProperties, ICameraProperties)

        NAU_CLASS_FIELDS(
            CLASS_NAMED_FIELD(m_fieldOfView, "FieldOfView"),
            CLASS_NAMED_FIELD(m_clipNearPlane, "ClipNearPlane"),
            CLASS_NAMED_FIELD(m_clipFarPlane, "ClipFarPlane"))

    protected:
        float m_fieldOfView = 90.f;
        float m_clipNearPlane = 0.1f;
        float m_clipFarPlane = 1000.f;
    };

}  // namespace nau::scene
