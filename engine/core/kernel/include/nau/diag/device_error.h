// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// nau/diag/device_error.h

// Core assert system header.

#pragma once

#include <memory>
#include <string_view>

#include "nau/diag/assertion.h"
#include "nau/kernel/kernel_config.h"
#include "nau/rtti/rtti_object.h"

namespace nau::diag
{
    struct FailureData
    {
        /**
         * @brief information about problem.
         * @param error - specific error name.
         * @param condition - possible condition triggered fatal.
         * @param file - file where was triggered fatal.
         * @param line - line where was triggered fatal.
         * @param function - function where was triggered fatal.
         * @param message - additional info about problem.
         */
        const uint32_t error;
        const AssertionKind kind = AssertionKind::Default;
        const SourceInfo source;
        const std::string_view condition;
        const std::string_view message;

        FailureData(uint32_t inError, AssertionKind inKind, SourceInfo inSource, std::string_view inCondition, std::string_view inMessage) :
            error(inError),
            kind(inKind),
            source(inSource),
            condition(inCondition),
            message(inMessage)
        {
        }
    };

    struct IDeviceError : virtual IRttiObject
    {
        NAU_INTERFACE(nau::diag::IDeviceError, IRttiObject)

        using Ptr = std::unique_ptr<IDeviceError>;

        virtual FailureActionFlag handleFailure(const FailureData&) = 0;
    };

    NAU_KERNEL_EXPORT void setDeviceError(IDeviceError::Ptr newDeviceError, IDeviceError::Ptr* prevDeviceError = nullptr);

    NAU_KERNEL_EXPORT IDeviceError* getDeviceError();

    NAU_KERNEL_EXPORT IDeviceError::Ptr createDefaultDeviceError();
}  // namespace nau::diag
