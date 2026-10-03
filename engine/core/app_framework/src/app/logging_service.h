// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <string_view>
#include <vector>

#include "nau/diag/logging.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/rtti/type_info.h"
#include "nau/service/service.h"

namespace nau
{
    class LoggingService : public IServiceShutdown
    {
    public:
        NAU_RTTI_CLASS(LoggingService, IServiceShutdown)

        LoggingService();

        virtual ~LoggingService();

        virtual void addFileOutput(std::string_view filename);

    private:
        async::Task<> shutdownService() override;

        std::vector<diag::Logger::SubscriptionHandle> m_logSubscriptions;
    };

}  // namespace nau
