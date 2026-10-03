// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include <memory>
#include <string>
#include <vector>

#include "nau/scene/scene.h"
#include "nau/scene/scene_factory.h"
#include "nau/scene/scene_manager.h"
#include "nau/service/service_provider.h"
#include "nau/utils/functor.h"

namespace nau::test
{
    /**
     */
    class SceneTestBase : public testing::Test
    {
    protected:
        using TestCallback = Functor<async::Task<testing::AssertionResult> ()>;

        static scene::ISceneFactory& getSceneFactory()
        {
            return getServiceProvider().get<scene::ISceneFactory>();
        }

        static scene::ISceneManager& getSceneManager()
        {
            return getServiceProvider().get<scene::ISceneManager>();
        }

        static scene::IScene::Ptr createEmptyScene()
        {
            return getSceneFactory().createEmptyScene();
        }

        template <std::derived_from<scene::SceneComponent> ComponentType = scene::SceneComponent>
        static scene::SceneObject::Ptr createObject(std::string name = "")
        {
            scene::SceneObject::Ptr newObject = getSceneFactory().createSceneObject<ComponentType>();
            NAU_FATAL(newObject);
            newObject->setName(name);

            return newObject;
        }

        template<typename ... T>
        static void registerClasses()
        {
            auto& serviceProvider = getServiceProvider();
            (serviceProvider.addClass<T>(), ...);
        }

        template<typename ... T>
        static void registerServices()
        {
            auto& serviceProvider = getServiceProvider();
            (serviceProvider.addService<T>(), ...);
        }

        void SetUp() override;

        void TearDown() override;

        virtual void initializeApp();

        Application& getApp();

        async::Task<> skipFrames(unsigned frameCount);

        testing::AssertionResult runTestApp(TestCallback callback);

    private:
        struct SkipFrameAwaiter
        {
            unsigned skipFramesCount;
            async::TaskSource<> signal;
        };

        std::unique_ptr<Application> m_app;
        unsigned m_stepCounter = 0;
        std::vector<SkipFrameAwaiter> m_frameSkipAwaiters;
    };
}  // namespace nau::test


#define ASSERT_MSG_ASYNC(condition, message)\
    if (!(condition)) { \
        co_return testing::AssertionFailure() << #condition << ":" << message;\
    }\

#define ASSERT_ASYNC(condition)\
    if (!(condition)) { \
        co_return testing::AssertionFailure() << #condition;\
    }\


#define ASSERT_FALSE_ASYNC(condition)\
    if ((condition)) { \
        co_return testing::AssertionFailure() << #condition;\
    }\
