// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

/**
 * @file fixed_blocks.h
 * @brief Definition of the FixedBlocksAllocator class.
 */
#pragma once

#include <memory>
#include <string>
#include <type_traits>

#include "nau/memory/aligned_allocator_debug.h"
#include "nau/memory/heap_allocator.h"
#include "nau/memory/mem_allocator.h"
#include "nau/memory/mem_section_ptr.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/threading/thread_local_value.h"

namespace nau
{
    /**
     * @brief Concept to check if one integer is greater than or equal to another.
     *
     * @tparam T The first integer.
     * @tparam U The second integer.
     */
    template <int T, int U>
    concept BigerThen = std::same_as<std::bool_constant<(T >= U)>, std::true_type>;

    /**
     * @brief FixedBlocksAllocator class is a custom memory allocator for fixed-size blocks.
     * The FixedBlocksAllocator class is particularly suited for scenarios where memory blocks
     * of a fixed size are frequently allocated and deallocated.
     * Its design ensures efficient memory management, minimizes fragmentation,
     * and improves performance, especially in multithreaded environments.
     * By reusing memory blocks and maintaining thread-local free lists,
     * it provides a robust solution for fixed-size memory allocation needs.
     *
     * @tparam BlockSize The size of each block to be allocated.
     */
    template <int BlockSize>
    requires BigerThen<BlockSize, sizeof(void*)>
    class FixedBlocksAllocator final : public IAlignedAllocatorDebug
    {
    public:
        /**
         * @brief Gets the singleton instance of the FixedBlocksAllocator.
         *
         * @return FixedBlocksAllocator& Reference to the singleton instance.
         */
        static FixedBlocksAllocator<BlockSize>& instance()
        {
            static auto instance = new FixedBlocksAllocator<BlockSize>;
            // The allocator's deletion is delayed until all allocated blocks are deallocated.
            static RAIIFunction releaser(nullptr, [&]()
            {
                instance->m_readyToRelease = true;
                int total = 0;
                instance->m_allocs.visitAll([&total](auto& val)
                {
                    total += val;
                });
                if (!total)
                    delete instance;
            });

            return *instance;
        }

        /**
         * @brief Allocates a block of memory of the given size.
         *
         * @param size Size of the memory to allocate.
         * @return void* Pointer to the allocated memory.
         */
        [[nodiscard]] void* allocate(size_t size) override
        {
            NAU_ASSERT(size <= BlockSize, "Invalid size");

            auto out = getFreePointer();

            void* next = static_cast<PtrPtr>(out)->next;
            if (!next)
            {
                next = getSection()->allocate(BlockSize);
                NAU_ASSERT(next, "Out of memory");
                if (next)
                    static_cast<PtrPtr>(next)->next = nullptr;
            }

            getFreePointer() = next;
            m_allocs.value()++;
            return out;
        }

        /**
         * @brief The reallocate method is designed to handle blocks of a fixed size defined by the template parameter BlockSize.
         * This means that the size parameter passed to the reallocate method must be less than or equal to BlockSize.
         * Unlike typical reallocate implementations that might resize the memory block to a new size,
         * this reallocate method does not change the size of the allocated block.
         * Instead, it verifies that the requested size is within the allowable fixed block size.
         *
         * @param ptr Pointer to the existing memory block.
         * @param size New size for the memory block.
         * @return void* The method returns the same pointer that was passed in without performing any memory copy or allocation.
         * This behavior is based on the assumption that the memory block does not need to be resized because all blocks are of the same fixed size.
         */
        [[nodiscard]] void* reallocate(void* ptr, size_t size) override
        {
            NAU_ASSERT(size <= BlockSize, "Invalid size");
            return ptr;
        }

        /**
         * @brief Deallocates the memory block at the given pointer.
         *
         * @param p Pointer to the memory block to deallocate.
         */
        void deallocate(void* p) override
        {
            static_cast<PtrPtr>(p)->next = getFreePointer();
            getFreePointer() = p;
            m_allocs.value()--;
            if (m_readyToRelease)
            {
                int total = 0;
                m_allocs.visitAll([&total](auto& val)
                {
                    total += val;
                });
                if (!total)
                    delete this;
            }
        }

        /**
         * @brief Gets the size of the allocated memory block.
         *
         * @param ptr Pointer to the memory block.
         * @return size_t Size of the memory block.
         */
        virtual size_t getSize(const void* ptr) const override
        {
            return BlockSize;
        }

    private:
        struct Ptr
        {
            void* next = nullptr;
        };
        using PtrPtr = Ptr*;

        struct FreePointer
        {
            void* pointer = nullptr;
        };

        bool m_readyToRelease = false;
        ThreadLocalValue<int> m_allocs;
        ThreadLocalValue<std::shared_ptr<FreePointer>> m_freePointersPool;
        ThreadLocalValue<MemSectionPtr> m_memSection;

        FixedBlocksAllocator() :
            m_allocs([](auto& val)
        {
            val = 0;
        })
        {
            getFreePointer() = getSection()->allocate(BlockSize);
            static_cast<PtrPtr>(getFreePointer())->next = nullptr;
        }

        MemSectionPtr& getSection()
        {
            auto& memSection = m_memSection.value();
            if (!memSection.valid())
                memSection = HeapAllocator::instance().getSection("FixedBlocksAllocator<" + std::to_string(BlockSize) + ">");
            return memSection;
        }

        void*& getFreePointer()
        {
            auto& val = m_freePointersPool.value();
            if (!val)
            {
                val = std::make_shared<FreePointer>();
                val->pointer = getSection()->allocate(BlockSize);
                static_cast<PtrPtr>(val->pointer)->next = nullptr;
            }
            return val->pointer;
        }
    };
}  // namespace nau
