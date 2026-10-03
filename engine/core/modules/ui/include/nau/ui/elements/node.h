// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "cocos/2d/CCNode.h"
#include "nau/math/dag_e3dColor.h"
#include "nau/math/math.h"
#include "nau/rtti/ptr.h"
#include "nau/scene/nau_object.h"
#include "nau/scene/scene_object.h"
#include "nau/utils/uid.h"

#ifndef UI_ELEMENT_DEBUG
#define UI_ELEMENT_DEBUG 0
#endif


namespace nau
{
    namespace animation
    {
        class IAnimationTarget;
        class AnimationComponent;
    }
    namespace scene
    {
        struct IScene;
    }
}

namespace nau::ui
{

enum class DebugDrawLevel
{
    borders,
    includingNestedElements
};

class UiNodeAnimator;
class DrawNode;


class NAU_UI_EXPORT Node : virtual protected cocos2d::Node
{
protected:
    template<std::derived_from<Node> TNode>
    static TNode* create()
    {
        if (TNode* newNode = new (std::nothrow) TNode())
        {
            if (static_cast<Node*>(newNode)->initialize())
            {
                newNode->autorelease();
                return newNode;
            }

            CC_SAFE_DELETE(newNode);
        }

        return nullptr;
    }

public:
    Node();
    virtual ~Node();

    nau::Uid getUid() const;
    
    static Node* create();
    template<class T>
    static T* cast(Node* node)
    {
        return as<T>(node);
    };

    virtual bool initialize();


    using cocos2d::Node::retain;
    using cocos2d::Node::release;

    animation::IAnimationTarget* getAnimator();
    animation::AnimationComponent* getAnimationComponent();

    using cocos2d::Node::getDescription;
    virtual void setZOrder(uint32_t order);
    virtual uint32_t geZOrder() const;
    virtual void setScaleX(float scaleX) override;
    using cocos2d::Node::getScaleX;
    virtual void setScaleY(float scaleY) override;
    using cocos2d::Node::getScaleY;
    virtual void setScale(float scale) override;
    virtual void setScale(float scaleX,float scaleY) override;
    using cocos2d::Node::getScale;
    virtual void setPosition(const math::vec2 &position);
    using cocos2d::Node::getPosition;
    virtual math::vec2 getPosition() const;
    virtual void setPositionX(float x) override;
    using cocos2d::Node::getPositionX;
    virtual void setPositionY(float y) override;
    using cocos2d::Node::getPositionY;
    virtual void setSkewX(float skewX) override;
    using cocos2d::Node::getSkewX;
    virtual void setSkewY(float skewY) override;
    using cocos2d::Node::getSkewY;
    virtual void setAnchorPoint(const math::vec2& anchorPoint);
    virtual math::vec2 getAnchorPoint() const;
    virtual void setContentSize(const math::vec2& contentSize);
    virtual math::vec2 getContentSize() const;
    virtual void setVisible(bool visible) override;
    using cocos2d::Node::isVisible;
    virtual void setRotation(float rotation) override;
    using cocos2d::Node::getRotation;
    virtual void setRotationSkewX(float rotationX) override;
    using cocos2d::Node::getRotationSkewX;
    virtual void setRotationSkewY(float rotationY) override;
    using cocos2d::Node::getRotationSkewY;
    virtual void addChild(Node* child);
    virtual void addChild(Node* child, const std::string& name);
    Node* getNestedNodeByName(const std::string& name);
    Node* getNestedNodeByUid(nau::Uid uid);
    virtual Node * getChildByTag(int tag) const override;
    using cocos2d::Node::getChildByName;
    virtual Node* getChildByName(const std::string& name) const;
    using cocos2d::Node::getChildren;
    virtual void getChildren(std::vector<Node*>& children) const;
    using cocos2d::Node::getChildrenCount;
    using cocos2d::Node::setParent;
    virtual Node* getParent();
    virtual const Node* getParent() const;
    using cocos2d::Node::removeFromParent;
    virtual void removeChild(Node* child);
    using cocos2d::Node::removeChildByTag;
    using cocos2d::Node::removeChildByName;
    using cocos2d::Node::removeAllChildren;
    virtual void removeAllChildren() override;
    virtual void reorderChild(Node *child, int zOrder);
    using cocos2d::Node::getTag;
    virtual void setTag(int tag) override;
    using cocos2d::Node::getName;
    void nau_setName(const std::string& name);
    using cocos2d::Node::onEnter;
    using cocos2d::Node::onExit;
    using cocos2d::Node::update;
    math::vec2 convertToNodeSpace(const math::vec2& worldPoint) const;
    math::vec2 convertToWorldSpace(const math::vec2& nodePoint) const;
    math::vec2 convertToNodeSpaceAR(const math::vec2& worldPoint) const;
    math::vec2 convertToWorldSpaceAR(const math::vec2& nodePoint) const;
    virtual void setOpacity(uint8_t opacity) override;
    using cocos2d::Node::getOpacity;
    using cocos2d::Node::isCascadeOpacityEnabled;
    virtual void setCascadeOpacityEnabled(bool cascadeOpacityEnabled) override;
    virtual nau::math::E3DCOLOR getColor() const;
    virtual void setColor(const nau::math::E3DCOLOR& color);
    using cocos2d::Node::isCascadeColorEnabled;
    virtual void setCascadeColorEnabled(bool cascadeColorEnabled) override;
    virtual void setOpacityModifyRGB(bool value) override;
    using cocos2d::Node::isOpacityModifyRGB;

    void callRecursivly(std::function<void(Node*)> fn);

    void enableDebugDraw(
        bool isEnable, 
        DebugDrawLevel level = DebugDrawLevel::borders, 
        const math::Color4& debugColor = {1.0f, 1.0f, 1.0f, 1.0f});

    virtual void redrawDebug();

protected:
    void markDirty();
    void markClean();
    bool isDirty() const;

#if UI_ELEMENT_DEBUG
    DrawNode* m_debugDrawNode{ nullptr };
    void debugDrawlContetnSize();
    void clearDebug();
#endif

private:
    void setUid(nau::Uid uid);
    virtual void* getPointer(const std::type_info& t) override;
    virtual nau::Ptr<UiNodeAnimator> createAnimator();
    void addToEngineScene(scene::IScene* scene);
    void removeFromEngineScene();

protected:
    math::Color4 m_debugColor;
    bool m_isDebugEnable = false;
    DebugDrawLevel m_debugLevel = DebugDrawLevel::borders;

#if NAU_UI_CALLBACK_ON_ELEMNT_CHANGE
    // dirty flag used to mark element after it was chaned
    // for each cnaged element ui manager will call notifyElementChangedCallback
    // at the end of the frame
    bool m_dirty = false;
#endif

private:
    nau::Uid m_uid;
    std::unordered_map<uint64_t, nau::ui::Node*> m_childNodesByName;
    std::unordered_map<nau::Uid, nau::ui::Node*> m_childNodesByUid;

    Ptr<UiNodeAnimator> m_animatorCached;
    scene::ObjectWeakRef<scene::SceneObject> m_sceneObject;

    friend class UiManagerImpl;
    friend class TestCanvasFitToSize;
    friend class TestCanvasFitVertically;
    friend class TestCanvasFitHorizontally;
    friend class TestCanvasStretch;
    friend class TestCanvasNoRescale;
};

} //namespace nau::ui
