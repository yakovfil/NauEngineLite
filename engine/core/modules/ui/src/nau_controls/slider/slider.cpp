// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/ui/slider.h"

#include <string>

namespace nau::ui
{
    NauSlider::NauSlider() 
    {
        setAnchorPoint(cocos2d::Vec2::ANCHOR_MIDDLE);
    }

    NauSlider::~NauSlider() {}

    NauSlider* NauSlider::create()
    {
        if (NauSlider* slider = Node::create<NauSlider>())
        {
            return slider;
        }

        return nullptr;
    }

    void NauSlider::setTrackSprite(const std::string& filename)
    {
        if (m_track)
        {
            removeChild(m_track);
        }
        
        if (Sprite* barSprite = Node::create<Sprite>())
        {
            m_track = barSprite;    
            if(m_track->initWithFile(filename))
            {
                addChild(m_track);
                updateTrack();
            }
            else
            {
                NAU_LOG_ERROR("Fail sprite init from from file:{}", filename);
            }
        }
        else
        {
            NAU_LOG_ERROR("Fail set slider bar from file:{}", filename);
        }      
    }

    void NauSlider::setThumbSprite(const std::string& filename)
    {
        if (m_thumb)
        {
            removeChild(m_thumb);
        }
        
        if (Sprite* buttnSprite = Node::create<Sprite>())
        {
            m_thumb = buttnSprite;    
            if(m_thumb->initWithFile(filename))
            {
                addChild(m_thumb);
                updateThumb();
            }
            else
            {
                NAU_LOG_ERROR("Fail sprite init from from file:{}", filename);
            }
        }
        else
        {
            NAU_LOG_ERROR("Fail set slider button from file:{}", filename);
        }
    }

    bool NauSlider::initialize()
    {
        if (!UIControl::initialize()) 
        {
            return false;
        }

        setOnPressedCallback([this](const math::vec2& mousePosition)
        {
            processSliderInput(mousePosition);
        });

        setOnTouchMovedCallback([this](const math::vec2& mousePosition, const math::vec2& delta) 
        {
            processSliderInput(mousePosition);
        });

        return true;
    }

    void NauSlider::processSliderInput(const math::vec2& inputPosition)
    {
        m_currentValue = positionToValue(inputPosition);

        if(m_onValueChanged)
        {
            m_onValueChanged(m_currentValue);
        }

        updateThumb();
    }

    void NauSlider::updateThumb()
    {
        if(!m_thumb || !m_track)
        {
            NAU_LOG_ERROR("Slider bar or thumb is empty!");

            return;
        }

        float buttonX = valueToPosition(m_currentValue);

        m_thumb->setPosition({buttonX, m_track->getContentSize().getY() * 0.5f});
    }

    void NauSlider::updateTrack()
    {
        if(m_track)
        {
            setContentSize(m_track->getContentSize());
            m_track->setPosition(getContentSize() * 0.5f);
        }
    }

    float NauSlider::positionToValue(math::vec2 inputValue)
    {
        if(!m_track)
        {
            NAU_LOG_ERROR("Slider bar is empty!");

            return 0.0f;
        }

        float trackWidth = m_track->getContentSize().getX();
        float roundedInputValue = inputValue.getX();

        if(roundedInputValue < 0.0f)
        {
            roundedInputValue = 0.0f;
        }

        if(roundedInputValue > trackWidth)
        {
            roundedInputValue = trackWidth;
        }

        return roundedInputValue / trackWidth;;
    }
    float NauSlider::valueToPosition(float value)
    {
        if(value > 1.0f || value < 0.0f)
        {
            NAU_LOG_ERROR("Incorrect entry value for slider");
            return 0.0f;
        }

        if(!m_track)
        {
            NAU_LOG_ERROR("Slider bar is empty!");
            return 0.0f;
        }

        float trackWidth = m_track->getContentSize().getX();

        return trackWidth * value;
    }
}