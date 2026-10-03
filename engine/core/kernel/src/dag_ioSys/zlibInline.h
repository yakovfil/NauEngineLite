// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#pragma once

#include <nau/dag_ioSys/dag_genIo.h>
#include <nau/osApiWrappers/dag_files.h>
#include <nau/utils/dag_globDef.h>

#include <vector>

#include "nau/memory/mem_allocator.h"

// #if !defined(USE_ZLIB_VER)
// #include <zlib.h>
// #elif USE_ZLIB_VER == 0xFF0000
#include <zlib.h>
// #endif

namespace nau::iosys
{
    class ZLibPacker
    {
    protected:
        z_stream strm;

    public:
        ZLibPacker(int level, nau::IMemAllocator::Ptr mem, bool raw_inflate)
        {
            strm.zalloc = Z_NULL;
            strm.zfree = Z_NULL;
            // strm.opaque = mem;
            int err = raw_inflate
                          ? deflateInit2(&strm, level, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL >= 8 ? 8 : MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY)
                          : deflateInit(&strm, level);
            if (err != Z_OK)
                NAU_FAILURE("zlib error {} in {}", err, u8"deflateInit");
        }
        ~ZLibPacker()
        {
            int err = deflateEnd(&strm);
            if (err != Z_OK)
                NAU_FAILURE("zlib error {} in {}", err, u8"deflateEnd");
        }
        // returns Z_OK or Z_STREAM_END
        int pack(void*& inp, unsigned& insz, void*& outp, unsigned& outsz, int flush)
        {
            strm.next_in = (Bytef*)inp;
            strm.avail_in = insz;
            strm.next_out = (Bytef*)outp;
            strm.avail_out = outsz;
            int err = deflate(&strm, flush);
            inp = strm.next_in;
            insz = strm.avail_in;
            outp = strm.next_out;
            outsz = strm.avail_out;
            if (err != Z_OK && err != Z_STREAM_END)
                NAU_FAILURE("zlib error {} in {}", err, u8"deflate");
            return err;
        }
    };

    class ZLibGeneralWriter : public ZLibPacker
    {
    protected:
        std::vector<char> buf;

    public:
        IGenSave* callback;
        unsigned uncompressedTotal;
        unsigned compressedTotal;

        ZLibGeneralWriter(IGenSave& cb, unsigned bufsz, int level, nau::IMemAllocator::Ptr mem, bool raw_inflate) :
            ZLibPacker(level, mem, raw_inflate),
            callback(&cb)
        {
            buf.resize(bufsz);
            uncompressedTotal = 0;
            compressedTotal = 0;
        }

        void pack(void* inp, unsigned sz, bool finish)
        {
            uncompressedTotal += sz;

            while (sz || finish)
            {
                void* op = &buf[0];
                unsigned osz = buf.size() * sizeof(decltype(buf)::value_type);
                int err = ZLibPacker::pack(inp, sz, op, osz, finish ? Z_FINISH : Z_NO_FLUSH);
                unsigned l = buf.size() * sizeof(decltype(buf)::value_type) - osz;
                if (l != 0)
                {
                    callback->write(&buf[0], l);
                    compressedTotal += l;
                }

                while (err == Z_OK && osz == 0)
                {
                    op = &buf[0];
                    osz = buf.size() * sizeof(decltype(buf)::value_type);
                    err = ZLibPacker::pack(inp, sz, op, osz, finish ? Z_FINISH : Z_NO_FLUSH);
                    l = buf.size() * sizeof(decltype(buf)::value_type) - osz;
                    if (l != 0)
                    {
                        callback->write(&buf[0], l);
                        compressedTotal += l;
                    }
                }

                if (err == Z_STREAM_END)
                    break;
                if (err != Z_OK)
                    NAU_FAILURE("pack error {}", err);
            }
        }

        float getCompressionRatio()
        {
            if (uncompressedTotal)
                return (float)compressedTotal / uncompressedTotal;
            else
                return 0;
        }
    };

    extern "C" void* zcalloc(void* opaque, unsigned items, unsigned size);
    extern "C" void zcfree(void* opaque, void* ptr);
}  // namespace nau::iosys