// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include "frustumClusters.h"

#include <array>

#include "dag_occlusionTest.h"
#include "lights_common.h"

#define SHRINK_SPHERE 1
#define VALIDATE_CLUSTERS 0
namespace nau::render
{
    using namespace math;

    static constexpr int get_target_mip(int srcW, int destW)
    {
        return srcW <= destW ? 0 : 1 + get_target_mip(srcW / 2, destW);
    }
    bool get_max_occlusion_depth(Vector4* destDepth)  // from occlusion
    {
        if (nau::math::OCCLUSION_W / nau::math::OCCLUSION_H != CLUSTERS_W / CLUSTERS_H)
        {
            return false;
        }
        constexpr int occlusionMip = get_target_mip(nau::math::OCCLUSION_W, CLUSTERS_W);
        NAU_STATIC_ASSERT((nau::math::OCCLUSION_H >> occlusionMip) == CLUSTERS_H);
        Vector4* occlusionZ = reinterpret_cast<Vector4*>(OcclusionTest<nau::math::OCCLUSION_W, nau::math::OCCLUSION_H>::getZbuffer(occlusionMip));
        for (int i = 0; i < CLUSTERS_W * CLUSTERS_H / 4; ++i, destDepth++, occlusionZ++)
        {
            *destDepth = occlusion_convert_from_internal_zbuffer(*occlusionZ);
        }
        return true;
    }

    static __forceinline Vector3 v_dist3_sq_x(Vector3 a, Vector3 b)
    {
        return Vector3(float(lengthSqr(a - b)));
    }

    NAU_FORCE_INLINE void v_unsafe_two_plane_intersection(Vector4 p1, Vector4 p2, Vector3& point, Vector3& dir)
    {
        dir = cross(p1.getXYZ(), p2.getXYZ());  // p3_normal
        // calculate the final (point, normal)
        point = Vector3((cross(dir, p2.getXYZ()) * Vector3(p1.getW())) + (cross(p1.getXYZ(), dir) * Vector3(p2.getW())));
        // vec4f det = v_dot3(dir, dir);
        // for safe check if abs(det) is bigger then eps
        point = (point / float(dot(dir, dir)));
    }

    NAU_FORCE_INLINE Vector3 v_unsafe_ray_intersect_plane(Vector3 point, Vector3 dir, Vector4 P)
    {
        Vector3 t = ((-(Vector3(P.getW())) - Vector3(float(dot(point, P.getXYZ())))) / dot(dir, P.getXYZ()));
        // for safe check if abs(v_dot3(dir, P)) is bigger than eps
        return (t * dir + point);
    }

    NAU_FORCE_INLINE Vector4 v_make_plane_norm(Vector3 p0, Vector3 norm)
    {
        Vector3 d = -(Vector3(dot(norm, p0)));
        return v_perm_xyzd(Vector4(norm), Vector4(d));
    }

    NAU_FORCE_INLINE Vector4 v_make_plane_dir(Vector3 p0, Vector3 dir0, Vector3 dir1)
    {
        Vector3 n = cross(dir0, dir1);
        return v_make_plane_norm(p0, n);
    }

