// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <compare>
#include <concepts>
#include <functional>
#include <memory>
#include <string_view>
#include <type_traits>
#include <vector>

#include "nau/kernel/kernel_config.h"
#include "nau/meta/class_info.h"
#include "nau/string/hash.h"
#include "nau/utils/type_tag.h"

#if defined(_MSC_VER)
    #define NAU_ABSTRACT_TYPE __declspec(novtable)
#else
    #define NAU_ABSTRACT_TYPE
#endif

namespace nau::rtti_detail
{
    struct TypeId
    {
        size_t typeId = 0;
        std::string_view typeName;  // TODO: possible typeName will be excluded from release/prod builds

        template <std::size_t N>
        constexpr TypeId(const char (&name)[N]) :
            typeId{strings::constHash(name)},
            typeName{name}
        {
        }

        constexpr TypeId(size_t tid) :
            typeId{tid}
        {
        }

        constexpr TypeId() = default;
        constexpr TypeId(const TypeId&) = default;
        constexpr TypeId& operator=(const TypeId&) = default;

        constexpr explicit operator bool() const noexcept
        {
            return typeId != 0;
        }

        constexpr std::string_view getTypeName() const noexcept
        {
            return typeName;
        }

        constexpr friend inline bool operator==(TypeId left, TypeId right) noexcept
        {
            return left.typeId == right.typeId;
        }

        constexpr friend inline bool operator<(TypeId left, TypeId right) noexcept
        {
            return left.typeId < right.typeId;
        }
    };

    template <typename T>
    struct DeclaredTypeId : std::false_type
    {
    };

    template <typename T>
    concept Concept_RttiTypeId = requires {
        { T::NauRtti_TypeId } -> std::same_as<const TypeId&>;
    };

    template <typename T>
    requires(Concept_RttiTypeId<T> || Concept_RttiTypeId<DeclaredTypeId<T>>)
    constexpr inline TypeId getTypeId()
    {
        if constexpr (Concept_RttiTypeId<T>)
        {
            return T::NauRtti_TypeId;
        }
        else
        {
            static_assert(DeclaredTypeId<T>::value, "TypeId is not declared");
            return DeclaredTypeId<T>::NauRtti_TypeId;
        }
    }

}  // namespace nau::rtti_detail

#define NAU_TYPEID(TypeName)                                                                \
public:                                                                                     \
    static_assert(std::is_trivial_v<::nau::TypeTag<TypeName>>, "Check actual type exists"); \
    [[maybe_unused]]                                                                        \
    static constexpr inline const ::nau::rtti_detail::TypeId NauRtti_TypeId{                \
        #TypeName};

#define NAU_DECLARE_TYPEID(TypeName)                                                            \
    namespace nau::rtti_detail                                                                  \
    {                                                                                           \
        template <>                                                                             \
        struct DeclaredTypeId<TypeName> : std::true_type                                        \
        {                                                                                       \
            static constexpr inline const ::nau::rtti_detail::TypeId NauRtti_TypeId{#TypeName}; \
        };                                                                                      \
    }

namespace nau::rtti
{
    class TypeInfo;

    template <typename T>
    inline constexpr bool HasTypeInfo = rtti_detail::Concept_RttiTypeId<T> || rtti_detail::DeclaredTypeId<T>::value;

    template <typename T>
    concept WithTypeInfo = HasTypeInfo<T>;

    template <typename T>
    concept ClassWithTypeInfo = HasTypeInfo<T> && !std::is_abstract_v<T>;

    template <typename T>
    const TypeInfo& getTypeInfo();
    /*

    */
    class [[nodiscard]] TypeInfo
    {
    public:
        TypeInfo() :
            m_typeId(0)
        {
        }
        TypeInfo(const TypeInfo&) = default;
        TypeInfo& operator=(const TypeInfo&) = default;

        constexpr inline size_t getHashCode() const
        {
            return m_typeId.typeId;
        }

        constexpr inline std::string_view getTypeName() const
        {
            return m_typeId.getTypeName();
        }

        constexpr explicit operator bool() const noexcept
        {
            return static_cast<bool>(m_typeId);
        }

    private:
        template <typename T>
        static TypeInfo makeTypeInfo()
        requires(HasTypeInfo<T>)
        {
            return TypeInfo{rtti_detail::getTypeId<T>()};
        }

        template <std::size_t N>
        constexpr TypeInfo(const char (&typeName)[N]) :
            m_typeId(typeName)
        {
        }

        constexpr TypeInfo(const rtti_detail::TypeId typeId) :
            m_typeId(typeId)
        {
        }

        rtti_detail::TypeId m_typeId;

        template <typename T>
        friend const TypeInfo& getTypeInfo();

        friend TypeInfo makeTypeInfoFromId(size_t typeId);
        friend TypeInfo makeTypeInfoFromName(const char* name);

        friend inline bool operator==(const TypeInfo& t1, const TypeInfo& t2) noexcept
        {
            return t1.m_typeId == t2.m_typeId;
        }

        friend inline bool operator<(const TypeInfo& t1, const TypeInfo& t2) noexcept
        {
            return t1.m_typeId < t2.m_typeId;
        }
    };

    /*
     */
    class NAU_KERNEL_EXPORT TypeIndex
    {
    public:
        template <typename T>
        requires(HasTypeInfo<T>)
        static TypeIndex of()
        {
            return TypeIndex{getTypeInfo<T>()};
        }

        TypeIndex(const TypeInfo& typeInfo) noexcept;
        TypeIndex(const TypeIndex&) noexcept;
        TypeIndex& operator=(const TypeIndex&) noexcept;

        bool operator==(const TypeIndex& other) const;

        std::strong_ordering operator<=>(const TypeIndex& other) const;

        size_t getHashCode() const
        {
            return m_typeInfo->getHashCode();
        }

        const TypeInfo& getType() const;

    private:
        const TypeInfo* m_typeInfo;
    };

    /*
     */
    template <typename T>
    const TypeInfo& getTypeInfo()
    {
        static_assert(rtti::HasTypeInfo<T>, "Nau TypeId not found for type");
        static const TypeInfo typeInfo = TypeInfo::makeTypeInfo<T>();

        return (typeInfo);
    }

    inline TypeInfo makeTypeInfoFromId(size_t typeId)
    {
        return rtti_detail::TypeId{typeId};
    }

    inline TypeInfo makeTypeInfoFromName(const char* name)
    {
        return rtti_detail::TypeId{nau::strings::constHash(name)};
    }

    template <rtti::WithTypeInfo T, rtti::WithTypeInfo... U>
    inline std::vector<rtti::TypeInfo> makeTypeInfoCollection()
    {
        return {
            rtti::getTypeInfo<T>(),
            rtti::getTypeInfo<U>()...};
    }

}  // namespace nau::rtti

template <>
struct std::hash<::nau::rtti::TypeIndex>
{
    [[nodiscard]]
    size_t operator()(const ::nau::rtti::TypeIndex& val) const
    {
        return val.getHashCode();
    }
};
