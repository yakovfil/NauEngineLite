// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "render_pipeline/static_mesh_manager.h"

#include <graphics_impl.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "nau/math/dag_lsbVisitor.h"

namespace nau
{
    async::Task<std::shared_ptr<StaticMeshInstanceGroup>> StaticMeshManager::findOrCreateGroup(StaticMeshAssetRef ref)
    {
        auto& graphics = getServiceProvider().get<nau::GraphicsImpl>();
        ASYNC_SWITCH_EXECUTOR(graphics.getPreRenderExecutor());

        auto findByRef = [&ref](const auto& val)
            {
                return val.first == ref && !val.second.expired();
            };

        auto meshAsset = co_await ref.getReloadableAssetViewTyped<StaticMeshAssetView>();

        std::shared_ptr<nau::StaticMeshInstanceGroup> group = nullptr;
        if (std::find_if(m_assetRefToGroup.begin(), m_assetRefToGroup.end(), findByRef) == m_assetRefToGroup.end())
        {
            NAU_ASSERT(meshAsset);
            group = std::make_shared<nau::StaticMeshInstanceGroup>(meshAsset);
            m_meshGroups.push_back(group);
            m_assetRefToGroup.emplace_back(ref, group);
        }

        if(group == nullptr)
        {
            std::weak_ptr<StaticMeshInstanceGroup> weakGroup = std::find_if(m_assetRefToGroup.begin(), m_assetRefToGroup.end(), findByRef)->second;
            group = weakGroup.lock();
            NAU_ASSERT(group);
        }
        
        co_return group;
    }

    async::Task<std::unique_ptr<nau::MeshHandle>> StaticMeshManager::addStaticMesh(StaticMeshAssetRef ref, const nau::math::Matrix4& matrix)
    {
        std::shared_ptr<StaticMeshInstanceGroup> group = co_await findOrCreateGroup(ref);

        auto& graphics = getServiceProvider().get<nau::GraphicsImpl>();
        ASYNC_SWITCH_EXECUTOR(graphics.getPreRenderExecutor());

        auto instance = group->addInstance(matrix);

        std::unique_ptr<nau::MeshHandle> ret = std::make_unique<nau::MeshHandle>();
        ret->m_group      = group;
        ret->m_instInfo   = instance;
        ret->m_generation = 0;
        ret->m_manager    = this;
        ret->m_scene      = m_sceneOwner;

        co_return std::move(ret);
    }


    void StaticMeshManager::render(nau::math::Matrix4 viewProj)
    {
        for(auto& weakGroup : m_meshGroups)
        {
            if (const auto& group = weakGroup.lock())
            {
                auto dummy = std::function<bool(const InstanceInfo&)>([](const InstanceInfo&) -> bool
                {
                    return true;
                });
                auto dummyForMaterials = std::function<bool(const MaterialAssetView::Ptr)>([](const MaterialAssetView::Ptr) -> bool
                {
                    return true;
                });
                auto list = group->createRenderList({}, dummy, dummyForMaterials);

                for (auto& ent : list->getEntities())
                {
                    ent.render(viewProj);
                }
            }
            else
            {
                m_isGroupsDirty = true;
            }
        }
    }

    RenderList::Ptr nau::StaticMeshManager::getRenderList(const nau::math::Vector3& viewerPosition,
                                                          std::function<bool(const InstanceInfo&)>& filterFunc,
                                                          std::function<bool(const nau::MaterialAssetView::Ptr)>& materialFilter)
    {
        std::vector<RenderList::Ptr> lists;
        lists.reserve(m_meshGroups.size());
        for (auto& weakGroup : m_meshGroups)
        {
            if (const auto& group = weakGroup.lock())
            {
                lists.emplace_back(group->createRenderList({}, filterFunc, materialFilter));
            }
            else
            {
                m_isGroupsDirty = true;
            }
        }

        return std::make_shared<RenderList>(std::move(lists));
    }


