// test_messaging.cpp
//
// Copyright (c) N-GINN LLC., 2023-2025. All rights reserved.
//

#include <vector>

#include "nau/dataBlock/dag_dataBlock.h"
#include "nau/math/math.h"
#include "nau/math/transform.h"

using namespace Vectormath;

#define EPS 0.000001f

namespace nau::math
{
    void PrintTo(const Transform& transform, std::ostream* os)
    {
        *os << std::endl
            << "t: (" << transform.getTranslation().getX() << ", " << transform.getTranslation().getY() << ", " << transform.getTranslation().getZ() << ")" << std::endl;
        *os << "s: (" << transform.getScale().getX() << ", " << transform.getScale().getY() << ", " << transform.getScale().getZ() << ")" << std::endl;
        auto rotation = transform.getRotation().toEuler();
        *os << "r: (" << rotation.getX() << ", " << rotation.getY() << ", " << rotation.getZ() << ")" << std::endl;
    }
}  // namespace nau::math

namespace nau::test
{
    TEST(TestMath, SimilarQuat)
    {
        const double BigEnoughtDiff = std::sqrt(MATH_SMALL_NUMBER) * 2;

        auto testQuat = [&](float x, float y, float z, float w)
        {
            EXPECT_TRUE(Quat(x, y, z, w).similar(Quat(x, y, z, w)));
            EXPECT_TRUE(Quat(x + MATH_SMALL_NUMBER, y, z, w).similar(Quat(x, y, z, w)));
            EXPECT_TRUE(Quat(x, y + MATH_SMALL_NUMBER, z, w).similar(Quat(x, y, z, w)));
            EXPECT_TRUE(Quat(x, y, z + MATH_SMALL_NUMBER, w + MATH_SMALL_NUMBER).similar(Quat(x, y, z, w)));
            EXPECT_TRUE(Quat(x + MATH_SMALL_NUMBER, y + MATH_SMALL_NUMBER, z + MATH_SMALL_NUMBER, w + MATH_SMALL_NUMBER).similar(Quat(x, y, z, w)));
            EXPECT_TRUE(Quat(x, y, z, w).similar(Quat(-x, -y, -z, -w)));
            EXPECT_TRUE(Quat(x + MATH_SMALL_NUMBER, y, z, w).similar(Quat(-x, -y, -z, -w)));
            EXPECT_TRUE(Quat(x, y + MATH_SMALL_NUMBER, z, w).similar(Quat(-x, -y, -z, -w)));
            EXPECT_TRUE(Quat(x, y, z + MATH_SMALL_NUMBER, w + MATH_SMALL_NUMBER).similar(Quat(-x, -y, -z, -w)));
            EXPECT_TRUE(Quat(x + MATH_SMALL_NUMBER, y + MATH_SMALL_NUMBER, z + MATH_SMALL_NUMBER, w + MATH_SMALL_NUMBER).similar(Quat(-x, -y, -z, -w)));
            EXPECT_FALSE(Quat(x + BigEnoughtDiff, y, z, w).similar(Quat(x, y, z, w)));
            EXPECT_FALSE(Quat(x, y + BigEnoughtDiff, z, w).similar(Quat(x, y, z, w)));
            EXPECT_FALSE(Quat(x, y, z + BigEnoughtDiff, w).similar(Quat(x, y, z, w)));
            EXPECT_FALSE(Quat(x, y, z, w + BigEnoughtDiff).similar(Quat(x, y, z, w)));
            EXPECT_FALSE(Quat(x + BigEnoughtDiff, y + BigEnoughtDiff, z + BigEnoughtDiff, w + BigEnoughtDiff).similar(Quat(x, y, z, w)));
            EXPECT_FALSE(Quat(x + BigEnoughtDiff, y, z, w).similar(Quat(-x, -y, -z, -w)));
            EXPECT_FALSE(Quat(x, y + BigEnoughtDiff, z, w).similar(Quat(-x, -y, -z, -w)));
            EXPECT_FALSE(Quat(x, y, z + BigEnoughtDiff, w).similar(Quat(-x, -y, -z, -w)));
            EXPECT_FALSE(Quat(x, y, z, w + BigEnoughtDiff).similar(Quat(-x, -y, -z, -w)));
            EXPECT_FALSE(Quat(x + BigEnoughtDiff, y + BigEnoughtDiff, z + BigEnoughtDiff, w + BigEnoughtDiff).similar(Quat(-x, -y, -z, -w)));
            EXPECT_TRUE(Quat(x, y, z, w).similar(Quat(x, y, z, w)));
            EXPECT_TRUE(Quat(x + 1, y, z, w).similar(Quat(x, y, z, w), 3));
            EXPECT_TRUE(Quat(x, y + 1, z, w).similar(Quat(x, y, z, w), 3));
            EXPECT_TRUE(Quat(x, y, z + 1, w + 1).similar(Quat(x, y, z, w), 3));
            EXPECT_TRUE(Quat(x + 1, y + 1, z + 1, w + 1).similar(Quat(x, y, z, w), 7));
            EXPECT_TRUE(Quat(x, y, z, w).similar(Quat(-x, -y, -z, -w), 3));
            EXPECT_TRUE(Quat(x + 1, y, z, w).similar(Quat(-x, -y, -z, -w), 3));
            EXPECT_TRUE(Quat(x, y + 1, z, w).similar(Quat(-x, -y, -z, -w), 3));
            EXPECT_TRUE(Quat(x, y, z + 1, w + 1).similar(Quat(-x, -y, -z, -w), 3));
            EXPECT_TRUE(Quat(x + 1, y + 1, z + 1, w + 1).similar(Quat(-x, -y, -z, -w), 7));
            EXPECT_FALSE(Quat(x, y, z, w).similar(Quat(x, y, z, w), 0));
            EXPECT_FALSE(Quat(x + 5, y, z, w).similar(Quat(x, y, z, w), 3));
            EXPECT_FALSE(Quat(x, y + 5, z, w).similar(Quat(x, y, z, w), 3));
            EXPECT_FALSE(Quat(x, y, z + 5, w).similar(Quat(x, y, z, w), 3));
            EXPECT_FALSE(Quat(x, y, z, w + 5).similar(Quat(x, y, z, w), 3));
            EXPECT_FALSE(Quat(x + 5, y + 5, z + 5, w + 5).similar(Quat(x, y, z, w), 3));
            EXPECT_FALSE(Quat(x + 5, y, z, w).similar(Quat(-x, -y, -z, -w), 3));
            EXPECT_FALSE(Quat(x, y + 5, z, w).similar(Quat(-x, -y, -z, -w), 3));
            EXPECT_FALSE(Quat(x, y, z + 5, w).similar(Quat(-x, -y, -z, -w), 3));
            EXPECT_FALSE(Quat(x, y, z, w + 5).similar(Quat(-x, -y, -z, -w), 3));
            EXPECT_FALSE(Quat(x + 5, y + 5, z + 5, w + 5).similar(Quat(-x, -y, -z, -w), 3));
        };

        testQuat(0, 0, 0, 1);

        Quat qvalue = {};

        auto testQuat2 = [&](float roll, float pitch, float yaw)
        {
            Quat qvalue = Quat{roll, pitch, yaw};
            testQuat(qvalue.getX(), qvalue.getY(), qvalue.getZ(), qvalue.getW());
        };

        testQuat2(0, 0, 0);
        testQuat2(0.00, 60.00, 180.00);
        testQuat2(0.00, 60.00, -150.00);
        testQuat2(0.00, 120.00, -120.00);
        testQuat2(0.00, 120.00, -90.00);
        testQuat2(60.00, -90.00, 120.00);
        testQuat2(60.00, -90.00, 150.00);
        testQuat2(60.00, -90.00, 180.00);
        testQuat2(120.00, -30.00, 150.00);
        testQuat2(120.00, -30.00, 180.00);
        testQuat2(180.00, 150.00, 0.00);
        testQuat2(180.00, 150.00, 30.00);
        testQuat2(180.00, 150.00, 60.00);
        testQuat2(180.00, 150.00, 90.00);
        testQuat2(-120.00, -90.00, 30.00);
        testQuat2(-120.00, -90.00, 60.00);
        testQuat2(-120.00, -90.00, 90.00);
        testQuat2(-60.00, 180.00, -60.00);
        testQuat2(-60.00, 180.00, -30.00);
        testQuat2(-60.00, -150.00, 0.00);
        testQuat2(-60.00, -150.00, 30.00);
        testQuat2(-30.00, -30.00, 150.00);
        testQuat2(-30.00, -30.00, 180.00);
        testQuat2(-30.00, -30.00, -150.00);
    }

