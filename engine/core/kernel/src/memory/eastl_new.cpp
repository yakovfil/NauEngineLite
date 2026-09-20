// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


// clang-format off

#if __has_include(<EASTL/allocator.h>)

#include <EASTL/allocator.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>

#if !defined(EASTL_USER_DEFINED_ALLOCATOR)

#if !EASTL_DLL

void* operator new[](size_t size, [[maybe_unused]] const char* pName, [[maybe_unused]] int flags, [[maybe_unused]] unsigned debugFlags, [[maybe_unused]] const char* file, [[maybe_unused]] int line)
    {
        return ::malloc(size);
    };

void* operator new[](size_t size, size_t alignment, [[maybe_unused]] size_t alignmentOffset, [[maybe_unused]] const char* pName, [[maybe_unused]] int flags, [[maybe_unused]] unsigned debugFlags, [[maybe_unused]] const char* file, [[maybe_unused]] int line)
    {
        return ::_aligned_malloc(size, alignment);
        // return ::malloc(size);
    };

#endif  // !EASTL_DLL

#else

namespace
{
    struct AllocationHeader
    {
        void* base;
        size_t size;
        size_t alignment;
        size_t offset;
    };

    AllocationHeader readAllocationHeader(void* ptr)
    {
        AllocationHeader header;
        // An alignment offset can leave the header itself unaligned.
        std::memcpy(&header, static_cast<unsigned char*>(ptr) - sizeof(header), sizeof(header));
        return header;
    }

    void* allocateAligned(size_t size, size_t alignment, size_t offset)
    {
        if (alignment == 0 || (alignment & (alignment - 1)) != 0)
        {
            return nullptr;
        }
        constexpr size_t maxSize = std::numeric_limits<size_t>::max();
        if (alignment - 1 > maxSize - sizeof(AllocationHeader))
        {
            return nullptr;
        }
        const size_t overhead = sizeof(AllocationHeader) + alignment - 1;
        if (size > maxSize - overhead)
        {
            return nullptr;
        }
        void* base = std::malloc(size + overhead);
        if (!base)
        {
            return nullptr;
        }
        auto* first = static_cast<unsigned char*>(base) + sizeof(AllocationHeader);
        // The bundled EASTL checks that pointer - offset is aligned.
        const size_t padding = (offset - reinterpret_cast<uintptr_t>(first)) & (alignment - 1);
        auto* ptr = first + padding;
        const AllocationHeader header{base, size, alignment, offset};
        std::memcpy(ptr - sizeof(header), &header, sizeof(header));
        return ptr;
    }
}

namespace eastl
{

    allocator::allocator(const char* EASTL_NAME(pName))
    {
#if EASTL_NAME_ENABLED
        mpName = pName ? pName : EASTL_ALLOCATOR_DEFAULT_NAME;
#endif
    }

    allocator::allocator(const allocator& EASTL_NAME(alloc))
    {
#if EASTL_NAME_ENABLED
        mpName = alloc.mpName;
#endif
    }

    allocator::allocator(const allocator&, const char* EASTL_NAME(pName))
    {
#if EASTL_NAME_ENABLED
        mpName = pName ? pName : EASTL_ALLOCATOR_DEFAULT_NAME;
#endif
    }

    allocator& allocator::operator=(const allocator& EASTL_NAME(alloc))
    {
#if EASTL_NAME_ENABLED
        mpName = alloc.mpName;
#endif
        return *this;
    }

    const char* allocator::get_name() const
    {
#if EASTL_NAME_ENABLED
        return mpName;
#else
        return EASTL_ALLOCATOR_DEFAULT_NAME;
#endif
    }

    void allocator::set_name(const char* EASTL_NAME(pName))
    {
#if EASTL_NAME_ENABLED
        mpName = pName;
#endif
    }

    void* allocator::allocate(size_t n, int flags)
    {
        return allocateAligned(n, alignof(std::max_align_t), 0);
    }

    void* allocator::realloc(void* p, size_t n, int flags)
    {
        if (!p)
        {
            return allocate(n, flags);
        }
        const AllocationHeader header = readAllocationHeader(p);
        if (n == 0)
        {
            std::free(header.base);
            return nullptr;
        }
        void* replacement = allocateAligned(n, header.alignment, header.offset);
        if (replacement)
        {
            std::memcpy(replacement, p, n < header.size ? n : header.size);
            std::free(header.base);
        }
        return replacement;
    }

    void* allocator::allocate(size_t n, size_t alignment, size_t offset, int flags)
    {
        return allocateAligned(n, alignment, offset);
    }

    void allocator::deallocate(void* p, size_t)
    {
        if (p)
        {
            std::free(readAllocationHeader(p).base);
        }
    }

    bool operator==(const allocator& a, const allocator& b)
    {
        return true;
    }

    bool operator!=(const allocator& a, const allocator& b)
    {
        return false;
    }

  /// gDefaultAllocator
  /// Default global allocator instance. 
  NAU_KERNEL_EXPORT allocator  gDefaultAllocator;
  NAU_KERNEL_EXPORT allocator* gpDefaultAllocator = &gDefaultAllocator;

  NAU_KERNEL_EXPORT allocator* GetDefaultAllocator()
  {
    return gpDefaultAllocator;
  }

  NAU_KERNEL_EXPORT allocator* SetDefaultAllocator(allocator* pAllocator)
  {
    allocator* const pPrevAllocator = gpDefaultAllocator;
    gpDefaultAllocator = pAllocator;
    return pPrevAllocator;
  }

}  // namespace eastl
#endif
#endif

namespace EA
{
    namespace StdC
    {
        NAU_KERNEL_EXPORT int Vsnprintf(char* EA_RESTRICT pDestination, size_t n, const char* EA_RESTRICT pFormat, va_list arguments)
        {
            return vsnprintf(pDestination, n, pFormat, arguments);
        };
    }
}





// clang-format on
