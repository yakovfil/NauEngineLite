// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

// #include <util/dag_globDef.h>
#include <nau/dag_ioSys/dag_genIo.h>
#include <nau/dag_ioSys/dag_zlibIo.h>
#include <nau/osApiWrappers/dag_fileIoErr.h>

#include <string>
#include <string_view>

#include "nau/debug/dag_except.h"
#include "nau/diag/assertion.h"
#include "nau/diag/logging.h"
#include "nau/memory/mem_allocator.h"
#include "nau/string/format.h"
#include "nau/string/string_conv.h"
#include "zlibInline.h"

namespace nau::iosys
{
    //-----------------------------------------------------------------------
    // ZlibLoadCB
    //-----------------------------------------------------------------------
    void ZlibLoadCB::open(IGenLoad& in_crd, int in_size, bool raw_inflate)
    {
        NAU_ASSERT(!loadCb && "already opened?");
        NAU_STATIC_ASSERT(SIZE_OF_Z_STREAM >= sizeof(z_stream));

        NAU_ASSERT(in_size > 0);
        loadCb = &in_crd;
        inBufLeft = in_size;
        isStarted = false;
        isFinished = false;
        rawInflate = raw_inflate;
        ((z_stream*)&strm)->zalloc = Z_NULL;
        ((z_stream*)&strm)->zfree = Z_NULL;
        //((z_stream *)&strm)->opaque = tmpmem;
        ((z_stream*)&strm)->next_in = Z_NULL;
        ((z_stream*)&strm)->avail_in = 0;
    }
    void ZlibLoadCB::close()
    {
        NAU_ASSERT(isFinished || !isStarted);
        ceaseReading();
        loadCb = NULL;
        inBufLeft = 0;
        isStarted = false;
        isFinished = false;
    }

    unsigned ZlibLoadCB::fetchInput(void* handle, void* strm)
    {
        ZlibLoadCB& zcrd = *(ZlibLoadCB*)handle;
        int sz = zcrd.inBufLeft;
        NAU_ASSERT(sz >= 0);

        if(sz > ZLIB_LOAD_BUFFER_SIZE)
            sz = ZLIB_LOAD_BUFFER_SIZE;

        if(sz <= 0)
            return 0;
        sz = zcrd.loadCb->tryRead(zcrd.buffer, sz);
        NAU_ASSERT(!zcrd.fatalErrors || sz > 0);
        ((z_stream*)strm)->next_in = zcrd.buffer;
        ((z_stream*)strm)->avail_in = sz;
        zcrd.inBufLeft -= sz;
        return sz;
    }

    inline int ZlibLoadCB::tryReadImpl(void* ptr, int size)
    {
        if(!size || isFinished)
            return 0;

        if(!isStarted)
        {
            int err = rawInflate ? inflateInit2((z_stream*)&strm, -MAX_WBITS) : inflateInit((z_stream*)&strm);
            if(err != Z_OK)
            {
                if(fatalErrors)
                {
                    if(hal::dag_on_zlib_error_cb)
                        hal::dag_on_zlib_error_cb(getTargetName(), 0x10000 | ((unsigned)err & 0xFF));
                    NAU_FAILURE("zlib error {} in {}\nsource: '{}'\n", err, "inflateInit", getTargetName());
                }
                return -1;
            }
            isStarted = true;
        }

        ((z_stream*)&strm)->avail_out = size;
        ((z_stream*)&strm)->next_out = (Bytef*)ptr;

        int res = inflateEx((z_stream*)&strm, Z_SYNC_FLUSH, (in_fetch_func)fetchInput, this);

        if(res != Z_OK && res != Z_STREAM_END)
        {
            if(fatalErrors)
            {
                if(hal::dag_on_zlib_error_cb)
                    hal::dag_on_zlib_error_cb(getTargetName(), 0x20000 | ((unsigned)res & 0xFF));

                NAU_FAILURE("zlib error {} ({}) in {}\nsource: '{}'\n", res, ((z_stream*)&strm)->msg, "inflate", getTargetName());
            }
            return -1;
        }
        size -= ((z_stream*)&strm)->avail_out;

        if(res == Z_STREAM_END && !ceaseReading())
            return -1;

        return size;
    }

    int ZlibLoadCB::tryRead(void* ptr, int size)
    {
        int rd_sz = ZlibLoadCB::tryReadImpl(ptr, size);
        int total_read_sz = rd_sz;
        while(rd_sz > 0 && rd_sz < size)
        {
            ptr = (char*)ptr + rd_sz;
            size -= rd_sz;
            rd_sz = ZlibLoadCB::tryReadImpl(ptr, size);
            if(rd_sz > 0)
                total_read_sz += rd_sz;
        }
        return total_read_sz;
    }

