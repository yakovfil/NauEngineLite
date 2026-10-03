// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

// Core assert system header.
#pragma once

#include <stdlib.h>

#include <atomic>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "nau/diag/source_info.h"
#include "nau/kernel/kernel_config.h"
#include "nau/rtti/rtti_object.h"
#include "nau/utils/functor.h"

namespace nau::diag_detail
{
    template <typename, typename>
    struct CompatibleUniquePtr : std::false_type
    {
    };

    template <typename T, typename U>
    struct CompatibleUniquePtr<std::shared_ptr<T>, U> : std::bool_constant<std::is_assignable_v<U&, T&>>
    {
    };

}  // namespace nau::diag_detail

namespace nau::diag
{
    struct IMessageFilter;
    struct ILogSubscriber;
    struct ILogSubscriber;

    enum class LogLevel : uint16_t
    {
        /** Prints a info in Debug configuration to console (and log file) */
        Debug,

        /** Prints a info to console (and log file) */
        Info,

        /** Prints a warning to console (and log file) */
        Warning,

        /** Sends an error and crashes in release builds */
        Error,

        /** Sends a fatal error and crashes */
        Critical,

        /** Sends a verbose message (if Verbose logging is enabled), usually used for detailed logging. */
        Verbose,
    };

    struct LoggerMessage
    {
        uint32_t index;
        int64_t time;
        LogLevel level;
        std::vector<std::string> tags;
        SourceInfo source;
        std::string data;
    };

    struct NAU_ABSTRACT_TYPE ILogSubscriber
    {
        NAU_TYPEID(nau::diag::ILogSubscriber)

        using Ptr = std::shared_ptr<ILogSubscriber>;

        virtual ~ILogSubscriber() = default;

        virtual void processMessage(const LoggerMessage& message) = 0;
    };

    struct NAU_ABSTRACT_TYPE ILogMessageFilter
    {
        NAU_TYPEID(nau::diag::ILogMessageFilter)

        using Ptr = std::shared_ptr<ILogMessageFilter>;

        virtual ~ILogMessageFilter() = default;

        virtual bool acceptMessage(const LoggerMessage& message) = 0;
    };

    template <typename T>
    constexpr bool IsInvocableLogSubscriber = std::is_invocable_v<T, const LoggerMessage&>;

    template <typename T>
    constexpr bool IsInvocableLogMessageFilter = std::is_invocable_r_v<bool, T, const LoggerMessage&>;

    template <typename T>
    concept LogSubscriberConcept = IsInvocableLogSubscriber<T> || diag_detail::CompatibleUniquePtr<T, ILogSubscriber>::value;

    template <typename T>
    concept LogFilterConcept = IsInvocableLogMessageFilter<T> || diag_detail::CompatibleUniquePtr<T, ILogMessageFilter>::value || std::is_null_pointer_v<T>;

    /**
     */
    class NAU_ABSTRACT_TYPE Logger
    {
    public:
        using Ptr = std::shared_ptr<Logger>;

        class NAU_KERNEL_EXPORT [[nodiscard]] SubscriptionHandle
        {
        public:
            SubscriptionHandle() = default;
            SubscriptionHandle(SubscriptionHandle&&);
            SubscriptionHandle(const SubscriptionHandle&) = delete;
            ~SubscriptionHandle();

            SubscriptionHandle& operator=(SubscriptionHandle&&);
            SubscriptionHandle& operator=(const SubscriptionHandle&) = delete;
            inline SubscriptionHandle& operator=(std::nullptr_t)
            {
                release();
                return *this;
            }

            explicit operator bool() const;

            void release();

        private:
            SubscriptionHandle(Logger::Ptr&&, uint32_t);

            std::weak_ptr<Logger> m_logger;
            uint32_t m_id = 0;

            friend Logger;
        };

        virtual ~Logger() = default;

