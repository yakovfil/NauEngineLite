// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <cstdint>
#include <memory>

#include "nau/diag/assertion.h"
#include "nau/rtti/type_info.h"

namespace nau
{
    /**
     * @brief Checks if a value is a power of 2.
     *
     * @param value The value to check.
     * @return true if the value is a power of 2, false otherwise.
     */
    constexpr inline bool isPowerOf2(size_t value)
    {
        return value && (value & (value - 1)) == 0;
    }

    /**
     * @brief Aligns a size to the specified alignment.
     *
     * @param size The size to align.
     * @param alignment The alignment to use.
     * @return The aligned size.
     */
    constexpr inline size_t alignedSize(size_t size, size_t alignment)
    {
        NAU_ASSERT(isPowerOf2(alignment), "alignment expected to be a power of two. Actual value: ({})", alignment);
        return (size + alignment - 1) & ~(alignment - 1);
    }

    inline bool isAligned(const void* ptr, size_t alignment) noexcept
    {
        return reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0;
    }
    /**
     * @brief Abstract interface for memory allocators.
     */
    struct NAU_ABSTRACT_TYPE IMemAllocator
    {
        using Ptr = std::shared_ptr<IMemAllocator>;

        /**
         * @brief Allocates a block of memory.
         *
         * @param size The size of memory to allocate.
         * @return A pointer to the allocated memory.
         */
        [[nodiscard]] virtual void* allocate(size_t size) = 0;

        /**
         * @brief Reallocates a block of memory.
         *
         * @param ptr The pointer to the previously allocated memory.
         * @param size The new size of memory to allocate.
         * @return A pointer to the reallocated memory.
         */
        [[nodiscard]] virtual void* reallocate(void* ptr, size_t size) = 0;

        /**
         * @brief Deallocates a block of memory.
         *
         * @param ptr The pointer to the memory to deallocate.
         */
        virtual void deallocate(void* ptr) = 0;

        /**
         * @brief Gets the size of a previously allocated block of memory.
         *
         * @param ptr The pointer to the memory block.
         * @return The size of the memory block.
         */
        [[nodiscard]] virtual size_t getSize(const void* ptr) const = 0;

        /**
         * @brief Allocates a block of aligned memory.
         *
         * @param size The size of memory to allocate.
         * @param alignment The alignment to use.
         * @return A pointer to the allocated memory.
         */
        [[nodiscard]] virtual void* allocateAligned(size_t size, size_t alignment) = 0;

        /**
         * @brief Reallocates a block of aligned memory.
         *
         * @param ptr The pointer to the previously allocated memory.
         * @param size The new size of memory to allocate.
         * @return A pointer to the reallocated memory.
         */
        [[nodiscard]] virtual void* reallocateAligned(void* ptr, size_t size, size_t alignment) = 0;

        /**
         * @brief Deallocates a block of aligned memory.
         *
         * @param ptr The pointer to the memory to deallocate.
         */
        virtual void deallocateAligned(void* ptr) = 0;

        /**
         * @brief Gets the size of a previously allocated block of memory.
         *
         * @param ptr The pointer to the memory block.
         * @return The size of the memory block.
         */
        [[nodiscard]] virtual size_t getSizeAligned(const void* ptr, size_t alignment) const = 0;

        // Debug functions

        /**
         * @brief Check, is block allocated by allocateAligned
         *
         * @param ptr The pointer to the memory block.
         * @return Is block allocated as aligned
         */
        [[nodiscard]] virtual bool isAligned(const void* ptr) const = 0;

        /**
         * @brief Check, if block is not damaged
         *
         * @param ptr The pointer to the memory block.
         * @return Block state
         */
        [[nodiscard]] virtual bool isValid(const void* ptr) const = 0;

        /**
         * @brief Get allocator name
         *
         * @return A pointer to the char* name string.
         */
        [[nodiscard]] virtual const char* getName() const = 0;

        /**
         * @brief Get allocator name
         *
         * @param name The pointer to the char* name string.
         */
        virtual void setName(const char* name) = 0;
    };

    /**
     * @brief Gets the default memory allocator.
     *
     * @return A shared pointer to the default memory allocator.
     */
    NAU_KERNEL_EXPORT const IMemAllocator::Ptr& getDefaultAllocator();

    /**
     * @brief Concept to check if a type derives from IMemAllocator.
     *
     * Used for template constraints.
     */
    template <class Derived>
    concept IsNauAllocator = std::is_base_of<IMemAllocator, Derived>::value;
}  // namespace nau
