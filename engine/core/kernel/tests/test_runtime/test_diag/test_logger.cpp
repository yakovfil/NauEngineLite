// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// test_asserts.cpp

#include <memory>
#include <string>
#include <vector>

#include "nau/diag/logging.h"
#include "test_diag.h"

namespace nau::test
{
    namespace
    {
        class TestLogSubscriber final : public diag::ILogSubscriber
        {
            std::vector<uint8_t> messages;

        public:
            TestLogSubscriber(size_t messages) :
                messages(messages, false)
            {
            }
            void processMessage(const nau::diag::LoggerMessage& message) override
            {
                if (message.index < messages.size())
                {
                    messages[message.index]++;
                }
            }

            bool checkMessages()
            {
                bool result = true;
                for (auto& counter : messages)
                {
                    result &= (counter == 1);
                    if (counter != 1)
                        break;
                }
                return result;
            }
            bool checkMessage(size_t index)
            {
                NAU_ASSERT(index < messages.size());
                return messages[index] == 1;
            }
        };

        class TestLogMessageFilter final : public diag::ILogMessageFilter
        {
            uint32_t m_mod = 0;
            std::atomic_uint32_t m_totalMessages = 0;
            diag::LogLevel m_criticality = diag::LogLevel::Debug;

        public:
            void setMod(size_t newMod)
            {
                m_mod = newMod;
            }
            uint32_t getTotalMessages()
            {
                return m_totalMessages.load();
            }
            void setLevel(nau::diag::LogLevel level)
            {
                m_criticality = level;
            }
            bool acceptMessage(const nau::diag::LoggerMessage& message) override
            {
                m_totalMessages++;
                return ((m_mod == 0) || ((message.index % m_mod) == 0)) && (message.level == m_criticality);
            }
        };

        class LoggerState
        {
        public:
            LoggerState()
            {
                // diag::Logger::setDefaultInstance();
                diag::setLogger(diag::createLogger());
            }

            ~LoggerState()
            {
                m_subscriptionHandles.clear();
                diag::setLogger(nullptr);
            }

        public:
            template <typename TSubscriber, typename TFilter>
            auto keepSubscription(std::shared_ptr<TSubscriber> subscriber, std::shared_ptr<TFilter> filter = nullptr)
            {
                std::tuple res{subscriber.get(), filter.get()};
                m_subscriptionHandles.emplace_back(diag::getLogger().subscribe(subscriber, filter));
                return res;
            }

            void keepSubscription(diag::Logger::SubscriptionHandle&& handle)
            {
                m_subscriptionHandles.emplace_back(std::move(handle));
            }

        private:
            std::vector<diag::Logger::SubscriptionHandle> m_subscriptionHandles;
        };

    }  // namespace

    struct LoggerBasicTestData
    {
        diag::LogLevel level = diag::LogLevel::Debug;
        size_t messageCount = 0;

        LoggerBasicTestData(diag::LogLevel inLevel, size_t inMessageCount = 100) :
            level(inLevel),
            messageCount(inMessageCount)
        {
        }

        static auto getDefaultValues()
        {
            using namespace nau::diag;

            return ::testing::Values(
                LoggerBasicTestData{LogLevel::Info},
                LoggerBasicTestData{LogLevel::Critical},
                LoggerBasicTestData{LogLevel::Debug},
                LoggerBasicTestData{LogLevel::Error},
                LoggerBasicTestData{LogLevel::Warning},
                LoggerBasicTestData{LogLevel::Verbose});
        }
    };

    /**
     */
    class Test_LoggerBasic : public ::testing::TestWithParam<LoggerBasicTestData>,
                             public LoggerState
    {
    };

    TEST_P(Test_LoggerBasic, Base)
    {
        const auto& data = GetParam();
        auto [subscriber, filter] = keepSubscription(std::make_shared<TestLogSubscriber>(data.messageCount), std::make_shared<TestLogMessageFilter>());
        filter->setLevel(data.level);

        for (int i = 0; i < data.messageCount; ++i)
        {
            NAU_LOG_MESSAGE(data.level)(u8"{}: {}", u8"Data", i);
        }

        ASSERT_TRUE(subscriber->checkMessages());
        ASSERT_EQ(filter->getTotalMessages(), data.messageCount);
    }

    class Test_LoggerFunctor : public ::testing::Test,
                               public LoggerState
    {
    };

    TEST_F(Test_LoggerFunctor, OnlySubscriber)
    {
        using namespace ::nau::diag;
        using namespace ::testing;

        const std::string ExpectedText = "test";

        std::string text;

        keepSubscription(getLogger().subscribe([&](const LoggerMessage& message)
        {
            text = message.data;
        }));

        NAU_LOG_INFO(ExpectedText);

        ASSERT_THAT(text, Eq(ExpectedText));
    }

