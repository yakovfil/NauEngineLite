// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <vector>

#include "nau/messaging/messaging.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/runtime/async_disposable.h"
#include "nau/service/service.h"

namespace nau
{
    class PlatformWindowService final : public IServiceInitialization,
                                        public IAsyncDisposable
    {
        NAU_RTTI_CLASS(PlatformWindowService, IServiceInitialization, IAsyncDisposable)
    public:
        PlatformWindowService() = default;

        ~PlatformWindowService();

        async::Task<> preInitService() override;

        async::Task<> initService() override;

        async::Task<> disposeAsync() override;

    private:
        std::thread m_platformAppThread;
        async::Task<> m_platformAppCompletedTask;
        std::vector<AsyncMessageSubscription> m_messageSubscriptions;
    };

}  // namespace nau
