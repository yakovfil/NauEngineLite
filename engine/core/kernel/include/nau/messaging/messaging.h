// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// nau/messaging/messaging.h

#pragma once

#include <mutex>
#include <string_view>

#include "nau/async/task.h"
#include "nau/kernel/kernel_config.h"
#include "nau/messaging/async_message_stream.h"
#include "nau/meta/function_info.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/runtime/disposable.h"
#include "nau/serialization/runtime_value_builder.h"
#include "nau/utils/cancellation.h"
#include "nau/utils/functor.h"

namespace nau::nau_detail
{
    template <typename>
    struct MessageHandlerArgument;

    template <typename T>
    struct MessageHandlerArgument<TypeList<T>>
    {
        using type = T;
    };

    template <>
    struct MessageHandlerArgument<TypeList<>>
    {
        using type = void;
    };

    /**
     */
    template <typename T>
    class MessageDeclaration
    {
    public:
        using ValueType = T;

        MessageDeclaration(const char streamName[]) :
            m_streamName(streamName)
        {
        }

        std::string_view getStreamName() const
        {
            return m_streamName;
        }

        operator std::string_view() const
        {
            return m_streamName;
        }

    private:
        const std::string_view m_streamName;
    };

}  // namespace nau::nau_detail

namespace nau
{
    NAU_KERNEL_EXPORT
    AsyncMessageSource& getBroadcaster();

    /**
     */
    class [[nodiscard]] NAU_KERNEL_EXPORT AsyncMessageSubscription final : public IDisposable,
                                                                           public IAsyncDisposable
    {
        NAU_RTTI_CLASS(nau::AsyncMessageSubscription, IDisposable, IAsyncDisposable)

    public:
        AsyncMessageSubscription();

        AsyncMessageSubscription(AsyncMessageSubscription&&);

        AsyncMessageSubscription(const AsyncMessageSubscription&) = delete;

        template <typename Callable>
        AsyncMessageSubscription(AsyncMessageSource&, std::string_view streamName, Callable handler, async::Executor::Ptr);

        ~AsyncMessageSubscription();

        AsyncMessageSubscription& operator=(AsyncMessageSubscription&&);

        AsyncMessageSubscription& operator=(std::nullptr_t);

        AsyncMessageSubscription& operator=(const AsyncMessageSubscription&) = delete;

        explicit operator bool() const;

        void dispose() override;

        async::Task<> disposeAsync() override;

    private:
        template <typename Callable>
        async::Task<> runStreamListener(AsyncMessageStream stream, Callable handler, async::Executor::Ptr executor, Cancellation cancellation);

        template <typename Callable, typename ResultType>
        static ResultType invokeHandler(Callable& handler, RuntimeValue::Ptr messageValue);

        async::Task<> m_task;
        CancellationSource m_cancellationSource;
    };

    /**
     */
    template <typename T>
    class MessageDeclaration : public nau_detail::MessageDeclaration<T>
    {
    public:
        using nau_detail::MessageDeclaration<T>::MessageDeclaration;

        inline void post(AsyncMessageSource& broadcaster, T value) const
        {
            broadcaster.post(this->getStreamName(), nau::makeValueCopy(std::move(value)));
        }

        template <typename Callable>
        requires(std::is_invocable_r_v<void, Callable, T> || std::is_invocable_r_v<async::Task<>, Callable, T>)
        inline AsyncMessageSubscription subscribe(AsyncMessageSource& broadcaster, Callable handler, async::Executor::Ptr executor = nullptr) const
        {
            return AsyncMessageSubscription{broadcaster, this->getStreamName(), std::move(handler), std::move(executor)};
        }
    };

    /**
     */
    template <>
    class MessageDeclaration<void> : public nau_detail::MessageDeclaration<void>
    {
    public:
        using nau_detail::MessageDeclaration<void>::MessageDeclaration;

        inline void post(AsyncMessageSource& broadcaster = getBroadcaster()) const
        {
            broadcaster.post(this->getStreamName());
        }

        template <typename Callable>
        requires(std::is_invocable_r_v<void, Callable> || std::is_invocable_r_v<async::Task<>, Callable>)
        inline AsyncMessageSubscription subscribe(AsyncMessageSource& broadcaster, Callable handler, async::Executor::Ptr executor = nullptr) const
        {
            return AsyncMessageSubscription{broadcaster, this->getStreamName(), std::move(handler), std::move(executor)};
        }
    };

