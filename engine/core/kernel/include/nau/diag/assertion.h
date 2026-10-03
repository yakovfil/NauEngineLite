// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// nau/diag/assertion.h

// Core assert system header.

#pragma once
#include <atomic>
#include <format>
#include <string>
#include <string_view>
#include <type_traits>

#include "nau/debug/debugger.h"
#include "nau/diag/source_info.h"
#include "nau/kernel/kernel_config.h"
#include "nau/string/format.h"
#include "nau/string/string_conv.h"
#include "nau/utils/preprocessor.h"
#include "nau/utils/typed_flag.h"

namespace nau::diag
{
    enum class AssertionKind
    {
        Default,
        Fatal
    };

    enum class FailureAction : unsigned
    {
        None = NauFlag(0),
        DebugBreak = NauFlag(1),
        Abort = NauFlag(2)
    };

    NAU_DEFINE_TYPED_FLAG(FailureAction)

}  // namespace nau::diag

namespace nau::diag_detail
{
    template <typename... Args>
    consteval inline std::string_view makeFailureMessage()
    {
        return "";
    }

    template <typename T>
    inline decltype(auto) makeFormatableArgs(T&& arg)
    {
        if constexpr (std::is_same_v<std::remove_cvref_t<T>, std::nullptr_t>)
        {
            return std::string_view{"NULLPTR"};
        }
        else if constexpr (std::is_convertible_v<T, const char*> && std::is_pointer_v<std::remove_cvref_t<T>>)
        {
            return arg ? std::string_view{arg} : std::string_view{"NULLPTR"};
        }
        else
        {
            return std::forward<T>(arg);
        }
    }

    template <typename... Args>
    inline std::string makeFailureMessage(std::u8string_view text, Args&&... formatArgs)
    {
        if constexpr (sizeof...(Args) == 0)
        {
            return std::string(reinterpret_cast<const char*>(text.data()), text.size());
        }
        else
        {
            return nau::utils::format(text, makeFormatableArgs<Args>(std::forward<Args>(formatArgs))...);
        }
    }

    template <typename... Args>
    inline std::string makeFailureMessage(std::string_view text, Args&&... formatArgs)
    {
        if constexpr (sizeof...(Args) == 0)
        {
            return std::string{text.data(), text.size()};
        }
        else
        {
            return nau::utils::format(text, makeFormatableArgs<Args>(std::forward<Args>(formatArgs))...);
        }
    }

    template <typename... Args>
    inline std::string makeFailureMessage(std::wstring_view text, const Args&... formatArgs)
    {
        if constexpr (sizeof...(Args) == 0)
        {
            return makeFailureMessage(strings::wstringToUtf8(text));
        }
        else
        {
            const std::wstring formattedMessage = ::fmt::vformat(fmt::wstring_view{text.data(), text.size()}, fmt::make_format_args<fmt::buffered_context<wchar_t>>(formatArgs...));
            return makeFailureMessage(strings::wstringToUtf8(std::wstring_view{formattedMessage.data(), formattedMessage.size()}));
        }
    }

    /**
     */
    NAU_KERNEL_EXPORT
    diag::FailureActionFlag raiseFailure(uint32_t error, diag::AssertionKind kind, diag::SourceInfo source, std::string_view condition, std::string_view message);

}  // namespace nau::diag_detail

