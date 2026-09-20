// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "scene_manager_impl.h"

#include <nau/assets/asset_ref.h>
#include <nau/assets/scene_asset.h>
#include <nau/scene/scene_factory.h>

#include "nau/memory/stack_allocator.h"
#include "nau/scene/scene_processor.h"
#include "scene_impl.h"

namespace nau::scene
{
    SceneListenerRegistration::SceneListenerRegistration(void* handle) :
        m_handle(handle)
    {
    }
    SceneListenerRegistration::SceneListenerRegistration(SceneListenerRegistration&& other) :
        m_handle(std::exchange(other.m_handle, nullptr))
    {
    }

    SceneListenerRegistration::~SceneListenerRegistration()
    {
        reset();
    }

    SceneListenerRegistration& SceneListenerRegistration::operator=(SceneListenerRegistration&& other)
    {
        reset();
        m_handle = std::exchange(other.m_handle, nullptr);
        return *this;
    }

    SceneListenerRegistration::operator bool() const
    {
        return m_handle != nullptr;
    }

    void SceneListenerRegistration::reset()
    {
        if (const auto handle = std::exchange(m_handle, nullptr); handle != nullptr && hasServiceProvider())
        {
            ServiceProvider& serviceProvider = getServiceProvider();
            if (serviceProvider.has<SceneManagerImpl>())
            {
                serviceProvider.get<SceneManagerImpl>().removeSceneListener(handle);
            }
        }
    }

    ISceneListener* SceneListenerRegistration::getListener() const
    {
        return reinterpret_cast<ISceneListener*>(m_handle);
    }

    SceneManagerImpl::UpdatableComponentEntry::UpdatableComponentEntry(Component& inComponent) :
        component(&inComponent),
        componentUpdate(inComponent.as<IComponentUpdate*>()),
        componentAsyncUpdate(inComponent.as<IComponentAsyncUpdate*>())
    {
    }

    SceneManagerImpl::SceneManagerImpl()
    {
        m_worlds.emplace_back(NauObject::classCreateInstance<WorldImpl>());
        m_worlds.front()->setName("game_main");
    }

    ActivationState SceneManagerImpl::getSceneObjectActivationState(const SceneObject& object) const
    {
        const auto state = object.m_activationState;
#ifdef NAU_ASSERT_ENABLED
        if (state != ActivationState::Inactive)
        {
            NAU_ASSERT(m_activeObjects.find(object.getUid()) != m_activeObjects.end(), "Invalid manager inner state");
        }
#endif

        return state;
    }

    async::Task<> SceneManagerImpl::activateSceneObject(SceneObject& rootObject)
    {
        using namespace nau::async;

        IScene* const scene = rootObject.getScene();
        if (getSceneIter(scene) == m_scenes.end())
        {
            NAU_ASSERT(rootObject.getActivationState() == ActivationState::Inactive);
            NAU_LOG_WARNING("Can not activate object that does not belongs to the scene (or scene is not active)");
            co_return;
        }

        Task<Functor<void(bool notifyListener)>> activateComponentsTask;

        {
            // Collect all the components that need to be activated (components does not contains inside this->m_activeComponents).
            // Make the root object and all its descendants as 'activating'.
            Vector<Component*> incomingComponents;

            // currently children can not be modified at activation time
            Vector<SceneObject*> allChildren = rootObject.getAllChildObjects();

            auto collectInactiveComponents = [&](SceneObject& object)
            {
                // making objects state = Activating
                if (object.m_activationState == ActivationState::Inactive)
                {
                    object.m_activationState = ActivationState::Activating;
                }

                for (Component* component : object.getDirectComponents())
                {
                    NAU_FATAL(component);

                    if (m_activeComponents.find(component->getUid()) == m_activeComponents.end())
                    {
                        incomingComponents.push_back(component);
                    }
                }
            };

            collectInactiveComponents(rootObject);

            for (SceneObject* descendantObject : allChildren)
            {
                collectInactiveComponents(*descendantObject);
            }

            activateComponentsTask = activateComponents(std::move(incomingComponents), true);
        }

        Vector<const SceneObject*> activatedObjectsForListener;
        const bool notifyOnlyRootObject = (rootObject.m_activationState == ActivationState::Activating);
        if (m_sceneListener && notifyOnlyRootObject)
        {
            activatedObjectsForListener.push_back(&rootObject);
        }

        const auto makeObjectActive = [&](SceneObject& object)
        {
            if (object.m_activationState == ActivationState::Activating)
            {
                if (m_sceneListener && !notifyOnlyRootObject)
                {
                    activatedObjectsForListener.push_back(&object);
                }

                object.m_activationState = ActivationState::Active;
                [[maybe_unused]] const auto [_, emplaceOk] = m_activeObjects.emplace(object.getUid(), &object);
                NAU_ASSERT(emplaceOk);
            }
#ifdef NAU_ASSERT_ENABLED
            else
            {
                NAU_ASSERT(object.m_activationState == ActivationState::Active);
                NAU_ASSERT(m_activeObjects.contains(object.getUid()));
            }
#endif
        };

        makeObjectActive(rootObject);
        Vector<SceneObject*> childObjects = rootObject.getAllChildObjects();

        for (SceneObject* object : childObjects)
        {
            makeObjectActive(*object);
        }

        Functor<void(bool notifyListener)> componentActivationFinalizer = co_await activateComponentsTask;
        NAU_FATAL(componentActivationFinalizer);

        // If there is no activated objects
        // then make the assumption that the activation is caused by the addition of new components.
        //
        // This logic should be reviewed and clarified, since formally in the activation process there can be operations of adding new objects
        // as well as adding components to existing ones (but for now exclude such cases).
        const bool notifyListenerAboutComponentActivation = activatedObjectsForListener.empty();
        componentActivationFinalizer(notifyListenerAboutComponentActivation);

        if (m_sceneListener && !activatedObjectsForListener.empty())
        {
            m_sceneListener->onAfterActivatingObjects(eastl::span{activatedObjectsForListener.data(), activatedObjectsForListener.size()});
        }
    }