    TEST_F(Test_LoggerFunctor, SubscriberAndFilter)
    {
        using namespace ::nau::diag;
        using namespace ::testing;

        const std::string ExpectedText = "test";

        std::string text;
        size_t acceptedMessageCount = 0;
        size_t processedMessageCount = 0;

        keepSubscription(getLogger().subscribe([&](const LoggerMessage& message)
        {
            text = message.data;
            ++acceptedMessageCount;
        }, [&](const LoggerMessage& message) -> bool
        {
            ++processedMessageCount;
            return message.level == LogLevel::Info;
        }));

        NAU_LOG_INFO(ExpectedText);
        NAU_LOG_DEBUG(u8"Debug");

        ASSERT_EQ(text, ExpectedText);
        ASSERT_EQ(processedMessageCount, 2);
        ASSERT_EQ(acceptedMessageCount, 1);
    }

    TEST_F(Test_LoggerFunctor, SubscriberAndFilterObject)
    {
        using namespace ::nau::diag;
        using namespace ::testing;

        const std::string ExpectedText = "test";

        std::string text;
        size_t acceptedMessageCount = 0;
        size_t processedMessageCount = 0;

        keepSubscription(getLogger().subscribe([&](const LoggerMessage& message)
        {
            text = message.data;
            ++acceptedMessageCount;
        }, [&](const LoggerMessage& message) -> bool
        {
            ++processedMessageCount;
            return message.level == LogLevel::Info;
        }));

        NAU_LOG_INFO(ExpectedText);
        NAU_LOG_DEBUG(u8"Debug");

        ASSERT_EQ(text, ExpectedText);
        ASSERT_EQ(processedMessageCount, 2);
        ASSERT_EQ(acceptedMessageCount, 1);
    }

    // class Test_LoggerFunctor : public ::testing::TestWithParam<typename T>,
    //                            public LoggerState
    // {

    // };

    // TEST_F(Test_LoggerFunctor, BaseInfo)
    // {

    // }

#if 0
    struct FunctorTester
    {
        uint32_t totalMessages = 0;
        uint32_t f1messages = 0;
        uint32_t f2messages = 0;
        uint32_t f3messages = 0;

        SubscriberHandle m_handle1;
        SubscriberHandle m_handle2;
        SubscriberHandle m_handle3;

        nau::diag::LogLevel criticality = nau::diag::LogLevel::Debug;

        struct FunctorFilter : public nau::diag::IMessageFilter
        {
            FunctorFilter(LogLevel criticality) :
                criticality(criticality)
            {
            }
            LogLevel criticality;
            bool filterMessage(const nau::diag::LoggerMessage& message) final
            {
                return message.criticality == criticality;
            }
        };

        FunctorFilter* filter = nullptr;

        FunctorTester(LogLevel criticality) :
            criticality(criticality),
            filter(new FunctorFilter(criticality))
        {
        }

        void setupFunctors()
        {
            m_handle1 = nau::diag::Logger::aquire()->addSubscriber(
                [this](const nau::diag::LoggerMessage& message)
                {
                    totalMessages++;
                    f1messages++;
                });
            m_handle2 = nau::diag::Logger::aquire()->addSubscriber([this](const LoggerMessage& message)
                                                                   {
                                                                       totalMessages++;
                                                                       f2messages++;
                                                                   },
                                                                   [this](const LoggerMessage& message)
                                                                   {
                                                                       return message.criticality == criticality;
                                                                   });
            m_handle3 = nau::diag::Logger::aquire()->addSubscriber([this](const LoggerMessage& message)
                                                                   {
                                                                       totalMessages++;
                                                                       f3messages++;
                                                                   },
                                                                   filter);
        }
        ~FunctorTester()
        {
            delete filter;
        }
    };

    TEST(Test_Logger, Functor)
    {
        {
            InitialiseLogger(200);
            fileFilter->setLevel(LogLevel::Info);
            FunctorTester testerInfo(LogLevel::Info);
            FunctorTester testerDebug(LogLevel::Debug);
            testerInfo.setupFunctors();
            testerDebug.setupFunctors();
            for(int i = 0; i < 100; ++i)
            {
                NAU_LOG_INFO({}, "{}: {}", "Data", i);
            }
            for(int i = 0; i < 100; ++i)
            {
                NAU_LOG_DEBUG({}, "{}: {}", "Data", i);
            }
            for(int i = 0; i < 100; ++i)
            {
                ASSERT_TRUE(fileSubscriber->checkMessage(i));
            }
            ASSERT_EQ(fileFilter->getTotalMessages(), 200);
            ASSERT_EQ(testerInfo.totalMessages, 400);
            ASSERT_EQ(testerInfo.f1messages, 200);
            ASSERT_EQ(testerInfo.f2messages, 100);
            ASSERT_EQ(testerInfo.f3messages, 100);
            ASSERT_EQ(testerDebug.totalMessages, 400);
            ASSERT_EQ(testerDebug.f1messages, 200);
            ASSERT_EQ(testerDebug.f2messages, 100);
            ASSERT_EQ(testerDebug.f3messages, 100);
        }
        TerminateLogger();
    }

