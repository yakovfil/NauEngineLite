// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <algorithm>
#include <concepts>
#include <tuple>
#include <vector>

#include "nau/diag/assertion.h"
#include "nau/rtti/weak_ptr.h"
#include "nau/serialization/runtime_value.h"
#include "nau/serialization/runtime_value_events.h"

namespace nau::ser_detail
{
    /**
     */
    class RuntimeValueEventsBase : public virtual IRuntimeValueEvents,
                                   public virtual IRuntimeValueEventsSource
    {
        NAU_INTERFACE(nau::ser_detail::RuntimeValueEventsBase, IRuntimeValueEvents, IRuntimeValueEventsSource)

        RuntimeValueEventsBase() = default;

    public:
        SubscriptionHandle subscribeOnChanges(nau::Ptr<IRuntimeValueChangesHandler> handler) final
        {
            NAU_FATAL(handler);
#ifdef NAU_ASSERT_ENABLED
            NAU_ASSERT(m_concurrentCheckFlag.exchange(true, std::memory_order_acquire) == false);
            scope_on_leave
            {
                m_concurrentCheckFlag.store(false, std::memory_order_release);
            };
#endif

            const auto& entry = m_changeHandlers.emplace_back(std::move(handler), ++m_id);
            return SubscriptionHandle{this, std::get<1>(entry)};
        }

        template <std::invocable<const RuntimeValue&, std::string_view> F>
        SubscriptionHandle subscribeOnChanges(F&& functorHandler)
        {
            return IRuntimeValueEvents::subscribeOnChanges(std::forward<F>(functorHandler));
        }

    protected:
        void notifyChanged(const RuntimeValue* source = nullptr) final
        {
            const std::string_view fieldName = source ? findFieldName(*source) : std::string_view{};
            const RuntimeValue& thisAsRuntimeValue = this->as<const RuntimeValue&>();

            notifyHandlers(thisAsRuntimeValue, fieldName);

            if (auto parent = getParent())
            {
                if (auto* const parentEvents = parent->as<IRuntimeValueEventsSource*>())
                {
                    parentEvents->notifyChanged(&thisAsRuntimeValue);
                }
            }
        }

        virtual std::string_view findFieldName(const RuntimeValue& value) const
        {
            return {};
        }

        virtual RuntimeValue* getParent() const
        {
            return nullptr;
        }

        virtual void onThisValueChanged(std::string_view key)
        {
        }

    private:
        using ChangesHandlerEntry = std::tuple<nau::Ptr<IRuntimeValueChangesHandler>, uint32_t>;

        void notifyHandlers(const RuntimeValue& thisAsRuntimeValue, std::string_view childKey)
        {
#ifdef NAU_ASSERT_ENABLED
            NAU_ASSERT(m_concurrentCheckFlag.exchange(true, std::memory_order_acquire) == false);
            scope_on_leave
            {
                m_concurrentCheckFlag.store(false, std::memory_order_release);
            };
#endif

            onThisValueChanged(childKey);

            for (auto& handlerEntry : m_changeHandlers)
            {
                auto& handler = std::get<0>(handlerEntry);
                handler->onValueChanged(thisAsRuntimeValue, childKey);
            }
        }

        void unsubscribe(uint32_t id) final
        {
#ifdef NAU_ASSERT_ENABLED
            NAU_ASSERT(m_concurrentCheckFlag.exchange(true, std::memory_order_acquire) == false);
            scope_on_leave
            {
                m_concurrentCheckFlag.store(false, std::memory_order_release);
            };
#endif
            auto entry = std::find_if(m_changeHandlers.begin(), m_changeHandlers.end(), [id](const ChangesHandlerEntry& entry)
            {
                return std::get<1>(entry) == id;
            });

            if (entry != m_changeHandlers.end())
            {
                m_changeHandlers.erase(entry);
            }
        }

        std::vector<ChangesHandlerEntry> m_changeHandlers;
        uint32_t m_id = 0;

#ifdef NAU_ASSERT_ENABLED
        mutable std::atomic<bool> m_concurrentCheckFlag{false};
#endif
    };

    /**
     */

    class ParentMutabilityGuard final : public virtual IRefCounted
    {
        NAU_CLASS_(nau::ser_detail::ParentMutabilityGuard, IRefCounted)

