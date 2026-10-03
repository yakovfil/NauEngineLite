// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "logging_service.h"

#include <string_view>

#include "nau/diag/log_subscribers.h"

namespace nau
{
    LoggingService::LoggingService()
    {
        using namespace nau::diag;

        if (!hasLogger())
        {
            setLogger(createLogger());
        }
        else
        {
            NAU_LOG_WARNING("Logger is already set");
        }

        m_logSubscriptions.reserve(2);

        m_logSubscriptions.push_back(getLogger().subscribe(createDebugOutputLogSubscriber()));
        m_logSubscriptions.push_back(getLogger().subscribe(createConioOutputLogSubscriber()));
    }

    LoggingService::~LoggingService()
    {
        m_logSubscriptions.clear();
        diag::setLogger(nullptr);
    }

    void LoggingService::addFileOutput(std::string_view filename)
    {
        using namespace nau::diag;
        m_logSubscriptions.push_back(getLogger().subscribe(createFileOutputLogSubscriber(filename)));
    }

    async::Task<> LoggingService::shutdownService()
    {
        if (!diag::hasLogger())
        {
            co_return;
        }
    }

}  // namespace nau
