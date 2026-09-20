// Standalone SDK capability probe; does not validate the Nau runtime.
#include <xxHash/xxhash.h>

#ifdef NAU_PROBE_FMT
#include <fmt/format.h>
#endif

#include <atomic>
#include <coroutine>
#include <cstdlib>
#include <thread>

#ifndef __EMSCRIPTEN_PTHREADS__
    #error The probe must compile with Emscripten pthread support.
#endif

#if defined(_WIN32) || defined(_WIN64) || defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
    #error Windows or x86 target definitions must not reach the browser probe.
#endif

struct ProbeTask
{
    struct promise_type
    {
        ProbeTask get_return_object()
        {
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept
        {
            return {};
        }
        std::suspend_always final_suspend() noexcept
        {
            return {};
        }
        void return_void() noexcept
        {
        }
        void unhandled_exception()
        {
            std::abort();
        }
    };

    std::coroutine_handle<promise_type> handle;

    ~ProbeTask()
    {
        handle.destroy();
    }
};

ProbeTask increment(std::atomic<int>& value)
{
    value.fetch_add(1);
    co_return;
}

int main()
{
    std::atomic<int> value{0};
    auto task = increment(value);
    std::thread worker([&task]
    {
        task.handle.resume();
    });
    worker.join();
#ifdef NAU_PROBE_FMT
    const auto message = fmt::format("steps={}", value.load());
    const auto hash = XXH32(message.data(), message.size(), 0);
#else
    const auto hash = XXH32("probe", 5, 0);
#endif
    return value.load() == 1 && task.handle.done() && hash != 0 ? 0 : 1;
}