    TEST(TestMath, SimilarVector3)
    {
        const double BigEnoughtDiff = std::sqrt(MATH_SMALL_NUMBER) * 2;

        auto testVector3 = [&](float x, float y, float z)
        {
            EXPECT_TRUE(Vector3(x, y, z).similar(Vector3(x, y, z)));
            EXPECT_TRUE(Vector3(x + MATH_SMALL_NUMBER, y, z).similar(Vector3(x, y, z)));
            EXPECT_TRUE(Vector3(x, y + MATH_SMALL_NUMBER, z).similar(Vector3(x, y, z)));
            EXPECT_TRUE(Vector3(x, y, z + MATH_SMALL_NUMBER).similar(Vector3(x, y, z)));
            EXPECT_TRUE(Vector3(x + MATH_SMALL_NUMBER, y + MATH_SMALL_NUMBER, z + MATH_SMALL_NUMBER).similar(Vector3(x, y, z)));
            EXPECT_FALSE(Vector3(x + BigEnoughtDiff, y, z).similar(Vector3(x, y, z)));
            EXPECT_FALSE(Vector3(x, y + BigEnoughtDiff, z).similar(Vector3(x, y, z)));
            EXPECT_FALSE(Vector3(x, y, z + BigEnoughtDiff).similar(Vector3(x, y, z)));
            EXPECT_FALSE(Vector3(x + BigEnoughtDiff, y + BigEnoughtDiff, z + BigEnoughtDiff).similar(Vector3(x, y, z)));
            EXPECT_TRUE(Vector3(x + 1, y, z).similar(Vector3(x, y, z), 3));
            EXPECT_TRUE(Vector3(x, y + 1, z).similar(Vector3(x, y, z), 3));
            EXPECT_TRUE(Vector3(x, y, z + 1).similar(Vector3(x, y, z), 3));
            EXPECT_TRUE(Vector3(x + 1, y + 1, z + 1).similar(Vector3(x, y, z), 7));
            EXPECT_FALSE(Vector3(x, y, z).similar(Vector3(x, y, z), 0));
            EXPECT_FALSE(Vector3(x + 5, y, z).similar(Vector3(x, y, z), 3));
            EXPECT_FALSE(Vector3(x, y + 5, z).similar(Vector3(x, y, z), 3));
            EXPECT_FALSE(Vector3(x, y, z + 5).similar(Vector3(x, y, z), 3));
            EXPECT_FALSE(Vector3(x + 5, y + 5, z + 5).similar(Vector3(x, y, z), 3));
        };

        testVector3(0, 0, 0);

        Vector3 vector = {};

        const std::vector<float> multipliersList = {
            0.000001,
            0.00001,
            0.0001,
            0.001,
            0.01,
            0.1,
            1,
            10,
            100};

        auto testVector3Loop = [&](float x, float y, float z)
        {
            for (auto scale : multipliersList)
            {
                vector = Vector3{x, y, z} * scale;
                testVector3(vector.getX(), vector.getY(), vector.getZ());
            }
        };

        testVector3Loop(0, 0, 0);

        testVector3Loop(1, 0, 0);
        testVector3Loop(0, 1, 0);
        testVector3Loop(1, 1, 0);
        testVector3Loop(0, 0, 1);
        testVector3Loop(1, 0, 1);
        testVector3Loop(0, 1, 1);
        testVector3Loop(1, 1, 1);

        testVector3Loop(-1, 0, 0);
        testVector3Loop(0, -1, 0);
        testVector3Loop(-1, -1, 0);
        testVector3Loop(0, 0, -1);
        testVector3Loop(-1, 0, -1);
        testVector3Loop(0, -1, -1);
        testVector3Loop(-1, -1, -1);

        testVector3Loop(-1, 1, 0);
        testVector3Loop(1, 0, -1);
        testVector3Loop(0, 1, -1);
        testVector3Loop(-1, 1, -1);

        testVector3Loop(0.00, 60.00, 180.00);
        testVector3Loop(0.00, 60.00, -150.00);
        testVector3Loop(0.00, 120.00, -120.00);
        testVector3Loop(0.00, 120.00, -90.00);
        testVector3Loop(60.00, -90.00, 120.00);
        testVector3Loop(60.00, -90.00, 150.00);
        testVector3Loop(60.00, -90.00, 180.00);
        testVector3Loop(120.00, -30.00, 150.00);
        testVector3Loop(120.00, -30.00, 180.00);
        testVector3Loop(180.00, 150.00, 0.00);
        testVector3Loop(180.00, 150.00, 30.00);
        testVector3Loop(180.00, 150.00, 60.00);
        testVector3Loop(180.00, 150.00, 90.00);
        testVector3Loop(-120.00, -90.00, 30.00);
        testVector3Loop(-120.00, -90.00, 60.00);
        testVector3Loop(-120.00, -90.00, 90.00);
        testVector3Loop(-60.00, 180.00, -60.00);
        testVector3Loop(-60.00, 180.00, -30.00);
        testVector3Loop(-60.00, -150.00, 0.00);
        testVector3Loop(-60.00, -150.00, 30.00);
        testVector3Loop(-30.00, -30.00, 150.00);
        testVector3Loop(-30.00, -30.00, 180.00);
        testVector3Loop(-30.00, -30.00, -150.00);
    }