    void FrustumClusters::prepareFrustum(const Matrix4& view_, const Matrix4& proj_, float zn, float minDist, float maxDist,
                                         bool use_occlusion)  // from occlusion
    {
        depthSliceScale = CLUSTERS_D / log2f(maxDist / minDist);
        depthSliceBias = -log2f(minDist) * depthSliceScale;

        minSliceDist = minDist;
        maxSliceDist = maxDist;
        znear = zn;
        view = view_;
        proj = proj_;

        // constant for fixed min/max dists
        for (int z = 0; z <= CLUSTERS_D; ++z)
            sliceDists[z] = getDepthAtSlice(z, depthSliceScale, depthSliceBias);

        {
            Vector4 destDepthW[(CLUSTERS_W * CLUSTERS_H + 3) / 4];
            if (use_occlusion && get_max_occlusion_depth(destDepthW))
            {
                Vector4* sliceDepth = destDepthW;
                uint32_t* slicesNo = (uint32_t*)maxSlicesNo.data();
                Vector4 depthSliceScaleBias = Vector4(depthSliceScale, depthSliceBias, 255, 0);
                for (int y = 0; y < CLUSTERS_H; ++y)
                {
                    Vector4 maxRowSlice = Vector4(depthSliceScaleBias.getZ());
                    for (int x = 0; x < CLUSTERS_W / 4; ++x, sliceDepth++, slicesNo++)
                    {
                        *slicesNo = getVecMaxSliceAtDepth(*sliceDepth, depthSliceScaleBias, maxRowSlice);
                    }
                    maxRowSlice = min(maxRowSlice, v_rot_2(maxRowSlice));
                    sliceNoRowMax[y] = GetX(FromFloatTrunc(Vector4(ceil(min(maxRowSlice, v_rot_1(maxRowSlice))))));
                }
                //
            }
            else
            {
                mem_set_ff(maxSlicesNo);
                mem_set_ff(sliceNoRowMax);
            }
        }
        //-constant for fixed min/max dists

        // constant for fixed projection
        Vector4 viewClip(((Matrix4&)proj)[0][0], -((Matrix4&)proj)[1][1], ((Matrix4&)proj)[2][0], ((Matrix4&)proj)[2][1]);
        for (int x = 0; x <= CLUSTERS_W; ++x)
        {
            static const float tileScaleX = 0.5f * CLUSTERS_W;
            float tileBiasX = tileScaleX - x;
            x_planes[x] = normalize(Point3(viewClip.getX() * tileScaleX, 0.0f, viewClip.getZ() * tileScaleX + tileBiasX));
            x_planes[x].setW(0);
            x_planes2[x] = Point2(x_planes[x].getX(), x_planes[x].getZ());
        }
        for (int y = 0; y <= CLUSTERS_H; ++y)
        {
            static const float tileScaleY = 0.5f * CLUSTERS_H;
            float tileBiasY = tileScaleY - y;
            y_planes[y] = normalize(Point3(0, viewClip.getY() * tileScaleY, viewClip.getW() * tileScaleY + tileBiasY));
            y_planes[y].setW(0);
            y_planes2[y] = Point2(y_planes[y].getY(), y_planes[y].getZ());
        }
        {
            // TIME_PROFILE(allPoints);
            Vector4* points = (Vector4*)&frustumPoints[0];
            for (int z = 0; z <= CLUSTERS_D; ++z)
            {
                Vector4 z_plane = Vector4(0, 0, 1, z == 0 ? -zn : -sliceDists.data()[z]);
                for (int y = 0; y <= CLUSTERS_H; ++y)
                {
                    Vector3 point, dir;
                    v_unsafe_two_plane_intersection(z_plane, Vector4(_mm_load_ps((float*)&y_planes.data()[y])), point, dir);
                    for (int x = 0; x <= CLUSTERS_W; ++x, points++)
                    {
                        points->setXYZ(v_unsafe_ray_intersect_plane(point, dir, Vector4(_mm_load_ps((float*)&x_planes.data()[x]))));
                    }
                }
            }
        }
#if HAS_FROXEL_SPHERES
        Vector4* spheres = (Vector4*)&frustumSpheres[0].x;
        for (int z = 0; z < CLUSTERS_D; ++z)
        {
            for (int y = 0; y < CLUSTERS_H; ++y)
            {
                Vector4* points = (Vector4*)&frustumPoints[z * (CLUSTERS_W + 1) * (CLUSTERS_H + 1) + y * (CLUSTERS_W + 1)].x;
                for (int x = 0; x < CLUSTERS_W; ++x, spheres++, points++)
                {
                    bbox3f box;
                    v_bbox3_init(box, points[0]);
                    v_bbox3_add_pt(box, points[1]);
                    v_bbox3_add_pt(box, points[(CLUSTERS_W + 1)]);
                    v_bbox3_add_pt(box, points[(CLUSTERS_W + 1) + 1]);
                    v_bbox3_add_pt(box, points[(CLUSTERS_W + 1) * (CLUSTERS_H + 1) + 0]);
                    v_bbox3_add_pt(box, points[(CLUSTERS_W + 1) * (CLUSTERS_H + 1) + 1]);
                    v_bbox3_add_pt(box, points[(CLUSTERS_W + 1) * (CLUSTERS_H + 1) + (CLUSTERS_W + 1)]);
                    v_bbox3_add_pt(box, points[(CLUSTERS_W + 1) * (CLUSTERS_H + 1) + (CLUSTERS_W + 1) + 1]);
                    Vector3 center = v_bbox3_center(box);
                    Vector3 dist2 = v_dist3_sq_x(points[0], center);
                    dist2 = v_max(dist2, v_dist3_sq_x(points[1], center));
                    dist2 = v_max(dist2, v_dist3_sq_x(points[(CLUSTERS_W + 1)], center));
                    dist2 = v_max(dist2, v_dist3_sq_x(points[(CLUSTERS_W + 1) + 1], center));
                    dist2 = v_max(dist2, v_dist3_sq_x(points[(CLUSTERS_W + 1) * (CLUSTERS_H + 1) + 0], center));
                    dist2 = v_max(dist2, v_dist3_sq_x(points[(CLUSTERS_W + 1) * (CLUSTERS_H + 1) + 1], center));
                    dist2 = v_max(dist2, v_dist3_sq_x(points[(CLUSTERS_W + 1) * (CLUSTERS_H + 1) + (CLUSTERS_W + 1)], center));
                    dist2 = v_max(dist2, v_dist3_sq_x(points[(CLUSTERS_W + 1) * (CLUSTERS_H + 1) + (CLUSTERS_W + 1) + 1], center));
                    *spheres = v_perm_xyzd(center, v_splat_x(v_sqrt_x(dist2)));
                }
            }
        }
#endif
        //-constant for fixed projection
    }

