// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <set>
#include <string>

#include "nau/math/math.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/service/service.h"
#include "nau/vfx_manager.h"
#include "vfx_instance.h"

namespace nau::vfx
{
    class NAU_VFX_EXPORT VFXManagerImpl final : public IServiceInitialization,
                                                public IServiceShutdown,
                                                public VFXManager
    {
        NAU_RTTI_CLASS(
            nau::vfx::VFXManagerImpl,
            nau::IServiceInitialization,
            nau::IServiceShutdown,
            nau::vfx::VFXManager)

    public:
        virtual async::Task<> preInitService() override;
        virtual async::Task<> initService() override;
        virtual async::Task<> shutdownService() override;

        void saveInstances(const std::string& filename) const;
        void loadInstances(const std::string& filename, const nau::MaterialAssetView::Ptr material);

        virtual std::shared_ptr<IVFXInstance> addInstance(const nau::MaterialAssetView::Ptr material) override;
        virtual void removeInstance(std::shared_ptr<IVFXInstance> instance) override;

        virtual void update(float dt) override;
        virtual void render(const nau::math::Matrix4& view, const nau::math::Matrix4& projection) override;

    private:
        std::set<std::shared_ptr<IVFXInstance>> m_vfxInstances;
    };
}  // namespace nau::vfx