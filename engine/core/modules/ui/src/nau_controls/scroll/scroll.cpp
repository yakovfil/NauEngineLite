// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/ui/scroll.h"

#include <cocos/base/CCDirector.h>

#include <algorithm>

#include "2d/CCDrawNode.h"
#include "base/CCEventDispatcher.h"
#include "base/CCEventListenerMouse.h"
#include "nau/ui/elements/clipping_node.h"

namespace nau::ui
{
    const int NauScroll::CLIPPER_TAG = 1;
    
    NauScroll::NauScroll()
    {
        setAnchorPoint(cocos2d::Vec2::ANCHOR_MIDDLE);

        m_inNeedRestrictInputForChildWidgets = true;
    }

    NauScroll::~NauScroll() {};

    NauScroll::ScrollType NauScroll::getScrollType()
    {
        return m_scrollType;
    };

    void NauScroll::setScrollType(NauScroll::ScrollType scrollType)
    {
        if(m_scrollType == scrollType)
        {
            return;
        }

        m_scrollType = scrollType;

        if(m_scrollType == NauScroll::ScrollType::horizontal)
        {
            reorderChildAsHorizontally();
        }
        else if(m_scrollType == NauScroll::ScrollType::vertical)
        {
            reorderChildAsVertically();
        }

#if UI_ELEMENT_DEBUG
        drawContentRect();
#endif
    };

    void NauScroll::reorderChildAsVertically()
    {
        markDirty();

        math::vec2 totalContentSize = {0, 0};
        float accumulatedHeight = 0.0f;

        for (const auto& contentItem : m_content) 
        {
            const math::vec2 itemSize = contentItem->getContentSize();
            totalContentSize.setX(std::max(totalContentSize.getX(), itemSize.getX()));
            totalContentSize.setY(totalContentSize.getY() + itemSize.getY());
        }

        m_contentRoot->setContentSize(totalContentSize);

    
        for (size_t i = m_content.size(); i > 0; --i) 
        {
            auto& contentItem = m_content[i - 1];
        
            float itemYPosition = accumulatedHeight + (contentItem->getContentSize().getY() * 0.5f);
            contentItem->setPosition({
                totalContentSize.getX() * 0.5f,
                itemYPosition
            });

            accumulatedHeight += contentItem->getContentSize().getY();
        }

        m_contentRoot->setPosition(getContentSize() * 0.5f);
        if (!m_content.empty()) 
        {
            moveTo(m_content[0]);
        }
    }

    void NauScroll::reorderChildAsHorizontally()
    {
        markDirty();

        math::vec2 totalContentSize = {0, 0};
        float accumulatedWidth = 0.0f;

        for (const auto& contentItem : m_content) {
            const math::vec2 itemSize = contentItem->getContentSize();
            totalContentSize.setX(totalContentSize.getX() + itemSize.getX());
            totalContentSize.setY(std::max(totalContentSize.getY(), itemSize.getY()));
        }

        m_contentRoot->setContentSize(totalContentSize);

        for (auto& contentItem : m_content) 
        {
            float itemXPosition = accumulatedWidth + (contentItem->getContentSize().getX() * 0.5f);
            contentItem->setPosition({
                itemXPosition,
                totalContentSize.getY() * 0.5f
            });

            accumulatedWidth += contentItem->getContentSize().getX();
        }

        m_contentRoot->setPosition(getContentSize() * 0.5f);
        if (!m_content.empty()) 
        {
            moveTo(m_content[0]);
        }
    }

    NauScroll* NauScroll::create(NauScroll::ScrollType scrollType)
    {
        if (NauScroll* scroll = Node::create<NauScroll>())
        {
            scroll->m_scrollType = scrollType;
            return scroll;
        }

        return nullptr;
    }

    NauScroll* NauScroll::create(NauScroll::ScrollType scrollType, const math::vec2& size)
    {
        if (NauScroll* scroll = Node::create<NauScroll>())
        {
            scroll->m_scrollType = scrollType;
            scroll->setContentSize(size);
            return scroll;
        }

        return nullptr;
    }

