// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "static_mesh_instance_group.h"

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "graphics_assets/static_mesh_asset.h"
#include "nau/string/hash.h"

namespace nau
{

    StaticMeshInstanceGroup::StaticMeshInstanceGroup(nau::ReloadableAssetView::Ptr mesh) :
        m_staticMesh(mesh)
    {
    }

    InstanceInfo StaticMeshInstanceGroup::addInstance(const nau::math::Matrix4& matrix)
    {
        auto inst = createInfo(matrix);
        addInstance(inst);

        return inst;
    }

    void StaticMeshInstanceGroup::addInstance(const InstanceInfo& inst)
    {
        m_instances[inst.id] = inst;
    }

    InstanceID StaticMeshInstanceGroup::reserveID()
    {
        InstanceID newID = freeInstanceId++;
        return newID;
    }

    InstanceInfo StaticMeshInstanceGroup::createInfo(const nau::math::Matrix4& matrix)
    {
        InstanceID newID = freeInstanceId++;
        auto info = InstanceInfo();
        info.id = newID;
        info.isVisible = true;
        info.worldMatrix = matrix;
        Ptr<StaticMeshAssetView> meshView;
        m_staticMesh->getTyped<StaticMeshAssetView>(meshView);
        info.localSphere = meshView->getMesh()->getLod0BSphere();
        info.worldSphere = meshView->getMesh()->getLod0BSphere();
        info.worldSphere.c = info.worldMatrix.getTranslation();

        return info;
    }

    RenderEntity nau::StaticMeshInstanceGroup::createRenderEntity()
    {
        return RenderEntity();
    }

    RenderList::Ptr nau::StaticMeshInstanceGroup::createRenderList(const nau::math::Vector3& viewerPosition,
                                                                   std::function<bool(const InstanceInfo&)>& filterFunc,
                                                                   std::function<bool(const nau::MaterialAssetView::Ptr)>& materialFilter)
    {
        NAU_FATAL(m_staticMesh);

        RenderList::Ptr ret = std::make_shared<RenderList>();

        Ptr<StaticMeshAssetView> meshView;
        m_staticMesh->getTyped<StaticMeshAssetView>(meshView);
        std::vector<std::vector<std::map<size_t /*material name*/, uint32_t /*entityIndex*/>>> lodSlotMats(meshView->getMesh()->getLodsCount());

        for (const auto& [id, info] : m_instances)
        {
            if (!info.isVisible)
            {
                continue;
            }

            if (!filterFunc(info))
            {
                continue;
            }

            // Calculate lod level based on screen size
            // TODO: calculate lodLevel based on distance when we get lods
            // float distance = length(info.worldMatrix.getTranslation() - viewerPosition); // distance for now, not screen size
            uint32_t lodLevel = 0;

            Ptr<StaticMeshAssetView> meshView;
            m_staticMesh->getTyped<StaticMeshAssetView>(meshView);

            const nau::StaticMeshLod& lod = meshView->getMesh()->getLod(lodLevel);

            auto& slotMats = lodSlotMats[lodLevel];

            if (slotMats.empty())
            {
                slotMats.resize(lod.m_materialSlots.size());
            }

            // iterate through slots
            for (size_t slotInd = 0; slotInd < lod.m_materialSlots.size(); slotInd++)
            {
                const nau::MaterialSlot& slot = lod.m_materialSlots[slotInd];
                uint64_t lodSlot = (uint64_t(lodLevel) << 32) | uint64_t(slotInd);

                nau::Ptr<nau::MaterialAssetView> material;
                if (info.overrideInfo.count(lodSlot))
                {
                    info.overrideInfo.at(lodSlot).material->getTyped<MaterialAssetView>(material);
                }
                else
                {
                    slot.m_material->getTyped<MaterialAssetView>(material);
                }

                if (!materialFilter(material))
                {
                    continue;
                }

                size_t matNameHash = material->getNameHash(); // TODO: cache this inside material
                auto& mats = slotMats[slotInd];

                if (!mats.count(matNameHash))
                {
                    mats[matNameHash] = ret->getEntitiesCount();

                    nau::RenderEntity& ent = ret->emplaceBack();
                    ent.positionBuffer = lod.m_positionsBuffer;
                    ent.normalsBuffer = lod.m_normalsBuffer;
                    ent.texcoordsBuffer = lod.m_texCoordsBuffer;
                    ent.tangentsBuffer = lod.m_tangentsBuffer;
                    ent.indexBuffer = lod.m_indexBuffer;
                    ent.startInstance = 0;
                    ent.instancesCount = 0;
                    ent.instanceData = {};
                    ent.tags = {};

                    ent.worldTransform = info.worldMatrix;  // keep first world matrix
                    ent.startIndex = slot.m_startIndex;
                    ent.endIndex = slot.m_endIndex;
                    ent.material = material;
                }

                uint32_t entInd = mats[matNameHash];
                nau::RenderEntity& entity = (*ret)[entInd];

                entity.instancesCount++;
                const nau::math::Matrix4 normalMatrix = math::transpose(math::inverse(info.worldMatrix));
                entity.instanceData.emplace_back(info.worldMatrix, normalMatrix, info.uid, info.isHighlighted);
            }
        }

        return ret;
    }

    void StaticMeshInstanceGroup::clearPendingInstances()
    {
        std::erase_if(m_instances, [](const auto& pair)
        {
            return pair.second.toDelete;
        });
    }

    void StaticMeshInstanceGroup::removeInstance(InstanceID instID)
    {
        NAU_ASSERT(contains(instID));
        m_instances.erase(instID);
    }

    bool StaticMeshInstanceGroup::contains(InstanceID instID) const
    {
        return m_instances.contains(instID);
    }

    size_t StaticMeshInstanceGroup::getInstancesCount() const
    {
        return m_instances.size();
    }

    InstanceInfo& StaticMeshInstanceGroup::getInstance(InstanceID instID)
    {
        return m_instances[instID];
    }

}  // namespace nau