    uint32_t FrustumClusters::getSpheresClipSpaceRects(const Vector4* pos_radius,
                                                       int aligned_stride,
                                                       int count,
                                                       dag::RelocatableFixedVector<ItemRect3D, ClusterGridItemMasks::MAX_ITEM_COUNT>& rects3d,
                                                       dag::RelocatableFixedVector<Vector4, ClusterGridItemMasks::MAX_ITEM_COUNT>& spheresViewSpace)
    {
        float clusterW = CLUSTERS_W, clusterH = CLUSTERS_H;
        for (int i = 0; i < count; ++i, pos_radius += aligned_stride)
        {
            Vector4 wpos = *pos_radius;
            Vector4 vpos = view * wpos;
            Vector4 lightViewSpace = v_perm_xyzd(vpos, wpos);

            FrustumScreenRect rect =
                findScreenSpaceBounds(((Matrix4&)proj)[0][0], ((Matrix4&)proj)[1][1], lightViewSpace, clusterW, clusterH, znear);
            if (rect.min_x > rect.max_x || rect.min_y > rect.max_y)
            {
                continue;
            }
            float zMinW = lightViewSpace.getZ() - lightViewSpace.getW(), zMaxW = lightViewSpace.getZ() + lightViewSpace.getW();
            if (zMinW >= maxSliceDist)  // todo: move to frustum culling
            {
                continue;
            }

            uint32_t zMin = getSliceAtDepth(max(zMinW, 1e-6f), depthSliceScale, depthSliceBias);
            uint32_t zMax = getSliceAtDepth(max(zMaxW, 1e-6f), depthSliceScale, depthSliceBias);
            rects3d.push_back(ItemRect3D(rect, min(zMin, uint32_t(CLUSTERS_D - 1)), min(zMax, uint32_t(CLUSTERS_D - 1)), i));
            spheresViewSpace.push_back(Vector4(_mm_load_ps((float*)&lightViewSpace)));
        }
        return rects3d.size();
    }

