// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

/**
 * @file array_allocator.h
 * @brief Definition of the ArrayAllocator class.
 */

#pragma once

#include "nau/memory/mem_allocator.h"
#include "nau/memory/mem_section_ptr.h"
#include "nau/memory/heap_allocator.h"
#include "nau/memory/aligned_allocator_debug.h"
#include "nau/diag/assertion.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/threading/thread_local_value.h"
#include "EASTL/memory.h"
#include "EASTL/stack.h"

namespace nau
{
    /**
     * @brief ArrayAllocator class is a custom memory allocator for arrays with a minimum size.
     * This allocator is useful for managing memory more efficiently by reusing previously allocated memory blocks and minimizing fragmentation
     * The ArrayAllocator manages memory blocks using a linked list of headers (Head structures) 
     * that store information about each allocated block. 
     * Each block also includes a signature to ensure the integrity of the memory allocation.
     * 
     * @tparam MinimumArraySize The minimum size of the array to be allocated.
     */
    template<int MinimumArraySize>
    class ArrayAllocator final : public IAlignedAllocatorDebug
    {
    public:
        /**
         * @brief Destructor for the ArrayAllocator.
         */
        ~ArrayAllocator() = default;

         /**
         * @brief Gets the singleton instance of the ArrayAllocator.
         * 
         * @return ArrayAllocator& Reference to the singleton instance.
         */
        static ArrayAllocator& instance()
        {
            static auto* instance = new ArrayAllocator<MinimumArraySize>;
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
         * @brief Allocates Allocates a block of memory of the given size and returns a pointer to it. 
         * If a suitable memory block is available in the free list, it is reused; otherwise, a new block is allocated.
         * 
         * @param size Size of the memory to allocate.
         * @return void* Pointer to the allocated memory.
         */
        [[nodiscard]] void* allocate(size_t size) override
        {
            void* ptr = nullptr;
            auto utilitySize = sizeof(Head) + sizeof(Signature);
            auto memSize = size + utilitySize;

            Head* priv = nullptr;
            auto it = getFreePointer();
            while (it)
            {
                if (it->reserve > size)
                {
                    ptr = reinterpret_cast<BytePtr>(it) + sizeof(Head);
                    if (priv)
                        priv->next = it->next;
                    else
                        getFreePointer() = it->next;

                    updateHeadAndSignature(it, size);
                    break;
                }

                priv = it;
                it = it->next;
            }

            if (!ptr)
            {
                auto pageSize = std::max(memSize, getSection()->getPageSize());
                auto newPage = getSection()->allocate(pageSize);
                auto head = static_cast<Head*>(newPage);
                head->reserve = pageSize - utilitySize;
                head->next = nullptr;
                updateHeadAndSignature(head, size);

                ptr = reinterpret_cast<BytePtr>(head) + sizeof(Head);
            }

            m_allocs.value()++;
            return ptr;
        }

        /**
         * @brief Reallocates the memory block at the given pointer to a new size.
         * 
         * @param ptr Pointer to the existing memory block.
         * @param size New size for the memory block.
         * @return void* Pointer to the reallocated memory block.
         */
        [[nodiscard]] void* reallocate(void* ptr, size_t size) override
        {
            if (!ptr)
                return allocate(size);

            Head* const pagePtr = getHead(ptr);
            void* newPtr = ptr;
            if (pagePtr->reserve > size)
            {
                updateHeadAndSignature(pagePtr, size);
            }
            else
            {
                newPtr = allocate(size);
                memcpy(newPtr, ptr, std::min(size, pagePtr->size));
                deallocate(ptr);
            }
            return newPtr;
        }

        /**
         * @brief Frees the memory allocated at the given pointer.
         * 
         * @param ptr Pointer to the memory to free.
         */
        void deallocate(void* ptr) override
        {
            Head* const pagePtr = getHead(ptr);

            pagePtr->next = getFreePointer();
            getFreePointer() = pagePtr;

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
        size_t getSize(const void* ptr) const override
        {
            if (!ptr)
                return 0;

            const auto* pagePtr = reinterpret_cast<const Head*>(static_cast<ConstBytePtr>(ptr) - sizeof(Head));
            return pagePtr->size;
        }


    private:

        struct alignas(std::max_align_t) Head
        {
            size_t reserve = 0;
            size_t size = 0;
            Head* next = nullptr;
        };

        struct Signature
        {
            uintptr_t value;
        };

        using BytePtr = std::byte*;
        using ConstBytePtr = const std::byte*;

        bool m_readyToRelease = false;
        ThreadLocalValue<int> m_allocs;
        ThreadLocalValue<eastl::shared_ptr<Head*>> m_freePointersPool;
        ThreadLocalValue<MemSectionPtr> m_memSection;

        ArrayAllocator()
            : m_allocs([](auto& val) {val = 0; })
        {};
        ArrayAllocator(const ArrayAllocator&) = delete;
        ArrayAllocator& operator=(const ArrayAllocator&) = delete;
        ArrayAllocator(ArrayAllocator&&) = delete;
        ArrayAllocator& operator=(ArrayAllocator&&) = delete;

        MemSectionPtr& getSection()
        {
            auto& memSection = m_memSection.value();
            if(!memSection.valid())
                memSection = HeapAllocator::instance().getSection("ArrayAllocator<" + eastl::to_string(MinimumArraySize) + ">");

            auto sizeRequest = MinimumArraySize + sizeof(Head) + sizeof(Signature);
            if (memSection->getPageSize() < sizeRequest)
                memSection->setPageSize(sizeRequest);
            return memSection;
        }

        Head*& getFreePointer()
        {
            auto& val = m_freePointersPool.value();
            if (!val)
            {
                val = eastl::make_shared<Head*>();
                *val = nullptr;
            }
            return *val;

        }

        Head* getHead(void* clientPtr)
        {
            Head* const head = reinterpret_cast<Head*>(reinterpret_cast<BytePtr>(clientPtr) - sizeof(Head));

            NAU_ASSERT(reinterpret_cast<uintptr_t>(head) % alignof(Head) == 0);

#ifdef NAU_ASSERT_ENABLED
            Signature signature;
            memcpy(&signature, static_cast<BytePtr>(clientPtr) + head->size, sizeof(signature));
            NAU_ASSERT(signature.value == reinterpret_cast<uintptr_t>(head));
#endif
            return head;
        }

        void updateHeadAndSignature(Head* head, size_t newSize)
        {
            head->size = newSize;

            BytePtr const clientPtr = reinterpret_cast<BytePtr>(head) + sizeof(Head);
            const Signature signature{reinterpret_cast<uintptr_t>(head)};
            memcpy(clientPtr + head->size, &signature, sizeof(signature));
        }
    };

}
