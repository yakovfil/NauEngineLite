// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

// forward declarations for external classes
struct TexImage32;
class IGenLoad;


// load PSD file (RGBX/RGBA)
TexImage32* load_psd32(const char* fn, nau::HeapByteAllocator* mem, bool* out_used_alpha);
TexImage32* load_psd32(IGenLoad& crd, nau::HeapByteAllocator* mem, bool* out_used_alpha);
#include "nau/memory/heap_byte_allocator.h"
