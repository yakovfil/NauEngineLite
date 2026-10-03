// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <chrono>
#include <memory>
#include <string>

#include "nau/app/application_init_delegate.h"
#include "nau/async/task_base.h"
#include "nau/utils/result.h"

namespace nau
{
    /**
     */
    struct ApplicationDelegate : ApplicationInitDelegate
    {
        using Ptr = std::unique_ptr<ApplicationDelegate>;

        /**
         */
        virtual std::string getModulesListString() const = 0;

        /**
         */
        virtual Result<> initializeServices() = 0;

        /**
         */
        virtual void onApplicationInitialized() = 0;

        /**
         */
        virtual async::Task<> startupApplication() = 0;

        virtual void onApplicationStep(std::chrono::milliseconds dt);

    protected:
        Result<> initializeApplication() override;
    };

    /**
     */
    ApplicationDelegate::Ptr createDefaultApplicationDelegate(std::string dynModulesList);
}  // namespace nau
