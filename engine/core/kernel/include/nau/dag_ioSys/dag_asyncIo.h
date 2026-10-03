// Copyright 2024 N-GINN LLC. All rights reserved.
// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <nau/dag_ioSys/dag_baseIo.h>
#include <nau/kernel/kernel_config.h>

#include <string>
namespace nau::iosys
{
    // generic load interface implemented as async reader
    class NAU_KERNEL_EXPORT AsyncLoadCB : public IBaseLoad
    {
        struct
        {
            int size;
            int pos;
            void* handle;
        } file;

        struct
        {
            int minimumChunk;  // if 1 - buffering should be on
            int size;          /// size= (pow of 2) * minimumChunk;
            int used;
            int pos;
            char* data;
        } buf;
        std::string targetFilename;

        void readBuffered(void* ptr, int size);

    public:
        AsyncLoadCB(const char* fpath);
        ~AsyncLoadCB();

        inline bool isOpen()
        {
            return file.handle != NULL;
        }
        inline int fileSize()
        {
            return file.size;
        }

        virtual void read(void* ptr, int size) override;
        virtual int tryRead(void* ptr, int size) override;
        virtual int tell() override;
        virtual void seekto(int) override;
        virtual void seekrel(int ofs) override;
        virtual const char* getTargetName() override
        {
            return targetFilename.c_str();
        }
        int64_t getTargetDataSize() override
        {
            return file.size;
        }
    };
}  // namespace nau::iosys