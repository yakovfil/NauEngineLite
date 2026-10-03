// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "ui_manager.h"

#include <cocos/base/CCDirector.h>
#include <cocos/base/CCEventDispatcher.h>
#include <cocos/base/CCEventKeyboard.h>
#include <cocos/base/CCEventMouse.h>
#include <cocos/cocos2d.h>
#include <cocos/math/CCGeometry.h>
#include <cocos/platform/CCRenderView.h>
#include <timeapi.h>

#include <memory>
#include <queue>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "2d/CCScene.h"
#include "nau/diag/assertion.h"
#include "nau/diag/error.h"
#include "nau/diag/logging.h"
#include "nau/graphics/core_graphics.h"
#include "nau/input.h"
#include "nau/math/math.h"
#include "nau/memory/general_allocator.h"
#include "nau/memory/mem_allocator.h"
#include "nau/scene/scene_factory.h"
#include "nau/scene/scene_manager.h"
#include "nau/service/service_provider.h"
#include "nau/ui/elements/canvas.h"
#include "nau_backend/device_nau.h"
#include "ui_render_view.h"

namespace
{

    static cocos2d::Touch* g_touches[cocos2d::EventTouch::MAX_TOUCHES] = {nullptr};
    static std::unordered_map<intptr_t, int> g_touchIdReorderMap;
    static unsigned int g_indexBitsUsed = 0;
    int getUnUsedIndex()
    {
        int i;
        int temp = g_indexBitsUsed;

        for (i = 0; i < cocos2d::EventTouch::MAX_TOUCHES; i++)
        {
            if (!(temp & 0x00000001))
            {
                g_indexBitsUsed |= (1 << i);
                return i;
            }

            temp >>= 1;
        }

        // all bits are used
        return -1;
    }

    void removeUsedIndexBit(int index)
    {
        if (index < 0 || index >= cocos2d::EventTouch::MAX_TOUCHES)
        {
            return;
        }

        unsigned int temp = 1 << index;
        temp = ~temp;
        g_indexBitsUsed &= temp;
    }

    std::vector<cocos2d::Touch*> getAllTouchesVector()
    {
        std::vector<cocos2d::Touch*> ret;
        int i;
        int temp = g_indexBitsUsed;

        for (i = 0; i < cocos2d::EventTouch::MAX_TOUCHES; i++)
        {
            if (temp & 0x00000001)
            {
                ret.push_back(g_touches[i]);
            }
            temp >>= 1;
        }
        return ret;
    }

    static const cocos2d::Size designResolutionSize = cocos2d::Size(650, 650);

    void animationIntervalSetterDelegate(float interval)
    {
        nau::getServiceProvider().get<nau::ui::UiManager>().setAnimationInterval(interval);
    }

    void applicationWillEnterForegroundCb()
    {
        nau::getServiceProvider().get<nau::ui::UiManager>().applicationWillEnterForeground();
    }

    void applicationDidEnterBackgroundCb()
    {
        nau::getServiceProvider().get<nau::ui::UiManager>().applicationDidEnterBackground();
    }
}  // namespace

namespace nau::ui
{

    async::Task<> UiManagerImpl::preInitService()
    {
        nau::getServiceProvider().addService<nau::cocos_backend::NauDeviceProvider>();


        return async::Task<>::makeResolved();
    }

