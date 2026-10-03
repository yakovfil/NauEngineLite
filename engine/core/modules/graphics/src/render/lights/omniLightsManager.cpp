// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include "nau/render/omniLightsManager.h"

#include <algorithm>
#include <bitset>
#include <memory>
#include <vector>

#include "lights_common.h"
#include "nau/debugRenderer/debug_render_system.h"

namespace nau::render
{
    OmniLightsManager::OmniLightsManager() :
        maxLightIndex(-1)
    {
        static_assert(1ULL << (sizeof(*freeLightIds.data()) * 8) >= MAX_LIGHTS);

        mem_set_0(rawLights);
        mem_set_0(freeLightIds);
        mem_set_0(lightPriority);

        freeLightIds.clear();
        /* TODO: Support photometry
        photometryTextures = IesTextureCollection::acquireRef();
        */
    }

    OmniLightsManager::~OmniLightsManager()
    {
        close();
    }

    void OmniLightsManager::close()
    {
        destroyAllLights();
        /* TODO: Support photometry
        if (photometryTextures)
        {
            IesTextureCollection::releaseRef();
            photometryTextures = nullptr;
        }
        */
    }

    void OmniLightsManager::prepare(const nau::math::NauFrustum& frustum,
                                    std::vector<uint16_t>& lights_with_camera_inside,
                                    std::vector<uint16_t>& lights_with_camera_outside,
                                    Occlusion* occlusion,
                                    nau::math::BBox3& inside_box,
                                    nau::math::BBox3& outside_box,
                                    const dag::RelocatableFixedVector<uint16_t, MAX_LIGHTS>& shadow,
                                    float markSmallLightsAsFarLimit,
                                    nau::math::Point3 cameraPos,
                                    mask_type_t accept_mask) const
    {
        prepare(frustum,
                lights_with_camera_inside,
                lights_with_camera_outside,
                occlusion,
                inside_box,
                outside_box,
                frustum.camPlanes[5],
                shadow,
                markSmallLightsAsFarLimit,
                cameraPos,
                accept_mask);
    }

    void OmniLightsManager::prepare(const nau::math::NauFrustum& frustum,
                                    std::vector<uint16_t>& lightsInside,
                                    std::vector<uint16_t>& lightsOutside,
                                    Occlusion* occlusion,
                                    nau::math::BBox3& inside_box,
                                    nau::math::BBox3& outside_box,
                                    nau::math::Vector4 znear_plane,
                                    const dag::RelocatableFixedVector<uint16_t, MAX_LIGHTS>& shadow,
                                    float markSmallLightsAsFarLimit,
                                    nau::math::Point3 cameraPos,
                                    mask_type_t accept_mask) const
    {
        prepare(frustum,
                lightsInside,
                lightsOutside,
                nullptr,
                occlusion,
                inside_box,
                outside_box,
                znear_plane,
                shadow,
                markSmallLightsAsFarLimit,
                cameraPos,
                accept_mask);
    }

    typedef uint16_t shadow_index_t;

    void OmniLightsManager::prepare(const nau::math::NauFrustum& frustum,
                                    std::vector<uint16_t>& lightsInside,
                                    std::vector<uint16_t>& lightsOutside,
                                    std::bitset<MAX_LIGHTS>* visibleIdBitset,
                                    Occlusion* occlusion,
                                    nau::math::BBox3& inside_box,
                                    nau::math::BBox3& outside_box,
                                    nau::math::Vector4 znear_plane,
                                    const dag::RelocatableFixedVector<uint16_t, MAX_LIGHTS>& shadows,
                                    float markSmallLightsAsFarLimit,
                                    nau::math::Point3 cameraPos,
                                    mask_type_t accept_mask) const
    {
        using namespace nau::math;
        int reserveSize = (maxLightIndex + 1) / 2;
        lightsInside.reserve(reserveSize);
        lightsOutside.reserve(reserveSize);
        Vector4 rad_scale = Vector4(1.1f);
        for (int i = 0; i <= maxLightIndex; ++i)
        {
            if (!(accept_mask & masks[i]))
            {
                continue;
            }
            const RawLight& l = rawLights[i];
            if (l.pos_radius.w <= 0)
            {
                continue;
            }
            nau::math::Vector4 lightPosRad = Vector4(l.pos_radius.toVec4());
            float rad = lightPosRad.getW();
            if (!frustum.testSphereB(lightPosRad.getXYZ(), Vector4(rad)))
            {
                //continue;
            }
            /* TODO Support occlusion
            if (occlusion && occlusion->isOccludedSphere(lightPosRad, rad))
                continue;
            */
            if (visibleIdBitset)
            {
                visibleIdBitset->set(i, true);
            }

            nau::math::Vector4 radScaled = rad_scale * rad;
            float res = lightPosRad.getX() * znear_plane.getX() - radScaled.getX() + znear_plane.getW();
            float length_sq = float(lengthSqr(cameraPos - math::Point3(lightPosRad.getXYZ())));
            float camInSphereVec = length_sq - rad * rad;

            bool intersectsNear = res < 0;
            bool camInSphere = camInSphereVec < 0;

            lightsOutside.push_back(i);//TODO: remove when the division of light sources into distant and close ones will be used

            bool smallLight = shadows[i] == shadow_index_t(~0) && is_viewed_small(lightPosRad.getW(), length_sq, markSmallLightsAsFarLimit);
            if ((intersectsNear || smallLight) && !camInSphere)
            {
                inside_box += lightPosRad.getXYZ() - Vector3(rad);
                inside_box += lightPosRad.getXYZ() + Vector3(rad);
                //lightsInside.push_back(i);
            }
            else
            {
                outside_box += lightPosRad.getXYZ() - Vector3(rad);
                outside_box += lightPosRad.getXYZ() + Vector3(rad);
                //lightsOutside.push_back(i);
            }
        }
    }

