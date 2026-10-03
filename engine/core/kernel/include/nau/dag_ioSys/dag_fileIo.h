// Copyright 2024 N-GINN LLC. All rights reserved.
// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <nau/dag_ioSys/dag_baseIo.h>
#include <nau/kernel/kernel_config.h>

#include <span>
#include <string>
namespace nau::iosys
{
    // forward declarations for external classes
    typedef void* file_ptr_t;

    /// @addtogroup utility_classes
    /// @{

    /// @addtogroup serialization
    /// @{

    /// @file
    /// Serialization callbacks.

    /// File save callback.
    class NAU_KERNEL_EXPORT LFileGeneralSaveCB : public IBaseSave
    {
    public:
        file_ptr_t fileHandle;
        std::string targetFilename;

        LFileGeneralSaveCB(file_ptr_t handle = NULL);

        void write(const void* ptr, int size);
        int tryWrite(const void* ptr, int size);
        int tell();
        void seekto(int);
        void seektoend(int ofs = 0);
        virtual const char* getTargetName()
        {
            return targetFilename.c_str();
        }
        virtual void flush();
    };

    /// File load callback.
    class NAU_KERNEL_EXPORT LFileGeneralLoadCB : public IBaseLoad
    {
    public:
        file_ptr_t fileHandle;
        std::string targetFilename;

        LFileGeneralLoadCB(file_ptr_t handle = NULL);

        void read(void* ptr, int size);
        int tryRead(void* ptr, int size);
        int tell();
        void seekto(int);
        void seekrel(int);
        virtual const char* getTargetName()
        {
            return targetFilename.c_str();
        }
        virtual const VirtualRomFsData* getTargetVromFs() const;
    };

    /// Callback for reading whole file. Closes file in destructor.
    class NAU_KERNEL_EXPORT FullFileLoadCB : public LFileGeneralLoadCB
    {
        int64_t targetDataSz = -1;

    public:
        inline FullFileLoadCB(const char* fname, int mode)
        {
            fileHandle = NULL;
            targetFilename = fname;
            open(fname, mode);
        }
        FullFileLoadCB(const char* fname);
        inline ~FullFileLoadCB()
        {
            close();
        }

        bool open(const char* fname, int mode);
        void close();
        void beginFullFileBlock();
        virtual const char* getTargetName() override
        {
            return targetFilename.c_str();
        }
        int64_t getTargetDataSize() override
        {
            return targetDataSz;
        }
        virtual std::span<const char> getTargetRomData() const override;
    };

    /// Callback for writing whole file. Closes file in destructor.
    class NAU_KERNEL_EXPORT FullFileSaveCB : public LFileGeneralSaveCB
    {
    public:
        inline FullFileSaveCB()
        {
        }

        inline FullFileSaveCB(const char* fname, int mode)
        {
            fileHandle = NULL;
            targetFilename = fname;
            open(fname, mode);
        }

        FullFileSaveCB(const char* fname);

        inline ~FullFileSaveCB()
        {
            close();
        }

        bool open(const char* fname, int mode);
        void close();
        virtual const char* getTargetName()
        {
            return targetFilename.c_str();
        }
    };

    /// @}

    /// @}
}  // namespace nau::iosys