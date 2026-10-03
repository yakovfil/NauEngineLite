// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <span>

#include "nau/scene/components/component.h"
#include "nau/scene/components/component_life_cycle.h"
#include "nau/scene/scene_processor.h"
#include "nau/service/service.h"

namespace nau::animation
{
    class NAU_ANIMATION_EXPORT AnimationSceneProcessor final : public IRefCounted,
                                                               public scene::ISceneProcessor,
                                                               public scene::IComponentsAsyncActivator,
                                                               public IServiceInitialization
    {
        NAU_CLASS_(AnimationSceneProcessor,
                   IRefCounted,
                   scene::ISceneProcessor,
                   scene::IComponentsAsyncActivator,
                   IServiceInitialization)

    public:
        AnimationSceneProcessor();
        ~AnimationSceneProcessor();

        async::Task<> preInitService() override;

        async::Task<> initService() override;

        async::Task<> activateComponentsAsync(Uid worldUid, std::span<const scene::Component*> components, async::Task<> barrier) override;

        void syncSceneState() override;
    };

}  // namespace nau::animation