    async::Task<> UiManagerImpl::initService()
    {
        m_mouseButtonRemap[nau::input::MouseKey::ButtonLeft] = cocos2d::EventMouse::MouseButton::BUTTON_LEFT;
        m_mouseButtonRemap[nau::input::MouseKey::ButtonRight] = cocos2d::EventMouse::MouseButton::BUTTON_RIGHT;
        m_mouseButtonRemap[nau::input::MouseKey::ButtonMiddle] = cocos2d::EventMouse::MouseButton::BUTTON_MIDDLE;
        m_mouseButtonRemap[nau::input::MouseKey::Button4] = cocos2d::EventMouse::MouseButton::BUTTON_4;
        m_mouseButtonRemap[nau::input::MouseKey::Button5] = cocos2d::EventMouse::MouseButton::BUTTON_5;
        m_mouseButtonRemap[nau::input::MouseKey::Button6] = cocos2d::EventMouse::MouseButton::BUTTON_6;
        m_mouseButtonRemap[nau::input::MouseKey::Button7] = cocos2d::EventMouse::MouseButton::BUTTON_7;

        ///////////////////////////////////////////////////////////////////////////
        /////////////// changing timer resolution
        ///////////////////////////////////////////////////////////////////////////
        UINT TARGET_RESOLUTION = 1;  // 1 millisecond target resolution
        TIMECAPS tc;
        if (TIMERR_NOERROR == timeGetDevCaps(&tc, sizeof(TIMECAPS)))
        {
            wTimerRes = std::min(std::max(tc.wPeriodMin, TARGET_RESOLUTION), tc.wPeriodMax);
            timeBeginPeriod(wTimerRes);
        }

        QueryPerformanceCounter(&m_nLast);

        initGLContextAttrs();

        // Initialize instance and cocos2d.
        if (!applicationDidFinishLaunching())
        {
            co_yield NauMakeError("nau::ui initialization failed");
        }

        // Retain glview to avoid glview being released in the while loop
        cocos2d::Director* director = cocos2d::Director::getInstance();
        cocos2d::RenderView* glview = director->getRenderView();
        glview->retain();
        cocos2d::backend::ProgramCache::getInstance();

        QueryPerformanceFrequency(&m_freq);

        scene::IScene::Ptr engineScene = nau::getServiceProvider().get<nau::scene::ISceneFactory>().createEmptyScene();
        engineScene->setName("UI service scene");
        setEngineScene(engineScene.getRef());
        co_await nau::getServiceProvider().get<nau::scene::ISceneManager>().activateScene(std::move(engineScene));

        m_isAlive = true;

        co_return;
    }

    void UiManagerImpl::update()
    {
        lock_(m_cocosRenderMutex);
        if (!m_isAlive)
        {
            return;
        }
        handleInput();

        cocos2d::Director* director = cocos2d::Director::getInstance();

        QueryPerformanceCounter(&m_nNow);
        m_interval = m_nNow.QuadPart - m_nLast.QuadPart;
        if (m_interval >= m_animationInterval.QuadPart)
        {
            m_nLast.QuadPart = m_nNow.QuadPart;
            director->mainLoop();
        }

        notifyElementsChanged();
    }

    void UiManagerImpl::update(float dt)
    {
        lock_(m_cocosRenderMutex);
        if (!m_isAlive)
        {
            return;
        }

        handleInput();

        cocos2d::Director* director = cocos2d::Director::getInstance();
        director->mainLoop(dt);

        notifyElementsChanged();
    }

