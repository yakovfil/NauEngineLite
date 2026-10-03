// test_messaging.cpp
//
// Copyright (c) N-GINN LLC., 2023-2025. All rights reserved.
//

#include <vector>

#include "nau/dataBlock/dag_dataBlock.h"
#include "nau/math/math.h"

using namespace Vectormath;

#define EPS 0.000001f

namespace nau::test
{
    TEST(TestMath, Vector3Init)
    {
        using namespace nau::math;

        vec3 vec = {1.0f, 2.0f, 3.0f};

#ifdef MATH_USE_DOUBLE_PRECISION
        ASSERT_DOUBLE_EQ(vec.getX(), 1.0);
        ASSERT_DOUBLE_EQ(vec.getY(), 2.0);
        ASSERT_DOUBLE_EQ(vec.getZ(), 3.0);
#else
        ASSERT_FLOAT_EQ(vec.getX(), 1.0f);
        ASSERT_FLOAT_EQ(vec.getY(), 2.0f);
        ASSERT_FLOAT_EQ(vec.getZ(), 3.0f);
#endif
    }

    TEST(TestMath, Vector3Sum)
    {
        using namespace nau::math;

        vec3 vec1 = {1.0f, 2.0f, 3.0f};
        vec3 vec2 = {1.0f, 2.0f, 3.0f};

        vec3 vec = vec1 + vec2;

#ifdef MATH_USE_DOUBLE_PRECISION
        ASSERT_DOUBLE_EQ(vec.getX(), 2.0);
        ASSERT_DOUBLE_EQ(vec.getY(), 4.0);
        ASSERT_DOUBLE_EQ(vec.getZ(), 6.0);
#else
        ASSERT_FLOAT_EQ(vec.getX(), 2.0f);
        ASSERT_FLOAT_EQ(vec.getY(), 4.0f);
        ASSERT_FLOAT_EQ(vec.getZ(), 6.0f);
#endif
    }

    TEST(TestMath, Vector3DotProduct)
    {
        using namespace nau::math;

        vec3 vec1 = {1.0f, 2.0f, 3.0f};
        vec3 vec2 = {1.0f, 2.0f, 3.0f};

        float res = dot(vec1, vec2);

#ifdef MATH_USE_DOUBLE_PRECISION
        ASSERT_DOUBLE_EQ(res, 14.0);
#else
        ASSERT_FLOAT_EQ(res, 14.0f);
#endif
    }

    TEST(TestMath, Vector3CrossProduct)
    {
        using namespace nau::math;

        vec3 vec1 = {1, 0, 0};
        vec3 vec2 = {0, 1, 0};

        vec3 vec = cross(vec1, vec2);

#ifdef MATH_USE_DOUBLE_PRECISION
        ASSERT_DOUBLE_EQ(vec.getX(), 0.0);
        ASSERT_DOUBLE_EQ(vec.getY(), 0.0);
        ASSERT_DOUBLE_EQ(vec.getZ(), 1.0);
#else
        ASSERT_FLOAT_EQ(vec.getX(), 0.0f);
        ASSERT_FLOAT_EQ(vec.getY(), 0.0f);
        ASSERT_FLOAT_EQ(vec.getZ(), 1.0f);
#endif
    }

    TEST(TestMath, GenerateSpherePoints)
    {
        using namespace nau::math;

        std::vector<float3> points;
        generateSpherePoints(points, 10);

        ASSERT_EQ(points.size(), 1200);

        ASSERT_FLOAT_EQ(points[0].x, -0.309017003f);
        ASSERT_FLOAT_EQ(points[0].y, -0.951056540f);
        ASSERT_FLOAT_EQ(points[0].z, 0.0f);
    }

    TEST(TestMath, EqualityAffineTransform)
    {
        using namespace nau::math;
        using TestType = AffineTransform;

        TestType a = {
            {0, 0, 1},
            {0, 0, 0, 1},
            {0, 1, 1}
        };

        TestType b = {
            {0, 0, 1},
            {0, 0, 0, 1},
            {0, 1, 1},
        };

        TestType c = {
            {0, 0, 1},
            {0, 1, 0, 1},
            {0, 1, 1}
        };

        EXPECT_TRUE(a == b);
        EXPECT_TRUE(a != c);
        EXPECT_TRUE(b != c);
    }

