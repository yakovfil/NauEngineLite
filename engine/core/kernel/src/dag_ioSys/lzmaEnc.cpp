// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include <LzmaEnc.h>
#include <nau/dag_ioSys/dag_lzmaIo.h>

#include <string>
#include <string_view>

#include "nau/diag/logging.h"
#include "nau/memory/mem_allocator.h"
#include "nau/string/format.h"
#include "nau/string/string_conv.h"

namespace nau::iosys
{

    // static void *SzAlloc(void *, size_t size) { return memalloc(size, tmpmem); }
    static void* SzAlloc(void*, size_t size)
    {
        return nau::getDefaultAllocator()->allocate(size);
    }
    // static void SzFree(void *, void *address) { memfree(address, tmpmem); }
    static void SzFree(void*, void* address)
    {
        nau::getDefaultAllocator()->deallocate(address);
    }
    static ISzAlloc g_Alloc = {SzAlloc, SzFree};

    struct DagSeqInStream : public ISeqInStream
    {
        DagSeqInStream(IGenLoad* _crd, int sz)
        {
            crd = _crd;
            Read = readCrd;
            dataSz = sz;
        }
        static SRes readCrd(void* p, void* buf, size_t* size)
        {
            int& dataSz = reinterpret_cast<DagSeqInStream*>(p)->dataSz;
            *size = reinterpret_cast<DagSeqInStream*>(p)->crd->tryRead(buf, int((*size > dataSz) ? dataSz : *size));
            dataSz -= int(*size);
            return SZ_OK;
        }
        IGenLoad* crd;
        int dataSz;
    };
    struct DagSeqOutStream : public ISeqOutStream
    {
        DagSeqOutStream(IGenSave* _cwr)
        {
            cwr = _cwr;
            Write = writeCwr;
            sumSz = 0;
        }
        static size_t writeCwr(void* p, const void* buf, size_t size)
        {
            reinterpret_cast<DagSeqOutStream*>(p)->cwr->write(buf, int(size));
            reinterpret_cast<DagSeqOutStream*>(p)->sumSz += int(size);
            return size;
        }
        IGenSave* cwr;
        int sumSz;
    };

    int lzma_compress_data(IGenSave& dest, int compression_level, IGenLoad& src, int sz, int dict_sz)
    {
        CLzmaEncHandle enc = LzmaEnc_Create(&g_Alloc);

        CLzmaEncProps props;
        LzmaEncProps_Init(&props);
        props.level = compression_level;
        props.dictSize = dict_sz;

        SRes res = LzmaEnc_SetProps(enc, &props);

        if(res != SZ_OK)
        {
            LzmaEnc_Destroy(enc, &g_Alloc, &g_Alloc);
            NAU_LOG_ERROR(nau::utils::format(nau::strings::toUtf8(u8"7zip error {} in LzmaEnc_SetProps\nsource: '{}'\n"), res, std::string(dest.getTargetName())));
            return -1;
        }

        Byte header[LZMA_PROPS_SIZE];
        size_t headerSize = LZMA_PROPS_SIZE;

        res = LzmaEnc_WriteProperties(enc, header, &headerSize);
        if(res != SZ_OK)
        {
            LzmaEnc_Destroy(enc, &g_Alloc, &g_Alloc);
            enc = NULL;
            NAU_LOG_ERROR(nau::utils::format(nau::strings::toUtf8(u8"7zip error {} in LzmaEnc_WriteProperties\nsource: '{}'\n"), res, std::string(dest.getTargetName())));
            return -1;
        }

        int st_ofs = dest.tell();
        dest.write(header, int(headerSize));
        DagSeqInStream inStrm(&src, sz);
        DagSeqOutStream outStrm(&dest);

        res = LzmaEnc_Encode(enc, &outStrm, &inStrm, NULL, &g_Alloc, &g_Alloc);
        LzmaEnc_Destroy(enc, &g_Alloc, &g_Alloc);
        if(res != SZ_OK)
        {
            NAU_LOG_ERROR(nau::utils::format(nau::strings::toUtf8(u8"7zip error {} in {}\nsource: 'LzmaEnc_Encode'\n"), res, std::string(dest.getTargetName())));
        }

        return dest.tell() - st_ofs;
    }
}  // namespace nau::iosys