    void UiManagerImpl::handleInput()
    {
        const auto [cursorX, cursorY] = getInputPosition(
            nau::input::getMouseAxisValue(0, nau::input::MouseKey::AxisX),
            nau::input::getMouseAxisValue(0, nau::input::MouseKey::AxisY));

        touchMove(&cursorX, &cursorY);

        if (nau::input::isMouseButtonPressed(0, nau::input::MouseKey::ButtonLeft))
        {
            m_touchCaptured = true;
            intptr_t id = 0;
            touchBegin(1, &id, &cursorX, &cursorY);
        }

        if (m_touchCaptured)
        {
            intptr_t id = 0;
            touchesMove(1, &id, &cursorX, &cursorY, nullptr, nullptr);
        }

        if (nau::input::isMouseButtonReleased(0, nau::input::MouseKey::ButtonLeft))
        {
            if (m_touchCaptured)
            {
                m_touchCaptured = false;
                intptr_t id = 0;
                touchEnd(1, &id, &cursorX, &cursorY);
            }
        }

        for (const auto& [nauMouseKey, cocosMouseKey] : m_mouseButtonRemap)
        {
            if (nau::input::isMouseButtonPressed(0, nauMouseKey))
            {
                cocos2d::EventMouse event(cocos2d::EventMouse::MouseEventType::MOUSE_DOWN);
                event.setCursorPosition(cursorX, cursorY);
                event.setMouseButton(cocosMouseKey);
                cocos2d::Director::getInstance()->getEventDispatcher()->dispatchEvent(&event);
            }

            if (nau::input::isMouseButtonPressed(0, nauMouseKey))
            {
                cocos2d::EventMouse event(cocos2d::EventMouse::MouseEventType::MOUSE_UP);
                event.setCursorPosition(cursorX, cursorY);
                event.setMouseButton(cocosMouseKey);
                cocos2d::Director::getInstance()->getEventDispatcher()->dispatchEvent(&event);
            }
        }

        static constexpr float scrollSensitivity = 10.f;

        float mouseWheelDelta = nau::input::getMouseAxisDelta(0, nau::input::MouseKey::Wheel) * scrollSensitivity;
        if (mouseWheelDelta != 0.f)
        {
            cocos2d::EventMouse event(cocos2d::EventMouse::MouseEventType::MOUSE_SCROLL);
            event.setScrollData(0, -mouseWheelDelta);
            event.setCursorPosition(cursorX, m_screenHeight - cursorY);
            cocos2d::Director::getInstance()->getEventDispatcher()->dispatchEvent(&event);
        }
    }

    void UiManagerImpl::touchMove(const float xs[], const float ys[])
    {
        intptr_t id = 1;
        float x = xs[0];
        float y = m_screenHeight - ys[0];

        static float sx = 0;
        static float sy = 0;

        if (x == sx && y == sy)
        {
            return;
        }
        sx = x;
        sy = y;

        cocos2d::EventMouse mouseEvent(cocos2d::EventMouse::MouseEventType::MOUSE_MOVE);
        mouseEvent.setCursorPosition(x, y);

        auto dispatcher = cocos2d::Director::getInstance()->getEventDispatcher();
        dispatcher->dispatchEvent(&mouseEvent);
    }

    void UiManagerImpl::touchesMove(int num, intptr_t ids[], const float xs[], const float ys[], float fs[], float ms[])
    {
        intptr_t id = 0;
        float x = 0.0f;
        float y = 0.0f;
        float force = 0.0f;
        float maxForce = 0.0f;
        cocos2d::EventTouch touchEvent;
        std::vector<cocos2d::Touch*> touches;

        for (int i = 0; i < num; ++i)
        {
            id = ids[i];
            x = xs[i];
            y = ys[i];
            force = fs ? fs[i] : 0.0f;
            maxForce = ms ? ms[i] : 0.0f;

            auto iter = g_touchIdReorderMap.find(id);
            if (iter == g_touchIdReorderMap.end())
            {
                NAU_LOG_INFO("if the index doesn't exist, it is an error");
                continue;
            }

            cocos2d::Touch* touch = g_touches[iter->second];
            if (touch)
            {
                touch->setTouchInfo(iter->second, x, y, force, maxForce);
                touches.push_back(touch);
            }
            else
            {
                NAU_LOG_ERROR("Ending mouse moved touches with id: {} error", static_cast<long>(id));
                return;
            }
        }

        if (touches.size() == 0)
        {
            NAU_LOG_INFO("touchesMoved: size = 0");
            return;
        }

        touchEvent.setTouches(std::move(touches));
        touchEvent.setEventCode(cocos2d::EventTouch::EventCode::MOVED);
        auto dispatcher = cocos2d::Director::getInstance()->getEventDispatcher();
        dispatcher->dispatchEvent(&touchEvent);
    }

