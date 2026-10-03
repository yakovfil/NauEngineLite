// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include "nau/core_defines.h"

#ifdef NAU_PLATFORM_WIN32
    #include "nau/platform/windows/windows_headers.h"
#endif

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "lua_toolkit/lua_headers.h"
#include "nau/rtti/ptr.h"
#include "nau/threading/lock_guard.h"