    TEST(TestMath, SimilarPoint3)
    {
        const double BigEnoughtDiff = std::sqrt(MATH_SMALL_NUMBER) * 2;

        auto testPoint3 = [&](float x, float y, float z)
        {
            EXPECT_TRUE(Point3(x, y, z).similar(Point3(x, y, z)));
            EXPECT_TRUE(Point3(x + MATH_SMALL_NUMBER, y, z).similar(Point3(x, y, z)));
            EXPECT_TRUE(Point3(x, y + MATH_SMALL_NUMBER, z).similar(Point3(x, y, z)));
            EXPECT_TRUE(Point3(x, y, z + MATH_SMALL_NUMBER).similar(Point3(x, y, z)));
            EXPECT_TRUE(Point3(x + MATH_SMALL_NUMBER, y + MATH_SMALL_NUMBER, z + MATH_SMALL_NUMBER).similar(Point3(x, y, z)));
            EXPECT_FALSE(Point3(x + BigEnoughtDiff, y, z).similar(Point3(x, y, z)));
            EXPECT_FALSE(Point3(x, y + BigEnoughtDiff, z).similar(Point3(x, y, z)));
            EXPECT_FALSE(Point3(x, y, z + BigEnoughtDiff).similar(Point3(x, y, z)));
            EXPECT_FALSE(Point3(x + BigEnoughtDiff, y + BigEnoughtDiff, z + BigEnoughtDiff).similar(Point3(x, y, z)));
            EXPECT_TRUE(Point3(x + 1, y, z).similar(Point3(x, y, z), 3));
            EXPECT_TRUE(Point3(x, y + 1, z).similar(Point3(x, y, z), 3));
            EXPECT_TRUE(Point3(x, y, z + 1).similar(Point3(x, y, z), 3));
            EXPECT_TRUE(Point3(x + 1, y + 1, z + 1).similar(Point3(x, y, z), 7));
            EXPECT_FALSE(Point3(x, y, z).similar(Point3(x, y, z), 0));
            EXPECT_FALSE(Point3(x + 5, y, z).similar(Point3(x, y, z), 3));
            EXPECT_FALSE(Point3(x, y + 5, z).similar(Point3(x, y, z), 3));
            EXPECT_FALSE(Point3(x, y, z + 5).similar(Point3(x, y, z), 3));
            EXPECT_FALSE(Point3(x + 5, y + 5, z + 5).similar(Point3(x, y, z), 3));
        };

        testPoint3(0, 0, 0);

        Point3 point = {};

        const std::vector<float> multipliersList = {
            0.000001,
            0.00001,
            0.0001,
            0.001,
            0.01,
            0.1,
            1,
            10,
            100};

        auto testPoint3Loop = [&](float x, float y, float z)
        {
            for (auto scale : multipliersList)
            {
                point = Point3{float(x * scale), float(y * scale), float(z * scale)};
                testPoint3(point.getX(), point.getY(), point.getZ());
            }
        };

        testPoint3Loop(0, 0, 0);

        testPoint3Loop(1, 0, 0);
        testPoint3Loop(0, 1, 0);
        testPoint3Loop(1, 1, 0);
        testPoint3Loop(0, 0, 1);
        testPoint3Loop(1, 0, 1);
        testPoint3Loop(0, 1, 1);
        testPoint3Loop(1, 1, 1);

        testPoint3Loop(-1, 0, 0);
        testPoint3Loop(0, -1, 0);
        testPoint3Loop(-1, -1, 0);
        testPoint3Loop(0, 0, -1);
        testPoint3Loop(-1, 0, -1);
        testPoint3Loop(0, -1, -1);
        testPoint3Loop(-1, -1, -1);

        testPoint3Loop(-1, 1, 0);
        testPoint3Loop(1, 0, -1);
        testPoint3Loop(0, 1, -1);
        testPoint3Loop(-1, 1, -1);

        testPoint3Loop(0.00, 60.00, 180.00);
        testPoint3Loop(0.00, 60.00, -150.00);
        testPoint3Loop(0.00, 120.00, -120.00);
        testPoint3Loop(0.00, 120.00, -90.00);
        testPoint3Loop(60.00, -90.00, 120.00);
        testPoint3Loop(60.00, -90.00, 150.00);
        testPoint3Loop(60.00, -90.00, 180.00);
        testPoint3Loop(120.00, -30.00, 150.00);
        testPoint3Loop(120.00, -30.00, 180.00);
        testPoint3Loop(180.00, 150.00, 0.00);
        testPoint3Loop(180.00, 150.00, 30.00);
        testPoint3Loop(180.00, 150.00, 60.00);
        testPoint3Loop(180.00, 150.00, 90.00);
        testPoint3Loop(-120.00, -90.00, 30.00);
        testPoint3Loop(-120.00, -90.00, 60.00);
        testPoint3Loop(-120.00, -90.00, 90.00);
        testPoint3Loop(-60.00, 180.00, -60.00);
        testPoint3Loop(-60.00, 180.00, -30.00);
        testPoint3Loop(-60.00, -150.00, 0.00);
        testPoint3Loop(-60.00, -150.00, 30.00);
        testPoint3Loop(-30.00, -30.00, 150.00);
        testPoint3Loop(-30.00, -30.00, 180.00);
        testPoint3Loop(-30.00, -30.00, -150.00);
    }