    class TestSubscriberMultiThread : public nau::diag::ILoggerSubscriber
    {
    public:
        static std::atomic_int messageCounter;
        // std::vector<LoggerMessage> messages;

        void processMessage(const LoggerMessage& message) override
        {
            messageCounter++;
            // messages.push_back(message);
        };
    };

    std::atomic_int TestSubscriberMultiThread::messageCounter = 0;

    void ProduceSubscribers(
        std::vector<nau::diag::ILoggerSubscriber*>& subscribers,
        std::vector<SubscriberHandle>& handles,
        int start,
        int end)
    {
        for(int i = start; i < end; i++)
        {
            auto* sub = new TestSubscriberMultiThread();
            subscribers.push_back(sub);
            handles.emplace_back(nau::diag::Logger::aquire()->addSubscriber(sub));
        }
    }

    void KillSubscribers(
        std::vector<SubscriberHandle>& handles,
        int start,
        int end)
    {
        for(int i = start; i < end; i++)
        {
            auto* sub = new TestSubscriberMultiThread();
            nau::diag::Logger::aquire()->removeSubscriber(handles[i]);
        }
    }

    void SendMessages(
        int id,
        int start,
        int end)
    {
        for(int i = start; i < end; i++)
        {
            NAU_LOG_MESSAGE(::nau::diag::LogLevel::Error, {"Test"}, "[id:{}][num:{}]", id, i);
        }
    }

