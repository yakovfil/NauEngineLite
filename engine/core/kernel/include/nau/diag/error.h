// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <fmt/format.h>

#include <exception>
#include <string>
#include <string_view>
#include <type_traits>

#include "nau/diag/source_info.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/rtti/rtti_object.h"

NAU_DECLARE_TYPEID(std::exception)

namespace nau
{
    /**
     */
    struct NAU_KERNEL_EXPORT NAU_ABSTRACT_TYPE Error : virtual IRttiObject,
                                                       virtual std::exception
    {
        NAU_INTERFACE(nau::Error, IRttiObject, std::exception)

        template <typename E>
        using PtrType = std::shared_ptr<E>;

        using Ptr = PtrType<Error>;

        virtual ~Error() = default;

        [[nodiscard]] virtual diag::SourceInfo getSource() const = 0;

        [[nodiscard]] virtual std::string getMessage() const = 0;

        [[nodiscard]] std::string getDiagMessage() const;
    };

    /**
     */
    template <std::derived_from<Error> T = Error>
    class DefaultError : public T
    {
        NAU_RTTI_CLASS(nau::DefaultError<T>, T)

    public:
        DefaultError(const diag::SourceInfo& sourceInfo, std::string message) :
            m_sourceInfo(sourceInfo),
            m_message(std::move(message))
        {
        }

        diag::SourceInfo getSource() const override
        {
            return m_sourceInfo;
        }

        std::string getMessage() const override
        {
            return m_message;
        }

        const char* what() const noexcept(noexcept(std::declval<std::exception>().what())) override
        {
            return this->m_message.c_str();
        }

    private:
        const diag::SourceInfo m_sourceInfo;
        const std::string m_message;
    };

    template <typename T>
    constexpr bool inline IsError = std::is_base_of_v<Error, T>;

    template <typename T>
    concept ErrorConcept = IsError<T>;

    namespace rt_detail
    {
        template <typename>
        struct IsErrorPtrHelper : std::false_type
        {
        };

        template <typename T>
        struct IsErrorPtrHelper<Error::template PtrType<T>> : std::bool_constant<IsError<T>>
        {
        };

    }  // namespace rt_detail

    template <typename T>
    constexpr inline bool IsErrorPtr = rt_detail::IsErrorPtrHelper<T>::value;

    template <std::derived_from<Error> ErrorType>
    struct ErrorFactory
    {
        [[maybe_unused]] const diag::SourceInfo sourceInfo;

        ErrorFactory(const diag::SourceInfo inSourceInfo) :
            sourceInfo(inSourceInfo)
        {
        }

        template <typename... Args>
        inline auto operator()(Args&&... args)
        {
            using ErrorImplType = ErrorType;
            static_assert(!std::is_abstract_v<ErrorImplType>);

            constexpr bool CanConstructWithSourceInfo = std::is_constructible_v<ErrorType, diag::SourceInfo, Args...>;
            constexpr bool CanConstructWithoutSourceInfo = std::is_constructible_v<ErrorType, Args...>;

            static_assert(CanConstructWithSourceInfo || CanConstructWithoutSourceInfo, "Invalid error's constructor arguments");
            static_assert(std::is_convertible_v<ErrorImplType*, ErrorType*>, "Implementation type is not compatible with requested error interface");

            if constexpr (CanConstructWithSourceInfo)
            {
                auto error = std::make_shared<ErrorImplType>(sourceInfo, std::forward<Args>(args)...);
                return std::static_pointer_cast<ErrorType>(std::move(error));
            }
            else
            {
                auto error = std::make_shared<ErrorImplType>(std::forward<Args>(args)...);
                return std::static_pointer_cast<ErrorType>(std::move(error));
            }
        }
    };

    template <>
    struct ErrorFactory<DefaultError<>>
    {
        [[maybe_unused]] const diag::SourceInfo sourceInfo;

        ErrorFactory(const diag::SourceInfo inSourceInfo) :
            sourceInfo(inSourceInfo)
        {
        }

        template <typename StringView, typename... Args>
        requires(std::is_constructible_v<std::string_view, StringView>)
        Error::Ptr operator()(StringView message, Args&&... args)
        {
            std::string_view sview{message};

            std::string formattedMessage;
            if constexpr (sizeof...(Args) == 0)
            {
                formattedMessage.assign(sview.data(), sview.size());
            }
            else
            {
                const std::string text = ::fmt::vformat(fmt::string_view{sview.data(), sview.size()}, fmt::make_format_args<fmt::buffered_context<char>>(args...));
                formattedMessage.assign(text.data(), text.size());
            }

            return std::make_shared<DefaultError<>>(sourceInfo, std::move(formattedMessage));
        }
    };

}  // namespace nau

#define NAU_ABSTRACT_ERROR(ErrorType, ...) NAU_INTERFACE(ErrorType, __VA_ARGS__)

#define NAU_ERROR(ErrorType, ...) NAU_RTTI_CLASS(ErrorType, __VA_ARGS__)

#define NauMakeErrorT(ErrorType) ::nau::ErrorFactory<ErrorType>(NAU_INLINED_SOURCE_INFO)

#define NauMakeError ::nau::ErrorFactory<::nau::DefaultError<>>(NAU_INLINED_SOURCE_INFO)
