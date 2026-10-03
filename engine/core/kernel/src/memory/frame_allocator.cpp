// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/memory/frame_allocator.h"

#include <string>

#include "nau/memory/heap_allocator.h"

namespace nau
{
    IFrameAllocator*& IFrameAllocator::globalAllocator()
    {
        static IFrameAllocator* allocator = nullptr;
        return allocator;
    }

    FrameAllocator::FrameAllocator() :
        m_memSection([](MemSectionPtr& value)
    {
        value = HeapAllocator::instance().getSection("FrameAllocator:" + std::to_string((size_t)&value));
    }),
        m_numAllocs([](int& value)
    {
        value = 0;
    })
    {
    }

    FrameAllocator::~FrameAllocator()
    {
        int total = 0;
        m_numAllocs.visitAll([&total](int& num)
        {
            total += num;
            num = 0;
        });
        NAU_ASSERT(total == 0, "FrameAllocator not all allocations has been deallocated");
    }

    bool FrameAllocator::prepareFrame()
    {
        int total = 0;
        m_numAllocs.visitAll([&total](int& num)
        {
            total += num;
            num = 0;
        });

        NAU_ASSERT(total == 0, "FrameAllocator not all allocations has been deallocated");
        m_memSection.visitAll([](MemSectionPtr& a)
        {
            a->reset();
        });
        return total == 0;
    }

    [[nodiscard]]
    void* FrameAllocator::allocate(size_t size)
    {
        auto realSize = size + sizeof(size_t);
        ++m_numAllocs.value();
        auto ptr = m_memSection.value()->allocate(realSize);
        *static_cast<size_t*>(ptr) = size;
        return static_cast<char*>(ptr) + sizeof(size_t);
    }

    void* FrameAllocator::reallocate(void* ptr, size_t size)
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

    void FrameAllocator::deallocate(void* ptr)
    {
        if (ptr)
            --m_numAllocs.value();
    }

    size_t FrameAllocator::getSize(const void* ptr) const
    {
        if (!ptr)
            return 0;

        return *reinterpret_cast<const size_t*>(static_cast<const char*>(ptr) - sizeof(size_t));
    }

    void IFrameAllocator::setFrameAllocator(IFrameAllocator* allocator)
    {
        IFrameAllocator::globalAllocator() = allocator;
    }

    IFrameAllocator* IFrameAllocator::getFrameAllocator()
    {
        auto* alloc = IFrameAllocator::globalAllocator();
        NAU_ASSERT(alloc, "global FrameAllocator is not initilized");
        return alloc;
    }

}  // namespace nau