    public:
        const RuntimeValue::Ptr& getParent() const
        {
            return m_parent;
        }

        ParentMutabilityGuard(RuntimeValue::Ptr parent) :
            m_parent(std::move(parent))
        {
        }

    private:
        const RuntimeValue::Ptr m_parent;
    };

    /**
     */
    class NativeChildValue
    {
        NAU_TYPEID(nau::ser_detail::NativeChildValue)

    public:
        void setParent(nau::Ptr<ParentMutabilityGuard>&& parentGuard)
        {
            NAU_ASSERT(!m_parentGuard);
            m_parentGuard = std::move(parentGuard);
        }

    private:
        RuntimeValue* getParentObject() const
        {
            return m_parentGuard ? m_parentGuard->getParent().get() : nullptr;
        }

        nau::Ptr<ParentMutabilityGuard> m_parentGuard;

        template <std::derived_from<RuntimeValue> T>
        friend class NativeRuntimeValueBase;

        template <std::derived_from<RuntimeValue> T>
        friend class NativePrimitiveRuntimeValueBase;
    };

    struct NAU_ABSTRACT_TYPE NativeParentValue
    {
        NAU_TYPEID(nau::ser_detail::NativeParentValue)

        virtual nau::Ptr<ParentMutabilityGuard> getThisMutabilityGuard() const = 0;
    };

    /**
     */
    template <std::derived_from<RuntimeValue> T>
    class NativePrimitiveRuntimeValueBase : public T,
                                            public NativeChildValue,
                                            public RuntimeValueEventsBase
    {
        NAU_INTERFACE(nau::ser_detail::NativePrimitiveRuntimeValueBase<T>, T, NativeChildValue, RuntimeValueEventsBase)

    private:
        RuntimeValue* getParent() const final
        {
            return this->getParentObject();
        }
    };

    /**
     */
    template <std::derived_from<RuntimeValue> T>
    class NativeRuntimeValueBase : public T,
                                   public NativeChildValue,
                                   public NativeParentValue,
                                   public RuntimeValueEventsBase
    {
        NAU_INTERFACE(nau::ser_detail::NativeRuntimeValueBase<T>, T, NativeChildValue, NativeParentValue, RuntimeValueEventsBase)

    protected:
        template <std::derived_from<RuntimeValue> U>
        nau::Ptr<U>& makeChildValue(nau::Ptr<U>& value) const
        {
            setThisAsParent(value);
            return (value);
        }

        template <std::derived_from<RuntimeValue> U>
        nau::Ptr<U> makeChildValue(nau::Ptr<U>&& value) const
        {
            setThisAsParent(value);
            return value;
        }

        bool hasChildren() const
        {
            return !m_mutabilityGuardRef.isDead();
        }

        template <std::derived_from<RuntimeValue> U>
        void setThisAsParent(nau::Ptr<U>& value) const
        {
            NAU_ASSERT(value);
            if (NativeChildValue* const childValue = value->template as<NativeChildValue*>())
            {
                childValue->setParent(getThisMutabilityGuard());
            }
        }

        // template<typename U>
        // decltype(auto) getMutableThis(const U* self)
        //{
        //     if constexpr (!U::IsMutable && !U::IsReference)
        //     {
        //         using MutableThis = std::remove_const_t<U>;
        //         return const_cast<MutableThis>(self);
        //     }

        //    return self;
        //}

    private:
        RuntimeValue* getParent() const final
        {
            return this->getParentObject();
        }

        nau::Ptr<ParentMutabilityGuard> getThisMutabilityGuard() const final
        {
            auto guard = m_mutabilityGuardRef.lock();
            if (!guard)
            {
                RuntimeValue::Ptr mutableThis = const_cast<NativeRuntimeValueBase<T>*>(this);
                m_mutabilityGuardRef.reset();
                guard = rtti::createInstanceInplace<ParentMutabilityGuard>(m_mutabilityGuardStorage, mutableThis);
                m_mutabilityGuardRef = guard;
            }

            return guard;
        }

        mutable nau::WeakPtr<ParentMutabilityGuard> m_mutabilityGuardRef;
        mutable rtti::InstanceInplaceStorage<ParentMutabilityGuard> m_mutabilityGuardStorage;
    };

}  // namespace nau::ser_detail
