// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "scene_test_base.h"

#include <string>

#include "./scene_test_components.h"
#include "nau/assets/asset_container_builder.h"

namespace nau::test
{
    namespace
    {
        bool valueEquals(const RuntimeValue::Ptr& left, const RuntimeValue::Ptr& right)
        {
            NAU_FATAL(left);
            NAU_FATAL(right);

            if (left->is<scene::RuntimeObjectWeakRefValue>())
            {
                return right->is<scene::RuntimeObjectWeakRefValue>();
            }

            if (left->is<RuntimeIntegerValue>())
            {
                return right->is<RuntimeIntegerValue>() &&
                       left->as<const RuntimeIntegerValue&>().getInt64() == right->as<const RuntimeIntegerValue&>().getInt64();
            }

            if (left->is<RuntimeFloatValue>())
            {
                return right->is<RuntimeFloatValue>() &&
                       left->as<const RuntimeFloatValue&>().getDouble() == right->as<const RuntimeFloatValue&>().getDouble();
            }

            if (left->is<RuntimeBooleanValue>())
            {
                return right->is<RuntimeBooleanValue>() &&
                       left->as<const RuntimeBooleanValue&>().getBool() == right->as<const RuntimeBooleanValue&>().getBool();
            }

            if (left->is<RuntimeStringValue>())
            {
                return right->is<RuntimeStringValue>() &&
                       left->as<const RuntimeStringValue&>().getString() == right->as<const RuntimeStringValue&>().getString();
            }

            // equals by default
            return true;
        }
    }  // namespace

    IAssetContainerBuilder* SceneTestBase::findSceneBuilder(const SceneAsset::Ptr& asset)
    {
        for (auto builder : getServiceProvider().getAll<IAssetContainerBuilder>())
        {
            if (builder->isAcceptable(asset))
            {
                return builder;
            }
        }

        return nullptr;
    }

    IAssetContainerLoader* SceneTestBase::findSceneLoader()
    {
        for (auto loader : getServiceProvider().getAll<IAssetContainerLoader>())
        {
            auto kinds = loader->getSupportedAssetKind();
            if (!kinds.empty() && kinds.front() == "scene/nscene")
            {
                return loader;
            }
        }

        return nullptr;
    }

    io::IMemoryStream::Ptr SceneTestBase::dumpSceneToMemoryStream(scene::IScene& scene)
    {
        SceneAsset::Ptr sceneAsset = scene::wrapSceneAsAsset(scene);
        return dumpSceneToMemoryStream(std::move(sceneAsset));
    }

    io::IMemoryStream::Ptr SceneTestBase::dumpSceneToMemoryStream(SceneAsset::Ptr sceneAsset)
    {
        using namespace nau::io;

        IAssetContainerBuilder* const builder = findSceneBuilder(sceneAsset);
        NAU_FATAL(builder);

        IMemoryStream::Ptr stream = createMemoryStream(AccessMode::Read | AccessMode::Write);
        builder->writeAssetToStream(stream, sceneAsset).ignore();
        stream->setPosition(OffsetOrigin::Begin, 0);

        return stream;
    }

    scene::IScene::Ptr SceneTestBase::restoreSceneFromStream(io::IStreamReader& streamReader, scene::CreateSceneOptionFlag options)
    {
        using namespace nau::io;
        IAssetContainer::Ptr container = loadSceneAssetContainerFromStream(streamReader);
        SceneAsset::Ptr sceneAsset = container->getAsset();
        return getSceneFactory().createSceneFromAsset(*sceneAsset, options);
    }

    scene::IScene::Ptr SceneTestBase::copySceneThroughStream(scene::IScene& scene, scene::CreateSceneOptionFlag options)
    {
        using namespace nau::io;
        
        IMemoryStream::Ptr stream = dumpSceneToMemoryStream(scene);
        return restoreSceneFromStream(*stream, options);
    }

