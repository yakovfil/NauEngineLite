// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <gtest/gtest.h>

#include <condition_variable>
#include <mutex>
#include <set>
#include <thread>

#include "app/global_properties_impl.h"
#include "nau/async/async_timer.h"
#include "nau/async/work_queue.h"
#include "nau/io/special_paths.h"
#include "nau/io/virtual_file_system.h"
#include "nau/math/math.h"
#include "nau/memory/general_allocator.h"
#include "nau/module/module.h"
#include "nau/module/module_manager.h"
#include "nau/runtime/internal/runtime_state.h"
#include "nau/serialization/json_utils.h"
#include "nau/threading/critical_section.h"
#include "nau/utils/uid.h"

TEST(CorePortability, GlobalPropertiesAndVirtualFileSystem)
{
    nau::GlobalPropertiesImpl properties;
    ASSERT_TRUE(properties.setValue("core/answer", 42));
    const auto answer = properties.getValue<int>("core/answer");
    ASSERT_TRUE(answer);
    EXPECT_EQ(*answer, 42);
    const auto fs = nau::io::createVirtualFileSystem();
    ASSERT_TRUE(fs);
    EXPECT_FALSE(fs->exists("/unmounted/file"));
#ifdef __EMSCRIPTEN__
    EXPECT_TRUE(nau::io::getKnownFolderPath(nau::io::KnownFolder::UserHome).empty());
    EXPECT_TRUE(nau::io::getNativeTempFilePath().empty());
    EXPECT_FALSE(nau::io::createNativeFileStream("host-only.json", nau::io::AccessMode::Read, nau::io::OpenFileMode::OpenExisting));
    EXPECT_FALSE(nau::io::getKnownFolderPath(nau::io::KnownFolder::Current).empty());
#endif
}

namespace
{
    int moduleInitialized = 0, modulePostInitialized = 0, moduleCleaned = 0;
    class CoreTestModule final : public nau::DefaultModuleImpl
    {
        void initialize() override
        {
            ++moduleInitialized;
        }
        void postInit() override
        {
            ++modulePostInitialized;
        }
        void deinitialize() override
        {
            ++moduleCleaned;
        }
    };
}  // namespace
IMPLEMENT_MODULE(CoreTestModule)

TEST(CorePortability, StaticModuleLifecycle)
{
    moduleInitialized = modulePostInitialized = moduleCleaned = 0;
    {
        auto manager = nau::createModuleManager();
        manager->doModulesPhase(nau::IModuleManager::ModulesPhase::Init);
        EXPECT_TRUE(manager->isModuleLoaded("CoreTest"));
        manager->doModulesPhase(nau::IModuleManager::ModulesPhase::PostInit);
        EXPECT_EQ(moduleInitialized, 1);
        EXPECT_EQ(modulePostInitialized, 1);
    }
    EXPECT_EQ(moduleCleaned, 1);
}

TEST(CorePortability, StringUidJsonRoundTrips)
{
    using nau::serialization::JsonUtils;
    const auto uid = nau::Uid::parseString("01234567-89ab-cdef-0123-456789abcdef");
    ASSERT_TRUE(uid);
    const auto uidJson = JsonUtils::stringify(*uid);
    const auto decodedUid = JsonUtils::parse<nau::Uid>(uidJson);
    ASSERT_TRUE(decodedUid);
    EXPECT_EQ(*decodedUid, *uid);
    const eastl::string value = "A\xd0\x96\xf0\x9f\x9a\x80";
    const auto decoded = JsonUtils::parse<eastl::string>(JsonUtils::stringify(value));
    ASSERT_TRUE(decoded);
    EXPECT_EQ(*decoded, value);
    EXPECT_FALSE(JsonUtils::parse<nau::Uid>(std::string_view{"\"invalid\""}));
    EXPECT_FALSE(JsonUtils::parse<int>(std::string_view{"{"}));
}

#include "nau/string/string.h"
#include "nau/threading/event.h"

#ifdef __EMSCRIPTEN__
    #include "nau/diag/device_error.h"
    #include "nau/rtti/rtti_impl.h"

TEST(CorePortability, DISABLED_FatalDiagnostic)
{
    nau::diag::setDeviceError(nau::diag::createDefaultDeviceError());
    NAU_FAILURE_IMPL(1, nau::diag::AssertionKind::Fatal, "core fatal fixture");
}

