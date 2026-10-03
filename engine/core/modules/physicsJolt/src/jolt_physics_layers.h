// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

#include <set>
#include <vector>

#include "nau/physics/physics_defines.h"

namespace nau::physics::jolt
{
    class DefaultObjectVsBroadPhaseLayerFilter : public JPH::ObjectVsBroadPhaseLayerFilter
    {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer, JPH::BroadPhaseLayer) const override
        {
            // At this moment filtering on the object layers level is enough.
            return true;
        }
    };

    /**
     * Allows all channels(if none specified) or only specified ones.
     */
    class DefaultRayCastChannelFilter : public JPH::ObjectLayerFilter
    {
    public:
        DefaultRayCastChannelFilter(const std::vector<nau::physics::CollisionChannel>& interestLayers) :
            m_interestLayers(interestLayers.begin(), interestLayers.end())
        {
        }

        bool ShouldCollide(JPH::ObjectLayer layer) const override
        {
            return m_interestLayers.empty() || m_interestLayers.find(layer) != m_interestLayers.end();
        }

    private:
        std::set<nau::physics::CollisionChannel> m_interestLayers;
    };

}  // namespace nau::physics::jolt
