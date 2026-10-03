// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <span>
#include <vector>

#include "nau/scene/components/component_life_cycle.h"
#include "nau/scene/internal/scene_listener.h"
#include "nau/scene/internal/scene_manager_internal.h"
#include "nau/scene/scene_processor.h"
#include "scene_test_base.h"
#include "scene_test_components.h"

namespace nau::test
{
    class TestSceneListener : public SceneTestBase
    {
    protected:
        class MySceneListener : public scene::ISceneListener
        {
        public:
            size_t getSceneBeginCounter() const
            {
                return m_sceneBeginCounter;
            }

            size_t getSceneEndCounter() const
            {
                return m_sceneEndCounter;
            }

            size_t getDeletedObjectCounter() const
            {
                return m_deletedObjectCounter;
            }

            size_t getDeletedComponentCounter() const
            {
                return m_deletedComponentCounter;
            }

            const auto& getActivatedObjects() const
            {
                return (m_activatedObjects);
            }

            void clearActivatedObjects()
            {
                m_activatedObjects.clear();
            }

            const auto& getActivatedComponents() const
            {
                return (m_activatedComponents);
            }

            void onSceneBegin() override
            {
                ++m_sceneBeginCounter;
            }

            void onSceneEnd() override
            {
                ++m_sceneEndCounter;
            }

            void onAfterActivatingObjects(std::span<const scene::SceneObject*> objects) override
            {
                for (auto obj : objects)
                {
                    m_activatedObjects.push_back(obj);
                }
            }

            void onBeforeDeletingObjects(std::span<const scene::SceneObject*> objects) override
            {
                m_deletedObjectCounter += objects.size();
            }

            void onAfterActivatingComponents(std::span<const scene::Component*> components) override
            {
                for (auto comp : components)
                {
                    m_activatedComponents.push_back(comp);
                }
            }

            void onBeforeDeletingComponents(std::span<const scene::Component*> components) override
            {
                m_deletedComponentCounter = +components.size();
            }

            void onComponentsChange(std::span<const scene::Component*> components) override
            {
            }

            size_t m_sceneBeginCounter = 0;
            size_t m_sceneEndCounter = 0;
            size_t m_deletedObjectCounter = 0;
            size_t m_deletedComponentCounter = 0;
            std::vector<const scene::SceneObject*> m_activatedObjects;
            std::vector<const scene::Component*> m_activatedComponents;
        };

        void initializeApp() override
        {
            SceneTestBase::initializeApp();
            m_sceneListenerReg = getServiceProvider().get<scene::ISceneManagerInternal>().addSceneListener(m_sceneListener);
        }

        MySceneListener m_sceneListener;
        scene::SceneListenerRegistration m_sceneListenerReg;
    };

    /**
     */
    TEST_F(TestSceneListener, OnSceneBeginEnd)
    {
        using namespace testing;
        using namespace nau::async;

        const AssertionResult testResult = runTestApp([&]() -> Task<AssertionResult>
        {
            constexpr unsigned FrameCount = 2;
            co_await skipFrames(FrameCount);
            co_return AssertionSuccess();
        });

        ASSERT_TRUE(testResult);

        ASSERT_GT(m_sceneListener.getSceneBeginCounter(), 0);
        ASSERT_GT(m_sceneListener.getSceneEndCounter(), 0);
    }

    /**
     */
    TEST_F(TestSceneListener, OnAfterActivatingObject_OnSceneActivate)
    {
        using namespace testing;
        using namespace nau::async;
        using namespace nau::scene;
        using namespace nau::scene_test;

        using namespace std::chrono_literals;

        const AssertionResult testResult = runTestApp([this]() -> Task<AssertionResult>
        {
            IScene::Ptr scene = createEmptyScene();

            // this must not appear in 'real' code
            const SceneObject* rootObjectPtr = &scene->getRoot();

            ObjectWeakRef object = scene->getRoot().attachChild(createObject());
            co_await getSceneManager().activateScene(std::move(scene));

            ASSERT_ASYNC(m_sceneListener.getActivatedObjects().size() == 1);
            ASSERT_ASYNC(m_sceneListener.getActivatedObjects().front() == rootObjectPtr);

            co_return AssertionSuccess();
        });

        ASSERT_TRUE(testResult);
    }

