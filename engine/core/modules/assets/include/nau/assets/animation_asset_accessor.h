// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <optional>
#include <span>
#include <string>
#include <vector>

#include "nau/assets/asset_accessor.h"
#include "nau/async/task.h"
#include "nau/dataBlock/dag_dataBlock.h"
#include "nau/math/math.h"

namespace nau
{
    /**
     * @brief Describes animation data.
     */
    struct AnimationDataDescriptor
    {
        /**
         * @brief Enumerates animated parameter types that are supported by animation assets.
         */
        enum class DataType
        {
            Unsupported,
            Translation,
            Rotation, 
            Scale,
            Skeletal
        };

        /**
         * @brief Enumerates methods to interpolate animated values between keyframes.
         */
        enum class InterpolationType
        {
            No,
            Linear
        };

        size_t animationIndex = 0;
        size_t channelIndex = 0;
        DataType dataType = DataType::Unsupported;
        InterpolationType interpolation = InterpolationType::No;
        std::string name;

        bool operator==(const AnimationDataDescriptor& other) const
        {
            return animationIndex == other.animationIndex && channelIndex == other.channelIndex && dataType == other.dataType;
        }
        bool operator!=(const AnimationDataDescriptor& other) const
        {
            return !(*this == other);
        }
    };

    /**
     * @brief Encapsulates frame event data as present in animation asset.
     * 
     * See FrameEvent.
     */
    struct FrameEventData
    {
        /**
         * @brief Event string identifier.
         */
        std::string name;

        /**
         * @brief Event activation type.
         * 
         * See FrameEventType.
         */
        int eventType;

        /**
         * @brief Event activation direction.
         * 
         * See FrameEventActivationDirection.
         */
        int activationDirection;
    };

    /**
     * @brief Encapsulates a collection of events attached to the frame as present in animation asset.
     * 
     * See Frame.
     */
    struct FrameData
    {
        int frame;
        std::vector<FrameEventData> events;
    };

    /**
     * @brief Provides access the data from an animation asset.
     */
    struct NAU_ABSTRACT_TYPE IAnimationAssetAccessor : IAssetAccessor
    {
        NAU_INTERFACE(nau::IAnimationAssetAccessor, IAssetAccessor)
        
        /**
         * @brief Retrieves animation description from the asset.
         * 
         * @return Animation descriptor.
         */
        virtual AnimationDataDescriptor getDataDescriptor() const = 0;

        /**
         * @brief A type of keframes timestamps collection. 
         */
        using TTimesContainer = std::vector<float>;

        /**
         * @brief A type of per-keyframe data extracted from the asset.
         */
        template <typename TData>
        using TDataContainer = std::vector<TData>;

        /** 
         * @brief Retrieves vector frame data from the asset.
         * 
         * @param [in]  desc    Animation asset description.
         * @param [out] times   A collection of extracted keyframes timesteps. Each element is a duration (in seconds) from the animation start until the moment when playback reaches the corresponding keyframe.
         * @param [out] data    A collection of extracted vector per-keframe data.
         * @return              Task object providing operation status.
         */
        virtual async::Task<> copyVectors(const AnimationDataDescriptor& desc, TTimesContainer& times, TDataContainer<math::vec3>& data) const = 0;

        /** 
         * @brief Retrieves rotation frame data from the asset.
         * 
         * @param [in]  desc    Animation asset description.
         * @param [out] times   A collection of extracted keyframes timesteps. Each element is a duration (in seconds) from the animation start until the moment when playback reaches the corresponding keyframe.
         * @param [out] data    A collection of extracted rotation per-keframe data.
         * @return              Task object providing operation status.
         */
        virtual async::Task<> copyRotations(const AnimationDataDescriptor& desc, TTimesContainer& times, TDataContainer<math::quat>& data) const = 0;

        /**
         * @brief Retrieves additional frame data from the asset.
         *
         * @param [in]  desc    Animation asset description.
         * @param [out] data    Data block to transfer the extracted data to.
         * @return              Task object providing operation status.
         * 
         * This function is used to extract frame events data from the asset.
         */
        virtual async::Task<> copyFramesData(const AnimationDataDescriptor& desc, DataBlock& data) const = 0;

        virtual nau::Ptr<struct ISkeletonAssetAccessor> getSkeletonAsset() const = 0;
    };

}  // namespace nau
