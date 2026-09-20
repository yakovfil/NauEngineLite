// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/memory/general_allocator.h"

#include "nau/memory/fixed_blocks.h"

namespace nau
{
    struct alignas(std::max_align_t) BlockHeader
    {
        IMemAllocator* allocator = nullptr;
        size_t size = 0;
    };
    using BlockHeaderPtr = BlockHeader*;
    using ConstBlockHeaderPtr = const BlockHeader*;
    using BytePtr = unsigned char*;
    using ConstBytePtr = const unsigned char*;

    void* GeneralAllocator::allocate(size_t size)
    {
        auto realSize = size + sizeof(BlockHeader);
        IMemAllocator* allocator = nullptr;

        if (realSize <= 32) allocator = &FixedBlocksAllocator<32>::instance();
        else if (realSize <= 64) allocator = &FixedBlocksAllocator<64>::instance();
        else if (realSize <= 128) allocator = &FixedBlocksAllocator<128>::instance();
        else if (realSize <= 256) allocator = &FixedBlocksAllocator<256>::instance();
        else if (realSize <= 512) allocator = &FixedBlocksAllocator<512>::instance();
        else if (realSize <= 1024) allocator = &FixedBlocksAllocator<1024>::instance();
        else allocator = &ArrayAllocator::instance();

        auto ptr = allocator->allocate(realSize);
        auto header = static_cast<BlockHeaderPtr>(ptr);
        header->allocator = allocator;
        header->size = size;
        void* retVal = static_cast<BytePtr>(ptr) + sizeof(BlockHeader);
        return retVal;
    }

    void* GeneralAllocator::reallocate(void* ptr, size_t size)
    {
        if (!ptr)
            return allocate(size);

        void* realPtr = static_cast<BytePtr>(ptr) - sizeof(BlockHeader);
        auto header = static_cast<BlockHeaderPtr>(realPtr);

        if (size < header->size)
            return ptr;

        auto realSize = size + sizeof(BlockHeader);
        IMemAllocator* allocator = nullptr;
        if (realSize <= 32) allocator = &FixedBlocksAllocator<32>::instance();
        else if (realSize <= 64) allocator = &FixedBlocksAllocator<64>::instance();
        else if (realSize <= 128) allocator = &FixedBlocksAllocator<128>::instance();
        else if (realSize <= 256) allocator = &FixedBlocksAllocator<256>::instance();
        else if (realSize <= 512) allocator = &FixedBlocksAllocator<512>::instance();
        else if (realSize <= 1024) allocator = &FixedBlocksAllocator<1024>::instance();
        else allocator = &ArrayAllocator::instance();
        
        void* newPtr = nullptr;
        if (header->allocator == allocator)
        {
            newPtr = allocator->reallocate(realPtr, realSize);
            header = static_cast<BlockHeaderPtr>(newPtr);
        }
        else
        {
            newPtr = allocator->allocate(realSize);
            memcpy(static_cast<BytePtr>(newPtr) + sizeof(BlockHeader), ptr, header->size);
            header->allocator->deallocate(realPtr);

            header = static_cast<BlockHeaderPtr>(newPtr);
            header->allocator = allocator;
        }

        header->size = size;
        return static_cast<BytePtr>(newPtr) + sizeof(BlockHeader);
    }

    void GeneralAllocator::deallocate(void* ptr)
    {
        void* realPtr = static_cast<BytePtr>(ptr) - sizeof(BlockHeader);
        auto allocator = static_cast<BlockHeaderPtr>(realPtr)->allocator;
        allocator->deallocate(realPtr);
    }

    size_t GeneralAllocator::getSize(const void* ptr) const 
    {
        if (!ptr)
            return 0;

        const void* realPtr = static_cast<ConstBytePtr>(ptr) - sizeof(BlockHeader);
        auto header = static_cast<ConstBlockHeaderPtr>(realPtr);
        return header->size;
    }

}
