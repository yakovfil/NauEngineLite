// Copyright 2024 N-GINN LLC. All rights reserved.
// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <nau/dag_ioSys/dag_baseIo.h>

#include <string>

#include "nau/kernel/kernel_config.h"
namespace nau::iosys
{
    /// @addtogroup utility_classes
    /// @{

    /// @addtogroup serialization
    /// @{

    /// @file
    /// Serialization callbacks.

    // generic load interface implemented as async reader
    class NAU_KERNEL_EXPORT AsyncLoadCachedCB : public IBaseLoad
    {
        struct
        {
            int size;
            int pos;
            void* handle;
        } file;

        struct
        {
            int size;
            int used;
            int pos;
            char* data;
        } buf;
        std::string targetFilename;

    public:
        AsyncLoadCachedCB(const char* fpath);
        ~AsyncLoadCachedCB();

        inline bool isOpen()
        {
            return file.handle != NULL;
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

    /// @}

    /// @}
}  // namespace nau::iosys
