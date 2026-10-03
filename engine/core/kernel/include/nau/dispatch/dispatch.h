// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <optional>
#include <string_view>

#include "nau/dispatch/class_descriptor.h"
#include "nau/dispatch/dispatch_args.h"
#include "nau/meta/class_info.h"
#include "nau/rtti/ptr.h"
#include "nau/rtti/rtti_object.h"
#include "nau/serialization/runtime_value.h"
#include "nau/utils/result.h"

namespace nau
{
    /**
     */
    struct NAU_ABSTRACT_TYPE IDispatch : virtual IRttiObject
    {
        NAU_INTERFACE(IDispatch, IRttiObject)

        virtual Result<nau::Ptr<>> invoke(std::string_view contract, std::string_view method, DispatchArguments args) = 0;

        virtual IClassDescriptor::Ptr getClassDescriptor() const = 0;
    };

}  // namespace nau