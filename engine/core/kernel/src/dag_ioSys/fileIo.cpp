// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include <nau/dag_ioSys/dag_fileIo.h>
#include <nau/debug/dag_except.h>
#include <nau/osApiWrappers/dag_files.h>

#include <span>
namespace nau::iosys
{
    LFileGeneralSaveCB::LFileGeneralSaveCB(file_ptr_t handle) :
        fileHandle(handle)
    {
    }

    void LFileGeneralSaveCB::write(const void* ptr, int size)
    {
        if (!fileHandle)
            NAU_THROW(SaveException("file not open", 0));
        if (hal::df_write(fileHandle, (void*)ptr, size) != size)
            NAU_THROW(SaveException("write error", tell()));
    }

    int LFileGeneralSaveCB::tryWrite(const void* ptr, int size)
    {
        return fileHandle ? hal::df_write(fileHandle, ptr, size) : -1;
    }

    int LFileGeneralSaveCB::tell()
    {
        if (!fileHandle)
            NAU_THROW(SaveException("file not open", 0));
        int o = hal::df_tell(fileHandle);
        if (o == -1)
            NAU_THROW(SaveException("tell returns error", 0));
        return o;
    }

    void LFileGeneralSaveCB::seekto(int o)
    {
        if (!fileHandle)
            NAU_THROW(SaveException("file not open", 0));
        if (hal::df_seek_to(fileHandle, o) == -1)
            NAU_THROW(SaveException("seek error", tell()));
    }

    void LFileGeneralSaveCB::seektoend(int o)
    {
        if (!fileHandle)
            NAU_THROW(SaveException("file not open", 0));
        if (hal::df_seek_end(fileHandle, o) == -1)
            NAU_THROW(SaveException("seek error", tell()));
    }

    void LFileGeneralSaveCB::flush()
    {
        if (fileHandle)
            hal::df_flush(fileHandle);
    }

    LFileGeneralLoadCB::LFileGeneralLoadCB(file_ptr_t handle) :
        fileHandle(handle)
    {
    }

    const VirtualRomFsData* LFileGeneralLoadCB::getTargetVromFs() const
    {
        return hal::df_get_vromfs_for_file_ptr(fileHandle);
    }
    void LFileGeneralLoadCB::read(void* ptr, int size)
    {
        if (!fileHandle)
            NAU_THROW(LoadException("file not open", 0));
        if (hal::df_read(fileHandle, ptr, size) != size)
            NAU_THROW(LoadException("read error", tell()));
    }
    int LFileGeneralLoadCB::tryRead(void* ptr, int size)
    {
        if (!fileHandle)
            return 0;
        return hal::df_read(fileHandle, ptr, size);
    }

    int LFileGeneralLoadCB::tell()
    {
        if (!fileHandle)
            NAU_THROW(LoadException("file not open", 0));
        int o = hal::df_tell(fileHandle);
        if (o == -1)
            NAU_THROW(LoadException("tell returns error", 0));
        return o;
    }

    void LFileGeneralLoadCB::seekto(int o)
    {
        if (!fileHandle)
            NAU_THROW(LoadException("file not open", 0));
        if (hal::df_seek_to(fileHandle, o) == -1)
            NAU_THROW(LoadException("seek error", tell()));
    }

    void LFileGeneralLoadCB::seekrel(int o)
    {
        if (!fileHandle)
            NAU_THROW(LoadException("file not open", 0));
        if (hal::df_seek_rel(fileHandle, o) == -1)
            NAU_THROW(LoadException("seek error", tell()));
    }

    FullFileLoadCB::FullFileLoadCB(const char* fname)
    {
        fileHandle = NULL;
        open(fname, hal::DF_READ);
    }

    bool FullFileLoadCB::open(const char* fname, int mode)
    {
        close();
        targetFilename = fname ? fname : "";
        targetDataSz = -1;
        if (!fname)
            return false;
        fileHandle = hal::df_open(fname, mode);
        targetDataSz = hal::df_length(fileHandle);
        return fileHandle != NULL;
    }

    void FullFileLoadCB::close()
    {
        if (fileHandle)
        {
            hal::df_close(fileHandle);
            fileHandle = NULL;
        }
    }

    void FullFileLoadCB::beginFullFileBlock()
    {
        NAU_VERIFY(blocks.size() == 0);
        blocks.emplace_back(Block{0, hal::df_length(fileHandle)});
    }

    std::span<const char> FullFileLoadCB::getTargetRomData() const
    {
        int data_sz = 0;
        if (const char* data = hal::df_get_vromfs_file_data_for_file_ptr(fileHandle, data_sz))
        {
            const std::span<const char> res(data, data_sz);
            return res;
        }
        return {};
    }

    FullFileSaveCB::FullFileSaveCB(const char* fname)
    {
        fileHandle = NULL;
        open(fname, hal::DF_WRITE | hal::DF_CREATE);
    }

    bool FullFileSaveCB::open(const char* fname, int mode)
    {
        close();
        fileHandle = hal::df_open(fname, mode);
        targetFilename = fname;
        return fileHandle != NULL;
    }

    void FullFileSaveCB::close()
    {
        if (fileHandle)
        {
            hal::df_close(fileHandle);
            fileHandle = NULL;
        }
    }
}  // namespace nau::iosys