    bool NauScroll::initialize()
    {
        if (!UIControl::initialize()) 
        {
            return false;
        }
        
        ClippingNode* clipper = ClippingNode::create();
        clipper->setAnchorPoint(cocos2d::Vec2::ANCHOR_MIDDLE);
        clipper->nau_setName("clipper");
        Node::addChild(clipper, "clipper");

        DrawNode* stencil = DrawNode::create();
        clipper->setStencil(stencil);
        
        m_contentRoot = Node::create();
        m_contentRoot->setAnchorPoint(cocos2d::Vec2::ANCHOR_MIDDLE);
        clipper->addChild(m_contentRoot);

        setOnTouchMovedCallback([this](const math::vec2& mousePosition, const math::vec2& delta) 
        {
            onScrollMovedByDrag(delta);
        });

        setOnPressedCallback([this](const math::vec2& mousePosition) 
        {
            if(isInputEventInScrollBarButtonBorder(mousePosition))
            {
                m_scrollTumbCaptured = true;
            }
        });

        setOnReleasedCallback([this]() 
        {
            m_scrollTumbCaptured = false;
        });

        auto mouseListener = cocos2d::EventListenerMouse::create();

        mouseListener->onMouseScroll = [this](cocos2d::Event* mouseEvent) 
        {
            const cocos2d::EventMouse* e = static_cast<const cocos2d::EventMouse*>(mouseEvent);
            const cocos2d::Vec2 mousePosition = cocos2d::Vec2(e->getCursorX(), e->getCursorY());
            const cocos2d::Vec2 mousePositionLocal = convertToNodeSpace(mousePosition);

            const math::vec2 scrollWidgetSize = getContentSize();
            const math::vec2 scrollCentrPosition = getPosition();

            cocos2d::Rect rect;
            rect.size = scrollWidgetSize;

            if (rect.containsPoint(mousePositionLocal))
            {
                onScrollMovedByWheel({e->getScrollX(), e->getScrollY()});
            }
        };

        mouseListener->onMouseMove = [this](cocos2d::Event* mouseEvent) 
        {
            if(m_scrollTumbCaptured)
            {
                const cocos2d::EventMouse* e = static_cast<const cocos2d::EventMouse*>(mouseEvent);
                const cocos2d::Vec2 mousePosition = cocos2d::Vec2(e->getCursorX(), e->getCursorY());
                const cocos2d::Vec2 mousePositionLocal = convertToNodeSpace(mousePosition);
                onScrollMovedByScrollBar(mousePositionLocal);
            }
        };

        _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);

#if UI_ELEMENT_DEBUG
        m_contentDebugNode = DrawNode::create();
        clipper->addChild(m_contentDebugNode);
#endif
        return true;
    }

    void NauScroll::onScrollMovedByWheel(const math::vec2& delta)
    {
        //only Y delta in scroll move event
        math::vec2 convertedDelta;

        if(m_scrollType == NauScroll::ScrollType::horizontal)
        {
            convertedDelta = {-delta.getY(), 0.0f};
        }

        if(m_scrollType == NauScroll::ScrollType::vertical)
        {   
            convertedDelta = {0.0f, -delta.getY()};
        }

        moveScroll(convertedDelta);
        
    }

    void NauScroll::onScrollMovedByDrag(const math::vec2& delta)
    {
        //it is necessary to move in the direction opposite to the change in coordinates
        math::vec2 convertedDelta;

        if(m_scrollType == NauScroll::ScrollType::horizontal)
        {
            convertedDelta = {-delta.getX(), 0.0f};
        }

        if(m_scrollType == NauScroll::ScrollType::vertical)
        {   
            convertedDelta = {0.0f, delta.getY()};
        }

        moveScroll(convertedDelta);
    }

    void NauScroll::onScrollMovedByScrollBar(const math::vec2& mousePosition)
    {
        math::vec2 calculatedDelta;

        if(m_scrollType == NauScroll::ScrollType::horizontal)
        {
            calculatedDelta = {mousePosition.getX() - m_scrollBarSprite->getPositionX(), 0.0f};
        }

        if(m_scrollType == NauScroll::ScrollType::vertical)
        {   
            calculatedDelta = {0.0f, -(mousePosition.getY() - m_scrollBarSprite->getPositionY())};
        }

        moveScroll(calculatedDelta);
    }

    void NauScroll::moveScroll(const math::vec2& delta)
    {
        markDirty();
        math::vec2 currentPosition = m_contentRoot->getPosition();
        math::vec2 afterDragContentPosition;

        auto x = delta.getX();

        if(m_scrollType == NauScroll::ScrollType::horizontal)
        {
            afterDragContentPosition = currentPosition - delta;

            //Check for overflow of horizontal scroll boundaries
            if(afterDragContentPosition.getX() + m_contentRoot->getContentSize().getX() * 0.5f < getContentSize().getX())
            {
                m_contentRoot->setPositionX(getContentSize().getX() - (m_contentRoot->getContentSize().getX() * 0.5f));
                return;
            }

            if(afterDragContentPosition.getX() > m_contentRoot->getContentSize().getX() * 0.5f)
            {
                m_contentRoot->setPositionX(m_contentRoot->getContentSize().getX() * 0.5f);
                return;
            }
        }

        if(m_scrollType == NauScroll::ScrollType::vertical)
        {   
            afterDragContentPosition = currentPosition + delta;

            //Check for overflow of vertical scroll boundaries
            if(afterDragContentPosition.getY() + m_contentRoot->getContentSize().getY() * 0.5f < getContentSize().getY())
            {
                m_contentRoot->setPositionY(getContentSize().getY() - (m_contentRoot->getContentSize().getY() * 0.5f));
                return;
            }

            if(afterDragContentPosition.getY() > m_contentRoot->getContentSize().getY() * 0.5f)
            {
                m_contentRoot->setPositionY(m_contentRoot->getContentSize().getY() * 0.5f);
                return;
            }
        }

        m_contentRoot->setPosition(afterDragContentPosition);

        updateScrollBarSpritePosition();


#if UI_ELEMENT_DEBUG
        drawContentRect();
#endif
    }

    void NauScroll::addChild(Node* item)
    {
        markDirty();
        m_contentRoot->addChild(item);     
        m_content.push_back(item);

#if UI_ELEMENT_DEBUG
        drawContentRect();
#endif
    }

    void NauScroll::addChildWithAlignment(Node* item)
    {
        if(m_scrollType == NauScroll::ScrollType::horizontal)
        {
            addChildAsHorizontally(item);
        }

        if(m_scrollType == NauScroll::ScrollType::vertical)
        {
            addChildAsVertically(item);
        }

#if UI_ELEMENT_DEBUG
        drawContentRect();
#endif
    }

    void NauScroll::addChildAsHorizontally(Node* item)
    {
        markDirty();
        item->setAnchorPoint(cocos2d::Vec2::ANCHOR_MIDDLE);

        const math::vec2 itemSize = item->getContentSize();
        const math::vec2 contentSize = m_contentRoot->getContentSize();

        if(m_content.empty())
        {
            m_contentRoot->setContentSize(itemSize);    
        }
        else
        {
            m_contentRoot->setContentSize({contentSize.getX() + itemSize.getX(),
                                           std::max(contentSize.getY(), itemSize.getY())});
        }

        m_contentRoot->addChild(item);     

        if (!m_content.empty()) 
        {
            Node* lastNode = m_content.back();

            item->setPosition({
                (lastNode->getPositionX() + lastNode->getContentSize().getX() * 0.5f) + (item->getContentSize().getX() * 0.5f),
                item->getContentSize().getY() * 0.5f});
        }
        else
        {
            item->setPosition({item->getContentSize() * 0.5f});
        }
        
        m_content.push_back(item);
    }

    void NauScroll::addChildAsVertically(Node* item)
    {
        markDirty();
        item->setAnchorPoint(cocos2d::Vec2::ANCHOR_MIDDLE);

        const math::vec2 itemSize = item->getContentSize();
        const math::vec2 contentSize = m_contentRoot->getContentSize();

        if(m_content.empty())
        {
            m_contentRoot->setContentSize(itemSize);    
        }
        else
        {
            m_contentRoot->setContentSize({std::max(contentSize.getX(), itemSize.getX()),
                                           contentSize.getY() + itemSize.getY()});
        }

        m_contentRoot->addChild(item);     

        for(auto& contentItem : m_content)
        {
            contentItem->setPositionY(contentItem->getPositionY() + item->getContentSize().getY());
        }
        
        m_content.push_back(item);

        item->setPosition({item->getContentSize() * 0.5f});
    }

    void NauScroll::removeChild(Node* item)
    {
        if(m_content.empty())
        {
            return;
        }

        markDirty();
        auto it = std::find(m_content.begin(), m_content.end(), item);
        if (it != m_content.end())
        {
            it = m_content.erase(it);
            m_contentRoot->removeChild(item);
        }
        else
        {
            NAU_LOG_ERROR("Scroll content node not found");
        }
    }

    void NauScroll::removeChildWithAlignment(Node* item)
    {
        if(m_content.empty())
        {
            return;
        }

        if(m_scrollType == NauScroll::ScrollType::horizontal)
        {
            removeChildAsHorizontally(item);
        }

        if(m_scrollType == NauScroll::ScrollType::vertical)
        {
            removeChildAsVertically(item);
        }  
    }

    void NauScroll::removeChildAsHorizontally(Node* item)
    {
        auto it = std::find(m_content.begin(), m_content.end(), item);
        if (it != m_content.end())
        {
            it = m_content.erase(it);
        }
        else
        {
            NAU_LOG_ERROR("Scroll content node not found");
            return;
        }

        const math::vec2 itemSize = item->getContentSize();
        const math::vec2 contentSize = m_contentRoot->getContentSize();

        if(m_content.empty())
        {
            m_contentRoot->setContentSize({0, 0});    
        }
        else
        {
            m_contentRoot->setContentSize({contentSize.getX() - itemSize.getX(), contentSize.getY()});    
        }

        for (auto i = it; i != m_content.end(); ++i)
        {
            if (*i)
            {
                (*i)->setPositionX((*i)->getPositionX() - itemSize.getX());
            }
        }

        m_contentRoot->removeChild(item);   
    }

    void NauScroll::removeChildAsVertically(Node* item)
    {
        auto it = std::find(m_content.begin(), m_content.end(), item);
        if (it != m_content.end())
        {
            it = m_content.erase(it);
        }
        else
        {
            NAU_LOG_ERROR("Scroll content node not found");
            return;
        }

        const math::vec2 itemSize = item->getContentSize();
        const math::vec2 contentSize = m_contentRoot->getContentSize();

        if(m_content.empty())
        {
            m_contentRoot->setContentSize({0, 0});    
        }
        else
        {
            m_contentRoot->setContentSize({contentSize.getX(), contentSize.getY() - itemSize.getY()});    
        }

        for (auto i = m_content.begin(); i != it; ++i)
        {
            if (*i)
            {
                (*i)->setPositionY((*i)->getPositionY() - itemSize.getY());
            }
        }

        m_contentRoot->removeChild(item);   
    }

    void NauScroll::setContentSize(const math::vec2& contentSize)
    {
        markDirty();

        Node* clipperNode = Node::getNestedNodeByName("clipper");
        if(!clipperNode)
        {
            NAU_LOG_ERROR("Clipper node not found by tag");
            return;
        }

        ClippingNode* clipper = dynamic_cast<ClippingNode*>(clipperNode);
        if(!clipper)
        {
            NAU_LOG_ERROR("Clipper node incorrect cast");
            return;
        }

        DrawNode* stencil = dynamic_cast<DrawNode*>(clipper->getStencil());
        if(!stencil)
        {
            NAU_LOG_ERROR("Stencil node incorrect cast");
            return;
        }

        Node::setContentSize(contentSize);
        
        clipper->setPosition(contentSize * 0.5f);
        clipper->setContentSize(cocos2d::Size(contentSize));

        stencil->clearDrawNode();
        stencil->drawSolidRect(cocos2d::Vec2(0, 0), cocos2d::Vec2(contentSize), cocos2d::Color4F::WHITE);
        stencil->setPosition(
        cocos2d::Vec2(
            clipper->getPosition() - (clipper->getContentSize() * 0.5f)));

        m_contentRoot->setPosition(contentSize * 0.5f);
    }

    void NauScroll::setContentRootSize(const math::vec2& contentSize)
    {
        markDirty();
        m_contentRoot->setContentSize(contentSize);
    }

    math::vec2 NauScroll::getContentRootSize()
    {
        return m_contentRoot->getContentSize();
    }

    math::vec2 NauScroll::getContentRootPosition()
    {
        return m_contentRoot->getPosition();
    }

    void NauScroll::addScrollBarSprite(const std::string& filePath)
    {
        markDirty();
        auto sprite = Sprite::create(filePath);
        if(sprite)
        {
            if(m_scrollBarSprite)
            {
                removeChild(m_scrollBarSprite);
            }

            m_scrollBarSprite = sprite;
    
            Node::addChild(m_scrollBarSprite);
            updateScrollBarSpritePosition();
        }
        else
        {
            NAU_LOG_ERROR("Scroll bar sprite init with file error");
        }
    }

    void NauScroll::addScrollBarSprite(Sprite* sprite)
    {
        markDirty();
        if(sprite)
        {
            if(m_scrollBarSprite)
            {
                removeChild(m_scrollBarSprite);
            }

            m_scrollBarSprite = sprite;
            
            Node::addChild(m_scrollBarSprite);
            updateScrollBarSpritePosition();
        }
        else
        {
            NAU_LOG_ERROR("Scroll bar sprite init error");
        }
    }

    void NauScroll::moveTo(Node* contentNode)
    {
        if(!contentNode)
        {
            NAU_LOG_ERROR("Content node for move to is null");
            return;
        }

        auto it = std::find(m_content.begin(), m_content.end(), contentNode);

        if (it == m_content.end()) 
        {
            NAU_LOG_ERROR("Content node for move to not founded");
            return;
        }

        if(m_scrollType == NauScroll::ScrollType::horizontal)
        {
            moveToHorizontally(contentNode);
        }

        if(m_scrollType == NauScroll::ScrollType::vertical)
        {
            moveToVertically(contentNode);
        }  

        updateScrollBarSpritePosition();
    }

    void NauScroll::moveTo(float x, float y)
    {
        if(m_scrollType == NauScroll::ScrollType::horizontal)
        {
            moveToHorizontally(x);
        }

        if(m_scrollType == NauScroll::ScrollType::vertical)
        {
            moveToVertically(y);
        }  

        updateScrollBarSpritePosition();
    }

    void NauScroll::moveToHorizontally(Node* contentNode)
    {
        math::vec2 scrollSize = getContentSize();
        math::vec2 contentRootSize = m_contentRoot->getContentSize();

        if(contentRootSize.getX() <= scrollSize.getX())
        {
            return;
        }

        math::vec2 contentNodePosition = contentNode->getPosition();
        m_contentRoot->setPositionX(scrollSize.getX() * 0.5f);
        moveScroll({-(contentRootSize.getX() * 0.5f - contentNodePosition.getX()), m_contentRoot->getPositionY()});
    }

    void NauScroll::moveToVertically(Node* contentNode)
    {
        math::vec2 scrollSize = getContentSize();
        math::vec2 contentRootSize = m_contentRoot->getContentSize();

        if(contentRootSize.getY() <= scrollSize.getY())
        {
            return;
        }

        math::vec2 contentNodePosition = contentNode->getPosition();
        m_contentRoot->setPositionY(scrollSize.getY() * 0.5f);
        moveScroll({m_contentRoot->getPositionX(), contentRootSize.getY() * 0.5f - contentNodePosition.getY()});
    }

    void NauScroll::moveToHorizontally(float x)
    {
        math::vec2 scrollSize = getContentSize();
        math::vec2 contentRootSize = m_contentRoot->getContentSize();

        if(contentRootSize.getX() <= scrollSize.getX())
        {
            return;
        }

        m_contentRoot->setPositionX(scrollSize.getX() * 0.5f);
        moveScroll({-(contentRootSize.getX() * 0.5f - x), m_contentRoot->getPositionY()});
    }

    void NauScroll::moveToVertically(float y)
    {
        math::vec2 scrollSize = getContentSize();
        math::vec2 contentRootSize = m_contentRoot->getContentSize();

        if(contentRootSize.getY() <= scrollSize.getY())
        {
            return;
        }

        m_contentRoot->setPositionY(scrollSize.getY() * 0.5f);
        moveScroll({m_contentRoot->getPositionX(), contentRootSize.getY() * 0.5f - y});
    }

    void NauScroll::updateScrollBarSpritePosition()
    {
        if(!m_scrollBarSprite)
        {
            return;
        }

        const math::vec2 scrollSize = getContentSize();
        const math::vec2 contentRootSize = m_contentRoot->getContentSize();
        const math::vec2 contentRootPosition = m_contentRoot->getPosition();
        const math::vec2 scrollCenterPosition = scrollSize * 0.5f;

        math::vec2 spritePosition;

        if(m_scrollType == NauScroll::ScrollType::horizontal)
        {
            float horizontalDeltaMax = (contentRootSize.getX() - scrollSize.getX());
            float horizontalDelta = (contentRootPosition.getX() - scrollCenterPosition.getX());
            float deltaRatio = horizontalDelta / horizontalDeltaMax;

            if(deltaRatio < 0)
            {
                spritePosition = {((scrollSize.getX() * 0.5f) + (scrollSize.getX() * deltaRatio * -1.0f)), 0.0f - m_scrollBarSprite->getContentSize().getY() * 0.5f};
            }
            else
            {
                spritePosition = {((scrollSize.getX() * 0.5f) - (scrollSize.getX() * deltaRatio)), 0.0f - m_scrollBarSprite->getContentSize().getY() * 0.5f};
            }
        }

        if(m_scrollType == NauScroll::ScrollType::vertical)
        {
            float vericalDeltaMax = (contentRootSize.getY() - scrollSize.getY());
            float vericalDelta = (contentRootPosition.getY() - scrollCenterPosition.getY());
            float deltaRatio = vericalDelta / vericalDeltaMax;

            if(deltaRatio < 0)
            {
                spritePosition = {
                    scrollSize.getX() + m_scrollBarSprite->getContentSize().getX() * 0.5f, 
                    ((scrollSize.getY() * 0.5f) + (scrollSize.getY() * deltaRatio * - 1.0f))
                };
            }
            else
            {
                spritePosition = {
                    scrollSize.getX() + m_scrollBarSprite->getContentSize().getX() * 0.5f, 
                    ((scrollSize.getY() * 0.5f) - (scrollSize.getY() * deltaRatio))
                };
            }
        }  

        m_scrollBarSprite->setPosition(spritePosition);
    }

    bool NauScroll::isInputEventInElementBorder(math::vec2 inputPosition)
    {
        return UIControl::isInputEventInElementBorder(inputPosition) || isInputEventInScrollBarButtonBorder(inputPosition);
    }

    bool NauScroll::isInputEventInScrollBarButtonBorder(math::vec2 inputPosition)
    {
        if(!m_scrollBarSprite)
        {
            return false;
        }

        cocos2d::Rect rect;
        rect.size = m_scrollBarSprite->getContentSize();
        rect.origin = m_scrollBarSprite->getPosition() - (rect.size * 0.5f);

        return rect.containsPoint(inputPosition);
    }

    void NauScroll::redrawDebug()
    {
#if UI_ELEMENT_DEBUG
        Node::redrawDebug();
        drawContentRect();
#endif
    }

#if UI_ELEMENT_DEBUG
    void NauScroll::drawContentRect()
    {
        if(!m_isDebugEnable)
        {
            return;
        }

        if(m_debugLevel == DebugDrawLevel::borders)
        {
            return;
        }

        m_contentDebugNode->clearDrawNode();
        auto contentNodeSize = m_contentRoot->getContentSize();
        auto contentNodePosition =  m_contentRoot->getPosition();
        m_contentDebugNode->drawRect(contentNodePosition - (contentNodeSize * 0.5f), contentNodePosition + (contentNodeSize * 0.5f), m_debugColor);
        
    }
#endif
}
