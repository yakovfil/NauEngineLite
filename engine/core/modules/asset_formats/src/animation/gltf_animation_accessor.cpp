// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "gltf_animation_accessor.h"

#include <algorithm>
#include <vector>

#include "nau/assets/skeleton_asset_accessor.h"
#include "nau/io/file_system.h"
#include "nau/io/stream_utils.h"
#include "nau/service/service_provider.h"

namespace nau
{

    template <typename TData>
    TData readElement(const std::vector<float>& rawData, size_t& seekp)
    {
        return TData(rawData[seekp++]);
    }

    template <>
    math::vec3 readElement(const std::vector<float>& rawData, size_t& seekp)
    {
        math::vec3 result{
            rawData[seekp],
            rawData[seekp + 1],
            rawData[seekp + 2]};

        seekp += 3;
        return result;
    }

    template <>
    math::quat readElement(const std::vector<float>& rawData, size_t& seekp)
    {
        math::quat result{
            rawData[seekp],
            rawData[seekp + 1],
            rawData[seekp + 2],
            rawData[seekp + 3]};

        seekp += 4;
        return result;
    }

    template <typename TData>
    void readRawGltfData(io::IStreamReader* reader, size_t size, std::vector<TData>& result)
    {
        const size_t numFloats = size / sizeof(float);
        std::vector<float> rawData;
        rawData.resize(numFloats);
        io::copyFromStream(rawData.data(), size, *reader).ignore();

        size_t seekp = 0;
        while (seekp < rawData.size())
        {
            result.push_back(readElement<TData>(rawData, seekp));
        }
    }

    template <typename TData>
    static void readBinaryData(const GltfAnimationAssetAccessor::BinaryAccessor& binAccessor, std::vector<TData>& result)
    {
        auto inputStream = binAccessor.file->createStream(io::AccessMode::Read);
        inputStream->setPosition(io::OffsetOrigin::Begin, binAccessor.offset);

        auto* reader = inputStream->as<io::IStreamReader*>();
        NAU_ASSERT(reader);

        if (reader)
        {
            readRawGltfData(reader, binAccessor.size, result);
        }
    }

    template <typename TData>
    void GltfAnimationAssetAccessor::copyTrackData(
        const AnimationDataDescriptor& desc,
        TTimesContainer& times,
        TDataContainer<TData>& data) const
    {
        if (m_descriptor.animationDesc == desc)
        {
            readBinaryData(m_descriptor.timesAccessor, times);
            readBinaryData(m_descriptor.dataAccessor, data);
        }
    }

    GltfAnimationAssetAccessor::GltfAnimationAssetAccessor(const GltfFile& file, size_t animationIndex, size_t channelIndex, const std::vector<io::IFile::Ptr>& bufferFiles)
    {
        NAU_ASSERT(animationIndex < file.animations.size());

        const GltfAnimation& animationData = file.animations[animationIndex];
        const GltfAnimation::Channel& channel = animationData.channels[channelIndex];

        const GltfAnimation::Sampler& sampler = animationData.samplers[channel.sampler];
        const GltfAccessor& timeAccessor = file.accessors[sampler.input];
        const GltfBufferView& timeBufferView = file.bufferViews[timeAccessor.bufferView];
        const GltfAccessor& dataAccessor = file.accessors[sampler.output];
        const GltfBufferView& dataBufferView = file.bufferViews[dataAccessor.bufferView];

        AnimationDataDescriptor::DataType dataType = AnimationDataDescriptor::DataType::Unsupported;

        if (channel.target.path == "translation")
        {
            dataType = AnimationDataDescriptor::DataType::Translation;
        }
        else if (channel.target.path == "rotation")
        {
            dataType = AnimationDataDescriptor::DataType::Rotation;
        }
        else if (channel.target.path == "scale")
        {
            dataType = AnimationDataDescriptor::DataType::Scale;
        }

        NAU_ASSERT(dataType != AnimationDataDescriptor::DataType::Unsupported);

        if (dataType == AnimationDataDescriptor::DataType::Unsupported)
        {
            return;
        }

        AnimationDataDescriptor::InterpolationType interpolation = AnimationDataDescriptor::InterpolationType::No;

        if (sampler.interpolation == "LINEAR")
        {
            interpolation = AnimationDataDescriptor::InterpolationType::Linear;
        }

        m_descriptor.animationDesc = AnimationDataDescriptor{
            animationIndex,
            channelIndex,
            dataType,
            interpolation,
            animationData.name};

        auto& timeStreamAccessor = m_descriptor.timesAccessor;
        timeStreamAccessor.file = bufferFiles[timeBufferView.buffer];
        timeStreamAccessor.offset = timeBufferView.byteOffset;
        timeStreamAccessor.size = timeBufferView.byteLength;

        auto& dataStreamAccessor = m_descriptor.dataAccessor;
        dataStreamAccessor.file = bufferFiles[dataBufferView.buffer];
        dataStreamAccessor.offset = dataBufferView.byteOffset;
        dataStreamAccessor.size = dataBufferView.byteLength;
    }

    AnimationDataDescriptor GltfAnimationAssetAccessor::getDataDescriptor() const
    {
        return m_descriptor.animationDesc;
    }

    async::Task<> GltfAnimationAssetAccessor::copyVectors(const AnimationDataDescriptor& desc, TTimesContainer& times, TDataContainer<math::vec3>& data) const
    {
        copyTrackData(desc, times, data);

        return async::Task<>::makeResolved();
    }

    async::Task<> GltfAnimationAssetAccessor::copyRotations(const AnimationDataDescriptor& desc, TTimesContainer& times, TDataContainer<math::quat>& data) const
    {
        copyTrackData(desc, times, data);

        return async::Task<>::makeResolved();
    }

    async::Task<> GltfAnimationAssetAccessor::copyFramesData(const AnimationDataDescriptor& desc, DataBlock& data) const
    {
        auto dataFilePath = fmt::format("/content/scenes/animation/{}.blk", desc.name);
        auto& fileSystem = getServiceProvider().get<io::IFileSystem>();

        if (fileSystem.exists(dataFilePath))
        {
            io::IFile::Ptr file = fileSystem.openFile(dataFilePath, io::AccessMode::Read, io::OpenFileMode::OpenExisting);
            NAU_FATAL(file);

            io::IStreamReader::Ptr stream = file->createStream();
            NAU_FATAL(stream);

            io::GenLoadOverStream genLoad{stream};
            [[maybe_unused]] const bool readOk = data.loadFromStream(genLoad);
            NAU_ASSERT(readOk);
        }

        return async::Task<>::makeResolved();
    }

    nau::Ptr<ISkeletonAssetAccessor> GltfAnimationAssetAccessor::getSkeletonAsset() const
    {
        return nullptr;
    }

}  // namespace nau
