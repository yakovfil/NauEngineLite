// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <list>
#include <type_traits>

#include "nau/rtti/rtti_impl.h"
#include "nau/serialization/native_runtime_value/native_value_base.h"
#include "nau/serialization/native_runtime_value/native_value_forwards.h"

namespace nau::ser_detail
{
    /**
     */
    template <typename T>
    class VectorLikeNativeCollection final : public ser_detail::NativeRuntimeValueBase<RuntimeCollection>
    {
        using Base = ser_detail::NativeRuntimeValueBase<RuntimeCollection>;
        using ContainerType = std::decay_t<T>;

        NAU_CLASS_(VectorLikeNativeCollection<T>, Base)

    public:
        static_assert(LikeStdVector<ContainerType>);
        static constexpr bool IsMutable = !std::is_const_v<std::remove_reference_t<T>>;
        static constexpr bool IsReference = std::is_lvalue_reference_v<T>;

        VectorLikeNativeCollection(const ContainerType& inCollection)
        requires(!IsReference)
            :
            m_collection(inCollection)
        {
        }

        VectorLikeNativeCollection(ContainerType&& inCollection)
        requires(!IsReference)
            :
            m_collection(std::move(inCollection))
        {
        }

        VectorLikeNativeCollection(T inCollection)
        requires(IsReference)
            :
            m_collection(inCollection)
        {
        }

        bool isMutable() const override
        {
            return IsMutable;
        }

        size_t getSize() const override
        {
            return m_collection.size();
        }

        RuntimeValue::Ptr getAt(size_t index) override
        {
            NAU_ASSERT(index < m_collection.size());

            return this->makeChildValue(makeValueRef(m_collection[index]));
        }

        Result<> setAt(size_t index, const RuntimeValue::Ptr& value) override
        {
            NAU_ASSERT(value);
            NAU_ASSERT(index < m_collection.size());

            return RuntimeValue::assign(makeValueRef(m_collection[index]), value);
        }

        void clear() override
        {
            if constexpr (IsMutable)
            {
                NAU_FATAL(!this->hasChildren(), "Attempt to modify Runtime Collection while there is still referenced children");

                value_changes_scope;
                m_collection.clear();
            }
            else
            {
                NAU_FAILURE("Attempt to modify non mutable array value");
            }
        }

        void reserve(size_t capacity) override
        {
            if constexpr (IsMutable)
            {
                NAU_FATAL(!this->hasChildren(), "Attempt to modify Runtime Collection while there is still referenced children");
                m_collection.reserve(capacity);
            }
            else
            {
                NAU_FAILURE("Can not reserve for non mutable array");
            }
        }

        Result<> append(const RuntimeValue::Ptr& value) override
        {
            if constexpr (!IsMutable)
            {
                NAU_FAILURE("Attempt to modify non mutable array");
                return NauMakeError("Attempt to modify non mutable value");
            }
            else
            {
                NAU_FATAL(!this->hasChildren(), "Attempt to modify Runtime Collection while there is still referenced children");

                value_changes_scope;

                m_collection.emplace_back();
                decltype(auto) newElement = m_collection.back();
                return RuntimeValue::assign(makeValueRef(newElement), value);
            }
        }

    private:
        T m_collection;
    };

    /**
     */
    template <typename T>
    class ListLikeNativeCollection final : public ser_detail::NativeRuntimeValueBase<RuntimeCollection>
    {
        using Base = ser_detail::NativeRuntimeValueBase<RuntimeCollection>;
        using ContainerType = std::decay_t<T>;

        NAU_CLASS_(ListLikeNativeCollection<T>, Base)

    public:
        static_assert(LikeStdList<ContainerType>);
        static constexpr bool IsMutable = !std::is_const_v<std::remove_reference_t<T>>;
        static constexpr bool IsReference = std::is_lvalue_reference_v<T>;

        ListLikeNativeCollection(const ContainerType& inCollection)
        requires(!IsReference)
            :
            m_collection(inCollection)
        {
        }

        ListLikeNativeCollection(ContainerType&& inCollection)
        requires(!IsReference)
            :
            m_collection(std::move(inCollection))
        {
        }

        ListLikeNativeCollection(T inCollection)
        requires(IsReference)
            :
            m_collection(inCollection)
        {
        }

        bool isMutable() const override
        {
            return IsMutable;
        }

        size_t getSize() const override
        {
            return m_collection.size();
        }

