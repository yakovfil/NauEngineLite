// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <daECS/core/entityId.h>
#include <nau/generated/generated_DagorECS_config.h>
#include <nau/rtti/rtti_object.h>

#include <memory>
#include <string>
#include <vector>

#include "nau/utils/sorted_vector.h"

namespace ecs  // link time resolved dependency on async resources
{
    enum class RequestResources
    {
        Loaded,    //Resources were loaded
        Scheduled, //Still loading resources
        Error      //Resources wasn`t loaded     
    };
    enum class RequestResourcesType : bool
    {
        ASYNC = false,
        SYNC = true
    };

    using gameres_list_t = nau::SortedVectorMap<std::string /*res_name*/, uint32_t /*res_class_id*/>;
    struct NAU_ABSTRACT_TYPE IECSResourceManager : virtual ::nau::IRttiObject
    {
        NAU_INTERFACE(IECSResourceManager, ::nau::IRttiObject)

        using Ptr = std::unique_ptr<IECSResourceManager>;

        virtual bool load_gameres_list(const gameres_list_t& reslist) = 0;
        virtual size_t filter_out_loaded_gameres(gameres_list_t& reslist) = 0;  // return size of filtered reslist(resources, that weren`t loaded)
        virtual void async_load_gameres_list(std::vector<ecs::EntityId>&&, gameres_list_t&&) = 0;
        virtual ~IECSResourceManager()
        {
        }
    };

    NAU_DAGORECS_EXPORT void setECSResourceManager(IECSResourceManager::Ptr newECSResourceManager);

    NAU_DAGORECS_EXPORT IECSResourceManager* getECSResourceManager();

    NAU_DAGORECS_EXPORT IECSResourceManager::Ptr createDefaultECSResourceManager();
};  // namespace ecs
