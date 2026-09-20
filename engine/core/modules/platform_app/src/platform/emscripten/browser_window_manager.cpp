// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "browser_window_manager.h"

#include <emscripten.h>
#include <emscripten/threading.h>

#include "nau/runtime/internal/runtime_object_registry.h"

namespace nau
{
    struct BrowserWindowContext
    {
        BrowserWindowManager* manager = nullptr;
        std::atomic<BrowserWindowState> state{BrowserWindowState::Initializing};
        std::atomic<bool> documentVisible{true};
        std::mutex mutex;
        std::condition_variable idle;
        unsigned inFlight = 0;
    };

    extern "C" EMSCRIPTEN_KEEPALIVE void nau_browser_window_event(BrowserWindowContext* context, int connected, int visible)
    {
        context->documentVisible = visible != 0;
        context->manager->getExecutor()->execute([](void*, void*) noexcept
        {
        }, nullptr);
        if (!connected)
        {
            auto ready = BrowserWindowState::Ready;
            if (context->state.compare_exchange_strong(ready, BrowserWindowState::Failed))
            {
                NAU_LOG_ERROR("Browser window host disconnected");
            }
        }
    }

    namespace
    {
        // This whole operation runs on the UI thread. No C++ mutex crosses a proxy.
        int domOperation(BrowserWindowContext* context, int operation, unsigned* values = nullptr, const char* name = nullptr)
        {
            // clang-format off
            return MAIN_THREAD_EM_ASM_INT({
                const key = $0;
                const op = $1;
                const values = $2;
                const name = $3;
                const hosts = Module['nauPlatformHosts'] || (Module['nauPlatformHosts'] = new Map());
                if (op === 0)
                {
                    let elements;
                    try
                    {
                        elements = document.querySelectorAll(Module['nauWindowSelector'] || '#nau-window');
                    }
                    catch (error)
                    {
                        console.error('Invalid browser host selector', error);
                        return 0;
                    }
                    if (elements.length !== 1 || !(elements[0] instanceof HTMLElement) || !elements[0].isConnected)
                        return 0;
                    const element = elements[0];
                    const notify = () => _nau_browser_window_event(key, element.isConnected, !document.hidden);
                    const observer = new MutationObserver(notify);
                    hosts.set(key, {element, notify, observer});
                    try
                    {
                        document.addEventListener('visibilitychange', notify);
                        observer.observe(document, {childList: true, subtree: true});
                        notify();
                    }
                    catch (error)
                    {
                        document.removeEventListener('visibilitychange', notify);
                        observer.disconnect();
                        hosts.delete(key);
                        console.error('Browser callback installation failed', error);
                        return 0;
                    }
                    return 1;
                }
                const host = hosts.get(key);
                if (op === 9)
                {
                    if (host)
                    {
                        document.removeEventListener('visibilitychange', host.notify);
                        host.observer.disconnect();
                        hosts.delete(key);
                    }
                    return 1;
                }
                if (!host || !host.element.isConnected)
                {
                    if (host)
                        host.notify();
                    return 0;
                }
                const element = host.element;
                if (op === 1 || op === 2)
                {
                    HEAPU32[values >> 2] = op === 1 ? element.offsetWidth : element.clientWidth;
                    HEAPU32[(values >> 2) + 1] = op === 1 ? element.offsetHeight : element.clientHeight;
                }
                else if (op === 3)
                {
                    const style = getComputedStyle(element);
                    const px = value => parseFloat(value) || 0;
                    const x = style.boxSizing === 'border-box' ? 0 : px(style.paddingLeft) + px(style.paddingRight) + px(style.borderLeftWidth) + px(style.borderRightWidth);
                    const y = style.boxSizing === 'border-box' ? 0 : px(style.paddingTop) + px(style.paddingBottom) + px(style.borderTopWidth) + px(style.borderBottomWidth);
                    element.style.width = Math.max(0, HEAPU32[values >> 2] - x) + 'px';
                    element.style.height = Math.max(0, HEAPU32[(values >> 2) + 1] - y) + 'px';
                }
                else if (op === 4)
                {
                    element.style.visibility = HEAPU32[values >> 2] ? 'visible' : 'hidden';
                }
                else if (op === 5)
                {
                    if (getComputedStyle(element).visibility !== 'visible')
                        return 0;
                    for (let parent = element; parent; parent = parent.parentElement)
                    {
                        if (getComputedStyle(parent).display === 'none')
                            return 0;
                    }
                    return 1;
                }
                else if (op === 6)
                {
                    element.setAttribute('aria-label', UTF8ToString(name));
                }
                return 1;
            },
                                          context, operation, values, name);
            // clang-format on
        }