    TEST(TestMath, EqualityTransform)
    {
        using namespace nau::math;
        using TestType = Transform3;

        TestType a = {
            {1, 0, 0, 1},
            {0, 0, 1}
        };

        TestType b = {
            {1, 0, 0, 1},
            {0, 0, 1},
        };

        TestType c = {
            {1, 1, 0, 1},
            {0, 1, 1}
        };

        EXPECT_TRUE(a == b);
        EXPECT_TRUE(a != c);
        EXPECT_TRUE(b != c);
    }

    TEST(TestMath, EqualityIVector3)
    {
        using namespace nau::math;
        using TestType = IVector3;

        TestType a = {0, 0, 1};
        TestType b = {0, 0, 1};
        TestType c = {0, 1, 1};
        TestType d = {0, 1, 0};

        EXPECT_TRUE(a == b);
        EXPECT_TRUE(a != c);
        EXPECT_TRUE(a != d);
        EXPECT_TRUE(b != c);
        EXPECT_TRUE(b != d);
        EXPECT_TRUE(c != d);
    }

    TEST(TestMath, EqualityIVector4)
    {
        using namespace nau::math;
        using TestType = IVector4;

        TestType a = {0, 0, 0, 1};
        TestType b = {0, 0, 0, 1};
        TestType c = {0, 0, 1, 1};
        TestType d = {0, 0, 1, 0};

        EXPECT_TRUE(a == b);
        EXPECT_TRUE(a != c);
        EXPECT_TRUE(a != d);
        EXPECT_TRUE(b != c);
        EXPECT_TRUE(b != d);
        EXPECT_TRUE(c != d);
    }

    TEST(TestMath, EqualityMatrix3)
    {
        using namespace nau::math;
        using TestType = Matrix3;

        TestType a = {
            {0, 0, 1},
            {0, 1, 1},
            {0, 1, 0}
        };

        TestType b = {
            {0, 0, 1},
            {0, 1, 1},
            {0, 1, 0}
        };

        TestType c = {
            {0,   0, 1},
            {0,   1, 1},
            {0, 1.5, 0}
        };

        EXPECT_TRUE(a == b);
        EXPECT_TRUE(a != c);
        EXPECT_TRUE(b != c);
    }

    TEST(TestMath, EqualityMatrix3d)
    {
        using namespace nau::math;
        using TestType = Matrix3d;

        TestType a = {
            {0, 0, 1},
            {0, 1, 1},
            {0, 1, 0}
        };

        TestType b = {
            {0, 0, 1},
            {0, 1, 1},
            {0, 1, 0}
        };

        TestType c = {
            {0,   0, 1},
            {0,   1, 1},
            {0, 1.5, 0}
        };

        EXPECT_TRUE(a == b);
        EXPECT_TRUE(a != c);
        EXPECT_TRUE(b != c);
    }

    TEST(TestMath, EqualityMatrix4d)
    {
        using namespace nau::math;
        using TestType = Matrix4;

        TestType a = {
            {0, 0, 0, 1},
            {0, 0, 0, 1},
            {0, 0, 1, 1},
            {0, 0, 1, 0}
        };

        TestType b = {
            {0, 0, 0, 1},
            {0, 0, 0, 1},
            {0, 0, 1, 1},
            {0, 0, 1, 0}
        };

        TestType c = {
            {0, 0,   0, 1},
            {0, 0,   0, 1},
            {0, 0,   1, 1},
            {0, 0, 1.5, 0}
        };

        EXPECT_TRUE(a == b);
        EXPECT_TRUE(a != c);
        EXPECT_TRUE(b != c);
    }

