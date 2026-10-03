// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string>

#include "nau/messaging/messaging.h"
#include "nau/meta/class_info.h"

namespace nau::animation::events
{
    static constexpr char ANIMATION_FRAME_EVENTS_STREAM_NAME[] = "animfe";

    struct FrameEventData
    {
        NAU_TYPEID(nau::animation::events::FrameEventData)

        NAU_CLASS_FIELDS(
            CLASS_FIELD(trackName),
            CLASS_FIELD(eventId)
        )

        std::string trackName;
        std::string eventId;
    };

    NAU_DECLARE_MESSAGE(AnimTrackPlaybackEvent, ANIMATION_FRAME_EVENTS_STREAM_NAME, FrameEventData);

    static constexpr char ANIMATION_EVENT_TRACK_STARTED[] = "track.started";
    static constexpr char ANIMATION_EVENT_TRACK_FINISHED[] = "track.finished";
}