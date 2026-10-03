// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "usd_animation_clip_composer.h"

#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/usd/usd/attribute.h>

#include <string>
#include <vector>

#include "nau/NauAnimationClipAsset/nauAnimationClip.h"
#include "nau/NauAnimationClipAsset/nauAnimationTrack.h"

namespace UsdTranslator
{
    AnimationClipComposer::AnimationClipComposer(const pxr::UsdPrim& prim)
        : m_playMode(nau::animation::PlayMode::Once)
    {
        const pxr::UsdNauAnimationClip clip{ prim };
        if (!clip) {
            return;
        }
        pxr::TfToken modeToken;
        clip.GetPlayModeAttr().Get(&modeToken);
        const std::string& mode = modeToken.GetString();
        if (mode == "Once")
        {
            m_playMode = nau::animation::PlayMode::Once;
        } else if (mode == "Looping")
        {
            m_playMode = nau::animation::PlayMode::Looping;
        } else if (mode == "PingPong")
        {
            m_playMode = nau::animation::PlayMode::PingPong;
        }
        for (auto&& trackPrim : prim.GetAllChildren())
        {
            const pxr::UsdNauAnimationTrack track{ trackPrim };

            pxr::TfToken dataTypeToken;
            track.GetDataTypeAttr().Get(&dataTypeToken);
            const std::string& dataTypeStr = dataTypeToken.GetString();

            nau::AnimationDataDescriptor& descriptor = m_descriptors.emplace_back();
            descriptor.name = prim.GetName().GetString().c_str();

            if (dataTypeStr == "Translation")
            {
                descriptor.dataType = nau::AnimationDataDescriptor::DataType::Translation;
            } else if (dataTypeStr == "Rotation")
            {
                descriptor.dataType = nau::AnimationDataDescriptor::DataType::Rotation;
            } else if (dataTypeStr == "Scale")
            {
                descriptor.dataType = nau::AnimationDataDescriptor::DataType::Scale;
            } else
            {
                m_descriptors.pop_back();
                continue;
            }

            descriptor.animationIndex = m_descriptors.size() - 1u;
            descriptor.channelIndex = 0;
            loadData(descriptor, trackPrim);

            pxr::TfToken interpolationToken;
            track.GetInterpolationAttr().Get(&interpolationToken);
            const std::string& interpolationStr = interpolationToken.GetString();
            if (interpolationStr == "No")
            {
                descriptor.interpolation = nau::AnimationDataDescriptor::InterpolationType::No;
            } else if (interpolationStr == "Linear")
            {
                descriptor.interpolation = nau::AnimationDataDescriptor::InterpolationType::Linear;
            }
        }
    }

    const std::vector<nau::AnimationDataDescriptor>& AnimationClipComposer::getAnimationDataDescriptors() const noexcept
    {
        return m_descriptors;
    }

    nau::animation::PlayMode AnimationClipComposer::getPlayMode() const noexcept
    {
        return m_playMode;
    }

    void AnimationClipComposer::loadData(const nau::AnimationDataDescriptor& descriptor, const pxr::UsdPrim& trackPrim)
    {
        TransformTrackPair& trackData = m_trackList.emplace_back();
        if (descriptor.dataType == nau::AnimationDataDescriptor::DataType::Unsupported)
        {
            return;
        }
        const pxr::TfToken keyframeToken{ "keyframes" };
        const auto&& keyframes = trackPrim.GetAttribute(keyframeToken);
        if (!keyframes)
        {
            return;
        }
        // so far, only the float3 type is supported
        const auto&& typeName = keyframes.GetTypeName();
        if (typeName != pxr::SdfValueTypeNames->Float3)
        {
            return;
        }

        std::vector<double> timeSamples;
        keyframes.GetTimeSamples(&timeSamples);

        trackData.first.reserve(timeSamples.size());
        trackData.second.reserve(timeSamples.size());
        
        for (double time : timeSamples)
        {
            trackData.first.emplace_back(static_cast<float>(time));
            pxr::GfVec3f vec;
            keyframes.Get(&vec, time);
            trackData.second.emplace_back(vec[0], vec[1], vec[2]);
        }
    }

    std::vector<nau::math::quat> AnimationClipComposer::convertToQuatVec(size_t animationIndex) const
    {
        const std::vector<nau::math::vec3>& data = m_trackList[animationIndex].second;

        std::vector<nau::math::quat> quats;
        quats.reserve(data.size());

        for (const auto& vec3f : data)
        {
            const float xAngle = nau::math::degToRad(vec3f.getX());
            const float yAngle = nau::math::degToRad(vec3f.getY());
            const float zAngle = nau::math::degToRad(vec3f.getZ());
            quats.emplace_back(nau::math::mat3::rotationZYX(nau::math::vec3(xAngle, yAngle, zAngle)));
        }
        return quats;
    }
}