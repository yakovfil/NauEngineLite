// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <array>
#include <span>
#include <type_traits>

#include "nau/kernel/kernel_config.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/rtti/weak_ptr.h"
#include "nau/serialization/native_runtime_value/native_value_base.h"
#include "nau/serialization/native_runtime_value/native_value_forwards.h"
#include "nau/string/string_utils.h"

namespace nau::ser_detail
{
    /**
     */
    class RuntimeFieldAccessor
    {
    public:
        using FieldFactory = RuntimeValue::Ptr (*)(const RuntimeValue& runtimeValue, void* objectPtr, const void* fieldInfoPtr) noexcept;

        RuntimeFieldAccessor(std::string_view name, const void* field, FieldFactory factory) :
            m_name(name),
            m_fieldMetaInfo(field),
            m_factory(factory)
        {
        }

        std::string_view getName() const
        {
            return m_name;
        }

        RuntimeValue::Ptr getRuntimeValue(const RuntimeValue& parent, void* obj) const
        {
            return m_factory(parent, obj, m_fieldMetaInfo);
        }

    private:
        const std::string_view m_name;
        const void* const m_fieldMetaInfo;
        const FieldFactory m_factory;
    };

    /**
     */
    class NAU_KERNEL_EXPORT NAU_ABSTRACT_TYPE RuntimeObjectState
    {
    public:
        virtual ~RuntimeObjectState() = default;

        virtual size_t getSize() const = 0;

        std::string_view getKey(size_t index) const;

        RuntimeValue::Ptr getValue(const RuntimeValue& parent, const void* obj, std::string_view key) const;

        bool containsKey(std::string_view key) const;

        Result<> setFieldValue(const RuntimeValue& parent, const void* obj, std::string_view key, const RuntimeValue::Ptr& value);

    protected:
        virtual std::span<RuntimeFieldAccessor> getFields() const = 0;

    private:
        RuntimeFieldAccessor* findField(std::string_view key) const;
    };

    /**
     */
    template <typename T>
    class RuntimeObjectStateImpl final : public RuntimeObjectState
    {
        using FieldFactory = RuntimeFieldAccessor::FieldFactory;
        using FieldsInfo = decltype(meta::getClassAllFields<T>());
        static constexpr size_t FieldsCount = std::tuple_size_v<FieldsInfo>;

        using FieldsArray = std::array<RuntimeFieldAccessor, FieldsCount>;

    public:
        RuntimeObjectStateImpl() :
            m_fieldInfos(meta::getClassAllFields<T>()),
            m_fields(RuntimeObjectStateImpl<T>::makeFields(m_fieldInfos))
        {
        }

        inline size_t getSize() const override
        {
            return RuntimeObjectStateImpl<T>::FieldsCount;
        }

    private:
        std::span<RuntimeFieldAccessor> getFields() const override
        {
            return {m_fields.data(), m_fields.size()};
        }

        template <typename FieldMetaInfo>
        static RuntimeFieldAccessor makeField(const FieldMetaInfo& fieldMetaInfo)
        {
            FieldFactory factory = [](const RuntimeValue& parent, void* objPtr, const void* fieldPtr) noexcept -> RuntimeValue::Ptr
            {
                NAU_FATAL(objPtr);
                NAU_FATAL(fieldPtr);

                using FieldDefinitionClass = typename FieldMetaInfo::Class;
                auto& obj = *reinterpret_cast<T*>(objPtr);

                const FieldMetaInfo* const fieldMetaInfo = reinterpret_cast<const FieldMetaInfo*>(fieldPtr);

                decltype(auto) fieldValue = fieldMetaInfo->getValue(static_cast<FieldDefinitionClass&>(obj));
                static_assert(std::is_reference_v<decltype(fieldValue)>);

                auto fieldRuntimeValue = makeValueRef(fieldValue);

                if (auto* const childValue = fieldRuntimeValue->template as<ser_detail::NativeChildValue*>())
                {
                    if (auto* const parentValue = parent.template as<const NativeParentValue*>())
                    {
                        childValue->setParent(parentValue->getThisMutabilityGuard());
                    }
                }

                return fieldRuntimeValue;
            };

            return RuntimeFieldAccessor(fieldMetaInfo.getName(), reinterpret_cast<const void*>(&fieldMetaInfo), factory);
        }

