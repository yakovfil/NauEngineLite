// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include <nau/dag_ioSys/dag_fastSeqRead.h>
#include <nau/osApiWrappers/dag_asyncRead.h>
// #include <nau/osApiWrappers/dag_miscApi.h>
#include <nau/osApiWrappers/dag_files.h>
// #include <nau/osApiWrappers/dag_cpuJobs.h>
// #include <nau/osApiWrappers/dag_vromfs.h>
#include <nau/core_defines.h>
#include <nau/utils/dag_globDef.h>

#include <cstdint>
#include <string>
#include <string_view>

#include "nau/diag/assertion.h"
#include "nau/diag/logging.h"
#include "nau/memory/mem_allocator.h"
#include "nau/string/format.h"
#include "nau/string/string_conv.h"
// #include <osApiWrappers/dag_dbgStr.h>
namespace nau::iosys
{

#if NAU_PLATFORM_WIN32 | _TARGET_XBOX
extern "C" __declspec(dllimport) unsigned long __stdcall SleepEx(unsigned long, int);
void sleep_msec_ex(int ms)
{
    SleepEx(ms, /*TRUE*/ 1);
}  // Alertable sleep in order to get AIO callbacks
#else
void sleep_msec_ex(int ms)
{
    sleep_msec(ms);
}
#endif

    FastSeqReader::FastSeqReader()
    {
        memset(&file, 0, sizeof(file));
        memset(buf, 0, sizeof(buf));

        for(int i = 0; i < BUF_CNT; i++)
        {
            buf[i].mask = 1 << i;
#if NAU_PLATFORM_WIN32 | \
    _TARGET_XBOX  // MS require special aligment for non_cached reads (see
                  // https://msdn.microsoft.com/en-us/library/windows/desktop/cc644950(v=vs.85).aspx#ALIGNMENT_AND_FILE_ACCESS_REQUIREMENTS)
            // tmpmem->
            buf[i].data = reinterpret_cast<char*>(nau::getDefaultAllocator()->allocateAligned(BUF_SZ, 4096));
#else
            buf[i].data = (char*)tmpmem->allocAligned(BUF_SZ, 32);
#endif
            buf[i].handle = nau::hal::dfa_alloc_asyncdata();
            NAU_ASSERT(buf[i].handle >= 0 && "FastSeqReader ran out of async handles?");
        }
        pendMask = doneMask = readAheadPos = maxBackSeek = lastSweepPos = 0;
        cBuf = NULL;
    }

    void FastSeqReader::assignFile(void* handle, unsigned base_ofs, int size, const char* fname, int min_chunk_size, int max_back_seek)
    {
        NAU_ASSERT(max_back_seek >= 0);
        reset();

        file.handle = handle;
        file.baseOfs = base_ofs == 0xFFFFFFFFu ? 0u : base_ofs;  // 0xFFFFFFFFu was stored in entry.data() for ofs=0 to differ from nullptr
        file.size = size;
        file.pos = 0;
        maxBackSeek = max_back_seek;
        NAU_ASSERT(min_chunk_size);
        file.chunkSize = min_chunk_size;
        targetFilename = fname;
    }
    void FastSeqReader::reset()
    {
        if(pendMask)
            do
            {
                for(int i = 0, bit = 1, sz; i < BUF_CNT; i++, bit <<= 1)
                    if((pendMask & bit) && nau::hal::dfa_check_complete(buf[i].handle, &sz))
                        pendMask &= ~bit;
                if(pendMask)
                    sleep_msec_ex(1);
                else
                    break;
            } while(1);
        doneMask = readAheadPos = lastSweepPos = 0;
        cBuf = NULL;
        file.pos = 0;
        curThreadId = -1;
    }

    void FastSeqReader::closeData()
    {
        reset();
        for(int i = 0; i < BUF_CNT; i++)
            if(buf[i].data)
            {
                nau::hal::dfa_free_asyncdata(buf[i].handle);
                // tmpmem->freeAligned(buf[i].data);
                nau::getDefaultAllocator()->deallocateAligned(buf[i].data);
            }
        memset(&file, 0, sizeof(file));
        memset(buf, 0, sizeof(buf));
    }
    void FastSeqReader::waitForBuffersFull()
    {
        placeRequests();

        while(pendMask)
        {
            sleep_msec_ex(1);
            placeRequests();
        }
    }