    void UiManagerImpl::touchBegin(int num, intptr_t ids[], const float xs[], const float ys[])
    {
        intptr_t id = 0;
        float x = 0.0f;
        float y = 0.0f;
        int unusedIndex = 0;
        cocos2d::EventTouch touchEvent;

        std::vector<cocos2d::Touch*> touches;
        for (int i = 0; i < num; ++i)
        {
            id = ids[i];
            x = xs[i];
            y = ys[i];

            auto iter = g_touchIdReorderMap.find(id);

            // it is a new touch
            if (iter == g_touchIdReorderMap.end())
            {
                unusedIndex = getUnUsedIndex();

                // The touches is more than MAX_TOUCHES ?
                if (unusedIndex == -1)
                {
                    NAU_LOG_INFO("The touches is more than MAX_TOUCHES, unusedIndex = {}", unusedIndex);
                    continue;
                }

                // TODO: allocate within NAU allocator
                // cocos2d::Touch* touch = g_touches[unusedIndex] = new(nau::getDefaultAllocator()->allocate(sizeof(cocos2d::Touch))) cocos2d::Touch();;
                cocos2d::Touch* touch = g_touches[unusedIndex] = new(std::nothrow) cocos2d::Touch();
                ;
                touch->setTouchInfo(unusedIndex, x, y);

                NAU_LOG_INFO("x = {} y = {}", touch->getLocationInView().x, touch->getLocationInView().y);

                g_touchIdReorderMap.emplace(id, unusedIndex);
                touches.push_back(touch);
            }
        }

        if (touches.size() == 0)
        {
            NAU_LOG_INFO("touchesBegan: size = 0");
            return;
        }

        touchEvent.setTouches(std::move(touches));
        touchEvent.setEventCode(cocos2d::EventTouch::EventCode::BEGAN);
        auto dispatcher = cocos2d::Director::getInstance()->getEventDispatcher();
        dispatcher->dispatchEvent(&touchEvent);
    }

    void UiManagerImpl::touchEnd(int num, intptr_t ids[], const float xs[], const float ys[])
    {
        intptr_t id = 0;
        float x = 0.0f;
        float y = 0.0f;
        cocos2d::EventTouch touchEvent;
        std::vector<cocos2d::Touch*> touches;

        for (int i = 0; i < num; ++i)
        {
            id = ids[i];
            x = xs[i];
            y = ys[i];

            auto iter = g_touchIdReorderMap.find(id);
            if (iter == g_touchIdReorderMap.end())
            {
                NAU_LOG_ERROR("if the index doesn't exist, it is an error");
                continue;
            }

            cocos2d::Touch* touch = g_touches[iter->second];
            if (touch)
            {
                NAU_LOG_INFO("Ending touches with id: {}, x={}, y={}", (int)id, x, y);
                touch->setTouchInfo(iter->second, x, y);
                touches.push_back(touch);

                g_touches[iter->second] = nullptr;
                removeUsedIndexBit(iter->second);

                g_touchIdReorderMap.erase(id);
            }
            else
            {
                NAU_LOG_ERROR("Ending touches with id: {} error", static_cast<long>(id));
                return;
            }
        }

        if (touches.size() == 0)
        {
            NAU_LOG_INFO("touchesEnded or touchesCancel: size = 0");
            return;
        }

        touchEvent.setTouches(std::move(touches));
        touchEvent.setEventCode(cocos2d::EventTouch::EventCode::ENDED);
        auto dispatcher = cocos2d::Director::getInstance()->getEventDispatcher();
        dispatcher->dispatchEvent(&touchEvent);

        for (auto& touch : touches)
        {
            touch->release();
        }
    }

    void UiManagerImpl::render(BaseTexture* backBuffer)
    {
        lock_(m_cocosRenderMutex);
        if (!m_isAlive)
        {
            return;
        }
        cocos2d::Director* director = cocos2d::Director::getInstance();
        cocos2d::RenderView* glview = director->getRenderView();

        director->renderScene(backBuffer);
    }

