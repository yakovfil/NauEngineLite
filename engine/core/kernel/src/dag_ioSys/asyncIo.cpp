// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include <nau/dag_ioSys/dag_asyncIo.h>
#include <nau/osApiWrappers/dag_asyncRead.h>
// #include <nau/osApiWrappers/dag_miscApi.h>
// #include <debug/dag_debug.h>
#include <stdio.h>

#include <string>
#include <string_view>

#include "nau/debug/dag_except.h"
#include "nau/diag/logging.h"
#include "nau/memory/mem_allocator.h"
#include "nau/string/format.h"
#include "nau/string/string_conv.h"
namespace nau::iosys
{
    extern void sleep_msec_ex(int ms);

    // generic load interface implemented as async reader
    AsyncLoadCB::AsyncLoadCB(const char* realname)
    {
        memset(&file, 0, sizeof(file));
        memset(&buf, 0, sizeof(buf));
        file.pos = -1;
        file.size = -1;

        buf.size = 64 << 10;

        file.pos = 0;
        // NAU_CORE_DEBUG_LF(nau::utils::format(nau::strings::toUtf8(u8"load: {}"), std::string(fpath));
        buf.minimumChunk = nau::hal::dfa_chunk_size(realname);
        if(buf.minimumChunk)
        {
            buf.size = buf.minimumChunk * 128;
            if(buf.size > (128 << 10))
            {
                buf.size = 128 << 10;
                if(buf.size < buf.minimumChunk)
                    buf.size = buf.minimumChunk;
            }
            // NAU_CORE_DEBUG_LF("sectorSize = {}", bytesPerSector);
            file.handle = nau::hal::dfa_open_for_read(realname, true);
        }
        else
        {
            // NAU_CORE_DEBUG_FI_LF("!getFreeSize");
            buf.minimumChunk = 1;
            buf.size = 64 << 10;
            file.handle = nau::hal::dfa_open_for_read(realname, false);
        }

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

        // if ( buf.size > file.size ) buf.size = file.size;
        // buf.data = (char *)tmpmem->allocAligned(buf.size, buf.minimumChunk);
        buf.data = reinterpret_cast<char*>(nau::getDefaultAllocator()->allocateAligned(buf.size, buf.minimumChunk));
    }

    AsyncLoadCB::~AsyncLoadCB()
    {
        if(buf.data)
        {
            // tmpmem->freeAligned(buf.data);
            nau::getDefaultAllocator()->deallocateAligned(buf.data);
        }
        if(file.handle)
            nau::hal::dfa_close(file.handle);

        buf.data = NULL;
        file.handle = NULL;
    }