    template <typename Callable>
    AsyncMessageSubscription::AsyncMessageSubscription(AsyncMessageSource& source, const std::string_view streamName, Callable handler, async::Executor::Ptr executor)
    {
        AsyncMessageStream stream = source.getStream(streamName);
        NAU_FATAL(stream);

        m_task = runStreamListener(std::move(stream), std::move(handler), std::move(executor), m_cancellationSource.getCancellation());
    }

    template <typename Callable>
    async::Task<> AsyncMessageSubscription::runStreamListener(AsyncMessageStream stream, Callable handler, async::Executor::Ptr executor, Cancellation cancellation)
    {
        using namespace nau::async;

        using CallableInfo = meta::template GetCallableTypeInfo<Callable>;

        using ResultType = typename CallableInfo::Result;

        static_assert(std::is_same_v<ResultType, void> || std::is_same_v<ResultType, Task<>>);

        if (executor)
        {  // must release executor as soon as possible
            scope_on_leave
            {
                executor.reset();
            };

            co_await executor;
        }

        while (!cancellation.isCancelled())
        {
            Task<RuntimeValue::Ptr> task = stream.getNextMessage();

            if (!task.isReady())
            {
                task.detach();
                // WA for coroutine related MSVC compiler issue error C7587: 'co_await' cannot appear in an unevaluated context
                auto result = co_await async::whenAny(cancellation, task);
                if (!result)
                {
                    co_return;
                }
            }

            if (task.isRejected())
            {
                co_yield task.getError();
            }

            RuntimeValue::Ptr message = *std::move(task);
            if constexpr (std::is_same_v<ResultType, void>)
            {
                invokeHandler<Callable, void>(handler, std::move(message));
            }
            else
            {
                Task<> continuation = invokeHandler<Callable, Task<>>(handler, std::move(message));
                if (continuation)
                {
                    co_await continuation;
                }
            }
        }
    }

    template <typename Callable, typename ResultType>
    ResultType AsyncMessageSubscription::invokeHandler(Callable& handler, RuntimeValue::Ptr messageValue)
    {
        using CallableInfo = meta::template GetCallableTypeInfo<Callable>;
        static_assert(CallableInfo::ParametersList::Size < 2, "Invalid handler arguments count. Expected zero or one.");

        using ArgumentType = std::decay_t<typename nau_detail::MessageHandlerArgument<typename CallableInfo::ParametersList>::type>;

        if constexpr (std::is_same_v<ArgumentType, void>)
        {
            return handler();
        }
        else
        {
            if (messageValue)
            {
                if constexpr (rtti::HasTypeInfo<ArgumentType>)
                {
                    RuntimeNativeValue* const nativeValue = messageValue->as<RuntimeNativeValue*>();
                    const rtti::TypeInfo* const messageTypeInfo = nativeValue ? nativeValue->getValueTypeInfo() : nullptr;

                    if (messageTypeInfo && (*messageTypeInfo == rtti::getTypeInfo<ArgumentType>()))
                    {
                        const ArgumentType& arg = nativeValue->getReadonlyRef<ArgumentType>();
                        return handler(arg);
                    }
                }

                if (Result<ArgumentType> arg = nau::runtimeValueCast<ArgumentType>(messageValue); arg)
                {
                    return handler(*std::move(arg));
                }
            }
            else
            {
                // ArgumentType argValue = nau::runtimeValueCast<ArgumentType>(message);
            }

            // eliminate compilation errors. In real never should ne here
            if constexpr (!std::is_same_v<ResultType, void>)
            {
                return ResultType{};
            }
        }
    }

}  // namespace nau

#define NAU_DECLARE_MESSAGE(Descriptor, StreamName, ValueType)   \
    inline const ::nau::MessageDeclaration<ValueType> Descriptor \
    {                                                            \
        StreamName                                               \
    }

#define NAU_DECLARE_SIGNAL_MESSAGE(Descriptor, StreamName) NAU_DECLARE_MESSAGE(Descriptor, StreamName, void)
