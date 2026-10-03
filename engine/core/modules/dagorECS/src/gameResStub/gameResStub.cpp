// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include <daECS/core/ecsGameRes.h>
#include <nau/rtti/rtti_impl.h>

#include <memory>
#include <vector>

namespace ecs
{

    struct IECSResourceManagerImpl : public IECSResourceManager
    {
        NAU_RTTI_CLASS(IECSResourceManagerImpl, IECSResourceManager)

        bool load_gameres_list(const gameres_list_t&) override
        {
            return true;
        }
        size_t filter_out_loaded_gameres(gameres_list_t& list) override
        {
            list.clear();
            return 0;
        }
        void async_load_gameres_list(std::vector<ecs::EntityId>&&, gameres_list_t&&) override
        {
        }
        virtual ~IECSResourceManagerImpl(){}
    };

    IECSResourceManager::Ptr ecsResourceManager;
    void setECSResourceManager(IECSResourceManager::Ptr newECSResourceManager)
    {
        ecsResourceManager = std::move(newECSResourceManager);
    }
    IECSResourceManager* getECSResourceManager()
    {
        return ecsResourceManager.get();
    };

    IECSResourceManager::Ptr createDefaultECSResourceManager()
    {
        return std::make_unique<IECSResourceManagerImpl>();
    }

}  // namespace ecs
