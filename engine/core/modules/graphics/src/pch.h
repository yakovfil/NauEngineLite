// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include "nau/core_defines.h"

#ifdef NAU_PLATFORM_WIN32
    #include "nau/platform/windows/windows_headers.h"
#endif

// clang-format off

#include <optional>
#include <string>
#include <memory>
#include <vector>


#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <dxgi1_6.h>

#include <wrl.h>

// clang-format on

#include <atomic>
#include <memory>
#include <mutex>

#include "nau/app/window_manager.h"
#include "nau/platform/windows/app/windows_window.h"
#include "nau/rtti/ptr.h"
#include "nau/service/service.h"
#include "nau/service/service_provider.h"
#include "nau/threading/lock_guard.h"
#include "nau/io/virtual_file_system.h"


