// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include "nau/asset_pack/asset_pack_builder.h"

#include <string>
#include <tuple>
#include <vector>

#include "nau/io/asset_pack.h"
#include "nau/io/file_system.h"
#include "nau/io/nau_container.h"
#include "nau/io/special_paths.h"
#include "nau/io/stream.h"
#include "nau/memory/bytes_buffer.h"
#include "nau/serialization/runtime_value_builder.h"
#include "nau/string/string_conv.h"

namespace nau
{
    Result<io::AssetPackIndexData> writeAssetPackIndexDataToStream(const std::vector<PackInputFileData>& content, PackBuildOptions buildOptions, const std::string& tempFilePath)
    {
        using namespace io;
        AssetPackIndexData packData;
        packData.version = buildOptions.version;
        packData.description = buildOptions.description;

        IStreamWriter::Ptr tempStream = createNativeFileStream(tempFilePath.data(), AccessMode::Write, OpenFileMode::CreateAlways);

        for (const PackInputFileData& content : content)
        {
            IStreamReader::Ptr srcStream = content.stream();
            NAU_ASSERT(srcStream, "Invalid stream:({})", content.filePathInPack);
            if (!srcStream)
            {
                continue;
            }

            AssetPackFileEntry& packEntry = packData.content.emplace_back();
            packEntry.blobData.offset = tempStream->getPosition();
            copyStream(*tempStream, *srcStream).ignore();

            packEntry.filePath = content.filePathInPack;
            packEntry.contentCompression = packEntry.clientSize;
            packEntry.clientSize = srcStream->getPosition();
            packEntry.blobData.size = srcStream->getPosition();
        }

        tempStream->flush();
        return packData;
    }

    Result<> buildAssetPackage(const std::vector<PackInputFileData>& content, PackBuildOptions buildOptions, io::IStreamWriter::Ptr outputStream)
    {
        using namespace io;
        const std::u8string u8tempFilePath = getNativeTempFilePath();
        const std::string tempFilePath(u8tempFilePath.cbegin(), u8tempFilePath.cend());

        AssetPackIndexData packIndexData = *writeAssetPackIndexDataToStream(content, buildOptions, tempFilePath);
        writeContainerHeader(outputStream, "nau-vfs-pack", nau::makeValueRef(packIndexData));

        IStreamReader::Ptr temp = createNativeFileStream(tempFilePath.data(), AccessMode::Read, OpenFileMode::OpenExisting);
        copyStream(*outputStream, *temp).ignore();

        return {};
    }

    Result<io::AssetPackIndexData> readAssetPackage(io::IStreamReader::Ptr packageStream)
    {
        RuntimeValue::Ptr packData;
        size_t headerDataOffset;
        std::tie(packData, headerDataOffset) = *readContainerHeader(packageStream);

        io::AssetPackIndexData packIndexData;
        auto value = nau::makeValueRef(packIndexData);
        auto res = RuntimeValue::assign(value, packData);

        for (io::AssetPackFileEntry& content : packIndexData.content)
        {
            content.blobData.offset += headerDataOffset;
        }

        return {packIndexData};
    }
}  // namespace nau