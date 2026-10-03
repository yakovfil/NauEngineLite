// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <nau/core_defines.h>

#ifdef NAU_PLATFORM_WIN32
    #include "nau/platform/windows/windows_headers.h"
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <exception>
#include <filesystem>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <regex>
#include <set>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "nau/diag/assertion.h"
#include "nau/diag/logging.h"
#include "nau/service/service_provider.h"
#include "nau/string/string_utils.h"
#include "nau/threading/lock_guard.h"
#include "nau/utils/performance_profiling.h"