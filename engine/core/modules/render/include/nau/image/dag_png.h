// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

// forward declarations for external classes
struct TexImage32;
struct TexPixel32;
class IGenLoad;


// load PNG file (RGBX/RGBA)
TexImage32* load_png32(const char* fn, nau::HeapByteAllocator* mem, bool* out_used_alpha);
TexImage32* load_png32(IGenLoad& crd, nau::HeapByteAllocator* mem, bool* out_used_alpha);

bool save_png32(const char* fn, TexPixel32* ptr, int wd, int ht, int stride,
                unsigned char* text_data,  // should be nullptr or valid C-string
                unsigned int text_data_len,
                bool save_alpha = true);
#include "nau/memory/heap_byte_allocator.h"
