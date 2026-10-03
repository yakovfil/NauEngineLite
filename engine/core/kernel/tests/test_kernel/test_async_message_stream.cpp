// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// test_message_stream.cpp

#include <string>
#include <vector>

#include "nau/async/task.h"
#include "nau/messaging/async_message_stream.h"
#include "nau/runtime/internal/runtime_state.h"
#include "nau/serialization/runtime_value_builder.h"

namespace nau::test
{

    class Test_AsyncMessageStream : public ::testing::Test
    {
    protected:
        static inline const std::string TestStream1Name = "test.stream_1";

        ~Test_AsyncMessageStream()
        {
            auto shutdown = m_runtime->shutdown();

            while (shutdown())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }

        AsyncMessageSource& broadcaster() const
        {
            return *m_broadcaster;
        }

        const RuntimeState::Ptr m_runtime = RuntimeState::create();
        const AsyncMessageSource::Ptr m_broadcaster = AsyncMessageSource::create();
    };

    TEST_F(Test_AsyncMessageStream, CreateDestroy)
    {
    }

    TEST_F(Test_AsyncMessageStream, SimpleGetStream)
    {
        auto stream1 = broadcaster().getStream(TestStream1Name);
        auto stream2 = broadcaster().getStream(TestStream1Name);
        stream1 = nullptr;
        stream2 = nullptr;
    }

    TEST_F(Test_AsyncMessageStream, PostAfterNext)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        threading::Event signal;

        auto receiver = [&]() -> Task<bool>
        {
            co_await Executor::getDefault();

            auto stream = broadcaster().getStream(TestStream1Name);
            auto message = stream.getNextMessage();
            signal.set();

            auto data = co_await message;

            auto text = *nau::runtimeValueCast<std::string>(data);
            co_return text == "text";
        }();

        auto sender = [&]() -> Task<>
        {
            co_await 1ms;
            signal.wait();
            broadcaster().post(TestStream1Name, makeValueCopy(std::string{"text"}));
        }();