    /**
     */
    TEST_F(TestSceneListener, OnAfterActivatingObject_OnObjectAttach)
    {
        using namespace testing;
        using namespace nau::async;
        using namespace nau::scene;
        using namespace nau::scene_test;

        using namespace std::chrono_literals;

        const AssertionResult testResult = runTestApp([this]() -> Task<AssertionResult>
        {
            IScene::Ptr scene = createEmptyScene();
            IScene::WeakRef sceneRef = co_await getSceneManager().activateScene(std::move(scene));

            m_sceneListener.clearActivatedObjects();

            SceneObject::Ptr object1 = createObject();
            object1->attachChild(createObject());

            ObjectWeakRef object1Ref = co_await sceneRef->getRoot().attachChildAsync(std::move(object1));
            ObjectWeakRef object12Ref = co_await object1Ref->attachChildAsync(createObject());

            ASSERT_ASYNC(m_sceneListener.getActivatedObjects().size() == 2);
            ASSERT_ASYNC(m_sceneListener.getActivatedObjects()[0] == object1Ref.get());
            ASSERT_ASYNC(m_sceneListener.getActivatedObjects()[1] == object12Ref.get());

            co_return AssertionSuccess();
        });

        ASSERT_TRUE(testResult);
    }

    /**
     */
    TEST_F(TestSceneListener, OnAfterActivatingComponents)
    {
        using namespace testing;
        using namespace nau::async;
        using namespace nau::scene;
        using namespace nau::scene_test;

        using namespace std::chrono_literals;

        const AssertionResult testResult = runTestApp([&]() -> Task<AssertionResult>
        {
            IScene::Ptr scene = createEmptyScene();

            ObjectWeakRef object = scene->getRoot().attachChild(createObject());
            co_await getSceneManager().activateScene(std::move(scene));

            ObjectWeakRef componentRef = co_await object->addComponentAsync<MyDefaultSceneComponent>();
            ASSERT_ASYNC(m_sceneListener.getActivatedComponents().size() == 1);
            ASSERT_ASYNC(m_sceneListener.getActivatedComponents().front() == componentRef.get());

            co_return AssertionSuccess();
        });

        ASSERT_TRUE(testResult);
    }

    /**
     */
    TEST_F(TestSceneListener, OnBeforeDeletingComponents)
    {
        using namespace testing;
        using namespace nau::async;
        using namespace nau::scene;
        using namespace nau::scene_test;

        using namespace std::chrono_literals;

        const AssertionResult testResult = runTestApp([this]() -> Task<AssertionResult>
        {
            IScene::Ptr scene = createEmptyScene();

            ObjectWeakRef objectRef = scene->getRoot().attachChild(createObject());
            ObjectWeakRef componentRef = objectRef->addComponent<MyDefaultSceneComponent>();

            co_await getSceneManager().activateScene(std::move(scene));

            objectRef->removeComponent(componentRef);
            ASSERT_ASYNC(m_sceneListener.getDeletedComponentCounter() == 1);

            co_return AssertionSuccess();
        });

        ASSERT_TRUE(testResult);
    }

    /**
     */
    TEST_F(TestSceneListener, OnBeforeDeletingObjects)
    {
        using namespace testing;
        using namespace nau::async;
        using namespace nau::scene;
        using namespace nau::scene_test;

        using namespace std::chrono_literals;

        const AssertionResult testResult = runTestApp([&]() -> Task<AssertionResult>
        {
            IScene::Ptr scene = createEmptyScene();

            ObjectWeakRef object = scene->getRoot().attachChild(createObject());
            ObjectWeakRef component = object->addComponent<MyDefaultSceneComponent>();

            ObjectWeakRef child_1 = object->attachChild(createObject<MyDisposableComponent>());

            ObjectWeakRef child_2 = object->attachChild(createObject<MyDisposableComponent>());

            ObjectWeakRef sceneRef = co_await getSceneManager().activateScene(std::move(scene));
            co_await skipFrames(1);

            object->removeChild(child_1);
            sceneRef->getRoot().removeChild(object);

            co_await skipFrames(2);

            ASSERT_ASYNC(m_sceneListener.getDeletedObjectCounter() == 2);

            co_return AssertionSuccess();
        });

        ASSERT_TRUE(testResult);
    }

}  // namespace nau::test