    async::Task<Functor<void(bool)>> SceneManagerImpl::activateComponents(Vector<Component*> components, bool delayActivation)
    {
        using namespace nau::async;

        NAU_ASSERT(!components.empty());

        const Uid worldUid = components.front()->getParentObject().getScene()->getWorld()->getUid();
#ifdef NAU_ASSERT_ENABLED
        if (!components.empty())
        {
            const bool belongsToTheSameWorld = eastl::all_of(components.begin(), components.end(), [&worldUid](const Component* component)
            {
                return component->getParentObject().getScene()->getWorld()->getUid() == worldUid;
            });

            NAU_ASSERT(belongsToTheSameWorld, "Currently (batch) activated components must belongs to the same world");
        }
#endif

        {
            auto iter = eastl::remove_if(components.begin(), components.end(), [](const Component* component)
            {
                return component->getActivationState() != ActivationState::Inactive;
            });

            if (iter != components.end())
            {
                NAU_LOG_WARNING("Some components  have unexpected state during activateComponents call");
                components.erase(iter, components.end());
            }
        }

        for (Component* const component : components)
        {
            component->changeActivationState(ActivationState::Activating);
        }

        // Activate components:
        // 1. Activate through component activators
        // - collect all scene processors with IComponentsActivator/IComponentsAsyncActivator API:
        // - call activateComponents/activateComponentsAsync for each processor
        // - async activation operations are non blocking
        //
        // 2. Activate components itself
        // - calling activateComponent/activateComponentAsync for each component that have IComponentActivation/IComponentAsyncActivation API
        // - async activation operations are non blocking
        //
        // 3. waiting for all asynchronous activation operations

        {  // Process IComponentsActivator. IComponentsActivator accept non constant components collection
            auto componentActivators = getServiceProvider().getAll<IComponentsActivator>();

            for (IComponentsActivator* componentActivator : componentActivators)
            {
                componentActivator->activateComponents(worldUid, components).ignore();
            }
        }

        Vector<Task<>> activationTasks;
        MultiTaskSource<> activationBarrier;

        {  // Process IComponentsAsyncActivator. IComponentsAsyncActivator accept constant components collection.
            // Mutable components can not be used, because async activation can be performed in concurrent fashion (in background threads)
            const eastl::span componentsConstSpan{const_cast<const Component**>(components.data()), components.size()};
            auto componentAsyncActivators = getServiceProvider().getAll<IComponentsAsyncActivator>();

            activationTasks.reserve(componentAsyncActivators.size() + components.size());

            // scene processors:
            for (IComponentsAsyncActivator* componentAsyncActivator : componentAsyncActivators)
            {
                auto barrierTask = activationBarrier.getNextTask().detach();
                if (auto task = componentAsyncActivator->activateComponentsAsync(worldUid, componentsConstSpan, std::move(barrierTask)); task && !task.isReady())
                {
                    activationTasks.emplace_back(std::move(task));
                }
            }
        }

        // components
        for (Component* component : components)
        {
            if (IComponentActivation* componentActivation = component->as<IComponentActivation*>())
            {
                componentActivation->activateComponent();
                if (auto task = componentActivation->activateComponentAsync(); task && !task.isReady())
                {
                    activationTasks.emplace_back(std::move(task));
                }
            }
        }

        co_await async::whenAll(activationTasks);
        activationBarrier.resolve();

        auto activationFinalizer = [this, components = std::move(components)](bool notifyListener) mutable
        {
            for (Component* const component : components)
            {
                m_activeComponents.emplace(component->getUid(), component);
                const bool isUpdatable = component->is<IComponentUpdate>() || component->is<IComponentAsyncUpdate>();
                if (isUpdatable)
                {
                    m_updatableComponents.emplace_back(*component);
                }

                // IComponentEvents::onComponentActivated must be called inside transferActivationState
                component->changeActivationState(ActivationState::Active);
            }

            if (notifyListener && m_sceneListener)
            {
                m_sceneListener->onAfterActivatingComponents(eastl::span{const_cast<const Component**>(components.data()), components.size()});
            }
        };

        if (delayActivation)
        {
            co_return std::move(activationFinalizer);
        }

        activationFinalizer(true);
        co_return nullptr;
    }

