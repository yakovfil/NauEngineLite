// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "./asset_db_impl.h"

#include <algorithm>
#include <format>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "nau/diag/logging.h"
#include "nau/io/file_system.h"
#include "nau/io/fs_path.h"
#include "nau/serialization/json.h"
#include "nau/serialization/runtime_value_builder.h"
#include "nau/service/service_provider.h"

namespace nau
{
    namespace
    {
        io::FsPath stripRootPath(const AssetPath& path)
        {
            const auto filePath = io::FsPath(path.getContainerPath());
            return filePath.getRelativePath(filePath.getRootPath());
        }

    }  // namespace

    void AssetDBImpl::addAssetDB(io::FsPath dbPath)
    {
        lock_(m_mutex);
        addAssetDBInternal(dbPath);
    }

    void AssetDBImpl::reloadAssetDB(io::FsPath dbPath)
    {
        lock_(m_mutex);
        reloadAssetDBInternal(dbPath);
    }

    AssetMetaInfoBase AssetDBImpl::findAssetMetaInfoByUid(const Uid& uid) const
    {
        auto it = m_allAssets.find(uid);
        if (it != m_allAssets.end())
        {
            return it->second;
        }

        NAU_LOG_WARNING("Can't find nausdPath by asset uid({})", toString(uid));
        return {};
    }

    std::vector<AssetMetaInfoBase> AssetDBImpl::findAssetMetaInfoByKind(const std::string& kind) const
    {
        std::vector<AssetMetaInfoBase> result;
        for (const auto& it : m_allAssets)
        {
            if (it.second.kind == kind)
            {
                result.emplace_back(it.second);
            }
        }
        return result;
    }

    std::string AssetDBImpl::getNausdPathFromUid(const Uid& uid) const
    {
        auto it = m_allAssets.find(uid);
        if (it != m_allAssets.end())
        {
            return it->second.nausdPath;
        }

        NAU_LOG_WARNING("Can't find nausdPath by asset uid({})", toString(uid));
        return {};
    }

    Uid AssetDBImpl::getUidFromNausdPath(const std::string& nausdPath) const
    {
        auto it = std::find_if(m_allAssets.cbegin(), m_allAssets.cend(), [nausdPath](const auto& assetInfo)
        {
            return nausdPath == assetInfo.second.nausdPath;
        });

        if (it != m_allAssets.end())
        {
            return it->second.uid;
        }

        NAU_LOG_WARNING("Can't find asset uid by nausdPath({})", nausdPath);
        return NullUid;
    }

    std::string AssetDBImpl::getSourcePathFromUid(const Uid& uid) const
    {
        auto it = m_allAssets.find(uid);
        if (it != m_allAssets.end())
        {
            return it->second.sourcePath;
        }
        NAU_LOG_WARNING("Can't find source path by asset uid({})", toString(uid));
        return {};
    }

    Uid AssetDBImpl::getUidFromSourcePath(const std::string& sourcePath) const
    {
        auto it = std::find_if(m_allAssets.cbegin(), m_allAssets.cend(), [sourcePath](const auto& assetInfo)
        {
            return sourcePath == assetInfo.second.sourcePath;
        });

        if (it != m_allAssets.end())
        {
            return it->second.uid;
        }

        NAU_LOG_WARNING("Can't find asset uid by sourcePath({})", sourcePath);
        return NullUid;
    }

    std::string AssetDBImpl::getSourcePathFromNausdPath(const std::string& nausdPath) const
    {
        auto it = std::find_if(m_allAssets.cbegin(), m_allAssets.cend(), [nausdPath](const auto& assetInfo)
        {
            return nausdPath == assetInfo.second.nausdPath;
        });

        if (it != m_allAssets.end())
        {
            return it->second.sourcePath;
        }

        NAU_LOG_WARNING("Can't find sourcePath by nausdPath({})", nausdPath);
        return {};
    }

    std::string AssetDBImpl::getNausdPathFromSourcePath(const std::string& sourcePath) const
    {
        auto it = std::find_if(m_allAssets.cbegin(), m_allAssets.cend(), [sourcePath](const auto& assetInfo)
        {
            return sourcePath == assetInfo.second.sourcePath;
        });

        if (it != m_allAssets.end())
        {
            return it->second.nausdPath;
        }

        NAU_LOG_WARNING("Can't find nausdPath by sourcePath({})", sourcePath);
        return {};
    }

