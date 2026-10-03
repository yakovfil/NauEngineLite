// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string>
#include <vector>

#include "nau/assets/asset_meta_info.h"
#include "nau/io/fs_path.h"
#include "nau/rtti/type_info.h"

namespace nau
{
    struct NAU_ABSTRACT_TYPE IAssetDB
    {
        NAU_TYPEID(nau::IAssetDB)

        virtual void addAssetDB(io::FsPath dbPath) = 0;

        virtual void reloadAssetDB(io::FsPath dbPath) = 0;

        virtual AssetMetaInfoBase findAssetMetaInfoByUid(const Uid& uid) const = 0;
        virtual std::vector<AssetMetaInfoBase> findAssetMetaInfoByKind(const std::string& kind) const = 0;

        virtual std::string getNausdPathFromUid(const Uid& uid) const = 0;
        virtual Uid getUidFromNausdPath(const std::string& nausdPath) const = 0;

        virtual std::string getSourcePathFromUid(const Uid& uid) const = 0;
        virtual Uid getUidFromSourcePath(const std::string& sourcePath) const = 0;

        virtual std::string getSourcePathFromNausdPath(const std::string& nausdPath) const = 0;
        virtual std::string getNausdPathFromSourcePath(const std::string& sourcePath) const = 0;
    };
}  // namespace nau
