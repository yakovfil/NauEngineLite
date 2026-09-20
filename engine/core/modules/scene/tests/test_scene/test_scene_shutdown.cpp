// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/scene/components/component_life_cycle.h"
#include "nau/scene/components/scene_component.h"
#include "scene_test_base.h"

namespace nau::test
{
    struct PendingSceneUpdate
    {
        async::TaskSource<> release;
        int updates = 0;
        int completed = 0;
        int destroyed = 0;
    };

    class PendingShutdownComponent final : public scene::SceneComponent,
                                           public scene::IComponentAsyncUpdate
    {
        NAU_OBJECT(PendingShutdownComponent, scene::SceneComponent, scene::IComponentAsyncUpdate)
        NAU_DECLARE_DYNAMIC_OBJECT
    public:
        PendingSceneUpdate* state = nullptr;
        ~PendingShutdownComponent()
        {
            if (state)
                ++state->destroyed;
        }
        async::Task<> updateComponentAsync(float) override
        {
            ++state->updates;
            co_await state->release.getTask();
            ++state->completed;
        }
    };

    NAU_IMPLEMENT_DYNAMIC_OBJECT(PendingShutdownComponent)

    class TestSceneShutdown : public SceneTestBase
    {
    };

    TEST_F(TestSceneShutdown, PendingUpdateCompletesWithoutNewUpdatesAfterStop)
    {
        PendingSceneUpdate state;
        const auto result = runTestApp([&]() -> async::Task<testing::AssertionResult>
        {
            registerClasses<PendingShutdownComponent>();
            auto scene = createEmptyScene();
            auto& object = scene->getRoot().attachChild(createObject<PendingShutdownComponent>());
            object.getRootComponent<PendingShutdownComponent>().state = &state;
            co_await getSceneManager().activateScene(std::move(scene));
            co_await skipFrames(2);
            EXPECT_EQ(state.updates, 1);
            EXPECT_EQ(state.completed, 0);
            getApp().stop();
            // The delay resumes on the application executor after shutdown starts;
            // the component continuation still belongs to the scene executor.
            co_await std::chrono::milliseconds(5);
            EXPECT_EQ(state.destroyed, 0);
            state.release.resolve();
            co_return testing::AssertionSuccess();
        });
        EXPECT_TRUE(result);
        EXPECT_EQ(state.updates, 1);
        EXPECT_EQ(state.completed, 1);
        EXPECT_EQ(state.destroyed, 1);
    }
}  // namespace nau::test
