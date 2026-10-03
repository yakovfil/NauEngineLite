// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <string>

#include "nau/dispatch/dispatch_args.h"
#include "nau/kernel/kernel_config.h"
#include "nau/scripts/script_manager.h"
#include "nau/serialization/runtime_value_builder.h"
#include "nau/service/service_provider.h"
#include "nau/utils/result.h"

namespace nau::scripts
{
    template <typename>
    struct GlobalFunction;

    template <typename R, typename... P>
    struct GlobalFunction<R(P...)>
    {
        const std::string name;

        template <typename Text>
        requires(std::is_constructible_v<std::string, Text>)
        GlobalFunction(Text&& inName) :
            name(std::forward<Text>(inName))
        {
        }

        Result<R> operator()(P... params)
        {
            DispatchArguments args;
            (args.emplace_back(makeValueCopy(std::move(params))), ...);

            if constexpr (!std::is_same_v<void, R>)
            {
                Result<R> retValue;

                auto invokeRes = getServiceProvider().get<scripts::ScriptManager>().invokeGlobal(name.c_str(), std::move(args), [&retValue](const nau::Ptr<>& invokeResultValue)
                {
                    if (invokeResultValue)
                    {
                        retValue = runtimeValueCast<R>(invokeResultValue);
                    }
                });

                NauCheckResult(invokeRes);
                NauCheckResult(retValue);
                return retValue;
            }
            else
            {
                auto invokeRes = getServiceProvider().get<scripts::ScriptManager>().invokeGlobal(name.c_str(), std::move(args), nullptr);
                NauCheckResult(invokeRes);

                return ResultSuccess;
            }
        }
    };
}  // namespace nau::scripts
