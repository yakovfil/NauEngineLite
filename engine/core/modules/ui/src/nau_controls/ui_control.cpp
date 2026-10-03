// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/ui/ui_control.h"

#include <algorithm>
#include <limits>

#include "base/CCEventDispatcher.h"
#include "base/CCEventListenerMouse.h"
#include "base/CCEventListenerTouch.h"
#include "base/CCTouch.h"

namespace nau::ui
{

UIControl::UIControl()
{
    addTouchListener();
}

UIControl::~UIControl() {}

void UIControl::setInteractable(bool interactable) 
{
    markDirty();
    m_interactable = interactable;

    if (m_interactable)
    {
        addTouchListener();
    }
    else
    {
        releaseTouchListener();
    }
}

bool UIControl::isInteractable() const 
{
    return m_interactable;
}

bool UIControl::isTouchCaptured() const
{
    return m_touchCaptured;
}

bool UIControl::isMouseCaptured() const
{
    return m_mouseCaptured;
}

void UIControl::handleEvent(EventType eventType) {}

bool UIControl::initialize()
{
    if (Node::initialize())
    {
        setAnchorPoint(cocos2d::Vec2::ANCHOR_MIDDLE);
        return true;
    }

    return false;
}

void UIControl::addTouchListener()
{
    auto touchListener = cocos2d::EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(false);

    touchListener->onTouchBegan = [this](cocos2d::Touch* touch, cocos2d::Event* event) -> bool 
    {
        if (!m_interactable)
        {
            return false;
        }
        cocos2d::Vec2 const touchPosition = convertToNodeSpace(touch->getLocation());

        if(isInputEventInElementBorder(touchPosition) && isInteractableAndVisible(touchPosition))
        {
            m_touchCaptured = true;
            handleEvent(EventType::press);

            if(m_onPressed)
            {
                m_onPressed(touchPosition);
            }

            return true;
        }

        return false;
    };

    touchListener->onTouchMoved = [this](cocos2d::Touch* touch, cocos2d::Event* event)
    {
        if (!m_interactable)
        {
            return;
        }

        if(!m_touchCaptured)
        {
            return;
        }

        auto delta = touch->getDelta();
        const float epsilon = std::numeric_limits<float>::epsilon();

        if (std::fabs(delta.x) < epsilon && std::fabs(delta.y) < epsilon)
        {
            return;
        }

        cocos2d::Vec2 const touchPosition = convertToNodeSpace(touch->getLocation());

        if (m_touchMovedCallback)
        {
            m_touchMovedCallback(touchPosition, delta);
        }
    };

    touchListener->onTouchEnded = [this](cocos2d::Touch* touch, cocos2d::Event* event) 
    {
        if (!m_interactable)
        {
            return;
        }

        if(m_touchCaptured)
        {
            handleEvent(EventType::release);
            if(m_onReleased)
            {
                m_onReleased();
            }

            m_touchCaptured = false;
        }
    };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);

    auto mouseListener = cocos2d::EventListenerMouse::create();

    if (mouseListener)
    {
        mouseListener->onMouseMove = [this](cocos2d::Event* mouseEvent) 
        {
            const cocos2d::EventMouse* e = static_cast<const cocos2d::EventMouse*>(mouseEvent);
            const cocos2d::Vec2 mousePosition = cocos2d::Vec2(e->getCursorX(), e->getCursorY());
            const cocos2d::Vec2 mousePositionLocal = convertToNodeSpace(mousePosition);

            if (isInputEventInElementBorder(mousePositionLocal) && isInteractableAndVisible(mousePositionLocal))
            {
                handleEvent(EventType::hover);
                if(m_onHover)
                {
                    m_onHover(mousePositionLocal);
                }
                m_mouseCaptured = true;
            }
            else
            {
                if(m_mouseCaptured)
                {
                    handleEvent(EventType::leave);
                    if(m_onLeave)
                    {
                        m_onLeave();
                    }
                    m_mouseCaptured = false;
                }
            }
        };

        _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);
    }
}

void UIControl::releaseTouchListener()
{
    _eventDispatcher->removeEventListenersForTarget(this);
}

UIControl* UIControl::getAncestorWidget(Node* node)
{
    if (nullptr == node)
    {
        return nullptr;
    }

    Node* parent = node->getParent();

    if (nullptr == parent)
    {
        return nullptr;
    }

    UIControl* parentWidget = dynamic_cast<UIControl*>(parent);

    if (parentWidget)
    {
        return parentWidget;
    }
    else
    {
        return getAncestorWidget(parent);
    }
}

bool UIControl::isInteractableAndVisible(math::vec2 localInputPosition)
{
    UIControl* childWidget = this;
    UIControl* parentWidget = nullptr;

    auto worldInput = convertToWorldSpace(localInputPosition);

    auto getBoundingBoxToWorld = [](Node* node) -> cocos2d::Rect 
    {
        math::vec2 worldBottomLeft = node->convertToWorldSpace({0, 0});
        math::vec2 worldTopRight = node->convertToWorldSpace(node->getContentSize());
        
        cocos2d::Rect worldBoundingBox(worldBottomLeft.getX(), worldBottomLeft.getY(), 
                              worldTopRight.getX() - worldBottomLeft.getX(), 
                              worldTopRight.getY() - worldBottomLeft.getY());
        return worldBoundingBox;
    };

    do
    {
        parentWidget = getAncestorWidget(childWidget);

        if(parentWidget)
        {
            if (!parentWidget->isInteractable() || !parentWidget->isVisible())
            {
                return false;
            }

            if(m_inNeedRestrictInputForChildWidgets)
            {
                cocos2d::Rect childRect = getBoundingBoxToWorld(childWidget);
                cocos2d::Rect parentRect = getBoundingBoxToWorld(parentWidget);

                cocos2d::Rect intersection;
                bool intersects = childRect.intersectsRect(parentRect);
                if (intersects) 
                {
                    float intersectLeft = std::max(childRect.getMinX(), parentRect.getMinX());
                    float intersectRight = std::min(childRect.getMaxX(), parentRect.getMaxX());

                    float intersectBottom = std::max(childRect.getMinY(), parentRect.getMinY());
                    float intersectTop = std::min(childRect.getMaxY(), parentRect.getMaxY());

                    if (intersectLeft < intersectRight && intersectBottom < intersectTop) 
                    {
                        intersection = cocos2d::Rect(intersectLeft, intersectBottom, 
                                    intersectRight - intersectLeft, 
                                    intersectTop - intersectBottom);
                    }
                    else
                    {
                        intersection = cocos2d::Rect(0, 0, 0, 0);
                    }
                }

                if (!intersection.containsPoint(worldInput)) 
                {
                    return false;
                }
            }

            childWidget = parentWidget;
        }
    }
    while(parentWidget != nullptr);

    return true;
}

bool UIControl::isInputEventInElementBorder(math::vec2 inputPosition)
{
    cocos2d::Rect rect;
    rect.size = getContentSize();

    return rect.containsPoint(inputPosition);
}

}
