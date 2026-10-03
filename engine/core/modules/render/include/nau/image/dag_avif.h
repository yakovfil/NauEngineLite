// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

// forward declarations for external classes
struct TexImage32;
struct TexPixel32;
class IGenLoad;


// load AVIF file (RGBX/RGBA)
TexImage32* load_avif32(const char* fn, nau::HeapByteAllocator* mem, bool* out_used_alpha = nullptr);
TexImage32* load_avif32(IGenLoad& crd, nau::HeapByteAllocator* mem, bool* out_used_alpha = nullptr);
#include "nau/memory/heap_byte_allocator.h"