    TEST(TestMath, EqualityMatrix4)
    {
        using namespace nau::math;
        using TestType = Matrix4d;

        TestType a = {
            {0, 0, 0, 1},
            {0, 0, 0, 1},
            {0, 0, 1, 1},
            {0, 0, 1, 0}
        };

        TestType b = {
            {0, 0, 0, 1},
            {0, 0, 0, 1},
            {0, 0, 1, 1},
            {0, 0, 1, 0}
        };

        TestType c = {
            {0, 0,   0, 1},
            {0, 0,   0, 1},
            {0, 0,   1, 1},
            {0, 0, 1.5, 0}
        };

        EXPECT_TRUE(a == b);
        EXPECT_TRUE(a != c);
        EXPECT_TRUE(b != c);
    }

    TEST(TestMath, EqualityPoint3)
    {
        using namespace nau::math;
        using TestType = Point3;

        TestType a = {0, 0, 1};
        TestType b = {0, 0, 1};
        TestType c = {0, 1, 1};
        TestType d = {0, 1.5, 0};
        a.setW(1);

        EXPECT_TRUE(a == b);
        EXPECT_TRUE(a != c);
        EXPECT_TRUE(a != d);
        EXPECT_TRUE(b != c);
        EXPECT_TRUE(b != d);
        EXPECT_TRUE(c != d);
    }

    TEST(TestMath, EqualityVector3)
    {
        using namespace nau::math;
        using TestType = Vector3;

        TestType a = {0, 0, 1};
        TestType b = {0, 0, 1};
        TestType c = {0, 1, 1};
        TestType d = {0, 1.5, 0};
        a.setW(1);

        EXPECT_TRUE(a == b);
        EXPECT_TRUE(a != c);
        EXPECT_TRUE(a != d);
        EXPECT_TRUE(b != c);
        EXPECT_TRUE(b != d);
        EXPECT_TRUE(c != d);
    }

    TEST(TestMath, EqualityVector2)
    {
        using namespace nau::math;
        using TestType = Vector2;

        TestType a = {0, 0};
        TestType b = {0, 0};
        TestType c = {0, 1};
        TestType d = {0, 1.5};

        EXPECT_TRUE(a == b);
        EXPECT_TRUE(a != c);
        EXPECT_TRUE(a != d);
        EXPECT_TRUE(b != c);
        EXPECT_TRUE(b != d);
        EXPECT_TRUE(c != d);
    }

    TEST(TestMath, EqualityVector3d)
    {
        using namespace nau::math;
        using TestType = Vector3d;

        TestType a = {0, 0, 1};
        TestType b = {0, 0, 1};
        TestType c = {0, 1, 1};
        TestType d = {0, 1.5, 0};
        a.setW(1);

        EXPECT_TRUE(a == b);
        EXPECT_TRUE(a != c);
        EXPECT_TRUE(a != d);
        EXPECT_TRUE(b != c);
        EXPECT_TRUE(b != d);
        EXPECT_TRUE(c != d);
    }

    TEST(TestMath, EqualityUVector3)
    {
        using namespace nau::math;
        using TestType = UVector3;

        TestType a = {0, 0, 1};
        TestType b = {0, 0, 1};
        TestType c = {0, 1, 1};
        TestType d = {0, 2, 0};

        EXPECT_TRUE(a == b);
        EXPECT_TRUE(a != c);
        EXPECT_TRUE(a != d);
        EXPECT_TRUE(b != c);
        EXPECT_TRUE(b != d);
        EXPECT_TRUE(c != d);
    }

    TEST(TestMath, EqualityQuat)
    {
        using namespace nau::math;
        using TestType = Quat;

        TestType a = {0, 0, 0, 1};
        TestType b = {0, 0, 0, 1};
        TestType c = {0, 0, 1, 1};
        TestType d = {0, 0, 1, 0};
        TestType e = {0, 0, 1.5, 0};

        EXPECT_TRUE(a == b);
        EXPECT_TRUE(a != c);
        EXPECT_TRUE(a != d);
        EXPECT_TRUE(b != c);
        EXPECT_TRUE(b != d);
        EXPECT_TRUE(c != d);

        EXPECT_TRUE(e != a);
        EXPECT_TRUE(e != b);
        EXPECT_TRUE(e != c);
        EXPECT_TRUE(e != d);
    }

