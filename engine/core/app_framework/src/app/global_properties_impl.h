// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "nau/app/global_properties.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/string/string_utils.h"

namespace nau
{
    /**
     */
    class GlobalPropertiesImpl final : public GlobalProperties
    {
        NAU_TYPEID(nau::GlobalPropertiesImpl)
        NAU_CLASS_BASE(GlobalProperties)

    public:
        GlobalPropertiesImpl();

        RuntimeValue::Ptr getRead(std::string_view path, IMemAllocator::Ptr allocator) const override;

        bool contains(std::string_view path) const override;

        Result<> set(std::string_view path, RuntimeValue::Ptr value) override;

        Result<RuntimeValue::Ptr> getModify(std::string_view path, ModificationLock& lock, IMemAllocator::Ptr allocator) override;

        Result<> mergeWithValue(const RuntimeValue& value) override;

        void addVariableResolver(std::string_view kind, VariableResolverCallback resolver) override;

    private:
        RuntimeValue::Ptr findValueAtPath(std::string_view valuePath) const;

        Result<RuntimeDictionary::Ptr> getDictionaryAtPath(std::string_view valuePath, bool createPath = true);

        std::optional<std::string> expandConfigString(std::string_view str) const;

        RuntimeDictionary::Ptr m_propsRoot;
        std::map<std::string, VariableResolverCallback, strings::CiStringComparer<std::string_view>> m_variableResolvers;
        mutable std::shared_mutex m_mutex;
    };
}  // namespace nau
