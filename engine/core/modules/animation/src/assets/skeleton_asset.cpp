// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/animation/assets/skeleton_asset.h"

#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>
#include <ozz/base/maths/simd_math.h>

#include <vector>

#include "nau/assets/animation_asset_accessor.h"
#include "nau/assets/skeleton_asset_accessor.h"
#include "nau/io/file_system.h"
#include "nau/io/virtual_file_system.h"
#include "nau/service/service_provider.h"
#include "nau/shaders/shader_defines.h"

namespace nau
{
    async::Task<nau::Ptr<SkeletonAssetView>> SkeletonAssetView::createFromAssetAccessor(nau::Ptr<> accessor)
    {
        using namespace nau::async;

        NAU_ASSERT(accessor);

        // TODO: Fix this. Does not return control to the thread
        //ASYNC_SWITCH_EXECUTOR(async::Executor::getDefault());

        nau::Ptr<> targetAccessor = accessor;

        if (auto* animationAccessor = accessor->as<IAnimationAssetAccessor*>())
        {
            targetAccessor = animationAccessor->getSkeletonAsset();
        }

        auto& skeletonAccessor = targetAccessor->as<ISkeletonAssetAccessor&>();
        auto skeletonAssetView = rtti::createInstance<SkeletonAssetView>();

        const auto skeletonDesc = skeletonAccessor.getDescriptor();
        const unsigned jointsCount = skeletonDesc.jointsCount;

        skeletonAssetView->inverseBindTransforms.reserve(jointsCount);

        skeletonAccessor.copyInverseBindMatrices(skeletonAssetView->inverseBindTransforms);

        auto& nauFileSystem = getServiceProvider().get<io::IFileSystem>();

        io::IFile::Ptr file = nauFileSystem.openFile(skeletonDesc.skeletonPath, io::AccessMode::Read, io::OpenFileMode::OpenExisting);

        if (!file || !file->isOpened())
        {
            NAU_ASSERT(false, "failed to open .ozz skeleton file: {}", skeletonDesc.skeletonPath);
            co_return nau::Ptr{};
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

        ozz::io::IArchive archive(&memStream);

        if (!archive.TestTag<ozz::animation::Skeleton>())
        {
            NAU_ASSERT(false, "Archive doesn't contain the expected object type.");
            co_return nau::Ptr{};
        }
        archive >> skeletonAssetView->skeleton;

        //NAU_ASSERT(skeletonAssetView->skeleton.num_joints() == skeletonAssetView->getInverseBindTransforms().size());
        NAU_ASSERT(skeletonAssetView->skeleton.num_joints() <= NAU_MAX_SKINNING_BONES_COUNT);

        skeletonAssetView->joints.resize(jointsCount);
        const auto& jointNames = skeletonAssetView->skeleton.joint_names();
        const auto& jointParents = skeletonAssetView->skeleton.joint_parents();
        for (size_t i = 0; i < jointsCount; ++i)
        {
            skeletonAssetView->joints[i].jointName = jointNames[i];
            skeletonAssetView->joints[i].parentIndex = jointParents[i];
        }

        // extract default pose
        ozz::span<const ozz::math::SoaTransform> restPosesSoa = skeletonAssetView->skeleton.joint_rest_poses();

        ozz::vector<ozz::math::Float4x4> models(jointsCount);

        ozz::animation::LocalToModelJob job;
        job.input = restPosesSoa;
        job.output = ozz::make_span(models);
        job.skeleton = &skeletonAssetView->skeleton;
        if (!job.Run()) {
            NAU_ASSERT(false);
        }

        skeletonAssetView->defaultPoseTransforms.resize(jointsCount);
        std::memcpy(skeletonAssetView->defaultPoseTransforms.data(), models.data(), 64 * jointsCount); // 64 == 16 elements * 4 (sizeof(float))

        co_return skeletonAssetView;
    }

    const std::vector<nau::math::Matrix4>& SkeletonAssetView::getInverseBindTransforms() const
    {
        return inverseBindTransforms;
    }

    const std::vector<nau::math::Matrix4>& SkeletonAssetView::getDefaultPoseTransforms() const
    {
        return defaultPoseTransforms;
    }

    const ozz::animation::Skeleton& SkeletonAssetView::getSkeleton() const
    {
        return skeleton;
    }

    const std::vector<SkeletonJoint>& SkeletonAssetView::getJoints() const
    {
        return joints;
    }
}  // namespace nau
