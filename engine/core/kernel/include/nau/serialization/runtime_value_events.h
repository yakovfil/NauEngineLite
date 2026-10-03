// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <atomic>
#include <concepts>
#include <string_view>
#include <type_traits>

#include "nau/diag/assertion.h"
#include "nau/kernel/kernel_config.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/rtti/weak_ptr.h"
#include "nau/serialization/runtime_value.h"
#include "nau/threading/lock_guard.h"
#include "nau/utils/preprocessor.h"
#include "nau/utils/scope_guard.h"

namespace nau
{
    /**
     */
    struct NAU_ABSTRACT_TYPE IRuntimeValueChangesHandler : virtual IRefCounted
    {
        NAU_INTERFACE(nau::IRuntimeValueChangesHandler, IRefCounted)

        virtual void onValueChanged(const RuntimeValue& target, std::string_view childKey) = 0;
    };

    /**
     */
    struct NAU_ABSTRACT_TYPE IRuntimeValueEvents : virtual IRefCounted
    {
        NAU_INTERFACE(nau::IRuntimeValueEvents, IRefCounted)

        class [[nodiscard]] SubscriptionHandle
        {
        public:
            SubscriptionHandle() = default;

            SubscriptionHandle(nau::Ptr<IRuntimeValueEvents> value, uint32_t uid) :
                m_valueRef(std::move(value)),
                m_uid(uid)
            {
            }

            SubscriptionHandle(const SubscriptionHandle&) = delete;

            SubscriptionHandle(SubscriptionHandle&& other) :
                m_valueRef(std::move(other.m_valueRef)),
                m_uid(std::exchange(other.m_uid, 0))
            {
            }

            ~SubscriptionHandle()
            {
                reset();
            }

            SubscriptionHandle& operator=(const SubscriptionHandle&) = delete;

            SubscriptionHandle& operator=(SubscriptionHandle&& other)
            {
                reset();

                m_valueRef = std::move(other.m_valueRef);
                m_uid = std::exchange(other.m_uid, 0);

                return *this;
            }

            SubscriptionHandle& operator=(std::nullptr_t)
            {
                reset();
                return *this;
            }

            void reset()
            {
#ifdef NAU_ASSERT_ENABLED
                scope_on_leave
                {
                    NAU_ASSERT(!m_valueRef);
                    NAU_ASSERT(m_uid == 0);
                };
#endif
                if (!m_valueRef)
                {
                    m_uid = 0;
                    return;
                }

                auto valueRef = std::move(m_valueRef);
                if (const auto uid = std::exchange(m_uid, 0); uid > 0)
                {
                    if (const auto value = valueRef.lock())
                    {
                        value->unsubscribe(uid);
                    }
                }
            }

            explicit operator bool() const
            {
                return m_uid != 0;
            }

        private:
            nau::WeakPtr<IRuntimeValueEvents> m_valueRef;
            uint32_t m_uid = 0;
        };

        virtual SubscriptionHandle subscribeOnChanges(nau::Ptr<IRuntimeValueChangesHandler>) = 0;

        template <std::invocable<const RuntimeValue&, std::string_view> F>
        SubscriptionHandle subscribeOnChanges(F&& functorHandler);

    protected:
        virtual void unsubscribe(uint32_t id) = 0;
    };

    /**
     */
    struct NAU_ABSTRACT_TYPE IRuntimeValueEventsSource : virtual IRefCounted
    {
        NAU_INTERFACE(nau::IRuntimeValueEventsSource, IRefCounted)

        virtual void notifyChanged(const RuntimeValue* source = nullptr) = 0;
    };

}  // namespace nau

namespace nau::ser_detail
{
    template <std::invocable<const RuntimeValue&, std::string_view> F>
    class ChangesHandlerFunctorWrapper final : public nau::IRuntimeValueChangesHandler
    {
        NAU_CLASS_(nau::ser_detail::ChangesHandlerFunctorWrapper<F>, nau::IRuntimeValueChangesHandler)

    public:
        ChangesHandlerFunctorWrapper(F&& handler) :
            m_handler(std::move(handler))
        {
        }

    private:
        void onValueChanged(const RuntimeValue& target, std::string_view childKey) override
        {
            m_handler(target, childKey);
        }

        F m_handler;
    };

    template <typename F>
    ChangesHandlerFunctorWrapper(F) -> ChangesHandlerFunctorWrapper<F>;

    struct ValueChangesScopeHelper
    {
        IRuntimeValueEventsSource& value;

        ValueChangesScopeHelper(IRuntimeValueEventsSource& inValue) :
            value(inValue)
        {
        }

        ~ValueChangesScopeHelper()
        {
            value.notifyChanged();
        }
    };

}  // namespace nau::ser_detail

namespace nau
{
    template <std::invocable<const RuntimeValue&, std::string_view> F>
    inline IRuntimeValueEvents::SubscriptionHandle IRuntimeValueEvents::subscribeOnChanges(F&& functorHandler)
    {
        using Handler = ser_detail::ChangesHandlerFunctorWrapper<F>;

        auto handlerObject = rtti::createInstance<Handler, IRuntimeValueChangesHandler>(std::move(functorHandler));
        return subscribeOnChanges(std::move(handlerObject));
    }
}  // namespace nau

// clang-format off
#define value_changes_scope const ::nau::ser_detail::ValueChangesScopeHelper ANONYMOUS_VAR(changesScope__) {*this}
// clang-format on