    TEST(TestMath, EqualityVector4)
    {
        using namespace nau::math;
        using TestType = Vector4;

        TestType a = {0, 0, 0, 1};
        TestType b = {0, 0, 0, 1};
        TestType c = {0, 0, 1, 1};
        TestType d = {0, 0, 1, 0};
        TestType e = {0, 0, 1.5, 0};

        EXPECT_TRUE(a == b);
        EXPECT_TRUE(a != c);
        EXPECT_TRUE(a != d);
        EXPECT_TRUE(b != c);
        EXPECT_TRUE(b != d);
        EXPECT_TRUE(c != d);

        EXPECT_TRUE(e != a);
        EXPECT_TRUE(e != b);
        EXPECT_TRUE(e != c);
        EXPECT_TRUE(e != d);
    }

    TEST(TestMath, EqualityUVector4)
    {
        using namespace nau::math;
        using TestType = UVector4;

        TestType a = {0, 0, 0, 1};
        TestType b = {0, 0, 0, 1};
        TestType c = {0, 0, 1, 1};
        TestType d = {0, 0, 1, 0};
        TestType e = {0, 0, 2, 0};

        EXPECT_TRUE(a == b);
        EXPECT_TRUE(a != c);
        EXPECT_TRUE(a != d);
        EXPECT_TRUE(b != c);
        EXPECT_TRUE(b != d);
        EXPECT_TRUE(c != d);

        EXPECT_TRUE(e != a);
        EXPECT_TRUE(e != b);
        EXPECT_TRUE(e != c);
        EXPECT_TRUE(e != d);
    }

    TEST(TestMath, EqualityVector4d)
    {
        using namespace nau::math;

        Vector4d a = {0, 0, 0, 1};
        Vector4d b = {0, 0, 0, 1};
        Vector4d c = {0, 0, 1, 1};
        Vector4d d = {0, 0, 1, 0};
        Vector4d e = {0, 0, 1.5, 0};

        EXPECT_TRUE(a == b);
        EXPECT_TRUE(a != c);
        EXPECT_TRUE(a != d);
        EXPECT_TRUE(b != c);
        EXPECT_TRUE(b != d);
        EXPECT_TRUE(c != d);

        EXPECT_TRUE(e != a);
        EXPECT_TRUE(e != b);
        EXPECT_TRUE(e != c);
        EXPECT_TRUE(e != d);
    }

    // TEST(TestMath, GetRandom)
    //{
    //     int randomInt1     = Vectormath::getRandomInt();
    //     int randomInt2     = Vectormath::getRandomInt();
    //     int randomInt3     = Vectormath::getRandomInt();
    //     float randomFloat1 = Vectormath::randomFloat(0.0f, 10.0f);
    //     float randomFloat2 = Vectormath::randomFloat(0.0f, 10.0f);
    //     float randomFloat3 = Vectormath::randomFloat(0.0f, 10.0f);
    //
    //     ASSERT_EQ(randomInt1, 1604589139);
    //     ASSERT_EQ(randomInt2, 1069070121);
    //     ASSERT_EQ(randomInt3, 1110651321);
    //
    //     ASSERT_TRUE(abs(randomFloat1 - 2.58911347f) < EPS);
    //     ASSERT_TRUE(abs(randomFloat2 - 5.63411617f) < EPS);
    //     ASSERT_TRUE(abs(randomFloat3 - 3.60538936f) < EPS);
    // }

