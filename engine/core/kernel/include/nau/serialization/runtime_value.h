// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "nau/diag/assertion.h"
#include "nau/kernel/kernel_config.h"
#include "nau/memory/mem_allocator.h"
#include "nau/rtti/ptr.h"
#include "nau/rtti/rtti_object.h"
#include "nau/string/format.h"
#include "nau/string/string_conv.h"
#include "nau/utils/result.h"
#include "nau/utils/typed_flag.h"

namespace nau
{

    enum class ValueAssignOption
    {
        MergeCollection = NauFlag(0)
    };

    NAU_DEFINE_TYPED_FLAG(ValueAssignOption)

    /**

    */
    struct NAU_ABSTRACT_TYPE RuntimeValue : virtual IRefCounted
    {
        NAU_INTERFACE(nau::RuntimeValue, IRefCounted)

        using Ptr = nau::Ptr<RuntimeValue>;

        NAU_KERNEL_EXPORT
        static Result<> assign(RuntimeValue::Ptr dst, RuntimeValue::Ptr src, ValueAssignOptionFlag option = {});

        virtual ~RuntimeValue() = default;

        virtual bool isMutable() const = 0;
    };

    /**

    */
    struct NAU_ABSTRACT_TYPE RuntimeValueRef : virtual RuntimeValue
    {
        NAU_INTERFACE(nau::RuntimeValueRef, RuntimeValue)

        using Ptr = nau::Ptr<RuntimeValueRef>;

        NAU_KERNEL_EXPORT
        static RuntimeValueRef::Ptr create(RuntimeValue::Ptr&, IMemAllocator::Ptr = nullptr);

        NAU_KERNEL_EXPORT
        static RuntimeValueRef::Ptr create(std::reference_wrapper<const RuntimeValue::Ptr>, IMemAllocator::Ptr = nullptr);

        virtual void set(RuntimeValue::Ptr) = 0;

        virtual RuntimeValue::Ptr get() const = 0;
    };

    /**
     */
    struct NAU_ABSTRACT_TYPE RuntimePrimitiveValue : virtual RuntimeValue
    {
        NAU_INTERFACE(nau::RuntimePrimitiveValue, RuntimeValue)

        using Ptr = nau::Ptr<RuntimePrimitiveValue>;
    };

    /**
     */
    struct NAU_ABSTRACT_TYPE RuntimeStringValue : virtual RuntimePrimitiveValue
    {
        NAU_INTERFACE(nau::RuntimeStringValue, RuntimePrimitiveValue)

        using Ptr = nau::Ptr<RuntimeStringValue>;

        /*
         */
        virtual Result<> setString(std::string_view) = 0;

        /*
         */
        virtual std::string getString() const = 0;
    };

    /**
     */
    struct NAU_ABSTRACT_TYPE RuntimeIntegerValue : virtual RuntimePrimitiveValue
    {
        NAU_INTERFACE(nau::RuntimeIntegerValue, RuntimePrimitiveValue)

        using Ptr = nau::Ptr<RuntimeIntegerValue>;

        /*
         */
        virtual bool isSigned() const = 0;

        /*
         */
        virtual size_t getBitsCount() const = 0;

        /*
         */
        virtual void setInt64(int64_t) = 0;

        /*
         */
        virtual void setUint64(uint64_t) = 0;

        /*
         */
        virtual int64_t getInt64() const = 0;

        /*
         */
        virtual uint64_t getUint64() const = 0;

        template <typename T>
        inline void set(T value)
        requires(std::is_integral_v<T>)
        {
            if constexpr (std::is_signed_v<T>)
            {
                this->setInt64(static_cast<int64_t>(value));
            }
            else
            {
                this->setUint64(static_cast<uint64_t>(value));
            }
        }

        template <typename T>
        T get() const
        requires(std::is_integral_v<T>)
        {
            if constexpr (std::is_signed_v<T>)
            {
                const int64_t value = this->getInt64();
                NAU_ASSERT(static_cast<int64_t>(std::abs(value) <= std::numeric_limits<T>::max()), "Integer overflow");

                return static_cast<T>(value);
            }
            else
            {
                const uint64_t value = this->getUint64();
                NAU_ASSERT(static_cast<uint64_t>(value < std::numeric_limits<T>::max()), "Integer overflow");

                return static_cast<T>(value);
            }
        }
    };

    /**
     */
    struct NAU_ABSTRACT_TYPE RuntimeFloatValue : virtual RuntimePrimitiveValue
    {
        NAU_INTERFACE(nau::RuntimeFloatValue, RuntimePrimitiveValue)

        using Ptr = nau::Ptr<RuntimeFloatValue>;

        virtual size_t getBitsCount() const = 0;

        virtual void setDouble(double) = 0;

        virtual void setSingle(float) = 0;

        virtual double getDouble() const = 0;

        virtual float getSingle() const = 0;

        template <typename T>
        T get() const
        requires(std::is_arithmetic_v<T>)
        {
            return static_cast<T>(this->getDouble());
        }

        template <typename T>
        void set(T value)
        requires(std::is_arithmetic_v<T>)
        {
            return this->setDouble(static_cast<double>(value));
        }
    };

    /**
     */
    struct NAU_ABSTRACT_TYPE RuntimeBooleanValue : virtual RuntimePrimitiveValue
    {
        NAU_INTERFACE(nau::RuntimeBooleanValue, RuntimePrimitiveValue)

        using Ptr = nau::Ptr<RuntimeBooleanValue>;

        virtual void setBool(bool) = 0;

        virtual bool getBool() const = 0;
    };

