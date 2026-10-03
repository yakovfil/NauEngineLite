// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <map>
#include <span>
#include <string_view>
#include <vector>

#include "nau/animation/data/frame.h"
#include "nau/animation/data/keyframe.h"
#include "nau/dataBlock/dag_dataBlock.h"

namespace nau::animation
{
    /**
     * @brief A utlity class that allows to accumulate changes to the managed animation object and then apply(commit) them all at once.
     * 
     * @tparam TKeyFrameValueType Type of the animated parameter.
     * 
     * @note    All modifying functions of this class actually accumulate changes and do not apply them to the underlying animation object.
     *          In order to apply the changes call @ref commit method.
     * @note    All `get...` methods retrieve values taking all local uncommited changes in the editor.
     *          If the actual state of the managed object is required, use the corresponding API.
     */
    template<typename TKeyFrameValueType>
    struct IAnimationEditor
    {
        /**
         * @brief Destructor.
         */
        virtual ~IAnimationEditor() = default;

        // key frames

        /**
         * @brief Retrieves the number of animation keyframes.
         * 
         * @return Number of key frames.
         */
        virtual int getKeyFrameCount() const = 0;

        /**
         * @brief Retrieves a keyframe by its index.
         * 
         * @param [in] index    Index of the keyframe among other keyframes.
         * @return              Retrieved keyframe.
         */
        virtual KeyFrameImpl<TKeyFrameValueType> getKeyFrameAtIndex(int index) const = 0;


        /**
         * @brief Removes all keyframes.
         */
        virtual void clearKeyFrames() = 0;

        /**
         * @brief Adds the keyframe to the animation.
         * 
         * @param [in] frame Index of the keyframe (among ordinary frames).
         */
        virtual void addKeyFrame(int frame, const TKeyFrameValueType& value) = 0;

        /**
         * @brief Removes the keyframe from the animation.
         * 
         * @param [in] frame Index of the keyframe (among ordinary frames).
         */
        virtual bool deleteKeyFrame(int frame) = 0;

        // regular frames

        /**
         * @brief Retrieves the number of registered frame data entries.
         * 
         * @return Number of frame data entries.
         * 
         * Currently frame data encapsulates a collection of events that are triggered upon playback reaching the frame.
         * See Frame.
         */
        virtual int getFrameDataCount() const = 0;

        /**
         * @brief Retrieves a frame data entry that is assoicated with the frame.
         * 
         * @param [in] index    Index of the frame.
         * @return              Data entry associated with the frame.
         */
        virtual Frame getFrameDataAtIndex(int index) const = 0;


        /**
         * @brief Removes all frame data entries.
         */
        virtual void clearFrameData() = 0;

        /**
         * @brief Adds the frame entry to the animation.
         * 
         * @param [in] frameData    Frame data entry to add.
         */
        virtual void addFrameData(const Frame& frameData) = 0;

        // events

        /**
         * @brief Attaches the frame event to the frame.
         * 
         * @param [in] frame Index of the frame to attach the event to.
         * @param [in] value Event to attach.
         * 
         * @note Maximal number of events attached to a frame is limited by MAX_EVENTS_PER_FRAME.
         */
        virtual void addFrameEvent(int frame, const FrameEvent& value) = 0;

        /**
         * @brief Detaches the frame event from the frame.
         * 
         * @param [in] frame    Index of the frame to detach the event from.
         * @param [in] eventid  Identifier of the event to detach.
         */
        virtual bool deleteFrameEvent(int frame, const char* eventId) = 0;

        /**
         * @brief Retrieves the number of events attached to the frame.
         * 
         * @param [in] frame    Index of the frame.
         * @return              Number of events attached to the frame. 
         */
        virtual int getEventCount(int frame) const = 0;

        /**
         * @brief Retrieves the identifier of the frame event.
         * 
         * @param [in] frame    Index of the frame the event is attached to.
         * @param [in] index    Index of the event in the event collection associated with the frame.
         * @return              Event string identifier.
         */
        virtual std::string_view getEventId(int frame, int index) const = 0;

        /**
         * @brief Deliver changes to the managed animation.
         */
        virtual void commit() = 0;


        /**
         * @brief Serializes keyframes (event) data into the blk object.
         *
         * @param [out] Blk object to write serialized data into.
         */
        virtual void serialize(DataBlock& blk) const = 0;

        /**
         * @brief Deserializes keyframes (event) data from the blk object.
         *
         * @param [in] Blk object to extract data from.
         */
        virtual void deserialize(DataBlock& blk) = 0;
    };

