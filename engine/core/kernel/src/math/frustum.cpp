// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include "nau/math/dag_frustum.h"

#define ALMOST_ZERO(F) (fabsf(F) <= 2.0f * FLT_EPSILON)
#define IS_SPECIAL(F)  ((FP_BITS(F) & 0x7f800000L) == 0x7f800000L)

///////////////////////////////////////////////////////////////////////////

namespace nau::math
{
    __m128 v_splat_x(__m128 a)
    {
        return _mm_shuffle_ps(a, a, _MM_SHUFFLE(0, 0, 0, 0));
    }
    __m128 v_splat_y(__m128 a)
    {
        return _mm_shuffle_ps(a, a, _MM_SHUFFLE(1, 1, 1, 1));
    }
    __m128 v_splat_z(__m128 a)
    {
        return _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 2, 2, 2));
    }
    __m128 v_splat_w(__m128 a)
    {
        return _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 3, 3, 3));
    }
    __m128 v_madd(__m128 a, __m128 b, __m128 c)
    {
        return _mm_add_ps(_mm_mul_ps(a, b), c);
    }

    #define V_SHUFFLE(v, mask) _mm_shuffle_ps(v, v, mask)
    #define V_SHUFFLE_REV(v, maskW, maskZ, maskY, maskX) V_SHUFFLE(v, _MM_SHUFFLE(maskW, maskZ, maskY, maskX))

    __m128 sse2_dot3(__m128 a, __m128 b)
    {
        __m128 m = _mm_mul_ps(a, b);

        return _mm_add_ps(
            _mm_add_ps(_mm_shuffle_ps(m, m, _MM_SHUFFLE(2, 2, 1, 0)),
                V_SHUFFLE_REV(m, 1, 1, 0, 2)),
            _mm_shuffle_ps(m, m, _MM_SHUFFLE(0, 0, 2, 1))
        );
    }
    __m128 v_dot3(__m128 a, __m128 b)
    {
        return sse2_dot3(a,b);
    }
    __m128 v_add(__m128 a, __m128 b) { return _mm_add_ps(a, b); }
    __m128 v_sub(__m128 a, __m128 b) { return _mm_sub_ps(a, b); }
    __m128 v_or(__m128 a, __m128 b) { return _mm_or_ps(a,b); }

    int v_test_vec_mask_eq_0(__m128 v) { return (~unsigned(-_mm_movemask_ps(v))) >> 31; }
    int v_test_vec_mask_neq_0(__m128 v) { return (unsigned(-_mm_movemask_ps(v))) >> 31; }


    inline int v_is_visible_sphere(Vector3 center, Vector4 r,
        const Vector4& plane03X, const Vector4& plane03Y, const Vector4& plane03Z, const Vector4& plane03W,
        const Vector4& plane4, const Vector4& plane5)
    {
        __m128 res03;
        res03 = v_madd(v_splat_x(center.get128()), plane03X.get128(), plane03W.get128());
        res03 = v_madd(v_splat_y(center.get128()), plane03Y.get128(), res03);
        res03 = v_madd(v_splat_z(center.get128()), plane03Z.get128(), res03);
        res03 = v_add(res03, r.get128());
        res03 = v_or(res03, v_add(v_add(v_dot3(center.get128(), plane4.get128()), r.get128()), v_splat_w(plane4.get128())));
        res03 = v_or(res03, v_add(v_add(v_dot3(center.get128(), plane5.get128()), r.get128()), v_splat_w(plane5.get128())));

        return v_test_vec_mask_eq_0(res03);
    }

    int NauFrustum::testSphereB(Vector3 center, Vector4 r) const
    {
        return v_is_visible_sphere(center, r, plane03X, plane03Y, plane03Z, plane03W, camPlanes[4], camPlanes[5]);
    }

    int v_sphere_intersect(Vector3 center, Vector4 r,
        const Vector4& plane03X, const Vector4& plane03Y, const Vector4& plane03Z, const Vector4& plane03W,
        const Vector4& plane4, const Vector4& plane5)
    {
        __m128 res03;
        res03 = v_madd(v_splat_x(center.get128()), plane03X.get128(), plane03W.get128());
        res03 = v_madd(v_splat_y(center.get128()), plane03Y.get128(), res03);
        res03 = v_madd(v_splat_z(center.get128()), plane03Z.get128(), res03);
        res03 = v_add(res03, r.get128());
        res03 = v_or(res03, v_add(v_add(v_dot3(center.get128(), plane4.get128()), r.get128()), v_splat_w(plane4.get128())));
        res03 = v_or(res03, v_add(v_add(v_dot3(center.get128(), plane5.get128()), r.get128()), v_splat_w(plane5.get128())));

        if (v_test_vec_mask_neq_0(res03))
            return 0;

        res03 = v_madd(v_splat_x(center.get128()), plane03X.get128(), plane03W.get128());
        res03 = v_madd(v_splat_y(center.get128()), plane03Y.get128(), res03);
        res03 = v_madd(v_splat_z(center.get128()), plane03Z.get128(), res03);
        res03 = v_sub(res03, r.get128());
        res03 = v_or(res03, v_add(v_sub(v_dot3(center.get128(), plane4.get128()), r.get128()), v_splat_w(plane4.get128())));
        res03 = v_or(res03, v_add(v_sub(v_dot3(center.get128(), plane5.get128()), r.get128()), v_splat_w(plane5.get128())));

        return v_test_vec_mask_neq_0(res03) + 1;
    }

    __m128 v_splats(float a) {return _mm_set1_ps(a);}

    __m128 v_perm_yzxx(__m128 a) { return V_SHUFFLE_REV(a, 0,0,2,1); }
    __m128 v_perm_yzxy(__m128 a) { return V_SHUFFLE_REV(a, 1,0,2,1); }
    __m128 v_perm_yzxw(__m128 a) { return V_SHUFFLE_REV(a, 3,0,2,1); }
    __m128 v_perm_zxyw(__m128 a) { return V_SHUFFLE_REV(a, 3,1,0,2); }
    __m128 v_perm_xxyy(__m128 a) { return V_SHUFFLE_REV(a, 1,1,0,0); }
    __m128 v_perm_zzww(__m128 a) { return V_SHUFFLE_REV(a, 3,3,2,2); }

    __m128 v_mul(__m128 a, __m128 b) { return _mm_mul_ps(a, b); }

    __m128 v_cross3(__m128 a, __m128 b)
    {
        // a.y * b.z - a.z * b.y
        // a.z * b.x - a.x * b.z
        // a.x * b.y - a.y * b.x
        __m128 tmp0 = v_perm_yzxw(a);
        __m128 tmp1 = v_perm_zxyw(b);
        __m128 tmp2 = v_mul(tmp0, b);
        __m128 tmp3 = v_mul(tmp0, tmp1);
        __m128 tmp4 = v_perm_yzxw(tmp2);
        return v_sub(tmp3, tmp4);
    }

    __m128 v_cmp_ge(__m128 a, __m128 b) { return _mm_cmpge_ps(a, b); }
    __m128 v_cmp_gt(__m128 a, __m128 b) { return _mm_cmpgt_ps(a, b); }

    __m128 v_cmp_le(__m128 a, __m128 b) { return v_cmp_ge(b, a); }
    __m128 v_cmp_lt(__m128 a, __m128 b) { return v_cmp_gt(b, a); }

    __m128 v_abs(__m128 a)
    {
        // Clear the sign bit using the same SSE2 implementation on MSVC and clang-cl.
        __m128i absmask = _mm_castps_si128(a);
        absmask = _mm_srli_epi32(_mm_cmpeq_epi32(absmask, absmask), 1);
        return _mm_and_ps(_mm_castsi128_ps(absmask), a);
    }

    __m128 v_is_unsafe_divisor(__m128 a)
    {
        return v_cmp_lt(v_abs(a), v_splats(4e-19f));
    }

    __m128 v_rcp(__m128 a)
    {
        __m128 y0 = _mm_rcp_ps(a);
        return _mm_sub_ps(_mm_add_ps(y0, y0), _mm_mul_ps(a, _mm_mul_ps(y0, y0)));
    }
    __m128 v_nmsub(__m128 a, __m128 b, __m128 c) { return _mm_sub_ps(c, _mm_mul_ps(a, b)); }
    __m128 v_zero() { return _mm_setzero_ps(); }
    __m128 v_andnot(__m128 a, __m128 b) { return _mm_andnot_ps(a,b); }

    Vector3 three_plane_intersection(Vector4 p0, Vector4 p1, Vector4 p2, Vector4& invalid)
    {
        __m128 n1_n2 = v_cross3(p1.get128(), p2.get128()), n2_n0 = v_cross3(p2.get128(), p0.get128()), n0_n1 = v_cross3(p0.get128(), p1.get128());
        __m128 cosTheta = v_dot3(p0.get128(), n1_n2);
        invalid = Vector4(v_is_unsafe_divisor(cosTheta));
        __m128 secTheta = v_rcp(cosTheta);

        __m128 intersectPt;
        intersectPt = v_nmsub(n1_n2, v_splat_w(p0.get128()), v_zero());
        intersectPt = v_nmsub(n2_n0, v_splat_w(p1.get128()), intersectPt);
        intersectPt = v_nmsub(n0_n1, v_splat_w(p2.get128()), intersectPt);
        return Vector3(v_mul(intersectPt, secTheta));
    }

    void v_construct_camplanes(const nau::math::Matrix4& clip,
        Vector4& camPlanes0, Vector4& camPlanes1, Vector4& camPlanes2, Vector4& camPlanes3, Vector4& camPlanes4, Vector4& camPlanes5)
    {
        nau::math::Matrix4 m2 = nau::math::transpose(clip);
        //v_mat44_transpose(m2, clip);
        camPlanes0 = m2.getCol3() - m2.getCol0();  // right
        camPlanes1 = m2.getCol3() + m2.getCol0();  // left
        camPlanes2 = m2.getCol3() - m2.getCol1();  // top
        camPlanes3 = m2.getCol3() + m2.getCol1();  // bottom
        camPlanes4 = m2.getCol3() - m2.getCol2();  // far if forward depth (near otherwise)
        camPlanes5 = m2.getCol3() + m2.getCol2();  // near if forward depth (far otherwise)
    }

    __m128 sse2_dot3_x(__m128 a, __m128 b)
    {
        __m128 m = _mm_mul_ps(a, b);

        return _mm_add_ss(_mm_add_ss(m, v_splat_y(m)), _mm_shuffle_ps(m, m, 2));
    }

     __m128 v_div(__m128 a, __m128 b) { return _mm_div_ps(a, b); }
     __m128 v_sqrt_x(__m128 a) { return _mm_sqrt_ss(a); }
     __m128 v_dot3_x(__m128 a, __m128 b) { return sse2_dot3_x(a,b); }

     __m128 v_norm3(__m128 a) { return v_div(a, v_splat_x(v_sqrt_x(v_dot3_x(a,a)))); }



    ///////////////////////////////////////////////////////////////////////////
    //  PlaneIntersection
    //    computes the point where three planes intersect
    //    returns whether or not the point exists.
    static __forceinline Vector3 planeIntersection(Vector4 p0, Vector4 p1, Vector4 p2)
    {
        Vector4 invalid;
        Vector3 result = three_plane_intersection(p0, p1, p2, invalid);
        result = Vector3(v_andnot(invalid.get128(), result.get128()));

        return result;
    }

    __m128 v_perm_xyzd(__m128 xyzw, __m128 abcd)
    {
        __m128 zzdd = _mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(3, 3, 2, 2));
        return _mm_shuffle_ps(xyzw, zzdd, _MM_SHUFFLE(3, 0, 1, 0));
    }

    __m128 v_min(__m128 a, __m128 b) { return _mm_min_ps(a, b); }
    __m128 v_max(__m128 a, __m128 b) { return _mm_max_ps(a, b); }

    void v_bbox3_add_pt(BBox3& b, Vector3 p)
    {
        b.lim[0] = Vector3(v_min(b.lim[0].get128(), p.get128()));
        b.lim[1] = Vector3(v_max(b.lim[1].get128(), p.get128()));
    }

    #define _MM_TRANSPOSE4_PS(row0, row1, row2, row3) {                 \
            __m128 _Tmp3, _Tmp2, _Tmp1, _Tmp0;                          \
                                                                    \
            _Tmp0   = _mm_shuffle_ps((row0.get128()), (row1.get128()), 0x44);          \
            _Tmp2   = _mm_shuffle_ps((row0.get128()), (row1.get128()), 0xEE);          \
            _Tmp1   = _mm_shuffle_ps((row2.get128()), (row3.get128()), 0x44);          \
            _Tmp3   = _mm_shuffle_ps((row2.get128()), (row3.get128()), 0xEE);          \
                                                                    \
            (row0) = Vector4(_mm_shuffle_ps(_Tmp0, _Tmp1, 0x88));              \
            (row1) = Vector4(_mm_shuffle_ps(_Tmp0, _Tmp1, 0xDD));              \
            (row2) = Vector4(_mm_shuffle_ps(_Tmp2, _Tmp3, 0x88));              \
            (row3) = Vector4(_mm_shuffle_ps(_Tmp2, _Tmp3, 0xDD));              \
        }

    void v_mat44_transpose(Vector4& r0, Vector4& r1, Vector4& r2, Vector4& r3)
    {
        _MM_TRANSPOSE4_PS(r0, r1, r2, r3);
    }

    //  build a frustum from a camera (projection, or viewProjection) matrix
    void NauFrustum::construct(const nau::math::Matrix4& matrix)
    {
        v_construct_camplanes(matrix, camPlanes[0], camPlanes[1], camPlanes[2], camPlanes[3], camPlanes[4], camPlanes[5]);
        for (int p = 0; p < 6; p++)
            camPlanes[p] = Vector4(v_norm3(camPlanes[p].get128())); // .w is divided by length(xyz) as side effect

        //  build a bit-field that will tell us the indices for the nearest and farthest vertices from each plane...
        plane03X = camPlanes[0];
        plane03Y = camPlanes[1];
        plane03Z = camPlanes[2];
        plane03W = camPlanes[3];
        v_mat44_transpose(plane03X, plane03Y, plane03Z, plane03W);

        plane03W2 = Vector4(v_add(plane03W.get128(), plane03W.get128()));

        plane4W2 = camPlanes[4];
        plane5W2 = camPlanes[5];
        plane4W2 = Vector4(v_perm_xyzd(plane4W2.get128(), v_add(plane4W2.get128(), plane4W2.get128())));
        plane5W2 = Vector4(v_perm_xyzd(plane5W2.get128(), v_add(plane5W2.get128(), plane5W2.get128())));
    }

    void NauFrustum::calcFrustumBBox(BBox3& box) const
    {
        box.lim[0] = box.lim[1] = planeIntersection(camPlanes[5], camPlanes[2], camPlanes[1]);
        v_bbox3_add_pt(box, planeIntersection(camPlanes[4], camPlanes[2], camPlanes[1]));
        v_bbox3_add_pt(box, planeIntersection(camPlanes[5], camPlanes[3], camPlanes[1]));
        v_bbox3_add_pt(box, planeIntersection(camPlanes[4], camPlanes[3], camPlanes[1]));
        v_bbox3_add_pt(box, planeIntersection(camPlanes[5], camPlanes[2], camPlanes[0]));
        v_bbox3_add_pt(box, planeIntersection(camPlanes[4], camPlanes[2], camPlanes[0]));
        v_bbox3_add_pt(box, planeIntersection(camPlanes[5], camPlanes[3], camPlanes[0]));
        v_bbox3_add_pt(box, planeIntersection(camPlanes[4], camPlanes[3], camPlanes[0]));
    }

    void NauFrustum::generateAllPointFrustm(Vector3* pntList) const
    {
        pntList[0] = planeIntersection(camPlanes[5], camPlanes[2], camPlanes[1]);
        pntList[1] = planeIntersection(camPlanes[4], camPlanes[2], camPlanes[1]);
        pntList[2] = planeIntersection(camPlanes[5], camPlanes[3], camPlanes[1]);
        pntList[3] = planeIntersection(camPlanes[4], camPlanes[3], camPlanes[1]);
        pntList[4] = planeIntersection(camPlanes[5], camPlanes[2], camPlanes[0]);
        pntList[5] = planeIntersection(camPlanes[4], camPlanes[2], camPlanes[0]);
        pntList[6] = planeIntersection(camPlanes[5], camPlanes[3], camPlanes[0]);
        pntList[7] = planeIntersection(camPlanes[4], camPlanes[3], camPlanes[0]);
    }


    int NauFrustum::testSphere(Vector3 center, Vector4 r) const
    {
        return v_sphere_intersect(center, r, plane03X, plane03Y, plane03Z, plane03W, camPlanes[4], camPlanes[5]);
    }


    __m128 v_perm_xycw(__m128 xyzw, __m128 abcd)
    {
      __m128 wwcc = _mm_shuffle_ps(xyzw, abcd, _MM_SHUFFLE(2, 2, 3, 3));
      return _mm_shuffle_ps(xyzw, wwcc, _MM_SHUFFLE(0, 3, 1, 0));
    }

    __m128i v_splatsi(int a)
    {
        return _mm_set1_epi32(a);
    }

    __m128 v_xor(__m128 a, __m128 b)
    {
        return _mm_xor_ps(a,b);
    }

    __m128 v_neg(__m128 a)
    {
        return v_xor(a, _mm_castsi128_ps(v_splatsi(0x80000000)));
    }

    Vector4 shrink_zfar_plane(Vector4 zfar_plane, Vector4 cur_view_pos, Vector4 max_z_far_dist)
    {
        Vector4 zfarDist = Vector4(distFromPlane(Point3(cur_view_pos), zfar_plane));
        Vector4 newZFarDist = Vector4(v_min(max_z_far_dist.get128(), v_splat_w(zfarDist.get128())));
        Vector4 ofsDist = newZFarDist - zfarDist;
        return Vector4(v_perm_xyzd(zfar_plane.get128(), v_add(zfar_plane.get128(), ofsDist.get128())));
    }

    Vector4 expand_znear_plane(Vector4 znear_plane, Vector4 cur_view_pos, Vector4 max_z_near_dist)
    {
        Vector4 znearDist = Vector4(distFromPlane(Point3(cur_view_pos), znear_plane)); // negative
        Vector4 newZnearDist = Vector4(v_min(v_neg(max_z_near_dist.get128()), v_splat_w(znearDist.get128())));
        Vector4 ofsDist = newZnearDist - znearDist;
        return Vector4(v_perm_xyzd(znear_plane.get128(), v_add(znear_plane.get128(), ofsDist.get128())));
    }

} // namespace nau::math