    TEST(TestMath, HasInfOrNaN)
    {
        FloatInVec testValue;
#define SET_VALUES(x, y, z, w) \
    testValue = FloatInVec{x, y, z, w}

        SET_VALUES(1, 1, 1, 1);
        EXPECT_TRUE(!testValue.hasInfOrNaN());

        SET_VALUES(0, 0, 0, 0);
        EXPECT_TRUE(!testValue.hasInfOrNaN());

        SET_VALUES(1, 1, 1, INFINITY);
        EXPECT_TRUE(testValue.hasInfOrNaN());
        SET_VALUES(1, 1, INFINITY, 1);
        EXPECT_TRUE(testValue.hasInfOrNaN());
        SET_VALUES(1, INFINITY, 1, 1);
        EXPECT_TRUE(testValue.hasInfOrNaN());
        SET_VALUES(INFINITY, 1, 1, 1);
        EXPECT_TRUE(testValue.hasInfOrNaN());
        ;

        SET_VALUES(1, 1, 1, -INFINITY);
        EXPECT_TRUE(testValue.hasInfOrNaN());
        SET_VALUES(1, 1, -INFINITY, 1);
        EXPECT_TRUE(testValue.hasInfOrNaN());
        SET_VALUES(1, -INFINITY, 1, 1);
        EXPECT_TRUE(testValue.hasInfOrNaN());
        SET_VALUES(-INFINITY, 1, 1, 1);
        EXPECT_TRUE(testValue.hasInfOrNaN());

        SET_VALUES(1, 1, 1, NAN);
        EXPECT_TRUE(testValue.hasInfOrNaN());
        SET_VALUES(1, 1, NAN, 1);
        EXPECT_TRUE(testValue.hasInfOrNaN());
        SET_VALUES(1, NAN, 1, 1);
        EXPECT_TRUE(testValue.hasInfOrNaN());
        SET_VALUES(NAN, 1, 1, 1);
        EXPECT_TRUE(testValue.hasInfOrNaN());

        SET_VALUES(INFINITY, 1, 1, NAN);
        EXPECT_TRUE(testValue.hasInfOrNaN());
        SET_VALUES(1, INFINITY, NAN, 1);
        EXPECT_TRUE(testValue.hasInfOrNaN());
        SET_VALUES(1, NAN, INFINITY, 1);
        EXPECT_TRUE(testValue.hasInfOrNaN());
        SET_VALUES(NAN, 1, 1, INFINITY);
        EXPECT_TRUE(testValue.hasInfOrNaN());

        SET_VALUES(INFINITY, INFINITY, INFINITY, INFINITY);
        EXPECT_TRUE(testValue.hasInfOrNaN());

        SET_VALUES(-INFINITY, -INFINITY, -INFINITY, -INFINITY);
        EXPECT_TRUE(testValue.hasInfOrNaN());

        SET_VALUES(NAN, NAN, NAN, NAN);
        EXPECT_TRUE(testValue.hasInfOrNaN());

#undef SET_VALUES
    }

    TEST(TestMath, Abs)
    {
        FloatInVec testValues;
#define SET_VALUES(x, y, z, w) \
    testValues = FloatInVec{x, y, z, w}

        FloatInVec absValues;
#define TEST_VALUES(x, y, z, w)                                                             \
    testValues = FloatInVec{x, y, z, w};                                                    \
    absValues = FloatInVec{fabs(float(x)), fabs(float(y)), fabs(float(z)), fabs(float(w))}; \
    EXPECT_TRUE(testValues.abs() == absValues)

        TEST_VALUES(0, 0, 0, 0);

        TEST_VALUES(1, 1, 1, 1);

        TEST_VALUES(-1, -1, -1, -1);

        TEST_VALUES(-1, 1, 1, 1);
        TEST_VALUES(1, -1, 1, 1);
        TEST_VALUES(1, 1, -1, 1);
        TEST_VALUES(1, 1, 1, -1);

        TEST_VALUES(-1, -1, 1, 1);
        TEST_VALUES(1, -1, -1, 1);
        TEST_VALUES(1, 1, -1, -1);
        TEST_VALUES(1, 1, -1, -1);

        TEST_VALUES(-INFINITY, -1, 1, 1);
        TEST_VALUES(1, -INFINITY, -1, 1);
        TEST_VALUES(1, 1, -INFINITY, -1);
        TEST_VALUES(1, 1, -1, -INFINITY);

        TEST_VALUES(INFINITY, -1, 1, 1);
        TEST_VALUES(1, INFINITY, -1, 1);
        TEST_VALUES(1, 1, INFINITY, -1);
        TEST_VALUES(1, 1, -1, INFINITY);

        TEST_VALUES(-10, -1, 1, 1);
        TEST_VALUES(1, -10, -1, 1);
        TEST_VALUES(1, 1, -10, -1);
        TEST_VALUES(1, 1, -1, -10);

#undef TEST_VALUES
    }

