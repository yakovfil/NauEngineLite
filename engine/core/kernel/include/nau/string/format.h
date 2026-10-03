// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// nau/service/format.h

#pragma once

#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "fmt/chrono.h"
#include "fmt/color.h"
#include "fmt/xchar.h"

namespace nau::utils
{
    namespace details
    {
        template <typename T>
        inline constexpr bool FormattableString =
            std::is_constructible_v<std::string_view, T> ||
            std::is_constructible_v<std::u8string_view, T>;

        template <typename T>
        static inline auto make_formatable_string_view(T&& arg)
        {
            if constexpr (std::is_constructible_v<std::string_view, T>)
            {
                return std::string_view{std::forward<T>(arg)};
            }
            else if constexpr (std::is_constructible_v<std::u8string_view, T>)
            {
                const auto view = std::u8string_view{std::forward<T>(arg)};
                return std::string_view(reinterpret_cast<const char*>(view.data()), view.size());
            }
            else
            {
                return std::forward<T>(arg);
            }
        }

        template <typename T>
        static inline decltype(auto) make_formatable_arg(T&& arg)
        {
            if constexpr (FormattableString<T>)
            {
                const auto view = make_formatable_string_view(std::forward<T>(arg));
                return fmt::string_view(view.data(), view.size());
            }
            else if constexpr (std::is_same_v<char8_t, std::remove_cvref_t<T>>)
            {
                return static_cast<char>(arg);
            }
            else
            {
                return std::forward<T>(arg);
            }
        }
    }  // namespace details

    template <class S, typename... Args>
    static inline std::string format(S&& formatString)
    {
        auto strV = details::make_formatable_string_view(std::forward<S>(formatString));
        return std::string(strV.data(), strV.size());
    }

    template <class S, typename... Args>
    static inline std::string format(S&& formatString, Args&&... args)
    {
        auto strV = details::make_formatable_string_view(std::forward<S>(formatString));
        const auto formatted = fmt::vformat(fmt::basic_string_view<char>{strV.data(), strV.size()},
                                            fmt::make_format_args<fmt::buffered_context<char>>(details::make_formatable_arg<Args>(std::forward<Args>(args))...));
        return std::string(formatted.data(), formatted.size());
    }

}  // namespace nau::utils