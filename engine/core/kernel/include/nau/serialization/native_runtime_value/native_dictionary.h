// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <type_traits>

#include "nau/rtti/rtti_impl.h"
#include "nau/serialization/native_runtime_value/native_value_base.h"
#include "nau/serialization/native_runtime_value/native_value_forwards.h"

namespace nau::ser_detail
{
    template <typename T>
    class MapLikeNativeDictionary final : public ser_detail::NativeRuntimeValueBase<RuntimeDictionary>
    {
        using Base = ser_detail::NativeRuntimeValueBase<RuntimeDictionary>;
        using DictionaryType = std::decay_t<T>;

        NAU_CLASS_(MapLikeNativeDictionary<T>, Base)

    public:
        static_assert(LikeStdMap<DictionaryType>);

        static inline constexpr bool IsMutable = !std::is_const_v<std::remove_reference_t<T>>;
        static inline constexpr bool IsReference = std::is_lvalue_reference_v<T>;

        MapLikeNativeDictionary(const DictionaryType& dict)
        requires(!IsReference)
            :
            m_dict(dict)
        {
        }

        MapLikeNativeDictionary(DictionaryType&& dict)
        requires(!IsReference)
            :
            m_dict(std::move(dict))
        {
        }

        MapLikeNativeDictionary(T dict)
        requires(IsReference)
            :
            m_dict(dict)
        {
        }

        bool isMutable() const override
        {
            return IsMutable;
        }

        size_t getSize() const override
        {
            return m_dict.size();
        }

        std::string_view getKey(size_t index) const override
        {
            auto head = m_dict.begin();
            // EASTL and std iterators have distinct category tags in libc++.
            for (size_t i = 0; i < index; ++i)
            {
                ++head;
            }
            return std::string_view{head->first.data(), head->first.size()};
        }

        RuntimeValue::Ptr getValue(std::string_view key) override
        {
            auto iter = m_dict.find(typename DictionaryType::key_type{key.data(), key.size()});
            if(iter == m_dict.end())
            {
                return nullptr;
            }

            return this->makeChildValue(makeValueRef(iter->second));
        }

        bool containsKey(std::string_view key) const override
        {
            return m_dict.find(typename DictionaryType::key_type{key.data(), key.size()}) != m_dict.end();
        }

        void clear() override
        {
            if constexpr(IsMutable)
            {
                NAU_FATAL(!this->hasChildren(), "Attempt to modify Runtime Collection while there is still referenced children");

                value_changes_scope;
                m_dict.clear();
            }
            else
            {
                NAU_FAILURE("Attempt to modify non mutable value");
            }
        }

        Result<> setValue([[maybe_unused]] std::string_view key, [[maybe_unused]] const RuntimeValue::Ptr& newValue) override
        {
            if constexpr(IsMutable)
            {
                value_changes_scope;

                auto iter = m_dict.find(typename DictionaryType::key_type{key.data(), key.size()});
                if(iter == m_dict.end())
                {
                    auto [emplacedIter, emplaceOk] = m_dict.try_emplace(typename DictionaryType::key_type{key.data(), key.size()});
                    NAU_ASSERT(emplaceOk);
                    iter = emplacedIter;
                }

                return RuntimeValue::assign(makeValueRef(iter->second), newValue);
            }
            else
            {
                NAU_FAILURE("Attempt to modify non mutable value");
            }

            return {};
        }

        RuntimeValue::Ptr erase([[maybe_unused]] std::string_view key) override
        {
            if constexpr(IsMutable)
            {
                // value_changes_scope;
                // DictionaryValueOperations<DictionaryType>::
                NAU_FAILURE("NativeDictionary::erase not implemented");
            }
            else
            {
                NAU_FAILURE("Attempt to modify non mutable value");
            }

            return nullptr;
        }

    private:
        T m_dict;
    };
}  // namespace nau::ser_detail

namespace nau
{
    template <LikeStdMap T>
    RuntimeDictionary::Ptr makeValueRef(T& dict, IMemAllocator::Ptr allocator)
    {
        using Dict = ser_detail::MapLikeNativeDictionary<T&>;

        return rtti::createInstanceWithAllocator<Dict>(std::move(allocator), dict);
    }

    template <LikeStdMap T>
    RuntimeDictionary::Ptr makeValueRef(const T& dict, IMemAllocator::Ptr allocator)
    {
        using Dict = ser_detail::MapLikeNativeDictionary<const T&>;

        return rtti::createInstanceWithAllocator<Dict>(std::move(allocator), dict);
    }

    template <LikeStdMap T>
    RuntimeDictionary::Ptr makeValueCopy(const T& dict, IMemAllocator::Ptr allocator)
    {
        using Dict = ser_detail::MapLikeNativeDictionary<T>;

        return rtti::createInstanceWithAllocator<Dict>(std::move(allocator), dict);
    }

    template <LikeStdMap T>
    RuntimeDictionary::Ptr makeValueCopy(T&& dict, IMemAllocator::Ptr allocator)
    {
        using Dict = ser_detail::MapLikeNativeDictionary<T>;

        return rtti::createInstanceWithAllocator<Dict>(std::move(allocator), std::move(dict));
    }

}  // namespace nau
