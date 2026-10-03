// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "./asset_file_content_provider.h"

#include <string>
#include <string_view>
#include <vector>

#include "nau/assets/asset_container.h"
#include "nau/assets/import_settings_provider.h"
#include "nau/io/file_system.h"
#include "nau/service/service_provider.h"

namespace nau
{
    Result<IAssetContentProvider::AssetContent> AssetFileContentProvider::openStreamOrContainer(const AssetPath& assetPath)
    {
        NAU_ASSERT(assetPath);
        NAU_ASSERT(assetPath.hasScheme("file"));

        auto& fileSystem = getServiceProvider().get<io::IFileSystem>();

        const std::string_view containerPath = assetPath.getContainerPath();

        auto file = fileSystem.openFile(containerPath, {io::AccessMode::Read, io::AccessMode::Async}, io::OpenFileMode::OpenExisting);
        if (!file)
        {
            NAU_LOG_ERROR("Can't open file. File path: ({})", containerPath);
            return NauMakeError("Can't open file. File path: ({})", containerPath);
        }

        IAssetContainerLoader* loader = nullptr;
        const auto importSettingsProviders = getServiceProvider().getAll<IImportSettingsProvider>();
        RuntimeReadonlyDictionary::Ptr importSettings;
        for (const auto& importSettingsProvider : importSettingsProviders)
        {
            if (importSettings = importSettingsProvider->getAssetImportSettings(containerPath, *loader); importSettings)
            {
                break;
            }
        }

        io::FsPath fileFsPath(containerPath);
        const std::string_view ext = strings::toStringView(fileFsPath.getExtension());
        if (ext.empty())
        {
            NAU_LOG_WARNING("Opening file with empty extension. Provider can not determine asset kind:({})", containerPath);
        }

        std::string assetKind(ext.empty() ? std::string_view{} : ext.substr(1, ext.size()));

        return AssetContent{
            file->createStream(),
            {.kind = std::move(assetKind), .path = std::move(containerPath), .importSettings = importSettings ? importSettings->as<RuntimeObject*>() : nullptr}
        };
    }

    std::vector<std::string_view> AssetFileContentProvider::getSupportedSchemes() const
    {
        return {"file"};
    }
}  // namespace nau
