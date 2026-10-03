// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// test_error.cpp

#include "nau/dataBlock/dag_dataBlock.h"
#include "nau/math/math.h"

namespace nau::test
{
    void TestTypes(DataBlock& block)
    {

        using namespace nau::math;
#define TEST_TYPE_FUNCTION(CppType, ApiName, testValue) \
    block.set##ApiName(#CppType, testValue);\
    EXPECT_TRUE(block.get##ApiName(#CppType) == testValue);\

        block.setStr("string_t", "testValue");
        EXPECT_TRUE(nau::string(block.getStr("string_t")) == "testValue");

        block.setReal("float", 9.87);
        EXPECT_TRUE(abs(block.getReal("float") - 9.87) < 0.00001);

        TEST_TYPE_FUNCTION(int, Int, 5)
        TEST_TYPE_FUNCTION(nau::math::E3DCOLOR, E3dcolor, nau::math::E3DCOLOR(1, 1, 5))
        TEST_TYPE_FUNCTION(int64_t, Int64, 7)
        TEST_TYPE_FUNCTION(bool, Bool, false)
        TEST_TYPE_FUNCTION(bool, Bool, true)
        Vectormath::Vector2 testVector2{4, 6};
        TEST_TYPE_FUNCTION(Vector2, Point2, testVector2)
        Vectormath::Vector3 testVector3{4, 6, 6};
        TEST_TYPE_FUNCTION(Vector3, Point3, testVector3)
        Vectormath::Vector4 testVector4{4, 6, 6, 6};
        TEST_TYPE_FUNCTION(:Vector4, Point4, testVector4)
        Vectormath::IVector2 testIVector2{4, 6};
        TEST_TYPE_FUNCTION(IVector2, IPoint2,testIVector2)
        Vectormath::IVector3 testIVector3{4, 6, 93};
        TEST_TYPE_FUNCTION(IVector3, IPoint3,testIVector3)
        Vectormath::Matrix4 testMatrix4 = Vectormath::Matrix4::rotation(1,{1,0,0});
        TEST_TYPE_FUNCTION(Matrix4, Tm,testMatrix4)

    }

    TEST(TestDataBlock, BaseTypes)
    {
        DataBlock block;
        TestTypes(block);
    }

}  // namespace nau::test
