// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <functional>
#include <memory>

#include "nau/memory/mem_allocator.h"

namespace nau
{
    /**
     * @brief Wrapper for NAU allocators.
     */
    struct NAU_KERNEL_EXPORT NauAllocatorWrapper final
    {
        IMemAllocator* m_allocator = nullptr;

        NauAllocatorWrapper() = default;

        /**
         * @brief Constructs the wrapper with the given allocator.
         *
         * @tparam TAllocator The type of the allocator.
         * @param allocator The allocator instance.
         */
        template <class TAllocator = IMemAllocator>
        requires IsNauAllocator<TAllocator>
        NauAllocatorWrapper(TAllocator& allocator) :
            m_allocator(&allocator)
        {
        }

        /**
         * @brief Gets the underlying allocator.
         *
         * @return Pointer to the allocator.
         */
        IMemAllocator* get() const
        {
            return m_allocator;
        }
    };

    /**
     * @brief A unique pointer type for objects allocated using the NAU allocator.
     *
     * @tparam T The type of the object.
     */
    template <class T>
    using alloc_unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;

    /**
     * @brief Functor to create unique pointers with custom deallocation.
     *
     * @tparam TAllocator The type of the allocator.
     */
    template <class TAllocator = IMemAllocator>
    requires IsNauAllocator<TAllocator>
    struct MakeUnique
    {
        TAllocator* m_allocator = nullptr;

        MakeUnique() = default;

        MakeUnique(TAllocator& allocator) :
            m_allocator(&allocator)
        {
        }

        /**
         * @brief Creates a unique pointer for the given raw pointer.
         * Use MakeUnique for Custom Allocation:
         * CustomAllocator myAllocator;
         * auto uniquePtr = nau::MakeUnique(myAllocator) | new int(42);
         * or
         * auto uniquePtr = unique_new(myAllocator) int(42)
         *
         * @tparam T The type of the object.
         * @param ptr The raw pointer to the object.
         * @return A unique pointer with custom deallocation.
         */
        template <class T>
        [[nodiscard]] alloc_unique_ptr<T> operator|(T* ptr) const
        {
            return alloc_unique_ptr<T>(ptr, [allocator = m_allocator](T* ptr)
            {
                if (allocator)
                {
                    ptr->~T();
                    allocator->deallocate(ptr);
                }
                else
                    delete ptr;
            });
        }
    };

}  // namespace nau

/**
 * @brief Custom new operator using the NAU allocator.
 *
 * @param size The size of memory to allocate.
 * @param wraper The NAU allocator wrapper.
 * @return Pointer to the allocated memory.
 */
NAU_KERNEL_EXPORT void* operator new(size_t size, const nau::NauAllocatorWrapper&);
#define unique_new(...) nau::MakeUnique(__VA_ARGS__) | new(nau::NauAllocatorWrapper{__VA_ARGS__})
#define unique_new_ unique_new()
