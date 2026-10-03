// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <string>
#include <string_view>

#include "nau/utils/functor.h"
#include "scene_test_base.h"

namespace nau::test
{
    namespace
    {
        struct NAU_ABSTRACT_TYPE ISomeInterface : virtual IRttiObject
        {
            NAU_INTERFACE(nau::test::ISomeInterface, IRttiObject)

            virtual std::string getName() const = 0;
        };

        class CustomObject : public scene::NauObject,
                             public ISomeInterface
        {
            NAU_OBJECT(CustomObject, scene::NauObject, ISomeInterface)
        public:
            ~CustomObject()
            {
                if (m_destructorCallback)
                {
                    m_destructorCallback();
                }
            }

            template <typename F>
            void setOnDestructorCallback(F callback)
            {
                m_destructorCallback = callback;
            }

            std::string getName() const override
            {
                return m_name;
            }

            void setName(std::string_view name)
            {
                m_name = name;
            }

        private:
            Functor<void()> m_destructorCallback;

            std::string m_name;
        };
    }  // namespace

    /**
     */
    class TestNauObject : public SceneTestBase
    {
    };

    TEST_F(TestNauObject, ObjectPtr_Traits)
    {
        using namespace nau::scene;

        static_assert(std::is_move_constructible_v<ObjectUniquePtr<CustomObject>>);
        static_assert(std::is_move_assignable_v<ObjectUniquePtr<CustomObject>>);
        static_assert(!std::is_copy_constructible_v<ObjectUniquePtr<CustomObject>>);
        static_assert(!std::is_copy_assignable_v<ObjectUniquePtr<CustomObject>>);
    }

    TEST_F(TestNauObject, ObjectPtr_EmptyByDefault)
    {
        scene::ObjectUniquePtr<scene::NauObject> objectPtr;
        scene::ObjectUniquePtr<scene::NauObject> objectPtr2 = nullptr;

        ASSERT_FALSE(objectPtr);
        ASSERT_FALSE(objectPtr2);
    }

    TEST_F(TestNauObject, ObjectPtr_ConstructDestruct)
    {
        using namespace nau::scene;
        bool destructorCalled = false;

        {
            ObjectUniquePtr<CustomObject> ptr{NauObject::classCreateInstance<CustomObject>()};
            ASSERT_TRUE(ptr);

            ptr->setOnDestructorCallback([&destructorCalled]
            {
                destructorCalled = true;
            });
        }

        ASSERT_TRUE(destructorCalled);
    }

    TEST_F(TestNauObject, ObjectPtr_MoveConstruct)
    {
        using namespace nau::scene;

        ObjectUniquePtr<CustomObject> ptr0{NauObject::classCreateInstance<CustomObject>()};

        auto ptr1 = std::move(ptr0);

        ASSERT_FALSE(ptr0);
        ASSERT_TRUE(ptr1);
    }

    TEST_F(TestNauObject, ObjectPtr_MoveAssign)
    {
        using namespace nau::scene;

        ObjectUniquePtr<CustomObject> ptr0{NauObject::classCreateInstance<CustomObject>()};

        ObjectUniquePtr<CustomObject> ptr1;
        ptr1 = std::move(ptr0);

        ASSERT_FALSE(ptr0);
        ASSERT_TRUE(ptr1);
    }

    TEST_F(TestNauObject, ObjectPtr_Conversion)
    {
        using namespace nau::scene;

        ObjectUniquePtr<CustomObject> ptr0{NauObject::classCreateInstance<CustomObject>()};
        ObjectUniquePtr<NauObject> ptr1 = std::move(ptr0);

        ASSERT_FALSE(ptr0);
        ASSERT_TRUE(ptr1);
    }

    TEST_F(TestNauObject, ObjectPtr_CrossCastConversion)
    {
        using namespace nau::scene;

        {
            ObjectUniquePtr<ISomeInterface> objectApi = NauObject::classCreateInstance<CustomObject>();
            ASSERT_TRUE(objectApi);

            ObjectUniquePtr<CustomObject> object = std::move(objectApi);
            ASSERT_TRUE(object);
            ASSERT_FALSE(objectApi);
        }

        {
            ObjectUniquePtr object = NauObject::classCreateInstance<CustomObject>();
            object->setName("Test_1");

            ObjectUniquePtr<CustomObject> objectApi = std::move(object);
            ASSERT_FALSE(object);
            ASSERT_TRUE(objectApi);
            ASSERT_EQ(objectApi->getName(), "Test_1");
        }
    }

    TEST_F(TestNauObject, ObjectRef_Traits)
    {
        using namespace nau::scene;

        // static_assert(std::is_move_constructible_v<ObjectWeakRef<CustomObject>>);
        // static_assert(std::is_move_assignable_v<ObjectWeakRef<CustomObject>>);
        static_assert(std::is_copy_constructible_v<ObjectWeakRef<CustomObject>>);
        static_assert(std::is_copy_assignable_v<ObjectWeakRef<CustomObject>>);
    }

    TEST_F(TestNauObject, ObjectRef_EmptyByDefault)
    {
        scene::ObjectWeakRef<scene::NauObject> objectRef;
        scene::ObjectWeakRef<scene::NauObject> objectRef2 = nullptr;

        ASSERT_FALSE(objectRef);
        ASSERT_FALSE(objectRef2);
    }