    std::tuple<AssetPath, AssetContentInfo> AssetDBImpl::resolvePath(const AssetPath& assetPath)
    {
        using namespace nau::strings;

        io::FsPath assetFsPath;

        {
            shared_lock_(m_mutex);

            auto assetInfoIter = m_allAssets.cend();

            if (assetPath.hasScheme("asset"))
            {
                const nau::io::FsPath relativePath = stripRootPath(assetPath);

                assetInfoIter = std::find_if(m_allAssets.cbegin(), m_allAssets.cend(), [relativePath](const auto& assetInfo)
                {
                    const std::string fullName = std::format("{}.{}", assetInfo.second.sourcePath.c_str(), assetInfo.second.sourceType.c_str());
                    return fullName == relativePath;
                });
            }
            else if (assetPath.hasScheme("uid"))
            {
                const Result<Uid> uid = Uid::parseString(strings::toStringView(assetPath.getContainerPath()));
                if (uid)
                {
                    assetInfoIter = m_allAssets.find(*uid);
                }
                else
                {
                    NAU_LOG_ERROR("Invalid uid value ({}):({})", assetPath.getContainerPath(), uid.getError()->getMessage());
                }
            }
            else
            {
                NAU_FAILURE("Unsupported scheme: ({})", assetPath.getScheme());
                return {};
            }

            if (assetInfoIter != m_allAssets.end())
            {
                assetFsPath = assetInfoIter->second.dbPath;
                assetFsPath.makeAbsolute();
            }
            else
            {
                NAU_LOG_ERROR("Asset info not found:({})", assetPath.toString());
                return {};
            }
        }

        NAU_FATAL(!assetFsPath.isEmpty());

        AssetPath resultAssetPath = assetPath;
        resultAssetPath.setScheme("file").setContainerPath(toStringView(assetFsPath.getString()));

        const std::string_view ext = assetFsPath.getExtension();
        if (ext.empty())
        {
            NAU_LOG_WARNING("Blob data currently is unsupported");
            return {};
        }

        return {
            std::move(resultAssetPath),
            {ext.substr(1, ext.size()).data(), std::move(assetFsPath), nullptr}
        };
    }

    std::vector<std::string_view> AssetDBImpl::getSupportedSchemes() const
    {
        return {"asset", "uid"};
    }

    void AssetDBImpl::addAssetDBInternal(io::FsPath dbPath)
    {
        auto it = std::find_if(m_allDbs.cbegin(), m_allDbs.cend(), [dbPath](const auto& assetDb)
        {
            return assetDb.rootPath == dbPath.getParentPath();
        });

        if (it != m_allDbs.end())
        {
            reloadAssetDBInternal(dbPath);
            return;
        }

        auto& fileSystem = getServiceProvider().get<io::IFileSystem>();
        auto file = fileSystem.openFile(dbPath, io::AccessMode::Read, io::OpenFileMode::OpenExisting);
        if (!file)
        {
            NAU_LOG_ERROR("Asset db not found: ({})", dbPath.getCStr());
            return;
        }

        auto parseResult = serialization::jsonParse(*file->createStream()->as<io::IStreamReader*>());
        NAU_VERIFY(parseResult);

        AssetDbInfo assetDb;
        auto res = runtimeValueApply(assetDb, *parseResult);
        if (!res)
        {
            NAU_LOG_ERROR("Fail to assign asset db value: ({})", res.getError()->getMessage());
            return;
        }

        AssetDBEntry& entry = m_allDbs.emplace_back();
        entry.uid = assetDb.uid ? assetDb.uid : Uid::generate();
        entry.rootPath = dbPath.getParentPath();

        for (auto& it : assetDb.content)
        {
            it.assetDbUid = entry.uid;
            it.dbPath = (entry.rootPath / it.dbPath).getCStr();
            m_allAssets.emplace(it.uid, it);
        }
    }

    void AssetDBImpl::reloadAssetDBInternal(io::FsPath dbPath)
    {
        auto it = std::find_if(m_allDbs.cbegin(), m_allDbs.cend(), [dbPath](const auto& assetDb)
        {
            return assetDb.rootPath == dbPath.getParentPath();
        });

        if (it == m_allDbs.end())
        {
            addAssetDBInternal(dbPath);
            return;
        }

        [[maybe_unused]] size_t count = std::erase_if(m_allAssets, [it](const auto& assetInfo)
        {
            return assetInfo.second.assetDbUid == (*it).uid;
        });

        m_allDbs.erase(it);
        addAssetDBInternal(dbPath);
    }
}  // namespace nau
