// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#ifdef _WIN32
    #include "nau/platform/windows/windows_headers.h"
#endif

// clang-format off

#include <string>
#include <memory>
#include <vector>

// clang-format on

#include <atomic>
#include <memory>
#include <mutex>

#include "nau/math/math.h"
#include "nau/math/transform.h"
#include "nau/rtti/ptr.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/threading/lock_guard.h"
