// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <unordered_map>
#include <utility>

#include "nau/memory/mem_allocator.h"
#include "nau/threading/spin_lock.h"
#include "nau/threading/thread_local_value.h"

namespace nau
{
    /**
     * @brief IMemAllocator partial implemantation for align allocations
     * Using base allocate/deallocate functionality with some overallocation
     */
    class NAU_KERNEL_EXPORT IAlignedAllocator : public IMemAllocator
    {
    public:
        IAlignedAllocator()
        {
            m_name.value().reserve(32);
            m_name.value() = "IAlignedAllocator";
            m_allocations.value().reserve(1024 * 16);
        }

        virtual ~IAlignedAllocator()
        {
        }

        /**
         * @brief Allocates a block of aligned memory.
         *
         * @param size The size of memory to allocate.
         * @param alignment The alignment to use.
         * @return A pointer to the allocated memory.
         */
        [[nodiscard]] virtual void* allocateAligned(size_t size, size_t alignment) override;

        /**
         * @brief Reallocates a block of aligned memory.
         *
         * @param ptr The pointer to the previously allocated memory.
         * @param size The new size of memory to allocate.
         * @return A pointer to the reallocated memory.
         */
        [[nodiscard]] virtual void* reallocateAligned(void* ptr, size_t size, size_t alignment) override;

        /**
         * @brief Deallocates a block of aligned memory.
         *
         * @param ptr The pointer to the memory to deallocate.
         */
        virtual void deallocateAligned(void* ptr) override;

        /**
         * @brief Gets the size of a previously allocated block of memory.
         *
         * @param ptr The pointer to the memory block.
         * @return The size of the memory block.
         */
        [[nodiscard]] virtual size_t getSizeAligned(const void* ptr, size_t alignment) const override;

        /**
         * @brief Check, is block allocated by allocateAligned
         *
         * @param ptr The pointer to the memory block.
         * @return Is block allocated as aligned
         */
        [[nodiscard]] virtual bool isAligned(const void* ptr) const override;

        /**
         * @brief Check, if block is not damaged
         *
         * @param ptr The pointer to the memory block.
         * @return Block state
         */
        [[nodiscard]] virtual bool isValid(const void* ptr) const override;

        /**
         * @brief Get allocator name
         *
         * @return A pointer to the char* name string.
         */
        virtual const char* getName() const override;

        /**
         * @brief Get allocator name
         *
         * @param name The pointer to the char* name string.
         */
        virtual void setName(const char* name) override;

    protected:
        struct AllocationInfo
        {
            void* m_unaligned;
            size_t m_size;
            size_t m_alignment;
        };

        std::pair<AllocationInfo*, std::unordered_map<void*, AllocationInfo>*> getAllocationInfo(const void* ptr) const;

        mutable ThreadLocalValue<std::string> m_name;
        mutable ThreadLocalValue<std::unordered_map<void*, AllocationInfo>> m_allocations;
        mutable threading::SpinLock m_lock;
    };

}  // namespace nau
