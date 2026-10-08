// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include <nau/dag_ioSys/dag_asyncWrite.h>

#include <vector>
// #include <nau/osApiWrappers/dag_miscApi.h>
// #include <nau/osApiWrappers/dag_unicode.h>
// #include <memory/dag_framemem.h>

#include <string>
#include <string_view>

#include "nau/core_defines.h"
#include "nau/string/format.h"
#include "nau/string/string_conv.h"
#include "utf8/checked.h"

#if NAU_PLATFORM_APPLE | NAU_PLATFORM_LINUX
    #include <errno.h>
    #include <fcntl.h>
    #include <stdlib.h>
    #include <unistd.h>
#elif NAU_PLATFORM_WIN32 | _TARGET_XBOX
    #include <io.h>
    #include <stddef.h>
#endif

#if NAU_PLATFORM_WIN32 | _TARGET_XBOX
    #include <windows.h>
    #define THREAD_YIELD ((void)0)  // Win sleeps within poll_aio_result
#elif NAU_PLATFORM_APPLE | NAU_PLATFORM_LINUX
    #include <aio.h>
    #define THREAD_YIELD sleep_msec(0)
#else
    #define THREAD_YIELD sleep_msec(0)
#endif
namespace nau::iosys
{
#if NAU_PLATFORM_WIN32 | _TARGET_XBOX
    typedef HANDLE file_handle_t;
    #define INVALID_FILE_HANDLE INVALID_HANDLE_VALUE
#else
    typedef int file_handle_t;
    #define INVALID_FILE_HANDLE -1
#endif

#define WRITE_DONE (-1)
#define WRITE_IN_PROGRESS (0)
#define WRITE_FAILED (1)

    class AsyncWriterCB final : public IGenSave
    {
        int done;
        int errorsLeft;
        file_handle_t fileHandle;
        int offs;
        const int bufMaxSize;
        std::vector<char> buf, bufNext;

#if NAU_PLATFORM_APPLE | NAU_PLATFORM_LINUX
        aiocb aio;
#elif NAU_PLATFORM_WIN32 | _TARGET_XBOX
        OVERLAPPED overlapped;
#endif

    public:
        AsyncWriterCB(int buf_size);
        bool open(const char* fname, AsyncWriterMode mode);
        bool openTemp(char* fname);
        ~AsyncWriterCB();
        void close();

#if NAU_PLATFORM_WIN32 | _TARGET_XBOX
        static void __stdcall on_write_done_cb(DWORD dwErr, DWORD cbWritten, OVERLAPPED* lpOverLap);
        void poll_aio_result()
        {
            (done == WRITE_IN_PROGRESS) ? (void)SleepEx(0, TRUE) : ((void)0);
        }  // aleartable sleep to be able get AIO callbacks
#elif NAU_PLATFORM_APPLE | NAU_PLATFORM_LINUX
        void poll_aio_result();
#else
        inline void poll_aio_result()
        {
        }
#endif

        void waitPendingIO()
        {
            while (done == WRITE_IN_PROGRESS)
            {
                THREAD_YIELD;
                poll_aio_result();
            }
        }

        virtual void write(const void* ptr, int size) override;

        virtual int tell() override
        {
            return offs + buf.size() + bufNext.size();
        }
        virtual void seekto(int ofsabs) override
        {
            NAU_ASSERT(buf.empty() && bufNext.empty(),
                       "seekto() can't be called while there are exist some data in flight. call flush() first");
            offs = ofsabs;
        }
        virtual void seektoend(int) override
        {
            issueFatal();
        }
        virtual const char* getTargetName() override
        {
            return "";
        }
        virtual void flush() override;
        virtual void beginBlock() override
        {
            issueFatal();
        }
        virtual void endBlock(unsigned /*block_flags*/ = 0) override
        {
            issueFatal();
        }
        virtual int getBlockLevel() override
        {
            issueFatal();
            return 0;
        }

        void issueFatal()
        {
            NAU_ASSERT(0, "restricted by design");
        }
    };

#define ERRORS_BEFORE_FAILING 16

    AsyncWriterCB::AsyncWriterCB(int buf_size) :
        done(WRITE_DONE),
        errorsLeft(ERRORS_BEFORE_FAILING),
        offs(0),
        // buf(midmem),
        // bufNext(midmem),
        bufMaxSize(buf_size),
        fileHandle(INVALID_FILE_HANDLE)
    {
#if NAU_PLATFORM_APPLE | NAU_PLATFORM_LINUX
        memset(&aio, 0, sizeof(aio));
#elif NAU_PLATFORM_WIN32 | _TARGET_XBOX
        memset(&overlapped, 0, sizeof(overlapped));
#endif
    }

