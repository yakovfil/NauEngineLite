// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <string>

#include "nau/memory/aligned_allocator_debug.h"
#include "nau/memory/heap_allocator.h"
#include "nau/memory/mem_allocator.h"
#include "nau/memory/mem_section_ptr.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/threading/thread_local_value.h"
#include "nau/utils/preprocessor.h"

namespace nau
{
    struct AllocatorsChain;

    /**
     * @brief Interface for stack-based memory allocators.
     *
     * This interface defines methods for allocating, reallocating, deallocating,
     * and checking the state of memory allocations within a stack allocator.
     */
    class NAU_KERNEL_EXPORT IStackAllocator : public IAlignedAllocatorDebug
    {
    public:
        virtual ~IStackAllocator()
        {
        }
        /**
         * @brief Checks if the stack allocator is clear (no active allocations).
         *
         * @return true if the allocator is clear, false otherwise.
         */
        [[nodiscard]] virtual bool isClear() const = 0;

        /**
         * @brief Sets the thread-local stack allocator instance.
         *
         * @param allocator Pointer to the stack allocator instance.
         */
        static void setStackAllocator(IStackAllocator* allocator);

        /**
         * @brief Retrieves the thread-local stack allocator instance.
         *
         * @return Pointer to the thread-local stack allocator instance.
         */
        [[nodiscard]] static IStackAllocator* getStackAllocator();

        /**
         * @brief Releases the thread-local stack allocator instance.
         *
         * This method releases the current thread's association with the
         * stack allocator instance.
         */
        static void releaseStackAllocator();

    private:
        [[nodiscard]] static AllocatorsChain*& globalAllocator();
    };

    /**
     * @brief StackAllocator class template for allocating fixed-size memory from a stack. Optimized for thread-local usage.
     * Each thread manages its own stack allocator instance, ensuring thread safety without explicit synchronization.
     *
     * @tparam Tsize Maximum size of memory that can be allocated at stack by this allocator.
     */
    template <size_t Tsize>
    class StackAllocator final : public IStackAllocator
    {
    public:
        /**
         * @brief Constructs a StackAllocator instance.
         *
         * Initializes the allocator with necessary data structures.
         */
        StackAllocator() :
            m_memSection([](MemSectionPtr& value)
        {
            value = HeapAllocator::instance().getSection("StackAllocator:" + std::to_string((size_t)&value));
        }),
            m_numAllocs([](int& value)
        {
            value = 0;
        }),
            m_motherThread([](bool& value)
        {
            value = false;
        })
        {
            m_motherThread.value() = true;
        }

        /**
         * @brief Destructor for StackAllocator.
         *
         * Asserts that all allocations have been deallocated upon destruction.
         */
        ~StackAllocator()
        {
            NAU_ASSERT(isClear(), "StackAllocator not all allocations has been deallocated");
        }

        /**
         * @brief Checks if the stack allocator is clear (no active allocations).
         *
         * @return true if the allocator is clear, false otherwise.
         */
        [[nodiscard]] bool isClear() const override
        {
            int total = 0;
            m_numAllocs.visitAll([&total](const int& num)
            {
                total += num;
            });
            return total == 0;
        }

        StackAllocator(const StackAllocator&) = delete;
        StackAllocator& operator=(const StackAllocator&) = delete;
        StackAllocator(StackAllocator&&) = delete;
        StackAllocator& operator=(StackAllocator&&) = delete;

        /**
         * @brief Allocates memory from the stack allocator.
         *
         * @param size Size of memory to allocate.
         * @return Pointer to the allocated memory block.
         */
        [[nodiscard]] void* allocate(size_t size) override
        {
            ++m_numAllocs.value();

            auto realSize = size + sizeof(size_t);
            if (m_motherThread.value() && (m_offset + realSize) < Tsize)
            {
                void* ptr = m_data + m_offset;
                m_offset += realSize;
                return ptr;
            }
            auto ptr = m_memSection.value()->allocate(realSize);
            *static_cast<size_t*>(ptr) = size;
            return static_cast<char*>(ptr) + sizeof(size_t);
        }

        /**
         * @brief Reallocates memory from the stack allocator.
         *
         * @param ptr Pointer to the previously allocated memory block.
         * @param size New size of memory to allocate.
         * @return Pointer to the reallocated memory block.
         */
        [[nodiscard]] void* reallocate(void* ptr, size_t size) override
        {
            if (!ptr)
                return allocate(size);

            auto oldPtr = static_cast<char*>(ptr) - sizeof(size_t);
            auto oldSize = *reinterpret_cast<size_t*>(oldPtr);
            if (size <= oldSize)
                return ptr;

            auto newPtr = allocate(size);
            memcpy(newPtr, ptr, oldSize);
            deallocate(ptr);
            return newPtr;
        }

