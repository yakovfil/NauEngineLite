// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#ifdef _WIN32
    #include "nau/platform/windows/windows_headers.h"
#endif

// clang-format off
#include <algorithm>
#include <string>
#include <memory>
#include <vector>

// clang-format on

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

#include <atomic>
#include <memory>
#include <mutex>

#include "nau/diag/assertion.h"
#include "nau/diag/logging.h"
#include "nau/rtti/ptr.h"
#include "nau/threading/lock_guard.h"
