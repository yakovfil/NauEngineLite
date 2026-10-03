// Copyright 2024 N-GINN LLC. All rights reserved.
// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <nau/dag_ioSys/dag_baseIo.h>
#include <nau/kernel/kernel_config.h>
#include <nau/memory/mem_allocator.h>

#include <span>
#include <vector>

namespace nau::iosys
{
    /// @addtogroup utility_classes
    /// @{

    /// @addtogroup serialization
    /// @{

    /// @file
    /// Serialization callbacks.

    /// Callback to write into dynamically allocated memory buffer.
    class NAU_KERNEL_EXPORT DynamicMemGeneralSaveCB : public IBaseSave
    {
    public:
        DynamicMemGeneralSaveCB(nau::IMemAllocator::Ptr _allocator, size_t sz = 0, size_t quant = 64 << 10);
        virtual ~DynamicMemGeneralSaveCB(void);

        void write(const void* ptr, int size);
        int tell(void);
        void seekto(int ofs);
        void seektoend(int ofs = 0);
        virtual const char* getTargetName()
        {
            return "(mem)";
        }
        virtual void flush()
        { /*noop*/
        }

        void resize(intptr_t sz);
        void setsize(intptr_t sz);

        /// Returns size of written buffer.
        inline intptr_t size(void) const
        {
            return datasize;
        }

        /// Returns pointer to buffer data.
        inline unsigned char* data(void)
        {
            return dataptr;
        }
        inline const unsigned char* data(void) const
        {
            return dataptr;
        }

        /// Returns pointer to copy of buffer data.
        unsigned char* copy(void);

    protected:
        unsigned char* dataptr;
        intptr_t datasize, data_avail, data_quant;
        intptr_t curptr;
        nau::IMemAllocator::Ptr allocator;
    };

    /// Constrained save to memory region (without any allocations and reallocations)
    /// if memory region size is exceeded during write operations exception is thrown
    class NAU_KERNEL_EXPORT ConstrainedMemSaveCB : public DynamicMemGeneralSaveCB
    {
    public:
        ConstrainedMemSaveCB(void* data, int sz) :
            DynamicMemGeneralSaveCB(nullptr, 0, 0)
        {
            setDestMem(data, sz);
        }
        ~ConstrainedMemSaveCB()
        {
            dataptr = nullptr;
        }
        void setDestMem(void* data, int sz)
        {
            dataptr = (unsigned char*)data;
            data_avail = sz;
            datasize = curptr = 0;
        }
    };

    /// Callback for reading from memory buffer. Allocates memory from #globmem allocator.
    class NAU_KERNEL_EXPORT MemGeneralLoadCB : public IBaseLoad
    {
    public:
        /// Allocates buffer and copies data to it.
        MemGeneralLoadCB(const void* ptr, int sz);
        virtual ~MemGeneralLoadCB(void);

        virtual void read(void* ptr, int size) override;
        virtual int tryRead(void* ptr, int size) override;
        virtual int tell(void) override;
        virtual void seekto(int ofs) override;
        virtual void seekrel(int ofs) override;
        virtual const char* getTargetName() override
        {
            return "(mem)";
        }
        int64_t getTargetDataSize() override
        {
            return datasize;
        }
        virtual std::span<const char> getTargetRomData() const override
        {
            return std::span<const char>((const char*)dataptr, datasize);
        }

        void close();

        /// Free buffer.
        void clear();

        /// Resize (reallocate) buffer.
        void resize(int sz);

        /// Returns buffer size.
        int size(void);

        /// Returns pointer to buffer data.
        const unsigned char* data(void);

        /// Returns pointer to copy of buffer data.
        unsigned char* copy(void);

    protected:
        const unsigned char* dataptr;
        int datasize;
        int curptr;
    };

    /// In-place (no copy) load from memory interface (fully inline).
    class NAU_KERNEL_EXPORT InPlaceMemLoadCB : public MemGeneralLoadCB
    {
    public:
        InPlaceMemLoadCB(const void* ptr, int sz) :
            MemGeneralLoadCB(NULL, 0)
        {
            dataptr = (const unsigned char*)ptr;
            datasize = sz;
        }
        ~InPlaceMemLoadCB()
        {
            dataptr = NULL;
        }

        const void* readAny(int sz)
        {
            const void* p = dataptr + curptr;
            seekrel(sz);
            return p;
        }

        void setSrcMem(const void* data, int sz)
        {
            dataptr = (const unsigned char*)data;
            datasize = sz;
            curptr = 0;
        }
    };

    enum class StreamDecompressResult
    {
        FAILED,
        FINISH,
        NEED_MORE_INPUT,
    };

    struct NAU_KERNEL_EXPORT IStreamDecompress
    {
        virtual ~IStreamDecompress()
        {
        }
        virtual StreamDecompressResult decompress(std::span<const uint8_t> in, std::vector<char>& out, size_t* nbytes_read = nullptr) = 0;
    };

    /// @}

    /// @}
}  // namespace nau::iosys