    uint32_t FrustumClusters::fillItemsSpheresGrid(ClusterGridItemMasks& items,
                                                   const dag::RelocatableFixedVector<Vector4, ClusterGridItemMasks::MAX_ITEM_COUNT>& lightsViewSpace,
                                                   uint32_t* result_mask,
                                                   uint32_t word_count)
    {
        if (!items.rects3d.size())
            return 0;

        uint32_t currentMasksStart = 0, totalItemsCount = 0;
        const ItemRect3D* grid = items.rects3d.data();
        for (int i = 0; i < items.rects3d.size(); ++i, grid++)
        {
            items.sliceMasksStart[i] = currentMasksStart;

            const uint32_t itemId = items.rects3d[i].itemId;
            uint32_t* resultMasksUse = result_mask + (itemId >> 5);
            const uint32_t itemMask = 1 << (itemId & 31);

            // int x_center = (grid->rect.max_x+grid->rect.min_x)/2;
            int z0 = grid->zmin, z1 = grid->zmax;
            int y0 = grid->rect.min_y, y1 = grid->rect.max_y + 1;
            int x0 = grid->rect.min_x, x1 = grid->rect.max_x + 1;

#if !SHRINK_SPHERE
            const MaskType x_mask = ((MaskType(1) << MaskType(grid->rect.max_x)) | ((MaskType(1) << MaskType(grid->rect.max_x)) - 1)) &
                                    (~((MaskType(1) << MaskType(x0)) - 1));
            totalItemsCount += (z1 - z0 + 1) * (y1 - y0) * (x1 - x0);
            (void)lightsViewSpace;
#else
            const Vector4& lightViewSpace = *(lightsViewSpace.data() + i);
            Vector4 pt = proj * lightsViewSpace[i];
            float radiusSq = lightViewSpace.getW() * lightViewSpace.getW();
            int center_z = lightViewSpace.getZ() <= znear ? -1 : getSliceAtDepth(lightViewSpace.getZ(), depthSliceScale, depthSliceBias);
            int center_y = (abs(pt.getW()) > 0.001f)
                               ? floorf(CLUSTERS_H * (pt.getY() / pt.getW()) * -0.5f + 0.5)
                               : grid->rect.center_y;  //(grid->rect.max_y+grid->rect.min_y)/2;
#endif
            for (int z = z0; z <= z1; z++)
            {
#if SHRINK_SPHERE
                Vector4 z_light_pos = lightViewSpace;
                float z_lightRadiusSq = radiusSq;
                if (z != center_z)
                {
                    float planeSign = z < center_z ? 1.0f : -1.0f;
                    int zPlaneId = z < center_z ? z + 1 : z;
                    const float sliceDist = center_z < 0 ? znear : sliceDists[zPlaneId];
                    float zPlaneDist = planeSign * (lightViewSpace.getZ() - sliceDist);
                    z_light_pos.setZ(z_light_pos.getZ() - zPlaneDist * planeSign);
                    z_lightRadiusSq = max(0.f, radiusSq - zPlaneDist * zPlaneDist);
                    z_light_pos.setW(sqrtf(z_lightRadiusSq));
                    // NAU_CORE_DEBUG_LF("%d: z = %d lightViewSpace = %@ z_light_pos= %@ zPlaneDist = %@ plane = %f dist %f",
                    //   i, z, lightViewSpace, z_light_pos, zPlaneDist, planeSign, sliceDists[zPlaneId]);
                }
                Point2 z_light_pos2(z_light_pos.getY(), z_light_pos.getZ());
#endif
                uint8_t* sliceNoRow = maxSlicesNo.data() + y0 * CLUSTERS_W;
                for (int y = y0; y < y1; y++, currentMasksStart++, sliceNoRow += CLUSTERS_W)
                {
                    if (sliceNoRowMax[y] < z)
                    {
                        continue;
                    }
#if SHRINK_SPHERE
                    // if (shrinkSphere.get())
                    {
                        Vector4 y_light_pos = z_light_pos;
                        if (y != center_y)
                        {  // Use original in the middle, shrunken sphere otherwise
                            // project to plane
                            // y_light = project_to_plane(y_light, plane);
                            // const Point3 &plane = (y < center_y) ? y_planes.data()[y + 1] : -y_planes.data()[y];
                            // float yPlaneT = dot(plane, *(Point3*)&y_light_pos);
                            // y_light_pos += yPlaneT*plane.xyz;
                            // y_light_pos.y -= yPlaneT*plane.y; y_light_pos.z -= yPlaneT*plane.z;

                            const Point2& plane2 = (y < center_y) ? y_planes2.data()[y + 1] : y_planes2.data()[y] * (-1.f);
                            float yPlaneT = dot(plane2, z_light_pos2);
                            y_light_pos.setY(y_light_pos.getY() - yPlaneT * plane2.getX());
                            y_light_pos.setZ(y_light_pos.getZ() - yPlaneT * plane2.getY());
                            float y_lightRadiusSq = z_lightRadiusSq - yPlaneT * yPlaneT;
                            if (y_lightRadiusSq < 0)
                            {
                                // NAU_CORE_DEBUG_LF("z=%d y=%d: plane=%@, dist = %@ z_light = %@ skip",z,y, plane, yPlaneT,
                                //   z_light_pos);
                                items.sliceMasks.data()[currentMasksStart] = 0;
                                continue;
                            }
                            y_light_pos.setW(sqrtf(y_lightRadiusSq));
                            // NAU_CORE_DEBUG_LF("z=%d y=%d: plane=%@, dist = %@ z_light = %@ y_light = %@",z,y, plane, yPlaneT,
                            //   z_light_pos, y_light_pos);
                        }
                        Point2 y_light_pos2(y_light_pos.getX(), y_light_pos.getZ());
                        int x = x0;
                        do
                        {  // Scan from left until with hit the sphere
                            ++x;
                        } while (x < x1 && (sliceNoRow[x] < z || dot(x_planes2.data()[x], y_light_pos2) >= y_light_pos.getW()));
                        // while (x < x1 && dot(x_planes.data()[x], *(Point3*)&y_light_pos) >= y_light_pos.w);

                        int xs = x1;
                        do
                        {  // Scan from right until with hit the sphere
                            --xs;
                        } while (xs >= x && (sliceNoRow[x] < z || -dot(x_planes2.data()[xs], y_light_pos2) >= y_light_pos.getW()));
                        // while (xs >= x && -dot(x_planes.data()[xs], *(Point3*)&y_light_pos) >= y_light_pos.w);

                        --x;
                        uint32_t* resultMaskAt = resultMasksUse + (x + y * CLUSTERS_W + z * CLUSTERS_W * CLUSTERS_H) * word_count;

    #if NO_OCCLUSION
                        items.sliceMasks.data()[currentMasksStart] =
                            ((MaskType(1) << MaskType(xs)) | ((MaskType(1) << MaskType(xs)) - 1)) & (~((MaskType(1) << MaskType(x)) - 1));

                        totalItemsCount += xs - x + 1;

                        for (; x <= xs; x++, resultMaskAt += word_count)
                        {
                            *resultMaskAt |= itemMask;
                        }
    #else
                        uint32_t mask = 0, bit = 1 << x;
                        for (; x <= xs; x++, bit <<= 1, resultMaskAt += word_count)
                        {
                            if (z <= sliceNoRow[x])
                            {
                                totalItemsCount++;
                                *resultMaskAt |= itemMask;
                                mask |= bit;
                            }
                        }
                        items.sliceMasks.data()[currentMasksStart] = mask;
    #endif
                        // if (lightsSliceMasks[currentMasksStart]!=x_mask)
                        //   NAU_CORE_DEBUG_LF("save lights");
                        // continue;
                    }
#else
                    items.sliceMasks[currentMasksStart] = x_mask;
                    uint32_t* resultMaskAt = resultMasksUse + (x0 + y * CLUSTERS_W + z * CLUSTERS_W * CLUSTERS_H) * word_count;
                    for (int x = x0; x < x1; x++, resultMaskAt += word_count)
                    {
                        *resultMaskAt |= itemMask;
                    }
#endif
                }
            }
            // currentMasksStart += (grid->zmax-grid->zmin+1) * (grid->rect.max_y-grid->rect.min_y+1);
        }
        NAU_ASSERT(currentMasksStart <= items.sliceMasks.size());
        items.itemsListCount = totalItemsCount;
        return totalItemsCount;
    }

    uint32_t FrustumClusters::fillItemsSpheres(const Vector4* pos_radius, int aligned_stride, int count, ClusterGridItemMasks& items, uint32_t* result_mask, uint32_t word_count)
    {
        // TIME_PROFILE(fillItemsSphere);
        NAU_STATIC_ASSERT(sizeof(items.sliceMasks[0]) * 8 >= CLUSTERS_W);
        items.reset();
        if (!count)
        {
            return 0;
        }
        // TIME_PROFILE(fillRects)
        dag::RelocatableFixedVector<Vector4, ClusterGridItemMasks::MAX_ITEM_COUNT> lightsViewSpace;
        items.itemsListCount = 0;
        if (!getSpheresClipSpaceRects(pos_radius, aligned_stride, count, items.rects3d, lightsViewSpace))
        {
            return 0;
        }

        return fillItemsSpheresGrid(items, lightsViewSpace, result_mask, word_count);
    }

#define V4D(a) v_extract_x(a), v_extract_y(a), v_extract_z(a), v_extract_w(a)
    static inline int is_point_out(Vector3 pt, const Matrix4& plane03_XYZW)
    {
        Vector4 res03;
        res03 = (Vector4(pt.getX()) * plane03_XYZW.getCol0() + plane03_XYZW.getCol3());
        res03 = (Vector4(pt.getY()) * plane03_XYZW.getCol1() + res03);
        res03 = (Vector4(pt.getZ()) * plane03_XYZW.getCol2() + res03);
        int result = _mm_movemask_ps(res03.get128());
        return result;
    }

