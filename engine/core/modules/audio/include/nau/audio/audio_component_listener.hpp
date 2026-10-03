// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <vector>

#include "nau/audio/audio_common.hpp"
#include "nau/audio/audio_component_emitter.hpp"
#include "nau/scene/components/component_attributes.h"
#include "nau/scene/components/internal/component_internal_attributes.h"
#include "nau/scene/scene.h"

NAU_AUDIO_BEGIN

struct NAU_AUDIO_EXPORT AudioComponentListener : public scene::SceneComponent
{
    NAU_OBJECT(AudioComponentListener, scene::SceneComponent)


    NAU_CLASS_ATTRIBUTES(
        CLASS_ATTRIBUTE(scene::SystemComponentAttrib, true),
        CLASS_ATTRIBUTE(scene::ComponentDisplayNameAttrib, "Audio Listener"),
        CLASS_ATTRIBUTE(scene::ComponentDescriptionAttrib, "Audio Listener (description)"))

    NAU_CLASS_FIELDS(
        CLASS_FIELD(emitters))

    // Properties
    std::vector<AudioComponentEmitter> emitters;
};

NAU_AUDIO_END