    /**
     * @brief A wrapper class around another IAnimationEditor instance that accumulates changes and commits them when destroyed.
     *      
     * @tparam TKeyFrameValueType Type of the animated parameter.
     * 
     * @note    All modifying functions of this class actually accumulate changes and do not apply them to the underlying animation object.
     *          In order to apply the changes call @ref commit method.
     * @note    All `get...` methods retrieve values taking all local uncommited changes in the editor.
     *          If the actual state of the managed object is required, use the corresponding API.
     */
    template<typename TKeyFrameValueType>
    class AutoAnimationEditor final : public IAnimationEditor<TKeyFrameValueType>
    {
    public:

        /**
         * @brief Initialization constructor.
         * 
         * @param [in] holder IAnimationEditor object that actually manages the animation. See m_editor.
         */
        AutoAnimationEditor(IAnimationEditor<TKeyFrameValueType>* holder)
            : m_editor(holder)
        {
        }

        /**
         * @brief Destructor.
         */
        virtual ~AutoAnimationEditor()
        {
            AutoAnimationEditor::commit();
        }

        /**
         * @brief Checks whether the editor inner state is valid.
         * 
         * @return `true` is the editor inner state is valid, `false` otherwise.
         */
        operator bool()
        {
            return m_editor != nullptr;
        }


        /**
         * @brief Passes all local changes to the wrapped editor object. 
         * 
         * @note This method does not actually change the underlying animation. The wrapped editor is responsible for applying the changes.
         */
        virtual void commit() override
        {
            if (m_editor)
            {
                if (m_isKeyFramesDirty)
                {
                    m_editor->clearKeyFrames();

                    for (const auto& pair : m_keyFrames)
                    {
                        m_editor->addKeyFrame(pair.first, pair.second);
                    }

                    m_isKeyFramesDirty = false;
                }
                if (m_isFrameDataDirty)
                {
                    m_editor->clearFrameData();

                    for (const auto& pair : m_frameData)
                    {
                        m_editor->addFrameData(pair.second);
                    }

                    m_isFrameDataDirty = false;
                }
            }
        }


        /**
         * @brief Retrieves the number of animation keyframes.
         *
         * @return Number of key frames.
         */
        virtual int getKeyFrameCount() const override
        {
            if (m_isKeyFramesDirty)
            {
                return m_keyFrames.size();
            }
            if (m_editor)
            {
                return m_editor->getKeyFrameCount();
            }
            return 0;
        }

        /**
         * @brief Retrieves a keyframe by its index.
         *
         * @param [in] index    Index of the keyframe among other keyframes.
         * @return              Retrieved keyframe.
         */
        virtual KeyFrameImpl<TKeyFrameValueType> getKeyFrameAtIndex(int index) const override
        {
            int i = 0;
            for (auto it = m_keyFrames.begin(); it != m_keyFrames.end(); ++it, ++i)
            {
                if (index == i)
                {
                    return KeyFrameImpl<TKeyFrameValueType>{it->first, it->second };
                }
            }

            return KeyFrameImpl<TKeyFrameValueType>();
        }

        /**
         * @brief Removes all keyframes.
         */
        virtual void clearKeyFrames() override
        {
            m_keyFrames.clear();
        }

        /**
         * @brief Adds a keyframe to the animation.
         * 
         * @param [in] frame Index of the key frame.
         * @param [in] value Value that the animated property should have when the playback reaches this keyframe.
         */
        virtual void addKeyFrame(int frame, const TKeyFrameValueType& value) override
        {
            touchKeyFrames();
            m_keyFrames[frame] = value;
        }

        /**
         * @brief Removes the keyframe from the animation.
         *
         * @param [in] frame Index of the keyframe (among ordinary frames).
         */
        virtual bool deleteKeyFrame(int frame) override
        {
            touchKeyFrames();

            return m_keyFrames.erase(frame) != 0;
        }

        /**
         * @brief Retrieves the number of registered frame data entries.
         *
         * @return Number of frame data entries.
         *
         * Currently frame data encapsulates a collection of events that are triggered upon playback reaching the frame.
         * See Frame.
         */
        virtual int getFrameDataCount() const override
        {
            if (m_isFrameDataDirty)
            {
                return m_frameData.size();
            }
            if (m_editor)
            {
                return m_editor->getFrameDataCount();
            }
            return 0;
        } 

        /**
         * @brief Retrieves a frame data entry that is assoicated with the frame.
         *
         * @param [in] index    Index of the frame.
         * @return              Data entry associated with the frame.
         */
        virtual Frame getFrameDataAtIndex(int index) const override
        {
            int i = 0;
            for (auto it = m_frameData.begin(); it != m_frameData.end(); ++it, ++i)
            {
                if (index == i)
                {
                    return it->second;
                }
            }

            return Frame();
        }

        /**
         * @brief Removes all frame data entries.
         */
        virtual void clearFrameData() override
        {
            m_frameData.clear();
        }

        /**
         * @brief Adds the frame entry to the animation.
         *
         * @param [in] frameData    Frame data entry to add.
         */
        virtual void addFrameData(const Frame& frameData) override
        {
            m_frameData[frameData.frame] = frameData;
        }