    void SceneManagerImpl::destroySceneObject(SceneObject& object)
    {
        deactivateSceneObjectInternal(object, true);
    }

    void SceneManagerImpl::destroyComponent(Component& component)
    {
        const auto componentState = component.m_activationState;
        if (componentState == ActivationState::Inactive)
        {
            NAU_FATAL(component.m_parentObject);
            component.deleteObjectNow();
            return;
        }
        NAU_ASSERT(componentState == ActivationState::Active, "Unexpected component activation state:({})", static_cast<int>(componentState));

        Vector<Component*> components{&component};
        if (m_sceneListener)
        {
            eastl::span<const Component*> componentsSpan{const_cast<const Component**>(components.data()), 1};
            m_sceneListener->onBeforeDeletingComponents(componentsSpan);
        }

        m_asyncTasks.push(deactivateComponentsInternal({&component}));
    }

    /**
        Need to collect object and components in reverse order: first comes child objects and components.
     */
    struct DeactivationSequence
    {
        eastl::list<SceneObject*> objects;
        Vector<Component*> components;

        DeactivationSequence(SceneObject& root)
        {
            visitObject(root, this);
            eastl::reverse(components.begin(), components.end());
        }

        static bool visitObject(SceneObject& obj, void* data)
        {
            DeactivationSequence& self = *reinterpret_cast<DeactivationSequence*>(data);

            self.objects.push_front(&obj);

            obj.walkComponents(DeactivationSequence::visitComponent, data, false);
            obj.walkChildObjects(DeactivationSequence::visitObject, data, false);

            return true;
        }

        static bool visitComponent(Component& component, void* data)
        {
            DeactivationSequence& self = *reinterpret_cast<DeactivationSequence*>(data);

            // components will be reversed at the end
            self.components.push_back(&component);
            return true;
        }
    };