    TEST(TestMath, SimilarVector4)
    {
        const double BigEnoughtDiff = std::sqrt(MATH_SMALL_NUMBER) * 2;

        auto testVector4 = [&](float x, float y, float z, float w)
        {
            EXPECT_TRUE(Vector4(x, y, z, w).similar(Vector4(x, y, z, w)));
            EXPECT_TRUE(Vector4(x + MATH_SMALL_NUMBER, y, z, w).similar(Vector4(x, y, z, w)));
            EXPECT_TRUE(Vector4(x, y + MATH_SMALL_NUMBER, z, w).similar(Vector4(x, y, z, w)));
            EXPECT_TRUE(Vector4(x, y, z + MATH_SMALL_NUMBER, w).similar(Vector4(x, y, z, w)));
            EXPECT_TRUE(Vector4(x, y, z, w + MATH_SMALL_NUMBER).similar(Vector4(x, y, z, w)));
            EXPECT_TRUE(Vector4(x + MATH_SMALL_NUMBER, y + MATH_SMALL_NUMBER, z + MATH_SMALL_NUMBER, w + MATH_SMALL_NUMBER).similar(Vector4(x, y, z, w)));
            EXPECT_FALSE(Vector4(x + BigEnoughtDiff, y, z, w).similar(Vector4(x, y, z, w)));
            EXPECT_FALSE(Vector4(x, y + BigEnoughtDiff, z, w).similar(Vector4(x, y, z, w)));
            EXPECT_FALSE(Vector4(x, y, z + BigEnoughtDiff, w).similar(Vector4(x, y, z, w)));
            EXPECT_FALSE(Vector4(x, y, z, w + BigEnoughtDiff).similar(Vector4(x, y, z, w)));
            EXPECT_FALSE(Vector4(x + BigEnoughtDiff, y + BigEnoughtDiff, z + BigEnoughtDiff, w + BigEnoughtDiff).similar(Vector4(x, y, z, w)));
            EXPECT_TRUE(Vector4(x, y, z, w).similar(Vector4(x, y, z, w), 3));
            EXPECT_TRUE(Vector4(x + 1, y, z, w).similar(Vector4(x, y, z, w), 3));
            EXPECT_TRUE(Vector4(x, y + 1, z, w).similar(Vector4(x, y, z, w), 3));
            EXPECT_TRUE(Vector4(x, y, z + 1, w).similar(Vector4(x, y, z, w), 3));
            EXPECT_TRUE(Vector4(x, y, z, w + 1).similar(Vector4(x, y, z, w), 3));
            EXPECT_TRUE(Vector4(x + 1, y + 1, z + 1, w + 1).similar(Vector4(x, y, z, w), 7));
            EXPECT_FALSE(Vector4(x, y, z, w).similar(Vector4(x, y, z, w), 0));
            EXPECT_FALSE(Vector4(x + 10, y, z, w).similar(Vector4(x, y, z, w), 2));
            EXPECT_FALSE(Vector4(x, y + 10, z, w).similar(Vector4(x, y, z, w), 2));
            EXPECT_FALSE(Vector4(x, y, z + 10, w).similar(Vector4(x, y, z, w), 2));
            EXPECT_FALSE(Vector4(x, y, z, w + 10).similar(Vector4(x, y, z, w), 2));
            EXPECT_FALSE(Vector4(x + 10, y + 10, z + 10, w + 10).similar(Vector4(x, y, z, w), 2));
        };

        testVector4(0, 0, 0, 0);

        EXPECT_FALSE(Vector4(0, 0, 0, 0).similar(Vector4(0, 0, 0, 0), 0));

        Vector4 vector = {};

        const std::vector<float> multipliersList = {
            0.000001,
            0.00001,
            0.0001,
            0.001,
            0.01,
            0.1,
            1,
            10,
            100};

        auto testVector4Loop = [&](float x, float y, float z, float w)
        {
            for (auto scale : multipliersList)
            {
                vector = Vector4{x, y, z, w} * scale;
                testVector4(vector.getX(), vector.getY(), vector.getZ(), vector.getW());
            }
        };

        testVector4Loop(0, 0, 0, 0);

        testVector4Loop(1, 0, 0, 0);
        testVector4Loop(0, 1, 0, 0);
        testVector4Loop(1, 1, 0, 0);
        testVector4Loop(0, 0, 1, 0);
        testVector4Loop(1, 0, 1, 0);
        testVector4Loop(0, 1, 1, 0);
        testVector4Loop(1, 1, 1, 0);
        testVector4Loop(1, 0, 0, 1);
        testVector4Loop(0, 1, 0, 1);
        testVector4Loop(1, 1, 0, 1);
        testVector4Loop(0, 0, 1, 1);
        testVector4Loop(1, 0, 1, 1);
        testVector4Loop(0, 1, 1, 1);
        testVector4Loop(1, 1, 1, 1);

        testVector4Loop(-1, 0, 0, 1);
        testVector4Loop(0, -1, 0, 1);
        testVector4Loop(-1, -1, 0, 1);
        testVector4Loop(0, 0, -1, 1);
        testVector4Loop(-1, 0, -1, 1);
        testVector4Loop(0, -1, -1, 1);
        testVector4Loop(-1, -1, -1, 1);

        testVector4Loop(-1, 1, 0, 1);
        testVector4Loop(1, 0, -1, 1);
        testVector4Loop(0, 1, -1, 1);
        testVector4Loop(-1, 1, -1, 1);

        testVector4Loop(0.00, 60.00, 180.00, -150.00);
        testVector4Loop(0.00, 60.00, -150.00, -150.00);
        testVector4Loop(0.00, 120.00, -120.00, -150.00);
        testVector4Loop(0.00, 120.00, -90.00, -150.00);
        testVector4Loop(60.00, -90.00, 120.00, -150.00);
        testVector4Loop(60.00, -90.00, 150.00, -150.00);
        testVector4Loop(60.00, -90.00, 180.00, -150.00);
        testVector4Loop(120.00, -30.00, 150.00, -150.00);
        testVector4Loop(120.00, -30.00, 180.00, -150.00);
        testVector4Loop(180.00, 150.00, 0.00, -150.00);
        testVector4Loop(180.00, 150.00, 30.00, -150.00);
        testVector4Loop(180.00, 150.00, 60.00, -150.00);
        testVector4Loop(180.00, 150.00, 90.00, -150.00);
        testVector4Loop(-120.00, -90.00, 30.00, -150.00);
        testVector4Loop(-120.00, -90.00, 60.00, -150.00);
        testVector4Loop(-120.00, -90.00, 90.00, -150.00);
        testVector4Loop(-60.00, 180.00, -60.00, -150.00);
        testVector4Loop(-60.00, 180.00, -30.00, -150.00);
        testVector4Loop(-60.00, -150.00, 0.00, -150.00);
        testVector4Loop(-60.00, -150.00, 30.00, -150.00);
        testVector4Loop(-30.00, -30.00, 150.00, -150.00);
        testVector4Loop(-30.00, -30.00, 180.00, -150.00);
        testVector4Loop(-30.00, -30.00, -150.00, -150.00);
    }