    int FastSeqReader::tryRead(void* ptr, int size)
    {
        checkThreadSanity();
        char* p = (char*)ptr;
        if(file.pos + size > file.size)
        {
            size = file.size - file.pos;
            if(size <= 0)
                return 0;
        }

        for(Range* __restrict r = ranges.data(), * __restrict re = r + ranges.size(); r < re; r++)
            if(file.pos >= r->start && file.pos < r->end)
            {
                if(file.pos + size > r->end)
                    size = r->end - file.pos;
                break;
            }

        if(cBuf)
        {
            NAU_ASSERT(!(pendMask & cBuf->mask) && (doneMask & cBuf->mask));
            if(file.pos + size <= cBuf->ea)
            {
                memcpy(p, cBuf->data + file.pos - cBuf->sa, size);
                file.pos += size;
                if((~(doneMask | pendMask)) & BUF_ALL_MASK)
                    placeRequests();
                return size;
            }

            int sz = cBuf->ea - file.pos;
            memcpy(p, cBuf->data + file.pos - cBuf->sa, sz);
            p += sz;
            file.pos += sz;
            size -= sz;
            if(!maxBackSeek)
                doneMask &= ~cBuf->mask;
            cBuf = NULL;
            placeRequests();
        }

        do
        {
            if(cBuf && file.pos >= cBuf->ea)
            {
                NAU_LOG_INFO(nau::utils::format(nau::strings::toUtf8(u8"{} {} reset cBuf due to file.pos {} cBuf->sa={} cBuf->ea={} i={}"), std::string(__FILE__), __LINE__, file.pos, cBuf->sa, cBuf->ea,
                                                cBuf - buf));
                cBuf = nullptr;
            }
            if(doneMask)
                for(int i = 0, bit = 1; i < BUF_CNT; i++, bit <<= 1)
                    if((doneMask & bit) && file.pos >= buf[i].sa && file.pos < buf[i].ea)
                    {
                        cBuf = buf + i;
                        break;
                    }
            if(cBuf)
            {
                if(file.pos + size <= cBuf->ea)
                {
                    memcpy(p, cBuf->data + file.pos - cBuf->sa, size);
                    file.pos += size;
                    placeRequests();
                    return p + size - (char*)ptr;
                }

                int sz = cBuf->ea - file.pos;
                if(!(file.pos >= cBuf->sa && sz <= size && file.pos + sz <= cBuf->ea && cBuf->ea - cBuf->sa <= BUF_SZ))
                {
                    NAU_LOG_INFO(nau::utils::format(nau::strings::toUtf8(u8"{} {} file.pos={:#x} sz={} size={} cBuf->sa={} cBuf->ea={} BUF_SZ={} i={} doneMask={:#x} pendMask={:#x} {}"), std::string(__FILE__), __LINE__,
                                                    file.pos, sz, size, cBuf->sa, cBuf->ea, static_cast<int>(BUF_SZ), cBuf - buf, doneMask, pendMask, std::string{targetFilename.c_str()}));
                    return p - (char*)ptr;
                }
                if(!(cBuf >= buf && cBuf < buf + BUF_CNT && cBuf->data
#if _TARGET_64BIT
                     && ((uint64_t(uintptr_t((void*)cBuf->data)) >> 48) != 0xAAAAu)
#endif
                         ))
                {
                    NAU_LOG_INFO(nau::utils::format(nau::strings::toUtf8(u8"{} {} cBuf={:#x} [{:#x}] cBuf->data={:#x} file.pos={} sz={} doneMask={:#x} pendMask={:#x} {}"), std::string(__FILE__), __LINE__,
                                                    intptr_t(cBuf),
                                                    intptr_t(cBuf - buf),
                                                    intptr_t(cBuf ? cBuf->data : nullptr),
                                                    file.pos, sz, doneMask, pendMask, std::string{targetFilename.c_str()}));
                    return p - (char*)ptr;
                }

                memcpy(p, cBuf->data + file.pos - cBuf->sa, sz);
                p += sz;
                file.pos += sz;
                size -= sz;
                if(!maxBackSeek)
                    doneMask &= ~cBuf->mask;
                cBuf = NULL;
            }

            if(pendMask)
                for(int i = 0, bit = 1; i < BUF_CNT; i++, bit <<= 1)
                    if((pendMask & bit) && file.pos >= buf[i].sa && file.pos < buf[i].ea)
                    {
                        waitForDone(bit);
                        if((doneMask & bit) && file.pos >= buf[i].sa && file.pos < buf[i].ea)
                            cBuf = buf + i;
                        else if(buf[i].ea > buf[i].sa)  // report only non-empty blocks (and just ignore empty ones)
                        {
                            NAU_LOG_INFO(nau::utils::format(nau::strings::toUtf8(u8"{} {} wait done mismatch: file.pos={} file.size={} buf[{}].sa={} buf[{}].ea={} {}"), std::string(__FILE__), __LINE__, file.pos,
                                                            file.size, i, buf[i].sa, i, buf[i].ea, std::string{targetFilename.c_str()}));
                        }
                        break;
                    }
            placeRequests();
            if(!cBuf && (readAheadPos >= file.size || (doneMask | pendMask) == BUF_ALL_MASK))
                sleep_msec_ex(0);
        } while(size > 0);
        return p - (char*)ptr;
    }

