// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string_view>

#include "nau/animation/playback/animation.h"
#include "nau/assets/asset_view.h"
#include "nau/async/task.h"
#include "nau/rtti/rtti_impl.h"

namespace nau::animation::data
{
    /**
     * @brief Encapsulates animation playback parameters.
     */
    struct AnimationPlaybackData
    {
        AnimationInterpolationMethod interpolationMethod;
    };

    /**
     * @brief Provides functionality to extract animation and playback data from an animation asset.
     */
    class NAU_ANIMATION_EXPORT AnimationAssetView : public IAssetView
    {
        NAU_CLASS_(nau::animation::data::AnimationAssetView, IAssetView)

    public:

        /**
         * @brief Creates a view from an animation asset accessor.
         * 
         * @param [in] accessor A pointer to IAnimationAssetAccessor interface.
         * @return              Task object providing operation status as well as access to the created view.
         */
        static nau::async::Task<nau::Ptr<AnimationAssetView>> createFromAssetAccessor(nau::Ptr<> accessor);

        /**
         * @brief Creates view from an ozz skeletal animation asset.
         * 
         * @param [in] path Path to ozz file.
         * @return          Task object providing operation status as well as access to the created view.
         */
        static nau::Ptr<AnimationAssetView> createFromOzzPath(std::string_view path);

        /**
         * @brief Retrieves animation object extracted from the asset.
         * 
         * @return A pointer to the animation object.
         */
        nau::Ptr<Animation> getAnimation();

        /**
         * @brief Reteieves animation playback data extracted from the asset.
         * 
         * @return Animation playback data.
         */
        const AnimationPlaybackData& getPlaybackData() const;

    private:
        nau::Ptr<Animation> m_animation;
        AnimationPlaybackData m_playbackData;
    };

}  // namespace nau::animation::data
