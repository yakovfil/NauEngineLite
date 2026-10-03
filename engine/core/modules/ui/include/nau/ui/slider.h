// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string>

#include "nau/ui/elements/sprite.h"
#include "nau/ui/ui_control.h"

namespace nau::ui
{
    class NAU_UI_EXPORT NauSlider : public UIControl
    {
    public:
        using OnValueChangedCallback = std::function<void(float value)>;

        NauSlider();
        virtual ~NauSlider();

        static NauSlider* create();

        FORCEINLINE void setOnValueChangedCallback(OnValueChangedCallback cb) { m_onValueChanged = cb; };

        void setTrackSprite(const std::string& filename);
        void setThumbSprite(const std::string& filename);

    protected:
        OnValueChangedCallback m_onValueChanged {nullptr};

        virtual bool initialize() override;

    private:
        Sprite* m_track {nullptr};
        Sprite* m_thumb {nullptr};

        float m_currentValue {1.0f};

        void updateThumb();
        void updateTrack();
        void processSliderInput(const math::vec2& inputPosition);
        float positionToValue(math::vec2 inputValue);
        float valueToPosition(float value);
    };
}