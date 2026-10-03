// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/ui/elements/node.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "2d/CCNode.h"
#include "cocos2d.h"
#include "nau/animation/components/animation_component.h"
#include "nau/animation/controller/animation_controller_direct.h"
#include "nau/effects/node_animation.h"
#include "nau/math/math.h"
#include "nau/scene/scene.h"
#include "nau/scene/scene_factory.h"
#include "nau/service/service_provider.h"
#include "nau/ui.h"
#include "nau/ui/elements/draw_node.h"

namespace nau::ui
{
Node::Node()
{
    setUid(nau::Uid::generate());
}

Node::~Node() 
{
    removeFromEngineScene();
}

nau::Uid Node::getUid() const
{
    return m_uid;
}

bool Node::initialize()
{
    if(cocos2d::Node::init())
    {
        if (auto* scene = getServiceProvider().get<UiManager>().getEngineScene())
        {
            addToEngineScene(scene);
            markDirty();
        }
        else
        {
            NAU_ASSERT(false);
        }

        return true;
    }

    return false;
}

void Node::setUid(nau::Uid uid)
{
    m_uid = uid;
}

Node* Node::create()
{
    return create<Node>();
}

void Node::setZOrder(uint32_t order)
{
    markDirty();
    setLocalZOrder(order);
}

uint32_t Node::geZOrder() const
{
    return getLocalZOrder();
}


void Node::setScaleX(float scaleX)
{
    markDirty();
    cocos2d::Node::setScaleX(scaleX);
}


void Node::setScaleY(float scaleY)
{
    markDirty();
    cocos2d::Node::setScaleY(scaleY);
}

void Node::setScale(float scale) 
{
    markDirty();
    cocos2d::Node::setScale(scale);
}

void Node::setScale(float scaleX,float scaleY)
{
    markDirty();
    cocos2d::Node::setScale(scaleX, scaleY);
}

void Node::setPosition(const math::vec2 &position)
{
    markDirty();
    cocos2d::Node::setPosition(position);
}

math::vec2 Node::getPosition() const
{
  return cocos2d::Node::_getPosition();
}

void Node::setPositionX(float x)
{
    markDirty();
    cocos2d::Node::setPositionX(x);
}

void Node::setPositionY(float y)
{
    markDirty();
    cocos2d::Node::setPositionY(y);
}

void Node::setSkewX(float skewX)
{
    markDirty();
    cocos2d::Node::setSkewX(skewX);
}

void Node::setSkewY(float skewY)
{
    markDirty();
    cocos2d::Node::setSkewY(skewY);
}

void Node::setAnchorPoint(const math::vec2& anchorPoint)
{
    markDirty();
    cocos2d::Node::setAnchorPoint(anchorPoint);
}

math::vec2 Node::getAnchorPoint() const
{
    return cocos2d::Node::_getAnchorPoint();
}

void Node::setContentSize(const math::vec2& contentSize)
{
    markDirty();
    cocos2d::Node::setContentSize(cocos2d::Size(contentSize));
}

math::vec2 Node::getContentSize() const
{
    return _getContentSize();
}

void Node::setVisible(bool visible)
{
    markDirty();
    cocos2d::Node::setVisible(visible);
}

void Node::setRotation(float rotation)
{
    markDirty();
    cocos2d::Node::setRotation(rotation);
}

void Node::setRotationSkewX(float rotationX)
{
    markDirty();
    cocos2d::Node::setRotationSkewX(rotationX);
}

void Node::setRotationSkewY(float rotationY)
{
    markDirty();
    cocos2d::Node::setRotationSkewY(rotationY);
}

void Node::addChild(Node* child)
{
    markDirty();
    m_childNodesByUid[child->getUid()] = child;
    cocos2d::Node::addChild(child);
}

void Node::addChild(Node* child, const std::string& name)
{
    markDirty();
    std::hash<std::string> hashFunc;
    size_t hashKey = hashFunc(name);

    NAU_ASSERT(m_childNodesByName.find(hashKey) == m_childNodesByName.end() && "Node with this name already exists!");

    m_childNodesByName[hashKey] = child;
    m_childNodesByUid[child->getUid()] = child;
    cocos2d::Node::addChild(child);
}

Node* Node::getNestedNodeByName(const std::string& name)
{
    if (this->getName() == name) 
    {
        return this;
    }

    std::vector<Node*> children;
    getChildren(children);

    for (Node* child : children) 
    {
        Node* foundNode = child->getNestedNodeByName(name);
        if (foundNode != nullptr) 
        {
            return foundNode;
        }
    }

    return nullptr;
}

Node* Node::getNestedNodeByUid(nau::Uid uid)
{
    auto it = m_childNodesByUid.find(uid);
    if (it != m_childNodesByUid.end())
    {
        return it->second;
    }

    for (const auto& pair : m_childNodesByUid)
    {
        auto result = pair.second->getNestedNodeByUid(uid);
        if (result)
        {
            return result;
        }
    }

    return nullptr;
}

Node * Node::getChildByTag(int tag) const
{
    return dynamic_cast<Node*>(cocos2d::Node::getChildByTag(tag));
}

Node* Node::getChildByName(const std::string& name) const
{
    return dynamic_cast<Node*>(cocos2d::Node::_getChildByName(name));
}

void Node::getChildren(std::vector<Node*>& children) const
{
    auto internalChildren = cocos2d::Node::getChildren();
    children.reserve(internalChildren.size());
    for (auto* child : internalChildren)
    {
        auto* nauNode = as<Node>(child);
        if (nauNode)
        {
            children.push_back(nauNode);
        }
    }
}

Node* Node::getParent()
{
    return as<Node>(cocos2d::Node::_getParent());
}

const Node* Node::getParent() const
{
    return dynamic_cast<const Node*>(cocos2d::Node::_getParent());
}

void Node::removeChild(Node* child)
{
    markDirty();
    cocos2d::Node::removeChild(child);
}

void Node::removeAllChildren()
{
    markDirty();
    cocos2d::Node::removeAllChildren();
}

void Node::reorderChild(Node *child, int zOrder)
{
    markDirty();
    cocos2d::Node::reorderChild(child, zOrder);
}

void Node::setTag(int tag)
{
    markDirty();
    cocos2d::Node::setTag(tag);
}

void Node::nau_setName(const std::string& name)
{
    markDirty();
    cocos2d::Node::setName(name);
}

math::vec2 Node::convertToNodeSpace(const math::vec2& worldPoint) const
{
    return cocos2d::Node::convertToNodeSpace(worldPoint);
}

math::vec2 Node::convertToWorldSpace(const math::vec2& nodePoint) const
{
    return cocos2d::Node::convertToWorldSpace(nodePoint);
}

math::vec2 Node::convertToNodeSpaceAR(const math::vec2& worldPoint) const
{
    return cocos2d::Node::convertToNodeSpaceAR(worldPoint);
}

math::vec2 Node::convertToWorldSpaceAR(const math::vec2& nodePoint) const
{
    return cocos2d::Node::convertToWorldSpaceAR(nodePoint);
}

void Node::setOpacity(uint8_t opacity)
{
    markDirty();
    cocos2d::Node::setOpacity(opacity);
}

void Node::setCascadeOpacityEnabled(bool cascadeOpacityEnabled)
{
    markDirty();
    cocos2d::Node::setCascadeOpacityEnabled(cascadeOpacityEnabled);
}

nau::math::E3DCOLOR Node::getColor() const
{
    return _getColor();
}

void Node::setColor(const nau::math::E3DCOLOR& color)
{
    markDirty();
    cocos2d::Node::setColor(color);
}

void Node::setCascadeColorEnabled(bool cascadeColorEnabled)
{
    markDirty();
    cocos2d::Node::setCascadeColorEnabled(cascadeColorEnabled);
}

void Node::setOpacityModifyRGB(bool value)
{
    markDirty();
    cocos2d::Node::setOpacityModifyRGB(value);
}

void Node::callRecursivly(std::function<void(Node*)> fn)
{
    fn(this);

    std::vector<Node*> children;
    getChildren(children);

    for (const auto& child : children)
    {
        child->callRecursivly(fn);
    }
}

void Node::enableDebugDraw(bool isEnable, DebugDrawLevel level, const math::Color4& debugColor)
{
#if UI_ELEMENT_DEBUG
    m_isDebugEnable = isEnable;
    m_debugColor = debugColor;
    m_debugLevel = level;

    if(!m_debugDrawNode)
    {
        m_debugDrawNode = DrawNode::create();
        addChild(m_debugDrawNode);
    }

    redrawDebug();
#endif
}

void Node::redrawDebug() 
{
#if UI_ELEMENT_DEBUG
    clearDebug();
    debugDrawlContetnSize();
#endif
}

void Node::addToEngineScene(scene::IScene* scene)
{
    if (scene)
    {
        NAU_ASSERT(!m_sceneObject);

        auto& sceneFactory = getServiceProvider().get<scene::ISceneFactory>();
        scene::SceneObject::Ptr newSceneObject = sceneFactory.createSceneObject<scene::SceneComponent>();
        auto& sceneObject = scene->getRoot().attachChild(std::move(newSceneObject));
        auto& animComp = sceneObject.addComponent<animation::AnimationComponent>();
        animComp.setController(rtti::createInstance<animation::DirectAnimationController>());

        m_sceneObject = sceneObject;
    }
}

void Node::removeFromEngineScene()
{
    if (auto* sceneObject = m_sceneObject.get())
    {
        sceneObject->destroy();
        m_sceneObject = nullptr;
    }
}

animation::IAnimationTarget* Node::getAnimator()
{
    if (!m_animatorCached)
    {
        m_animatorCached = createAnimator();
    }

    return m_animatorCached.get();
}

animation::AnimationComponent* Node::getAnimationComponent()
{
    if (m_sceneObject)
    {
        return m_sceneObject->findFirstComponent<animation::AnimationComponent>();
    }

    return nullptr;
}

nau::Ptr<UiNodeAnimator> Node::createAnimator()
{
    return rtti::createInstance<UiNodeAnimator>(*this);
}

#if NAU_UI_CALLBACK_ON_ELEMNT_CHANGE
void Node::markDirty()
{
    m_dirty = true;
}

void Node::markClean()
{
    m_dirty = false;
}

bool Node::isDirty() const
{
    return m_dirty;
}
#else
void Node::markDirty() {}
void Node::markClean() {}
bool Node::isDirty() const { return false; }
#endif

void* Node::getPointer(const std::type_info& t)
{
    if (t == typeid(Node))
    {
        return this;
    }
    return nullptr;
}

#if UI_ELEMENT_DEBUG
    
    void Node::debugDrawlContetnSize()
    {
        if(!m_isDebugEnable)
        {
            return;
        }

        if(m_debugDrawNode)
        {
            m_debugDrawNode->drawRect({0,0}, getContentSize(), m_debugColor);
        }
    }
    void Node::clearDebug()
    {
        if(m_debugDrawNode)
        {
            m_debugDrawNode->clearDrawNode();
        }
        
    }
#endif

} //namaespace nau::ui
