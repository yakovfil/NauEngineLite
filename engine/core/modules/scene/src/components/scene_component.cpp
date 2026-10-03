// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/scene/components/scene_component.h"

namespace nau::scene
{
    NAU_IMPLEMENT_DYNAMIC_OBJECT(SceneComponent)

    SceneComponent::~SceneComponent()
    {
        // Transform membership must end while both list and nodes are alive,
        // including inactive trees destroyed without scene-manager deactivation.
        if (m_transformParent)
            m_transformParent->removeTransformChild(*this);
        while (!m_transformChildren.empty())
            removeTransformChild(static_cast<SceneComponent&>(m_transformChildren.front()));
    }

    const math::Transform& SceneComponent::getWorldTransform() const
    {
        if (!m_worldTransformCache)
        {
            if (m_transformParent)
            {
                m_worldTransformCache = static_cast<SceneComponent&>(*m_transformParent).getWorldTransform() * m_transform;
            }
            else
            {
                m_worldTransformCache = m_transform;
            }
        }

        return *m_worldTransformCache;
    }

    void SceneComponent::setWorldTransform(const math::Transform& worldTransform)
    {
        if (m_transformParent)
        {
            const auto& parentWorldTransform = m_transformParent->getWorldTransform();
            m_transform = parentWorldTransform.getRelativeTransform(worldTransform);
        }
        else
        {
            m_transform = worldTransform;
        }

        notifyTransformChanged(worldTransform);
    }

    const math::Transform& SceneComponent::getTransform() const
    {
        return m_transform;
    }

    void SceneComponent::setTransform(const math::Transform& transform)
    {
        m_transform = transform;
        notifyTransformChanged();
    }

    void SceneComponent::setRotation(math::quat rotation)
    {
        m_transform.setRotation(rotation);
        notifyTransformChanged();
    }

    void SceneComponent::setTranslation(math::vec3 position)
    {
        m_transform.setTranslation(position);
        notifyTransformChanged();
    }

    void SceneComponent::setScale(math::vec3 scale)
    {
        m_transform.setScale(scale);
        notifyTransformChanged();
    }

    math::quat SceneComponent::getRotation() const
    {
        return m_transform.getRotation();
    }

    math::vec3 SceneComponent::getTranslation() const
    {
        return m_transform.getTranslation();
    }

    math::vec3 SceneComponent::getScale() const
    {
        return m_transform.getScale();
    }

    void SceneComponent::appendTransformChild(SceneComponent& child)
    {
        NAU_ASSERT(!m_transformChildren.contains(child));
        NAU_FATAL(&child != this);

        m_transformChildren.push_back(child);
        child.m_transformParent = this;
    }

    void SceneComponent::removeTransformChild(SceneComponent& child)
    {
        NAU_ASSERT(m_transformChildren.contains(child));
        NAU_FATAL(&child != this);
        NAU_FATAL(child.m_transformParent == this);

        m_transformChildren.remove(child);
        child.m_transformParent = nullptr;
    }

    void SceneComponent::notifyTransformChanged()
    {
        m_worldTransformCache.reset();
        notifyChanged();

        for (auto& transformChild : m_transformChildren)
        {
            static_cast<SceneComponent&>(transformChild).notifyTransformChanged();
        }
    }

    void SceneComponent::notifyTransformChanged(const math::Transform& worldTransformCache)
    {
        m_worldTransformCache.emplace(worldTransformCache);
        notifyChanged();

        for (auto& transformChild : m_transformChildren)
        {
            static_cast<SceneComponent&>(transformChild).notifyTransformChanged();
        }
    }

}  // namespace nau::scene
