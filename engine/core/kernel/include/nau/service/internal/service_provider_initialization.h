// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <atomic>

#include "nau/async/task_base.h"
#include "nau/rtti/rtti_object.h"

namespace nau
{
    struct IServiceInitialization;
}

namespace nau::core_detail
{

    /**
        @brief internal API dedicated to manage service initialization and shutting down phases
     */
    struct NAU_ABSTRACT_TYPE IServiceProviderInitialization
    {
        NAU_TYPEID(nau::core_detail::IServiceProviderInitialization);

        virtual ~IServiceProviderInitialization() = default;

        /**
            @brief setting initialization proxy for specified service.

            In some cases, a service may require a specific initialization procedure (for example, a call must occur in a dedicated thread).
            A proxy object can be used for this. If a proxy is installed, all IServiceInitialization calls will be directed to it.
            If proxy exposes IServiceShutdown API then shutdownService() will also be called for proxy object
            (otherwise shutdownService() will be called for original object).

            IMPORTANT! To use the proxy mechanism, the source type must also expose IServiceInitialization/IServiceShutdown interfaces
            (this is required because service inter dependencies are computed for original types, not proxies, also other types knowns nothing about proxies.
            i.e. IServiceInitialization::getServiceDependencies() for proxy are ignored and never be called).
         */
        virtual void setInitializationProxy(const IServiceInitialization& source, IServiceInitialization* proxy) = 0;

        virtual async::Task<> preInitServices() = 0;

        // The owner retains this token until all initialization work settles.
        virtual void setInitializationStopToken(const std::atomic<bool>* token) = 0;

        virtual async::Task<> initServices() = 0;

        virtual Error::Ptr getLifecycleError() const = 0;
        virtual uint64_t getLifecycleProgress() const = 0;

        virtual async::Task<> shutdownServices() = 0;
    };
}  // namespace nau::core_detail
