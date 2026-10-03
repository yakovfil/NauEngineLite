// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// nau/serialization/serialization.h

#pragma once

#include <fmt/format.h>

#include <string>
#include <string_view>

#include "nau/diag/error.h"
#include "nau/diag/source_info.h"
#include "nau/meta/attribute.h"
#include "nau/utils/result.h"

namespace nau::serialization
{

    /**
     */
    enum class TypeCoercion
    {
        Default,
        Allow,
        Strict
    };

    /**
     */
    class NAU_ABSTRACT_TYPE SerializationError : public DefaultError<>
    {
        NAU_ERROR(nau::serialization::SerializationError, nau::DefaultError<>)
    public:
        SerializationError(const diag::SourceInfo& sourceInfo, std::string message) :
            DefaultError<>(sourceInfo, std::move(message))
        {
        }
    };

    /**
     */
    class RequiredFieldMissedError : public SerializationError
    {
        NAU_ERROR(nau::serialization::RequiredFieldMissedError, SerializationError)

    public:
        RequiredFieldMissedError(diag::SourceInfo sourceInfo, std::string typeName, std::string fieldName) :
            SerializationError(sourceInfo, makeMessage(typeName, fieldName)),
            m_typeName(std::move(typeName)),
            m_fieldName(std::move(fieldName))
        {
        }

        std::string_view getTypeName() const
        {
            return m_typeName;
        }

        std::string_view getFieldName() const
        {
            return m_fieldName;
        }

    private:
        static std::string makeMessage(std::string_view type, std::string_view field)
        {
            std::string message = ::fmt::format("Required field ({}.{}) missed", type, field);
            return std::string{message.data(), message.size()};
        }

        const std::string m_typeName;
        const std::string m_fieldName;
    };

    /**
     */
    class TypeMismatchError : public SerializationError
    {
        NAU_ERROR(nau::serialization::TypeMismatchError, SerializationError)

    public:
        TypeMismatchError(diag::SourceInfo sourceInfo, std::string expectedTypeName, std::string actualTypeName) :
            SerializationError(sourceInfo, makeMessage(expectedTypeName, actualTypeName)),
            m_expectedTypeName(std::move(expectedTypeName)),
            m_actualTypeName(std::move(actualTypeName))
        {
        }

        std::string_view getExpectedTypeName() const
        {
            return m_expectedTypeName;
        }

        std::string_view getActualTypeName() const
        {
            return m_actualTypeName;
        }

    private:
        static std::string makeMessage(std::string_view expectedType, std::string_view actualType)
        {
            std::string message = ::fmt::format("Expected type(category):({}), but:({})", expectedType, actualType);
            return std::string{message.data(), message.size()};
        }

        const std::string m_expectedTypeName;
        const std::string m_actualTypeName;
    };

    /**
     */
    class NumericOverflowError : public SerializationError
    {
        NAU_ERROR(nau::serialization::NumericOverflowError, SerializationError)
    public:
        using SerializationError::SerializationError;

        NumericOverflowError(diag::SourceInfo sourceInfo) :
            SerializationError(sourceInfo, "Numeric Overflow")
        {
        }
    };

    class EndOfStreamError : public SerializationError
    {
        NAU_ERROR(nau::serialization::EndOfStreamError, SerializationError)
    public:
        using SerializationError::SerializationError;

        EndOfStreamError(diag::SourceInfo sourceInfo) :
            SerializationError(sourceInfo, "Unexpected end of stream")
        {
        }
    };

    NAU_DEFINE_ATTRIBUTE(RequiredFieldAttribute, "nau.serialization.required_field", meta::AttributeOptionsNone)
    NAU_DEFINE_ATTRIBUTE(IgnoreEmptyFieldAttribute, "nau.serialization.ignore_empty_field", meta::AttributeOptionsNone)

}  // namespace nau::serialization