        class BrowserWindow final : public IPlatformWindow
        {
            NAU_CLASS_(nau::BrowserWindow, IPlatformWindow)
        public:
            explicit BrowserWindow(std::shared_ptr<BrowserWindowContext> context) :
                m_context(std::move(context))
            {
            }

            int access(int operation, unsigned* values = nullptr, const char* name = nullptr) const
            {
                {
                    std::lock_guard lock(m_context->mutex);
                    if (m_context->state != BrowserWindowState::Ready)
                    {
                        NAU_LOG_WARNING("Browser window operation rejected: service is not ready");
                        return 0;
                    }
                    ++m_context->inFlight;
                }
                const int result = domOperation(m_context.get(), operation, values, name);
                {
                    std::lock_guard lock(m_context->mutex);
                    --m_context->inFlight;
                    m_context->idle.notify_all();
                }
                return result;
            }

            void setVisible(bool visible) override
            {
                unsigned value = visible;
                access(4, &value);
            }
            bool isVisible() const override
            {
                return access(5) != 0;
            }
            eastl::pair<unsigned, unsigned> getSize() const override
            {
                unsigned values[2]{};
                access(1, values);
                return {values[0], values[1]};
            }
            eastl::pair<unsigned, unsigned> getClientSize() const override
            {
                unsigned values[2]{};
                access(2, values);
                return {values[0], values[1]};
            }
            void setSize(unsigned x, unsigned y) override
            {
                unsigned values[]{x, y};
                access(3, values);
            }
            void setName(const char* name) override
            {
                access(6, nullptr, name ? name : "");
            }
            void setPosition(unsigned, unsigned) override
            {
                NAU_LOG_WARNING("Browser desktop positioning is unsupported");
            }
            eastl::pair<unsigned, unsigned> getPosition() const override
            {
                return {0, 0};
            }

        private:
            std::shared_ptr<BrowserWindowContext> m_context;
        };
    }  // namespace

    BrowserWindowManager::BrowserWindowManager() :
        m_context(std::make_shared<BrowserWindowContext>())
    {
        m_context->manager = this;
        RuntimeObjectRegistration{nau::Ptr<>{this}}.setAutoRemove();
    }

    BrowserWindowManager::~BrowserWindowManager()
    {
        NAU_ASSERT(m_closed || m_thread == std::thread::id{});
    }

    void BrowserWindowManager::bindToCurrentThread()
    {
        m_thread = std::this_thread::get_id();
        if (emscripten_is_main_browser_thread() || !domOperation(m_context.get(), 0))
        {
            m_context->state = BrowserWindowState::Failed;
            NAU_LOG_ERROR("Browser platform initialization failed: expected one connected HTMLElement host on a service worker");
            m_ready.reject(NauMakeError("Browser host selection or callback initialization failed"));
            return;
        }
        m_window = rtti::createInstance<BrowserWindow>(m_context);
        m_context->state = BrowserWindowState::Ready;
        m_ready.resolve();
    }

    IPlatformWindow& BrowserWindowManager::getActiveWindow()
    {
        NAU_FATAL(m_window);
        return *m_window;
    }
    nau::Ptr<IPlatformWindow> BrowserWindowManager::createWindow(bool)
    {
        NAU_LOG_WARNING("Additional browser windows are unsupported");
        return nullptr;
    }
    async::Task<> BrowserWindowManager::windowReady()
    {
        return m_ready.getTask();
    }
    BrowserWindowState BrowserWindowManager::getBrowserState() const
    {
        return m_context->state;
    }
    bool BrowserWindowManager::isDocumentVisible() const
    {
        return m_context->documentVisible;
    }
    async::Executor::Ptr BrowserWindowManager::getExecutor()
    {
        return static_cast<async::Executor*>(this);
    }

