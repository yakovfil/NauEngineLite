// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <string>

#include "nau/serialization/json_utils.h"
#include "scene_test_base.h"
#include "scene_test_components.h"

namespace nau::test
{
    /**
     */
    class TestSceneTransform : public SceneTestBase
    {
    };

    /**
     */
    TEST_F(TestSceneTransform, IdentityByDefault)
    {
        using namespace nau::scene;

        SceneObject::Ptr object = createObject();
        ASSERT_EQ(object->getTransform(), math::Transform::identity());
        ASSERT_EQ(object->getWorldTransform(), math::Transform::identity());
    }

    /**
     */
    TEST_F(TestSceneTransform, TransformAppliedToChildren)
    {
        using namespace nau::math;
        using namespace nau::scene;

        const vec3 offset{10, 5, 10};
        const quat rotation = quat::rotationY(1.5708);

        SceneObject::Ptr parent1 = createObject();
        ObjectWeakRef child1 = parent1->attachChild(createObject());
        ObjectWeakRef child2 = child1->attachChild(createObject());

        parent1->setTransform(Transform{rotation, offset, vec3::one()});

        ASSERT_TRUE(child1->getWorldTransform().similar(parent1->getTransform()));
        ASSERT_TRUE(child2->getWorldTransform().similar(parent1->getTransform()));
    }

    /**
     */
    TEST_F(TestSceneTransform, WorldTranslation)
    {
        using namespace nau::math;
        using namespace nau::scene;

        const vec3 ParentWorldPos{10, 0, 0};
        const vec3 Child1WorldPos{10, 10, 0};
        const vec3 Child2WorldPos{10, 10, 10};

        SceneObject::Ptr parent1 = createObject();
        parent1->setTranslation(ParentWorldPos);

        ObjectWeakRef child1 = parent1->attachChild(createObject());
        ObjectWeakRef child2 = child1->attachChild(createObject());

        child1->setWorldTransform(Transform{quat::identity(), Child1WorldPos});
        ASSERT_TRUE(child1->getTranslation().similar(Child1WorldPos - ParentWorldPos));

        child2->setWorldTransform(Transform{quat::identity(), Child2WorldPos});
        ASSERT_TRUE(child2->getTranslation().similar(Child2WorldPos - Child1WorldPos, 0.01f));
    }

    /**
     */
    TEST_F(TestSceneTransform, DirectChangeNotification)
    {
        using namespace nau::math;
        using namespace nau::scene;

        SceneObject::Ptr object = createObject();

        bool isChanged = false;
        auto subscription = object->getRootComponent().subscribeOnChanges([&isChanged](const RuntimeValue&, std::string_view)
        {
            isChanged = true;
        });

        object->setTranslation({1, 1, 1});
        ASSERT_TRUE(std::exchange(isChanged, false));

        object->setRotation(quat::rotationX(1.f));
        ASSERT_TRUE(std::exchange(isChanged, false));

        object->setScale({2, 2, 2});
        ASSERT_TRUE(std::exchange(isChanged, false));

        object->setTransform(Transform::identity());
        ASSERT_TRUE(std::exchange(isChanged, false));

        object->setWorldTransform(Transform{
            quat::identity(),
            {1, 1, 1}
        });
        ASSERT_TRUE(std::exchange(isChanged, false));
    }

    /**
     */
    TEST_F(TestSceneTransform, ChangeNotificationFromParent)
    {
        using namespace nau::math;
        using namespace nau::scene;

        SceneObject::Ptr object = createObject();
        ObjectWeakRef child1 = object->attachChild(createObject());
        ObjectWeakRef child2 = child1->attachChild(createObject());

        bool isChanged = false;
        auto subscription = child2->getRootComponent().subscribeOnChanges([&isChanged](const RuntimeValue&, std::string_view)
        {
            isChanged = true;
        });

        object->setTranslation({1, 1, 1});
        ASSERT_TRUE(std::exchange(isChanged, false));

        object->setRotation(quat::rotationX(1.f));
        ASSERT_TRUE(std::exchange(isChanged, false));

        object->setScale({2, 2, 2});
        ASSERT_TRUE(std::exchange(isChanged, false));

        object->setTransform(Transform::identity());
        ASSERT_TRUE(std::exchange(isChanged, false));

        object->setWorldTransform(Transform{
            quat::identity(),
            {1, 1, 1}
        });
        ASSERT_TRUE(std::exchange(isChanged, false));
    }