    void ZlibLoadCB::read(void* ptr, int size)
    {
        int rd_sz = tryRead(ptr, size);
        if(rd_sz != size)
        {
            isFinished = true;
            NAU_THROW(LoadException("ZLIB read error", -1));
        }
    }
    void ZlibLoadCB::seekrel(int ofs)
    {
        if(ofs < 0)
            issueFatal();
        else
            while(ofs > 0)
            {
                char buf[4096];
                int sz = ofs > sizeof(buf) ? sizeof(buf) : ofs;
                read(buf, sz);
                ofs -= sz;
            }
    }

    bool ZlibLoadCB::ceaseReading()
    {
        if(isFinished || !isStarted)
            return true;

        loadCb->seekrel(inBufLeft > 0x70000000 ? -int(((z_stream*)&strm)->avail_in) : inBufLeft);

        int err = inflateEnd((z_stream*)&strm);

        bool ret = err == Z_OK;
        if(!ret && fatalErrors)
        {
            if(hal::dag_on_zlib_error_cb)
                hal::dag_on_zlib_error_cb(getTargetName(), 0x30000 | ((unsigned)err & 0xFF));

            NAU_FAILURE("zlib error {} in {}\nsource: '{}'\n", err, "inflateEnd", getTargetName());
        }

        isFinished = true;

        return ret;
    }

    void BufferedZlibLoadCB::read(void* ptr, int size)
    {
        int rd = tryRead(ptr, size);
        if(rd != size)
        {
            NAU_LOG_ERROR(nau::utils::format(nau::strings::toUtf8(u8"BufferedZlibLoadCB::read({}, {})={} totalOut={} curPos={}"), ptr, size, rd, totalOut, curPos));
            NAU_THROW(LoadException("ZLIB read error", -1));
        }
    }
    int BufferedZlibLoadCB::tryRead(void* _ptr, int size)
    {
        char* ptr = (char*)_ptr;
        if(curPos + size <= totalOut)
        {
            memcpy(ptr, outBuf + curPos, size);
            curPos += size;
            return size;
        }

        if(totalOut - curPos && totalOut - curPos < size)
        {
            memcpy(ptr, outBuf + curPos, totalOut - curPos);
            ptr += totalOut - curPos;
            size -= totalOut - curPos;
        }

        if(size > OUT_BUF_SZ / 2)
        {
            while(size > 0)
            {
                totalOut = ZlibLoadCB::tryRead(outBuf, size >= OUT_BUF_SZ ? OUT_BUF_SZ : size);
                memcpy(ptr, outBuf, totalOut);
                ptr += totalOut;
                size -= totalOut;
                if(!totalOut)
                    break;
            }

            totalOut = 0;
            curPos = 0;
        }
        else
        {
            totalOut = ZlibLoadCB::tryReadImpl(outBuf, OUT_BUF_SZ);
            NAU_ASSERT(totalOut >= size);
            memcpy(ptr, outBuf, size);
            ptr += size;
            curPos = size;
        }
        return ptr - (char*)_ptr;
    }

    //-----------------------------------------------------------------------
    // ZlibSaveCB
    //-----------------------------------------------------------------------

    ZlibSaveCB::ZlibSaveCB(IGenSave& in_save_cb, int compression_level, bool raw_inflate) :
        saveCb(&in_save_cb)
    {
        zlibWriter = new ZLibGeneralWriter(in_save_cb, 64 << 10, compression_level, /*tmpmem*/ nau::getDefaultAllocator(), raw_inflate);
        isFinished = false;
    }

    ZlibSaveCB::~ZlibSaveCB()
    {
        NAU_ASSERT(isFinished);
        delete zlibWriter;
    }

    void ZlibSaveCB::write(const void* ptr, int size)
    {
        NAU_ASSERT(!isFinished);
        zlibWriter->pack((void*)ptr, size, false);
    }

    void ZlibSaveCB::finish()
    {
        zlibWriter->pack(NULL, 0, true);
        isFinished = true;
    }

    int zlib_compress_data(IGenSave& dest, int compression_level, IGenLoad& src, int sz)
    {
        static constexpr int BUF_SZ = 64 << 10;
        ZLibGeneralWriter zlibWriter(dest, BUF_SZ, compression_level, /*tmpmem*/ nau::getDefaultAllocator(), false);
        char* buf = new char[BUF_SZ];

        while(sz > 0)
        {
            int b_sz = (sz > BUF_SZ) ? BUF_SZ : sz;
            b_sz = src.tryRead(buf, b_sz);
            sz -= b_sz;
            zlibWriter.pack(buf, b_sz, sz <= 0);
        }
        delete[] buf;

        return zlibWriter.compressedTotal;
    }

    float ZlibSaveCB::getCompressionRatio()
    {
        return zlibWriter->getCompressionRatio();
    }

    extern "C" void* zcalloc(void* opaque, unsigned items, unsigned size)
    {
        (void)(opaque);
        // return memalloc_default(items * size);
        return nau::getDefaultAllocator()->allocate(items * size);
    }

    extern "C" void zcfree(void* opaque, void* ptr)
    {
        // memfree_default(ptr);
        nau::getDefaultAllocator()->deallocate(ptr);
        (void)(opaque);
    }
}  // namespace nau::iosys