    Result<> BrowserWindowManager::pumpMessageQueue(bool wait, std::optional<std::chrono::milliseconds> limit)
    {
        if (std::this_thread::get_id() != m_thread)
            return NauMakeError("Browser pump called from the wrong thread");
        const auto start = std::chrono::steady_clock::now();
        const Executor::InvokeGuard guard{*this};
        for (;;)
        {
            Invocation invocation;
            {
                std::unique_lock lock(m_mutex);
                if (wait && m_queue.empty() && !m_closing)
                {
                    if (limit)
                        m_signal.wait_until(lock, start + *limit, [this]
                        {
                            return !m_queue.empty() || m_closing;
                        });
                    else
                        m_signal.wait(lock, [this]
                        {
                            return !m_queue.empty() || m_closing;
                        });
                }
                if (m_queue.empty() && m_closing && !m_cleanupComplete)
                {
                    if (!wait)
                        return {};
                    if (limit)
                    {
                        if (!m_signal.wait_until(lock, start + *limit, [this]
                        {
                            return m_cleanupComplete;
                        }))
                            return {};
                    }
                    else
                        m_signal.wait(lock, [this]
                        {
                            return m_cleanupComplete;
                        });
                }
                wait = false;
                if (m_queue.empty())
                {
                    if (m_closing)
                    {
                        m_closed = true;
                        m_context->state = BrowserWindowState::Closed;
                        m_signal.notify_all();
                        return NauMakeError("Browser platform disposed");
                    }
                    return {};
                }
                invocation = std::move(m_queue.front());
                m_queue.pop_front();
                ++m_invoking;
            }
            Executor::invoke(*this, std::move(invocation));
            {
                std::lock_guard lock(m_mutex);
                --m_invoking;
                m_signal.notify_all();
            }
            if (limit && std::chrono::steady_clock::now() - start >= *limit)
                return {};
        }
    }

    void BrowserWindowManager::scheduleInvocation(Invocation invocation) noexcept
    {
        {
            std::lock_guard lock(m_mutex);
            if (!m_closing)
            {
                m_queue.emplace_back(std::move(invocation));
                m_signal.notify_all();
                return;
            }
        }
        // Invocation has no cancellation contract. Do not discard continuations.
        // Closing executors complete late submissions on their submitting thread.
        NAU_LOG_WARNING("Browser executor is closing: completing late invocation on submitting thread");
        const Executor::InvokeGuard guard{*this};
        Executor::invoke(*this, std::move(invocation));
    }

    void BrowserWindowManager::waitAnyActivity() noexcept
    {
        std::unique_lock lock(m_mutex);
        m_signal.wait(lock, [this]
        {
            return !m_queue.empty() || !m_invoking || m_closed;
        });
    }

    bool BrowserWindowManager::hasWorks()
    {
        std::lock_guard lock(m_mutex);
        return !m_queue.empty() || m_invoking;
    }

    void BrowserWindowManager::dispose()
    {
        {
            std::lock_guard lock(m_context->mutex);
            const auto state = m_context->state.load();
            if (state == BrowserWindowState::Closing || state == BrowserWindowState::Closed)
                return;
            m_context->state = BrowserWindowState::Closing;
            std::lock_guard queueLock(m_mutex);
            m_closing = true;
            m_signal.notify_all();
        }
        // Service disposal is initiated from a worker, never by blocking the UI.
        NAU_FATAL(!emscripten_is_main_browser_thread());
        {
            std::unique_lock lock(m_context->mutex);
            m_context->idle.wait(lock, [this]
            {
                return m_context->inFlight == 0;
            });
        }
        domOperation(m_context.get(), 9);
        {
            std::lock_guard lock(m_mutex);
            m_cleanupComplete = true;
            m_signal.notify_all();
        }
    }
}  // namespace nau
