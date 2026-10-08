// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include <nau/dag_ioSys/dag_memIo.h>
#include <nau/utils/dag_baseDef.h>

#include <string>
#include <string_view>

#include "nau/diag/logging.h"
#include "nau/memory/mem_allocator.h"
#include "nau/string/format.h"
#include "nau/string/string_conv.h"

namespace nau::iosys
{
    DynamicMemGeneralSaveCB::DynamicMemGeneralSaveCB(nau::IMemAllocator::Ptr _allocator, size_t sz, size_t quant) :
        dataptr(NULL),
        datasize(0),
        data_avail(0),
        data_quant(quant),
        curptr(0),
        allocator(_allocator)
    {
        if(sz && allocator && data_quant)
            resize(sz);
    }

    DynamicMemGeneralSaveCB::~DynamicMemGeneralSaveCB(void)
    {
        if(dataptr && allocator)
        {
            allocator->deallocate(dataptr);
        }
        dataptr = NULL;
        datasize = 0;
        curptr = 0;
    }

    void DynamicMemGeneralSaveCB::write(const void* ptr, int size)
    {
        if(!size || !ptr)
            return;

        if(curptr + size > data_avail)
            resize(curptr + size);
        memcpy(dataptr + curptr, ptr, size);
        curptr += size;
        if(curptr > datasize)
            datasize = curptr;
    }

    int DynamicMemGeneralSaveCB::tell(void)
    {
        if(curptr < 0 || curptr > datasize)
            NAU_THROW(SaveException("invalid curptr", 0));
        return curptr;
    }

    void DynamicMemGeneralSaveCB::seekto(int ofs)
    {
        if(ofs < 0 || ofs > datasize)
            NAU_THROW(SaveException("seek pos out of range", tell()));
        curptr = ofs;
    }

    void DynamicMemGeneralSaveCB::seektoend(int ofs)
    {
        if(datasize + ofs < 0 || ofs > 0)
            NAU_THROW(SaveException("seek pos out of range", tell()));
        curptr = datasize + ofs;
    }

    void DynamicMemGeneralSaveCB::resize(intptr_t sz)
    {
        if(data_quant > 0)
            sz = (sz + data_quant - 1) / data_quant * data_quant;
        if(!allocator)
        {
            if(!sz)
                datasize = 0;
            if(sz <= data_avail)
                return;

            NAU_LOG_ERROR(nau::utils::format(nau::strings::toUtf8(u8"{}: cannot resize from data_avail={} to {} due to realloc prohibited"), std::string(__FUNCTION__), data_avail, sz));
            NAU_THROW(SaveException("cannot enlarge dest data size", tell()));
            return;
        }

        if(!sz)
        {
            if(dataptr)
            {
                allocator->deallocate(dataptr);
                dataptr = NULL;
            }
            curptr = 0;
            datasize = 0;
            data_avail = 0;
            return;
        }
        NAU_ASSERT(sz >= datasize);

        if(sz == data_avail)
            return;
        // if (!allocator->resizeInplace(dataptr, sz))
        dataptr = (unsigned char*)allocator->reallocate(dataptr, sz);
        data_avail = sz;
        if(datasize > data_avail)
            datasize = data_avail;
        if(curptr > datasize)
            curptr = datasize;
    }

    void DynamicMemGeneralSaveCB::setsize(intptr_t sz)
    {
        if(sz < 0)
            NAU_THROW(SaveException("size out of range", sz));

        if(sz > data_avail)
            resize(sz);
        NAU_ASSERT(sz <= data_avail);

        if(curptr > sz)
            curptr = sz;
        datasize = sz;
    }

    unsigned char* DynamicMemGeneralSaveCB::copy(void)
    {
        if(!size() || !allocator)
            return NULL;

        unsigned char* buf = reinterpret_cast<unsigned char*>(allocator->allocate(datasize));
        memcpy(buf, dataptr, datasize);
        return buf;
    }