    static inline int are_points_out(Vector3* pt, const Matrix4& plane03_XYZW)
    {
        Vector4 res03, ret03;
        res03 = (Vector4(pt[0].getX()) * plane03_XYZW.getCol0() + plane03_XYZW.getCol3());
        res03 = (Vector4(pt[0].getY()) * plane03_XYZW.getCol1() + res03);
        res03 = (Vector4(pt[0].getZ()) * plane03_XYZW.getCol2() + res03);
        // NAU_CORE_DEBUG_LF("0=%f %f %f %f",V4D(res03));
        ret03 = res03;
        res03 = (Vector4(pt[1].getX()) * plane03_XYZW.getCol0() + plane03_XYZW.getCol3());
        res03 = (Vector4(pt[1].getY()) * plane03_XYZW.getCol1() + res03);
        res03 = (Vector4(pt[1].getZ()) * plane03_XYZW.getCol2() + res03);
        ret03 = andPerElem(res03, ret03);
        // NAU_CORE_DEBUG_LF("1=%f %f %f %f",V4D(res03));

        pt += CLUSTERS_W + 1;

        res03 = (Vector4(pt[0].getX()) * plane03_XYZW.getCol0() + plane03_XYZW.getCol3());
        res03 = (Vector4(pt[0].getY()) * plane03_XYZW.getCol1() + res03);
        res03 = (Vector4(pt[0].getZ()) * plane03_XYZW.getCol2() + res03);
        ret03 = andPerElem(res03, ret03);
        // NAU_CORE_DEBUG_LF("2=%f %f %f %f",V4D(res03));
        res03 = (Vector4(pt[1].getX()) * plane03_XYZW.getCol0() + plane03_XYZW.getCol3());
        res03 = (Vector4(pt[1].getY()) * plane03_XYZW.getCol1() + res03);
        res03 = (Vector4(pt[1].getZ()) * plane03_XYZW.getCol2() + res03);
        ret03 = andPerElem(res03, ret03);
        // NAU_CORE_DEBUG_LF("3=%f %f %f %f",V4D(res03));

        pt += (CLUSTERS_W + 1) * (CLUSTERS_H + 1) - (CLUSTERS_W + 1);

        res03 = (Vector4(pt[0].getX()) * plane03_XYZW.getCol0() + plane03_XYZW.getCol3());
        res03 = (Vector4(pt[0].getY()) * plane03_XYZW.getCol1() + res03);
        res03 = (Vector4(pt[0].getZ()) * plane03_XYZW.getCol2() + res03);
        ret03 = andPerElem(res03, ret03);
        // NAU_CORE_DEBUG_LF("4=%f %f %f %f",V4D(res03));
        res03 = (Vector4(pt[1].getX()) * plane03_XYZW.getCol0() + plane03_XYZW.getCol3());
        res03 = (Vector4(pt[1].getY()) * plane03_XYZW.getCol1() + res03);
        res03 = (Vector4(pt[1].getZ()) * plane03_XYZW.getCol2() + res03);
        ret03 = andPerElem(res03, ret03);
        // NAU_CORE_DEBUG_LF("5=%f %f %f %f",V4D(res03));

        pt += CLUSTERS_W + 1;

        res03 = (Vector4(pt[0].getX()) * plane03_XYZW.getCol0() + plane03_XYZW.getCol3());
        res03 = (Vector4(pt[0].getY()) * plane03_XYZW.getCol1() + res03);
        res03 = (Vector4(pt[0].getZ()) * plane03_XYZW.getCol2() + res03);
        ret03 = andPerElem(res03, ret03);
        // NAU_CORE_DEBUG_LF("6=%f %f %f %f",V4D(res03));
        res03 = (Vector4(pt[1].getX()) * plane03_XYZW.getCol0() + plane03_XYZW.getCol3());
        res03 = (Vector4(pt[1].getY()) * plane03_XYZW.getCol1() + res03);
        res03 = (Vector4(pt[1].getZ()) * plane03_XYZW.getCol2() + res03);
        ret03 = andPerElem(res03, ret03);
        // NAU_CORE_DEBUG_LF("7=%f %f %f %f",V4D(res03));

        int result = _mm_movemask_ps(ret03.get128());
        return result;
    }

