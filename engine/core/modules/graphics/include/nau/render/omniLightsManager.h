// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <array>

#include "nau/math/dag_bounds3.h"
#include "nau/math/dag_frustum.h"
#include "nau/render/omniLight.h"

namespace nau::render
{
#include "nau/render/renderLights.hlsli"
}

#include <bitset>
#include <vector>

#include "nau/utils/dag_relocatableFixedVector.h"

/* TODO: Support photometry
#include <render/iesTextureManager.h>
*/

namespace nau::render
{
    class Occlusion;

    class OmniLightsManager
    {
    public:
        typedef OmniLight Light;
        typedef Light RawLight;
        typedef uint8_t mask_type_t;

        static constexpr int MAX_LIGHTS = 2048;
        enum
        {
            GI_LIGHT_MASK = 0x1,  // TODO: maybe rename it
            MASK_ALL = 0xFF
        };

        OmniLightsManager();
        ~OmniLightsManager();

        void close();

        void prepare(const nau::math::NauFrustum& frustum,
                     std::vector<uint16_t>& lights_inside_plane,
                     std::vector<uint16_t>& lights_outside_plane,
                     std::bitset<MAX_LIGHTS>* visibleIdBitset,
                     Occlusion*,
                     nau::math::BBox3& inside_box,
                     nau::math::BBox3& outside_box,
                     nau::math::Vector4 znear_plane,
                     const dag::RelocatableFixedVector<uint16_t, MAX_LIGHTS>& shadow,
                     float markSmallLightsAsFarLimit = 0,
                     nau::math::Point3 cameraPos = {},
                     mask_type_t accept_mask = MASK_ALL) const;

        void prepare(const nau::math::NauFrustum& frustum,
                     std::vector<uint16_t>& lights_inside_plane,
                     std::vector<uint16_t>& lights_outside_plane,
                     Occlusion*,
                     nau::math::BBox3& inside_box,
                     nau::math::BBox3& outside_box,
                     nau::math::Vector4 znear_plane,
                     const dag::RelocatableFixedVector<uint16_t, MAX_LIGHTS>& shadow,
                     float markSmallLightsAsFarLimit = 0,
                     nau::math::Point3 cameraPos = {},
                     mask_type_t accept_mask = MASK_ALL) const;

        void prepare(const nau::math::NauFrustum& frustum,
                     std::vector<uint16_t>& lights_with_camera_inside,
                     std::vector<uint16_t>& lights_with_camera_outside,
                     Occlusion*,
                     nau::math::BBox3& inside_box,
                     nau::math::BBox3& outside_box,
                     const dag::RelocatableFixedVector<uint16_t, MAX_LIGHTS>& shadow,
                     float markSmallLightsAsFarLimit = 0,
                     nau::math::Point3 cameraPos = {},
                     mask_type_t accept_mask = MASK_ALL) const;

        void drawDebugInfo();
        void renderDebugBboxes();

        /* TODO: Support photometry
        int addLight(int priority,
                     const nau::math::Point3& pos,
                     const nau::math::Vector3& dir,
                     const Color3& color,
                     float radius,
                     int tex,
                     float attenuation_k = 1.f);

        int addLight(int priority,
                     const nau::math::Point3& pos,
                     const nau::math::Vector3& dir,
                     const Color3& color,
                     float radius,
                     int tex,
                     const nau::math::Matrix4& box,
                     float attenuation_k = 1.f);
        */

        int addLight(int priority, const Light& l);

        void destroyLight(unsigned int id);
        void destroyAllLights();

