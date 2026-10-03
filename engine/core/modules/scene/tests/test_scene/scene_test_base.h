// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include <memory>
#include <string>
#include <vector>

#include "nau/assets/asset_container.h"
#include "nau/assets/asset_container_builder.h"
#include "nau/io/memory_stream.h"
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
    public:
        using TestCallback = Functor<async::Task<testing::AssertionResult>()>;

        static scene::ISceneFactory& getSceneFactory()
        {
            return getServiceProvider().get<scene::ISceneFactory>();
        }

        static scene_internal::ISceneFactoryInternal& getSceneFactoryInternal()
        {
            return getServiceProvider().get<scene_internal::ISceneFactoryInternal>();
        }

        static scene::ISceneManager& getSceneManager()
        {
            return getServiceProvider().get<scene::ISceneManager>();
        }

        static scene::IScene::Ptr createEmptyScene()
        {
            return getSceneFactory().createEmptyScene();
        }

        static IAssetContainerBuilder* findSceneBuilder(const SceneAsset::Ptr& asset);

        static IAssetContainerLoader* findSceneLoader();

        static io::IMemoryStream::Ptr dumpSceneToMemoryStream(scene::IScene& scene);

        static io::IMemoryStream::Ptr dumpSceneToMemoryStream(SceneAsset::Ptr sceneAsset);

        static scene::IScene::Ptr restoreSceneFromStream(io::IStreamReader& streamReader, scene::CreateSceneOptionFlag options);

        static scene::IScene::Ptr copySceneThroughStream(scene::IScene& scene, scene::CreateSceneOptionFlag options);

        static scene::SceneObject::Ptr copySceneObjectThroughStream(scene::SceneObject& object, scene::CreateSceneOptionFlag options);

        static std::string sceneToString(scene::IScene& scene);

        static std::string sceneObjectToString(scene::SceneObject& sceneObject);

        static testing::AssertionResult componentsEqualSimple(scene::Component& left, scene::Component& right, bool compareUids = true);

        static testing::AssertionResult sceneObjectsEqualSimple(scene::SceneObject& left, scene::SceneObject& right, bool compareUids = true);

        static testing::AssertionResult scenesEqualSimple(scene::IScene& scene1, scene::IScene& scene2, bool compareUids = true);

        


        static std::string memStreamToString(const io::IMemoryStream& stream)
        {
            auto buffer = stream.getBufferAsSpan();
            return std::string{reinterpret_cast<const char*>(buffer.data()), buffer.size()};
        }

        static IAssetContainer::Ptr loadSceneAssetContainerFromStream(io::IStreamReader& stream, std::string assetKind = "nscene");

        template <std::derived_from<scene::SceneComponent> ComponentType = scene::SceneComponent>
        static scene::SceneObject::Ptr createObject(std::string name = "")
        {
            scene::SceneObject::Ptr newObject = getSceneFactory().createSceneObject<ComponentType>();
            NAU_FATAL(newObject);
            newObject->setName(name);

            return newObject;
        }

        template <typename... T>
        static void registerClasses()
        {
            auto& serviceProvider = getServiceProvider();
            (serviceProvider.addClass<T>(), ...);
        }

        template <typename... T>
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

#define ASSERT_MSG_ASYNC(condition, message)                                   \
    if (!(condition))                                                          \
    {                                                                          \
        co_return testing::AssertionFailure() << #condition << ":" << message; \
    }

#define ASSERT_ASYNC(condition)                              \
    if (!(condition))                                        \
    {                                                        \
        co_return testing::AssertionFailure() << #condition; \
    }

#define ASSERT_FALSE_ASYNC(condition)                        \
    if ((condition))                                         \
    {                                                        \
        co_return testing::AssertionFailure() << #condition; \
    }\
