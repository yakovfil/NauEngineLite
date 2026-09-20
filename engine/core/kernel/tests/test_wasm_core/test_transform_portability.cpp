#include <gtest/gtest.h>

#include <limits>

#include "nau/math/transform.h"

namespace
{
    using namespace nau::math;

    template <class T, class U>
    void expectXYZ(const T& actual, const U& expected, float tolerance = 1.0e-4f)
    {
        for (int i = 0; i < 3; ++i)
        {
            EXPECT_NEAR(float(actual.getElem(i)), float(expected.getElem(i)), tolerance);
        }
    }

    TEST(TransformPortability, NormalizationAcceptsIdentityAndUnitRotations)
    {
        EXPECT_TRUE(Transform::identity().isRotationNormalized());
        EXPECT_TRUE(Transform::identity().isValid());
        Transform rotated(Quat::rotationY(0.785398163f));
        EXPECT_TRUE(rotated.isRotationNormalized());
        EXPECT_TRUE(rotated.isValid());
        Transform rounded(Quat(0, 0, 0, 1.00001f));
        EXPECT_TRUE(rounded.isRotationNormalized());
        Transform normalized(normalize(Quat(1, 2, 3, 4)));
        EXPECT_TRUE(normalized.isRotationNormalized());
    }

    TEST(TransformPortability, NormalizationRejectsNonUnitAndNonFinite)
    {
        for (const float w : {0.0f, 0.5f, 1.001f, 2.0f,
                              std::numeric_limits<float>::infinity(),
                              std::numeric_limits<float>::quiet_NaN()})
        {
            Transform value(Quat(0, 0, 0, w));
            EXPECT_FALSE(value.isRotationNormalized());
            EXPECT_FALSE(value.isValid());
        }
        for (int component = 0; component < 4; ++component)
        {
            Transform value;
            value.getRotation().setElem(component, std::numeric_limits<float>::infinity());
            EXPECT_TRUE(value.isRotateNaN());
            EXPECT_FALSE(value.isRotationNormalized());
        }
    }

    TEST(TransformPortability, NonFinitePositionAndScaleAreInvalid)
    {
        for (int component = 0; component < 3; ++component)
        {
            Transform value;
            value.getTranslation().setElem(component, std::numeric_limits<float>::quiet_NaN());
            EXPECT_TRUE(value.isTranslationNaN());
            EXPECT_FALSE(value.isValid());
            value = Transform{};
            value.getScale().setElem(component, std::numeric_limits<float>::infinity());
            EXPECT_TRUE(value.isScaleNaN());
            EXPECT_FALSE(value.isValid());
        }
    }

    TEST(TransformPortability, IndexedWritesAgreeWithNamedComponents)
    {
        Vector3 vector(0);
        Quat quaternion(0, 0, 0, 0);
        for (int i = 0; i < 4; ++i)
        {
            vector.setElem(i, float(i + 1));
            quaternion[i] = float(i + 5);
        }
        EXPECT_FLOAT_EQ(float(vector.getX()), 1);
        EXPECT_FLOAT_EQ(float(vector.getY()), 2);
        EXPECT_FLOAT_EQ(float(vector.getZ()), 3);
        EXPECT_FLOAT_EQ(float(vector.getW()), 4);
        EXPECT_FLOAT_EQ(float(quaternion.getX()), 5);
        EXPECT_FLOAT_EQ(float(quaternion.getY()), 6);
        EXPECT_FLOAT_EQ(float(quaternion.getZ()), 7);
        EXPECT_FLOAT_EQ(float(quaternion.getW()), 8);
        const auto& constantQuaternion = quaternion;
        const auto& constantVector = vector;
        for (int i = 0; i < 4; ++i)
        {
            EXPECT_FLOAT_EQ(float(constantQuaternion[i]), float(i + 5));
            EXPECT_FLOAT_EQ(float(constantVector[i]), float(i + 1));
        }
    }

    TEST(TransformPortability, PointAndVectorAgreeWithMatrix)
    {
        Transform value(Quat::rotationY(0.7f), Vector3(2, 3, 4), Vector3(2, 3, 4));
        const Vector3 vector(1, 2, -1);
        const Point3 point(1, 2, -1);
        expectXYZ(value.transformVector(vector), value.getMatrix() * Vector4(vector, 0));
        expectXYZ(value.transformPoint(point), value.getMatrix() * Vector4(Vector3(point), 1));
    }

    TEST(TransformPortability, CompositionAndInverseWithUniformScale)
    {
        Transform parent(Quat::rotationY(0.4f), Vector3(2, 3, 4), Vector3(2));
        Transform child(Quat::rotationX(0.2f), Vector3(-1, 2, 1));
        const Point3 point(1, 2, 3);
        // Native quaternion normalization uses unrefined _mm_rsqrt_ps. Retain its
        // existing accuracy instead of changing the desktop math implementation.
        expectXYZ((parent * child).transformPoint(point), parent.transformPoint(child.transformPoint(point)), 0.005f);
        expectXYZ(parent.inverse().transformPoint(parent.transformPoint(point)), point);
        const auto relative = (parent * child).getRelativeTransformInverse(parent);
        expectXYZ(relative.transformPoint(point), child.transformPoint(point), 0.005f);
    }

    TEST(TransformPortability, NegativeAndZeroScaleRemainFinite)
    {
        Transform mirrored(Quat::identity(), Vector3(2, 3, 4), Vector3(-2, 3, 4));
        Transform translated(Quat::identity(), Vector3(1, 0, 0));
        const auto combined = mirrored * translated;
        EXPECT_FALSE(combined.containsNaN());
        expectXYZ(combined.getTranslation(), Vector3(0, 3, 4));
        EXPECT_FALSE(Transform(Quat::identity(), Vector3(0), Vector3(0)).inverse().containsNaN());
    }

    TEST(TransformPortability, InterpolationEndpointsAndFormatting)
    {
        Transform a(Quat::identity(), Vector3(0), Vector3(1));
        Transform b(Quat::rotationY(0.5f), Vector3(2, 4, 6), Vector3(3));
        EXPECT_TRUE(lerpTransform(a, b, 0).similar(a, 1.0e-4f));
        EXPECT_TRUE(slerpTransform(a, b, 1).similar(b, 1.0e-4f));
        expectXYZ(lerpTransform(a, b, 0.5f).getTranslation(), Vector3(1, 2, 3));
        EXPECT_FALSE(fmt::format("{}", b).empty());
        EXPECT_TRUE(a == Transform::identity());
    }
}  // namespace
