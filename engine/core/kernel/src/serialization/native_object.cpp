// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/serialization/native_runtime_value/native_object.h"

#include <span>

namespace nau::ser_detail
{

    std::string_view RuntimeObjectState::getKey(size_t index) const
    {
        auto fields = getFields();

        NAU_ASSERT(index < fields.size());
        return fields[index].getName();
    }

    RuntimeValue::Ptr RuntimeObjectState::getValue(const RuntimeValue& parent, const void* obj, std::string_view key) const
    {
        auto fields = getFields();

        auto field = std::find_if(fields.begin(), fields.end(), [key](const RuntimeFieldAccessor& field)
        {
            return strings::icaseEqual(field.getName(), key);
        });

        return field != fields.end() ? field->getRuntimeValue(parent, const_cast<void*>(obj)) : nullptr;
    }

    bool RuntimeObjectState::containsKey(std::string_view key) const
    {
        return findField(key) != nullptr;
    }

    Result<> RuntimeObjectState::setFieldValue(const RuntimeValue& parent, const void* obj, std::string_view key, const RuntimeValue::Ptr& value)
    {
        auto* const field = findField(key);
        if (field == nullptr)
        {
            return NauMakeError("Class does not contains field:({})", key);
        }

        return RuntimeValue::assign(field->getRuntimeValue(parent, const_cast<void*>(obj)), value);
    }

    RuntimeFieldAccessor* RuntimeObjectState::findField(std::string_view key) const
    {
        const std::span<RuntimeFieldAccessor> fields = getFields();

        const auto field = std::find_if(fields.begin(), fields.end(), [key](const RuntimeFieldAccessor& field)
        {
            return strings::icaseEqual(field.getName(), key);
        });

        return field != fields.end() ? std::addressof(*field) : nullptr;
    }
}  // namespace nau::ser_detail
