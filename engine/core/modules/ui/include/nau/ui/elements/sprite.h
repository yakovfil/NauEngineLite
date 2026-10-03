// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string>

#include "cocos/2d/CCSprite.h"
#include "nau/math/math.h"
#include "nau/ui/elements/draw_node.h"
#include "nau/ui/elements/node.h"

namespace nau::ui
{

struct SpriteFrameHandler;
struct Texture2DHandler;

/**
 * @brief Provides functionality for sprite managing, i.e. an image container with standard GUI element behavior.
 */
class NAU_UI_EXPORT Sprite : public ::nau::ui::Node, protected cocos2d::Sprite
{
public:

    /**
     * @brief Creates an empty sprite.
     * 
     * @return A pointer to the constructed sprite object.
     */
    static Sprite* create();

    /**
     * @brief Creates a sprite from the image file.
     * 
     * @param [in] filename Path to the image file.
     * @return              A pointer to the constructed sprite object.
     */
    static Sprite* create(const std::string& filename);

    /**
     * @brief Creates a sprite from the image file.
     * 
     * @param [in] filename Path to the image file.
     * @param [in] rect     Image area rectangle.
     * @return              A pointer to the constructed sprite object.
     */
    static Sprite* create(const std::string& filename, const cocos2d::Rect& rect);


    /**
     * @brief Initializes the sprite from a sprite frame handler.
     * 
     * @param [in] Sprite frame handler.
     */
    bool initWithSpriteFrameContainer(const SpriteFrameHandler& container);

    /**
     * @brief Initializes the sprite from a texture handler.
     * 
     * @param [in] container Texture handle.
     */
    bool initWithTexture2dContainer(const Texture2DHandler& container);

    /**
     * @brief Attaches a child GUI object to the sprite.
     * 
     * @param [in] child A pointer to the object to attach.
     * 
     * See cocos2d::Node::addChild.
     */
    virtual void addChild(Node* child) override;

    /**
     * @brief Changes z-order of a sprite child.
     * 
     * @param [in] child    A pointer to the object to reorder.
     * @param [in] zOrder   z-order value to set.
     */
    virtual void reorderChild(Node* child, int zOrder) override;

    /**
     * @brief Detaches the GUI element from the sprite.
     * 
     * @param [in] child    A pointer to the object to detach.
     * @param [in] cleanup  Indicates whether all running actions and callbacks on the detached object should be cleaned up.
     */
    virtual void removeChild(Node *child, bool cleanup);

    using cocos2d::Sprite::removeAllChildrenWithCleanup;
    using cocos2d::Sprite::sortAllChildren;

    using cocos2d::Sprite::init;


    /**
     * @brief Initializes the sprite with the image file.
     * 
     * @param [in] filename Path to the image file.
     * @return              `true` on success, `false` otherwise.
     */
    virtual bool initWithFile(const std::string& filename);

    /**
     * @brief Initializes the sprite with the image file.
     * 
     * @param [in] filename Path to the image file.
     * @param [in] rect     Image area rectangle.
     * @return              `true` on success, `false` otherwise.
     */
    virtual bool initWithFile(const std::string& filename, const cocos2d::Rect& rect);

    using cocos2d::Sprite::updateTransform;
    using cocos2d::Sprite::getDescription;


    /**
     * @brief Changes sprite scale along X-axis.
     * 
     * @param [in] scaleX Value to assign.
     */
    virtual void setScaleX(float scaleX) override;

    /**
     * @brief Changes sprite scale along Y-axis.
     *
     * @param [in] scaleY Value to assign.
     */
    virtual void setScaleY(float scaleY) override;

    /**
     * @brief Changes sprite scale uniformly.
     *
     * @param [in] scale Value to assign.
     */
    virtual void setScale(float scale) override;

    /**
     * @brief Changes sprite scale.
     * 
     * @param [in] scaleX, scaleY New sprite scales along the axes.
     */
    virtual void setScale(float scaleX, float scaleY) override;


    /**
     * @brief Changes sprite position.
     * 
     * @param [in] pos Value to assign.
     */
    virtual void setPosition(const nau::math::vec2& pos) override;

    /**
     * @brief Changes sprite rotation.
     *
     * @param [in] rotation Value to assign.
     */
    virtual void setRotation(float rotation) override;

    /**
     * @brief Changes sprite skew along X-axis with rotation.
     * 
     * @[aram [in] fRotationX Value to set.
     */
    virtual void setRotationSkewX(float fRotationX) override;

    /**
     * @brief Changes sprite skew along Y-axis with rotation.
     *
     * @[aram [in] fRotationY Value to set.
     */
    virtual void setRotationSkewY(float fRotationY) override;

    /**
     * @brief Changes sprite skew along X-axis.
     *
     * @[aram [in] fRotationX Value to set.
     */
    virtual void setSkewX(float sx) override;

    /**
     * @brief Changes sprite skew along Y-axis.
     *
     * @[aram [in] fRotationY Value to set.
     */
    virtual void setSkewY(float sy) override;

    /**
     * @brief Changes z-value (depth) of the sprite.
     * 
     * @param [in] fVertexZ Value to set.
     */
    virtual void setPositionZ(float fVertexZ) override;


    /**
     * @brief Changes sprite anchor point position. 
     * 
     * @param [in] anchorPoint New position of the anchor point.
     * 
     * An anchor point of a GUI element is a point all element transfomations happen about.
     */
    virtual void setAnchorPoint(const math::vec2& anchorPoint) override;

    /**
     * @brief Changes the 'base' (unscaled) size of the sprite.
     * 
     * @param [in] size Value to assign.
     */
    virtual void setContentSize(const math::vec2& size) override;

    using cocos2d::Sprite::setIgnoreAnchorPointForPosition;


    /**
     * @brief Changes sprite visibility.
     * 
     * @param [in] bVisible Indicates whether the sprite should be visible or not.
     */
    virtual void setVisible(bool bVisible) override;

    using cocos2d::Sprite::draw;

    /**
     * @brief Change whether the opacity should impact sprite color.
     * 
     * @param [in] modify Indicates whether the opacity should impact sprite color.
     */
    virtual void setOpacityModifyRGB(bool modify) override;

    using cocos2d::Sprite::isOpacityModifyRGB;
};

} // namespace nau::ui