    void StaticMeshManager::update()
    {
        for (auto& weakGroup : m_meshGroups)
        {
            if (const auto& group = weakGroup.lock())
            {
                group->clearPendingInstances();
            }
            else
            {
                m_isGroupsDirty = true;
            }
        }

        if (m_isGroupsDirty)
        {
            std::erase_if(m_assetRefToGroup, [](const auto& refToGroup)
            {
                return refToGroup.second.expired();
            });

            std::erase_if(m_meshGroups, [](const auto& group)
            {
                return group.expired();
            });
            m_isGroupsDirty = false;
        }
    }


    bool MeshHandle::isValid() const
    {
        return true;
    }


    void MeshHandle::setWorldTransform(const nau::math::Transform& transform)
    {
        m_instInfo.worldMatrix = transform.getMatrix();

        float maxScale = maxElem(transform.getScale());
        // update aabb, bounding sphere, etc.
        m_instInfo.worldSphere = nau::math::BSphere3(m_instInfo.worldMatrix.getTranslation(), m_instInfo.localSphere.r * maxScale);
    }

    nau::math::Matrix4 MeshHandle::getWorldPos()
    {
        return m_instInfo.worldMatrix;
    }

    void MeshHandle::setVisibility(bool isVisible)
    {
        m_instInfo.isVisible = isVisible;
    }

    bool MeshHandle::getVisibility() const
    {
        return m_instInfo.isVisible;
    }

    void MeshHandle::setHighlighted(bool isHighlighted)
    {
        m_instInfo.isHighlighted = isHighlighted;
    }

    bool MeshHandle::isHighlighted() const
    {
        return m_instInfo.isHighlighted;
    }

    void nau::MeshHandle::setUid(const nau::Uid& uid)
    {
        m_instInfo.uid = uid;
    }

    nau::Uid nau::MeshHandle::getUid() const
    {
        return m_instInfo.uid;
    }

    void MeshHandle::addRenderTag(RenderTag tag)
    {
    }

    void MeshHandle::removeRenderTag(RenderTag tag)
    {
    }

    void MeshHandle::setCastShadow(bool castShadow)
    {
        m_instInfo.isCastShadow = castShadow;
    }


    void MeshHandle::syncState(nau::scene::StaticMeshComponent& component)
    {
        NAU_ASSERT(m_group);
        using DirtyFlags = nau::scene::StaticMeshComponent::DirtyFlags;

        nau::InstanceInfo& info = m_group->getInstance(m_instInfo.id);
        info.isHighlighted = m_instInfo.isHighlighted;
        info.uid = m_instInfo.uid;

        if(isMaterialDirty)
        {
            info.overrideInfo = m_instInfo.overrideInfo;
            isMaterialDirty = false;
        }

        uint32_t flags = component.getDirtyFlags();
        for (auto flag : nau::math::LsbVisitor{ flags })
        {
            switch (1 << flag)
            {
            case static_cast<uint32_t>(DirtyFlags::WorldPos):
                setWorldTransform(component.getWorldTransform());
                info.worldMatrix = m_instInfo.worldMatrix;
                info.worldSphere = m_instInfo.worldSphere;
                break;
            //case static_cast<uint32_t>(DirtyFlags::Material):
            //    info.overrideInfo = m_instInfo.overrideInfo;
            //    break;
            case static_cast<uint32_t>(DirtyFlags::Visibility):
                setVisibility(component.getVisibility());
                info.isVisible = m_instInfo.isVisible;
                break;
            case static_cast<uint32_t>(DirtyFlags::CastShadow):
                setCastShadow(component.getCastShadow());
                info.isCastShadow = m_instInfo.isCastShadow;
                break;
            }
        }
    }

    void MeshHandle::overrideMaterial(uint32_t lodIndex, uint32_t slotIndex, ReloadableAssetView::Ptr material)
    {
        NAU_ASSERT(m_manager);

        uint64_t lodSlot = (uint64_t(lodIndex) << 32) | uint64_t(slotIndex);
        nau::MaterialOverrideInfo overInfo = {.lodSlot = lodSlot, .material = material};

        m_instInfo.overrideInfo[lodSlot] = std::move(overInfo);
        isMaterialDirty = true;
    }

    MeshHandle::~MeshHandle()
    {
        NAU_ASSERT(m_manager);
        NAU_ASSERT(m_group);

        m_group->getInstance(m_instInfo.id).toDelete = true;
        m_group.reset();
    }

} //namespace nau
