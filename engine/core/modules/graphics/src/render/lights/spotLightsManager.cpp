// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include "nau/render/spotLightsManager.h"

#include <algorithm>
#include <bitset>
#include <memory>
#include <vector>

#include "lights_common.h"
#include "nau/debugRenderer/debug_render_system.h"

namespace nau::render
{
    SpotLightsManager::SpotLightsManager() :
        maxLightIndex(-1)
    {
        static_assert(1ULL << (sizeof(*freeLightIds.data()) * 8) >= MAX_LIGHTS);
    }

    SpotLightsManager::~SpotLightsManager()
    {
        close();
    }

    void SpotLightsManager::init()
    {
        mem_set_0(rawLights);
        mem_set_0(freeLightIds);
        freeLightIds.clear();
        nonOptLightIds.reset();
        /* TODO: Support photometry
        photometryTextures = IesTextureCollection::acquireRef();
        */
    }

    void SpotLightsManager::close()
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

    typedef uint16_t shadow_index_t;

    template <bool use_small>
    void SpotLightsManager::prepare(const nau::math::NauFrustum& frustum,
                                    std::vector<uint16_t>& lights_inside_plane,
                                    std::vector<uint16_t>& lights_outside_plane,
                                    std::bitset<MAX_LIGHTS>* visibleIdBitset,
                                    Occlusion* occlusion,
                                    nau::math::BBox3& inside_box,
                                    nau::math::BBox3& outside_box,
                                    nau::math::Vector4 znear_plane,
                                    const dag::RelocatableFixedVector<shadow_index_t, SpotLightsManager::MAX_LIGHTS>& shadows,
                                    float markSmallLightsAsFarLimit,
                                    nau::math::Point3 cameraPos,
                                    mask_type_t accept_mask) const
    {
        using nau::math::Vector3;
        using nau::math::Vector4;
        inside_box.setempty();
        outside_box.setempty();
        const Vector4* boundings = (Vector4*)boundingSpheres.data();
        for (int i = 0; i <= maxLightIndex; ++i, boundings++)
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
            Vector4 lightPosRad = *boundings;
            float rad = lightPosRad.getW();
            if (!frustum.testSphereB(lightPosRad.getXYZ(), Vector4{rad}))  // completely not visible
            {
                continue;
            }
            // if (occlusion && occlusion->isOccludedSphere(lightPosRad, rad))
            //   continue;
            /* TODO Support occlusion
            if (occlusion && occlusion->isOccludedBox(boundingBoxes[i]))
                continue;
            */
            if (visibleIdBitset)
                visibleIdBitset->set(i, true);

            float res = lightPosRad.getX() * znear_plane.getX() - rad + znear_plane.getW();
            float length_sq = distSqr(cameraPos, math::Point3(lightPosRad.getXYZ()));
            float camInSphereVec = length_sq - rad * rad;

            bool intersectsNear = res < 0;
            bool camInSphere = camInSphereVec < 0;

            const bool smallLight = shadows[i] == shadow_index_t(~0) && is_viewed_small(lightPosRad.getX(), length_sq, markSmallLightsAsFarLimit);

            lights_outside_plane.push_back(i);//TODO: remove when the division of light sources into distant and close ones will be used

            if ((intersectsNear || smallLight) && !camInSphere)
            {
                inside_box += lightPosRad.getXYZ() - Vector3(rad);
                inside_box += lightPosRad.getXYZ() + Vector3(rad);
                //lights_inside_plane.push_back(i);
            }
            else
            {
                outside_box += lightPosRad.getXYZ() - Vector3(rad);
                outside_box += lightPosRad.getXYZ() + Vector3(rad);
                //lights_outside_plane.push_back(i);
            }
        }
    }

    int SpotLightsManager::addLight(const RawLight& light)
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
                NAU_LOG_ERROR("Adding spotlight failed, already have %d lights in scene!", MAX_LIGHTS);
            }
        }
        if (id < 0)
        {
            return id;
        }
        masks[id] = ~mask_type_t(0);
        rawLights[id] = light;
        resetLightOptimization(id);
        updateBoundingSphere(id);
        return id;
    }

    struct AscCompare
    {
        bool operator()(const uint16_t a, const uint16_t b) const
        {
            return a < b;
        }
    };

    void SpotLightsManager::removeEmpty()
    {
        std::ranges::sort(freeLightIds, AscCompare());
        for (int i = freeLightIds.size() - 1; i >= 0 && maxLightIndex == freeLightIds[i]; --i)
        {
            freeLightIds.pop_back();
            maxLightIndex--;
        }
    }

    void SpotLightsManager::renderDebugBboxes()
    {
        using nau::math::BBox3;
        using nau::math::Vector3;
        for (int i = 0; i <= maxLightIndex; ++i)
        {
            const RawLight& l = rawLights[i];
            if (l.pos_radius.w <= 0)
                continue;
            BBox3 box;
            box = boundingBoxes[i];
            nau::getDebugRenderer().drawBoundingBox(box,
                                                    math::Matrix4::translation(f3Tov3(l.pos_radius.getXYZ())),
                                                    Color4(0, 255, 255, 255),
                                                    0);
        }
    }

    void SpotLightsManager::destroyLight(unsigned int id)
    {
        NAU_ASSERT_RETURN(id <= maxLightIndex, );

        setLightOptimized(id);
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

    void SpotLightsManager::destroyAllLights()
    {
        maxLightIndex = -1;
        freeLightIds.clear();
        nonOptLightIds.reset();
    }
    namespace
    {
        inline void v_view_matrix_from_tangentZ(nau::math::Vector3& left, nau::math::Vector3& up, nau::math::Vector4 vdir)
        {
            up = vdir.getZ() > 0.999f ? nau::math::Vector3{1.0f, 0.0f, 0.0f} : nau::math::Vector3{0.0f, 0.0f, 1.0f};
            left = normalizeApprox(nau::math::cross(up, vdir.getXYZ()));
            up = nau::math::cross(vdir.getXYZ(), left);
        }

        inline void view_matrix_from_tangentZ(const nau::math::Vector3& tangentZ, nau::math::Matrix4& tm)
        {
            using nau::math::Vector3;
            using nau::math::Vector4;
            tm.setCol2(Vector4(tangentZ));
            Vector3 up = tangentZ.getZ() > 0.999f ? Vector3{1.0f, 0.0f, 0.0f} : Vector3{0.0f, 0.0f, 1.0f};
            tm.setCol0(Vector4(normalize(nau::math::cross(up, tangentZ))));
            tm.setCol1(Vector4(nau::math::cross(tangentZ, tm.getCol0().getXYZ())));
        }

        inline void v_mat44_make_persp_reverse(nau::math::Matrix4& dest, float wk, float hk, float zn, float zf)
        {
            dest.setCol0({wk, 0, 0, 0});
            dest.setCol1({0, hk, 0, 0});
            dest.setCol2({0, 0, zn / (zn - zf), 1.f});
            dest.setCol3({0, 0, (zn * zf) / (zf - zn), 0});
        }

    }  // namespace

    void SpotLightsManager::updateBoundingBox(unsigned id)
    {
        using nau::math::BBox3;
        using nau::math::Point3;
        using nau::math::Vector3;
        using nau::math::Vector4;
        const RawLight& l = rawLights[id];

        Vector3 left, up;
        Point3 pos = Point3(f3Tov3(l.pos_radius.getXYZ()));
        float radius = l.culling_radius == -1 ? l.pos_radius.w : l.culling_radius;
        Vector4 vrad = Vector4(radius);

        Vector4 vdir = l.dir_angle.toVec4();
        v_view_matrix_from_tangentZ(left, up, vdir);

        float tanHalf = vdir.getW();
        float sinHalfAngle = tanHalf / sqrt(1 + tanHalf * tanHalf);
        float mulR = sinHalfAngle * radius;

        static const bool buildOctahedron = true;
        if (buildOctahedron)
            mulR = mulR * 1.082392200292394f;  // we build octahedron, so we have to scale radius by R/r
        left = left * mulR;
        up = up * mulR;

        BBox3 box;
        box += left;

        if (buildOctahedron)
        {
            // v_bbox3_add_pt(box, left);//already inited
            box += up;
            box += -up;
            box += -left;
            box += -up;
            left = left * 0.7071067811865476f;
            up = up * 0.7071067811865476f;
        }
        Vector3 corner0 = left + up, corner1 = left - up;
        box += corner0;
        box += -corner0;
        ;
        box += corner1;
        box += -corner1;
        Vector3 farCenter = vdir.getXYZ() * radius;
        box[0] = box[0] + farCenter;
        box[1] = box[1] + farCenter;
        box += {0, 0, 0};

        boundingBoxes[id][0] = box[0] + Vector3(pos);
        boundingBoxes[id][1] = box[1] + Vector3(pos);
    }

    void SpotLightsManager::getLightView(unsigned int id, nau::math::Matrix4& viewITM)
    {
        nau::math::Matrix4 view;
        const Light& l = rawLights[id];
        view_matrix_from_tangentZ(l.dir_angle.toVec4().getXYZ(), view);
        view.setCol3(math::Vector4(l.pos_radius.toVec4().getXYZ()));
        viewITM = view;
    }

    /* TODO: Support photometry
    int SpotLightsManager::addLight(const Point3& pos, const Color3& color, const Point3& dir, const float angle, float radius, float attenuation_k, bool contact_shadows, int tex)
    {
        IesTextureCollection::PhotometryData photometryData = getPhotometryData(tex);
        return addLight(
            Light(pos, color, radius, attenuation_k, dir, angle, contact_shadows, tex, photometryData.zoom, photometryData.rotated));
    }

    IesTextureCollection::PhotometryData SpotLightsManager::getPhotometryData(int texId) const
    {
        return photometryTextures->getTextureData(texId);
    }
    */

    void SpotLightsManager::getLightPersp(unsigned int id, nau::math::Matrix4& proj)
    {
        const Light& l = rawLights[id];
        float zn = 0.001f * l.pos_radius.w;
        float zf = l.pos_radius.w;
        float wk = 1. / l.dir_angle.w;

        v_mat44_make_persp_reverse(proj, wk, wk, zn, zf);
    }

    template void SpotLightsManager::prepare<true>(const nau::math::NauFrustum& frustum,
                                                   std::vector<uint16_t>& lights_inside_plane,
                                                   std::vector<uint16_t>& lights_outside_plane,
                                                   std::bitset<MAX_LIGHTS>* visibleIdBitset,
                                                   Occlusion*,
                                                   nau::math::BBox3& inside_box,
                                                   nau::math::BBox3& outside_box,
                                                   nau::math::Vector4 znear_plane,
                                                   const dag::RelocatableFixedVector<uint16_t, SpotLightsManager::MAX_LIGHTS>& shadow,
                                                   float markSmallLightsAsFarLimit,
                                                   nau::math::Point3 cameraPos,
                                                   mask_type_t accept_mask) const;

    template void SpotLightsManager::prepare<false>(const nau::math::NauFrustum& frustum,
                                                    std::vector<uint16_t>& lights_inside_plane,
                                                    std::vector<uint16_t>& lights_outside_plane,
                                                    std::bitset<MAX_LIGHTS>* visibleIdBitset,
                                                    Occlusion*,
                                                    nau::math::BBox3& inside_box,
                                                    nau::math::BBox3& outside_box,
                                                    nau::math::Vector4 znear_plane,
                                                    const dag::RelocatableFixedVector<uint16_t, SpotLightsManager::MAX_LIGHTS>& shadow,
                                                    float markSmallLightsAsFarLimit,
                                                    nau::math::Point3 cameraPos,
                                                    mask_type_t accept_mask) const;
}  // namespace nau::render