        RuntimeValue::Ptr getAt(size_t index) override
        {
            NAU_ASSERT(index < m_collection.size(), "[{}], size():{}", index, m_collection.size());

            auto element = m_collection.begin();
            std::advance(element, index);

            return this->makeChildValue(makeValueRef(*element));
        }

        Result<> setAt(size_t index, const RuntimeValue::Ptr& value) override
        {
            NAU_ASSERT(value);
            NAU_ASSERT(index < m_collection.size(), "[{}], size():{}", index, m_collection.size());

            auto element = m_collection.begin();
            std::advance(element, index);

            return RuntimeValue::assign(makeValueRef(*element), value);
        }

        void clear() override
        {
            if constexpr (IsMutable)
            {
                NAU_FATAL(!this->hasChildren(), "Attempt to modify Runtime Collection while there is still referenced children");

                value_changes_scope;
                m_collection.clear();
            }
            else
            {
                NAU_FAILURE("Attempt to modify non mutable array value");
            }
        }

        void reserve([[maybe_unused]] size_t) override
        {
            if constexpr (!IsMutable)
            {
                NAU_FAILURE("Can not reserve for non mutable array");
            }
        }

        Result<> append(const RuntimeValue::Ptr& value) override
        {
            if constexpr (!IsMutable)
            {
                NAU_FAILURE("Attempt to modify non mutable value");
                return NauMakeError("Attempt to modify non mutable value");
            }
            else
            {
                value_changes_scope;

                m_collection.emplace_back();
                decltype(auto) newElement = m_collection.back();
                return RuntimeValue::assign(makeValueRef(newElement), value);
            }
        }

    private:
        T m_collection;
    };

    template <typename T>
    class SetLikeNativeCollection final : public ser_detail::NativeRuntimeValueBase<RuntimeCollection>
    {
        using Base = ser_detail::NativeRuntimeValueBase<RuntimeCollection>;
        using ContainerType = std::decay_t<T>;

        NAU_CLASS_(SetLikeNativeCollection<T>, Base)

    public:
        static_assert(LikeSet<ContainerType>);
        static constexpr bool IsMutable = !std::is_const_v<std::remove_reference_t<T>>;
        static constexpr bool IsReference = std::is_lvalue_reference_v<T>;

        SetLikeNativeCollection(const ContainerType& inCollection)
        requires(!IsReference)
            :
            m_collection(inCollection)
        {
        }

        SetLikeNativeCollection(ContainerType&& inCollection)
        requires(!IsReference)
            :
            m_collection(std::move(inCollection))
        {
        }

        SetLikeNativeCollection(T inCollection)
        requires(IsReference)
            :
            m_collection(inCollection)
        {
        }

        bool isMutable() const override
        {
            return IsMutable;
        }

        size_t getSize() const override
        {
            return m_collection.size();
        }

        RuntimeValue::Ptr getAt(size_t index) override
        {
            NAU_ASSERT(index < m_collection.size(), "[{}], size():{}", index, m_collection.size());

            auto element = m_collection.begin();
            std::advance(element, index);

            return this->makeChildValue(makeValueRef(*element));
        }

        Result<> setAt(size_t index, const RuntimeValue::Ptr& value) override
        {
            NAU_ASSERT(value);
            NAU_ASSERT(index < m_collection.size(), "[{}], size():{}", index, m_collection.size());

            auto element = m_collection.begin();
            std::advance(element, index);

            return RuntimeValue::assign(makeValueRef(*element), value);
        }

        void clear() override
        {
            if constexpr (IsMutable)
            {
                NAU_FATAL(!this->hasChildren(), "Attempt to modify Runtime Collection while there is still referenced children");

                value_changes_scope;
                m_collection.clear();
            }
            else
            {
                NAU_FAILURE("Attempt to modify non mutable array value");
            }
        }

        void reserve([[maybe_unused]] size_t) override
        {
            if constexpr (!IsMutable)
            {
                NAU_FAILURE("Can not reserve for non mutable array");
            }
        }

