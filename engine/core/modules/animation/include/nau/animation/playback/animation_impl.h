// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <algorithm>
#include <string_view>
#include <vector>

#include "nau/animation/edit/animation_editor.h"
#include "nau/animation/playback/animation.h"
#include "nau/dataBlock/dag_dataBlock.h"

namespace nau::animation
{
 
    template<class TValue>
    class NAU_ABSTRACT_TYPE AnimationImpl : public Animation, private IAnimationEditor<TValue>
    {
        NAU_CLASS(nau::animation::AnimationImpl<TValue>, rtti::RCPolicy::StrictSingleThread, Animation)   

    public:
        using TKeyFrame = KeyFrameImpl<TValue>;
        using TImpl = AnimationImpl<TValue>;

        virtual int getLastFrame() const override
        {
            return !m_keyFrames.empty() ? m_keyFrames.back().getFrame() : 0;
        }

        virtual float getDurationInFrames() const override
        {
            return getLastFrame() + 1;
        }

        AutoAnimationEditor<TValue> createEditor()
        {
            return AutoAnimationEditor<TValue> { this };
        }
        IAnimationEditor<TValue>& asInplaceEditor()
        {
            return *this;
        }

        const TKeyFrame* getKeyFrameAt(int index) const
        {
            return 0 <= index && index < m_keyFrames.size() ? &m_keyFrames[index] : nullptr;
        }
        int getNumKeyFrames() const
        {
            return m_keyFrames.size();
        }

    private:
        // key frames
        virtual int getKeyFrameCount() const override
        {
            return getNumKeyFrames();
        }
        virtual TKeyFrame getKeyFrameAtIndex(int index) const override
        {
            return *getKeyFrameAt(index);
        }
        virtual void clearKeyFrames() override
        {
            m_keyFrames.clear();
        }

        virtual void addKeyFrame(int frame, const TValue& value) override
        {
            auto newTempKeyFrame = TKeyFrame(frame, value);
            addKeyFrame(&newTempKeyFrame);
        }
        virtual bool deleteKeyFrame(int frame) override
        {
            if (auto eraseIt = std::remove_if(m_keyFrames.begin(), m_keyFrames.end(), [frame](const auto& kf)
            {
                return kf.getFrame() == frame;
            });
                eraseIt != m_keyFrames.end())
            {
                m_keyFrames.erase(eraseIt);
                return true;
            }

            return false;
        }

        // regular frames
        virtual int getFrameDataCount() const override
        {
            return m_perFrameData.size();
        }
        virtual Frame getFrameDataAtIndex(int index) const override
        {
            return m_perFrameData[index];
        }
        virtual void clearFrameData() override
        {
            m_perFrameData.clear();
        }
        virtual void addFrameData(const Frame& frameData) override
        {
            m_perFrameData.push_back(frameData);
        }

        // events
        virtual void addFrameEvent(int frame, const FrameEvent& value) override
        {
            auto& frameData = getOrCreateFrameData(frame);
            frameData.events.push_back(value);
        }
        virtual bool deleteFrameEvent(int frame, const char* eventId) override
        {
            if (auto* frameData = getFrameData(frame))
            {
                frameData->events.erase(std::remove_if(frameData->events.begin(), frameData->events.end(),
                                                       [eventId](const auto& event)
                {
                    return strcmp(event.getId().c_str(), eventId) == 0;
                }));

                return true;
            }

            return false;
        }
        virtual int getEventCount(int frame) const override
        {
            if (const auto* frameData = getFrameData(frame))
            {
                return frameData->events.size();
            }

            return 0;
        }
        virtual std::string_view getEventId(int frame, int index) const override
        {
            if (const auto* frameData = getFrameData(frame))
            {
                return frameData->events[index].getId();
            }

            return std::string_view();
        }

        virtual void commit() override
        {
            sortFrames();
        }

        void serialize(DataBlock& blk) const override
        {
            Animation::toBlk(blk);
        }

        void deserialize(DataBlock& blk) override
        {
            Animation::fromBlk(blk);
        }

    protected:
        virtual void addKeyFrame(const KeyFrame* keyFrame) override
        {
            if (keyFrame)
            {
                if (auto* frame = keyFrame->as<const TKeyFrame*>())
                {
                    m_keyFrames.push_back(*frame);
                }
            }
        }

    private:
        void sortFrames()
        {
            std::sort(m_keyFrames.begin(), m_keyFrames.end(), [](const auto& a, const auto& b)
                {
                    return a.getFrame() < b.getFrame();
                });

            Animation::sortFrames();
        }
        
    private:
        std::vector<TKeyFrame> m_keyFrames;
    };

}  // namespace nau::animation