namespace
{
    class UtfDiagnostic final : public nau::diag::IDeviceError
    {
        NAU_RTTI_CLASS(UtfDiagnostic, nau::diag::IDeviceError)
    public:
        unsigned count = 0;
        nau::diag::FailureActionFlag handleFailure(const nau::diag::FailureData& data) override
        {
            EXPECT_EQ(data.kind, nau::diag::AssertionKind::Default);
            EXPECT_NE(data.message.find("Invalid UTF"), eastl::string_view::npos);
            ++count;
            return {};
        }
    };
}  // namespace

TEST(CorePortability, InvalidUtfIsEmptyAndDiagnostic)
{
    auto device = eastl::make_unique<UtfDiagnostic>();
    auto* observed = device.get();
    nau::diag::IDeviceError::Ptr previous;
    nau::diag::setDeviceError(std::move(device), &previous);
    // Include a valid prefix: invalid input must not produce a partial result.
    for (const auto& input : {
             std::u16string{  u'A', 0xd800},
             std::u16string{  u'A', 0xdc00},
             std::u16string{0xd800,   u'B'}
    })
    {
        EXPECT_TRUE(nau::string(input).empty());
    }
    EXPECT_TRUE(nau::string(std::wstring{L'A', 0x110000}).empty());
    EXPECT_TRUE(nau::strings::wstringToUtf8(eastl::wstring{L'A', 0xd800}).empty());
    EXPECT_TRUE(nau::strings::utf8ToWString(eastl::u8string_view{u8"A\xc0\xaf"}).empty());
    EXPECT_EQ(observed->count, 6u);
    nau::diag::setDeviceError(std::move(previous));
}
#endif

TEST(CorePortability, ValidUtfRoundTrip)
{
    const eastl::u16string input = u"A\u0416\U0001f680";
    const nau::string value(input);
    EXPECT_EQ(value.tou16string(), input);
    const auto wide = nau::strings::utf8ToWString(u8"A\u0416\U0001f680");
    EXPECT_EQ(nau::strings::wstringToUtf8(wide), u8"A\u0416\U0001f680");
    EXPECT_TRUE(nau::string(eastl::u16string{}).empty());
}

TEST(CorePortability, EventReset)
{
    using namespace std::chrono_literals;
    nau::threading::Event event(nau::threading::Event::ResetMode::Manual);
    EXPECT_FALSE(event.wait(1ms));
    event.set();
    EXPECT_TRUE(event.wait(1ms));
    EXPECT_TRUE(event.wait(1ms));
    event.reset();
    EXPECT_FALSE(event.wait(1ms));
}

TEST(CorePortability, WorkerSynchronizationAndLocalCleanup)
{
    using namespace std::chrono_literals;
    using nau::threading::Event;
    Event signal(Event::ResetMode::Auto);
    std::atomic<unsigned> released = 0;
    auto wait = [&]
    {
        if (signal.wait(100ms))
            ++released;
    };
    std::thread first(wait), second(wait);
    signal.set();
    first.join();
    second.join();
    EXPECT_EQ(released.load(), 1u);
    EXPECT_FALSE(signal.wait(1ms));
    Event manual(Event::ResetMode::Manual);
    released = 0;
    std::thread manualFirst([&]
    {
        if (manual.wait(1s))
            ++released;
    });
    std::thread manualSecond([&]
    {
        if (manual.wait(1s))
            ++released;
    });
    manual.set();
    manualFirst.join();
    manualSecond.join();
    EXPECT_EQ(released.load(), 2u);
    EXPECT_TRUE(manual.wait(1ms));
    manual.reset();
    EXPECT_FALSE(manual.wait(1ms));
    signal.set();
    signal.reset();
    EXPECT_FALSE(signal.wait(1ms));
    dag::CriticalSection mutex;
    unsigned total = 0;
    std::atomic<unsigned> destroyed = 0;
    nau::ThreadLocalValue<std::unique_ptr<int>> locals;
    auto update = [&]
    {
        thread_local std::unique_ptr<nau::RAIIFunction> cleanup;
        cleanup = std::make_unique<nau::RAIIFunction>(nullptr, [&]
        {
            ++destroyed;
        });
        EXPECT_FALSE(locals.value());
        locals.value() = std::make_unique<int>(7);
        for (unsigned i = 0; i < 1000; ++i)
        {
            mutex.lock();
            ++total;
            mutex.unlock();
        }
        locals.destroy();
    };
    std::thread a(update), b(update);
    a.join();
    b.join();
    EXPECT_EQ(total, 2000u);
    EXPECT_EQ(destroyed.load(), 2u);
#ifdef __EMSCRIPTEN__
    EXPECT_TRUE(mutex.timedLock(1));
    std::thread contender([&]
    {
        EXPECT_FALSE(mutex.timedLock(2));
    });
    contender.join();
    mutex.unlock();
#endif
}

