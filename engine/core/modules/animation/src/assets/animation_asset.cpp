// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/animation/assets/animation_asset.h"

#include <ozz/animation/runtime/animation.h>
#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>

#include <string_view>
#include <vector>

#include "nau/animation/playback/animation_skeleton.h"
#include "nau/animation/playback/animation_transforms.h"
#include "nau/assets/animation_asset_accessor.h"
#include "nau/assets/skeleton_asset_accessor.h"
#include "nau/dataBlock/dag_dataBlock.h"
#include "nau/io/file_system.h"
#include "nau/io/virtual_file_system.h"
#include "nau/math/math.h"
#include "nau/math/transform.h"
#include "nau/service/service_provider.h"

namespace nau::animation::data
{
    namespace
    {
        template <class TEditor>
        async::Task<> insertFramesData(TEditor& animationEditor, IAnimationAssetAccessor& animationAccessor)
        {
            const auto& animDescriptor = animationAccessor.getDataDescriptor();
            DataBlock framesDataBlock;
            co_await animationAccessor.copyFramesData(animDescriptor, framesDataBlock);
            animationEditor.deserialize(framesDataBlock);
        }
    }  // namespace

    async::Task<nau::Ptr<AnimationAssetView>> AnimationAssetView::createFromAssetAccessor(nau::Ptr<> accessor)
    {
        using namespace nau::async;

        NAU_ASSERT(accessor);

        //ASYNC_SWITCH_EXECUTOR(async::Executor::getDefault());

        auto& animationAccessor = accessor->as<IAnimationAssetAccessor&>();
        auto instancePtr = rtti::createInstance<AnimationAssetView>();
        AnimationAssetView* instance = instancePtr.get();

        const auto& animDescriptor = animationAccessor.getDataDescriptor();
        constexpr float frameRate = 60.f;  // todo: set time directly and deal with frames inside the anim engine

        switch (animDescriptor.dataType)
        {
            case AnimationDataDescriptor::DataType::Skeletal:
            {
                auto trackFilePath = animationAccessor.getSkeletonAsset()->getDescriptor().animationPath;
                co_return createFromOzzPath(trackFilePath);
            }
            break;
            case AnimationDataDescriptor::DataType::Translation:
            {
                std::vector<float> frameTimes;
                std::vector<math::vec3> translations;
                co_await animationAccessor.copyVectors(animDescriptor, frameTimes, translations);
                auto animation = rtti::createInstance<animation::TranslationAnimation>();

                if (auto animationEditor = animation->createEditor())
                {
                    NAU_ASSERT(frameTimes.size() == translations.size());

                    for (int i = 0; i < frameTimes.size(); ++i)
                    {
                        animationEditor.addKeyFrame((int)(frameRate * frameTimes[i]), translations[i]);
                    }

                    co_await insertFramesData(animationEditor, animationAccessor);
                }

                instance->m_animation = animation;
            }
            break;
            case AnimationDataDescriptor::DataType::Rotation:
            {
                std::vector<float> frameTimes;
                std::vector<math::quat> rotations;
                co_await animationAccessor.copyRotations(animDescriptor, frameTimes, rotations);
                auto animation = rtti::createInstance<animation::RotationAnimation>();

                if (auto animationEditor = animation->createEditor())
                {
                    NAU_ASSERT(frameTimes.size() == rotations.size());

                    for (int i = 0; i < frameTimes.size(); ++i)
                    {
                        animationEditor.addKeyFrame((int)(frameRate * frameTimes[i]), rotations[i]);
                    }

                    co_await insertFramesData(animationEditor, animationAccessor);
                }

                instance->m_animation = animation;
            }
            break;
            case AnimationDataDescriptor::DataType::Scale:
            {
                std::vector<float> frameTimes;
                std::vector<math::vec3> scales;
                co_await animationAccessor.copyVectors(animDescriptor, frameTimes, scales);
                auto animation = rtti::createInstance<animation::ScaleAnimation>();

                if (auto animationEditor = animation->createEditor())
                {
                    NAU_ASSERT(frameTimes.size() == scales.size());

                    for (int i = 0; i < frameTimes.size(); ++i)
                    {
                        animationEditor.addKeyFrame((int)(frameRate * frameTimes[i]), scales[i]);
                    }

                    co_await insertFramesData(animationEditor, animationAccessor);
                }

                instance->m_animation = animation;
            }
            break;
            case AnimationDataDescriptor::DataType::Unsupported:
                NAU_ASSERT(false);
                break;
        }

        if (instance->m_animation)
        {
            switch (animDescriptor.interpolation)
            {
                case AnimationDataDescriptor::InterpolationType::No:
                    instance->m_playbackData.interpolationMethod = animation::AnimationInterpolationMethod::Step;
                    break;
                case AnimationDataDescriptor::InterpolationType::Linear:
                    instance->m_playbackData.interpolationMethod = animation::AnimationInterpolationMethod::Linear;
                    break;
            }
        }

        co_return instancePtr;
    }

    nau::Ptr<AnimationAssetView> AnimationAssetView::createFromOzzPath(std::string_view path)
    {
        auto& nauFileSystem = getServiceProvider().get<io::IFileSystem>();

        io::IFile::Ptr file = nauFileSystem.openFile(path, io::AccessMode::Read, io::OpenFileMode::OpenExisting);

        if (!file || !file->isOpened())
        {
            NAU_ASSERT(false, "failed to open .ozz animation file: {}", path);
            return nau::Ptr{};
        }

        nau::io::IStreamReader::Ptr nauFileStreamRead = static_cast<nau::io::IStreamReader::Ptr>(file->createStream(io::AccessMode::Read));

        const size_t fileSize = file->getSize();

        std::vector<std::byte> fileData(fileSize);
        Result<size_t> readResult = nauFileStreamRead->read(fileData.data(), fileSize);
        if (!readResult)
        {
            NAU_ASSERT(false);
        }

        ozz::io::MemoryStream memStream;  // extra copy of data since there is no required API
        memStream.Write(fileData.data(), fileData.size());
        memStream.Seek(0, ozz::io::Stream::Origin::kSet);

        ozz::io::IArchive animArchive(&memStream);

        if (!animArchive.TestTag<ozz::animation::Animation>())
        {
            NAU_ASSERT(false, "Archive doesn't contain the expected object type.");
            return nau::Ptr{};
        }

        auto instancePtr = rtti::createInstance<AnimationAssetView>();

        auto animation = rtti::createInstance<animation::SkeletalAnimation>();

        animArchive >> animation->ozzAnimation;  // we don't store in Asset, store directly in Anim

        instancePtr->m_playbackData.interpolationMethod = AnimationInterpolationMethod::Linear;

        instancePtr->m_animation = animation;

        return instancePtr;
    }

    nau::Ptr<Animation> AnimationAssetView::getAnimation()
    {
        return m_animation;
    }

    const AnimationPlaybackData& AnimationAssetView::getPlaybackData() const
    {
        return m_playbackData;
    }

}  // namespace nau::animation::data
