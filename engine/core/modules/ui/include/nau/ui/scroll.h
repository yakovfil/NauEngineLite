// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <vector>

#include "nau/ui/elements/draw_node.h"
#include "nau/ui/elements/sprite.h"
#include "nau/ui/ui_control.h"

namespace nau::ui
{
    /**
     * @brief Encapsulates  scroll GUI element which allows to scroll through a collection of its children either horizontally or verticall using a scroll bar.
     */
    class NAU_UI_EXPORT NauScroll : public UIControl
    {
    public:

        /**
         * @brief Enumerates scrolling directions.
         */
        enum class ScrollType
        {
            vertical,
            horizontal
        };

        static const int CLIPPER_TAG;
        
        /**
         * @brief Default constructor.
         */
        NauScroll();

        /**
         * @brief Default destructor.
         */
        virtual ~NauScroll();

		/**
         * @brief Creates a scroll element.
         * 
         * @param [in] scrollType   Scrolling direction.
         * @return                  A pointer to the created scroll object.
         */
        static NauScroll* create(NauScroll::ScrollType srollType);
        
        /**
         * @brief Creates a scroll element.
         * 
         * @param [in] scrollType   Scrolling direction.
         * @param [in] size         Scroll size.
         * @return                  A pointer to the created scroll object.
         */
        static NauScroll* create(NauScroll::ScrollType srollType, const math::vec2& size);
        

        /**
         * @brief Retrieves the scroll bar sprite.
         * 
         * @return A pointer to scroll bar sprite.
         */
        FORCEINLINE Sprite* getScrollBarSprite() { return m_scrollBarSprite; };

        NauScroll::ScrollType getScrollType();
        void setScrollType(NauScroll::ScrollType scrollType);

        /**
         * @brief Attaches a child GUI object to the scroll.
         *
         * @param [in] contentNode A pointer to the object to attach.
         *
         */
        virtual void addChild(Node* contentNode) override;

        /**
         * @brief Attaches a child GUI object to the scroll and performs necessary resizing and allignment according to the scrolling direction.
         * 
         * @param [in] contentNode A pointer to the object to attach.
         */
        void addChildWithAlignment(Node* contentNode);

        /**
         * @brief Detaches the GUI element from the scroll.
         *
         * @param [in] contentNode    A pointer to the object to detach.
         */
        virtual void removeChild(Node* contentNode) override;

        /**
         * @brief Detaches the GUI element from the scroll and performs necessary resizing and allignment according to the scrolling direction.
         */
        void removeChildWithAlignment(Node* contentNode);

        /**
         * @brief Resizes the content area of the scroll.
         * 
         * @param [in] contentSize Size to set the scroll content to.
         * 
         * Calling this method also resizes the clipping area of the scroll and aligns its content by the scroll center.
         * Should you wish to provide different resize/alignment logic, address to setContentRootSize method.
         */
        virtual void setContentSize(const math::vec2& contentSize) override;

        /**
         * @brief Resizes the content area of the scroll.
         * 
         * @param [in] contentSize Size to set the scroll content to.
         * 
         * @note Users should apply some external logic for clipping and aligning the content when using this method to resize the scroll.
         */
        void setContentRootSize(const math::vec2& contentSize);
        math::vec2 getContentRootSize();
        math::vec2 getContentRootPosition();


        /**
         * @brief Changes the sprite of the scroll bar.
         * 
         * @param [in] Path to the image file to construct sprite from.
         */
        void addScrollBarSprite(const std::string& filePath);

        /**
         * @brief Changes the sprite of the scroll bar.
         * 
         * @param [in] sprite A pointer to the sprite to use.
         */
        void addScrollBarSprite(Sprite* sprite);

        /**
         * @brief Scrolls the bar to the specified GUI element.
         * 
         * @param [in] contentNode A pointer to the element to scroll to. It has to be a child of the scroll.
         */
        void moveTo(Node* contentNode);

        /**
         * @brief Scrolls the bar to the specified position.
         * 
         * @param [in] x,y Position to scroll the bar to.
         */
        void moveTo(float x, float y);

        /**
         * @brief Scrolls the bar by the specified offset.
         * 
         * @param [in] delta Offset to scroll the bar by.
         */
        void moveScroll(const math::vec2& delta);

        /**
         * @brief Draws outline around scroll content.
         */
        virtual void redrawDebug() override;

    protected:
        virtual bool initialize() override;
        virtual bool isInputEventInElementBorder(math::vec2 inputPosition) override;

    private:
        Node* m_contentRoot {nullptr};
        std::vector<Node*> m_content{};
        NauScroll::ScrollType m_scrollType {NauScroll::ScrollType::vertical};
        Sprite* m_scrollBarSprite {nullptr};
        bool m_scrollTumbCaptured {false};

        void onScrollMovedByWheel(const math::vec2& delta);
        void onScrollMovedByDrag(const math::vec2& delta);
        void onScrollMovedByScrollBar(const math::vec2& delta);
        void addChildAsHorizontally(Node* contentNode);
        void addChildAsVertically(Node* contentNode);
        void removeChildAsHorizontally(Node* contentNode);
        void removeChildAsVertically(Node* contentNode);
        void reorderChildAsVertically();
        void reorderChildAsHorizontally();
        void moveToHorizontally(Node* contentNode);
        void moveToVertically(Node* contentNode);

        void moveToHorizontally(float x);
        void moveToVertically(float y);

        void updateScrollBarSpritePosition();
        bool isInputEventInScrollBarButtonBorder(math::vec2 inputPosition);

#if UI_ELEMENT_DEBUG
    DrawNode* m_contentDebugNode {nullptr};
    void drawContentRect();
#endif        
    };
}

