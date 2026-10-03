// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nanim_animation_accessor.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "nanim_asset_container.h"
#include "nau/assets/asset_manager.h"
#include "nau/io/file_system.h"
#include "nau/io/stream_utils.h"
#include "nau/service/service_provider.h"

namespace nau
{
    struct NanimAnimationAssetAccessor::KeyFramesData
    {
        TTimesContainer times;
        TDataContainer<float> values;

        template<class TData>
        void append(const DataBlock& valueBlock)
        { }
        template<>
        void append<math::vec3>(const DataBlock& valueBlock)
        {
            const float time = valueBlock.getReal("t");
            const math::vec3 value = valueBlock.getPoint3("v");

            times.push_back(time);
            values.push_back(value.getX());
            values.push_back(value.getY());
            values.push_back(value.getZ());
        }
        template<>
        void append<math::quat>(const DataBlock& valueBlock)
        {
            const float time = valueBlock.getReal("t");
            const math::vec4 value = valueBlock.getPoint4("v");

            times.push_back(time);
            values.push_back(value.getX());
            values.push_back(value.getY());
            values.push_back(value.getZ());
            values.push_back(value.getW());
        }

        template<class TData>
        void fromBlk(const DataBlock& animationBlock)
        {
            if (const auto* valuesBlock = animationBlock.getBlockByName("values"))
            {
                for (int valueBlockIndex = 0; valueBlockIndex < valuesBlock->blockCount(); ++valueBlockIndex)
                {
                    if (const auto* valueBlock = valuesBlock->getBlock(valueBlockIndex))
                    {
                        append<TData>(*valueBlock);
                    }
                }
            }
        }

        template<class TData>
        TData get(int frameIndex) const
        { }
        template<>
        math::vec3 get(int frameIndex) const
        {
            const int componentIndex = frameIndex * 3;
            return math::vec3{
                values[componentIndex],
                values[componentIndex + 1],
                values[componentIndex + 2]
            };
        }
        template<>
        math::quat get(int frameIndex) const
        {
            const int componentIndex = frameIndex * 4;
            return math::quat{
                values[componentIndex],
                values[componentIndex + 1],
                values[componentIndex + 2],
                values[componentIndex + 3]
            };
        }
    };

    struct SkeletalTrackData
    {
        void fromBlk(const DataBlock& blk)
        {
            skeletonAssetPath = blk.getStr("skeleton");
            animationTrackAssetPath = blk.getStr("animation");
        }

        std::string skeletonAssetPath;
        std::string animationTrackAssetPath;
    };

    struct NanimAnimationAssetAccessor::SkeletalTracksData
    {
        void addTrackFromBlk(const DataBlock& blk)
        {
            SkeletalTrackData track;
            track.fromBlk(blk);
            skeletalTracks.emplace_back(std::move(track));
        }
        void fromBlk(const DataBlock& blk)
        {
            if (const auto* jointsBlock = blk.getBlockByName("joints"))
            {
                jointsCount = jointsBlock->getInt("jointsCount");

                if (const auto* bindsBlock = jointsBlock->getBlockByName("bind_matrices"))
                {
                    for (int bmBlockIndex = 0; bmBlockIndex < bindsBlock->blockCount(); ++bmBlockIndex)
                    {
                        if (const auto* bmBlock = bindsBlock->getBlock(bmBlockIndex); bmBlock && !strcmp("bm", bindsBlock->getName(bmBlock->getNameId())))
                        {
                            math::mat4 bm;
                            bm.setCol0(bmBlock->getPoint4("c0"));
                            bm.setCol1(bmBlock->getPoint4("c1"));
                            bm.setCol2(bmBlock->getPoint4("c2"));
                            bm.setCol3(bmBlock->getPoint4("c3"));
                            bindMatrices.emplace_back(bm);
                        }
                    }
                }
            }
        }

        std::vector<SkeletalTrackData> skeletalTracks;
        std::vector<nau::math::mat4> bindMatrices;
        int32_t jointsCount = 0;
    };

    namespace
    {
        AnimationDataDescriptor readDescriptor(const DataBlock& animationBlock)
        {
            if (const auto* descriptorBlock = animationBlock.getBlockByName("descriptor"))
            {
                AnimationDataDescriptor descriptor;

                descriptor.animationIndex = descriptorBlock->getInt("animationIndex");
                descriptor.channelIndex = descriptorBlock->getInt("channelIndex");
                descriptor.dataType = static_cast<AnimationDataDescriptor::DataType>(descriptorBlock->getInt("dataType"));
                descriptor.interpolation = static_cast<AnimationDataDescriptor::InterpolationType>(descriptorBlock->getInt("interpolation"));
                descriptor.name = descriptorBlock->getStr("name");

                return descriptor;
            }

            return { };
        }
    } // namespace

    NanimAnimationAssetAccessor::NanimAnimationAssetAccessor(NanimStreamAssetContainer& container)
    {
        DataBlock animationsBlock;

        container.getStream()->setPosition(io::OffsetOrigin::Begin, 0ull);
        io::GenLoadOverStream genLoad{ container.getStream() };
        [[maybe_unused]] const bool readOk = animationsBlock.loadFromStream(genLoad);
        NAU_ASSERT(readOk);

        fromBlk(animationsBlock);
    }

