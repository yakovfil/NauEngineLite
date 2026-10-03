// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <concepts>
#include <string_view>
#include <tuple>

#include "nau/meta/class_info.h"
#include "nau/serialization/runtime_value.h"
#include "nau/utils/type_list/contains.h"
#include "nau/utils/type_list/transform.h"
#include "nau/utils/type_tag.h"
#include "nau/utils/type_utility.h"
#include "nau/utils/typed_flag.h"

/**
    Compile time attributes.

    1. Defining attribute key.

    // Defining attribute:
    //  compile time meta-key: MyAttribute_0
    //  runtime filed name (if required): "my.attribute_0.runtime.name"
    //  attribute options flag: AttributeOptionsNone
    //
    NAU_DEFINE_ATTRIBUTE(MyAttribute_0, "my.attribute_0.runtime.name", meta::AttributeOptionsNone)

    NAU_DEFINE_ATTRIBUTE(MyAttribute_1, "my.attribute_1.runtime.name", meta::AttributeOptionsNone)

    NAU_DEFINE_ATTRIBUTE(MyAttribute_2, "my.attribute_2.runtime.name", meta::AttributeOptionsNone)

    2. Declaring attribute for type:
    // using CLASS_ATTRIBUTE(AttributeKey, [AnyValue])

    struct MyAttributeValue
    {};

    class MyClass
    {
        NAU_CLASS_ATTRIBUTES(
            CLASS_ATTRIBUTE(MyAttribute_0, MyAttributeValue{}), //
            CLASS_ATTRIBUTE(MyAttribute_1, 75)
        )
    };

    // can define attributes outside of the class definition
    NAU_CLASS_ATTRIBUTES_EXT(MyClass,
        CLASS_ATTRIBUTE(MyAttribute_3, std::string_view{"attrib_3_value"})
    )

    3. By default, attributes are not inherited. But if this is required, then attribute must specify AttributeOptions::Inherited

    NAU_DEFINE_ATTRIBUTE(MyAttribute_44, "my.attribute_44.runtime.name", meta::AttributeOptions::Inherited)
    //
    // NAU_CLASS_BASE() // must be used to specify bases

    4. Using Attribute at compile time

    template<typename T>
    void someFunc(T& instance)
    {
        using namespace nau;

        if constexpr (meta::AttributeDefined<T, MyAttribute_0>)
        {
            const MyAttributeValue value_0 = meta::getAttributeValue<T, MyAttribute_0>();
            //
        }

        if constexpr (meta::AttributeDefined<T, MyAttribute_1>)
        {
            static_assert(std::is_integral_v<meta::AttributeValueType<T, MyAttribute_1>);

            const int value_1 = static_cast<int>(meta::getAttributeValue<T, MyAttribute_1>());
            //
        }
 */

namespace nau::meta
{
    /**
     */
    enum class AttributeOptions
    {
        Inherited = NauFlag(1)
    };

    NAU_DEFINE_TYPED_FLAG(AttributeOptions)

    constexpr inline AttributeOptionsFlag AttributeOptionsNone = AttributeOptionsFlag{};

    /**
     */
    struct Attribute
    {
        const std::string_view strValue;

        constexpr Attribute(const char value[]) :
            strValue(value)
        {
        }
    };

    /**
     */
    template <std::derived_from<Attribute> K, typename T>
    struct AttributeField
    {
        using Key = K;
        using Value = T;

        T value;

        AttributeField(T&& inValue) :
            value(std::move(value))
        {
        }

        AttributeField(TypeTag<Key>, T&& inValue) :
            value(std::move(inValue))
        {
        }
    };

    template <typename K, typename T>
    AttributeField(TypeTag<K>, T&&) -> AttributeField<K, T>;

}  // namespace nau::meta

namespace nau::meta_detail
{

    template <typename T>
    concept Concept_ClassAttributes = requires(const T& instance) {
        { instance.template nauGetClassAttributes<T>() } -> nau::TemplateOfConcept<std::tuple>;
    };

    template <typename T>
    concept Concept_ClassExternAttributes = requires(const T&) {
        { nauGetClassAttributes(TypeTag<T>{}) } -> nau::TemplateOfConcept<std::tuple>;
    };

    template <typename T>
    concept Concept_AttributeHasOptions = requires {
        { T::Options } -> std::same_as<const meta::AttributeOptionsFlag&>;
    };

    template <std::derived_from<meta::Attribute> T>
    consteval meta::AttributeOptionsFlag getAttributeOptions()
    {
        if constexpr (Concept_AttributeHasOptions<T>)
        {
            return T::Options;
        }
        else
        {
            return meta::AttributeOptionsNone;
        }
    }

    template <Concept_ClassAttributes T>
    auto getClassEmbeddedAttributes()
    {
        const T* const nullInstance = nullptr;
        return nullInstance->template nauGetClassAttributes<T>();
    }

    template <typename T>
    auto getClassEmbeddedAttributes()
    {
        return std::tuple{};
    }

    template <typename T>
    auto getClassExternAttributes()
    {
        if constexpr (Concept_ClassExternAttributes<T>)
        {
            return nauGetClassAttributes(TypeTag<T>{});
        }
        else
        {
            return std::tuple{};
        }
    }

    template <typename T>
    auto getClassDirectAttributes()
    {
        return std::tuple_cat(getClassEmbeddedAttributes<T>(), getClassExternAttributes<T>());
    }

    template <typename, meta::AttributeOptions, size_t... I>
    consteval auto getAttributeFilteredIndices(std::index_sequence<I...> result, std::index_sequence<>)
    {
        return result;
    }

