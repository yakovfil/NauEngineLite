// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/app/application_init_delegate.h"

#include <optional>
#include <string>
#include <vector>

#include "./logging_service.h"
#include "nau/app/global_properties.h"
#include "nau/assets/asset_db.h"
#include "nau/io/asset_pack_file_system.h"
#include "nau/io/virtual_file_system.h"
#include "nau/string/string_conv.h"

namespace nau
{
    namespace
    {
        /**
         */
        struct LogFileEntry
        {
            std::string contentType;
            std::string location;
            std::string fileNamePrefix;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(contentType),
                CLASS_FIELD(location),
                CLASS_FIELD(fileNamePrefix))
        };

        /**
         */
        struct LogConfig
        {
            std::vector<RuntimeValue::Ptr> files;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(files))
        };

        /**
         **/
        struct VfsMountPoint
        {
            std::string mountPoint;
            std::string kind;
            std::string entryPoint;
            std::string path;
            bool readOnly = true;
            bool forceCreate = false;
            bool isOptional = false;
            bool enabled = true;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(mountPoint),
                CLASS_FIELD(kind),
                CLASS_FIELD(entryPoint),
                CLASS_FIELD(path),
                CLASS_FIELD(readOnly),
                CLASS_FIELD(forceCreate),
                CLASS_FIELD(isOptional),
                CLASS_FIELD(enabled))
        };

        /**
         */
        struct VfsConfig
        {
            std::vector<VfsMountPoint> mounts;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(mounts))
        };

        /**
         */
        struct AssetDbConfig
        {
            std::vector<VfsMountPoint> mounts;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(mounts))
        };

        /**
         */
        struct AppConfig
        {
            std::string name;
            std::string author;
            LogConfig log;
            VfsConfig vfs;
            AssetDbConfig asset_db;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(name),
                CLASS_FIELD(author),
                CLASS_FIELD(log),
                CLASS_FIELD(vfs),
                CLASS_FIELD(asset_db))
        };

        Result<> setupLog(const LogConfig& logConfig)
        {
            namespace fs = std::filesystem;
            using namespace nau::strings;

            LoggingService& loggingService = getServiceProvider().get<LoggingService>();

            for (const RuntimeValue::Ptr& entry : logConfig.files)
            {
                NAU_FATAL(entry);

                if (const RuntimeStringValue* const str = entry->as<const RuntimeStringValue*>())
                {
                    const std::string logFilePath{toStringView(str->getString())};
                    loggingService.addFileOutput(logFilePath);
                }
                else if (entry->is<RuntimeReadonlyDictionary>())
                {
                    const Result<LogFileEntry> fileEntry = runtimeValueCast<LogFileEntry>(entry);
                    NauCheckResult(fileEntry)

                    if (fileEntry->fileNamePrefix.empty())
                    {
                        loggingService.addFileOutput(fileEntry->location);
                    }
                    else
                    {
                        const fs::path location{toStringView(utf8ToWString(fileEntry->location))};
                        NAU_ASSERT(!fs::exists(location) || fs::is_directory(location));
                        const fs::path filePath = location / toStringView(utf8ToWString(fileEntry->fileNamePrefix));
                        const std::u8string utf8FilePath = wstringToUtf8(toStringView(filePath.wstring()));
                        loggingService.addFileOutput(toStringView(utf8FilePath));
                    }
                }
                else
                {
                    return NauMakeError("Invalid log file entry");
                }
            }

            return ResultSuccess;
        }

        Result<> setupVfs(const VfsConfig& vfsConfig)
        {
            namespace fs = std::filesystem;
            using namespace nau::strings;

            io::IVirtualFileSystem& vfs = getServiceProvider().get<io::IVirtualFileSystem>();

            for (const auto& mount : vfsConfig.mounts)
            {
                if (!mount.enabled)
                {
                    continue;
                }

                fs::path mountPath = toStringView((utf8ToWString(mount.path)));
                if (!fs::exists(mountPath))
                {
                    if (mount.forceCreate)
                    {
                        std::error_code ec;
                        if (!fs::create_directories(mountPath, ec))
                        {
                            NAU_FATAL(mount.isOptional, L"Fail to create non-existent path:({})", mountPath.wstring());
                            continue;
                        }
                    }
                    else
                    {
                        NAU_FATAL(mount.isOptional, L"Attempt to mount a non-existent path:({})", mountPath.wstring());
                        continue;
                    }
                }

                mountPath.make_preferred();
                const std::u8string utf8Path = wstringToUtf8(toStringView(mountPath.wstring()));

                if (fs::is_directory(mountPath))
                {
                    io::IFileSystem::Ptr contentFs = io::createNativeFileSystem(reinterpret_cast<const char*>(utf8Path.c_str()), mount.readOnly);
                    vfs.mount(mount.mountPoint, std::move(contentFs)).ignore();
                }
                else
                {
                    NAU_ASSERT(fs::is_regular_file(mountPath));

                    io::IFileSystem::Ptr packFs = io::createAssetPackFileSystem(utf8Path);
                    vfs.mount(mount.mountPoint, std::move(packFs)).ignore();
                }
            }

            return ResultSuccess;
        }

        Result<> setupAssetDatabase(const AssetDbConfig& dbConfig)
        {
            namespace fs = std::filesystem;
            using namespace nau::strings;

            auto& fileSystem = getServiceProvider().get<io::IFileSystem>();
            auto& assetDb = getServiceProvider().get<nau::IAssetDB>();

            // In order to mount asset_database.db the foulder with compiled assets (asset_database, assets.content etc) must exist and must be mounted first
            for (const auto& mount : dbConfig.mounts)
            {
                if (!mount.enabled)
                {
                    continue;
                }

                if (!fileSystem.exists(mount.entryPoint))
                {
                    NAU_LOG_WARNING("Asset database not found: {}", mount.entryPoint);
                    continue;
                }

                NAU_ASSERT(mount.kind == "asset_db", "Invalid mount kind!");

                assetDb.addAssetDB(mount.entryPoint);

                NAU_LOG("Mounted asset database: {}", mount.entryPoint);
            }

            return ResultSuccess;
        }

    }  // namespace

    Result<> applyDefaultAppConfiguration()
    {
        const std::optional<AppConfig> appConfig = getServiceProvider().get<GlobalProperties>().getValue<AppConfig>("/app");

        if (!appConfig)
        {
            return ResultSuccess;
        }

        NauCheckResult(setupLog(appConfig->log))
        NauCheckResult(setupVfs(appConfig->vfs))

        return ResultSuccess;
    }

    Result<> initializeDefaultApplication()
    {
        const std::optional<AppConfig> appConfig = getServiceProvider().get<GlobalProperties>().getValue<AppConfig>("/app");

        if (!appConfig)
        {
            return ResultSuccess;
        }

        NauCheckResult(setupAssetDatabase(appConfig->asset_db))

        return ResultSuccess;
    }

}  // namespace nau