    inline bool test_cone_vs_sphere(Vector4 pos_radius, Vector4 dir_cos_angle, Vector4 testSphere, const Vector3& sinAngle)
    {
        Vector3 V = (testSphere - pos_radius).getXYZ();
        Vector3 VlenSq = Vector3(float(dot(V, V)));                      //_x
        Vector3 V1len = Vector3(float(dot(V, dir_cos_angle.getXYZ())));  //_x
        /*float tanHalf = v_extract_w(dir_angle);
        float cosHalfAngle = 1./sqrtf(1 + tanHalf*tanHalf);
        Vector3  distanceClosestPoint = v_sub_x(v_mul_x(v_splats(cosHalfAngle), v_sqrt_x(v_sub_x(VlenSq, v_mul_x(V1len, V1len)))),
                                              v_mul_x(V1len, v_splats(sqrtf(1-cosHalfAngle*cosHalfAngle))));//_x*/

        float distanceClosestPoint =
            (((dir_cos_angle.getW()) * sqrt((VlenSq - (V1len * V1len)).getX())) - (V1len * sinAngle).getX());  //_x

        float sphereRad = (testSphere.getW());

        bool nangleCull = (sphereRad >= distanceClosestPoint);
        bool result = nangleCull;
        bool nfrontCull = (sphereRad + pos_radius.getW()) >= V1len.getX();
        result = result && nfrontCull;
        bool nbackCull = V1len.getX() >= -sphereRad;
        result = result && nbackCull;
        return result;
    }

    uint32_t FrustumClusters::cullSpot(ClusterGridItemMasks& items, int i, Vector4 pos_radius, Vector4 dir_angle, uint32_t* result_mask, uint32_t word_count)
    {
        NAU_ASSERT(HAS_FROXEL_SPHERES);
#if HAS_FROXEL_SPHERES
        const ItemRect3D* grid = &items.rects3d[i];

        const uint32_t itemId = items.rects3d[i].itemId;
        uint32_t* resultMasksUse = result_mask + (itemId >> 5);
        const uint32_t itemMask = 1 << (itemId & 31);

        int z0 = grid->zmin, z1 = grid->zmax;
        int y0 = grid->rect.min_y, y1 = grid->rect.max_y + 1;
        int x0 = grid->rect.min_x, x1 = grid->rect.max_x + 1;

        if (z0 == z1 && y1 - y0 == 1 && x1 - x0 == 1 && x0 != 0 && y0 != 0 && x1 != CLUSTERS_W && y1 != CLUSTERS_H && z0 != 0 &&
            z1 != CLUSTERS_D - 1)  // it is very small, inside one cluster and so can't culled
            return items.itemsListCount;

        float tanHalf = v_extract_w(dir_angle);
        float cosHalfAngle = 1.f / sqrtf(1.f + tanHalf * tanHalf);
        dir_angle = v_perm_xyzd(dir_angle, v_splats(cosHalfAngle));
        Vector3 sinAngle = v_splats(sqrtf(1 - cosHalfAngle * cosHalfAngle));

        uint32_t currentMasksStart = items.sliceMasksStart[i];
        // uint8_t *cPlanes = planeBits.data();
        for (int z = z0; z <= z1; z++)  //, cPlanes+=planeYStride)
        {
            for (int y = y0; y < y1; y++, currentMasksStart++)  //, cPlanes++)
            {
                const MaskType mask = items.sliceMasks[currentMasksStart];
                MaskType x_mask = (MaskType(1) << MaskType(x0));
                const Vector4* spheres = (const Vector4*)&frustumSpheres[x0 + y * CLUSTERS_W + z * CLUSTERS_W * CLUSTERS_H].x;
                uint32_t* resultMaskAt = resultMasksUse + (x0 + y * CLUSTERS_W + z * CLUSTERS_W * CLUSTERS_H) * word_count;
                for (int x = x0; x < x1; x++, x_mask <<= MaskType(1), spheres++, resultMaskAt += word_count)
                {
                    if (!(mask & x_mask))
                        continue;
                    if (!test_cone_vs_sphere(pos_radius, dir_angle, *spheres, sinAngle))
                    {
                        *resultMaskAt &= ~itemMask;
                        items.sliceMasks[currentMasksStart] &= ~x_mask;
                        items.itemsListCount--;
                    }
                }
            }
        }
#endif
        return items.itemsListCount;
    }