    scene::SceneObject::Ptr SceneTestBase::copySceneObjectThroughStream(scene::SceneObject& object, scene::CreateSceneOptionFlag options)
    {
        using namespace nau::io;
        using namespace nau::scene;
        
        SceneAsset::Ptr prefabAsset = scene::wrapSceneObjectAsAsset(object);
        IAssetContainerBuilder* const builder = findSceneBuilder(prefabAsset);
        NAU_FATAL(builder);

        IMemoryStream::Ptr stream = createMemoryStream(AccessMode::Read | AccessMode::Write);
        builder->writeAssetToStream(stream, prefabAsset).ignore();
        stream->setPosition(OffsetOrigin::Begin, 0);

        IAssetContainer::Ptr container = loadSceneAssetContainerFromStream(*stream);
        SceneAsset::Ptr prefabAssetCopy = container->getAsset();
        return getSceneFactoryInternal().createSceneObjectFromAssetWithOptions(*prefabAssetCopy, options);
    }

    std::string SceneTestBase::sceneToString(scene::IScene& scene)
    {
        using namespace nau::io;
        IMemoryStream::Ptr stream = createMemoryStream(AccessMode::Read | AccessMode::Write);

        SceneAsset::Ptr sceneAsset = scene::wrapSceneAsAsset(scene);

        IAssetContainerBuilder* builder = findSceneBuilder(sceneAsset);
        builder->writeAssetToStream(stream, sceneAsset).ignore();

        auto bytes = stream->getBufferAsSpan();
        std::string str{reinterpret_cast<const char*>(bytes.data()), bytes.size()};

        return str;
    }

    IAssetContainer::Ptr SceneTestBase::loadSceneAssetContainerFromStream(io::IStreamReader& stream, std::string assetKind)
    {
        using namespace nau::async;
        using namespace nau::io;

        IAssetContainerLoader* const assetLoader = findSceneLoader();
        NAU_FATAL(assetLoader);

        Task<IAssetContainer::Ptr> assetContainerTask = async::run([](IAssetContainerLoader* loader, std::string assetKind, IStreamReader& stream) -> Task<IAssetContainer::Ptr>
        {
            AssetContentInfo assetInfo{
                .kind = std::move(assetKind)};

            stream.setPosition(OffsetOrigin::Begin, 0);
            auto container = co_await loader->loadFromStream(nau::Ptr{&stream}, assetInfo);
            co_return container;
        }, Executor::getDefault(), assetLoader, std::move(assetKind), std::ref(stream));

        async::wait(assetContainerTask);
        return *assetContainerTask;
    }

    std::string SceneTestBase::sceneObjectToString(scene::SceneObject& sceneObject)
    {
        using namespace nau::io;
        IMemoryStream::Ptr stream = createMemoryStream(AccessMode::Read | AccessMode::Write);

        SceneAsset::Ptr sceneAsset = scene::wrapSceneObjectAsAsset(sceneObject);
        IAssetContainerBuilder* builder = findSceneBuilder(sceneAsset);
        builder->writeAssetToStream(stream, sceneAsset).ignore();

        auto bytes = stream->getBufferAsSpan();
        std::string str{reinterpret_cast<const char*>(bytes.data()), bytes.size()};

        return str;
    }

    testing::AssertionResult SceneTestBase::componentsEqualSimple(scene::Component& left, scene::Component& right, bool compareUids)
    {
        if (left.getClassDescriptor()->getClassTypeInfo() != right.getClassDescriptor()->getClassTypeInfo())
        {
            return testing::AssertionFailure() << "Component class mismatch";
        }

        if (compareUids && (left.getUid() != right.getUid()))
        {
            return testing::AssertionFailure() << "Component Uids mismatch";
        }

        NAU_FATAL(left.getSize() == right.getSize());

        for (size_t i = 0, fieldsCount = left.getSize(); i < fieldsCount; ++i)
        {
            auto [fieldNameLeft, fieldValueLeft] = left[i];
            auto [fieldNameRight, fieldValueRight] = right[i];

            if (fieldNameLeft != fieldNameRight)
            {
                return testing::AssertionFailure() << "Field name mismatch";
            }

            if (strings::icaseEqual("uid", fieldNameLeft))
            {
                continue;
            }

            if (!valueEquals(fieldValueLeft, fieldValueRight))
            {
                return testing::AssertionFailure() << ::fmt::format("Field ({}) value mismatch", fieldNameLeft);
            }
        }

        return testing::AssertionSuccess();
    }