    bool AsyncWriterCB::open(const char* fname, AsyncWriterMode mode)
    {
#if NAU_PLATFORM_APPLE | NAU_PLATFORM_LINUX
        fileHandle = ::open(fname, O_WRONLY | O_CREAT | (mode == AsyncWriterMode::TRUNC ? O_TRUNC : O_APPEND), 0666);
#elif NAU_PLATFORM_WIN32 | _TARGET_XBOX
        fileHandle = CreateFileW(reinterpret_cast<const wchar_t*>(nau::strings::utf8ToUtf16(fname).c_str()), (mode == AsyncWriterMode::TRUNC ? GENERIC_WRITE : FILE_APPEND_DATA),
                                 FILE_SHARE_READ, NULL, (mode == AsyncWriterMode::TRUNC ? CREATE_ALWAYS : OPEN_EXISTING), FILE_FLAG_OVERLAPPED, NULL);
#else
        G_UNUSED(fname);
        G_UNUSED(mode);
#endif
        return fileHandle != INVALID_FILE_HANDLE;
    }

    bool AsyncWriterCB::openTemp(char* fname)
    {
#if NAU_PLATFORM_APPLE | NAU_PLATFORM_LINUX
        fileHandle = mkstemp(fname);
#elif NAU_PLATFORM_WIN32 | _TARGET_XBOX
        if (_mktemp_s(fname, DAGOR_MAX_PATH) == 0)
            fileHandle = CreateFileW(reinterpret_cast<const wchar_t*>(nau::strings::utf8ToUtf16(fname).c_str()), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                                     FILE_FLAG_OVERLAPPED, NULL);
#else
        G_UNUSED(fname);
#endif
        return fileHandle != INVALID_FILE_HANDLE;
    }

    AsyncWriterCB::~AsyncWriterCB()
    {
        close();
    }

    void AsyncWriterCB::close()
    {
        if (fileHandle == INVALID_FILE_HANDLE)
            return;
        flush();
#if NAU_PLATFORM_APPLE | NAU_PLATFORM_LINUX
        G_VERIFY(::close(fileHandle) == 0);
#elif NAU_PLATFORM_WIN32 | _TARGET_XBOX
        const bool closed = CloseHandle(fileHandle);
        NAU_ASSERT(closed);
#endif
        fileHandle = INVALID_FILE_HANDLE;

        buf.clear();
        bufNext.clear();
    }

#if NAU_PLATFORM_WIN32 | _TARGET_XBOX
    void __stdcall AsyncWriterCB::on_write_done_cb(DWORD dwErr, DWORD cbWritten, OVERLAPPED* lpOverLap)
    {
        AsyncWriterCB* wcb = (AsyncWriterCB*)((char*)lpOverLap - offsetof(AsyncWriterCB, overlapped));
        if (dwErr == ERROR_SUCCESS)
        {
            NAU_ASSERT(wcb->buf.size() == cbWritten);
            wcb->offs += cbWritten;
            wcb->buf.clear();
            wcb->done = WRITE_DONE;
        }
        else
        {
            wcb->done = WRITE_FAILED;
            wcb->buf.clear();
            wcb->bufNext.clear();
        }
    }
#elif NAU_PLATFORM_APPLE | NAU_PLATFORM_LINUX
    void AsyncWriterCB::poll_aio_result()
    {
        int bwr = 0;
        if (done == WRITE_IN_PROGRESS)
            switch (aio_error(&aio))
            {
                case EINPROGRESS:
                    return;
                case 0:
                    bwr = aio_return(&aio);
                    NAU_ASSERT(buf.size() == bwr);
                    offs += bwr;
                    NAU_FAST_ASSERT(offs >= 0);  // Attempt to write > 2G of data?
                    buf.clear();
                    done = WRITE_DONE;
                    break;
                default:
                    done = WRITE_FAILED;
                    break;
            }
    }
#endif