    NanimAnimationAssetAccessor::NanimAnimationAssetAccessor(const io::FsPath& containerFilePath, size_t animationIndex, size_t channelIndex)
    {
        auto animationsFilePath = containerFilePath.replace_extension("nanim");
        auto& fileSystem = getServiceProvider().get<io::IFileSystem>();

        DataBlock animationsBlock;

        if (fileSystem.exists(animationsFilePath))
        {
            io::IFile::Ptr file = fileSystem.openFile(animationsFilePath, io::AccessMode::Read, io::OpenFileMode::OpenExisting);
            NAU_FATAL(file);

            io::IStreamReader::Ptr stream = file->createStream();
            NAU_FATAL(stream);

            io::GenLoadOverStream genLoad{ stream };
            [[maybe_unused]] const bool readOk = animationsBlock.loadFromStream(genLoad);
            NAU_ASSERT(readOk);
        }

        fromBlk(animationsBlock);
    }

    NanimAnimationAssetAccessor::~NanimAnimationAssetAccessor() = default;

    AnimationDataDescriptor NanimAnimationAssetAccessor::getDataDescriptor() const
    {
        return m_descriptor;
    }

    async::Task<> NanimAnimationAssetAccessor::copyVectors(const AnimationDataDescriptor& desc, TTimesContainer& times, TDataContainer<math::vec3>& data) const
    {
        if (const auto* rawData = m_data.get())
        {
            NAU_ASSERT(rawData->values.size() == 3 * rawData->times.size());

            times = rawData->times;
            data.resize(times.size());

            for (int i = 0; i < times.size(); ++i)
            {
                data[i] = rawData->get<math::vec3>(i);
            }
        }
        return async::Task<>::makeResolved();
    }

    async::Task<> NanimAnimationAssetAccessor::copyRotations(const AnimationDataDescriptor& desc, TTimesContainer& times, TDataContainer<math::quat>& data) const
    {
        if (const auto* rawData = m_data.get())
        {
            NAU_ASSERT(rawData->values.size() == 4 * rawData->times.size());

            times = rawData->times;
            data.resize(times.size());

            for (int i = 0; i < times.size(); ++i)
            {
                data[i] = rawData->get<math::quat>(i);
            }
        }
        return async::Task<>::makeResolved();
    }

    async::Task<> NanimAnimationAssetAccessor::copyFramesData(const AnimationDataDescriptor& desc, DataBlock& data) const
    {
        auto dataFilePath = fmt::format("/content/scenes/animation/{}.blk", desc.name);
        auto& fileSystem = getServiceProvider().get<io::IFileSystem>();

        if (fileSystem.exists(dataFilePath))
        {
            io::IFile::Ptr file = fileSystem.openFile(dataFilePath, io::AccessMode::Read, io::OpenFileMode::OpenExisting);
            NAU_FATAL(file);

            io::IStreamReader::Ptr stream = file->createStream();
            NAU_FATAL(stream);

            io::GenLoadOverStream genLoad{ stream };
            [[maybe_unused]] const bool readOk = data.loadFromStream(genLoad);
            NAU_ASSERT(readOk);
        }

        return async::Task<>::makeResolved();
    }

    void NanimAnimationAssetAccessor::fromBlk(const class DataBlock& blk)
    {
        m_skeletalData = std::make_unique<SkeletalTracksData>();
        m_skeletalData->fromBlk(blk);

        for (int animationBlockIndex = 0; animationBlockIndex < blk.blockCount(); ++animationBlockIndex)
        {
            if (const auto* animationBlock = blk.getBlock(animationBlockIndex); animationBlock && !strcmp("track", blk.getName(animationBlock->getNameId())))
            {
                m_descriptor = readDescriptor(*animationBlock);
                m_data = std::make_unique<KeyFramesData>();
                auto* rawData = m_data.get();

                if (rawData && m_descriptor.dataType != AnimationDataDescriptor::DataType::Unsupported)
                {
                    switch (m_descriptor.dataType)
                    {
                    case AnimationDataDescriptor::DataType::Skeletal:
                        m_skeletalData->addTrackFromBlk(*animationBlock);
                        break;
                    case AnimationDataDescriptor::DataType::Rotation:
                        rawData->fromBlk<math::quat>(*animationBlock);
                        break;
                    default:
                        rawData->fromBlk<math::vec3>(*animationBlock);
                        break;
                    }
                }

                break;
            }
        }
    }

    SkeletonDataDescriptor NanimAnimationAssetAccessor::getDescriptor() const
    {
        SkeletonDataDescriptor descriptor;

        if (const auto* skeletalData = m_skeletalData.get())
        {
            auto& assetManager = getServiceProvider().get<IAssetManager>();

            if (!skeletalData->skeletalTracks.empty())
            {
                AssetPath skeletonAsset{ skeletalData->skeletalTracks.front().skeletonAssetPath };
                auto skeletonFilePath = *assetManager.resolvePath(skeletonAsset);

                descriptor.skeletonPath = skeletonFilePath.getContainerPath();

                AssetPath animationAsset{ skeletalData->skeletalTracks.front().animationTrackAssetPath };
                auto animationFilePath = *assetManager.resolvePath(animationAsset);

                descriptor.animationPath = animationFilePath.getContainerPath();
            }


            descriptor.jointsCount = skeletalData->jointsCount;
        }

        return descriptor;
    }

    void NanimAnimationAssetAccessor::copyInverseBindMatrices(std::vector<math::mat4>& data) const
    {
        data.clear();

        if (const auto* skeletalData = m_skeletalData.get())
        {
            for (const auto& bm : skeletalData->bindMatrices)
            {
                data.push_back(bm);
            }
        }
    }

    nau::Ptr<ISkeletonAssetAccessor> NanimAnimationAssetAccessor::getSkeletonAsset() const
    {
        return const_cast<NanimAnimationAssetAccessor*>(this);
    }

}  // namespace nau
