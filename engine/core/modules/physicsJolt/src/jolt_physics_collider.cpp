// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/physics/jolt/jolt_physics_collider.h"

#include <algorithm>

#include "nau/physics//jolt/jolt_physics_shapes.h"
#include "nau/physics/jolt/jolt_physics_material.h"
#include "nau/physics/jolt/jolt_physics_math.h"

namespace nau::physics::jolt
{
    namespace
    {
        inline TFloat getBoxConvexRadius(math::vec3 extent)
        {
            return std::min(std::min(JPH::cDefaultConvexRadius, extent.getX() * 0.5f), std::min(extent.getY() * 0.5f, extent.getZ() * 0.5f));
        }

        inline TFloat getCylinderConvexRadius(TFloat halfHeight, TFloat radius)
        {
            return std::min(std::min(JPH::cDefaultConvexRadius, halfHeight), radius);
        }

    }  // namespace

    JoltCollisionShape::JoltCollisionShape() = default;

    JoltCollisionShape::JoltCollisionShape(JPH::RefConst<JPH::Shape> shape) :
        m_collisionShape(std::move(shape))
    {
    }

    JPH::RefConst<JPH::Shape> JoltCollisionShape::getCollisionShape() const
    {
        auto shape = m_collisionShape;
#if 0
        if (auto scale = m_transform.getScale(); !scale.similar(math::vec3::one()))
        {
            const JPH::Vec3Arg vec3Scale{scale.getX(), scale.getY(), scale.getZ()};
            JPH::ShapeSettings::ShapeResult scaledShape = shape->ScaleShape(vec3Scale);
            NAU_ASSERT(scaledShape.IsValid(), "Fail to scale shape:({})", scaledShape.GetError());
            if (scaledShape.IsValid())
            {
                shape = scaledShape.Get();
            }
            else
            {
                NAU_LOG_ERROR(scaledShape.GetError());
            }
        }
#endif
        return shape;
    }

    void JoltCollisionShape::setCollisionShape(JPH::RefConst<JPH::Shape> shape)
    {
        m_collisionShape = std::move(shape);
    }

    JoltSphereCollision::JoltSphereCollision(TFloat radius, JPH::PhysicsMaterial* material) :
        Base(new JoltSphereShape(radius, material))
    {
    }

    TFloat JoltSphereCollision::getRadius() const
    {
        NAU_FAILURE("Not implemented");
        return 0.f;
    }

    void JoltSphereCollision::setRadius(TFloat)
    {
        NAU_FAILURE("Not implemented");
    }

    JoltBoxCollision::JoltBoxCollision(math::vec3 extent, JPH::PhysicsMaterial* material) :
        Base(new JPH::BoxShape(vec3ToJolt(extent), getBoxConvexRadius(extent), material))
    {
    }

    JoltCapsuleCollision::JoltCapsuleCollision(const ConstructionData& data, JPH::PhysicsMaterial* material) :
        Base(new JPH::CapsuleShape(.5f * data.height, data.radius, material))
    {
    }

    JoltCylinderCollision::JoltCylinderCollision(const ConstructionData& data, JPH::PhysicsMaterial* material)
    {
        const TFloat halfHeight = .5f * data.height;
        const TFloat convexRadius = getCylinderConvexRadius(halfHeight, data.radius);

        setCollisionShape(new JPH::CylinderShape(halfHeight, data.radius, convexRadius, material));
    }

    JoltConvexHullCollision::JoltConvexHullCollision(const ConstructionData& constructionData, JPH::PhysicsMaterial* material)
    {
        JPH::Array<JPH::Vec3> list;
        for (auto const& point : constructionData.points)
        {
            list.push_back({point.getX(), point.getY(), point.getZ()});
        }

        JPH::ConvexHullShapeSettings convexSettings{list, JPH::cDefaultConvexRadius, material};
        if (auto result = convexSettings.Create(); !result.HasError())
        {
            setCollisionShape(result.Get());
        }
    }

    JoltConvexHullCollision::JoltConvexHullCollision(nau::Ptr<JoltConvexHullAssetView> convexHullAssetView) :
        m_convexHullAsset(convexHullAssetView)
    {
        NAU_ASSERT(m_convexHullAsset);
        if (m_convexHullAsset)
        {
            JPH::ShapeSettings::ShapeResult shapeResult = m_convexHullAsset->getShapeSettings().Create();
            if (!shapeResult.HasError())
            {
                setCollisionShape(shapeResult.Get());
            }
            else
            {
                NAU_LOG_ERROR(shapeResult.GetError());
            }
        }
    }

    JoltMeshCollision::JoltMeshCollision(const ConstructionData& constructionData)
    {
        JPH::TriangleList list;
        for (const auto& triangle : constructionData.triangles)
        {
            list.push_back(JPH::Triangle{
                JPH::Float3{triangle.p1.getX(), triangle.p1.getY(), triangle.p1.getZ()},
                JPH::Float3{triangle.p2.getX(), triangle.p2.getY(), triangle.p2.getZ()},
                JPH::Float3{triangle.p3.getX(), triangle.p3.getY(), triangle.p3.getZ()},
                triangle.materialIndex
            });
        }

        JPH::PhysicsMaterialList materialList;
        for (const auto& engineMaterial : constructionData.materials)
        {
            materialList.push_back(engineMaterial->as<const NauJoltPhysicsMaterialImpl*>()->joltMaterial());
        }

        const JPH::MeshShapeSettings meshSettings{list, materialList};
        auto result = meshSettings.Create();

        if (!result.HasError())
        {
            setCollisionShape(result.Get());
        }
    }

    JoltMeshCollision::JoltMeshCollision(nau::Ptr<JoltTriMeshAssetView> meshAssetView) :
        m_meshAsset(meshAssetView)
    {
        NAU_ASSERT(m_meshAsset);
        if (m_meshAsset)
        {
            JPH::ShapeSettings::ShapeResult shapeResult = m_meshAsset->getShapeSettings().Create();
            if (!shapeResult.HasError())
            {
                setCollisionShape(shapeResult.Get());
            }
            else
            {
                NAU_LOG_ERROR(shapeResult.GetError());
            }
        }
    }
}  // namespace nau::physics::jolt
