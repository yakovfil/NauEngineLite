// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include "nau/async/work_queue.h"
#include "nau/memory/eastl_aliases.h"
#include "nau/scene/components/component_life_cycle.h"
#include "nau/scene/internal/scene_listener.h"
#include "nau/scene/internal/scene_manager_internal.h"
#include "nau/scene/scene_manager.h"
#include "nau/scene/scene_object.h"
#include "scene_impl.h"
#include "world_impl.h"

namespace nau::scene
{
    class SceneManagerImpl final : public ISceneManager,
                                   public ISceneManagerInternal,
                                   public IRefCounted
    {
        NAU_CLASS_(nau::scene::SceneManagerImpl, ISceneManager, ISceneManagerInternal, IRefCounted)
    public:
        SceneManagerImpl();

        ActivationState getSceneObjectActivationState(const SceneObject&) const;

        Vector<IScene::WeakRef> getActiveScenes(const WorldImpl* world) const;

        async::Task<IScene::WeakRef> activateScene(ObjectWeakRef<WorldImpl> world, IScene::Ptr&& scene);

        async::Task<> activateSceneObject(SceneObject& object);

        async::Task<Functor<void(bool)>> activateComponents(Vector<Component*> components, bool delayActivation);

        void destroySceneObject(SceneObject& object);

        void destroyComponent(Component& component);

        IWorld& getDefaultWorld() const override;

        Vector<IWorld::WeakRef> getWorlds() const override;

        IWorld::WeakRef findWorld(Uid worldUid) const override;

        IWorld::WeakRef createWorld() override;

        void destroyWorld(IWorld::WeakRef worldRef) override;

        Vector<IScene::WeakRef> getActiveScenes() const override;

        async::Task<IScene::WeakRef> activateScene(IScene::Ptr&& scene) override;

        void deactivateScene(IScene::WeakRef sceneRef) override;

        ObjectWeakRef<> querySingleObject(const SceneQuery& query) override;

        void update(float dt) override;

        Component* findComponent(Uid componentId) override;

        async::Task<> shutdown() override;

        void pollShutdown() override;

        void notifyListenerComponentWasChanged(const Component& component);

    private:
        struct UpdatableComponentEntry
        {
            Component* component = nullptr;
            IComponentUpdate* componentUpdate = nullptr;
            IComponentAsyncUpdate* componentAsyncUpdate = nullptr;
            async::Task<> asyncUpdateTask;

            UpdatableComponentEntry(Component& inComponent);
            UpdatableComponentEntry(UpdatableComponentEntry&&) = default;
            UpdatableComponentEntry& operator=(UpdatableComponentEntry&&) = default;
            bool isActive() const
            {
                return component->getActivationState() == ActivationState::Active;
            }
        };

        struct SceneEntry
        {
            ObjectUniquePtr<SceneImpl> scene;

            SceneEntry(IScene::Ptr&& inScene) :
                scene(std::move(inScene))
            {
            }
        };

        void deactivateSceneInternal(ObjectWeakRef<SceneImpl> scene);

        void deactivateSceneObjectInternal(SceneObject& object, bool destroy);

        async::Task<> deactivateComponentsInternal(Vector<Component*> components);

        eastl::list<SceneEntry>::iterator getSceneIter(IScene* scene);

        void notifyListenerBeginScene();
        void notifyListenerEndScene();

        SceneListenerRegistration addSceneListener(ISceneListener& sceneListener) override;
        void removeSceneListener(void* sceneListenerHandle);

        ObjectWeakRef<> lookupComponent(const SceneQuery& query);

        ObjectWeakRef<> lookupSceneObject(const SceneQuery& query);

        eastl::list<ObjectUniquePtr<WorldImpl>> m_worlds;
        eastl::list<SceneEntry> m_scenes;
        eastl::list<UpdatableComponentEntry> m_updatableComponents;
        eastl::unordered_map<Uid, SceneObject*> m_activeObjects;
        eastl::unordered_map<Uid, Component*> m_activeComponents;

        bool m_insideUpdate = false;
        async::TaskCollection m_asyncTasks;
        WorkQueue::Ptr m_updateWorkQueue = WorkQueue::create();
        WorkQueue::Ptr m_postUpdateWorkQueue = WorkQueue::create();

        ISceneListener* m_sceneListener = nullptr;

        // TODO: The allocator is currently being used incorrectly (there is single allocator that is being used from the graphics thread)
        // eastl::unordered_set<const Component*, eastl::hash<const Component*>, eastl::equal_to<const Component*>, EastlFrameAllocator> m_changedComponents;
        eastl::unordered_set<const Component*> m_changedComponents;

        friend struct SceneListenerRegistration;
    };
}  // namespace nau::scene