    MemGeneralLoadCB::MemGeneralLoadCB(const void* ptr, int sz) :
        dataptr(NULL),
        datasize(0),
        curptr(0)
    {
        if(sz)
        {
            resize(sz);
            memcpy((void*)dataptr, ptr, sz);
        }
    }

    MemGeneralLoadCB::~MemGeneralLoadCB(void)
    {
        if(dataptr)
        {
            // memfree((void *)dataptr, globmem);
            nau::getDefaultAllocator()->deallocate(reinterpret_cast<void*>(const_cast<unsigned char*>(dataptr)));
            dataptr = nullptr;
        }
        datasize = 0;
        curptr = 0;
    }

    void MemGeneralLoadCB::read(void* ptr, int size)
    {
        if(!size || !ptr)
            return;
        if(curptr + size > datasize)
            NAU_THROW(LoadException("read error", tell()));
        memcpy(ptr, dataptr + curptr, size);
        curptr += size;
    }
    int MemGeneralLoadCB::tryRead(void* ptr, int size)
    {
        if(!size || !ptr)
            return 0;
        if(curptr + size > datasize)
            size = datasize - curptr;
        memcpy(ptr, dataptr + curptr, size);
        curptr += size;
        return size;
    }

    int MemGeneralLoadCB::tell(void)
    {
        if(curptr < 0 || curptr > datasize)
            NAU_THROW(LoadException("invalid curptr", 0));
        return curptr;
    }

    void MemGeneralLoadCB::seekto(int ofs)
    {
        if(ofs < 0 || ofs > datasize)
            NAU_THROW(LoadException("seek ofs out of range", this->tell()));
        curptr = ofs;
    }

    void MemGeneralLoadCB::seekrel(int ofs)
    {
        if(curptr + ofs < 0 || curptr + ofs > datasize)
            NAU_THROW(LoadException("seek ofs out of range", this->tell()));
        curptr += ofs;
    }

    void MemGeneralLoadCB::close(void)
    {
        NAU_ASSERT((dataptr && datasize) || (!dataptr && !datasize));
    }

    void MemGeneralLoadCB::clear(void)
    {
        NAU_ASSERT((dataptr && datasize) || (!dataptr && !datasize));
        if(dataptr && datasize)
        {
            // memfree((void *)dataptr, globmem);
            nau::getDefaultAllocator()->deallocate(reinterpret_cast<void*>(const_cast<unsigned char*>(dataptr)));
            dataptr = nullptr;
            datasize = 0;
        }
        curptr = 0;
    }

    void MemGeneralLoadCB::resize(int sz)
    {
        NAU_ASSERT((dataptr && datasize) || (!dataptr && !datasize));

        if(!sz)
        {
            clear();
            return;
        }

        // unsigned char *data = (unsigned char *)memalloc(sz, globmem);
        unsigned char* data = reinterpret_cast<unsigned char*>(nau::getDefaultAllocator()->allocate(sz));

        if(dataptr)
        {
            if(datasize < sz)
            {
                memcpy(data, dataptr, datasize);
            }
            else
            {
                memcpy(data, dataptr, sz);
                if(curptr > sz)
                    curptr = sz;
            }
            // memfree((void *)dataptr, globmem);
            nau::getDefaultAllocator()->deallocate(reinterpret_cast<void*>(const_cast<unsigned char*>(dataptr)));
            dataptr = NULL;
        }

        dataptr = data;
        datasize = sz;
    }

    int MemGeneralLoadCB::size(void)
    {
        return datasize;
    }

    const unsigned char* MemGeneralLoadCB::data(void)
    {
        return dataptr;
    }

    unsigned char* MemGeneralLoadCB::copy(void)
    {
        if(!size())
            return NULL;
        // unsigned char *data = (unsigned char *)memalloc(datasize, globmem);
        unsigned char* data = reinterpret_cast<unsigned char*>(nau::getDefaultAllocator()->allocate(datasize));
        memcpy(data, dataptr, datasize);
        return data;
    }
}  // namespace nau::iosys