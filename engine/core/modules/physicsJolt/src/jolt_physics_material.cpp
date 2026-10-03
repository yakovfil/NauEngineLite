// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/physics/jolt/jolt_physics_material.h"

#include <optional>
#include <string_view>
#include <utility>

namespace nau::physics::jolt
{
    const char* JoltPhysicsMaterial::GetDebugName() const
    {
        return m_engineMaterial->getName().data();
    }

    const nau::physics::IPhysicsMaterial::Ptr& JoltPhysicsMaterial::engineMaterial() const
    {
        return m_engineMaterial;
    }

    void JoltPhysicsMaterial::setEngineMaterial(nau::physics::IPhysicsMaterial::Ptr engineMaterial)
    {
        m_engineMaterial = std::move(engineMaterial);
    }

    NauJoltPhysicsMaterialImpl::NauJoltPhysicsMaterialImpl(std::string_view name,
                                                           std::optional<TFloat> friction,
                                                           std::optional<TFloat> restitution) :
        m_name(name),
        m_friction(friction),
        m_restitution(restitution)
    {
        m_joltMaterial = new JoltPhysicsMaterial();
        m_joltMaterial->setEngineMaterial(this);
    }

    std::string_view NauJoltPhysicsMaterialImpl::getName() const
    {
        return m_name;
    }

    std::optional<TFloat> NauJoltPhysicsMaterialImpl::getFriction() const
    {
        return m_friction;
    }

    std::optional<TFloat> NauJoltPhysicsMaterialImpl::getRestitution() const
    {
        return m_restitution;
    }

    JoltPhysicsMaterial* NauJoltPhysicsMaterialImpl::joltMaterial() const
    {
        return m_joltMaterial;
    }
}