        virtual void logMessage(LogLevel criticality, std::vector<std::string> tags, SourceInfo sourceInfo, std::string text) = 0;

        template <LogSubscriberConcept TSubscriber, LogFilterConcept TFilter = std::nullptr_t>
        SubscriptionHandle subscribe(TSubscriber subscriber, TFilter filter = nullptr);

        template <LogFilterConcept TFilter>
        void setFilter(const SubscriptionHandle& handle, TFilter filter);

        void resetFilter(const SubscriptionHandle& handle);

    protected:
        virtual SubscriptionHandle subscribeImpl(ILogSubscriber::Ptr subscriber, ILogMessageFilter::Ptr = nullptr) = 0;

        virtual void releaseSubscriptionImpl(uint32_t subscriptionId) = 0;

        virtual void setFilterImpl(const SubscriptionHandle& handle, ILogMessageFilter::Ptr) = 0;

        static class SubscriptionHandle makeSubscriptionHandle(Logger::Ptr&&, uint32_t);

    private:
        template <LogFilterConcept TFilter>
        static ILogMessageFilter::Ptr makeLogMessageFilterPtr(TFilter filter);
    };

    NAU_KERNEL_EXPORT Logger::Ptr createLogger();

    NAU_KERNEL_EXPORT void setLogger(Logger::Ptr&&);

    NAU_KERNEL_EXPORT Logger& getLogger();

    NAU_KERNEL_EXPORT bool hasLogger();

}  // namespace nau::diag

namespace nau::diag_detail
{
    template <typename F>
    requires(diag::IsInvocableLogSubscriber<F>)
    class FunctionalLogSubscriber final : public diag::ILogSubscriber
    {
    public:
        FunctionalLogSubscriber(F&& callback) :
            m_callback(std::move(callback))
        {
        }

    private:
        void processMessage(const diag::LoggerMessage& message) override
        {
            // assert m_callback
            m_callback(message);
        }

        nau::Functor<void(const diag::LoggerMessage&)> m_callback;
    };

    template <typename F>
    requires(diag::IsInvocableLogMessageFilter<F>)
    class FunctionalMessageFilter final : public diag::ILogMessageFilter
    {
    public:
        FunctionalMessageFilter(F&& callback) :
            m_callback(std::move(callback))
        {
        }

    private:
        bool acceptMessage(const diag::LoggerMessage& message) override
        {
            // assert m_callback
            return m_callback(message);
        }

        nau::Functor<bool(const diag::LoggerMessage&)> m_callback;
    };

    struct InplaceLogData
    {
        diag::LogLevel level;
        diag::SourceInfo sourceInfo;

        InplaceLogData(diag::LogLevel inLevel, diag::SourceInfo inSourceInfo) :
            level(inLevel),
            sourceInfo(inSourceInfo)
        {
        }

        template <typename S, typename... Args>
        void operator()(std::vector<std::string> tags, S&& formatStr, Args&&... args)
        {
            if constexpr (sizeof...(Args) > 0)
            {
                auto message = nau::utils::format(formatStr, std::forward<Args>(args)...);
                diag::getLogger().logMessage(level, std::move(tags), sourceInfo, std::move(message));
            }
            else
            {
                auto message = nau::utils::format(formatStr);
                diag::getLogger().logMessage(level, std::move(tags), sourceInfo, std::move(message));
            }
        }

        template <typename S, typename... Args>
        void operator()(S&& formatStr, Args&&... args)
        {
            operator()(std::vector<std::string>{}, std::forward<S>(formatStr), std::forward<Args>(args)...);
        }
    };

}  // namespace nau::diag_detail

namespace nau::diag
{
    template <LogFilterConcept TFilter>
    ILogMessageFilter::Ptr Logger::makeLogMessageFilterPtr(TFilter filter)
    {
        if constexpr (IsInvocableLogMessageFilter<TFilter>)
        {
            using MessageFilter = diag_detail::FunctionalMessageFilter<TFilter>;
            return std::make_shared<MessageFilter>(std::move(filter));
        }
        else if constexpr (std::is_null_pointer_v<TFilter>)
        {
            return nullptr;
        }
        else
        {
            return std::move(filter);
        }
    }