    template <typename Attribs, meta::AttributeOptions Option, size_t... I, size_t Current, size_t... Tail>
    consteval auto getAttributeFilteredIndices(std::index_sequence<I...> result, std::index_sequence<Current, Tail...>)
    {
        using Field = std::tuple_element_t<Current, Attribs>;

        if constexpr (getAttributeOptions<typename Field::Key>().has(Option))
        {
            return getAttributeFilteredIndices<Attribs, Option>(std::index_sequence<I..., Current>{}, std::index_sequence<Tail...>{});
        }
        else
        {
            return getAttributeFilteredIndices<Attribs, Option>(result, std::index_sequence<Tail...>{});
        }
    }

    template <typename T, meta::AttributeOptions Option>
    auto getClassFilteredAttributes()
    {
        auto attributes = getClassDirectAttributes<T>();

        using InputIndices = TupleUtils::Indexes<decltype(attributes)>;
        using SelectedIndices = decltype(getAttributeFilteredIndices<decltype(attributes), Option>(std::index_sequence<>{}, InputIndices{}));

        return []<size_t... I>(auto&& input, std::index_sequence<I...>)
        {
            return std::tuple{std::get<I>(std::move(input))...};
        }(std::move(attributes), SelectedIndices{});
    }

    template <typename T>
    auto getClassAllAttributes()
    {
        return []<typename... Base>(TypeList<Base...>)
        {
            return std::tuple_cat(getClassDirectAttributes<T>(), getClassFilteredAttributes<Base, meta::AttributeOptions::Inherited>()...);
        }(meta::ClassAllUniqueBase<T>{});
    }

    template <typename T>
    using ClassAllAttributesType = decltype(getClassAllAttributes<T>());

    template <typename AField, typename Key, int Index>
    consteval ConstIndex attribIdx()
    {
        return std::is_same_v<typename AField::Key, Key> ? Index : ConstIndex::NotIndex;
    }

    template <typename AttribsTuple, typename Key>
    consteval ConstIndex findAttribIndexHelper(std::index_sequence<>)
    {
        static_assert(std::is_same_v<std::tuple<>, AttribsTuple>);
        return ConstIndex::NotIndex;
    }

    template <typename AttribsTuple, typename Key, size_t... Index>
    consteval ConstIndex findAttribIndexHelper(std::index_sequence<Index...>)
    {
        return (attribIdx<std::tuple_element_t<Index, AttribsTuple>, Key, Index>() || ...);
    }

    template <typename AttribsTuple, std::derived_from<meta::Attribute> Key>
    consteval ConstIndex findAttribIndex()
    {
        using Indexes = std::make_index_sequence<std::tuple_size_v<AttribsTuple>>;
        return findAttribIndexHelper<AttribsTuple, Key>(Indexes{});
    }

    /**
     */
    template <typename T, std::derived_from<meta::Attribute> Key>
    inline constexpr bool AttributeDefined = findAttribIndex<ClassAllAttributesType<T>, Key>().value >= 0;

    template <typename T, std::derived_from<meta::Attribute> Key>
    auto getAttributeField()
    {
        auto attributes = getClassAllAttributes<T>();

        constexpr ConstIndex AttribIndex = findAttribIndex<decltype(attributes), Key>();
        static_assert(AttribIndex.value >= 0, "Requested attribute not defined for specified type");

        return std::get<AttribIndex>(attributes);
    }

}  // namespace nau::meta_detail

namespace nau::meta
{
    template <typename T, std::derived_from<meta::Attribute> Key>
    inline constexpr bool AttributeDefined = meta_detail::AttributeDefined<T, Key>;

    template <typename T, typename A>
    auto getAttributeValue()
    {
        using namespace nau::meta_detail;
        return getAttributeField<T, A>().value;
    }

    template <typename T, typename A>
    using AttributeValueType = decltype(getAttributeValue<T, A>());

    template <typename T>
    auto getClassAllAttributes()
    {
        return meta_detail::getClassAllAttributes<T>();
    }

}  // namespace nau::meta

#define NAU_DEFINE_ATTRIBUTE(AttributeKeyType, AttributeKeyString, ...)          \
    struct AttributeKeyType : nau::meta::Attribute                               \
    {                                                                            \
        [[maybe_unused]]                                                         \
        static inline constexpr meta::AttributeOptionsFlag Options{__VA_ARGS__}; \
        constexpr AttributeKeyType() :                                           \
            nau::meta::Attribute{AttributeKeyString}                             \
        {                                                                        \
        }                                                                        \
    };

// clang-format off
#define NAU_DEFINE_ATTRIBUTE_(AttributeKeyType) NAU_DEFINE_ATTRIBUTE(AttributeKeyType, #AttributeKeyType, nau::meta::AttributeOptionsNone)

#define CLASS_ATTRIBUTE(Attribute, Value)  ::nau::meta::AttributeField {nau::TypeTag<Attribute>{}, Value }

// clang-format on

#define NAU_CLASS_ATTRIBUTES(...)                                                       \
public:                                                                                 \
    template <typename InstanceT>                                                       \
    auto nauGetClassAttributes() const                                                  \
    {                                                                                   \
        using ThisType = std::remove_const_t<std::remove_reference_t<decltype(*this)>>; \
        if constexpr (std::is_same_v<InstanceT, ThisType>)                              \
        {                                                                               \
            std::tuple attributes{                                                      \
                __VA_ARGS__};                                                           \
                                                                                        \
            return attributes;                                                          \
        }                                                                               \
        else                                                                            \
        {                                                                               \
            return unsigned{0};                                                         \
        }                                                                               \
    }

#define NAU_CLASS_ATTRIBUTES_EXT(Type, ...)                 \
    [[maybe_unused]]                                        \
    inline auto nauGetClassAttributes(::nau::TypeTag<Type>) \
    {                                                       \
        return std::tuple{                                  \
            __VA_ARGS__};                                   \
    }