        void setLightPos(unsigned int id, const nau::math::Point3& pos)
        {
            NAU_ASSERT(id < MAX_LIGHTS);
            if (isnan(pos.getX() + pos.getY() + pos.getZ()))
            {
                NAU_FAILURE("nan in setLightPos");
                return;
            }
            rawLights[id].pos_radius.x = pos.getX();
            rawLights[id].pos_radius.y = pos.getY();
            rawLights[id].pos_radius.z = pos.getZ();
        }
        void setLightCol(unsigned int id, const Color3& col)
        {
            NAU_ASSERT(id < MAX_LIGHTS);
            rawLights[id].color_atten.r = col.r;
            rawLights[id].color_atten.g = col.g;
            rawLights[id].color_atten.b = col.b;
        }
        void setLightPosAndCol(unsigned int id, const nau::math::Point3& pos, const Color3& color)
        {
            NAU_ASSERT(id < MAX_LIGHTS);
            setLightPos(id, pos);
            setLightCol(id, color);
        }
        void setLightRadius(unsigned int id, float radius)
        {
            NAU_ASSERT(id < MAX_LIGHTS);
            if (isnan(radius))
            {
                NAU_FAILURE("nan in setLightRadius");
                return;
            }
            rawLights[id].pos_radius.w = radius;
        }
        void setLightBox(unsigned int id, const nau::math::Matrix4& box)
        {
            NAU_ASSERT(id < MAX_LIGHTS);
            rawLights[id].setBox(box);
        }

        void setLightDirection(unsigned int id, const nau::math::Vector3& dir)
        {
            NAU_ASSERT(id < MAX_LIGHTS);
            rawLights[id].setDirection(float3(dir));
        }

        /* TODO: Support photometry
        void setLightTexture(unsigned int id, int tex);
         */

        const mask_type_t getLightMask(unsigned int id) const
        {
            NAU_ASSERT(id < MAX_LIGHTS);
            return masks[id];
        }
        void setLightMask(unsigned int id, mask_type_t mask)
        {
            NAU_ASSERT(id < MAX_LIGHTS);
            masks[id] = mask;
        }

        const Light& getLight(unsigned int id) const
        {
            NAU_ASSERT(id < MAX_LIGHTS);
            return rawLights[id];
        }
        void setLight(unsigned int id, const Light& l)
        {
            NAU_ASSERT(id < MAX_LIGHTS);
            if (isnan(l.pos_radius.x + l.pos_radius.y + l.pos_radius.z + l.pos_radius.w))
            {
                NAU_FAILURE("nan in setLight");
                return;
            }
            rawLights[id] = l;
        }
        void removeEmpty();
        int maxIndex() const
        {
            return maxLightIndex;
        }

        /* TODO: Support photometry
        IesTextureCollection::PhotometryData getPhotometryData(int texId) const;
        */

        RenderOmniLight getRenderLight(unsigned int id) const
        {
            NAU_ASSERT(id < MAX_LIGHTS);
            const Light& l = rawLights[id];
            RenderOmniLight ret;
            ret.posRadius = l.pos_radius;
            ret.colorFlags = float4(l.color_atten.vector4());
            ret.direction__tex_scale = l.dir__tex_scale;
            ret.boxR0 = l.boxR0;
            ret.boxR1 = l.boxR1;
            ret.boxR2 = l.boxR2;
            ret.posRelToOrigin_cullRadius = l.posRelToOrigin_cullRadius;
            return ret;
        }

        nau::math::Vector4 getBoundingSphere(unsigned id) const
        {
            NAU_ASSERT(id < MAX_LIGHTS);
            return rawLights[id].pos_radius.toVec4();
        }

    private:
        std::array<Light, MAX_LIGHTS> rawLights = {};
        std::array<uint8_t, MAX_LIGHTS> lightPriority = {};
        // masks allows to ignore specific lights in specific cases
        // for example, we can ignore highly dynamic lights for GI
        std::array<mask_type_t, MAX_LIGHTS> masks = {};  //-V730_NOINIT
        dag::RelocatableFixedVector<uint16_t, MAX_LIGHTS> freeLightIds = {};

        /* TODO: Support photometry
         IesTextureCollection* photometryTextures = nullptr;
        */
        int maxLightIndex;
    };
}  // namespace nau::render