    void FastSeqReader::waitForDone(int wait_mask)
    {
        while(!(doneMask & wait_mask))
        {
            placeRequests();
            sleep_msec_ex(0);
        }
    }

    void FastSeqReader::placeRequests()
    {
        checkThreadSanity();
        if(pendMask)
            for(int i = 0, bit = 1, sz; i < BUF_CNT; i++, bit <<= 1)
                if((pendMask & bit) && nau::hal::dfa_check_complete(buf[i].handle, &sz))
                {
                    if(sz < 0) [[unlikely]]  // err?
                    {
#if NAU_EXCEPTIONS_ENABLED
                        NAU_THROW(LoadException("async read failed", buf[i].sa));  // To consider: store copy of string within NauException?
#else
                        NAU_FAILURE("exception: LoadException(\"async read from '{}' failed with error {}\")", targetFilename.c_str(), sz);
#endif
                    }
                    NAU_ASSERT(sz);
                    // out_debug_str_fmt("complete %d\n", i);
                    doneMask |= bit;
                    pendMask &= ~bit;
                    buf[i].ea = buf[i].sa + sz;
                }

        if(maxBackSeek && doneMask && file.pos >= maxBackSeek && file.pos >= lastSweepPos + BUF_SZ - 1)
        {
            lastSweepPos = file.pos;
            for(int i = 0, bit = 1; i < BUF_CNT; i++, bit <<= 1)
            {
                if((doneMask & bit) && buf[i].ea + maxBackSeek <= file.pos)
                    doneMask &= ~bit;
                else if((pendMask & bit) && buf[i].sa < lastSweepPos)
                    lastSweepPos = buf[i].sa;
            }
        }

        unsigned unusedMask = (~(doneMask | pendMask)) & BUF_ALL_MASK;
        if(unusedMask && readAheadPos < file.size)
            for(int i = 0, bit = 1; i < BUF_CNT; i++, bit <<= 1)
                if(unusedMask & bit)
                {
                again:
                    buf[i].sa = readAheadPos;
                    readAheadPos += BUF_SZ;
                    if(readAheadPos > file.size)
                        readAheadPos = (file.size + file.chunkSize - 1) / file.chunkSize * file.chunkSize;
                    buf[i].ea = readAheadPos;

                    if(ranges.size())
                    {
                        const Range* __restrict r = ranges.data(), * __restrict re = r + ranges.size();
                        int pos = buf[i].sa;

                        for(; r < re; r++)
                            if(r->start < readAheadPos && pos < r->end)
                            {
                                pos = r->start / (BLOCK_SIZE) * (BLOCK_SIZE);
                                if(pos > buf[i].sa)
                                    buf[i].sa = pos;

                                pos = (r->end + (BLOCK_SIZE)-1) / (BLOCK_SIZE) * (BLOCK_SIZE);
                                if(pos < readAheadPos)
                                    buf[i].ea = readAheadPos = pos;
                                r = NULL;
                                break;
                            }

                        if(r != NULL)
                        {
                            if(readAheadPos >= file.size)
                                break;
                            else if(readAheadPos < ranges.back().end)
                                goto again;
                            else
                            {
                                // out_debug_str_fmt("cease it: %d < %d (%d..%d) %d\n", readAheadPos, file.size, ranges[0].start, ranges.back().end,
                                // ranges.size());
                                readAheadPos = file.size;
                                break;
                            }
                        }
                    }

                    NAU_ASSERT(buf[i].ea - buf[i].sa);
                    // G_VERIFY(dfa_read_async(file.handle, buf[i].handle, buf[i].sa + file.baseOfs, buf[i].data, buf[i].ea - buf[i].sa));
                    //  out_debug_str_fmt("place req %d: %d-%d\n", i, buf[i].sa, buf[i].ea);
                    if(cBuf == buf + i)
                    {
                        NAU_LOG_DEBUG("reset cBuf==buf[{}]", i);
                        cBuf = nullptr;
                    }
                    pendMask |= bit;
                    if(readAheadPos >= file.size)
                        break;
                }
    }

