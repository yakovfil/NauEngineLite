// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <cstdlib>

#define NAU_PLATFORM_BREAK (__builtin_trap())
#define NAU_PLATFORM_ABORT (std::abort())
