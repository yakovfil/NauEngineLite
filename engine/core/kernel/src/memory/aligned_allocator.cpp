// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include <memory>
#include "nau/utils/performance_profiling.h"
#include "nau/memory/aligned_allocator.h"

namespace nau
{
    void* IAlignedAllocator::allocateAligned(size_t size, size_t alignment)
    {
        if (!isPowerOf2(alignment))
        {
            return nullptr;
        }
        size_t reserved = size + alignment;
        auto* unaligned = this->allocate(reserved);
        if (unaligned != nullptr)
        {
            lock_(m_lock);
            size_t space = reserved;
            void* aligned = unaligned;
            aligned = std::align(alignment, size, aligned, space);
            auto infoRes = m_allocations.value().emplace(aligned, AllocationInfo());
            if (infoRes.second)
            {
                auto& info = infoRes.first->second;
                info.m_unaligned = unaligned;
                info.m_size = size;
                info.m_alignment = alignment;
// TODO Tracy                TracyAllocN(unaligned, reserved, m_name.value().c_str());
                return aligned;
            }
        }
        NAU_FAILURE("IAlignedAllocator::allocateAligned failed");
        return nullptr;
    }

    void* IAlignedAllocator::reallocateAligned(void* ptr, size_t size, size_t alignment)
    {
        if (!ptr)
        {
            return allocateAligned(size, alignment);
        }

        auto oldSize = getSizeAligned(ptr, alignment);
        if (size <= oldSize)
        {
            return ptr;
        }

        auto newPtr = allocateAligned(size, alignment);
        memcpy(newPtr, ptr, oldSize);
        deallocateAligned(ptr);
        return newPtr;
    }

    void IAlignedAllocator::deallocateAligned(void* ptr)
    {
        lock_(m_lock);
        auto info = getAllocationInfo(ptr);
        NAU_ASSERT(info.first != nullptr);
        if (info.first != nullptr)
        {
            this->deallocate(info.first->m_unaligned);
// TODO Tracy            TracyFreeN(info.first->m_unaligned, m_name.value().c_str());
            info.second->erase(ptr);
        }
    }

    size_t IAlignedAllocator::getSizeAligned(const void* ptr, size_t alignment) const
    {
        if (ptr == nullptr)
        {
            NAU_FAILURE("getSizeAligned for nullptr");
            return 0;
        }
        lock_(m_lock);
        auto info = getAllocationInfo(ptr);
        if (info.first != nullptr)
        {
            return info.first->m_size;
        }
        NAU_FAILURE("getSizeAligned for unaligned ptr");
        return 0;
    }

    bool IAlignedAllocator::isAligned(const void* ptr) const
    {
        lock_(m_lock);
        auto info = getAllocationInfo(ptr);
        return info.first != nullptr;
    }

    bool IAlignedAllocator::isValid(const void* ptr) const
    {
        return isAligned(ptr);
    }

    eastl::pair<IAlignedAllocator::AllocationInfo*, eastl::unordered_map<void*, IAlignedAllocator::AllocationInfo>*> IAlignedAllocator::getAllocationInfo(const void* ptr) const
    {
        // Retreive allocation info
        // First, look in current thread data
        // If not, check in other threads
        // Not thread safe
        IAlignedAllocator::AllocationInfo* info = nullptr;
        eastl::unordered_map<void*, IAlignedAllocator::AllocationInfo>* mapPtr = nullptr;
        if (m_allocations.value().contains(ptr))
        {
            info = &m_allocations.value()[const_cast<void*>(ptr)];
            mapPtr = &m_allocations.value();
        }
        else
        {
            m_allocations.visitAll([&info, &mapPtr, ptr](eastl::unordered_map<void*, AllocationInfo>& allocation)
            {
                if (allocation.contains(ptr))
                {
                    info = &allocation[const_cast<void*>(ptr)];
                    mapPtr = &allocation;
                }
            });
        }
        return {info, mapPtr};
    }

    const char* IAlignedAllocator::getName() const
    {
        return m_name.value().c_str();
    }

    void IAlignedAllocator::setName(const char* name)
    {
        m_name.value() = name;
    }

}  // namespace nau