#define NAU_ASSERT_IMPL(error, kind, condition, ...)                                                                                                                         \
    do                                                                                                                                                                       \
    {                                                                                                                                                                        \
        if (!(condition)) [[unlikely]]                                                                                                                                       \
        {                                                                                                                                                                    \
            const auto errorFlags = ::nau::diag_detail::raiseFailure(error, kind, NAU_INLINED_SOURCE_INFO, #condition, ::nau::diag_detail::makeFailureMessage(__VA_ARGS__)); \
            if (errorFlags.has(::nau::diag::FailureAction::DebugBreak) && ::nau::debug::isRunningUnderDebugger())                                                            \
            {                                                                                                                                                                \
                NAU_PLATFORM_BREAK;                                                                                                                                          \
            }                                                                                                                                                                \
            if (errorFlags.has(::nau::diag::FailureAction::Abort))                                                                                                           \
            {                                                                                                                                                                \
                NAU_PLATFORM_ABORT;                                                                                                                                          \
            }                                                                                                                                                                \
        }                                                                                                                                                                    \
    } while (0)

#define NAU_FAILURE_IMPL(error, kind, ...)                                                                                                                              \
    do                                                                                                                                                                  \
    {                                                                                                                                                                   \
        const auto errorFlags = ::nau::diag_detail::raiseFailure(error, kind, NAU_INLINED_SOURCE_INFO, "Failure", ::nau::diag_detail::makeFailureMessage(__VA_ARGS__)); \
        if (errorFlags.has(::nau::diag::FailureAction::DebugBreak) && ::nau::debug::isRunningUnderDebugger())                                                           \
        {                                                                                                                                                               \
            NAU_PLATFORM_BREAK;                                                                                                                                         \
        }                                                                                                                                                               \
        if (errorFlags.has(::nau::diag::FailureAction::Abort))                                                                                                          \
        {                                                                                                                                                               \
            NAU_PLATFORM_ABORT;                                                                                                                                         \
        }                                                                                                                                                               \
    } while (0)

#ifdef NAU_ASSERT_ENABLED

    #define NAU_ASSERT(condition, ...) NAU_ASSERT_IMPL(1, ::nau::diag::AssertionKind::Default, condition, ##__VA_ARGS__)

    #define NAU_FATAL(condition, ...) NAU_ASSERT_IMPL(1, ::nau::diag::AssertionKind::Fatal, condition, ##__VA_ARGS__)

    #define NAU_FAILURE(...) NAU_FAILURE_IMPL(1, ::nau::diag::AssertionKind::Default, ##__VA_ARGS__)

    #define NAU_FATAL_FAILURE(...) NAU_FAILURE_IMPL(1, ::nau::diag::AssertionKind::Fatal, ##__VA_ARGS__)

    #define NAU_ENSURE(condition, ...)                                                                  \
        do                                                                                              \
        {                                                                                               \
            static std::atomic<bool> s_wasTrigered = false;                                             \
            bool value = s_wasTrigered.load(std::memory_order_relaxed);                                 \
            if (!value && (!(condition)))                                                               \
            {                                                                                           \
                if (!value && !s_wasTrigered.compare_exchange_strong(value, true))                      \
                {                                                                                       \
                    if (!s_wasTrigered && !s_wasTrigered.compare_exchange_strong(false, true))          \
                    {                                                                                   \
                        NAU_ASSERT_IMPL(1, nau::diag::AssertionKind::Default, condition, ##__VA_ARGS__) \
                    }                                                                                   \
                }                                                                                       \
            }                                                                                           \
        } while (0)

    #define NAU_ENSURE_ALWAYS(condition, ...)                                                           \
        do                                                                                              \
        {                                                                                               \
            static std::atomic<bool> s_wasTrigered = false;                                             \
            bool value = s_wasTrigered.load(std::memory_order_relaxed);                                 \
            if ((!(condition)))                                                                         \
            {                                                                                           \
                if (!value && !s_wasTrigered.compare_exchange_strong(value, true))                      \
                {                                                                                       \
                    if (!s_wasTrigered && !s_wasTrigered.compare_exchange_strong(false, true))          \
                    {                                                                                   \
                        NAU_ASSERT_IMPL(1, nau::diag::AssertionKind::Default, condition, ##__VA_ARGS__) \
                    }                                                                                   \
                }                                                                                       \
            }                                                                                           \
        } while (0)
    #define NAU_FAST_ASSERT(expr)      \
        do                             \
        {                              \
            if (NAU_UNLIKELY(!(expr))) \
            {                          \
                NAU_PLATFORM_BREAK;    \
            }                          \
        } while (0)
#else
    #define NAU_ASSERT(condition, ...)

    #define NAU_FATAL(condition, ...)

    #define NAU_FAILURE(...)

    #define NAU_FATAL_FAILURE(...)
    #define NAU_FAST_ASSERT(expr)
#endif

#define NAU_VERIFY(condition, ...) NAU_ASSERT_IMPL(1, ::nau::diag::AssertionKind::Default, condition, ##__VA_ARGS__)

#define NAU_FAILURE_ALWAYS(...) NAU_FAILURE_IMPL(1, ::nau::diag::AssertionKind::Default, ##__VA_ARGS__)

#if ((_MSC_VER >= 1600 && !defined(__INTEL_COMPILER)) || (__cplusplus > 199711L))
    #define HAVE_STATIC_ASSERT 1
#else
    #define HAVE_STATIC_ASSERT 0
#endif

#if !defined(NAU_STATIC_ASSERT) && HAVE_STATIC_ASSERT
    #define NAU_STATIC_ASSERT(x) static_assert((x), "assertion failed: " #x)
#endif

#ifndef NAU_STATIC_ASSERT
    #define NAU_STATIC_ASSERT(x)                   \
        if (sizeof(char[2 * (((x) ? 1 : 0) - 1)])) \
            ;                                      \
        else
#endif

#undef HAVE_STATIC_ASSERT

// Warning: this macro is unhygienic! Use with care around ifs and loops.
#define NAU_ASSERT_AND_DO_UNHYGIENIC(expr, cmd, ...)           \
    {                                                          \
        const bool g_assert_result_do_ = !!(expr);             \
        NAU_ASSERT(g_assert_result_do_, #expr, ##__VA_ARGS__); \
        if (NAU_UNLIKELY(!g_assert_result_do_))                \
        {                                                      \
            cmd;                                               \
        }                                                      \
    }

#define NAU_ASSERT_AND_DO(expr, cmd, ...)                      \
    do                                                         \
    {                                                          \
        NAU_ASSERT_AND_DO_UNHYGIENIC(expr, cmd, ##__VA_ARGS__) \
    } while (0)

#define NAU_RETURN_FIRST_ARG(returnValue, ...) returnValue
#define NAU_RETURN_RESTS_ARG(returnValue, ...) __VA_OPT__(, ) __VA_ARGS__

#define NAU_ASSERT_RETURN(expr, ...) NAU_ASSERT_AND_DO(expr, return __VA_OPT__(NAU_RETURN_FIRST_ARG(__VA_ARGS__)); NAU_RETURN_RESTS_ARG(__VA_ARGS__))
#define NAU_ASSERT_BREAK(expr, ...) NAU_ASSERT_AND_DO_UNHYGIENIC(expr, break, ##__VA_ARGS__)
#define NAU_ASSERT_CONTINUE(expr, ...) NAU_ASSERT_AND_DO_UNHYGIENIC(expr, continue, ##__VA_ARGS__)
