// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <EASTL/functional.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

#include "nau/kernel/kernel_config.h"
#include "nau/rtti/type_info.h"
#include "nau/utils/result.h"

namespace nau
{

    /**

    */
    struct NAU_KERNEL_EXPORT Uid
    {
        static Uid generate();
        static Result<Uid> parseString(std::string_view);

        /**
         */
        Uid() noexcept;
        Uid(const Uid&) = default;
        Uid& operator=(const Uid&) = default;

        /**
         */
        explicit operator bool() const noexcept;

    private:
        Uid(std::array<uint8_t, 16> data) noexcept;

        size_t getHashCode() const;

        std::array<uint8_t, 16> m_data;

        /**
         */
        NAU_KERNEL_EXPORT friend Result<> parse(std::string_view str, Uid&);
        NAU_KERNEL_EXPORT friend std::string toString(const Uid& uid);
        NAU_KERNEL_EXPORT friend bool operator<(const Uid& uid, const Uid& uidOther) noexcept;
        NAU_KERNEL_EXPORT friend bool operator==(const Uid& uid, const Uid& uidOther) noexcept;
        NAU_KERNEL_EXPORT friend bool operator!=(const Uid& uid, const Uid& uidOther) noexcept;

        friend std::hash<nau::Uid>;
        friend eastl::hash<nau::Uid>;
    };

    NAU_KERNEL_EXPORT std::string toString(const Uid& uid);

}  // namespace nau

NAU_DECLARE_TYPEID(nau::Uid)

template <>
struct std::hash<nau::Uid>
{
    [[nodiscard]] size_t operator()(const nau::Uid& val) const
    {
        return val.getHashCode();
    }
};

template <>
struct eastl::hash<nau::Uid>
{
    [[nodiscard]] size_t operator()(const nau::Uid& val) const
    {
        return val.getHashCode();
    }
};