    void AsyncWriterCB::write(const void* ptr, int size)
    {
        NAU_ASSERT(fileHandle != INVALID_FILE_HANDLE);

        poll_aio_result();

        if (done == WRITE_IN_PROGRESS)
        {
            if (bufNext.size() + size <= bufMaxSize)
            {
                for (int i = 0; i < size; ++i)
                {
                    bufNext.push_back(reinterpret_cast<const char*>(ptr)[i]);
                }
                return;
            }
            else  // too big to grow
                waitPendingIO();
        }
        else if (done == WRITE_FAILED)
            return;

        NAU_ASSERT(done == WRITE_DONE);
        NAU_ASSERT(buf.empty());
#if (NAU_PLATFORM_WIN32 | _TARGET_XBOX) && DAGOR_DBGLEVEL > 0
        DWORD lpNumberOfBytesTransferred = 0;
        NAU_ASSERT(GetOverlappedResult(fileHandle, &overlapped, &lpNumberOfBytesTransferred, /*wait*/ FALSE), "{}", GetLastError());
#endif

        buf.swap(bufNext);
        for (int i = 0; i < size; ++i)
        {
            buf.push_back(reinterpret_cast<const char*>(ptr)[i]);
        }
        if (buf.empty())
            return;

#if NAU_PLATFORM_APPLE | NAU_PLATFORM_LINUX
        NAU_ASSERT(offs >= 0);
        memset(&aio, 0, sizeof(aio));
        aio.aio_fildes = fileHandle;
        aio.aio_offset = offs;
        aio.aio_buf = buf.data();
        aio.aio_nbytes = buf.size();
        aio.aio_sigevent.sigev_notify = SIGEV_NONE;
        aio.aio_sigevent.sigev_notify_function = NULL;
#elif NAU_PLATFORM_WIN32 | _TARGET_XBOX
        overlapped.Offset = offs;
#endif

        done = WRITE_IN_PROGRESS;

#if NAU_PLATFORM_APPLE | NAU_PLATFORM_LINUX
        int failcnt = 0;
        while (aio_write(&aio) != 0)
            if (errno == EAGAIN)
                continue;
            else if (failcnt < ERRORS_BEFORE_FAILING)
                failcnt++;
            else
            {
                done = WRITE_FAILED;
                break;
            }
#elif NAU_PLATFORM_WIN32 | _TARGET_XBOX
        BOOL result = WriteFileEx(fileHandle, buf.data(), buf.size(), &overlapped, on_write_done_cb);
        // APC can't be delivered to different thread then the one that done prior request,
        // so we have no choice but to do synchronous write (wait) when this function is called from non main thread

        NAU_ASSERT(false, "To check this later");
        if (/*!is_main_thread() &&*/ result)
        {
            // Note: GetOverlappedResultEx is more effective then manual loop but it's not supported on win7
            DWORD _1;
            const bool res = GetOverlappedResult(fileHandle, &overlapped, &_1, /*wait*/ TRUE);
            NAU_ASSERT(res);
            while (done == WRITE_IN_PROGRESS)
                SleepEx(0, TRUE);
        }

        if (result)  // Note: this is probably not needed since drop support of Xbox360
        {
            errorsLeft = ERRORS_BEFORE_FAILING;
        }
        else
        {
            errorsLeft--;
            if (errorsLeft < 0)
            {
                done = WRITE_FAILED;
                buf.clear();
                bufNext.clear();
            }
            else
            {
                done = WRITE_DONE;
                bufNext.resize(buf.size());
                memcpy(bufNext.data(), buf.data(), buf.size());
                buf.clear();
            }
        }
#endif
    }

    void AsyncWriterCB::flush()
    {
        waitPendingIO();
        if (bufNext.size() > 0)
        {
            write(NULL, 0);
            waitPendingIO();
        }
        NAU_ASSERT(buf.empty() && bufNext.empty());
    }

    IGenSave* create_async_writer(const char* fname, int buf_size, AsyncWriterMode mode)
    {
        AsyncWriterCB* ret = new AsyncWriterCB(buf_size);
        if (!ret->open(fname, mode))
        {
            if (ret)
            {
                delete ret;
                ret = nullptr;
            }
        }
        return ret;
    }

    IGenSave* create_async_writer_temp(char* in_out_fname, int buf_size)
    {
        AsyncWriterCB* ret = new AsyncWriterCB(buf_size);
        if (!ret->openTemp(in_out_fname))
        {
            if (ret)
            {
                delete ret;
                ret = nullptr;
            }
        }
        return ret;
    }
}  // namespace nau::iosys