    bool UiManagerImpl::applicationDidFinishLaunching()
    {
        // initialize director
        cocos2d::Director* director = cocos2d::Director::getInstance();
        cocos2d::RenderView* glview = director->getRenderView();

        director->setAnimationIntervalSetterDelegate(animationIntervalSetterDelegate);

        if (!glview)
        {
            glview = nau::ui::UIRenderView::create(designResolutionSize.width, designResolutionSize.height);
            director->setOpenGLView(glview);
            glview->setApplicationWillEnterForegroundCb(applicationWillEnterForegroundCb);
            glview->setApplicationDidEnterBackgroundCb(applicationDidEnterBackgroundCb);
        }

        // turn on display FPS
        director->setDisplayStats(false);

        // set FPS. the default value is 1.0/60 if you don't call this
        director->setAnimationInterval(1.0f / 60);

        // Set the design resolution
        glview->setDesignResolutionSize(designResolutionSize.width, designResolutionSize.height, ResolutionPolicy::NO_BORDER);
        director->setContentScaleFactor(1.f);

        // create a scene. it's an autorelease object
        auto scene = cocos2d::Scene::create();

        // run
        director->runWithScene(scene);

        return true;
    }

    async::Task<> UiManagerImpl::shutdownService()
    {
        lock_(m_cocosRenderMutex);
        m_isAlive = false;

        {
            cocos2d::Director* director = cocos2d::Director::getInstance();
            cocos2d::RenderView* glview = director->getRenderView();

            // Director should still do a cleanup if the window was closed manually.
            if (glview->isOpenGLReady())
            {
                director->end();
                director->mainLoop();
                director = nullptr;
            }
            glview->release();

            for (auto* touch : g_touches)
            {
                CC_SAFE_RELEASE(touch);
            }
        }
        cocos2d::Ref::printLeaks();

        ///////////////////////////////////////////////////////////////////////////
        /////////////// restoring timer resolution
        ///////////////////////////////////////////////////////////////////////////
        if (wTimerRes != 0)
        {
            timeEndPeriod(wTimerRes);
        }

        return async::Task<>::makeResolved();
    }

    void UiManagerImpl::applicationDidEnterBackground()
    {
    }

    void UiManagerImpl::applicationWillEnterForeground()
    {
    }

    void UiManagerImpl::setAnimationInterval(float interval)
    {
        lock_(m_cocosRenderMutex);
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        m_animationInterval.QuadPart = (LONGLONG)(interval * freq.QuadPart);
    }

    bool UiManagerImpl::shouldShutDown() const
    {
        cocos2d::Director* director = cocos2d::Director::getInstance();
        cocos2d::RenderView* glview = director->getRenderView();
        return glview->windowShouldClose();
    }

    void UiManagerImpl::initGLContextAttrs()
    {
    }

    void UiManagerImpl::notifyElementsChanged()
    {
#if NAU_UI_CALLBACK_ON_ELEMNT_CHANGE
        if (m_callbacks.size() == 0)
        {
            return;
        }

        for (const auto& [canvasName, canvas] : m_canvases)
        {
            canvas->callRecursivly([this](Node* node)
            {
                if (node->isDirty())
                {
                    invokeElementChangedCallbacks(node);
                    node->markClean();
                }
            });
        }
#endif
    }

    void UiManagerImpl::invokeElementChangedCallbacks(Node* node)
    {
#if NAU_UI_CALLBACK_ON_ELEMNT_CHANGE
        for (const auto& entry : m_callbacks) 
        {
            if (entry.callback) 
            {
                entry.callback(node);
            }
        }
        
        if (m_needsCleanup) 
        {
            cleanupCallbacks();
        }
#endif
    }

    void UiManagerImpl::cleanupCallbacks()
    {
#if NAU_UI_CALLBACK_ON_ELEMNT_CHANGE
        std::lock_guard<std::mutex> lock(m_elementChangedCbMutex);
        m_callbacks.erase(
            std::remove_if(m_callbacks.begin(), m_callbacks.end(), [](const ElementChangedCallbackEntry& entry) {
                return entry.callback == nullptr;
            }),
            m_callbacks.end()
        );
        m_needsCleanup = false;
#endif
    }

    void UiManagerImpl::setEngineScene(scene::IScene::WeakRef scene)
    {
        m_engineScene = scene;
    }

    scene::IScene* UiManagerImpl::getEngineScene()
    {
        return m_engineScene.get();
    }