    void SceneManagerImpl::deactivateSceneObjectInternal(SceneObject& object, bool destroy)
    {
        using namespace nau::async;

        const auto objectState = getSceneObjectActivationState(object);

        if (objectState == ActivationState::Inactive)
        {  // object is not attached to the scene: just destroying it (if it has no ObjectUniquePtr ownership).

            if (object.m_hasPtrOwner)
            {
                // Sane check: in general this situation should not occur, but
                // this n is possible when the user calls SceneObject::destroy() for an object that is not attached to the scene.
                // In this case, the object must be deleted in the UniqueObjectPtr's destructor.
                object.clearSceneReferencesRecursive();
                return;
            }

            if (destroy)
            {
                object.deleteObjectNow();
            }
            return;
        }

        NAU_ASSERT(objectState == ActivationState::Active, "Unexpected object's ({}) activation state:({})", object.getName(), static_cast<int>(objectState));
        if (objectState != ActivationState::Active)
        {
            return;
        }

        if (m_sceneListener)
        {
            const SceneObject* objectPtr = &object;
            eastl::span<const SceneObject*> objectsSpan{&objectPtr, 1};
            m_sceneListener->onBeforeDeletingObjects(objectsSpan);
        }

        // collect objects from bottom to top: first deactivate descendant objects and components
        DeactivationSequence sequence{object};

        auto& allObjects = sequence.objects;

        for (auto* obj : allObjects)
        {
            NAU_ASSERT(obj->m_activationState == ActivationState::Active);
            obj->m_activationState = ActivationState::Deactivating;
        }

        m_asyncTasks.push(deactivateComponentsInternal(std::move(sequence.components)));

        for (SceneObject* const obj : allObjects)
        {
            NAU_ASSERT(obj->m_activationState == ActivationState::Deactivating);

            obj->m_activationState = ActivationState::Inactive;
            [[maybe_unused]] const bool removedFromActiveObjects = m_activeObjects.erase(obj->getUid()) > 0;
            NAU_ASSERT(removedFromActiveObjects);
            obj->clearAllWeakReferences();
        }

        if (destroy)
        {
            object.deleteObjectNow();
        }
        else
        {
            object.resetParentInternal(nullptr, SetParentOpts::DontKeepWorldTransform);
        }
    }

    async::Task<> SceneManagerImpl::deactivateComponentsInternal(Vector<Component*> components)
    {
        using namespace nau::async;

        NAU_ASSERT(!components.empty());

        const Uid worldUid = components.front()->getParentObject().getScene()->getWorld()->getUid();
#ifdef NAU_ASSERT_ENABLED
        if (!components.empty())
        {
            const bool belongsToTheSameWorld = eastl::all_of(components.begin(), components.end(), [&worldUid](const Component* component)
            {
                return component->getParentObject().getScene()->getWorld()->getUid() == worldUid;
            });

            NAU_ASSERT(belongsToTheSameWorld, "Currently (batch) activated components must belongs to the same world");
        }
#endif

        // 1. Marking components as 'Deactivating' (this will  prevents update call (if it is not called yet)).
        for (Component* component : components)
        {
            if (IComponentActivation* componentActivation = component->as<IComponentActivation*>())
            {
                componentActivation->deactivateComponent();
            }
        }

        {
            // must call IComponentsActivator::deactivateComponents prior detaching components from scene.
            const eastl::span componentsSpan{components.data(), components.size()};

            for (IComponentsActivator* componentActivator : getServiceProvider().getAll<IComponentsActivator>())
            {
                componentActivator->deactivateComponents(worldUid, componentsSpan);
            }
        }

        // for async deactivation must use DeactivatedComponentData,
        // because all deactivated components going to be non operable after first async co_await
        Vector<DeactivatedComponentData> deactivatedComponents;
        deactivatedComponents.reserve(components.size());
        eastl::transform(components.begin(), components.end(), eastl::back_inserter(deactivatedComponents), [](Component* component)
        {
            NAU_FATAL(component);
            const auto& parentObject = component->getParentObject();
            const auto* scene = parentObject.getScene();
            NAU_FATAL(scene);

            return DeactivatedComponentData{
                .component = component,
                .componentUid = component->getUid(),
                .parentObjectUid = parentObject.getUid(),
                .sceneUid = scene->getUid(),
                .worldUid = scene->getWorld()->getUid()};
        });

        // removing components from hierarchy and clear all external references.
        // this step must be preformed only after all IComponentActivation::deactivateComponent() are called.
        // from this moment each deactivated Component is not valid ony more.
        for (Component* component : components)
        {
            NAU_FATAL(component);
            NAU_FATAL(component->isOperable());

            component->changeActivationState(ActivationState::Deactivating);
            component->clearAllWeakReferences();
            component->getParentObject().removeComponentFromList(*component);

            if (const auto iter = m_activeComponents.find(component->getUid()); iter != m_activeComponents.end())
            {
                m_activeComponents.erase(iter);
            }
        }

        // Component deactivation must be processed only from outside of the main update
        if (m_insideUpdate)
        {
            co_await m_postUpdateWorkQueue;
        }

        m_updatableComponents.remove_if([](UpdatableComponentEntry& entry)
        {
            NAU_FATAL(entry.component);
            const bool deactivating = entry.component->m_activationState == ActivationState::Deactivating;
            if (deactivating)
            {
                // keep listener's finalization as component's internal async operation
                // that will be awaited prior component deletion
                if (entry.asyncUpdateTask && !entry.asyncUpdateTask.isReady())
                {
                    entry.component->m_asyncTasks.push(std::move(entry.asyncUpdateTask));
                }
            }

            return deactivating;
        });

        {
            // all scene processors will be notified through ISceneProcessor/IComponentsActivator or ISceneProcessor/IComponentsAsyncActivator
            auto componentAsyncActivators = getServiceProvider().getAll<IComponentsAsyncActivator>();
            Vector<Task<>> deactivationTasks;
            deactivationTasks.reserve(components.size() + componentAsyncActivators.size());

            // scene processors/activators:
            for (IComponentsAsyncActivator* const asyncActivator : componentAsyncActivators)
            {
                if (auto task = asyncActivator->deactivateComponentsAsync(worldUid, deactivatedComponents); task && !task.isReady())
                {
                    deactivationTasks.emplace_back(std::move(task));
                }
            }

            co_await async::whenAll(deactivationTasks);
        }

        //
        // finalize components all async operations, including:
        // 1. cancel async listeners, transfer component's state to Deactivating
        // 2. wait for all running async tasks
        Vector<Task<>> finalizationTasks;
        finalizationTasks.reserve(components.size());

        eastl::erase_if(components, [&](Component* component)
        {
            component->changeActivationState(ActivationState::Inactive);
            if (auto task = component->finalizeAsyncOperations(); task && !task.isReady())
            {
                finalizationTasks.emplace_back(std::move(task));
                return false;
            }
            component->deleteObjectNow();

            return true;
        });

        co_await async::whenAll(finalizationTasks);

        for (Component* component : components)
        {
            component->deleteObjectNow();
        }
    }

