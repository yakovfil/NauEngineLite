// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#ifdef _WIN32
    #include "nau/platform/windows/windows_headers.h"
#endif

#include <atomic>
#include <list>
#include <memory>
#include <mutex>
#include <numeric>
#include <regex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "nau/async/multi_task_source.h"
#include "nau/async/task.h"
#include "nau/async/task_collection.h"
#include "nau/async/work_queue.h"
#include "nau/diag/assertion.h"
#include "nau/diag/logging.h"
#include "nau/memory/stack_allocator.h"
#include "nau/memory/standard_aliases.h"
#include "nau/rtti/ptr.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/service/service_provider.h"
#include "nau/threading/lock_guard.h"
#include "nau/utils/intrusive_list.h"