    nau::ui::Node* UiManagerImpl::getCanvasChildUnderCursor(const std::string& canvasName)
    {
#if NODE_DEBUG_SEARCH
        Canvas* canvas = getCanvas(canvasName);
        if (!canvas)
        {
            return nullptr;
        }

        const auto [cursorX, cursorY] = getInputPosition(
            nau::input::getMouseAxisValue(0, nau::input::MouseKey::AxisX),
            nau::input::getMouseAxisValue(0, nau::input::MouseKey::AxisY));

        std::queue<Node*> nodesQueue;
        std::vector<Node*> children;
        canvas->getChildren(children);

        for (const auto& child : children)
        {
            nodesQueue.push(child);
        }

        while (!nodesQueue.empty())
        {
            Node* currentNode = nodesQueue.front();
            nodesQueue.pop();

            // TODO:Again hack with inverting mouse Y - (m_screenHeight - cursorY)
            math::vec2 const localCursorPosition = currentNode->convertToNodeSpace({cursorX, m_screenHeight - cursorY});

            cocos2d::Rect rect;
            rect.size = currentNode->getContentSize();

            if (rect.containsPoint(localCursorPosition))
            {
                return currentNode;
            }

            std::vector<Node*> childNodes;
            currentNode->getChildren(childNodes);
            for (const auto& childNode : childNodes)
            {
                nodesQueue.push(childNode);
            }
        }

#endif
        return nullptr;
    }

    void UiManagerImpl::setElementChangedCallback(nau::Uid elementUid, TElementChangedCallback&& callback)
    {
#if NAU_UI_CALLBACK_ON_ELEMNT_CHANGE

        std::lock_guard<std::mutex> lock(m_elementChangedCbMutex);
        m_callbacks.push_back({ elementUid, std::move(callback) });
#endif
    }

    void UiManagerImpl::removeElementChangedCallback(nau::Uid id)
    {
#if NAU_UI_CALLBACK_ON_ELEMNT_CHANGE
        std::lock_guard<std::mutex> lock(m_elementChangedCbMutex);
        for (auto& entry : m_callbacks) 
        {
            if (entry.id == id) 
            {
                entry.callback = nullptr;
                m_needsCleanup = true;
                break;
            }
        }
#endif
    }

    void UiManagerImpl::setScreenSize(float wight, float height)
    {
        lock_(m_cocosRenderMutex);
        m_screenWidght = wight;
        m_screenHeight = height;

        cocos2d::Director* director = cocos2d::Director::getInstance();
        cocos2d::RenderView* glview = director->getRenderView();
        glview->setFrameSize(m_screenWidght, m_screenHeight);
        glview->setDesignResolutionSize(m_screenWidght, m_screenHeight, ResolutionPolicy::NO_BORDER);

        updateCanvases();
    }

    void UiManagerImpl::setReferenceResolution(float width, float height)
    {
        m_screenWidghtReference = width;
        m_screenHeightReference = height;
    }

    std::pair<float, float> UiManagerImpl::getInputPosition(float axisX, float axisY) const
    {
        float inputX = m_screenWidght * axisX;
        float inputY = m_screenHeight * axisY;

        if (m_screenWidghtReference > 0.0f && m_screenHeightReference > 0.0f)
        {
            inputX = inputX * (m_screenWidghtReference / m_screenWidght);
            inputY = inputY * (m_screenHeightReference / m_screenHeight);
        }

        return {inputX, inputY};
    }