    async::Task<IScene::WeakRef> SceneManagerImpl::activateScene(ObjectWeakRef<WorldImpl> world, IScene::Ptr&& scene)
    {
        using namespace nau::async;
        NAU_ASSERT(world);
        if (!world)
        {
            co_return IScene::WeakRef{};
        }

        NAU_ASSERT(scene);
        if (!scene)
        {
            co_return IScene::WeakRef{};
        }

        NAU_FATAL(getSceneIter(scene.get()) == m_scenes.end());

        m_scenes.emplace_back(std::move(scene));
        ObjectWeakRef sceneRef = *m_scenes.back().scene;
        sceneRef->setWorld(*world);

        co_await activateSceneObject(sceneRef->getRoot());

        co_return sceneRef;
    }

    async::Task<IScene::WeakRef> SceneManagerImpl::activateScene(IScene::Ptr&& scene)
    {
        return activateScene(getDefaultWorld(), std::move(scene));
    }

    void SceneManagerImpl::deactivateScene(IScene::WeakRef sceneRef)
    {
        using namespace nau::async;
        // The method is implemented taking into account the fact that the scene deletion operation can be called multiple times for the same scene object.
        // In this case, the first call initiates the actual operation of deactivating and deleting the scene: deactivateSceneInternal.
        // All subsequent ones will wait for the first call to complete.

        NAU_ASSERT(sceneRef);
        if (!sceneRef)
        {
            return;
        }

        auto sceneEntry = getSceneIter(sceneRef.get());
        if (sceneEntry == m_scenes.end())
        {
            NAU_FAILURE("Scene reference is valid, but actual scene object not exists");
            return;
        }

        deactivateSceneObjectInternal(sceneEntry->scene->getRoot(), false);
        m_scenes.erase(sceneEntry);
    }

    IWorld& SceneManagerImpl::getDefaultWorld() const
    {
        NAU_FATAL(!m_worlds.empty());
        return *m_worlds.front();
    }

    Vector<IWorld::WeakRef> SceneManagerImpl::getWorlds() const
    {
        Vector<IWorld::WeakRef> worlds;
        worlds.reserve(m_worlds.size());

        for (const ObjectUniquePtr<WorldImpl>& worldImpl : m_worlds)
        {
            worlds.emplace_back(worldImpl.getRef());
        }

        return worlds;
    }