    void OmniLightsManager::drawDebugInfo()
    {
        for (int i = 0; i <= maxLightIndex; ++i)
        {
            const RawLight& l = rawLights[i];
            if (l.pos_radius.w <= 0)
            {
                continue;
            }
            nau::getDebugRenderer().drawSphere(l.pos_radius.w, l.color_atten,
                                               math::Matrix4::translation(f3Tov3(l.pos_radius.getXYZ())), 10, 0);
        }
    }

    void OmniLightsManager::renderDebugBboxes()
    {
        using nau::math::BBox3;
        using nau::math::Vector3;
        for (int i = 0; i <= maxLightIndex; ++i)
        {
            const RawLight& l = rawLights[i];
            if (l.pos_radius.w <= 0)
            {
                continue;
            }
            Vector3 center = f3Tov3(l.pos_radius.getXYZ());
            Vector3 radius = Vector3{l.pos_radius.w};
            BBox3 box{center - radius, center + radius};
            nau::getDebugRenderer().drawBoundingBox(box,
                                                    math::Matrix4::translation(f3Tov3(l.pos_radius.getXYZ())),
                                                    Color4(0, 255, 255, 255),
                                                    0);
        }
    }

    int OmniLightsManager::addLight(int priority, const RawLight& l)
    {
        int id = -1;
        if (freeLightIds.size())
        {
            id = freeLightIds.back();
            freeLightIds.pop_back();
        }
        else
        {
            if (maxLightIndex < (MAX_LIGHTS - 1))
            {
                id = ++maxLightIndex;
            }
            else
            {
                NAU_LOG_ERROR("Adding omnilight failed, already have %d lights in scene!", MAX_LIGHTS);
            }
        }
        if (id < 0)
        {
            return id;
        }
        rawLights[id] = l;
        masks[id] = ~mask_type_t(0);
        lightPriority[id] = priority;
        return id;
    }

    struct AscCompare
    {
        bool operator()(const uint16_t a, const uint16_t b) const
        {
            return a < b;
        }
    };

    void OmniLightsManager::removeEmpty()
    {
        std::ranges::sort(freeLightIds, AscCompare());
        for (int i = freeLightIds.size() - 1; i >= 0 && maxLightIndex == freeLightIds[i]; --i)
        {
            freeLightIds.pop_back();
            maxLightIndex--;
        }
    }

    void OmniLightsManager::destroyLight(unsigned int id)
    {
        NAU_ASSERT_RETURN(id <= maxLightIndex, );

        memset(&rawLights[id], 0, sizeof(rawLights[id]));
        masks[id] = 0;

        if (id == maxLightIndex)
        {
            maxLightIndex--;
            return;
        }

#if DAGOR_DBGLEVEL > 0
        for (int i = 0; i < freeLightIds.size(); ++i)
            if (freeLightIds[i] == id)
            {
                G_ASSERTF(freeLightIds[i] != id, "Light %d is already destroyed, re-destroy is invalid", id);
                return;
            }
#endif
        freeLightIds.push_back(id);
    }

    void OmniLightsManager::destroyAllLights()
    {
        maxLightIndex = -1;
        freeLightIds.clear();
    }
    /*
        IesTextureCollection::PhotometryData OmniLightsManager::getPhotometryData(int texId) const
        {
            return photometryTextures->getTextureData(texId);
        }

        int OmniLightsManager::addLight(int priority, const Point3& pos, const Point3& dir, const Color3& color, float radius, int tex, float attenuation_k)
        {
            IesTextureCollection::PhotometryData photometryData = getPhotometryData(tex);
            return addLight(priority, Light(pos, dir, color, radius, attenuation_k, tex, photometryData.zoom, photometryData.rotated));
        }

        int OmniLightsManager::addLight(int priority, const Point3& pos, const Point3& dir, const Color3& color, float radius, int tex, const TMatrix& box, float attenuation_k)
        {
            IesTextureCollection::PhotometryData photometryData = getPhotometryData(tex);
            return addLight(priority, Light(pos, dir, color, radius, attenuation_k, tex, photometryData.zoom, photometryData.rotated, box));
        }

        void OmniLightsManager::setLightTexture(unsigned int id, int tex)
        {
            IesTextureCollection::PhotometryData photometryData = getPhotometryData(tex);
            rawLights[id].setTexture(tex, photometryData.zoom, photometryData.rotated);
        }
        */
}  // namespace nau::render
