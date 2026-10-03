// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <memory>
#include <string>

#include "nau/memory/general_allocator.h"
#include "nau/memory/mem_allocator.h"
#include "nau/memory/nau_allocator_wrapper.h"
#include "nau/rtti/rtti_impl.h"

namespace nau
{
    const IMemAllocator::Ptr& getDefaultAllocator()
    {
        static IMemAllocator::Ptr defaultAlloc = std::make_shared<GeneralAllocator>();
        return defaultAlloc;
    }

    MemSectionPtr::MemSectionPtr(const std::string kind, MemSection* ptr) :
        m_ptr(ptr),
        m_kind(kind)
    {
    }

    MemSectionPtr::~MemSectionPtr()
    {
        HeapAllocator::instance().releaseSection(*this);
    }

    MemSectionPtr::MemSectionPtr(MemSectionPtr&& val) :
        m_ptr(val.m_ptr),
        m_kind(std::move(val.m_kind))
    {
        val.m_ptr = nullptr;
    }

    MemSectionPtr& MemSectionPtr::operator=(MemSectionPtr&& val)
    {
        m_ptr = val.m_ptr;
        m_kind = std::move(val.m_kind);
        val.m_ptr = nullptr;
        return *this;
    }

    MemSection* MemSectionPtr::operator->() const
    {
        return m_ptr;
    }

    [[nodiscard]]
    MemSection* MemSectionPtr::get() const
    {
        return m_ptr;
    }

    [[nodiscard]]
    bool MemSectionPtr::valid() const
    {
        return m_ptr != nullptr;
    }

    [[nodiscard]]
    const std::string& MemSectionPtr::getKind() const
    {
        return m_kind;
    }

    HeapAllocator& HeapAllocator::instance()
    {
        static auto* instance = new HeapAllocator;
        static RAIIFunction releaser(nullptr, [&]()
        {
            instance->m_readyToRelease = true;
            instance->releasePools();
        });

        return *instance;
    }

    MemSectionPtr HeapAllocator::getSection(const std::string& kind)
    {
        auto& localPool = freeMemSectionsPool();
        auto it = localPool.find(kind);
        if (it == localPool.end() || it->second.empty())
        {
            auto& memSection = getSectionsMap()[kind];
            memSection.m_inWork = true;
            return {kind, &memSection};
        }

        auto section = it->second.top();
        it->second.pop();
        section->m_inWork = true;
        return {it->first, section};
    }

    inline void HeapAllocator::releaseSection(MemSectionPtr& ptr)
    {
        if (m_readyToRelease)
        {
            ptr->m_inWork = false;
            releasePools();
        }
        else if (ptr.valid())
        {
            ptr->m_inWork = false;
            freeMemSectionsPool()[ptr.getKind()].push(ptr.get());
        }
    }

    HeapAllocator::MemSectionsMap& HeapAllocator::freeMemSectionsPool()
    {
        auto& val = m_freeSectionsPool.value();
        if (!val)
            val = std::make_shared<MemSectionsMap>();
        return *val;
    }

    HeapAllocator::SectionMap& HeapAllocator::getSectionsMap()
    {
        auto& val = m_freeSectionsMaps.value();
        if (!val)
        {
            val = std::make_shared<SectionMap>();
            lock_(m_sync);
            m_sections.insert(val);
        }
        return *val;
    }

    void HeapAllocator::releasePools()
    {
        bool canDestroy = true;
        {
            lock_(m_sync);

            m_freeSectionsMaps.visitAll([&canDestroy](auto& val)
            {
                for (auto it = val->begin(); it != val->end();)
                {
                    if (!it->second.m_inWork)
                    {
                        auto toRemove = it;
                        it++;
                        val->erase(toRemove);
                    }
                    else
                        it++;
                }

                canDestroy = canDestroy && val->empty();
            });
        }

        if (canDestroy)
            delete this;
    }

}  // namespace nau

void* operator new(size_t size, const nau::NauAllocatorWrapper& wraper)
{
    if (wraper.get())
        return wraper.get()->allocate(size);

    return malloc(size);
}
