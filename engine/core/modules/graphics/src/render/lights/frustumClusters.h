// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#pragma once

#include <corecrt_math.h>

#include <algorithm>
#include <array>
#include <cstdint>

#include "dag_vecMath_est.h"
#include "frustumClipRegion.h"
#include "nau/math/dag_frustum.h"
#include "nau/utils/dag_relocatableFixedVector.h"

namespace nau::render
{

#define CLUSTERS_W 32
#define CLUSTERS_H 16
#define CLUSTERS_D 24

    struct FrustumClusters
    {
        static inline float getDepthAtSlice(uint32_t slice, float depthSliceScale, float depthSliceBias)
        {
            // slice = log2f(depth) * depthSliceScale + depthSliceBias
            return powf(2.0f, slice / depthSliceScale - depthSliceBias / depthSliceScale);
            // return (powf( 2.0f, mExponentK * uSlice ) + minDist);
        }
        //-----------------------------------------------------------------------------------
        static inline float safe_log2f(float v)
        {
            return v > 1e-5f ? log2f(v) : -1000000.f;
        }
        static inline uint32_t getSliceAtDepth(float depth, float depthSliceScale, float depthSliceBias)
        {
            return std::max(int(floorf(safe_log2f(depth) * depthSliceScale + depthSliceBias)), int(0));
            // return uint32_t(floorf( log2f( max( depth - minDist, 1.f ) ) * mInvExponentK ) );
        }
        static inline uint32_t getMaxSliceAtDepth(float depth, float depthSliceScale, float depthSliceBias)
        {
            return std::max(int(ceilf(safe_log2f(depth) * depthSliceScale + depthSliceBias)), int(0));
            // return uint32_t(floorf( log2f( max( depth - minDist, 1.f ) ) * mInvExponentK ) );
        }
        static inline uint32_t getVecMaxSliceAtDepth(nau::math::Vector4 depth,
                                                     nau::math::Vector4 depthSliceScaleBias,
                                                     nau::math::Vector4& maxRowSlice)
        {
            using namespace nau::math;
            Vector4 maxSlices = Vector4(ceil((v_log2_est_p3(depth) * Vector4(depthSliceScaleBias.getX())) + Vector4(depthSliceScaleBias.getY())));
            maxSlices = max(maxSlices, Vector4(0));
            maxSlices = min(maxSlices, Vector4(255.f));
            maxRowSlice = max(maxSlices, maxRowSlice);
            IVector4 maxSlicesi = IVector4(FromFloatRound(Vector4(ceil(maxSlices))));
            return maxSlicesi.getElem(0) | (maxSlicesi.getElem(1) << 8) | (maxSlicesi.getElem(2) << 16) | (maxSlicesi.getElem(3) << 24);
        }

        std::array<float, CLUSTERS_D + 1> sliceDists;                                                    //-V730_NOINIT
        std::array<math::Point3, CLUSTERS_W + 1> x_planes;                                               //-V730_NOINIT
        std::array<math::Point3, CLUSTERS_H + 1> y_planes;                                               //-V730_NOINIT
        std::array<math::Point2, CLUSTERS_W + 1> x_planes2;                                              //-V730_NOINIT
        std::array<math::Point2, CLUSTERS_H + 1> y_planes2;                                              //-V730_NOINIT
        std::array<math::Point3, (CLUSTERS_W + 1) * (CLUSTERS_H + 1) * (CLUSTERS_D + 1)> frustumPoints;  //-V730_NOINIT
        std::array<uint8_t, CLUSTERS_H> sliceNoRowMax;                                                   //-V730_NOINIT
        std::array<uint8_t, CLUSTERS_W * CLUSTERS_H> maxSlicesNo;                                        //-V730_NOINIT

#define HAS_FROXEL_SPHERES 0
#if HAS_FROXEL_SPHERES
        std::array<Point3_vec4, (CLUSTERS_W) * (CLUSTERS_H) * (CLUSTERS_D)> frustumSpheres;  //-V730_NOINIT
#endif

        struct ItemRect3D
        {
            FrustumScreenRect rect;  //-V730_NOINIT
            uint8_t zmin, zmax;      //-V730_NOINIT
            uint16_t itemId;         //-V730_NOINIT
            ItemRect3D()
            {
            }
            ItemRect3D(const FrustumScreenRect& r, uint8_t zmn, uint8_t zmx, uint16_t l) :
                rect(r),
                zmin(zmn),
                zmax(zmx),
                itemId(l)
            {
            }
        };

        void prepareFrustum(const nau::math::Matrix4& view_,
                            const nau::math::Matrix4& proj_,
                            float zn,
                            float minDist,
                            float maxDist,
                            bool use_occlusion);

        typedef uint32_t MaskType;
        struct ClusterGridItemMasks
        {
            static constexpr int MAX_ITEM_COUNT = 256;                                    // can be replaced with dynamic
            std::array<MaskType, MAX_ITEM_COUNT * CLUSTERS_H * CLUSTERS_D> sliceMasks;    //-V730_NOINIT
            std::array<uint16_t, MAX_ITEM_COUNT> sliceMasksStart;                         //-V730_NOINIT

            dag::RelocatableFixedVector<ItemRect3D, MAX_ITEM_COUNT> rects3d;

            uint32_t itemsListCount = 0;
            void reset()
            {
                itemsListCount = 0;
                rects3d.clear();
            }
        };

        uint32_t getSpheresClipSpaceRects(const math::Vector4* pos_radius,
                                          int aligned_stride,
                                          int count,
                                          dag::RelocatableFixedVector<ItemRect3D, ClusterGridItemMasks::MAX_ITEM_COUNT>& rects3d,
                                          dag::RelocatableFixedVector<math::Vector4, ClusterGridItemMasks::MAX_ITEM_COUNT>& spheresViewSpace);

        uint32_t fillItemsSpheresGrid(ClusterGridItemMasks& items,
                                      const dag::RelocatableFixedVector<math::Vector4, ClusterGridItemMasks::MAX_ITEM_COUNT>& lightsViewSpace,
                                      uint32_t* result_mask,
                                      uint32_t word_count);

        uint32_t fillItemsSpheres(const math::Vector4* pos_radius,
                                  int aligned_stride,
                                  int count,
                                  ClusterGridItemMasks& items,
                                  uint32_t* result_mask,
                                  uint32_t word_count);

        uint32_t cullFrustum(ClusterGridItemMasks& items,
                             int i,
                             const nau::math::Matrix4& plane03_XYZW,
                             const nau::math::Matrix4& plane47_XYZW,
                             uint32_t* result_mask,
                             uint32_t word_count);

        uint32_t cullSpot(ClusterGridItemMasks& items,
                          int i,
                          math::Vector4 pos_radius,
                          math::Vector4 dir_angle,
                          uint32_t* result_mask,
                          uint32_t word_count);

        uint32_t cullSpots(const math::Vector4* pos_radius,
                           int pos_aligned_stride,
                           const math::Vector4* dir_angle,
                           int dir_aligned_stride,
                           ClusterGridItemMasks& items,
                           uint32_t* result_mask,
                           uint32_t word_count);

        float depthSliceScale = 1, depthSliceBias = 0, minSliceDist = 0, maxSliceDist = 1, znear = 0.01;

        nau::math::Matrix4 view, proj;  //-V730_NOINIT
    };
}  // namespace nau::render