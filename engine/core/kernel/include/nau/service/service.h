// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <vector>

#include "nau/async/task_base.h"
#include "nau/kernel/kernel_config.h"
#include "nau/rtti/rtti_object.h"

namespace nau
{
    /**
     */
    struct NAU_ABSTRACT_TYPE IServiceInitialization : virtual IRttiObject
    {
        NAU_INTERFACE(nau::IServiceInitialization, IRttiObject)

        virtual async::Task<> preInitService()
        {
            return async::Task<>::makeResolved();
        }

        virtual async::Task<> initService()
        {
            return async::Task<>::makeResolved();
        }

        virtual std::vector<const rtti::TypeInfo*> getServiceDependencies() const
        {
            return {};
        }
    };

    /**
     */
    struct NAU_ABSTRACT_TYPE IServiceShutdown : virtual IRttiObject
    {
        NAU_INTERFACE(nau::IServiceShutdown, IRttiObject)

        virtual async::Task<> shutdownService() = 0;
    };
}  // namespace nau