TEST(CorePortability, VectorMatrixValues)
{
    const nau::math::vec3 value(1.0f, 2.0f, 3.0f);
    EXPECT_NEAR(nau::math::lengthSqr(value), 14.0f, 1e-5f);
    const auto transformed = nau::math::mat4::identity() * nau::math::vec4(value, 1.0f);
    EXPECT_NEAR(transformed.getX(), 1.0f, 1e-5f);
    EXPECT_NEAR(transformed.getY(), 2.0f, 1e-5f);
    EXPECT_NEAR(transformed.getZ(), 3.0f, 1e-5f);
    EXPECT_NEAR(transformed.getW(), 1.0f, 1e-5f);
    const auto soa = Vectormath::Soa::SoaFloat3::Load(nau::math::vec4(1.0f), nau::math::vec4(2.0f), nau::math::vec4(3.0f));
    const auto doubled = soa + soa;
    EXPECT_NEAR(doubled.x.getX(), 2.0f, 1e-5f);
    EXPECT_NEAR(doubled.y.getY(), 4.0f, 1e-5f);
    EXPECT_NEAR(doubled.z.getW(), 6.0f, 1e-5f);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(&transformed) % alignof(nau::math::vec4), 0u);
}

TEST(CorePortability, UidFixturesAndWorkers)
{
    static_assert(sizeof(nau::Uid) == 16);
    const auto value = nau::Uid::parseString("01234567-89ab-cdef-0123-456789abcdef");
    const auto upper = nau::Uid::parseString("01234567-89AB-CDEF-0123-456789ABCDEF");
    ASSERT_TRUE(value);
    ASSERT_TRUE(upper);
    EXPECT_EQ(*value, *upper);
    EXPECT_EQ(toString(*value), "01234567-89ab-cdef-0123-456789abcdef");
    EXPECT_EQ(std::hash<nau::Uid>{}(*value), std::hash<nau::Uid>{}(*upper));
    for (const auto text : {"", "01234567-89ab-cdef-0123-456789abcdeg", "01234567_89ab-cdef-0123-456789abcdef", "01234567-89ab-cdef-0123-456789abcde"})
    {
        EXPECT_FALSE(nau::Uid::parseString(text));
    }
    std::set<std::string> generated;
    std::mutex mutex;
    auto generate = [&]
    {
        for (unsigned i = 0; i < 32; ++i)
        {
            const auto id = nau::Uid::generate();
            EXPECT_TRUE(id);
            std::lock_guard lock(mutex);
            EXPECT_TRUE(generated.insert(toString(id)).second);
        }
    };
    std::thread first(generate), second(generate);
    first.join();
    second.join();
    EXPECT_EQ(generated.size(), 64u);
}

TEST(CorePortability, AllocationAlignmentResizeAndCrossThreadFree)
{
    nau::GeneralAllocator allocator;
    for (size_t alignment : {size_t(8), size_t(16), size_t(32), size_t(64)})
    {
        auto* ptr = static_cast<unsigned char*>(allocator.allocateAligned(37, alignment));
        ASSERT_NE(ptr, nullptr);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % alignment, 0u);
        for (unsigned i = 0; i < 37; ++i)
            ptr[i] = static_cast<unsigned char>(i);
        ptr = static_cast<unsigned char*>(allocator.reallocateAligned(ptr, 4097, alignment));
        ASSERT_NE(ptr, nullptr);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % alignment, 0u);
        for (unsigned i = 0; i < 37; ++i)
            EXPECT_EQ(ptr[i], i);
        ptr = static_cast<unsigned char*>(allocator.reallocateAligned(ptr, 17, alignment));
        for (unsigned i = 0; i < 17; ++i)
            EXPECT_EQ(ptr[i], i);
        std::thread worker([&]
        {
            EXPECT_TRUE(allocator.isValid(ptr));
            allocator.deallocateAligned(ptr);
            EXPECT_FALSE(allocator.isAligned(ptr));
        });
        worker.join();
        EXPECT_FALSE(allocator.isAligned(ptr));
    }
    auto* plain = static_cast<unsigned char*>(allocator.allocate(2048));
    EXPECT_EQ(reinterpret_cast<uintptr_t>(plain) % alignof(std::max_align_t), 0u);
    std::memset(plain, 0x5a, 2048);
    plain = static_cast<unsigned char*>(allocator.reallocate(plain, 8192));
    EXPECT_EQ(allocator.getSize(plain), 8192u);
    for (size_t i = 0; i < 2048; ++i)
        EXPECT_EQ(plain[i], 0x5a);
    allocator.deallocate(plain);
    void* exported = nullptr;
    std::thread producer([&]
    {
        exported = allocator.allocateAligned(73, 64);
        std::memset(exported, 0x7b, 73);
    });
    producer.join();
    EXPECT_TRUE(allocator.isValid(exported));
    EXPECT_EQ(static_cast<unsigned char*>(exported)[72], 0x7b);
    allocator.deallocateAligned(exported);
    EXPECT_FALSE(allocator.isAligned(exported));
}