        Result<> append(const RuntimeValue::Ptr& value) override
        {
            if constexpr (!IsMutable)
            {
                NAU_FAILURE("Attempt to modify non mutable value");
                return NauMakeError("Attempt to modify non mutable value");
            }
            else
            {
                value_changes_scope;

                typename ContainerType::value_type newElement;
                NauCheckResult(RuntimeValue::assign(makeValueRef(newElement), value))
                [[maybe_unused]] auto [iter, emplaceOk] = m_collection.emplace(std::move(newElement));
                NAU_ASSERT(emplaceOk, "Fail to emplace element (expects that collection holds only unique values)");

                return emplaceOk ? ResultSuccess : NauMakeError("Fail to append (unique) value");
            }
        }

    private:
        T m_collection;
    };
}  // namespace nau::ser_detail

namespace nau
{

    template <LikeStdVector T>
    RuntimeCollection::Ptr makeValueRef(T& collection, IMemAllocator::Ptr allocator)
    {
        using Collection = ser_detail::VectorLikeNativeCollection<T&>;

        return rtti::createInstanceWithAllocator<Collection>(std::move(allocator), collection);
    }

    template <LikeStdVector T>
    RuntimeCollection::Ptr makeValueRef(const T& collection, IMemAllocator::Ptr allocator)
    {
        using Collection = ser_detail::VectorLikeNativeCollection<const T&>;

        return rtti::createInstanceWithAllocator<Collection>(std::move(allocator), collection);
    }

    template <LikeStdVector T>
    RuntimeCollection::Ptr makeValueCopy(const T& collection, IMemAllocator::Ptr allocator)
    {
        using Collection = ser_detail::VectorLikeNativeCollection<T>;

        return rtti::createInstanceWithAllocator<Collection>(std::move(allocator), collection);
    }

    template <LikeStdVector T>
    RuntimeCollection::Ptr makeValueCopy(T&& collection, IMemAllocator::Ptr allocator)
    {
        using Collection = ser_detail::VectorLikeNativeCollection<T>;

        return rtti::createInstanceWithAllocator<Collection>(std::move(allocator), std::move(collection));
    }

    template <LikeStdList T>
    RuntimeCollection::Ptr makeValueRef(T& collection, IMemAllocator::Ptr allocator)
    {
        using Collection = ser_detail::ListLikeNativeCollection<T&>;

        return rtti::createInstanceWithAllocator<Collection>(std::move(allocator), collection);
    }

    template <LikeStdList T>
    RuntimeCollection::Ptr makeValueRef(const T& collection, IMemAllocator::Ptr allocator)
    {
        using Collection = ser_detail::ListLikeNativeCollection<const T&>;

        return rtti::createInstanceWithAllocator<Collection>(std::move(allocator), collection);
    }

    template <LikeStdList T>
    RuntimeCollection::Ptr makeValueCopy(const T& collection, IMemAllocator::Ptr allocator)
    {
        using Collection = ser_detail::ListLikeNativeCollection<T>;

        return rtti::createInstanceWithAllocator<Collection>(std::move(allocator), collection);
    }

    template <LikeStdList T>
    RuntimeCollection::Ptr makeValueCopy(T&& collection, IMemAllocator::Ptr allocator)
    {
        using Collection = ser_detail::ListLikeNativeCollection<T>;

        return rtti::createInstanceWithAllocator<Collection>(std::move(allocator), std::move(collection));
    }

    template <LikeSet T>
    RuntimeCollection::Ptr makeValueRef(T& collection, IMemAllocator::Ptr allocator)
    {
        using Collection = ser_detail::SetLikeNativeCollection<T&>;

        return rtti::createInstanceWithAllocator<Collection>(std::move(allocator), collection);
    }

    template <LikeSet T>
    RuntimeCollection::Ptr makeValueRef(const T& collection, IMemAllocator::Ptr allocator)
    {
        using Collection = ser_detail::SetLikeNativeCollection<const T&>;

        return rtti::createInstanceWithAllocator<Collection>(std::move(allocator), collection);
    }

    template <LikeSet T>
    RuntimeCollection::Ptr makeValueCopy(const T& collection, IMemAllocator::Ptr allocator)
    {
        using Collection = ser_detail::SetLikeNativeCollection<T>;

        return rtti::createInstanceWithAllocator<Collection>(std::move(allocator), collection);
    }

    template <LikeSet T>
    RuntimeCollection::Ptr makeValueCopy(T&& collection, IMemAllocator::Ptr allocator)
    {
        using Collection = ser_detail::SetLikeNativeCollection<T>;

        return rtti::createInstanceWithAllocator<Collection>(std::move(allocator), std::move(collection));
    }
}  // namespace nau