    /**
     */
    TEST_F(TestSceneTransform, Serialization)
    {
        using namespace nau::math;
        using namespace nau::scene;
        using namespace nau::serialization;

        SceneObject::Ptr object1 = createObject();
        object1->setTransform(Transform{
            quat::rotationX(0.5f),
            {10, 10, 10},
            { 2,  3,  4}
        });

        std::string jsonString;
        {
            io::InplaceStringWriter<char> writer{jsonString};
            jsonWrite(writer, nau::Ptr{object1->getRootComponent().as<RuntimeValue*>()}, JsonSettings{}).ignore();
        }

        SceneObject::Ptr object2 = createObject();

        {
            auto parseResult = jsonParseString({reinterpret_cast<const char8_t*>(jsonString.data()), jsonString.size()});
            RuntimeValue::assign(nau::Ptr{object2->getRootComponent().as<RuntimeValue*>()}, *parseResult).ignore();
        }

        ASSERT_TRUE(object1->getTransform().similar(object2->getTransform()));
    }

    /**
        TEST:
            Change the parent of an object.
            Check that the world transform remains the same.
     */
    TEST_F(TestSceneTransform, ChangeParent)
    {
        using namespace nau::math;
        using namespace nau::scene;

        SceneObject::Ptr object1 = createObject();
        object1->setTransform(Transform{
            quat::rotationX(0.5f),
            {10, 10, 10},
            { 2,  3,  4}
        });

        ObjectWeakRef child1 = object1->attachChild(createObject());
        ObjectWeakRef child2 = child1->attachChild(createObject());
        child1->setTranslation({0, -2, 0});
        child2->setTranslation({0, 2, 0});

        SceneObject::Ptr object2 = createObject();
        object2->setTransform(Transform{
            quat::rotationY(-0.5f),
            {-10, -10, -10},
        });

        const auto child1InitialWorldTransform = child1->getWorldTransform();
        const auto child2InitialWorldTransform = child2->getWorldTransform();

        child1->setParent(*object2);

        ASSERT_TRUE(child1->getWorldTransform().similar(child1InitialWorldTransform));
        ASSERT_TRUE(child2->getWorldTransform().similar(child2InitialWorldTransform));
    }

    /**
        TEST:
            Change the parent of an object with specified SetParentOpts::DontKeepWorldTransform option.
            Make sure that the local transform remains the same, but the world transform changes (the internal transform cache was reset).
     */
    TEST_F(TestSceneTransform, ChangeParentDontKeepWorldTransform)
    {
        using namespace nau::math;
        using namespace nau::scene;

        SceneObject::Ptr object1 = createObject();
        object1->setTransform(Transform{
            quat::rotationX(0.5f),
            {10, 10, 10},
            { 2,  3,  4}
        });

        ObjectWeakRef child1 = object1->attachChild(createObject());
        ObjectWeakRef child2 = child1->attachChild(createObject());
        child1->setTranslation({0, -2, 0});
        child2->setTranslation({0, 2, 0});

        SceneObject::Ptr object2 = createObject();
        object2->setTransform(Transform{
            quat::rotationY(-0.5f),
            {-10, -10, -10},
        });

        const auto child1InitialTransform = child1->getTransform();
        const auto child2InitialTransform = child2->getTransform();


        const auto child1InitialWorldTransform = child1->getWorldTransform();
        const auto child2InitialWorldTransform = child2->getWorldTransform();

        child1->setParent(*object2, SetParentOpts::DontKeepWorldTransform);

        ASSERT_TRUE(child1->getTransform().similar(child1InitialTransform));
        ASSERT_TRUE(child2->getTransform().similar(child2InitialTransform));
        ASSERT_FALSE(child1->getWorldTransform().similar(child1InitialWorldTransform));
        ASSERT_FALSE(child2->getWorldTransform().similar(child2InitialWorldTransform));
    }

}  // namespace nau::test