    void UiManagerImpl::addCanvas(Canvas* canvas)
    {
        NAU_ASSERT(canvas);
        NAU_ASSERT(m_canvases.find(canvas->getCanvasName()) == m_canvases.end(), "UI: Attempt to add same canvas twice");

        cocos2d::Director* director = cocos2d::Director::getInstance();

        if (!m_cocoScene)
        {
            m_cocoScene = cocos2d::Scene::createWithSize(canvas->getReferenceSize());
            director->replaceScene(m_cocoScene);
        }

        m_cocoScene->addChild(canvas);

        const math::vec2 halfRealSize = math::vec2{m_screenWidght, m_screenHeight} * 0.5f;
        const math::vec2 referenceSize = canvas->getReferenceSize();
        const math::vec2 halfScaledReferenceSize = math::vec2{referenceSize.getX() * canvas->getScaleX(), referenceSize.getY() * canvas->getScaleY()} * 0.5f;
        canvas->setPosition(halfRealSize - halfScaledReferenceSize);
        m_canvases[canvas->getCanvasName()] = canvas;

        {
            lock_(m_canvasLoadedCbMutex);
            for (auto& [handle, cb] : m_canvasLoadedCbs)
            {
                cb(canvas->getCanvasName());
            }
        }
    }

    Canvas* UiManagerImpl::getCanvas(const std::string& canvasName)
    {
        auto it = m_canvases.find(canvasName);

        if (it != m_canvases.end())
        {
            return it->second;
        }
        else
        {
            NAU_LOG_ERROR("Canvas id:{} not found", canvasName);
            return nullptr;
        }
    }

    void UiManagerImpl::removeCanvas(const std::string& canvasName)
    {
        NAU_ASSERT(m_cocoScene);

        Canvas* canvas = getCanvas(canvasName);
        NAU_ASSERT(canvas, "UI: Attempt to remove inexistent canvas {}", canvasName);
        m_cocoScene->removeChild(canvas);
        m_canvases.erase(canvasName);
    }

    void UiManagerImpl::updateCanvases()
    {
         for (auto& pair : m_canvases) 
         {
            pair.second->setRescalePolicy(pair.second->getRescalePolicy());

            const math::vec2 referenceSize = pair.second->getReferenceSize();
            const math::vec2 halfRealSize = math::vec2{m_screenWidght, m_screenHeight} * 0.5f;
            const math::vec2 halfScaledReferenceSize = math::vec2{referenceSize.getX() * pair.second->getScaleX(), referenceSize.getY() * pair.second->getScaleY()} * 0.5f;
            pair.second->setPosition(halfRealSize - halfScaledReferenceSize);
         }
    }

    UiManager::TCallbackHandle UiManagerImpl::setOnCanvasLoadedCallback(TCanvasLoadedCallback&& callback)
    {
        lock_(m_canvasLoadedCbMutex);

        uint32_t handle = INVALID_CB_HANDLE;
        if (freeCanvasLoadedCallbackHandles.size() > 0)
        {
            handle = freeCanvasLoadedCallbackHandles.back();
            freeCanvasLoadedCallbackHandles.pop_back();
        }
        else
        {
            handle = highestFreeCanvasLoadedCallbackHandle++;
        }

        m_canvasLoadedCbs[handle] = std::move(callback);

        return handle;
    }

    bool UiManagerImpl::setOnCanvasLoadedCallback(TCallbackHandle handle)
    {
        lock_(m_canvasLoadedCbMutex);

        auto it = m_canvasLoadedCbs.find(handle);
        if (it == m_canvasLoadedCbs.end())
        {
            NAU_LOG_WARNING("Attempt to remove canvas loaded callback with invalid handle {}", handle);
            return false;
        }

        freeCanvasLoadedCallbackHandles.push_back(handle);
        m_canvasLoadedCbs.erase(it);

        return true;
    }

    void UiManagerImpl::configureResourcePath()
    {
        std::string path = "Resources";

        if (path[path.length() - 1] != '/')
        {
            path += '/';
        }

        cocos2d::FileUtils* pFileUtils = cocos2d::FileUtils::getInstance();

        std::vector<std::string> searchPaths = pFileUtils->getSearchPaths();

        searchPaths.insert(searchPaths.begin(), path);

        pFileUtils->setSearchPaths(searchPaths);
    }

    std::vector<const rtti::TypeInfo*> UiManagerImpl::getServiceDependencies() const
    {
        return {&rtti::getTypeInfo<ICoreGraphics>(), &rtti::getTypeInfo<cocos2d::backend::IDeviceProvider>()};
    }
}  // namespace nau::ui
