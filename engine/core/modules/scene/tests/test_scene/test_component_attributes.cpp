// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <span>

#include "nau/scene/components/camera_component.h"
#include "nau/scene/components/component_attributes.h"
#include "nau/scene/components/internal/component_internal_attributes.h"
#include "nau/scene/components/static_mesh_component.h"
#include "nau/service/service_provider.h"
#include "scene_test_base.h"
#include "scene_test_components.h"

namespace nau::test
{
    class TestComponentAttributes : public SceneTestBase
    {
    protected:
        template <rtti::WithTypeInfo T>
        static IClassDescriptor::Ptr findClass(std::span<const IClassDescriptor::Ptr> classes)
        {
            for (auto& classDesc : classes)
            {
                if (classDesc->hasInterface<T>())
                {
                    return classDesc;
                }
            }

            return nullptr;
        }
    };

    /**
     */
    TEST_F(TestComponentAttributes, GetComponentClasses)
    {
        auto componentClasses = getServiceProvider().findClasses<scene::Component>();
        ASSERT_TRUE(!componentClasses.empty());
        ASSERT_TRUE(findClass<scene::Component>(componentClasses) != nullptr);
        ASSERT_TRUE(findClass<scene::SceneComponent>(componentClasses) != nullptr);
        ASSERT_TRUE(findClass<scene::StaticMeshComponent>(componentClasses) != nullptr);
        ASSERT_TRUE(findClass<scene::CameraComponent>(componentClasses) != nullptr);
    }

    /**
     */
    TEST_F(TestComponentAttributes, HasSystemComponentAttribute)
    {
        auto componentClasses = getServiceProvider().findClasses<scene::Component>();

        {
            IClassDescriptor::Ptr classDesc = findClass<scene::SceneComponent>(componentClasses);
            const meta::IRuntimeAttributeContainer* attributes = classDesc->getClassAttributes();
            ASSERT_TRUE(attributes);
            ASSERT_TRUE(attributes->contains<scene::SystemComponentAttrib>());
        }

        {
            IClassDescriptor::Ptr classDesc = findClass<scene::CameraComponent>(componentClasses);
            const meta::IRuntimeAttributeContainer* attributes = classDesc->getClassAttributes();
            ASSERT_TRUE(attributes);
            ASSERT_TRUE(attributes->contains<scene::SystemComponentAttrib>());
        }

        {
            IClassDescriptor::Ptr classDesc = findClass<scene::StaticMeshComponent>(componentClasses);
            const meta::IRuntimeAttributeContainer* attributes = classDesc->getClassAttributes();
            ASSERT_TRUE(attributes);
            ASSERT_TRUE(attributes->contains<scene::SystemComponentAttrib>());
        }
    }

    /**
    */
    TEST_F(TestComponentAttributes, HasNoSystemComponentAttribute)
    {
        registerClasses<scene_test::MyDisposableComponent, scene_test::MyDefaultSceneComponent>();

        auto componentClasses = getServiceProvider().findClasses<scene::Component>();
        ASSERT_TRUE(findClass<scene_test::MyDisposableComponent>(componentClasses) != nullptr);
        ASSERT_TRUE(findClass<scene_test::MyDefaultSceneComponent>(componentClasses) != nullptr);

        {
            IClassDescriptor::Ptr classDesc = findClass<scene_test::MyDisposableComponent>(componentClasses);
            const meta::IRuntimeAttributeContainer* attributes = classDesc->getClassAttributes();
            ASSERT_TRUE(attributes);
            ASSERT_FALSE(attributes->contains<scene::SystemComponentAttrib>());
        }

        {
            IClassDescriptor::Ptr classDesc = findClass<scene_test::MyDefaultSceneComponent>(componentClasses);
            const meta::IRuntimeAttributeContainer* attributes = classDesc->getClassAttributes();
            ASSERT_TRUE(attributes);
            ASSERT_FALSE(attributes->contains<scene::SystemComponentAttrib>());
        }

    }

}  // namespace nau::test
