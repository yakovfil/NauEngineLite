// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/util/common.h"

#include "nau/memory/heap_byte_allocator.h"
#include "nau/osApiWrappers/basePath.h"

nau::HeapByteAllocator* midmem = new nau::HeapByteAllocator();

nau::HeapByteAllocator* inimem = new nau::HeapByteAllocator();

nau::HeapByteAllocator* strmem = new nau::HeapByteAllocator();

nau::HeapByteAllocator* tmpmem = new nau::HeapByteAllocator();

nau::HeapByteAllocator* globmem = new nau::HeapByteAllocator();