    testing::AssertionResult SceneTestBase::sceneObjectsEqualSimple(scene::SceneObject& left, scene::SceneObject& right, bool compareUids)
    {
        if (compareUids && (left.getUid() != right.getUid()))
        {
            return testing::AssertionFailure() << "Uids mismatch";
        }

        if (left.getName() != right.getName())
        {
            return testing::AssertionFailure() << "Names mismatch";
        }

        {
            const auto leftComponents = left.getDirectComponents();
            const auto rightComponents = right.getDirectComponents();

            if (leftComponents.size() != rightComponents.size())
            {
                return testing::AssertionFailure() << "Components count mismatch";
            }

            for (size_t i = 0, count = leftComponents.size(); i < count; ++i)
            {
                if (auto res = componentsEqualSimple(*leftComponents[i], *rightComponents[i], compareUids); !res)
                {
                    return res;
                }
            }
        }

        {
            const auto leftChildren = left.getDirectChildObjects();
            const auto rightChildren = right.getDirectChildObjects();
            if (leftChildren.size() != rightChildren.size())
            {
                return testing::AssertionFailure() << "Child count mismatch";
            }

            for (size_t i = 0, count = leftChildren.size(); i < count; ++i)
            {
                if (auto res = sceneObjectsEqualSimple(*leftChildren[i], *rightChildren[i], compareUids); !res)
                {
                    return res;
                }
            }
        }

        return testing::AssertionSuccess();
    }

    testing::AssertionResult SceneTestBase::scenesEqualSimple(scene::IScene& scene1, scene::IScene& scene2, bool compareUids)
    {
        return sceneObjectsEqualSimple(scene1.getRoot(), scene2.getRoot(), compareUids);
    }

    void SceneTestBase::SetUp()
    {
        m_app = createApplication([this]
        {
            nau::loadModulesList(NAU_MODULES_LIST).ignore();

            initializeApp();

            return ResultSuccess;
        });
        m_app->startupOnCurrentThread();
    }

    void SceneTestBase::TearDown()
    {
        m_app->stop();
        while (m_app->step())
        {
            std::this_thread::yield();
        }
    }

    void SceneTestBase::initializeApp()
    {
        scene_test::registerAllTestComponentClasses();
    }

    Application& SceneTestBase::getApp()
    {
        return *m_app;
    }

    async::Task<> SceneTestBase::skipFrames(unsigned frameCount)
    {
        if (frameCount == 0)
        {
            return async::makeResolvedTask();
        }

        m_frameSkipAwaiters.emplace_back(frameCount);
        auto& awaiter = m_frameSkipAwaiters.back();
        return awaiter.signal.getTask();
    }

    testing::AssertionResult SceneTestBase::runTestApp(TestCallback callback)
    {
        using namespace std::chrono_literals;
        using namespace testing;
        using namespace nau::async;

        auto task = [](TestCallback&& callback) -> Task<AssertionResult>
        {
            scope_on_leave
            {
                getApplication().stop();
            };

            if (!callback)
            {
                co_return AssertionSuccess();
            }
            auto testTask = callback();
            NAU_FATAL(testTask);

            co_await testTask;
            co_return *testTask;
        }(std::move(callback));

        while (m_app->step())
        {
            std::this_thread::sleep_for(1ms);
            ++m_stepCounter;

            auto iter = std::remove_if(m_frameSkipAwaiters.begin(), m_frameSkipAwaiters.end(), [](SkipFrameAwaiter& awaiter)
            {
                if (--awaiter.skipFramesCount == 0)
                {
                    awaiter.signal.resolve();
                }

                return awaiter.skipFramesCount == 0;
            });

            m_frameSkipAwaiters.erase(iter, m_frameSkipAwaiters.end());
        }

        NAU_FATAL(task.isReady());

        return *task;
    }
}  // namespace nau::test