    TEST(TestMath, SimilarMatrix4)
    {
        const double BigEnoughtDiff = std::sqrt(MATH_SMALL_NUMBER) * 2;

        auto testMatrix4 = [&](float x, float y, float z, float w)
        {
            EXPECT_TRUE(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w))));
            EXPECT_TRUE(Matrix4(Vector4(x + MATH_SMALL_NUMBER, y, z, w), Vector4(x + MATH_SMALL_NUMBER, y, z, w), Vector4(x + MATH_SMALL_NUMBER, y, z, w), Vector4(x + MATH_SMALL_NUMBER, y, z, w)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w))));
            EXPECT_TRUE(Matrix4(Vector4(x, y + MATH_SMALL_NUMBER, z, w), Vector4(x, y + MATH_SMALL_NUMBER, z, w), Vector4(x, y + MATH_SMALL_NUMBER, z, w), Vector4(x, y + MATH_SMALL_NUMBER, z, w)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w))));
            EXPECT_TRUE(Matrix4(Vector4(x, y, z + MATH_SMALL_NUMBER, w), Vector4(x, y, z + MATH_SMALL_NUMBER, w), Vector4(x, y, z + MATH_SMALL_NUMBER, w), Vector4(x, y, z + MATH_SMALL_NUMBER, w)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w))));
            EXPECT_TRUE(Matrix4(Vector4(x, y, z, w + MATH_SMALL_NUMBER), Vector4(x, y, z, w + MATH_SMALL_NUMBER), Vector4(x, y, z, w + MATH_SMALL_NUMBER), Vector4(x, y, z, w + MATH_SMALL_NUMBER)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w))));
            EXPECT_TRUE(Matrix4(Vector4(x + MATH_SMALL_NUMBER, y + MATH_SMALL_NUMBER, z + MATH_SMALL_NUMBER, w + MATH_SMALL_NUMBER), Vector4(x + MATH_SMALL_NUMBER, y + MATH_SMALL_NUMBER, z + MATH_SMALL_NUMBER, w + MATH_SMALL_NUMBER), Vector4(x + MATH_SMALL_NUMBER, y + MATH_SMALL_NUMBER, z + MATH_SMALL_NUMBER, w + MATH_SMALL_NUMBER), Vector4(x + MATH_SMALL_NUMBER, y + MATH_SMALL_NUMBER, z + MATH_SMALL_NUMBER, w + MATH_SMALL_NUMBER)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w))));
            EXPECT_FALSE(Matrix4(Vector4(x + BigEnoughtDiff, y, z, w), Vector4(x + BigEnoughtDiff, y, z, w), Vector4(x + BigEnoughtDiff, y, z, w), Vector4(x + BigEnoughtDiff, y, z, w)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w))));
            EXPECT_FALSE(Matrix4(Vector4(x, y + BigEnoughtDiff, z, w), Vector4(x, y + BigEnoughtDiff, z, w), Vector4(x, y + BigEnoughtDiff, z, w), Vector4(x, y + BigEnoughtDiff, z, w)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w))));
            EXPECT_FALSE(Matrix4(Vector4(x, y, z + BigEnoughtDiff, w), Vector4(x, y, z + BigEnoughtDiff, w), Vector4(x, y, z + BigEnoughtDiff, w), Vector4(x, y, z + BigEnoughtDiff, w)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w))));
            EXPECT_FALSE(Matrix4(Vector4(x, y, z, w + BigEnoughtDiff), Vector4(x, y, z, w + BigEnoughtDiff), Vector4(x, y, z, w + BigEnoughtDiff), Vector4(x, y, z, w + BigEnoughtDiff)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w))));
            EXPECT_FALSE(Matrix4(Vector4(x + BigEnoughtDiff, y + BigEnoughtDiff, z + BigEnoughtDiff, w + BigEnoughtDiff), Vector4(x + BigEnoughtDiff, y + BigEnoughtDiff, z + BigEnoughtDiff, w + BigEnoughtDiff), Vector4(x + BigEnoughtDiff, y + BigEnoughtDiff, z + BigEnoughtDiff, w + BigEnoughtDiff), Vector4(x + BigEnoughtDiff, y + BigEnoughtDiff, z + BigEnoughtDiff, w + BigEnoughtDiff)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w))));
            EXPECT_TRUE(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w)), 3));
            EXPECT_TRUE(Matrix4(Vector4(x + 1, y, z, w), Vector4(x + 1, y, z, w), Vector4(x + 1, y, z, w), Vector4(x + 1, y, z, w)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w)), 3));
            EXPECT_TRUE(Matrix4(Vector4(x, y + 1, z, w), Vector4(x, y + 1, z, w), Vector4(x, y + 1, z, w), Vector4(x, y + 1, z, w)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w)), 3));
            EXPECT_TRUE(Matrix4(Vector4(x, y, z + 1, w), Vector4(x, y, z + 1, w), Vector4(x, y, z + 1, w), Vector4(x, y, z + 1, w)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w)), 3));
            EXPECT_TRUE(Matrix4(Vector4(x, y, z, w + 1), Vector4(x, y, z, w + 1), Vector4(x, y, z, w + 1), Vector4(x, y, z, w + 1)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w)), 3));
            EXPECT_TRUE(Matrix4(Vector4(x + 1, y + 1, z + 1, w + 1), Vector4(x + 1, y + 1, z + 1, w + 1), Vector4(x + 1, y + 1, z + 1, w + 1), Vector4(x + 1, y + 1, z + 1, w + 1)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w)), 7));
            EXPECT_FALSE(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w)), 0));
            EXPECT_FALSE(Matrix4(Vector4(x + 10, y, z, w), Vector4(x + 10, y, z, w), Vector4(x + 10, y, z, w), Vector4(x + 10, y, z, w)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w)), 2));
            EXPECT_FALSE(Matrix4(Vector4(x, y + 10, z, w), Vector4(x, y + 10, z, w), Vector4(x, y + 10, z, w), Vector4(x, y + 10, z, w)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w)), 2));
            EXPECT_FALSE(Matrix4(Vector4(x, y, z + 10, w), Vector4(x, y, z + 10, w), Vector4(x, y, z + 10, w), Vector4(x, y, z + 10, w)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w)), 2));
            EXPECT_FALSE(Matrix4(Vector4(x, y, z, w + 10), Vector4(x, y, z, w + 10), Vector4(x, y, z, w + 10), Vector4(x, y, z, w + 10)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w)), 2));
            EXPECT_FALSE(Matrix4(Vector4(x + 10, y + 10, z + 10, w + 10), Vector4(x + 10, y + 10, z + 10, w + 10), Vector4(x + 10, y + 10, z + 10, w + 10), Vector4(x + 10, y + 10, z + 10, w + 10)).similar(Matrix4(Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w), Vector4(x, y, z, w)), 2));
        };

        testMatrix4(0, 0, 0, 0);

        testMatrix4(1, 0, 0, 0);
        testMatrix4(0, 1, 0, 0);
        testMatrix4(1, 1, 0, 0);
        testMatrix4(0, 0, 1, 0);
        testMatrix4(1, 0, 1, 0);
        testMatrix4(0, 1, 1, 0);
        testMatrix4(1, 1, 1, 0);
        testMatrix4(1, 0, 0, 1);
        testMatrix4(0, 1, 0, 1);
        testMatrix4(1, 1, 0, 1);
        testMatrix4(0, 0, 1, 1);
        testMatrix4(1, 0, 1, 1);
        testMatrix4(0, 1, 1, 1);
        testMatrix4(1, 1, 1, 1);
    }

    TEST(TestMath, SimilarNewTransform)
    {
        using namespace nau::math;
        const float BigEnoughtDiff = std::sqrt(MATH_SMALL_NUMBER) * 2;

        auto testTransform = [&](
                                 float tx, float ty, float tz,
                                 float roll, float pitch, float yaw,
                                 float sx, float sy, float sz)
        {
            EXPECT_TRUE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_TRUE(Transform({roll, pitch, yaw}, {tx + MATH_SMALL_NUMBER, ty, tz}, {sx, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_TRUE(Transform({roll, pitch, yaw}, {tx, ty + MATH_SMALL_NUMBER, tz}, {sx, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_TRUE(Transform({roll, pitch, yaw}, {tx, ty, tz + MATH_SMALL_NUMBER}, {sx, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_TRUE(Transform({roll, pitch, yaw}, {tx + MATH_SMALL_NUMBER, ty + MATH_SMALL_NUMBER, tz + MATH_SMALL_NUMBER}, {sx, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx + BigEnoughtDiff, ty, tz}, {sx, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx, ty + BigEnoughtDiff, tz}, {sx, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx, ty, tz + BigEnoughtDiff}, {sx, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx + BigEnoughtDiff, ty + BigEnoughtDiff, tz + BigEnoughtDiff}, {sx, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_TRUE(Transform({roll, pitch, yaw}, {tx + 1, ty, tz}, {sx, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_TRUE(Transform({roll, pitch, yaw}, {tx, ty + 1, tz}, {sx, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_TRUE(Transform({roll, pitch, yaw}, {tx, ty, tz + 1}, {sx, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_TRUE(Transform({roll, pitch, yaw}, {tx + 1, ty + 1, tz + 1}, {sx, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}), 7));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}), 0));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx + 5, ty, tz}, {sx, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx, ty + 5, tz}, {sx, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx, ty, tz + 5}, {sx, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx + 5, ty + 5, tz + 5}, {sx, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}), 3));

            auto quat = Quat{roll, pitch, yaw};
            float qx = quat.getX();
            float qy = quat.getY();
            float qz = quat.getZ();
            float qw = quat.getW();

            EXPECT_TRUE(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_TRUE(Transform({qx + MATH_SMALL_NUMBER, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_TRUE(Transform({qx, qy + MATH_SMALL_NUMBER, qz, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_TRUE(Transform({qx, qy, qz + MATH_SMALL_NUMBER, qw + MATH_SMALL_NUMBER}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_TRUE(Transform({qx + MATH_SMALL_NUMBER, qy + MATH_SMALL_NUMBER, qz + MATH_SMALL_NUMBER, qw + MATH_SMALL_NUMBER}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_TRUE(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({-qx, -qy, -qz, -qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_TRUE(Transform({qx + MATH_SMALL_NUMBER, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({-qx, -qy, -qz, -qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_TRUE(Transform({qx, qy + MATH_SMALL_NUMBER, qz, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({-qx, -qy, -qz, -qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_TRUE(Transform({qx, qy, qz + MATH_SMALL_NUMBER, qw + MATH_SMALL_NUMBER}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({-qx, -qy, -qz, -qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_TRUE(Transform({qx + MATH_SMALL_NUMBER, qy + MATH_SMALL_NUMBER, qz + MATH_SMALL_NUMBER, qw + MATH_SMALL_NUMBER}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({-qx, -qy, -qz, -qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({qx + BigEnoughtDiff, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({qx, qy + BigEnoughtDiff, qz, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({qx, qy, qz + BigEnoughtDiff, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({qx, qy, qz, qw + BigEnoughtDiff}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({qx + BigEnoughtDiff, qy + BigEnoughtDiff, qz + BigEnoughtDiff, qw + BigEnoughtDiff}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({qx + BigEnoughtDiff, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({-qx, -qy, -qz, -qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({qx, qy + BigEnoughtDiff, qz, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({-qx, -qy, -qz, -qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({qx, qy, qz + BigEnoughtDiff, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({-qx, -qy, -qz, -qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({qx, qy, qz, qw + BigEnoughtDiff}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({-qx, -qy, -qz, -qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({qx + BigEnoughtDiff, qy + BigEnoughtDiff, qz + BigEnoughtDiff, qw + BigEnoughtDiff}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({-qx, -qy, -qz, -qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_TRUE(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_TRUE(Transform({qx + 1, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_TRUE(Transform({qx, qy + 1, qz, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_TRUE(Transform({qx, qy, qz + 1, qw + 1}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_TRUE(Transform({qx + 1, qy + 1, qz + 1, qw + 1}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}), 7));
            EXPECT_TRUE(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({-qx, -qy, -qz, -qw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_TRUE(Transform({qx + 1, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({-qx, -qy, -qz, -qw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_TRUE(Transform({qx, qy + 1, qz, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({-qx, -qy, -qz, -qw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_TRUE(Transform({qx, qy, qz + 1, qw + 1}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({-qx, -qy, -qz, -qw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_TRUE(Transform({qx + 1, qy + 1, qz + 1, qw + 1}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({-qx, -qy, -qz, -qw}, {tx, ty, tz}, {sx, sy, sz}), 7));
            EXPECT_FALSE(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}), 0));
            EXPECT_FALSE(Transform({qx + 5, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_FALSE(Transform({qx, qy + 5, qz, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_FALSE(Transform({qx, qy, qz + 5, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_FALSE(Transform({qx, qy, qz, qw + 5}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_FALSE(Transform({qx + 5, qy + 5, qz + 5, qw + 5}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_FALSE(Transform({qx + 5, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({-qx, -qy, -qz, -qw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_FALSE(Transform({qx, qy + 5, qz, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({-qx, -qy, -qz, -qw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_FALSE(Transform({qx, qy, qz + 5, qw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({-qx, -qy, -qz, -qw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_FALSE(Transform({qx, qy, qz, qw + 5}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({-qx, -qy, -qz, -qw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_FALSE(Transform({qx + 5, qy + 5, qz + 5, qw + 5}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({-qx, -qy, -qz, -qw}, {tx, ty, tz}, {sx, sy, sz}), 3));

            EXPECT_TRUE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_TRUE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx + MATH_SMALL_NUMBER, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_TRUE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy + MATH_SMALL_NUMBER, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_TRUE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz + MATH_SMALL_NUMBER}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_TRUE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx + MATH_SMALL_NUMBER, sy + MATH_SMALL_NUMBER, sz + MATH_SMALL_NUMBER}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx + BigEnoughtDiff, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy + BigEnoughtDiff, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz + BigEnoughtDiff}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz + BigEnoughtDiff}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx + BigEnoughtDiff, sy + BigEnoughtDiff, sz + BigEnoughtDiff}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_TRUE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx + 1, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_TRUE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy + 1, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_TRUE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz + 1}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_TRUE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx + 1, sy + 1, sz + 1}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}), 7));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}), 0));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx + 5, sy, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy + 5, sz}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz + 5}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}), 3));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx + 5, sy + 5, sz + 5}).similar(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}), 3));

            EXPECT_TRUE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}) == (Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_NE(Transform({roll, pitch, yaw}, {tx + 1, ty, tz}, {sx, sy, sz}), (Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx, ty + 1, tz}, {sx, sy, sz}) == (Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx, ty, tz + 1}, {sx, sy, sz}) == (Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx + 1, ty + 1, tz + 1}, {sx, sy, sz}) == (Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));

            EXPECT_TRUE(Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}) == (Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({qx + 1, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz}) == (Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({qx, qy + 1, qz, qw}, {tx, ty, tz}, {sx, sy, sz}) == (Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({qx, qy, qz + 1, qw + 1}, {tx, ty, tz}, {sx, sy, sz}) == (Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({qx + 1, qy + 1, qz + 1, qw + 1}, {tx, ty, tz}, {sx, sy, sz}) == (Transform({qx, qy, qz, qw}, {tx, ty, tz}, {sx, sy, sz})));

            EXPECT_TRUE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz}) == (Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx + 1, sy, sz}) == (Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy + 1, sz}) == (Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
            EXPECT_FALSE(Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz + 1}) == (Transform({roll, pitch, yaw}, {tx, ty, tz}, {sx, sy, sz})));
        };

        testTransform(0.000000000, 0.000000000, 0.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 0.00, 0.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 30.00, 20.00, 10.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 10.00, 0.00, 0.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, -10.00, -10.00, -10.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, -30.00, 20.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 0.00, 10.00, 0.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 1.00, -10.00, 1.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 0.00, 0.00, 0.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 0.00, 0.00, 10.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 1.00, 1.00, -10.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 10.00, 0.00, 0.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, -10.00, -10.00, -10.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 30.00, 20.00, 10.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 0.00, 10.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 1.00, -10.00, 1.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, -30.00, 20.00, 10.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 0.00, 0.00, 10.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 1.00, 1.00, -10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, -10.00, -10.00, -10.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 30.00, 20.00, 10.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 10.00, 0.00, 0.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 1.00, -10.00, 1.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, -30.00, 20.00, 10.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 0.00, 10.00, 0.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 1.00, 1.00, -10.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 0.00, 0.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(0.000000000, 0.000000000, 0.000000000, 30.00, 20.00, 10.00, 1.000000000, -10.000000000, 1.000000000);

        testTransform(10.000000000, 0.000000000, 0.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 0.00, 0.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 30.00, 20.00, 10.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 10.00, 0.00, 0.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, -10.00, -10.00, -10.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, -30.00, 20.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 0.00, 10.00, 0.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 1.00, -10.00, 1.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 0.00, 0.00, 0.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 0.00, 0.00, 10.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 1.00, 1.00, -10.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 10.00, 0.00, 0.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, -10.00, -10.00, -10.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 30.00, 20.00, 10.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 0.00, 10.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 1.00, -10.00, 1.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, -30.00, 20.00, 10.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 0.00, 0.00, 10.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 1.00, 1.00, -10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, -10.00, -10.00, -10.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 30.00, 20.00, 10.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 10.00, 0.00, 0.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 1.00, -10.00, 1.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, -30.00, 20.00, 10.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 0.00, 10.00, 0.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 1.00, 1.00, -10.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 0.00, 0.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(10.000000000, 0.000000000, 0.000000000, 30.00, 20.00, 10.00, 1.000000000, -10.000000000, 1.000000000);

        testTransform(0.000000000, 10.000000000, 0.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 0.00, 0.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 30.00, 20.00, 10.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 10.00, 0.00, 0.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, -10.00, -10.00, -10.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, -30.00, 20.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 0.00, 10.00, 0.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 1.00, -10.00, 1.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 0.00, 0.00, 0.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 0.00, 0.00, 10.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 1.00, 1.00, -10.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 10.00, 0.00, 0.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, -10.00, -10.00, -10.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 30.00, 20.00, 10.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 0.00, 10.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 1.00, -10.00, 1.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, -30.00, 20.00, 10.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 0.00, 0.00, 10.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 1.00, 1.00, -10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, -10.00, -10.00, -10.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 30.00, 20.00, 10.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 10.00, 0.00, 0.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 1.00, -10.00, 1.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, -30.00, 20.00, 10.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 0.00, 10.00, 0.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 1.00, 1.00, -10.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 0.00, 0.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(0.000000000, 10.000000000, 0.000000000, 30.00, 20.00, 10.00, 1.000000000, -10.000000000, 1.000000000);

        testTransform(0.000000000, 0.000000000, 10.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 0.00, 0.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 30.00, 20.00, 10.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 10.00, 0.00, 0.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, -10.00, -10.00, -10.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, -30.00, 20.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 0.00, 10.00, 0.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 1.00, -10.00, 1.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 0.00, 0.00, 0.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 0.00, 0.00, 10.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 1.00, 1.00, -10.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 10.00, 0.00, 0.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, -10.00, -10.00, -10.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 30.00, 20.00, 10.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 0.00, 10.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 1.00, -10.00, 1.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, -30.00, 20.00, 10.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 0.00, 0.00, 10.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 1.00, 1.00, -10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, -10.00, -10.00, -10.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 30.00, 20.00, 10.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 10.00, 0.00, 0.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 1.00, -10.00, 1.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, -30.00, 20.00, 10.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 0.00, 10.00, 0.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 1.00, 1.00, -10.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 0.00, 0.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(0.000000000, 0.000000000, 10.000000000, 30.00, 20.00, 10.00, 1.000000000, -10.000000000, 1.000000000);

        testTransform(-10.000000000, -10.000000000, -10.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 0.00, 0.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 30.00, 20.00, 10.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 10.00, 0.00, 0.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, -10.00, -10.00, -10.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, -30.00, 20.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 0.00, 10.00, 0.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 1.00, -10.00, 1.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 0.00, 0.00, 0.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 0.00, 0.00, 10.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 1.00, 1.00, -10.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 10.00, 0.00, 0.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, -10.00, -10.00, -10.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 30.00, 20.00, 10.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 0.00, 10.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 1.00, -10.00, 1.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, -30.00, 20.00, 10.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 0.00, 0.00, 10.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 1.00, 1.00, -10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, -10.00, -10.00, -10.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 30.00, 20.00, 10.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 10.00, 0.00, 0.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 1.00, -10.00, 1.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, -30.00, 20.00, 10.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 0.00, 10.00, 0.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 1.00, 1.00, -10.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 0.00, 0.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(-10.000000000, -10.000000000, -10.000000000, 30.00, 20.00, 10.00, 1.000000000, -10.000000000, 1.000000000);

        testTransform(1.000000000, -10.000000000, 1.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 0.00, 0.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 30.00, 20.00, 10.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 10.00, 0.00, 0.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, -10.00, -10.00, -10.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, -30.00, 20.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 0.00, 10.00, 0.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 1.00, -10.00, 1.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 0.00, 0.00, 0.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 0.00, 0.00, 10.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 1.00, 1.00, -10.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 10.00, 0.00, 0.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, -10.00, -10.00, -10.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 30.00, 20.00, 10.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 0.00, 10.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 1.00, -10.00, 1.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, -30.00, 20.00, 10.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 0.00, 0.00, 10.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 1.00, 1.00, -10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, -10.00, -10.00, -10.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 30.00, 20.00, 10.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 10.00, 0.00, 0.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 1.00, -10.00, 1.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, -30.00, 20.00, 10.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 0.00, 10.00, 0.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 1.00, 1.00, -10.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 0.00, 0.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(1.000000000, -10.000000000, 1.000000000, 30.00, 20.00, 10.00, 1.000000000, -10.000000000, 1.000000000);

        testTransform(1.000000000, 1.000000000, -10.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 0.00, 0.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 30.00, 20.00, 10.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 10.00, 0.00, 0.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, -10.00, -10.00, -10.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, -30.00, 20.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 0.00, 10.00, 0.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 1.00, -10.00, 1.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 0.00, 0.00, 0.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 0.00, 0.00, 10.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 1.00, 1.00, -10.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 10.00, 0.00, 0.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, -10.00, -10.00, -10.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 30.00, 20.00, 10.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 0.00, 10.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 1.00, -10.00, 1.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, -30.00, 20.00, 10.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 0.00, 0.00, 10.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 1.00, 1.00, -10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, -10.00, -10.00, -10.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 30.00, 20.00, 10.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 10.00, 0.00, 0.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 1.00, -10.00, 1.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, -30.00, 20.00, 10.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 0.00, 10.00, 0.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 1.00, 1.00, -10.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 0.00, 0.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(1.000000000, 1.000000000, -10.000000000, 30.00, 20.00, 10.00, 1.000000000, -10.000000000, 1.000000000);

        testTransform(30.000000000, 20.000000000, 10.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 0.00, 0.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 30.00, 20.00, 10.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 10.00, 0.00, 0.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, -10.00, -10.00, -10.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, -30.00, 20.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 0.00, 10.00, 0.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 1.00, -10.00, 1.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 0.00, 0.00, 0.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 0.00, 0.00, 10.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 1.00, 1.00, -10.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 10.00, 0.00, 0.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, -10.00, -10.00, -10.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 30.00, 20.00, 10.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 0.00, 10.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 1.00, -10.00, 1.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, -30.00, 20.00, 10.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 0.00, 0.00, 10.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 1.00, 1.00, -10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, -10.00, -10.00, -10.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 30.00, 20.00, 10.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 10.00, 0.00, 0.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 1.00, -10.00, 1.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, -30.00, 20.00, 10.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 0.00, 10.00, 0.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 1.00, 1.00, -10.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 0.00, 0.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(30.000000000, 20.000000000, 10.000000000, 30.00, 20.00, 10.00, 1.000000000, -10.000000000, 1.000000000);

        testTransform(-30.000000000, 20.000000000, 10.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 0.00, 0.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 30.00, 20.00, 10.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 10.00, 0.00, 0.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, -10.00, -10.00, -10.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, -30.00, 20.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 0.00, 10.00, 0.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 1.00, -10.00, 1.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 0.00, 0.00, 0.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 0.00, 0.00, 10.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 1.00, 1.00, -10.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 10.00, 0.00, 0.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, -10.00, -10.00, -10.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 30.00, 20.00, 10.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 0.00, 10.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 1.00, -10.00, 1.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, -30.00, 20.00, 10.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 0.00, 0.00, 10.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 1.00, 1.00, -10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, -10.00, -10.00, -10.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 30.00, 20.00, 10.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 10.00, 0.00, 0.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 1.00, -10.00, 1.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, -30.00, 20.00, 10.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 0.00, 10.00, 0.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 1.00, 1.00, -10.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 0.00, 0.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(-30.000000000, 20.000000000, 10.000000000, 30.00, 20.00, 10.00, 1.000000000, -10.000000000, 1.000000000);

        testTransform(50.000000000, 100.000000000, -1000.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 0.00, 0.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 30.00, 20.00, 10.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 10.00, 0.00, 0.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, -10.00, -10.00, -10.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, -30.00, 20.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 0.00, 10.00, 0.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 1.00, -10.00, 1.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 0.00, 0.00, 0.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 0.00, 0.00, 10.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 1.00, 1.00, -10.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 10.00, 0.00, 0.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, -10.00, -10.00, -10.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 30.00, 20.00, 10.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 0.00, 10.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 1.00, -10.00, 1.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, -30.00, 20.00, 10.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 0.00, 0.00, 10.00, 10.000000000, 0.000000000, 0.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 1.00, 1.00, -10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, -10.00, -10.00, -10.00, 0.000000000, 10.000000000, 0.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 30.00, 20.00, 10.00, 30.000000000, 20.000000000, 10.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 10.00, 0.00, 0.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 1.00, -10.00, 1.00, 0.000000000, 0.000000000, 10.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, -30.00, 20.00, 10.00, -30.000000000, 20.000000000, 10.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 0.00, 10.00, 0.00, 1.000000000, -10.000000000, 1.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 1.00, 1.00, -10.00, -10.000000000, -10.000000000, -10.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 0.00, 0.00, 0.00, 0.000000000, 0.000000000, 0.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 0.00, 0.00, 10.00, 1.000000000, 1.000000000, -10.000000000);
        testTransform(50.000000000, 100.000000000, -1000.000000000, 30.00, 20.00, 10.00, 1.000000000, -10.000000000, 1.000000000);
    }

}  // namespace nau::test