        /**
         * @brief Deallocates memory from the stack allocator.
         *
         * @param ptr Pointer to the memory block to deallocate.
         */
        void deallocate(void* ptr) override
        {
            if (ptr)
                --m_numAllocs.value();
        }

        /**
         * @brief Retrieves the size of a previously allocated memory block.
         *
         * @param ptr Pointer to the memory block.
         * @return Size of the memory block.
         */
        virtual size_t getSize(const void* ptr) const override
        {
            if (!ptr)
                return 0;

            return *reinterpret_cast<const size_t*>(static_cast<const char*>(ptr) - sizeof(size_t));
        }

    private:
        size_t m_offset = 0;
        char m_data[Tsize];

        ThreadLocalValue<MemSectionPtr> m_memSection;
        ThreadLocalValue<int> m_numAllocs;
        ThreadLocalValue<bool> m_motherThread;
    };

    /**
     * @brief Concept to check if a type derives from IStackAllocator.
     *
     * Used for template constraints.
     */
    template <class Derived>
    concept IsStackAllocator = std::is_base_of<IStackAllocator, Derived>::value;

    /**
     * @brief RAII wrapper for setting and releasing a thread-local stack allocator.
     *
     * Automatically sets the provided stack allocator as the thread-local allocator
     * upon construction and releases it upon destruction.
     *
     * @tparam TAllocator Type of the stack allocator.
     */
    template <class TAllocator = StackAllocator<64 * 1024>>
    requires IsStackAllocator<TAllocator>
    class LocalStackAllocator
    {
    public:
        /**
         * @brief Constructs the LocalStackAllocator.
         *
         * Sets the provided allocator as the thread-local stack allocator.
         */
        LocalStackAllocator()
        {
            IStackAllocator::setStackAllocator(&m_allocator);
        }

        /**
         * @brief Destroys the LocalStackAllocator.
         *
         * Releases the thread-local association with the stack allocator.
         */
        ~LocalStackAllocator()
        {
            IStackAllocator::releaseStackAllocator();
        }

        /**
         * @brief Accesses the underlying stack allocator instance.
         *
         * @return Pointer to the stack allocator instance.
         */
        TAllocator* operator->()
        {
            return &m_allocator;
        }

        /**
         * @brief Retrieves a reference to the underlying stack allocator instance.
         *
         * @return Reference to the stack allocator instance.
         */
        TAllocator& get()
        {
            return m_allocator;
        }

    private:
        TAllocator m_allocator;
    };

    /**
     * @brief RAII guard for setting and releasing a thread-local stack allocator.
     *
     * Similar to LocalStackAllocator but takes an external stack allocator instance
     * as a constructor parameter instead of creating one internally.
     *
     * @tparam TAllocator Type of the stack allocator.
     */
    template <class TAllocator>
    requires IsStackAllocator<TAllocator>
    class StackAllocatorGuard
    {
    public:
        /**
         * @brief Constructs the StackAllocatorGuard with the provided allocator.
         *
         * Sets the provided allocator as the thread-local stack allocator.
         *
         * @param allocator Reference to the stack allocator instance.
         */
        StackAllocatorGuard(TAllocator& allocator) :
            m_allocator(allocator)
        {
            IStackAllocator::setStackAllocator(&m_allocator);
        }

        /**
         * @brief Destroys the StackAllocatorGuard.
         *
         * Releases the thread-local association with the stack allocator.
         */
        ~StackAllocatorGuard()
        {
            IStackAllocator::releaseStackAllocator();
        }

        /**
         * @brief Accesses the underlying stack allocator instance.
         *
         * @return Pointer to the stack allocator instance.
         */
        TAllocator* operator->() const
        {
            return &m_allocator;
        }

    private:
        TAllocator& m_allocator;
    };

}  // namespace nau

/**
 * @brief Macro creates an anonymous local stack allocator instance that
 * can be used for allocating memory within a specific scope.
 */
#define StackAllocatorUnnamed nau::LocalStackAllocator ANONYMOUS_VAR(stack_allocator)
/**
 * @brief Macro creates a guard that sets the current stack allocator to a
 * specified instance for the duration of its scope.
 */
#define StackAllocatorInherit(...) nau::StackAllocatorGuard ANONYMOUS_VAR(guard)(__VA_ARGS__)
/**
 * @brief Macro provides a convenient way to allocate memory using the global stack allocator.
 */
#define stack_new unique_new(*nau::IStackAllocator::getStackAllocator())

/**
 * @brief Macro for capture parent stack allocator in lambda capture list
 */
#define ParentStackAllocator parentStackAllocator = IStackAllocator::getStackAllocator()
/**
 * @brief Macro creates a guard that sets the parent stack allocator to a
 * specified instance for the duration of its scope.
 */
#define StackAllocatorInheritParent StackAllocatorInherit(*parentStackAllocator);