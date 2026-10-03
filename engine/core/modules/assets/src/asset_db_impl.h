// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "nau/assets/asset_db.h"
#include "nau/assets/asset_meta_info.h"
#include "nau/assets/asset_path_resolver.h"
#include "nau/utils/uid.h"

namespace nau
{
    struct AssetDBEntry
    {
        Uid uid;
        io::FsPath rootPath;
    };

    struct AssetMetaInfoInternal : AssetMetaInfoBase
    {
        Uid assetDbUid;

#pragma region Class Info
        NAU_CLASS_BASE(AssetMetaInfoBase)
        NAU_CLASS_FIELDS(
            CLASS_FIELD(assetDbUid))
#pragma endregion
    };

    struct AssetDbInfo
    {
        Uid uid;
        std::vector<AssetMetaInfoInternal> content;

#pragma region Class Info
        NAU_CLASS_FIELDS(
            CLASS_FIELD(uid),
            CLASS_FIELD(content))
#pragma endregion
    };

    class AssetDBImpl final : public IAssetDB,
                              public IAssetPathResolver
    {
        NAU_INTERFACE(nau::AssetDBImpl, IAssetDB, IAssetPathResolver)

        void addAssetDB(io::FsPath dbPath) override;

        void reloadAssetDB(io::FsPath dbPath) override;

        AssetMetaInfoBase findAssetMetaInfoByUid(const Uid& uid) const override;
        std::vector<AssetMetaInfoBase> findAssetMetaInfoByKind(const std::string& kind) const override;

        std::string getNausdPathFromUid(const Uid& uid) const override;
        Uid getUidFromNausdPath(const std::string& nausdPath) const override;

        std::string getSourcePathFromUid(const Uid& uid) const override;
        Uid getUidFromSourcePath(const std::string& sourcePath) const override;

        std::string getSourcePathFromNausdPath(const std::string& nausdPath) const override;
        std::string getNausdPathFromSourcePath(const std::string& sourcePath) const override;

        std::tuple<AssetPath, AssetContentInfo> resolvePath(const AssetPath& assetPath) override;

        std::vector<std::string_view> getSupportedSchemes() const override;

    private:
        void addAssetDBInternal(io::FsPath dbPath);
        void reloadAssetDBInternal(io::FsPath dbPath);

    private:
        std::map<Uid, AssetMetaInfoInternal> m_allAssets;
        std::vector<AssetDBEntry> m_allDbs;
        std::shared_mutex m_mutex;
    };
}  // namespace nau
