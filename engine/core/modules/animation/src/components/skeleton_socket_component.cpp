// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/animation/components/skeleton_socket_component.h"

#include <string>
#include <vector>

#include "nau/animation/components/skeleton_component.h"
#include "nau/math/math.h"
#include "nau/scene/scene_object.h"

namespace nau
{
    void SkeletonSocketComponent::updateComponent(float dt)
    {
        nau::scene::SceneObject& socketObject = getParentObject();
        nau::scene::SceneObject* skeletonObject = socketObject.getParentObject();
        SkeletonComponent* skeletonComponent = skeletonObject->findFirstComponent<SkeletonComponent>();

        if (!skeletonComponent)
        {
            NAU_ASSERT(false);
            return;
        }

        const std::vector<SkeletonJoint>& joints = skeletonComponent->getJoints();

        for (size_t jointIndex = 0; jointIndex < joints.size(); ++jointIndex)
        {
            if (joints[jointIndex].jointName == m_boneName)
            {
                const auto& modelSpaceJointMatrices = skeletonComponent->getModelSpaceJointMatrices();

                math::Matrix4 socketTransform;
                std::memcpy(&socketTransform, &modelSpaceJointMatrices.at(jointIndex), 64);

                setTransform(nau::math::Transform(socketTransform) * m_relativeToBoneOffset);
                break;
            }
        }
    }

    void SkeletonSocketComponent::setBoneName(const std::string& boneName)
    {
        m_boneName = boneName;
    }

    const std::string& SkeletonSocketComponent::getBoneName() const
    {
        return m_boneName;
    }

    void SkeletonSocketComponent::setRelativeToBoneOffset(const math::Transform& transform)
    {
        m_relativeToBoneOffset = transform;
    }
    
    const math::Transform& SkeletonSocketComponent::getRelativeToBoneOffset() const
    {
        return m_relativeToBoneOffset;
    }
}  // namespace nau
