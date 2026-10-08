// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include <nau/dag_ioSys/dag_asyncIoCached.h>
#include <nau/osApiWrappers/dag_asyncRead.h>
// #include <nau/osApiWrappers/dag_miscApi.h>
// #include <debug/dag_debug.h>
#include <nau/diag/logging.h>
#include <nau/memory/mem_allocator.h>
#include <stdio.h>

#include <string>
#include <string_view>

#include "nau/string/format.h"
#include "nau/string/string_conv.h"
namespace nau::iosys
{
    extern void sleep_msec_ex(int ms);

    // generic load interface implemented as async reader
    AsyncLoadCachedCB::AsyncLoadCachedCB(const char* realname)
    {
        memset(&file, 0, sizeof(file));
        memset(&buf, 0, sizeof(buf));
        file.pos = -1;
        file.size = -1;

        file.handle = nau::hal::dfa_open_for_read(realname, false);
        targetFilename = realname;
        if(!file.handle)
            return;

        file.size = nau::hal::dfa_file_length(file.handle);
        if(file.size <= 0)
        {
            nau::hal::dfa_close(file.handle);
            file.handle = NULL;
            return;
        }

        file.pos = 0;
        buf.size = 128 << 10;
        if(buf.size > file.size)
            buf.size = file.size;

        // buf.data = (char *)memalloc(buf.size, tmpmem);
        buf.data = reinterpret_cast<char*>(nau::getDefaultAllocator()->allocate(buf.size));
    }
    AsyncLoadCachedCB::~AsyncLoadCachedCB()
    {
        if(buf.data)
        {
            // memfree(buf.data, tmpmem);
            nau::getDefaultAllocator()->deallocate(buf.data);
        }
        if(file.handle)
            nau::hal::dfa_close(file.handle);

        buf.data = NULL;
        file.handle = NULL;
    }

    void AsyncLoadCachedCB::read(void* ptr, int size)
    {
        if(!size)
            return;
        if(file.pos + size > file.size)
        {
            NAU_LOG_DEBUG(nau::utils::format(nau::strings::toUtf8(u8"{} {} read({},{}), file.size={}, file.pos={}"), std::string(__FILE__), __LINE__), ptr, size, file.size, file.pos);
            NAU_THROW(LoadException("eof", file.pos));
        }

        if(file.pos >= buf.pos && file.pos < buf.pos + buf.used)
        {
            int sz;

            if(file.pos + size < buf.pos + buf.used)
                sz = size;
            else
                sz = buf.pos + buf.used - file.pos;
            NAU_LOG_DEBUG(nau::utils::format(nau::strings::toUtf8(u8"read to {}  sz={} (from {})"), ptr, sz, file.pos));
            memcpy(ptr, buf.data + file.pos - buf.pos, sz);

            file.pos += sz;
            ptr = ((char*)ptr) + sz;
            size -= sz;
        }

        if(!size)
            return;

        if(size > buf.size)
        {
            // read to data ptr immediately
            int asyncdata_handle = nau::hal::dfa_alloc_asyncdata();
            while(asyncdata_handle < 0)
            {
                sleep_msec_ex(1);
                asyncdata_handle = nau::hal::dfa_alloc_asyncdata();
            }

            if(!nau::hal::dfa_read_async(file.handle, asyncdata_handle, file.pos, ptr, size))
            {
                nau::hal::dfa_free_asyncdata(asyncdata_handle);
                NAU_THROW(LoadException("can't place read request", file.pos));
                return;
            }
            sleep_msec_ex(0);

            int lres;
            while(!nau::hal::dfa_check_complete(asyncdata_handle, &lres))
                sleep_msec_ex(1);
            nau::hal::dfa_free_asyncdata(asyncdata_handle);

            if(lres != size)
            {
                NAU_LOG_DEBUG(nau::utils::format(nau::strings::toUtf8(u8"{} {} read({},{}), file.size={}, file.pos={}"), std::string(__FILE__), __LINE__, ptr, size, file.size, file.pos));
                NAU_THROW(LoadException("incomplete read", file.pos));
            }

            file.pos += size;
            return;
        }

        // read to buffer and copy to data ptr
        int asyncdata_handle = nau::hal::dfa_alloc_asyncdata();
        while(asyncdata_handle < 0)
        {
            sleep_msec_ex(1);
            asyncdata_handle = nau::hal::dfa_alloc_asyncdata();
        }

        if(file.pos + buf.size > file.size)
            buf.used = file.size - file.pos;
        else
            buf.used = buf.size;
        buf.pos = file.pos;

        if(!nau::hal::dfa_read_async(file.handle, asyncdata_handle, buf.pos, buf.data, buf.used))
        {
            nau::hal::dfa_free_asyncdata(asyncdata_handle);
            NAU_THROW(LoadException("can't place read request", file.pos));
            return;
        }
        sleep_msec_ex(0);

        int lres;
        while(!nau::hal::dfa_check_complete(asyncdata_handle, &lres))
            sleep_msec_ex(1);

        nau::hal::dfa_free_asyncdata(asyncdata_handle);
        if(lres != buf.used)
        {
            NAU_LOG_DEBUG(nau::utils::format(nau::strings::toUtf8(u8"{} {} read({},{}), file.size={}, file.pos={}"), std::string(__FILE__), __LINE__, ptr, size, file.size, file.pos));
            NAU_THROW(LoadException("incomplete read", file.pos));
        }

        NAU_LOG_DEBUG(nau::utils::format(nau::strings::toUtf8(u8"read to {}  sz={} (from {})"), ptr, size, file.pos));
        memcpy(ptr, buf.data, size);
        file.pos += size;
    }
    int AsyncLoadCachedCB::tryRead(void* ptr, int size)
    {
        if(file.pos + size > file.size)
            size = file.size - file.pos;
        if(size <= 0)
            return 0;
        NAU_TRY
        {
            AsyncLoadCachedCB::read(ptr, size);
        }
        NAU_CATCH(LoadException)
        {
            return 0;
        }

        return size;
    }
    int AsyncLoadCachedCB::tell()
    {
        return file.pos;
    }
    void AsyncLoadCachedCB::seekto(int pos)
    {
        if(pos < 0 || pos > file.size)
        {
            NAU_LOG_DEBUG(nau::utils::format(nau::strings::toUtf8(u8"{} {} seekto({}), file.size={}, file.pos={}"), std::string(__FILE__), __LINE__, pos, file.size, file.pos));
            NAU_THROW(LoadException("seek out of range", file.pos));
        }
        file.pos = pos;
    }
    void AsyncLoadCachedCB::seekrel(int ofs)
    {
        seekto(file.pos + ofs);
    }
}  // namespace dag