// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/diag/error.h"

#include <string>

namespace nau
{
    std::string Error::getDiagMessage() const
    {
        const auto source = getSource();
        if (!source)
        {
            return getMessage();
        }

        const std::string message = source.line ? ::fmt::format("{}({}): error:{}", source.filePath, *source.line, getMessage()) : ::fmt::format("{}: error:{}", source.filePath, getMessage());

        return {message.data(), message.size()};
    }

}  // namespace nau