    /**

    */
    struct NAU_ABSTRACT_TYPE RuntimeOptionalValue : virtual RuntimeValue
    {
        NAU_INTERFACE(nau::RuntimeOptionalValue, RuntimeValue)

        using Ptr = nau::Ptr<RuntimeOptionalValue>;

        virtual bool hasValue() const = 0;

        virtual RuntimeValue::Ptr getValue() = 0;

        virtual Result<> setValue(RuntimeValue::Ptr value = nullptr) = 0;

        inline void reset()
        {
            [[maybe_unused]] auto res = this->setValue(nullptr);
            NAU_ASSERT(res);
            if (res.isError())
            {
                // Halt(std::format("Fail to reset optional:", res.getError()->Message()));
            }
        }
    };

    /**
     */
    struct NAU_ABSTRACT_TYPE RuntimeReadonlyCollection : virtual RuntimeValue
    {
        NAU_INTERFACE(nau::RuntimeReadonlyCollection, RuntimeValue)

        using Ptr = nau::Ptr<RuntimeReadonlyCollection>;

        virtual size_t getSize() const = 0;

        virtual RuntimeValue::Ptr getAt(size_t index) = 0;

        virtual Result<> setAt(size_t index, const RuntimeValue::Ptr& value) = 0;

        RuntimeValue::Ptr operator[](size_t index)
        {
            return getAt(index);
        }
    };

    /**
     */
    struct NAU_ABSTRACT_TYPE RuntimeCollection : virtual RuntimeReadonlyCollection
    {
        NAU_INTERFACE(nau::RuntimeCollection, RuntimeReadonlyCollection)

        using Ptr = nau::Ptr<RuntimeCollection>;

        virtual void clear() = 0;

        virtual void reserve(size_t) = 0;

        virtual Result<> append(const RuntimeValue::Ptr&) = 0;
    };

    /**
     */
    struct NAU_ABSTRACT_TYPE RuntimeReadonlyDictionary : virtual RuntimeValue
    {
        NAU_INTERFACE(nau::RuntimeReadonlyDictionary, RuntimeValue)

        using Ptr = nau::Ptr<RuntimeReadonlyDictionary>;

        virtual size_t getSize() const = 0;

        virtual std::string_view getKey(size_t index) const = 0;

        virtual RuntimeValue::Ptr getValue(std::string_view) = 0;

        virtual Result<> setValue(std::string_view, const RuntimeValue::Ptr& value) = 0;

        virtual bool containsKey(std::string_view) const = 0;

        RuntimeValue::Ptr operator[](std::string_view key)
        {
            return getValue(key);
        }

        inline std::pair<std::string_view, RuntimeValue::Ptr> operator[](size_t index)
        {
            auto key = getKey(index);
            return {key, getValue(key)};
        }
    };

    /**
     *
     */
    struct NAU_ABSTRACT_TYPE RuntimeDictionary : virtual RuntimeReadonlyDictionary
    {
        NAU_INTERFACE(nau::RuntimeDictionary, RuntimeReadonlyDictionary)

        using Ptr = nau::Ptr<RuntimeDictionary>;

        virtual void clear() = 0;

        virtual RuntimeValue::Ptr erase(std::string_view) = 0;
    };

    /**
        Generalized object runtime representation.
    */
    struct NAU_ABSTRACT_TYPE RuntimeObject : virtual RuntimeReadonlyDictionary
    {
        NAU_INTERFACE(nau::RuntimeObject, RuntimeReadonlyDictionary)

        using Ptr = nau::Ptr<RuntimeObject>;

        struct FieldInfo
        {
        };

        virtual std::optional<FieldInfo> findFieldInfo(std::string_view) const = 0;

        inline Result<> setFieldValue(std::string_view key, const RuntimeValue::Ptr& value)
        {
            return setValue(key, value);
        }
    };

    /**
     */
    // struct NAU_ABSTRACT_TYPE RuntimeCustomValue : virtual RuntimeValue
    // {
    //     NAU_INTERFACE(nau::RuntimeCustomValue, RuntimeValue)

    //     virtual Result<> restore(const RuntimeValue&) = 0;

    //     virtual Result<> store(RuntimeValue&) const = 0;
    // };

    /**
     */
    struct NAU_ABSTRACT_TYPE RuntimeNativeValue : virtual RuntimeValue  // virtual RuntimeCustomValue
    {
        NAU_INTERFACE(nau::RuntimeNativeValue, RuntimeValue)

        virtual const rtti::TypeInfo* getValueTypeInfo() const = 0;

        virtual const void* getReadonlyValuePtr() const = 0;

        virtual void* getValuePtr() = 0;

        template <typename T>
        const T& getReadonlyRef() const
        {
            if constexpr (rtti::HasTypeInfo<T>)
            {
                [[maybe_unused]]
                const auto* const type = getValueTypeInfo();
                NAU_ASSERT(*type == rtti::getTypeInfo<T>());
            }

            const void* const valuePtr = getReadonlyValuePtr();
            NAU_ASSERT(valuePtr);

            return *reinterpret_cast<const T*>(valuePtr);
        }

        template <typename T>
        T& getRef()
        {
            if constexpr (rtti::HasTypeInfo<T>)
            {
                [[maybe_unused]]
                const auto* const type = getValueTypeInfo();
                NAU_ASSERT(*type == rtti::getTypeInfo<T>());
            }

            void* const valuePtr = getValuePtr();
            NAU_ASSERT(valuePtr);

            return *reinterpret_cast<T*>(valuePtr);
        }
    };

}  // namespace nau
