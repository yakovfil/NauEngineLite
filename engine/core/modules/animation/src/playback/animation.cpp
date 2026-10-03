// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/animation/playback/animation.h"

#include <algorithm>
#include <span>
#include <string>

#include "nau/animation/playback/animation_player_keyframe.h"

namespace nau::animation
{
    float AnimationState::getFullWeight() const
    {
        return weight * blendInOutWeight;
    }

    bool AnimationState::addEvent(const std::string& id, bool pin)
    {
        for (auto& event : events)
        {
            if (!event.flags.has(FrameEventControl::IsActive))
            {
                event.flags.set(FrameEventControl::IsActive);
                event.id = id;
                if (pin)
                {
                    event.flags.set(FrameEventControl::IsPinned);
                }
                return true;
            }
        }

        NAU_ASSERT(false, "Out of frame events slots");

        return false;
    }

    bool AnimationState::removeEvent(const std::string& id)
    {
        for (auto& event : events)
        {
            if (event.flags.has(FrameEventControl::IsActive) && event.id == id)
            {
                event.flags.clear();
                return true;
            }
        }

        return false;
    }

    void AnimationState::clearEvents()
    {
        for (auto& event : events)
        {
            event.flags.clear();
        }
    }

    IAnimationPlayer::Ptr Animation::createPlayer(AnimationInstance& instance) const
    {
        return rtti::createInstance<KeyFrameAnimationPlayer>(instance);
    }

    const std::span<const FrameEvent> Animation::getEvents(int frame) const
    {
        auto it = std::lower_bound(m_perFrameData.begin(), m_perFrameData.end(), Frame{frame},
                                   [](const auto& a, const auto& b)
        {
            return a.frame < b.frame;
        });

        if (it != m_perFrameData.end() && it->frame == frame)
        {
            return std::span(it->events);
        }

        return std::span<FrameEvent>();
    }

    void Animation::sortFrames()
    {
        std::sort(m_perFrameData.begin(), m_perFrameData.end(), [](const auto& a, const auto& b)
            {
                return a.frame < b.frame;
            });
    }

    Frame& Animation::getOrCreateFrameData(int frame)
    {
        if (auto* frameData = getFrameData(frame))
        {
            return *frameData;
        }

        return m_perFrameData.emplace_back(Frame{ .frame = frame });
    }

    Frame* Animation::getFrameData(int frame)
    {
        for (auto it = m_perFrameData.rbegin(); it != m_perFrameData.rend(); ++it)
        {
            if (it->frame == frame)
            {
                return &*it;
            }
        }

        return nullptr;
    }

    const Frame* Animation::getFrameData(int frame) const
    {
        for (auto it = m_perFrameData.rbegin(); it != m_perFrameData.rend(); ++it)
        {
            if (it->frame == frame)
            {
                return &*it;
            }
        }

        return nullptr;
    }

    void Animation::toBlk(DataBlock& blk) const
    {
        for (const auto& frameData : m_perFrameData)
        {
            if (auto* frameBlock = blk.addNewBlock("frame"))
            {
                frameBlock->addInt("i", frameData.frame);

                for (const auto& event : frameData.events)
                {
                    if (auto* eventBlock = frameBlock->addNewBlock("event"))
                    {
                        eventBlock->addStr("id", event.getId().c_str());
                        eventBlock->addInt("type", static_cast<int>(event.getEventType()));
                        eventBlock->addInt("dir", static_cast<int>(event.getActivationDirection()));
                    }
                }
            }
        }
    }

    void Animation::fromBlk(DataBlock& blk)
    {
        m_perFrameData.clear();

        for (int frameBlockIndex = 0; frameBlockIndex < blk.blockCount(); ++frameBlockIndex)
        {
            if (auto* frameBlock = blk.getBlock(frameBlockIndex); frameBlock && !strcmp("frame", frameBlock->getName(frameBlock->getNameId())))
            {
                int frame = frameBlock->getInt("i");

                Frame newFrameData{ frame };

                for (int eventBlockIndex = 0; eventBlockIndex < frameBlock->blockCount(); ++eventBlockIndex)
                {
                    if (auto* eventBlock = frameBlock->getBlock(eventBlockIndex); eventBlock && !strcmp("event", eventBlock->getName(eventBlock->getNameId())))
                    {
                        FrameEvent newEvent;
                        newEvent.setId(eventBlock->getStr("id"));
                        newEvent.setEventType(static_cast<FrameEventType>(eventBlock->getInt("type")));
                        newEvent.setActivationDirection(static_cast<FrameEventActivationDirection>(eventBlock->getInt("dir")));
                        newFrameData.events.push_back(newEvent);
                    }
                }

                m_perFrameData.emplace_back(newFrameData);
            }
        }
    }

 }  // namespace nau::animation