    uint32_t FrustumClusters::cullFrustum(ClusterGridItemMasks& items, int i, const Matrix4& plane03_XYZW, const Matrix4& plane47_XYZW, uint32_t* result_mask, uint32_t word_count)
    {
        const ItemRect3D* grid = &items.rects3d[i];

        const uint32_t itemId = items.rects3d[i].itemId;
        uint32_t* resultMasksUse = result_mask + (itemId >> 5);
        const uint32_t itemMask = 1 << (itemId & 31);

        int z0 = grid->zmin, z1 = grid->zmax;
        int y0 = grid->rect.min_y, y1 = grid->rect.max_y + 1;
        int x0 = grid->rect.min_x, x1 = grid->rect.max_x + 1;

        if (z0 == z1 && y1 - y0 == 1 && x1 - x0 == 1 && x0 != 0 && y0 != 0 && x1 != CLUSTERS_W && y1 != CLUSTERS_H && z0 != 0 &&
            z1 != CLUSTERS_D - 1)  // it is very small, inside one cluster and so can't culled
            return items.itemsListCount;

        std::array<uint8_t, (CLUSTERS_W + 1) * (CLUSTERS_H + 1) * (CLUSTERS_D + 1)> planeBits;
        const int ez1 = min(z1 + 1, CLUSTERS_D);
        const int ey1 = min(y1 + 1, CLUSTERS_H + 1);
        const int ex1 = min(x1 + 1, CLUSTERS_W + 1);
        const int planeZStride = (CLUSTERS_W + 1) * (CLUSTERS_H + 1);  // todo: replace with tight
        const int planeYStride = (CLUSTERS_W + 1);                     // todo: replace with tight
        // uint8_t *planes = planeBits.data();

        // todo: we should optimize it. we check too much of points, which are definetly visible, and a lot of points which won't be tested
        // at all
        //  each plane can directly solve planes equasion, so we can know valid line for each z,y without checking each point. it is always
        //  00011111000 case, we only need boundaries
        for (int z = z0; z <= ez1; z++)
        {
            uint8_t* planes = planeBits.data() + z * planeZStride + y0 * planeYStride + x0;
            Vector4* point =
                reinterpret_cast<Vector4*>(frustumPoints.data()) + z * (CLUSTERS_W + 1) * (CLUSTERS_H + 1) + y0 * (CLUSTERS_W + 1) + x0;
            for (int y = y0; y < ey1; y++, planes += planeYStride - (ex1 - x0), point += (CLUSTERS_W + 1) - (ex1 - x0))  //
                for (int x = x0; x < ex1; x++, point++, planes++)
                {
                    // debug_it = ((x == 1 || x==2) && (y==1 || y==2) && (z==0||z==1) && i==0);
                    *planes = is_point_out(point->getXYZ(), plane03_XYZW) | (is_point_out(point->getXYZ(), plane47_XYZW) << 4);
                    // if (debug_it)
                    //   NAU_CORE_DEBUG_LF("*planes = 0x%X %dx%dx%d: index= %d",*planes,x,y,z, planes-planeBits.data());
                }
        }
        // const int planeYStride = (ex1-x0);//todo: replace with tight
        // const int planeZStride = planeYStride*(ey1-y0);

        uint32_t currentMasksStart = items.sliceMasksStart[i];
        // uint8_t *cPlanes = planeBits.data();
        for (int z = z0; z <= z1; z++)  //, cPlanes+=planeYStride)
        {
            for (int y = y0; y < y1; y++, currentMasksStart++)  //, cPlanes++)
            {
                const MaskType mask = items.sliceMasks[currentMasksStart];
                MaskType x_mask = (MaskType(1) << MaskType(x0));
                uint8_t* cPlanes = planeBits.data() + z * planeZStride + y * planeYStride + x0;
                uint32_t* resultMaskAt = resultMasksUse + (x0 + y * CLUSTERS_W + z * CLUSTERS_W * CLUSTERS_H) * word_count;
                for (int x = x0; x < x1; x++, x_mask <<= MaskType(1), cPlanes++, resultMaskAt += word_count)
                {
                    if (!(mask & x_mask))
                        continue;
                    uint8_t planeOut = cPlanes[0] & cPlanes[1] & cPlanes[planeYStride] & cPlanes[planeYStride + 1];

                    planeOut &= cPlanes[planeZStride + 0] & cPlanes[planeZStride + 1] & cPlanes[planeZStride + planeYStride] &
                                cPlanes[planeZStride + planeYStride + 1];
// #define CHECK_OPTIMIZATION 1
#if CHECK_OPTIMIZATION
                    uint32_t index = z * (CLUSTERS_W + 1) * (CLUSTERS_H + 1) + y * (CLUSTERS_W + 1) + x;
                    Vector4* point = (Vector4*)frustumPoints.data() + index;
#endif
                    if (planeOut)
                    {
#if CHECK_OPTIMIZATION
                        if (!are_points_out(point, plane03_XYZW) && !are_points_out(point, plane47_XYZW))
                        {
                            NAU_CORE_DEBUG_LF("planeOut = %d=%d&%d&%d&%d & %d&%d&%d&%d index = %d + %d+%d", planeOut, cPlanes[0], cPlanes[1],
                                              cPlanes[planeYStride], cPlanes[planeYStride + 1], cPlanes[planeZStride], cPlanes[planeZStride + 1],
                                              cPlanes[planeZStride + planeYStride], cPlanes[planeZStride + planeYStride + 1], cPlanes - planeBits.data(), planeZStride,
                                              planeYStride);
                            // NAU_CORE_DEBUG_LF("i=%d pt =%dx%dx%d", i, x,y,z);
                            // if (z == z0)
                            //   NAU_CORE_DEBUG_LF("%dx%d pt = %d %f %f %f", x, y, pt, v_extract_x(*point), v_extract_y(*point), v_extract_z(*point));
                            // items.gridCount.data()[x+y*CLUSTERS_W + z*CLUSTERS_W*CLUSTERS_H]--;
                            // items.sliceMasks[currentMasksStart] &= ~x_mask;
                            // items.itemsListCount --;
                        }
#endif
                        *resultMaskAt &= ~itemMask;
                        items.sliceMasks[currentMasksStart] &= ~x_mask;
                        items.itemsListCount--;
                    }
                    else
                    {
#if CHECK_OPTIMIZATION
                        if (are_points_out(point, plane03_XYZW) || are_points_out(point, plane47_XYZW))
                            NAU_CORE_DEBUG_LF("false");
#endif
                    }
                }
            }
        }

        return items.itemsListCount;
    }

