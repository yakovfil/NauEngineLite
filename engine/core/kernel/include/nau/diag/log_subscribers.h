// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// nau/diag/log_subscribers.h

#pragma once

#include <string_view>

#include "nau/diag/logging.h"
#include "nau/kernel/kernel_config.h"

namespace nau::diag
{
    NAU_KERNEL_EXPORT ILogSubscriber::Ptr createDebugOutputLogSubscriber();

    NAU_KERNEL_EXPORT ILogSubscriber::Ptr createConioOutputLogSubscriber();

    NAU_KERNEL_EXPORT ILogSubscriber::Ptr createFileOutputLogSubscriber(std::string_view filename);

}  // namespace nau::diag