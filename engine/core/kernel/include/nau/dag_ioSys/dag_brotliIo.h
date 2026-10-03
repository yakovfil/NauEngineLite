// Copyright 2024 N-GINN LLC. All rights reserved.
// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <nau/dag_ioSys/dag_memIo.h>

#include <span>
#include <vector>
namespace nau::iosys
{
    typedef struct BrotliDecoderStateStruct* BrotliDecoderStateCtx;

    struct NAU_KERNEL_EXPORT BrotliStreamDecompress : public IStreamDecompress
    {
        BrotliStreamDecompress();
        ~BrotliStreamDecompress();
        virtual StreamDecompressResult decompress(std::span<const uint8_t> in, std::vector<char>& out, size_t* nbytes_read = nullptr) override;

    private:
        static constexpr int TEMP_BUFFER_SIZE = 16384;
        uint8_t tmpBuffer[TEMP_BUFFER_SIZE];
        BrotliDecoderStateCtx state;
    };
}  // namespace nau::iosys