    IWorld::WeakRef SceneManagerImpl::findWorld(Uid worldUid) const
    {
        auto iter = eastl::find_if(m_worlds.begin(), m_worlds.end(), [&worldUid](const ObjectUniquePtr<WorldImpl>& world)
        {
            return world->getUid() == worldUid;
        });

        return iter != m_worlds.end() ? (*iter).getRef() : ObjectWeakRef<WorldImpl>{};
    }

    IWorld::WeakRef SceneManagerImpl::createWorld()
    {
        m_worlds.emplace_back(NauObject::classCreateInstance<WorldImpl>());
        return m_worlds.back().getRef();
    }

    void SceneManagerImpl::destroyWorld(IWorld::WeakRef worldRef)
    {
        if (!worldRef)
        {
            return;
        }

        NAU_ASSERT(worldRef.get() != &getDefaultWorld(), "Default world can not be removed");
        if (worldRef.get() == &getDefaultWorld())
        {
            return;
        }

        const auto getWorldNextScene = [&]() -> IScene::WeakRef
        {
            for (auto& sceneEntry : m_scenes)
            {
                if (sceneEntry.scene->getWorld() == worldRef.get())
                {
                    return sceneEntry.scene.getRef();
                }
            }

            return IScene::WeakRef{};
        };

        // deactivateScene properly handle multiple calls for the same scene object.
        // So multiple calls destroyWorld for same world object will be handled automatically:
        // when first destroyWorld call is completed then all worldRefs (from subsequent calls) will become invalid.
        for (IScene::WeakRef sceneRef = getWorldNextScene(); sceneRef; sceneRef = getWorldNextScene())
        {
            deactivateScene(sceneRef);
        }

        if (worldRef)
        {
            auto worldIter = std::find_if(m_worlds.begin(), m_worlds.end(), [worldPtr = worldRef.get()](const ObjectUniquePtr<WorldImpl>& worldUniquePtr)
            {
                return worldUniquePtr.get() == worldPtr;
            });

            NAU_ASSERT(worldIter != m_worlds.end());
            if (worldIter != m_worlds.end())
            {
                m_worlds.erase(worldIter);
            }
        }
    }

    Vector<IScene::WeakRef> SceneManagerImpl::getActiveScenes(const WorldImpl* world) const
    {
        Vector<IScene::WeakRef> scenes;
        scenes.reserve(m_scenes.size());

        for (const SceneEntry& sceneEntry : m_scenes)
        {
            if (world == nullptr || sceneEntry.scene->getWorld()->as<const WorldImpl*>() == world)
            {
                scenes.emplace_back(*sceneEntry.scene);
            }
        }

        return scenes;
    }

    Vector<IScene::WeakRef> SceneManagerImpl::getActiveScenes() const
    {
        NAU_FATAL(!m_worlds.empty());

        return getActiveScenes(m_worlds.front().get());
    }

    void SceneManagerImpl::update(float dt)
    {
        using namespace nau::async;

        NAU_ASSERT(!m_insideUpdate);

        Executor::Ptr prevThisThreadExecutor = Executor::getThisThreadExecutor();
        Executor::setThisThreadExecutor(m_updateWorkQueue);

        m_insideUpdate = true;
        notifyListenerBeginScene();

        scope_on_leave
        {
            m_insideUpdate = false;
            m_postUpdateWorkQueue->poll();
            Executor::setThisThreadExecutor(std::move(prevThisThreadExecutor));

            notifyListenerEndScene();
        };

        m_updateWorkQueue->poll();

        for (auto iter = m_updatableComponents.begin(); iter != m_updatableComponents.end(); ++iter)
        {
            UpdatableComponentEntry& entry = *iter;

            NAU_FATAL(entry.component);
            if (!entry.isActive())
            {
                continue;
            }

            // TODO: this is very temporal solution.
            // m_updatableComponents must keeps components in separated groups related to different worlds.
            if (entry.component->getParentObject().getScene()->getWorld()->isSimulationPaused())
            {
                continue;
            }

            if (entry.componentUpdate)
            {
                entry.componentUpdate->updateComponent(dt);
                if (!entry.isActive())
                {
                    continue;
                }
            }

            if (entry.componentAsyncUpdate && entry.isActive())
            {
                if (!entry.asyncUpdateTask || entry.asyncUpdateTask.isReady())
                {
                    // TODO:
                    // Most likely, using 'dt' in this case is incorrect and a time interval between the previous and current updateComponentAsync calls is required.
                    entry.asyncUpdateTask = entry.componentAsyncUpdate->updateComponentAsync(dt);
                }
            }
        }
    }