    uint32_t FrustumClusters::cullSpots(const Vector4* pos_radius, int pos_aligned_stride, const Vector4* dir_angle, int dir_aligned_stride, ClusterGridItemMasks& items, uint32_t* result_mask, uint32_t word_count)
    {
        if (!items.itemsListCount)
            return 0;
        const ItemRect3D* grid = items.rects3d.data();
        Vector4 v_c099 = Vector4(0.999f);
        for (int i = 0; i < items.rects3d.size(); ++i, grid++)
        {
            if (grid->zmin == grid->zmax && grid->rect.min_y == grid->rect.max_y && grid->rect.min_x == grid->rect.max_x && grid->zmin != 0 &&
                grid->rect.min_x != 0 && grid->rect.min_y != 0 && grid->rect.min_x != CLUSTERS_W - 1 &&
                grid->rect.min_y != CLUSTERS_H - 1)  // it is very small, inside one cluster and so can't culled
                return items.itemsListCount;

            uint32_t id = grid->itemId;
            Vector4 wpos = pos_radius[id * pos_aligned_stride], wdir = dir_angle[id * dir_aligned_stride];
            // float cosHalfAngle = 1./sqrtf(1 + dir_angle[id].w*dir_angle[id].w);
            float tanHalf = wdir.getW();
            Vector4 vpos = (view * wpos);
            Vector4 vdir = (view * wdir);

// cullSpot(items, i, v_perm_xyzd(vpos, wpos), v_perm_xyzd(vdir, tanHalf));// a bit faster
// continue;

// Vector3 vposDist = v_mul(vdir, v_splat_w(pos_radius));
// Point3 up0 = fabsf(tangentZ.z) < 0.999 ? Point3(0,0,1) : Point3(1,0,0);
#define TWO_MORE_PLANES 1
            Vector4 up0 = (abs(vdir.getZ()) > 0.999f) ? Vector4(0, 0, 1, 0) : Vector4(1, 0, 0, 0);
            Vector3 left = normalize(cross(up0.getXYZ(), vdir.getXYZ())), up = cross(vdir.getXYZ(), left);
            Vector3 vFar2 = (tanHalf * Vector4(_mm_add_ps(left.get128(), up.get128()))).getXYZ();
            Vector3 vFar1 = (tanHalf * (left - up));
            Vector3 vFar3 = -(vFar1);
            Vector3 vFar0 = -(vFar2);
#if TWO_MORE_PLANES
            Vector3 leftrot = (vFar2 * Vector3(0.7071067811865476f)), uprot = (vFar1 * Vector3(0.7071067811865476f));
#endif
            vFar0 = (vFar0 + vdir.getXYZ());
            vFar1 = (vFar1 + vdir.getXYZ());
            vFar2 = (vFar2 + vdir.getXYZ());
            vFar3 = (vFar3 + vdir.getXYZ());

            Matrix4 plane03;
            plane03.setCol0(v_make_plane_dir(vpos.getXYZ(), vFar0, vFar1));
            plane03.setCol1(v_make_plane_dir(vpos.getXYZ(), vFar1, vFar2));
            plane03.setCol2(v_make_plane_dir(vpos.getXYZ(), vFar2, vFar3));
            plane03.setCol3(v_make_plane_dir(vpos.getXYZ(), vFar3, vFar0));
            plane03 = transpose(plane03);

            Vector4 planeNear = v_perm_xyzd(vdir, Vector4(-(dot(vdir.getXYZ(), vpos.getXYZ())))),
                    planeFar = v_perm_xyzd(-(vdir), Vector4((wpos - planeNear).getW()));
            Matrix4 plane47;
            plane47.setCol0(planeNear);
            plane47.setCol1(planeFar);

#if TWO_MORE_PLANES
            vFar2 = (leftrot + uprot);
            vFar1 = (leftrot - uprot);
            vFar0 = -(vFar2);  // vFar3 = v_neg(vFar1);
            vFar0 = (vFar0 + vdir.getXYZ());
            vFar1 = (vFar1 + vdir.getXYZ());
            vFar2 = (vFar2 + vdir.getXYZ());  // vFar3 = v_add(vFar3, vdir);
            plane47.setCol2(v_make_plane_dir(vpos.getXYZ(), vFar1, vFar0));
            plane47.setCol3(v_make_plane_dir(vpos.getXYZ(), vFar2, vFar1));
            // we can add even more planes, but it will be at a cost of additional 4 planes..
#else
            plane47.getCol2() = planeNear;
            plane47.getCol3() = planeFar;
#endif
            plane47 = transpose(plane47);

            cullFrustum(items, i, plane03, plane47, result_mask, word_count);
            // v_mat44_transpose(plane03, plane03);
            // Vector3 tp = v_make_Vector4(-3.7626, 1.98745, 4.89138, 0);//v_add(vpos, v_mul(vdir, v_mul(V_C_HALF, v_splat_w(wpos))));
            // NAU_CORE_DEBUG_LF("dist %g %g %g %g | %g %g ", v_extract_x(v_plane_dist_x(plane03.getCol0(), tp)),
            //       v_extract_x(v_plane_dist_x(plane03.getCol1(), tp)), v_extract_x(v_plane_dist_x(plane03.getCol2(), tp)),
            //       v_extract_x(v_plane_dist_x(plane03.getCol3(), tp)),
            //       v_extract_x(v_plane_dist_x(planeNear, tp)), v_extract_x(v_plane_dist_x(planeFar, tp)));

            // static inline uint32_t
            // int x_center = (grid->rect.max_x+grid->rect.min_x)/2;
            // currentMasksStart += (grid->zmax-grid->zmin+1) * (grid->rect.max_y-grid->rect.min_y+1);
        }
        return items.itemsListCount;
    }
}  // namespace nau::render