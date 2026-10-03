// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// nau/diag/source_info.h

#pragma once

#include <optional>
#include <string_view>

#include "nau/kernel/kernel_config.h"
#include "nau/utils/preprocessor.h"

namespace nau::diag
{

    /**
     */
    struct NAU_KERNEL_EXPORT SourceInfo
    {
        std::string_view moduleName;
        std::string_view functionName;
        std::string_view filePath;
        std::optional<unsigned> line;

        SourceInfo() = default;
        SourceInfo(const SourceInfo&) = default;

        SourceInfo(std::string_view module, std::string_view func, std::string_view file, std::optional<unsigned> ln = std::nullopt) :
            moduleName(module),
            functionName(func),
            filePath(file),
            line(ln)
        {
        }

        SourceInfo(std::string_view func, std::string_view file, std::optional<unsigned> ln = std::nullopt) :
            SourceInfo{{}, func, file, ln}
        {
        }

        SourceInfo& operator=(const SourceInfo&) = default;

        explicit operator bool() const
        {
            return !functionName.empty() || !filePath.empty();
        }
    };

}  // namespace nau::diag

#define NAU_INLINED_SOURCE_INFO                                                                         \
    ::nau::diag::SourceInfo                                                                             \
    {                                                                                                   \
        ::std::string_view{__FUNCTION__}, ::std::string_view{__FILE__}, static_cast<unsigned>(__LINE__) \
    }