        template <typename... FieldMetaInfo>
        static FieldsArray makeFields(const std::tuple<FieldMetaInfo...>& fieldsMeta)
        {
            const auto makeFieldsHelper = [&fieldsMeta]<size_t... Index>(std::index_sequence<Index...>)
            {
                return std::array<RuntimeFieldAccessor, sizeof...(Index)>{makeField(std::get<Index>(fieldsMeta))...};
            };

            return makeFieldsHelper(TupleUtils::Indexes<decltype(fieldsMeta)>{});
        }

        FieldsInfo m_fieldInfos;
        mutable FieldsArray m_fields;
    };

    /**
     */
    template <typename T>
    class NativeObject : public ser_detail::NativeRuntimeValueBase<RuntimeObject>,
                         public RuntimeNativeValue
    {
        using Base = ser_detail::NativeRuntimeValueBase<RuntimeObject>;
        using ValueType = std::decay_t<T>;

        NAU_CLASS_(NativeObject<T>, Base, RuntimeNativeValue)

    public:
        static inline constexpr bool IsMutable = !std::is_const_v<std::remove_reference_t<T>>;
        static inline constexpr bool IsReference = std::is_lvalue_reference_v<T>;

        NativeObject(T obj)
        requires(IsReference)
            :
            m_object(obj)
        {
        }

        NativeObject(const ValueType& obj)
        requires(!IsReference && std::is_copy_constructible_v<ValueType>)
            :
            m_object(obj)
        {
        }

        NativeObject(ValueType&& obj)
        requires(!IsReference && std::is_move_constructible_v<ValueType>)
            :
            m_object(std::move(obj))
        {
        }

        bool isMutable() const override
        {
            return IsMutable;
        }

        size_t getSize() const override
        {
            return m_state.getSize();
        }

        std::string_view getKey(size_t index) const override
        {
            return m_state.getKey(index);
        }

        RuntimeValue::Ptr getValue(std::string_view key) override
        {
            return m_state.getValue(*this, &m_object, key);
        }

        bool containsKey(std::string_view key) const override
        {
            return m_state.containsKey(key);
        }

        Result<> setValue(std::string_view key, const RuntimeValue::Ptr& value) override
        {
            if constexpr (!IsMutable)
            {
                return NauMakeError("Object is non mutable");
            }
            else
            {
                value_changes_scope;

                return m_state.setFieldValue(*this, &m_object, key, value);
            }
        }

        std::optional<FieldInfo> findFieldInfo(std::string_view) const override
        {
            return std::nullopt;
        }

        const rtti::TypeInfo* getValueTypeInfo() const override
        {
            if constexpr (rtti::HasTypeInfo<ValueType>)
            {
                return &rtti::getTypeInfo<ValueType>();
            }

            return nullptr;
        }

        const void* getReadonlyValuePtr() const override
        {
            return reinterpret_cast<const ValueType*>(&m_object);
        }

        void* getValuePtr() override
        {
            NAU_ASSERT(isMutable());
            if constexpr (IsMutable)
            {
                return reinterpret_cast<ValueType*>(&m_object);
            }

            return nullptr;
        }

    private:
        T m_object;
        ser_detail::RuntimeObjectStateImpl<ValueType> m_state;
    };
}  // namespace nau::ser_detail

namespace nau
{
    template <NauClassWithFields T>
    RuntimeObject::Ptr makeValueRef(T& obj, IMemAllocator::Ptr allocator)
    {
        using Object = ser_detail::NativeObject<T&>;

        return rtti::createInstanceWithAllocator<Object>(std::move(allocator), obj);
    }

    template <NauClassWithFields T>
    RuntimeObject::Ptr makeValueRef(const T& obj, IMemAllocator::Ptr allocator)
    {
        using Object = ser_detail::NativeObject<const T&>;

        return rtti::createInstanceWithAllocator<Object>(std::move(allocator), obj);
    }

    template <NauClassWithFields T>
    RuntimeObject::Ptr makeValueCopy(const T& obj, IMemAllocator::Ptr allocator)
    {
        using Object = ser_detail::NativeObject<T>;

        return rtti::createInstanceWithAllocator<Object>(std::move(allocator), obj);
    }

    template <NauClassWithFields T>
    RuntimeObject::Ptr makeValueCopy(T&& obj, IMemAllocator::Ptr allocator)
    {
        using Object = ser_detail::NativeObject<T>;

        return rtti::createInstanceWithAllocator<Object>(std::move(allocator), std::move(obj));
    }
}  // namespace nau
