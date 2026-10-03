// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <string>
#include <type_traits>

#include "nau/meta/attribute.h"

namespace nau::meta
{
    NAU_DEFINE_ATTRIBUTE(ClassNameAttribute, "nau.class.name", AttributeOptionsNone)

    template <typename T>
    inline constexpr bool ClassHasName = AttributeDefined<T, ClassNameAttribute>;

    /**
     */
    template <typename T>
    std::string getClassName()
    {
        if constexpr (AttributeDefined<T, ClassNameAttribute>)
        {
            static_assert(std::is_constructible_v<std::string, AttributeValueType<T, ClassNameAttribute>>);
            return meta::getAttributeValue<T, ClassNameAttribute>();
        }
        else
        {
            return std::string{};
        }
    }

}  // namespace nau::meta

#define CLASS_NAME_ATTRIBUTE(ClassName) CLASS_ATTRIBUTE(nau::meta::ClassNameAttribute, std::string{ClassName})
