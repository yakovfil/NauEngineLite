// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <span>

#include "nau/scene/scene_processor.h"
#include "scene_test_base.h"
#include "scene_test_components.h"

namespace nau::test
{
    /**
     */
    class MyAsyncProcessor final : public scene::ISceneProcessor,
                                   public scene::IComponentsAsyncActivator
    {
        NAU_RTTI_CLASS(MyAsyncProcessor, scene::ISceneProcessor, scene::IComponentsAsyncActivator)

    public:
        size_t getActivateCounter() const
        {
            return m_activateCounter;
        }

        size_t getDeactivateCounter() const
        {
            return m_deactivateCounter;
        }

        void setAsyncDeactivation(bool asyncDeactivation)
        {
            m_doAsyncDeactivation = asyncDeactivation;
        }

    private:
        async::Task<> activateComponentsAsync([[maybe_unused]] Uid worldUid, std::span<const scene::Component*> components, [[maybe_unused]] async::Task<> barrier) override
        {
            m_activateCounter += components.size();
            return async::makeResolvedTask();
        }

        async::Task<> deactivateComponentsAsync([[maybe_unused]] Uid worldUid, std::span<const scene::DeactivatedComponentData> components) override
        {
            m_deactivateCounter += components.size();

            if (m_doAsyncDeactivation)
            {
                co_await async::Executor::getDefault();
            }
        }

        void syncSceneState() override
        {
        }

        size_t m_activateCounter = 0;
        size_t m_deactivateCounter = 0;
        bool m_doAsyncDeactivation = false;
    };

    /**
     */
    class TestActiveScene : public SceneTestBase
    {
    private:
        void initializeApp() override
        {
            registerServices<MyAsyncProcessor>();
            SceneTestBase::initializeApp();
        }
    };


    TEST_F(TestActiveScene, HasNoActiveScenesByDefault)
    {
        auto scenes = getSceneManager().getActiveScenes();
        ASSERT_TRUE(scenes.empty());
    }

    /**
        Test:
     */
    TEST_F(TestActiveScene, SimpleActivateScene)
    {
        using namespace testing;
        using namespace nau::async;
        using namespace nau::scene;

        const AssertionResult testResult = runTestApp([]() -> Task<AssertionResult>
        {
            IScene::Ptr scene = createEmptyScene();
            ObjectWeakRef sceneRef = co_await getSceneManager().activateScene(std::move(scene));

            ASSERT_FALSE_ASYNC(scene);
            ASSERT_ASYNC(sceneRef);

            auto scenes = getSceneManager().getActiveScenes();
            ASSERT_FALSE_ASYNC(scenes.empty());
            ASSERT_ASYNC(scenes.front() == sceneRef);
            
            co_return AssertionSuccess();
        });

        ASSERT_TRUE(testResult);
    }

    /**
        Test:
            transferring scene object's activation state
            1. initially object's activations state = Inactive
            2. during scene activation activations state = Activating (preventing immediately activation by using MyComponent1::setBlockActivation)
            3. allow activation to be finished and wait activation task. activations state = Active.
     */
    TEST_F(TestActiveScene, ObjectActivationState)
    {
        using namespace testing;
        using namespace nau::async;
        using namespace nau::scene;
        using namespace nau::scene_test;

        using namespace std::chrono_literals;

        const AssertionResult testResult = runTestApp([]() -> Task<AssertionResult>
        {
            IScene::Ptr scene = createEmptyScene();

            ObjectWeakRef object = scene->getRoot().attachChild(createObject<MyDefaultSceneComponent>());
            ASSERT_ASYNC(object->getActivationState() == ActivationState::Inactive);
            object->getRootComponent<MyDefaultSceneComponent>().setBlockActivation(true);

            auto activateTask = getSceneManager().activateScene(std::move(scene));

            ASSERT_ASYNC(object->getActivationState() == ActivationState::Active);
            ASSERT_ASYNC(object->getRootComponent<MyDefaultSceneComponent>().getActivationState() == ActivationState::Activating);

            object->getRootComponent<MyDefaultSceneComponent>().setBlockActivation(false);
            co_await activateTask;
            ASSERT_ASYNC(object->getRootComponent<MyDefaultSceneComponent>().getActivationState() == ActivationState::Active);

            co_return AssertionSuccess();
        });

        ASSERT_TRUE(testResult);
    }

    /**
     */
    TEST_F(TestActiveScene, ComponentActivatedDuringActivation)
    {
        using namespace testing;
        using namespace nau::async;
        using namespace nau::scene;
        using namespace nau::scene_test;

        const AssertionResult testResult = runTestApp([]() -> Task<AssertionResult>
        {
            IScene::Ptr scene = createEmptyScene();

            {
                auto child = createObject<MyDefaultSceneComponent>();
                scene->getRoot().attachChild(std::move(child));
            }

            ObjectWeakRef sceneRef = co_await getSceneManager().activateScene(std::move(scene));

            size_t counter = getServiceProvider().get<MyAsyncProcessor>().getActivateCounter();
            ASSERT_MSG_ASYNC(counter > 0, "Processor invalid active counter")

            MyDefaultSceneComponent* const component = sceneRef->getRoot().findFirstComponent<MyDefaultSceneComponent>(true);
            NAU_FATAL(component);

            ASSERT_ASYNC(component->isActivated());
            ASSERT_ASYNC(component->isActivatedAsync());

            co_return AssertionSuccess();
        });

        ASSERT_TRUE(testResult);
    }

    /**
     */
    TEST_F(TestActiveScene, AddComponentAsync)
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

            ObjectWeakRef component = co_await object->addComponentAsync<MyDefaultSceneComponent>();
            ASSERT_ASYNC(component);

