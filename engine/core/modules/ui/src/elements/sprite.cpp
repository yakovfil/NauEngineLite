// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/ui/elements/sprite.h"

#include <string>

#include "../nau_controls/Button/States/sprite_frame_handler.h"
#include "../nau_controls/Label/texture_2d_handler.h"
#include "2d/CCSprite.h"

namespace nau::ui
{

Sprite* Sprite::create()
{
    return Node::create<Sprite>();
}

Sprite* Sprite::create(const std::string& filename)
{
    Sprite *sprite = new (std::nothrow) Sprite();
    if (sprite && sprite->initWithFile(filename.c_str()))
    {
        sprite->autorelease();
        return sprite;
    }
    CC_SAFE_DELETE(sprite);
    return nullptr;
}

Sprite* Sprite::create(const std::string& filename, const cocos2d::Rect& rect)
{
    Sprite *sprite = new (std::nothrow) Sprite();
    if (sprite && sprite->initWithFile(filename.c_str(), rect))
    {
        sprite->autorelease();
        return sprite;
    }
    CC_SAFE_DELETE(sprite);
    return nullptr;
}

bool Sprite::initWithSpriteFrameContainer(const SpriteFrameHandler& container)
{
    return initWithSpriteFrame(container.m_spriteFrame);
}

bool Sprite::initWithTexture2dContainer(const Texture2DHandler& container)
{
    return initWithTexture(container.m_texture, container.m_rect); 
}

bool Sprite::initWithFile(const std::string& filename)
{
    return cocos2d::Sprite::initWithFile(filename.c_str());
}

bool Sprite::initWithFile(const std::string& filename, const cocos2d::Rect& rect)
{
    return cocos2d::Sprite::initWithFile(filename.c_str(), rect);
}

void Sprite::addChild(Node* node)
{
    markDirty();
    cocos2d::Sprite::addChild(node);
}

void Sprite::reorderChild(Node* child, int zOrder)
{
    markDirty();
    cocos2d::Sprite::reorderChild(child, zOrder);
}

void Sprite::removeChild(Node *child, bool cleanup)
{
    markDirty();
    cocos2d::Sprite::removeChild(child, cleanup);
}

void Sprite::setScaleX(float scaleX)
{
    markDirty();
    cocos2d::Sprite::setScaleX(scaleX);
}

void Sprite::setScaleY(float scaleY)
{
    markDirty();
    cocos2d::Sprite::setScaleY(scaleY);
}

void Sprite::setScale(float scale)
{
    markDirty();
    cocos2d::Sprite::setScale(scale);
}

void Sprite::setScale(float scaleX, float scaleY)
{
    markDirty();
    cocos2d::Sprite::setScale(scaleX, scaleY);
}


void Sprite::setPosition(const nau::math::vec2& pos)
{
    markDirty();
    cocos2d::Sprite::setPosition(pos);
}

void Sprite::setRotation(float rotation)
{
    markDirty();
    cocos2d::Sprite::setRotation(rotation);
}

void Sprite::setRotationSkewX(float fRotationX)
{
    markDirty();
    cocos2d::Sprite::setRotationSkewX(fRotationX);
}

void Sprite::setRotationSkewY(float fRotationY)
{
    markDirty();
    cocos2d::Sprite::setRotationSkewY(fRotationY);
}

void Sprite::setSkewX(float sx)
{
    markDirty();
    cocos2d::Sprite::setSkewX(sx);
}

void Sprite::setSkewY(float sy)
{
    markDirty();
    cocos2d::Sprite::setSkewY(sy);
}

void Sprite::setPositionZ(float fVertexZ)
{
    markDirty();
    cocos2d::Sprite::setPositionZ(fVertexZ);
}

void Sprite::setAnchorPoint(const math::vec2& anchorPoint)
{
    markDirty();
    cocos2d::Sprite::setAnchorPoint(anchorPoint);
}

void Sprite::setContentSize(const math::vec2& size)
{
    markDirty();
    cocos2d::Sprite::setContentSize(cocos2d::Size(size));
}

void Sprite::setVisible(bool bVisible)
{
    markDirty();
    cocos2d::Sprite::setVisible(bVisible);
}

void Sprite::setOpacityModifyRGB(bool modify)
{
    markDirty();
    cocos2d::Sprite::setOpacityModifyRGB(modify);
}

} // namespace nau::ui