TEST(CorePortability, TimerExpiryCancelReentryAndShutdown)
{
    using namespace std::chrono_literals;
    auto runtime = nau::RuntimeState::create();
    auto& timers = nau::async::ITimerManager::getInstance();
    {
        auto queue = nau::WorkQueue::create();
        auto captured = std::make_shared<int>(42);
        std::weak_ptr<int> lifetime = captured;
        auto task = [](nau::async::Executor::Ptr executor, std::shared_ptr<int> resource) -> nau::async::Task<int>
        {
            ASYNC_SWITCH_EXECUTOR(executor)
            co_await 1ms;
            co_return *resource;
        }(queue, std::move(captured));
        EXPECT_FALSE(task.isReady());
        std::thread worker([&]
        {
            const auto end = std::chrono::steady_clock::now() + 5s;
            while (!task.isReady() && std::chrono::steady_clock::now() < end)
            {
                queue->poll();
                std::this_thread::sleep_for(1ms);
            }
        });
        worker.join();
        EXPECT_TRUE(task.isReady());
        if (task.isReady())
            EXPECT_EQ(*task.asResult(), 42);
        EXPECT_TRUE(lifetime.expired());
    }
    std::atomic<unsigned> cancelledCalls = 0;
    const auto cancelled = timers.invokeAfter(60s, [](void* data) noexcept
    {
        ++*static_cast<std::atomic<unsigned>*>(data);
    }, &cancelledCalls);
    timers.cancelInvokeAfter(cancelled);
    nau::threading::Event expired(nau::threading::Event::ResetMode::Manual);
    timers.invokeAfter(2ms, [](void* data) noexcept
    {
        nau::async::ITimerManager::getInstance().invokeAfter(1ms, [](void* nested) noexcept
        {
            static_cast<nau::threading::Event*>(nested)->set();
        }, data);
    }, &expired);
    EXPECT_TRUE(expired.wait(5s));
    std::atomic<unsigned> shutdownCalls = 0;
    std::array<std::atomic<unsigned>, 64> raceCalls{};
    std::array<nau::async::ITimerManager::InvokeAfterHandle, 64> handles;
    for (size_t i = 0; i < handles.size(); ++i)
    {
        handles[i] = timers.invokeAfter(1ms, [](void* data) noexcept
        {
            ++*static_cast<std::atomic<unsigned>*>(data);
        }, &raceCalls[i]);
    }
    std::thread cancelRaces([&]
    {
        for (auto handle : handles)
            timers.cancelInvokeAfter(handle);
    });
    cancelRaces.join();
    timers.executeAfter(60s, nullptr, [](nau::Error::Ptr error, void* data) noexcept
    {
        EXPECT_TRUE(error);
        ++*static_cast<std::atomic<unsigned>*>(data);
    }, &shutdownCalls);
    struct PendingWork
    {
        nau::threading::Event release{nau::threading::Event::ResetMode::Manual};
        std::atomic<unsigned> completed = 0;
    } pendingWork;
    for (unsigned i = 0; i < 32; ++i)
    {
        nau::async::Executor::getDefault()->execute([](void* data, void*) noexcept
        {
            auto& work = *static_cast<PendingWork*>(data);
            EXPECT_TRUE(work.release.wait(5s));
            ++work.completed;
        }, &pendingWork);
    }
    auto shutdown = runtime->shutdown();
    EXPECT_TRUE(shutdown());
    pendingWork.release.set();
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    bool pending;
    while ((pending = shutdown()) && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(1ms);
    EXPECT_FALSE(pending);
    EXPECT_EQ(cancelledCalls.load(), 0u);
    EXPECT_EQ(shutdownCalls.load(), 1u);
    EXPECT_EQ(pendingWork.completed.load(), 32u);
    for (const auto& count : raceCalls)
        EXPECT_LE(count.load(), 1u);
}