    void AsyncLoadCB::readBuffered(void* ptr, int size)
    {
        if(!size)
            return;
        if(file.pos + size > file.size)
        {
            NAU_LOG_DEBUG(u8"{} {} read({},{}), file.size={}, file.pos={}", std::string(__FILE__), __LINE__, ptr, size, file.size, file.pos);
            NAU_THROW(LoadException("eof", file.pos));
        }
        if(file.pos >= buf.pos && file.pos < buf.pos + buf.used)
        {
            int sz;

            if(file.pos + size < buf.pos + buf.used)
                sz = size;
            else
                sz = buf.pos + buf.used - file.pos;
            // NAU_CORE_DEBUG_LF( "read to {:p}  sz={} (from {:p})", ptr, sz, file.pos );
            memcpy(ptr, buf.data + file.pos - buf.pos, sz);

            file.pos += sz;
            ptr = ((char*)ptr) + sz;
            size -= sz;
            // measure.stop();
            // NAU_CORE_DEBUG_LF("memcpy {}", (int)measure.getTotal());
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
            {
                sleep_msec_ex(1);
            }
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
        // measure.go();
        int asyncdata_handle = nau::hal::dfa_alloc_asyncdata();
        while(asyncdata_handle < 0)
        {
            sleep_msec_ex(1);
            asyncdata_handle = nau::hal::dfa_alloc_asyncdata();
        }
        // measure.stop();
        // NAU_CORE_DEBUG_LF("alloc {}", (int)measure.getTotal());

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
        {
            sleep_msec_ex(1);
        }

        nau::hal::dfa_free_asyncdata(asyncdata_handle);
        if(lres != buf.used)
        {
            // NAU_CORE_DEBUG_FI_LF("read({:p},{}), file.size={}, file.pos={}", ptr, size, file.size, file.pos);
            NAU_THROW(LoadException("incomplete read", file.pos));
        }

        // NAU_CORE_DEBUG_LF( "read to {:p}  sz={} (from {:p})", ptr, size, file.pos );
        memcpy(ptr, buf.data, size);
        file.pos += size;
    }

    void AsyncLoadCB::read(void* ptr, int size)
    {
        if(buf.minimumChunk == 1)
        {
            readBuffered(ptr, size);
            return;
        }
        if(file.pos + size > file.size)
        {
            NAU_LOG_DEBUG(nau::utils::format(nau::strings::toUtf8(u8"{} {} read({},{}), file.size={}, file.pos={}"), std::string(__FILE__), __LINE__, ptr, size, file.size, file.pos));
            NAU_THROW(LoadException("eof", file.pos));
        }

        if(file.pos >= buf.pos && file.pos < buf.pos + buf.used)
        {
            int sz;

            if(file.pos + size < buf.pos + buf.used)
                sz = size;
            else
                sz = buf.pos + buf.used - file.pos;
            // memcpy ( ptr, buf.data+file.pos-buf.pos, sz );
            memcpy(ptr, buf.data + file.pos - buf.pos, sz);

            file.pos += sz;
            ptr = ((char*)ptr) + sz;
            size -= sz;
        }
        if(!size)
            return;

        int posStart = file.pos;
        int readSize = size;
        int chunkBits = (buf.minimumChunk - 1);
        if(file.pos & chunkBits)
        {
            posStart = file.pos & (~chunkBits);
            readSize += file.pos - posStart;
        }

        while(readSize > buf.size)
        {
            int asyncdata_handle = nau::hal::dfa_alloc_asyncdata();
            while(asyncdata_handle < 0)
            {
                sleep_msec_ex(1);
                asyncdata_handle = nau::hal::dfa_alloc_asyncdata();
            }

            if(!nau::hal::dfa_read_async(file.handle, asyncdata_handle, posStart, buf.data, buf.size))
            {
                nau::hal::dfa_free_asyncdata(asyncdata_handle);
                NAU_THROW(LoadException("can't place read request", file.pos));
                return;
            }

            int lres;
            while(!nau::hal::dfa_check_complete(asyncdata_handle, &lres))
            {
                sleep_msec_ex(0);
            }
            nau::hal::dfa_free_asyncdata(asyncdata_handle);

            if(lres != buf.size)
            {
                NAU_LOG_DEBUG(nau::utils::format(nau::strings::toUtf8(u8"{} {} read({:p},{}), file.size={}, file.pos={}"), std::string(__FILE__), __LINE__, ptr, buf.size, file.size, file.pos));
                NAU_THROW(LoadException("incomplete read", file.pos));
            }

            int offset = file.pos - posStart;
            posStart += buf.size;
            file.pos = posStart;
            memcpy(ptr, buf.data + offset, buf.size - offset);
            ptr = (char*)ptr + buf.size - offset;
            readSize -= buf.size;
        }

        if(!readSize)
            return;

        int sizeLeft = file.size - posStart;
        if(sizeLeft > buf.size)
            sizeLeft = buf.size;
        else if(sizeLeft & chunkBits)
        {
            sizeLeft = buf.minimumChunk + (sizeLeft & (~chunkBits));
        }

        int asyncdata_handle = nau::hal::dfa_alloc_asyncdata();
        while(asyncdata_handle < 0)
        {
            sleep_msec_ex(1);
            asyncdata_handle = nau::hal::dfa_alloc_asyncdata();
        }

        if(!nau::hal::dfa_read_async(file.handle, asyncdata_handle, posStart, buf.data, sizeLeft))
        {
            nau::hal::dfa_free_asyncdata(asyncdata_handle);
            NAU_THROW(LoadException("can't place read request", file.pos));
            return;
        }

        int lres;
        while(!nau::hal::dfa_check_complete(asyncdata_handle, &lres))
        {
            sleep_msec_ex(0);
        }
        nau::hal::dfa_free_asyncdata(asyncdata_handle);

        if(lres != sizeLeft && lres != file.size - posStart)
        {  // readSize - useLessData
            NAU_LOG_DEBUG(nau::utils::format(nau::strings::toUtf8(u8"{} {} {} read({}, size={} readSize={}), file.size={}, file.pos={} posStart={}"), std::string(__FILE__), __LINE__, lres, ptr, size, sizeLeft, file.size,
                                             file.pos, posStart));
            NAU_THROW(LoadException("incomplete read", file.pos));
        }
        buf.pos = posStart;
        buf.used = lres;

        int offset = file.pos - posStart;
        file.pos = posStart + readSize;
        memcpy(ptr, buf.data + offset, readSize - offset);
    }

    int AsyncLoadCB::tryRead(void* ptr, int size)
    {
        if(file.pos + size > file.size)
            size = file.size - file.pos;
        if(size <= 0)
            return 0;
        NAU_TRY
        {
            AsyncLoadCB::read(ptr, size);
        }
        NAU_CATCH(LoadException)
        {
            return 0;
        }

        return size;
    }

    int AsyncLoadCB::tell()
    {
        return file.pos;
    }

    void AsyncLoadCB::seekto(int pos)
    {
        if(pos < 0 || pos > file.size)
        {
            NAU_LOG_DEBUG(nau::utils::format(nau::strings::toUtf8(u8"{} {} seekto({}), file.size={}, file.pos={}"), std::string(__FILE__), __LINE__, pos, file.size, file.pos));
            NAU_THROW(LoadException("seek out of range", file.pos));
        }
        // NAU_CORE_DEBUG_FI_LF ( "seekto(%d), file.size=%d, file.pos=%d", pos, file.size, file.pos );
        file.pos = pos;
    }

    void AsyncLoadCB::seekrel(int ofs)
    {
        seekto(file.pos + ofs);
    }

}  // namespace nau::iosys