    TEST_F(TestNauObject, ObjectRef_NullOnObjectReset)
    {
        using namespace nau::scene;

        ObjectUniquePtr<CustomObject> ptr0{NauObject::classCreateInstance<CustomObject>()};

        ObjectWeakRef ref0 = *ptr0;
        ASSERT_TRUE(ref0);

        ptr0.reset();
        ASSERT_FALSE(ref0);
    }

    TEST_F(TestNauObject, ObjectRef_CopyConstruct)
    {
        using namespace nau::scene;

        ObjectUniquePtr<CustomObject> ptr0{NauObject::classCreateInstance<CustomObject>()};

        ObjectWeakRef ref0 = *ptr0;
        ASSERT_TRUE(ref0);

        ObjectWeakRef ref1 = ref0;
        ASSERT_TRUE(ref0);
        ASSERT_TRUE(ref1);

        ptr0.reset();
        ASSERT_FALSE(ref0);
        ASSERT_FALSE(ref1);
    }

    TEST_F(TestNauObject, ObjectRef_CopyAssign)
    {
        using namespace nau::scene;

        ObjectUniquePtr<CustomObject> ptr0{NauObject::classCreateInstance<CustomObject>()};

        ObjectWeakRef ref0 = *ptr0;
        ASSERT_TRUE(ref0);

        ObjectWeakRef<CustomObject> ref1;
        ref1 = ref0;
        ASSERT_TRUE(ref0);
        ASSERT_TRUE(ref1);

        ptr0.reset();
        ASSERT_FALSE(ref0);
        ASSERT_FALSE(ref1);
    }

    TEST_F(TestNauObject, ObjectRef_ConversionOnConstructFromObject)
    {
        using namespace nau::scene;

        ObjectUniquePtr<CustomObject> ptr0{NauObject::classCreateInstance<CustomObject>()};

        ObjectWeakRef<scene::NauObject> ref0 = *ptr0;
        ASSERT_TRUE(ref0);
    }

    TEST_F(TestNauObject, ObjectRef_ConversionOnAssignFromObject)
    {
        using namespace nau::scene;

        ObjectUniquePtr<CustomObject> ptr0{NauObject::classCreateInstance<CustomObject>()};

        ObjectWeakRef<scene::NauObject> ref0;
        ref0 = *ptr0;
        ASSERT_TRUE(ref0);
    }

    TEST_F(TestNauObject, ObjectRef_ConversionOnConstructFromRef)
    {
        using namespace nau::scene;

        ObjectUniquePtr<CustomObject> ptr0{NauObject::classCreateInstance<CustomObject>()};

        ObjectWeakRef<CustomObject> ref0 = *ptr0;
        ASSERT_TRUE(ref0);

        ObjectWeakRef<NauObject> ref1 = ref0;
        ASSERT_TRUE(ref1);

        ObjectWeakRef<NauObject> ref2 = ref1;
        ASSERT_TRUE(ref2);
    }

    TEST_F(TestNauObject, ObjectRef_ConversionOnAssignFromRef)
    {
        using namespace nau::scene;

        ObjectUniquePtr<CustomObject> ptr0{NauObject::classCreateInstance<CustomObject>()};

        ObjectWeakRef<CustomObject> ref0 = *ptr0;
        ASSERT_TRUE(ref0);

        ObjectWeakRef<NauObject> ref1;
        ref1 = ref0;
        ASSERT_TRUE(ref1);

        ObjectWeakRef<NauObject> ref2;
        ref2 = std::move(ref0);
        ASSERT_TRUE(ref2);
    }

    TEST_F(TestNauObject, ObjectRef_CrossCastConversionOnConsruct)
    {
        using namespace nau::scene;

        {
            ObjectUniquePtr<ISomeInterface> objectApi = NauObject::classCreateInstance<CustomObject>();
            ASSERT_TRUE(objectApi);

            ObjectWeakRef<CustomObject> objectRef = *objectApi;
            ASSERT_TRUE(objectRef);

            objectApi.reset();
            ASSERT_FALSE(objectRef);
        }

        {
            ObjectUniquePtr object = NauObject::classCreateInstance<CustomObject>();
            object->setName("Test_1");

            ObjectWeakRef<CustomObject> objectRef = *object;
            ASSERT_TRUE(objectRef);
            ASSERT_EQ(objectRef->getName(), "Test_1");

            object.reset();
            ASSERT_FALSE(objectRef);
        }
    }

    TEST_F(TestNauObject, ObjectRef_CrossCastConversionOnAssign)
    {
        using namespace nau::scene;

        {
            ObjectUniquePtr<ISomeInterface> objectApi = NauObject::classCreateInstance<CustomObject>();
            ASSERT_TRUE(objectApi);

            ObjectWeakRef<CustomObject> objectRef;
            objectRef = *objectApi;
            ASSERT_TRUE(objectRef);

            objectApi.reset();
            ASSERT_FALSE(objectRef);
        }

        {
            ObjectUniquePtr object = NauObject::classCreateInstance<CustomObject>();
            object->setName("Test_1");

            ObjectWeakRef<CustomObject> objectRef;
            objectRef = *object;
            ASSERT_TRUE(objectRef);
            ASSERT_EQ(objectRef->getName(), "Test_1");

            object.reset();
            ASSERT_FALSE(objectRef);
        }
    }

}  // namespace nau::test