        /**
         * @brief Attaches the frame event to the frame.
         *
         * @param [in] frame Index of the frame to attach the event to.
         * @param [in] value Event to attach.
         *
         * @note Maximal number of events attached to a frame is limited by MAX_EVENTS_PER_FRAME.
         */
        virtual void addFrameEvent(int frame, const FrameEvent& value) override
        {
            touchFrameData();

            if (auto framesIt = m_frameData.find(frame); framesIt != m_frameData.end())
            {
                framesIt->second.events.push_back(value);
            }
            else
            {
                auto& newFrameIt = m_frameData.try_emplace(frame).first->second;
                newFrameIt.frame = frame;
                newFrameIt.events.push_back(value);
            }
        }

        /**
         * @brief Detaches the frame event from the frame.
         *
         * @param [in] frame    Index of the frame to detach the event from.
         * @param [in] eventid  Identifier of the event to detach.
         */
        virtual bool deleteFrameEvent(int frame, const char* eventId) override 
        {
            touchFrameData();

            if (auto framesIt = m_frameData.find(frame); framesIt != m_frameData.end())
            {
                auto& events = framesIt->second.events;
                auto eraseIt = std::remove_if(events.begin(), events.end(), [&eventId](const auto& event)
                    {
                        return event.getId() == eventId;
                    });
                if (eraseIt != events.end())
                {
                    events.erase(eraseIt);
                    return true;
                }
            }

            return false;
        }

        /**
         * @brief Retrieves the number of events attached to the frame.
         *
         * @param [in] frame    Index of the frame.
         * @return              Number of events attached to the frame.
         */
        virtual int getEventCount(int frame) const override
        {
            if (m_isFrameDataDirty)
            {
                if (auto it = m_frameData.find(frame); it != m_frameData.end())
                {
                    return it->second.events.size();
                }
            }
            if (m_editor)
            {
                return m_editor->getEventCount(frame);
            }
            return 0;
        }

        /**
         * @brief Retrieves the identifier of the frame event.
         *
         * @param [in] frame    Index of the frame the event is attached to.
         * @param [in] index    Index of the event in the event collection associated with the frame.
         * @return              Event string identifier.
         */
        virtual std::string_view getEventId(int frame, int index) const override
        {
            if (m_isFrameDataDirty)
            {
                if (auto it = m_frameData.find(frame); it != m_frameData.end())
                {
                    return it->second.events[index].getId();
                }
            }
            if (m_editor)
            {
                return m_editor->getEventId(frame, index);
            }
            return std::string_view();
        }
        
        /**
         * @brief Serializes keyframes (event) data into the blk object.
         *
         * @param [out] Blk object to write serialized data into.
         */
        virtual void serialize(DataBlock& blk) const override
        {
            if (m_editor)
            {
                m_editor->serialize(blk);
            }
        }

        /**
         * @brief Deserializes keyframes (event) data from the blk object.
         *
         * @param [in] Blk object to extract data from.
         */
        virtual void deserialize(DataBlock& blk) override
        {
            if (m_editor)
            {
                m_editor->deserialize(blk);
            }
        }

    private:

        /**
         * @brief Synchronizes wrapper key frames collection with the underlying animation state.
         */
        void touchKeyFrames()
        {
            if (!m_isKeyFramesDirty && m_editor)
            {
                for (int frameIndex = 0; frameIndex < m_editor->getKeyFrameCount(); ++frameIndex)
                {
                    const auto keyFrame = m_editor->getKeyFrameAtIndex(frameIndex);
                    m_keyFrames[keyFrame.getFrame()] = keyFrame.getValue();
                }

                m_isKeyFramesDirty = true;
            }
        }

        /**
         * @brief Synchronizes wrapper frame data collection with the underlying animation state.
         */
        void touchFrameData()
        {
            if (!m_isFrameDataDirty)
            {
                for (int frameIndex = 0; frameIndex < m_editor->getFrameDataCount(); ++frameIndex)
                {
                    const auto frameData = m_editor->getFrameDataAtIndex(frameIndex);
                    m_frameData[frameData.frame] = frameData;
                }

                m_isFrameDataDirty = true;
            }
        }

        IAnimationEditor<TKeyFrameValueType>* m_editor; ///< A pointer to the IAnimationEditor object that actually manages the animation. It receives all accumulated changes when @ref commit is called.

        std::map<int, TKeyFrameValueType> m_keyFrames;  ///< A collection of keyframes.
        std::map<int, Frame> m_frameData;               ///< A collection of frame data entries.
        bool m_isKeyFramesDirty = false;                ///< Indicates whether the keyframes collection contains uncommitted changes.
        bool m_isFrameDataDirty = false;                ///< Indicates whether the frame data entries collection contains uncommitted changes.
    };

} // namespace nau::animation