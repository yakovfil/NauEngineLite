// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <vector>
#include <string>
#include <algorithm>



namespace nau::hal::detail
{
    inline void remove_duplicate_file_paths(std::vector<std::string>& paths)
    {
        std::sort(paths.begin(), paths.end());
        paths.erase(std::unique(paths.begin(), paths.end()), paths.end());
    }
}  // namespace nau::hal::detail