    template <LogSubscriberConcept TSubscriber, LogFilterConcept TFilter>
    Logger::SubscriptionHandle Logger::subscribe(TSubscriber subscriber, TFilter filter)
    {
        ILogSubscriber::Ptr subscriberPtr;
        if constexpr (IsInvocableLogSubscriber<TSubscriber>)
        {
            using Subscriber = diag_detail::FunctionalLogSubscriber<TSubscriber>;
            subscriberPtr = std::make_shared<Subscriber>(std::move(subscriber));
        }
        else
        {
            subscriberPtr = subscriber;
        }

        auto filterPtr = makeLogMessageFilterPtr(filter);

        return subscribeImpl(subscriberPtr, filterPtr);
    }

    template <LogFilterConcept TFilter>
    void Logger::setFilter(const SubscriptionHandle& handle, TFilter filter)
    {
        setFilterImpl(handle, makeLogMessageFilterPtr(std::move(filter)));
    }

    inline void Logger::resetFilter(const SubscriptionHandle& handle)
    {
        setFilterImpl(handle, nullptr);
    }

}  // namespace nau::diag

// clang-format off

#define NAU_LOG_MESSAGE(criticality) \
    ::nau::diag_detail::InplaceLogData{ criticality, NAU_INLINED_SOURCE_INFO }

#define NAU_LOG_INFO \
    ::nau::diag_detail::InplaceLogData{ ::nau::diag::LogLevel::Info, NAU_INLINED_SOURCE_INFO }

#define NAU_LOG_DEBUG \
    ::nau::diag_detail::InplaceLogData{ ::nau::diag::LogLevel::Debug, NAU_INLINED_SOURCE_INFO }

#define NAU_LOG_WARNING \
    ::nau::diag_detail::InplaceLogData{ ::nau::diag::LogLevel::Warning, NAU_INLINED_SOURCE_INFO }

#define NAU_LOG_ERROR \
    ::nau::diag_detail::InplaceLogData{ ::nau::diag::LogLevel::Error, NAU_INLINED_SOURCE_INFO }

#define NAU_LOG_CRITICAL \
    ::nau::diag_detail::InplaceLogData{ ::nau::diag::LogLevel::Critical, NAU_INLINED_SOURCE_INFO }

#ifdef  NAU_VERBOSE_LOG
#define NAU_LOG_VERBOSE \
    ::nau::diag_detail::InplaceLogData{ ::nau::diag::LogLevel::Verbose, NAU_INLINED_SOURCE_INFO }
#else
#define NAU_LOG_VERBOSE(tags, formatStr, ...) \
    do {        }                             \
    while(false)
#endif

#define NAU_LOG \
    ::nau::diag_detail::InplaceLogData{ ::nau::diag::LogLevel::Debug, NAU_INLINED_SOURCE_INFO }

// clang-format on

#define NAU_CONDITION_LOG(condition, criticality, tags, formatStr, ...) \
    {                                                                   \
        if (!!(condition))                                              \
        {                                                               \
            NAU_LOG_MESSAGE(criticality)                                \
            (tags, formatStr, ##__VA_ARGS__);                           \
        }                                                               \
    }

#define NAU_ENSURE_LOG(criticality, tags, formatStr, ...)                  \
    {                                                                      \
        static std::atomic<bool> s_wasTrigered = false;                    \
        bool value = s_wasTrigered.load(std::memory_order_relaxed);        \
        if (!value && !s_wasTrigered.compare_exchange_strong(value, true)) \
        {                                                                  \
            NAU_LOG_MESSAGE(criticality)                                   \
            (tags, formatStr, ##__VA_ARGS__);                              \
        }                                                                  \
    }
