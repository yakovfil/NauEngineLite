// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include "nau/io/stream.h"
#include "nau/rtti/type_info.h"
#include "nau/utils/result.h"

namespace nau
{
    /**
     */
    struct NAU_ABSTRACT_TYPE ApplicationInitDelegate
    {
        virtual ~ApplicationInitDelegate() = default;

        /**
            @brief called for initial configuration setup (using GlobalProperties API)
                Must perform only very basic initialization. Most services currently is not acceptable.
         */
        virtual Result<> configureApplication() = 0;

        /**
            @brief called after basic core configuration is completed and right after all known modules are loaded
         */
        virtual Result<> initializeApplication() = 0;
    };

    Result<> applyDefaultAppConfiguration();

    // Are to be called after all core mudules are loaded
    Result<> initializeDefaultApplication();

}  // namespace nau