    Component* SceneManagerImpl::findComponent(Uid componentUid)
    {
        auto component = m_activeComponents.find(componentUid);
        return component != m_activeComponents.end() ? component->second : nullptr;
    }

    void SceneManagerImpl::pollShutdown()
    {
        using namespace nau::async;
        NAU_ASSERT(!m_insideUpdate);
        auto previousExecutor = Executor::getThisThreadExecutor();
        scope_on_leave
        {
            Executor::setThisThreadExecutor(std::move(previousExecutor));
        };
        Executor::setThisThreadExecutor(m_updateWorkQueue);
        m_updateWorkQueue->poll();
        m_postUpdateWorkQueue->poll();
    }

    async::Task<> SceneManagerImpl::shutdown()
    {
#ifdef NAU_ASSERT_ENABLED
        scope_on_leave
        {
            NAU_ASSERT(m_worlds.size() == 1);
            NAU_ASSERT(m_scenes.empty());
            NAU_ASSERT(m_activeObjects.empty());
            NAU_ASSERT(m_activeComponents.empty());
            NAU_ASSERT(m_updatableComponents.empty());
            NAU_ASSERT(m_asyncTasks.isEmpty());
        };
#endif

        while (!m_scenes.empty())
        {
            deactivateScene(*m_scenes.front().scene);
        }

        co_await m_asyncTasks.awaitCompletion();

        m_worlds.erase(++m_worlds.begin(), m_worlds.end());
    }

    eastl::list<SceneManagerImpl::SceneEntry>::iterator SceneManagerImpl::getSceneIter(IScene* scene)
    {
        if (!scene)
        {
            return m_scenes.end();
        }

        NAU_FATAL(scene->is<SceneImpl>());
        return std::find_if(m_scenes.begin(), m_scenes.end(), [scenePtr = scene->as<const SceneImpl*>()](const auto& otherScene)
        {
            return otherScene.scene.get() == scenePtr;
        });
    }

    inline void SceneManagerImpl::notifyListenerBeginScene()
    {
        if (m_sceneListener)
        {
            m_sceneListener->onSceneBegin();
        }
    }

    inline void SceneManagerImpl::notifyListenerEndScene()
    {
        if (!m_sceneListener)
        {
            return;
        }
        if (!m_changedComponents.empty())
        {
            StackVector<const Component*> components;
            components.resize(m_changedComponents.size());
            eastl::copy(m_changedComponents.begin(), m_changedComponents.end(), components.begin());
            m_changedComponents.clear();

            m_sceneListener->onComponentsChange(eastl::span{components.begin(), components.size()});
        }

        m_sceneListener->onSceneEnd();
    }

    void SceneManagerImpl::notifyListenerComponentWasChanged(const Component& component)
    {
        if (m_sceneListener)
        {
            m_changedComponents.emplace(&component);
        }
    }

    SceneListenerRegistration SceneManagerImpl::addSceneListener(ISceneListener& sceneListener)
    {
        NAU_ASSERT(!m_sceneListener, "Currently only single scene listener is supported");
        if (m_sceneListener)
        {
            return SceneListenerRegistration{};
        }

        m_sceneListener = &sceneListener;
        return SceneListenerRegistration{m_sceneListener};
    }

    void SceneManagerImpl::removeSceneListener(void* sceneListenerHandle)
    {
        NAU_ASSERT(sceneListenerHandle == m_sceneListener);

        if (sceneListenerHandle == m_sceneListener)
        {
            m_sceneListener = nullptr;
        }
    }

    async::Task<IScene::Ptr> openScene(const eastl::string& path)
    {
        AssetRef<> sceneAssetRef{path};

        if (!sceneAssetRef)
        {
            NAU_LOG_WARNING("Scene {} not found!", path.c_str());
            co_return nullptr;
        }

        nau::SceneAsset::Ptr sceneAsset = co_await sceneAssetRef.getAssetViewTyped<SceneAsset>();
        nau::scene::IScene::Ptr scene = getServiceProvider().get<scene::ISceneFactory>().createSceneFromAsset(*sceneAsset);

        co_return scene;
    }
}  // namespace nau::scene
