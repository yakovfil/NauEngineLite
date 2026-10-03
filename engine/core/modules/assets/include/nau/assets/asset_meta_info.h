// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string>

#include "nau/rtti/type_info.h"
#include "nau/utils/uid.h"

namespace nau
{
    struct AssetMetaInfoBase
    {
        Uid uid;
        std::string dbPath;
        std::string kind;
        std::string sourceType;
        std::string sourcePath;
        std::string nausdPath;

#pragma region Class Info
        NAU_CLASS_FIELDS(
            CLASS_FIELD(uid),
            CLASS_FIELD(dbPath),
            CLASS_FIELD(kind),
            CLASS_FIELD(sourceType),
            CLASS_FIELD(sourcePath),
            CLASS_FIELD(nausdPath))
#pragma endregion
    };
}  // namespace nau
