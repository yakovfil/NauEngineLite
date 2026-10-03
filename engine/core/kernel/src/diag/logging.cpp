// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/diag/logging.h"

#include <algorithm>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "nau/diag/assertion.h"
#include "nau/memory/singleton_memop.h"
#include "nau/threading/lock_guard.h"

namespace nau::diag
{
    class LoggerImpl final : public Logger,
                             public std::enable_shared_from_this<LoggerImpl>
    {
    public:
        ~LoggerImpl()
        {
        }

        SubscriptionHandle subscribeImpl(ILogSubscriber::Ptr subscriber, ILogMessageFilter::Ptr) override;

        void releaseSubscriptionImpl(uint32_t subscriptionId) override;

        void setFilterImpl(const SubscriptionHandle& handle, ILogMessageFilter::Ptr) override;

        void logMessage(LogLevel criticality, std::vector<std::string> tags, SourceInfo sourceInfo, std::string text) override;

    private:
        struct SubscriberEntry
        {
            ILogSubscriber::Ptr subscriber;
            ILogMessageFilter::Ptr filter;
            uint32_t id;

            SubscriberEntry(ILogSubscriber::Ptr inSubscriber, ILogMessageFilter::Ptr inFilter, uint64_t inIndex) :
                subscriber(std::move(inSubscriber)),
                filter(std::move(inFilter)),
                id(inIndex)
            {
            }

            inline void operator()(const LoggerMessage& message) const
            {
                // assert subscriber
                if (!filter || filter->acceptMessage(message))
                {
                    subscriber->processMessage(message);
                }
            }
        };

        std::shared_mutex m_mutex;
        std::atomic_uint32_t m_messageIndex = 0;
        uint32_t m_subscriberId = 0;
        std::list<SubscriberEntry> m_subscribers;
    };

    Logger::SubscriptionHandle LoggerImpl::subscribeImpl(ILogSubscriber::Ptr subscriber, ILogMessageFilter::Ptr filter)
    {
        lock_(m_mutex);

        m_subscribers.emplace_back(std::move(subscriber), std::move(filter), ++m_subscriberId);
        return makeSubscriptionHandle(shared_from_this(), m_subscribers.back().id);
    }

    void LoggerImpl::releaseSubscriptionImpl(uint32_t subscriptionId)
    {
        if (subscriptionId == 0)
        {
            return;
        }

        lock_(m_mutex);

        auto iter = std::find_if(m_subscribers.begin(), m_subscribers.end(), [subscriptionId](const SubscriberEntry& entry)
        {
            return entry.id == subscriptionId;
        });
        NAU_ASSERT(iter != m_subscribers.end());

        if (iter != m_subscribers.end())
        {
            m_subscribers.erase(iter);
        }
    }

    void LoggerImpl::setFilterImpl(const SubscriptionHandle& handle, ILogMessageFilter::Ptr)
    {
        lock_(m_mutex);
    }

    void LoggerImpl::logMessage(LogLevel criticality, std::vector<std::string> tags, SourceInfo sourceInfo, std::string text)
    {
        static thread_local unsigned recursionCounter = 0;
        static thread_local std::vector<LoggerMessage> pendingMessages;

        // preventing logMessage from recursive call (ever subscriber can subsequently invoke logging):
        ++recursionCounter;
        scope_on_leave
        {
            NAU_FATAL(recursionCounter > 0);
            --recursionCounter;
        };

        LoggerMessage message{
            .index = m_messageIndex.fetch_add(1, std::memory_order_relaxed),
            .time = std::time(nullptr),
            .level = criticality,
            .tags = std::move(tags),
            .source = sourceInfo,
            .data = std::move(text)};

        if (recursionCounter > 1)
        {
            pendingMessages.emplace_back(std::move(message));
            return;
        }

        const std::shared_lock lock{m_mutex};

        for (auto& subscriber : m_subscribers)
        {
            subscriber(message);
        }

        while (!pendingMessages.empty())
        {
            auto messages = std::move(pendingMessages);
            pendingMessages.clear();

            for (const auto& message : messages)
            {
                for (auto& subscriber : m_subscribers)
                {
                    subscriber(message);
                }
            }
        }
    }

    Logger::SubscriptionHandle::SubscriptionHandle(Logger::Ptr&& logger, uint32_t id) :
        m_logger(std::move(logger)),
        m_id(id)
    {
    }

    Logger::SubscriptionHandle::SubscriptionHandle(SubscriptionHandle&& other) :
        m_logger(std::move(other.m_logger)),
        m_id(std::exchange(other.m_id, 0))
    {
        NAU_ASSERT(other.m_logger.expired());
    }

    Logger::SubscriptionHandle::~SubscriptionHandle()
    {
        release();
    }

    Logger::SubscriptionHandle& Logger::SubscriptionHandle::operator=(SubscriptionHandle&& other)
    {
        release();

        m_logger = std::move(other.m_logger);
        m_id = std::exchange(other.m_id, 0);

        NAU_ASSERT(other.m_logger.expired());

        return *this;
    }

    Logger::SubscriptionHandle::operator bool() const
    {
        return !m_logger.expired() && m_id > 0;
    }

    void Logger::SubscriptionHandle::release()
    {
        if (auto logger = m_logger.lock())
        {
            const auto id = std::exchange(m_id, 0);
            m_logger.reset();

            static_cast<LoggerImpl*>(logger.get())->releaseSubscriptionImpl(id);
        }
    }

    namespace
    {
        Logger::Ptr& getLoggerRef()
        {
            static Logger::Ptr s_logger;
            return (s_logger);
        }
    }  // namespace

    Logger::Ptr createLogger()
    {
        return std::make_shared<LoggerImpl>();
    }

    void setLogger(Logger::Ptr&& logger)
    {
        NAU_ASSERT(!logger || !getLoggerRef(), "Logger instance already set");
        getLoggerRef() = std::move(logger);
    }

    Logger& getLogger()
    {
        auto& logger = getLoggerRef();
        NAU_FATAL(logger);
        return *logger;
    }

    bool hasLogger()
    {
        return static_cast<bool>(getLoggerRef());
    }

    Logger::SubscriptionHandle Logger::makeSubscriptionHandle(Logger::Ptr&& logger, uint32_t id)
    {
        return {std::move(logger), id};
    }

}  // namespace nau::diag
