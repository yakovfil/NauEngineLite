// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include <cstdio>
#include <cstdlib>

// EASTL's unsupported-architecture fallback explicitly requires this user hook.
void EASTL_DEBUG_BREAK()
{
    std::fputs("EASTL assertion failed in WebAssembly\n", stderr);
    std::abort();
}