    void FastSeqReader::seekto(int pos)
    {
        if(pos < 0 || pos > file.size)
            NAU_THROW(LoadException("seek out of range", file.pos));
        if(pos < file.pos)
        {
            if(file.pos > pos + maxBackSeek)
                NAU_FAILURE("too long back seek: pos={}, relseek={}, maxBackSeek={}, src={}", file.pos, pos - file.pos, maxBackSeek, getTargetName());
            if(cBuf && pos < cBuf->sa)
                cBuf = NULL;
        }
        if(cBuf && pos >= cBuf->ea)
        {
            if(!maxBackSeek)
                doneMask &= ~cBuf->mask;
            cBuf = NULL;
        }

        file.pos = pos;
        if(pos >= readAheadPos + maxBackSeek)
        {
            int npos = (pos - maxBackSeek) / (BLOCK_SIZE) * (BLOCK_SIZE);
            // out_debug_str_fmt("move readAheadPos=%d->%d due to seekto %d, cBuf=%p done=%p pend=%p\n",
            //   readAheadPos, npos, pos, cBuf, doneMask, pendMask);
            if(readAheadPos < npos)
                readAheadPos = npos;
            placeRequests();
        }
    }

    void FastSeqReader::checkThreadSanity()
    {
        /*
        int64_t cur_thread = get_current_thread_id();
        if (curThreadId == -1)
        {
          curThreadId = cur_thread;
          // NAU_CORE_DEBUG_LF("{:p}.thread={} {}", this, curThreadId, targetFilename);
        }
        else if (curThreadId != cur_thread)
        {
          logmessage_ctx(_MAKE4C('nEvt'), "checkThreadSanity failed (%d and %d) for %s, file.pos=%d", curThreadId, cur_thread,
            targetFilename, file.pos);
        }

        NAU_ASSERT(curThreadId == cur_thread, "checkThreadSanity failed ({} and {}) for {}, file.pos={}", curThreadId, cur_thread,
          targetFilename, file.pos);
        */
    }

    const char* FastSeqReader::resolve_real_name(const char* fname, unsigned& out_base_ofs, unsigned& out_size, bool& out_non_cached)
    {
        out_non_cached = true;
        out_base_ofs = 0;
        out_size = 0;

        // TODO: VROM integration
        // VirtualRomFsData *vrom = NULL;
        // VromReadHandle data = vromfs_get_file_data(fname, &vrom);
        // if (data.data() && vrom && static_cast<VirtualRomFsPack *>(vrom)->isValid())
        //{
        //   out_base_ofs = ptrdiff_t(data.data());
        //   out_size = data.size();
        //   out_non_cached = (out_base_ofs & 0xFFF) == 0 && (out_size & 0xFFF) == 0;
        //   fname = static_cast<VirtualRomFsPack *>(vrom)->getFilePath();
        // }
        return nau::hal::df_get_real_name(fname);
    }

    bool FastSeqReadCB::open(const char* fname, int max_back_seek, const char* base_path)
    {
        close();
        NAU_ASSERT(max_back_seek <= 5 * BUF_SZ);

        unsigned base_ofs = 0, size = 0;
        bool non_cached = true;
        const char* real_name = FastSeqReader::resolve_real_name(fname, base_ofs, size, non_cached);
        (void)base_path;
        if(!real_name)
            return false;

        void* handle = nau::hal::dfa_open_for_read(real_name, non_cached);
        if(!handle)
            return false;

        if(!size)
            size = nau::hal::dfa_file_length(handle);
        if(size == 0xffffffffu)
        {
            nau::hal::dfa_close(handle);
            return false;
        }

        assignFile(handle, base_ofs, size, fname, nau::hal::dfa_chunk_size(real_name), max_back_seek);
        return true;
    }
    void FastSeqReadCB::close()
    {
        reset();
        nau::hal::dfa_close(file.handle);
        file.handle = NULL;
    }
}  // namespace nau::iosys