            ASSERT_ASYNC(component->isActivated());
            ASSERT_ASYNC(component->isActivatedAsync());

            co_await skipFrames(1);
            ASSERT_ASYNC(component->getUpdateCounter() > 0);

            co_return AssertionSuccess();
        });

        ASSERT_TRUE(testResult);
    }

    /**
        Test:
            add object into active scene.
     */
    TEST_F(TestActiveScene, AttachObjectAsync)
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

            SceneObject::Ptr child = createObject();

            ObjectWeakRef component = child->addComponent<MyDefaultSceneComponent>();
            ASSERT_FALSE_ASYNC(component->isActivated());
            ASSERT_FALSE_ASYNC(component->isActivatedAsync());

            ObjectWeakRef childRef = co_await object->attachChildAsync(std::move(child));
            ASSERT_ASYNC(childRef);
            ASSERT_ASYNC(component->isActivated());
            ASSERT_ASYNC(component->isActivatedAsync());

            co_await skipFrames(1);
            ASSERT_ASYNC(component->getUpdateCounter() > 0);

            co_return AssertionSuccess();
        });

        ASSERT_TRUE(testResult);
    }

    /**
     */
    TEST_F(TestActiveScene, RemoveComponent)
    {
        using namespace testing;
        using namespace nau::async;
        using namespace nau::scene;
        using namespace nau::scene_test;

        using namespace std::chrono_literals;

        const AssertionResult testResult = runTestApp([&]() -> Task<AssertionResult>
        {
            size_t destructorCounter = 0;
            size_t disposedCounter = 0;
            size_t destroyedCounter = 0;
            size_t deactivatedCounter = 0;

            IScene::Ptr scene = createEmptyScene();

            getServiceProvider().get<MyAsyncProcessor>().setAsyncDeactivation(true);

            ObjectWeakRef object = scene->getRoot().attachChild(createObject());
            ObjectWeakRef component = object->addComponent<MyDisposableComponent>();
            component->setOnDeactivated([&]
            {
                ++deactivatedCounter;
            });

            component->setOnDestroyed([&]
            {
                ++destroyedCounter;
            });

            component->setOnDisposed([&]
            {
                ++disposedCounter;
            });

            component->setOnDestructor([&]
            {
                ++destructorCounter;
            });

            co_await getSceneManager().activateScene(std::move(scene));
            co_await skipFrames(1);

            object->removeComponent(component);

            co_await skipFrames(1);

            ASSERT_ASYNC(object->findFirstComponent<MyDisposableComponent>() == nullptr);
            ASSERT_FALSE_ASYNC(component);

            co_await skipFrames(1);

            const auto processorDeactivateCounter = getServiceProvider().get<MyAsyncProcessor>().getDeactivateCounter();

            ASSERT_ASYNC(processorDeactivateCounter > 0)
            ASSERT_ASYNC(destructorCounter == 1)
            ASSERT_ASYNC(disposedCounter == 1)
            ASSERT_ASYNC(deactivatedCounter == 1)

            co_return AssertionSuccess();
        });

        ASSERT_TRUE(testResult);
    }

    /**
        Test:
     */
    TEST_F(TestActiveScene, RemoveSceneObject)
    {
        using namespace testing;
        using namespace nau::async;
        using namespace nau::scene;
        using namespace nau::scene_test;

        using namespace std::chrono_literals;

        const AssertionResult testResult = runTestApp([&]() -> Task<AssertionResult>
        {
            size_t destructorCounter = 0;
            size_t disposedCounter = 0;
            size_t destroyedCounter = 0;
            size_t deactivatedCounter = 0;
            size_t componentCounter = 0;

            auto setupComponent = [&](MyDisposableComponent& component)
            {
                ++componentCounter;
                component.setOnDeactivated([&]
                {
                    ++deactivatedCounter;
                });

                component.setOnDestroyed([&]
                {
                    ++destroyedCounter;
                });

                component.setOnDisposed([&]
                {
                    ++disposedCounter;
                });

                component.setOnDestructor([&]
                {
                    ++destructorCounter;
                });
            };

            IScene::Ptr scene = createEmptyScene();

            ObjectWeakRef object = scene->getRoot().attachChild(createObject("Object_1"));
            ObjectWeakRef component = object->addComponent<MyDisposableComponent>();
            setupComponent(*component);

            ObjectWeakRef child_1 = object->attachChild(createObject<MyDisposableComponent>("Child_1"));
            setupComponent(child_1->getRootComponent<MyDisposableComponent>());

            ObjectWeakRef child_2 = object->attachChild(createObject<MyDisposableComponent>("Child_2"));
            setupComponent(child_2->getRootComponent<MyDisposableComponent>());

            ASSERT_ASYNC(component);

            ObjectWeakRef sceneRef = co_await getSceneManager().activateScene(std::move(scene));
            co_await skipFrames(1);

            sceneRef->getRoot().removeChild(object);
            ASSERT_ASYNC(sceneRef->getRoot().getDirectChildObjects().empty())
            
            // All references expected to be cleared immediately
            ASSERT_ASYNC(!object)
            ASSERT_ASYNC(!component)
            ASSERT_ASYNC(!child_1)
            ASSERT_ASYNC(!child_2)

            co_await skipFrames(1);

            ASSERT_ASYNC(destructorCounter == componentCounter)
            ASSERT_ASYNC(disposedCounter == componentCounter)
            ASSERT_ASYNC(deactivatedCounter == componentCounter)

            co_return AssertionSuccess();
        });

        ASSERT_TRUE(testResult);
    }
}  // namespace nau::test