    TEST(TestMath, Decompose)
    {
        auto testDecompose = [](float roll, float pitch, float yaw)
        {
            Matrix4 testMatrix;
            Quat testQuat;
            Vector3 decomposeTranslation;
            Quat decomposeRotation;
            Vector3 decomposeScale;

            testQuat = Quat(roll, pitch, yaw);
            testMatrix = Matrix4::rotation(testQuat);
            decompose(testMatrix, decomposeTranslation, decomposeRotation, decomposeScale);
            EXPECT_TRUE(decomposeRotation.similar(testQuat));
            EXPECT_TRUE(decomposeTranslation.similar({0, 0, 0}));
            EXPECT_TRUE(decomposeScale.similar({1, 1, 1}));
            testMatrix = Matrix4::rotation(testQuat) * Matrix4::scale({-10, -10, -1});
            decompose(testMatrix, decomposeTranslation, decomposeRotation, decomposeScale);
            EXPECT_TRUE((Matrix4::translation({0, 0, 0}) * Matrix4::rotation(decomposeRotation) * Matrix4::scale(decomposeScale)).similar(testMatrix));
            testMatrix = Matrix4::rotation(testQuat) * Matrix4::scale({-10, 10, -10});
            decompose(testMatrix, decomposeTranslation, decomposeRotation, decomposeScale);
            EXPECT_TRUE((Matrix4::translation({0, 0, 0}) * Matrix4::rotation(decomposeRotation) * Matrix4::scale(decomposeScale)).similar(testMatrix));
            testMatrix = Matrix4::rotation(testQuat) * Matrix4::scale({-10, -10, 10});
            decompose(testMatrix, decomposeTranslation, decomposeRotation, decomposeScale);
            EXPECT_TRUE((Matrix4::translation({0, 0, 0}) * Matrix4::rotation(decomposeRotation) * Matrix4::scale(decomposeScale)).similar(testMatrix));
            testMatrix = Matrix4::rotation(testQuat) * Matrix4::scale({10, -1, -10});
            decompose(testMatrix, decomposeTranslation, decomposeRotation, decomposeScale);
            EXPECT_TRUE((Matrix4::translation({0, 0, 0}) * Matrix4::rotation(decomposeRotation) * Matrix4::scale(decomposeScale)).similar(testMatrix));
            testMatrix = Matrix4::rotation(testQuat) * Matrix4::scale({10, 1, -10});
            decompose(testMatrix, decomposeTranslation, decomposeRotation, decomposeScale);
            EXPECT_TRUE((Matrix4::translation({0, 0, 0}) * Matrix4::rotation(decomposeRotation) * Matrix4::scale(decomposeScale)).similar(testMatrix));
            testMatrix = Matrix4::rotation(testQuat) * Matrix4::scale({10, -10, 10});
            decompose(testMatrix, decomposeTranslation, decomposeRotation, decomposeScale);
            EXPECT_TRUE((Matrix4::translation({0, 0, 0}) * Matrix4::rotation(decomposeRotation) * Matrix4::scale(decomposeScale)).similar(testMatrix));
            testMatrix = Matrix4::rotation(testQuat) * Matrix4::scale({-10, 1, 10}) * Matrix4::translation({10, 10, 10});
            decompose(testMatrix, decomposeTranslation, decomposeRotation, decomposeScale);
            EXPECT_TRUE((Matrix4::translation(decomposeTranslation) * Matrix4::rotation(decomposeRotation) * Matrix4::scale(decomposeScale)).similar(testMatrix));
            testMatrix = Matrix4::rotation(testQuat) * Matrix4::scale({-10, 1, 10}) * Matrix4::translation({0, 0, 10});
            decompose(testMatrix, decomposeTranslation, decomposeRotation, decomposeScale);
            EXPECT_TRUE((Matrix4::translation(decomposeTranslation) * Matrix4::rotation(decomposeRotation) * Matrix4::scale(decomposeScale)).similar(testMatrix));
            testMatrix = Matrix4::rotation(testQuat) * Matrix4::scale({-10, 1, 10}) * Matrix4::translation({0, 10, 0});
            decompose(testMatrix, decomposeTranslation, decomposeRotation, decomposeScale);
            EXPECT_TRUE((Matrix4::translation(decomposeTranslation) * Matrix4::rotation(decomposeRotation) * Matrix4::scale(decomposeScale)).similar(testMatrix));
            testMatrix = Matrix4::rotation(testQuat) * Matrix4::scale({-10, 1, 10}) * Matrix4::translation({10, 0, 0});
            decompose(testMatrix, decomposeTranslation, decomposeRotation, decomposeScale);
            EXPECT_TRUE((Matrix4::translation(decomposeTranslation) * Matrix4::rotation(decomposeRotation) * Matrix4::scale(decomposeScale)).similar(testMatrix));
            testMatrix = Matrix4::rotation(testQuat) * Matrix4::scale({-10, 1, 10}) * Matrix4::translation({10, 0, 10});
            decompose(testMatrix, decomposeTranslation, decomposeRotation, decomposeScale);
            EXPECT_TRUE((Matrix4::translation(decomposeTranslation) * Matrix4::rotation(decomposeRotation) * Matrix4::scale(decomposeScale)).similar(testMatrix));
            testMatrix = Matrix4::rotation(testQuat) * Matrix4::scale({-10, 1, 10}) * Matrix4::translation({10, 10, 0});
            decompose(testMatrix, decomposeTranslation, decomposeRotation, decomposeScale);
            EXPECT_TRUE((Matrix4::translation(decomposeTranslation) * Matrix4::rotation(decomposeRotation) * Matrix4::scale(decomposeScale)).similar(testMatrix));
            testMatrix = Matrix4::rotation(testQuat) * Matrix4::scale({-10, 1, 10}) * Matrix4::translation({0, 10, 10});
            decompose(testMatrix, decomposeTranslation, decomposeRotation, decomposeScale);
            EXPECT_TRUE((Matrix4::translation(decomposeTranslation) * Matrix4::rotation(decomposeRotation) * Matrix4::scale(decomposeScale)).similar(testMatrix));
            testMatrix = Matrix4::rotation(testQuat) * Matrix4::scale({-10, 1, 10}) * Matrix4::translation({10, -10, 10});
            decompose(testMatrix, decomposeTranslation, decomposeRotation, decomposeScale);
            EXPECT_TRUE((Matrix4::translation(decomposeTranslation) * Matrix4::rotation(decomposeRotation) * Matrix4::scale(decomposeScale)).similar(testMatrix));
            testMatrix = Matrix4::rotation(testQuat) * Matrix4::scale({-10, 1, 10}) * Matrix4::translation({0, -10, -100});
            decompose(testMatrix, decomposeTranslation, decomposeRotation, decomposeScale);
            EXPECT_TRUE((Matrix4::translation(decomposeTranslation) * Matrix4::rotation(decomposeRotation) * Matrix4::scale(decomposeScale)).similar(testMatrix));
        };
        testDecompose(0, 0, 0);
        testDecompose(0, 0, 0);

        testDecompose(1, 0, 0);
        testDecompose(1, 0, 0);
        testDecompose(0, 1, 0);
        testDecompose(0, 1, 0);
        testDecompose(1, 1, 0);
        testDecompose(1, 1, 0);
        testDecompose(0, 0, 1);
        testDecompose(0, 0, 1);
        testDecompose(1, 0, 1);
        testDecompose(1, 0, 1);
        testDecompose(0, 1, 1);
        testDecompose(0, 1, 1);
        testDecompose(1, 1, 1);
        testDecompose(1, 1, 1);

        testDecompose(-1, 0, 0);
        testDecompose(-1, 0, 0);
        testDecompose(0, -1, 0);
        testDecompose(0, -1, 0);
        testDecompose(-1, -1, 0);
        testDecompose(-1, -1, 0);
        testDecompose(0, 0, -1);
        testDecompose(0, 0, -1);
        testDecompose(-1, 0, -1);
        testDecompose(-1, 0, -1);
        testDecompose(0, -1, -1);
        testDecompose(0, -1, -1);
        testDecompose(-1, -1, -1);
        testDecompose(-1, -1, -1);

        testDecompose(-1, 1, 0);
        testDecompose(-1, 1, 0);
        testDecompose(1, 0, -1);
        testDecompose(1, 0, -1);
        testDecompose(0, 1, -1);
        testDecompose(0, 1, -1);
        testDecompose(-1, 1, -1);
        testDecompose(-1, 1, -1);

        testDecompose(0.00, 0.00, 10.00);
        testDecompose(0.00, 0.00, 10.00);
        testDecompose(0.00, 10.00, 00.00);
        testDecompose(0.00, 10.00, 00.00);
        testDecompose(0.00, 60.00, 180.00);
        testDecompose(0.00, 60.00, 180.00);
        testDecompose(0.00, 60.00, -150.00);
        testDecompose(0.00, 60.00, -150.00);
        testDecompose(0.00, 120.00, -120.00);
        testDecompose(0.00, 120.00, -120.00);
        testDecompose(0.00, 120.00, -90.00);
        testDecompose(0.00, 120.00, -90.00);
        testDecompose(60.00, -90.00, 120.00);
        testDecompose(60.00, -90.00, 120.00);
        testDecompose(60.00, -90.00, 150.00);
        testDecompose(60.00, -90.00, 150.00);
        testDecompose(60.00, -90.00, 180.00);
        testDecompose(60.00, -90.00, 180.00);
        testDecompose(120.00, -30.00, 150.00);
        testDecompose(120.00, -30.00, 150.00);
        testDecompose(120.00, -30.00, 180.00);
        testDecompose(120.00, -30.00, 180.00);
        testDecompose(180.00, 150.00, 0.00);
        testDecompose(180.00, 150.00, 0.00);
        testDecompose(180.00, 150.00, 30.00);
        testDecompose(180.00, 150.00, 30.00);
        testDecompose(180.00, 150.00, 60.00);
        testDecompose(180.00, 150.00, 60.00);
        testDecompose(180.00, 150.00, 90.00);
        testDecompose(180.00, 150.00, 90.00);
        testDecompose(-120.00, -90.00, 30.00);
        testDecompose(-120.00, -90.00, 30.00);
        testDecompose(-120.00, -90.00, 60.00);
        testDecompose(-120.00, -90.00, 60.00);
        testDecompose(-120.00, -90.00, 90.00);
        testDecompose(-120.00, -90.00, 90.00);
        testDecompose(-60.00, 180.00, -60.00);
        testDecompose(-60.00, 180.00, -60.00);
        testDecompose(-60.00, 180.00, -30.00);
        testDecompose(-60.00, 180.00, -30.00);
        testDecompose(-60.00, -150.00, 0.00);
        testDecompose(-60.00, -150.00, 0.00);
        testDecompose(-60.00, -150.00, 30.00);
        testDecompose(-60.00, -150.00, 30.00);
        testDecompose(-30.00, -30.00, 150.00);
        testDecompose(-30.00, -30.00, 150.00);
        testDecompose(-30.00, -30.00, 180.00);
        testDecompose(-30.00, -30.00, 180.00);
        testDecompose(-30.00, -30.00, -150.00);
        testDecompose(-30.00, -30.00, -150.00);
    }

}  // namespace nau::test