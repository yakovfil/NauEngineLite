// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <concepts>
#include <type_traits>

#include "nau/async/task_base.h"
#include "nau/async/task_collection.h"
#include "nau/dispatch/dynamic_object_impl.h"
#include "nau/meta/attribute.h"
#include "nau/runtime/disposable.h"
#include "nau/scene/nau_object.h"
#include "nau/utils/functor.h"
#include "nau/utils/intrusive_list.h"

namespace nau::scene_internal
{
    struct ComponentListNode : nau::IntrusiveListNode
    {
    };

}  // namespace nau::scene_internal

namespace nau::scene
{
    class SceneObject;
    class SceneManagerImpl;

    /**
     */
    class NAU_ABSTRACT_TYPE NAU_CORESCENE_EXPORT Component : public NauObject,
                                                             public DynamicObjectImpl,
                                                             private scene_internal::ComponentListNode
    {
        NAU_INTERFACE(nau::scene::Component, NauObject, DynamicObjectImpl)

    public:
        ~Component();

        /**
         * @brief Detaches the component from the parent object and the scene and destroys it.
         */
        void destroy() final;

        /**
         * @brief Retrieves the object which this component is attached to.
         *
         * @note It is assumed that a component always has an owner object.
         */
        SceneObject& getParentObject() const;

        /**
         * @brief Checks whether the component is operable.
         *
         * @note    A component is live when it is attached to the scene hierarchy.
         *          However, it is possible that after deleting a component, when the component is still active,
         *          the system is waiting for the completion of asynchronous operations associated with this component,
         *          meanwhile any interaction between the component and scene is impossible since it has been deleted.
         *          For such cases, it is necessary to check the state of the component via this method.
         */
        bool isOperable() const;

        ActivationState getActivationState() const;

    protected:
        Component();

        // TODO: clone operation
        Component(const Component&) = delete;
        Component& operator=(const Component&) = delete;

        /**
         * @brief Schedules a callable for an asynchronous call.
         *
         * @tparam Callable Callable type.
         * @tparam Args     Parameter pack for the callable.
         *
         * @param [in] callable Functor object or a pointer to the function to schedule.
         * @param [in] args     Arguments for **callable** call.
         */
        template <typename Callable, typename... Args>
        requires(std::is_invocable_r_v<async::Task<>, Callable, Args...>)
        void runAsync(Callable callable, Args&&... args);

        /**
         * @brief Pushes the (active) task into the list of asynchronous operations of the component.
         *
         * @tparam T Task return value type.
         *
         * @param [in] task Task to schedule.
         *
         * The component are not actually deleted until all tracked operations have been completed.
         */
        template <typename T>
        void trackAsyncOperation(async::Task<T>&& task);

        void onBeforeDeleteObject() override;

    private:
        void addRef() final;
        void releaseRef() final;
        nau::IWeakRef* getWeakRef() final;
        uint32_t getRefsCount() const final;

        void changeActivationState(ActivationState newState);

        /**
         * @brief Flushes all async tasks associated with the object.
         *
         * @return Task object, which can provide finalization status.
         */
        async::Task<> finalizeAsyncOperations();

        void onThisValueChanged(std::string_view key) final;

    protected:
        SceneObject* m_parentObject = nullptr;
        ActivationState m_activationState = ActivationState::Inactive;
        async::TaskCollection m_asyncTasks;

    private:
        // TODO: used for notification about value changes.
        //  should not be used (excluded) when scene listener support is off.
        SceneManagerImpl* m_sceneManager = nullptr;

        friend SceneObject;
        friend class SceneManagerImpl;
    };

    template <typename Callable, typename... Args>
    requires(std::is_invocable_r_v<async::Task<>, Callable, Args...>)
    void Component::runAsync(Callable callable, Args&&... args)
    {
        using namespace nau::async;

        // Using outer task wrapper to keep callable object alive while fabricated task is running.
        // This is needed to allow using lambda's captured environment during task life time.
        Task<> task = [](auto callable, Args&&... args) -> Task<>
        {
            auto task = callable(std::forward<Args>(args)...);
            co_await task;
        }(std::move(callable), std::forward<Args>(args)...);

        m_asyncTasks.push(std::move(task));
    }

    template <typename T>
    void Component::trackAsyncOperation(async::Task<T>&& task)
    {
        m_asyncTasks.push(std::move(task));
    }
}  // namespace nau::scene

#define NAU_COMPONENT(ComponentType, ...)  \
    NAU_OBJECT(ComponentType, __VA_ARGS__) \
    NAU_DECLARE_DYNAMIC_OBJECT

#define NAU_IMPLEMENT_COMPONENT(ComponentType) \
    NAU_IMPLEMENT_DYNAMIC_OBJECT(ComponentType)