        async::waitResult(std::ref(sender)).ignore();
        const bool success = *async::waitResult(std::ref(receiver));
        ASSERT_TRUE(success);
    }

    TEST_F(Test_AsyncMessageStream, PostBeforeNext)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        threading::Event signal;
        threading::Barrier barrier(2);

        auto receiver = [&]() -> Task<bool>
        {
            co_await Executor::getDefault();

            auto stream = broadcaster().getStream(TestStream1Name);
            signal.set();
            barrier.enter();

            auto message = stream.getNextMessage();
            auto data = co_await message;

            auto text = *nau::runtimeValueCast<std::string>(data);
            co_return text == "text";
        }();

        signal.wait();
        broadcaster().post(TestStream1Name, makeValueCopy(std::string{"text"}));
        barrier.enter();

        const bool success = *async::waitResult(std::ref(receiver));
        ASSERT_TRUE(success);
    }

    /**
        Test unsubscribe
    */
    TEST_F(Test_AsyncMessageStream, Unsubscribe)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        constexpr size_t Count = 1000;

        std::vector<Task<>> subscriptionTasks;
        subscriptionTasks.reserve(Count);

        std::atomic<size_t> startCounter = 0;
        std::atomic<size_t> messageCounter = 0;

        for (size_t i = 0; i < Count; ++i)
        {
            subscriptionTasks.emplace_back([](AsyncMessageSource& broadcaster, std::atomic<size_t>& startC, std::atomic<size_t>& messageC) -> Task<>
            {
                auto stream = broadcaster.getStream(TestStream1Name);
                ++startC;
                co_await stream.getNextMessage();
                ++messageC;
                stream = nullptr;
            }(broadcaster(), startCounter, messageCounter));
        }

        auto sender = [](AsyncMessageSource& broadcaster, std::atomic<size_t>& startC, std::atomic<size_t>& messageC, size_t count) -> Task<bool>
        {
            co_await Executor::getDefault();
            while (startC < count)
            {
                co_await 1ms;
            }

            broadcaster.post(TestStream1Name);
            while (messageC < count)
            {
                co_await 1ms;
            }

            co_return !broadcaster.hasSubscribers(TestStream1Name);
        }(broadcaster(), startCounter, messageCounter, Count);

        async::waitResult(async::whenAll(subscriptionTasks)).ignore();
        const bool success = *async::waitResult(std::ref(sender));
        ASSERT_TRUE(success);
    }

    /**
        After the stream is reseted (client side) all current waits return a stream error,
        all subsequent ones are immediately created in an error state (rejected).
    */
    TEST_F(Test_AsyncMessageStream, ResetedStreamReturnError)
    {
        auto subscription = broadcaster().getStream(TestStream1Name);
        auto task1 = subscription.getNextMessage();
        ASSERT_FALSE(task1.isReady());

        subscription = nullptr;

        ASSERT_TRUE(task1.isReady());
        ASSERT_TRUE(task1.isRejected());

        auto task2 = subscription.getNextMessage();
        ASSERT_TRUE(task2.isReady());
        ASSERT_TRUE(task2.isRejected());
    }

    /**
        After the StreamSource object is destroyed, all current and subsequent calls to AsyncMessageStream::Next()
         should return an error
    */
    TEST_F(Test_AsyncMessageStream, StreamReturnErrorAfterSourceReset_1)
    {
        using namespace nau::async;

        constexpr size_t Count = 1;

        std::vector<AsyncMessageStream> subscribers;
        std::vector<Task<RuntimeValue::Ptr>> subscriptionTasks;

        subscribers.reserve(Count);
        subscriptionTasks.reserve(Count);

        auto testBroadcaster = AsyncMessageSource::create();

        for (size_t i = 0; i < Count; ++i)
        {
            auto& stream = subscribers.emplace_back(testBroadcaster->getStream(TestStream1Name));
            subscriptionTasks.emplace_back(stream.getNextMessage()).detach();
        }

        testBroadcaster.reset();

        const bool allAreRejected = std::all_of(subscriptionTasks.begin(), subscriptionTasks.end(), [](const auto& task)
        {
            return task.isRejected();
        });

        ASSERT_TRUE(allAreRejected);
    }

    /**
        After the StreamSource object is destroyed, all current and subsequent calls to AsyncMessageStream::Next()
        will return an error, accordingly all asynchronous waits (without .Try()) should also be completed automatically
    */
    TEST_F(Test_AsyncMessageStream, StreamReturnErrorAfterSourceReset_2)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        constexpr size_t Count = 1000;

        std::vector<Task<>> subscriptionTasks;
        subscriptionTasks.reserve(Count);

        auto testBroadcaster = AsyncMessageSource::create();

        std::atomic_size_t counter = 0;
        std::atomic_bool mustNeverBeCalled = true;

        for (size_t i = 0; i < Count; ++i)
        {
            subscriptionTasks.emplace_back([](AsyncMessageSource& broadcaster, std::atomic_size_t& c, std::atomic_bool& neverCalled) -> Task<>
            {
                auto stream = broadcaster.getStream(TestStream1Name);
                ++c;
                [[maybe_unused]]
                auto message = co_await stream.getNextMessage();

                neverCalled = false;
            }(*testBroadcaster, counter, mustNeverBeCalled));
        }

        auto sender = [](AsyncMessageSource::Ptr broadcaster, std::atomic_size_t& c, size_t count) mutable -> Task<>
        {
            co_await Executor::getDefault();
            while (c < count)
            {
                co_await 1ms;
            }

            broadcaster.reset();
        }(std::move(testBroadcaster), counter, Count);

        async::waitResult(std::ref(sender)).ignore();
        async::waitResult(async::whenAll(subscriptionTasks)).ignore();

        const bool allAreRejected = std::all_of(subscriptionTasks.begin(), subscriptionTasks.end(), [](const auto& task)
        {
            return task.isRejected();
        });

        ASSERT_TRUE(allAreRejected);
        ASSERT_TRUE(mustNeverBeCalled);
    }

    /**
        Test for canceling subscriptions through an external Cancellation object.
        After calling AsyncMessageSource::SetCancellation() and calling Cancel the set cancellation
    */
    TEST_F(Test_AsyncMessageStream, StreamReturnErrorAfterSourceCancelled)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        constexpr size_t Count = 2000;

        CancellationSource cancellationSource;
        broadcaster().setCancellation(cancellationSource.getCancellation());

        std::vector<Task<>> subscriptionTasks;
        subscriptionTasks.reserve(Count);

        std::atomic_size_t counter = 0;
        std::atomic_bool mustNeverBeCalled = true;

        for (size_t i = 0; i < Count; ++i)
        {
            subscriptionTasks.emplace_back([](AsyncMessageSource& broadcaster, std::atomic_size_t& c, std::atomic_bool& neverCalled) -> Task<>
            {
                auto stream = broadcaster.getStream(TestStream1Name);
                ++c;
                [[maybe_unused]]
                auto message = co_await stream.getNextMessage();

                neverCalled = false;
            }(broadcaster(), counter, mustNeverBeCalled));
        }

        auto sender = [](CancellationSource cancellationSource, std::atomic_size_t& c, size_t count) mutable -> Task<>
        {
            co_await Executor::getDefault();
            while (c < count / 2)
            {
                co_await 1ms;
            }

            cancellationSource.cancel();
        }(std::move(cancellationSource), counter, Count);

        async::waitResult(std::ref(sender)).ignore();
        async::waitResult(async::whenAll(subscriptionTasks)).ignore();

        const bool allAreRejected = std::all_of(subscriptionTasks.begin(), subscriptionTasks.end(), [](const auto& task)
        {
            return task.isRejected();
        });

        ASSERT_TRUE(allAreRejected);
        ASSERT_TRUE(mustNeverBeCalled);
    }

    /**
        A simple check that all new subscriptions always fail after canceling via Cancellation
    */
    TEST_F(Test_AsyncMessageStream, CancelledSourceReturnErrorStream)
    {
        CancellationSource cancellationSource;
        broadcaster().setCancellation(cancellationSource.getCancellation());

        cancellationSource.cancel();

        auto stream = broadcaster().getStream(TestStream1Name);
        auto task = stream.getNextMessage();
        ASSERT_TRUE(task.isReady());
        ASSERT_TRUE(task.isRejected());
    }

    /**
       A simple check that all existing subscriptions will fail after canceling via Cancellation
    */
    TEST_F(Test_AsyncMessageStream, CancelExistingStream)
    {
        auto stream = broadcaster().getStream(TestStream1Name);
        auto task = stream.getNextMessage();
        ASSERT_FALSE(task.isReady());

        CancellationSource cancellationSource;
        broadcaster().setCancellation(cancellationSource.getCancellation());

        cancellationSource.cancel();

        ASSERT_TRUE(task.isReady());
        ASSERT_TRUE(task.isRejected());
    }

    TEST_F(Test_AsyncMessageStream, Stress)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        constexpr size_t SendersCount = 20;
        constexpr size_t SubscribersCount = 100;
        constexpr size_t SendCount = 200;

        const std::vector<std::string> StreamNames = {
            "stream_1",
            "stream_2"};

        std::vector<std::atomic_size_t> receiveCounters(StreamNames.size());
        for (size_t i = 0; i < receiveCounters.size(); ++i)
        {
            receiveCounters[i] = 0;
        }

        std::atomic_size_t subscriberCounter = 0;

        const auto subscriberFactory = [&](std::string streamName) -> Task<>
        {
            co_await Executor::getDefault();

            // auto stream = broadcaster().GetStreamT<size_t>(streamName);
            auto stream = broadcaster().getStream(streamName);
            ++subscriberCounter;

            for (size_t i = 0; i < SendCount * SendersCount; ++i)
            {
                RuntimeValue::Ptr indexVal = co_await stream.getNextMessage();
                const size_t index = *runtimeValueCast<size_t>(indexVal);

                receiveCounters[index].fetch_add(1);
            }
        };

        const auto senderFactory = [&](std::string streamName, size_t streamIndex) -> Task<>
        {
            size_t x = 0;
            for (size_t counter = 0; counter < SendCount; ++counter, ++x)
            {
                broadcaster().post(streamName, makeValueCopy(streamIndex));
                if (x == 50)
                {
                    co_await 1ms;
                }
            }

            // std::cout << "Complete sender:" << streamName << std::endl;
        };

        std::vector<Task<>> senders;
        std::vector<Task<>> subscribers;

        senders.reserve(SendersCount);
        subscribers.reserve(SubscribersCount);

        for (size_t i = 0; i < SubscribersCount; ++i)
        {
            for (const auto& streamName : StreamNames)
            {
                subscribers.emplace_back(subscriberFactory(streamName));
            }
        }

        const size_t ExpectedSubscribersCount = SubscribersCount * StreamNames.size();
        while (subscriberCounter < ExpectedSubscribersCount)
        {
            std::this_thread::sleep_for(1ms);
        }

        for (size_t i = 0; i < SendersCount; ++i)
        {
            size_t streamIndex = 0;
            for (const auto& stream : StreamNames)
            {
                senders.emplace_back(senderFactory(stream, streamIndex++));
            }
        }

        async::waitResult(async::whenAll(senders)).ignore();
        async::waitResult(async::whenAll(subscribers)).ignore();

        const size_t ExpectedReceiveCount = SendersCount * SubscribersCount * SendCount;
        for (const auto& counter : receiveCounters)
        {
            ASSERT_EQ(counter, ExpectedReceiveCount);
        }
    }

    // TEST_F(Test_MessageStream, SubscribeAsTask) {
    //
    //	AsyncMessageSource::Ptr broadcaster = AsyncMessageSource::Create();
    //
    //
    //	Task<> receiver = broadcaster->SubscribeAsTask(TestStream1Name, [](const std::string& message) {
    //
    //		std::cout << message << std::endl;
    //
    //	}, Scheduler::GetDefault());
    //
    //	broadcaster->Post(TestStream1Name, "text");
    //
    //	broadcaster.reset();
    //	Async::Wait(receiver);
    //
    // }

}  // namespace nau::test
