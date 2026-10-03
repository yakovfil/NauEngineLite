// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "vfx_impl.h"

#include <string>

#include "vfx_mod_fx_instance.h"

namespace nau::vfx
{
    async::Task<> VFXManagerImpl::preInitService()
    {
        return async::Task<>::makeResolved();
    }

    async::Task<> VFXManagerImpl::initService()
    {
        return async::Task<>::makeResolved();
    }

    async::Task<> VFXManagerImpl::shutdownService()
    {
        m_vfxInstances.clear();

        return async::Task<>::makeResolved();
    }

    void VFXManagerImpl::saveInstances(const std::string& filename) const
    {
        nau::DataBlock rootBlock;

        // Serialize each VFX instance
        int index = 0;
        for (const auto& instance : m_vfxInstances)
        {
            auto* instanceBlock = rootBlock.addNewBlock(("instance_" + std::to_string(index++)).c_str());
            instance->deserialize(instanceBlock);
        }

        // Save the DataBlock to a file
        if (!rootBlock.saveToTextFile(filename.c_str()))
        {
            NAU_LOG_ERROR("Failed to save VFX instances to file : " + filename);
        }
    }

    void VFXManagerImpl::loadInstances(const std::string& filename, const nau::MaterialAssetView::Ptr material)
    {
        nau::DataBlock rootBlock;

        // Load the DataBlock from the file
        if (!rootBlock.load(filename.c_str()))
        {
            NAU_LOG_ERROR("Failed to load VFX instances to file : " + filename);
        }

        m_vfxInstances.clear();

        int index = 0;
        while (true)
        {
            // TODO Add a type definition in the future
            auto* instanceBlock = rootBlock.getBlockByName(("instance_" + std::to_string(index++)).c_str());
            if (!instanceBlock)
                break;

            // Create a new instance (currently only ModFX supported)
            // TODO Add a type definition in the future
            std::shared_ptr<IVFXInstance> vfxInstance = std::make_shared<modfx::VFXModFXInstance>(material);
            if (vfxInstance->deserialize(instanceBlock))
            {
                m_vfxInstances.insert(vfxInstance);
            }
            else
            {
                NAU_LOG_ERROR("Failed to deserialize a VFX instance from file.");
            }
        }
    }

    std::shared_ptr<IVFXInstance> VFXManagerImpl::addInstance(const nau::MaterialAssetView::Ptr material)
    {
        // We only have a ModFX instance
        // TODO Add a factory in the future
        std::shared_ptr<IVFXInstance> vfxInstance = std::make_shared<modfx::VFXModFXInstance>(material);
        m_vfxInstances.insert(vfxInstance);

        return vfxInstance;
    }

    void VFXManagerImpl::removeInstance(std::shared_ptr<IVFXInstance> instance)
    {
        m_vfxInstances.erase(instance);
    }

    void VFXManagerImpl::update(float dt)
    {
        if (m_vfxInstances.empty())
            return;

        for (auto&& vfx : m_vfxInstances)
            vfx->update(dt);
    }

    void VFXManagerImpl::render(const nau::math::Matrix4& view, const nau::math::Matrix4& projection)
    {
        if (m_vfxInstances.empty())
            return;

        for (auto&& vfx : m_vfxInstances)
            vfx->render(view, projection);
    }
}  // namespace nau::vfx