    TEST(Test_Logger, Multithreading)
    {
        InitialiseLogger(0);
        {
            std::vector<std::thread> prodThreads;
            std::vector<std::thread> consThreads;
            std::vector<std::vector<nau::diag::ILoggerSubscriber*>> subscribers;
            std::vector<std::vector<SubscriberHandle>> handles;
            subscribers.resize(60);
            handles.resize(60);
            for(int i = 0; i < 60; ++i)
            {
                prodThreads.emplace_back(&ProduceSubscribers,
                                         std::ref(subscribers[i]),
                                         std::ref(handles[i]),
                                         0, 60);
            }
            for(int i = 0; i < 60; ++i)
            {
                prodThreads[i].join();
            }

            prodThreads.clear();
            NAU_LOG_ERROR({"Test"}, "a{}a", 101);
            ASSERT_TRUE(TestSubscriberMultiThread::messageCounter == 3600);
            handles.clear();

            consThreads.clear();

            for(int i = 0; i < 60; ++i)
            {
                for(int k = 0; k < 60; ++k)
                {
                    delete subscribers[i][k];
                }
            }
        }
        TerminateLogger();
        InitialiseLogger(0);
        {
            std::vector<std::thread> prodThreads;
            std::vector<std::thread> consThreads;
            std::vector<std::vector<nau::diag::ILoggerSubscriber*>> subscribers;
            std::vector<std::vector<SubscriberHandle>> handles;
            subscribers.resize(100);
            handles.resize(10);
            for(int i = 0; i < 10; ++i)
            {
                prodThreads.emplace_back(&ProduceSubscribers,
                                         std::ref(subscribers[i]),
                                         std::ref(handles[i]),
                                         0, 100);
                prodThreads.emplace_back(&SendMessages,
                                         i,
                                         0, 100);
            }
            for(int i = 0; i < 20; ++i)
            {
                prodThreads[i].join();
            }

            ASSERT_TRUE(TestSubscriberMultiThread::messageCounter > 0);
            std::cout << std::format("Total messages send: [{}] PSPSPS...", TestSubscriberMultiThread::messageCounter.load()) << std::endl;
            prodThreads.clear();
            TestSubscriberMultiThread::messageCounter = 0;

            for(int i = 0; i < 10; ++i)
            {
                prodThreads.emplace_back(&KillSubscribers,
                                         std::ref(handles[i]),
                                         0, 100);
                prodThreads.emplace_back(&SendMessages,
                                         i,
                                         0, 100);
            }
            for(int i = 0; i < 20; ++i)
            {
                prodThreads[i].join();
            }

            ASSERT_TRUE(TestSubscriberMultiThread::messageCounter > 0);
            std::cout << std::format("Total messages send: [{}] KSKSKS...", TestSubscriberMultiThread::messageCounter.load()) << std::endl;
            prodThreads.clear();

            TestSubscriberMultiThread::messageCounter = 0;
            NAU_LOG_ERROR({"Test"}, "a{}a", 101);
            ASSERT_TRUE(TestSubscriberMultiThread::messageCounter == 0);
            handles.clear();

            consThreads.clear();

            for(int i = 0; i < 10; ++i)
            {
                for(int k = 0; k < 100; ++k)
                {
                    delete subscribers[i][k];
                }
            }
            subscribers.clear();
            handles.resize(100);
            subscribers.resize(10);

            for(int i = 0; i < 10; ++i)
            {
                prodThreads.emplace_back(&ProduceSubscribers,
                                         std::ref(subscribers[i]),
                                         std::ref(handles[i]),
                                         0, 100);
            }

            for(int i = 0; i < 10; ++i)
            {
                prodThreads.emplace_back(&SendMessages,
                                         i,
                                         0, 100);
            }
            for(int i = 0; i < 20; ++i)
            {
                prodThreads[i].join();
            }

            ASSERT_TRUE(TestSubscriberMultiThread::messageCounter > 0);
            std::cout << std::format("Total messages send: [{}] PPPSSS...", TestSubscriberMultiThread::messageCounter.load()) << std::endl;
            prodThreads.clear();
            TestSubscriberMultiThread::messageCounter = 0;

            for(int i = 0; i < 10; ++i)
            {
                prodThreads.emplace_back(&SendMessages,
                                         i,
                                         0, 100);
            }
            for(int i = 0; i < 10; ++i)
            {
                prodThreads.emplace_back(&KillSubscribers,
                                         std::ref(handles[i]),
                                         0, 100);
            }
            for(int i = 0; i < 20; ++i)
            {
                prodThreads[i].join();
            }

            ASSERT_TRUE(TestSubscriberMultiThread::messageCounter > 0);
            std::cout << std::format("Total messages send: [{}] SSSKKK...", TestSubscriberMultiThread::messageCounter.load()) << std::endl;
            prodThreads.clear();

            TestSubscriberMultiThread::messageCounter = 0;
            NAU_LOG_ERROR({"Test"}, "a{}a", 101);
            ASSERT_TRUE(TestSubscriberMultiThread::messageCounter == 0);
            handles.clear();

            consThreads.clear();

            for(int i = 0; i < 10; ++i)
            {
                for(int k = 0; k < 100; ++k)
                {
                    delete subscribers[i][k];
                }
            }

            subscribers.clear();
            handles.resize(100);
            subscribers.resize(10);

            for(int i = 0; i < 10; ++i)
            {
                prodThreads.emplace_back(&SendMessages,
                                         i,
                                         0, 100);
            }
            for(int i = 0; i < 10; ++i)
            {
                prodThreads.emplace_back(&ProduceSubscribers,
                                         std::ref(subscribers[i]),
                                         std::ref(handles[i]),
                                         0, 100);
            }

            for(int i = 0; i < 20; ++i)
            {
                prodThreads[i].join();
            }

            ASSERT_TRUE(TestSubscriberMultiThread::messageCounter > 0);
            std::cout << std::format("Total messages send: [{}] SSSPPP....", TestSubscriberMultiThread::messageCounter.load()) << std::endl;
            prodThreads.clear();
            TestSubscriberMultiThread::messageCounter = 0;

            for(int i = 0; i < 10; ++i)
            {
                prodThreads.emplace_back(&KillSubscribers,
                                         std::ref(handles[i]),
                                         0, 100);
            }
            for(int i = 0; i < 10; ++i)
            {
                prodThreads.emplace_back(&SendMessages,
                                         i,
                                         0, 100);
            }
            for(int i = 0; i < 20; ++i)
            {
                prodThreads[i].join();
            }

            ASSERT_TRUE(TestSubscriberMultiThread::messageCounter > 0);
            std::cout << std::format("Total messages send: [{}] KKKSSS...", TestSubscriberMultiThread::messageCounter.load()) << std::endl;
            prodThreads.clear();

            TestSubscriberMultiThread::messageCounter = 0;
            NAU_LOG_ERROR({"Test"}, "a{}a", 101);
            ASSERT_TRUE(TestSubscriberMultiThread::messageCounter == 0);
            handles.clear();

            consThreads.clear();

            for(int i = 0; i < 10; ++i)
            {
                for(int k = 0; k < 100; ++k)
                {
                    delete subscribers[i][k];
                }
            }

            std::cout << std::format("Expected messages send: [1000000].", TestSubscriberMultiThread::messageCounter.load()) << std::endl;
        }
        TerminateLogger();
    }
#endif

    INSTANTIATE_TEST_SUITE_P(Default, Test_LoggerBasic, LoggerBasicTestData::getDefaultValues());

}  // namespace nau::test