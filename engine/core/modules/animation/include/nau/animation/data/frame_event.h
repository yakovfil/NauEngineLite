// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string>

namespace nau::animation
{
    /**
     * @brief Enumerates event activation types.
     */
    enum class FrameEventType
    {
        OneTime, ///< Event remains active only for a single frame after it has been triggered. 
        Start,   ///< Event remains active until it is deactivated manually (see FrameEventType::Stop) or the playback reaches the last frame.
        Stop     ///< This used to deactivate previously triggered event. 
    };

    /**
     * @brief Enumerates directions of playback that triggers the event.
     */
    enum class FrameEventActivationDirection
    {
        Forward,
        Backward,
        Any
    };

    /**
     * @brief Encapsulates frame event parameters.
     */
    class FrameEvent
    {
    public:

        /**
         * @brief Desfault constructor.
         */
        FrameEvent() = default;

        /**
         * @brief Initialization constructor,
         * 
         * @param [in] id                   Identifier of the event. See m_id.
         * @param [in] type                 Event activation type.
         * @param [in] activationDirection  Direction of playback that triggers the event.
         */
        FrameEvent(
            const char* id, 
            FrameEventType type = FrameEventType::OneTime, 
            FrameEventActivationDirection activationDirection = FrameEventActivationDirection::Forward)
            : m_id(id)
            , m_eventType(type)
            , m_activationDirection(activationDirection)
        {
        }

        /**
         * @brief Retrieves the frame event identifier.
         * 
         * @return Identifier of the event.
         */
        const std::string& getId() const
        {
            return m_id;
        }

        /**
         * @brief Assigns the identifier to the event.
         * 
         * @param [in] id Identifier to assign.
         */
        void setId(const char* id)
        {
            m_id = id;
        }

        /**
         * @brief Retrieves the event activation type.
         * 
         * @return Event activation type.
         */
        FrameEventType getEventType() const
        {
            return m_eventType;
        }

        /**
         * @brief Sets activation type for the event.
         * 
         * @pram [in] value Type to assign.
         */
        void setEventType(FrameEventType value)
        {
            m_eventType = value;
        }

        /**
         * @brief Retrieves the event activation direction.
         * 
         * @return Event activation direction.
         */
        FrameEventActivationDirection getActivationDirection() const
        {
            return m_activationDirection;
        }

        /**
         * @brief Sets activation direction for the event.
         *
         * @param [in] value Direction to assign.
         */
        void setActivationDirection(FrameEventActivationDirection value)
        {
            m_activationDirection = value;
        }

    private:

        /**
         * @brief String identifier of the event.
         * 
         * @note    Both event of type FrameEventType::Stop and a pinned event (FrameEventType::Start) it deactivates have to
         *          have equal identifiers
         */
        std::string m_id;
        FrameEventType m_eventType = FrameEventType::OneTime;
        FrameEventActivationDirection m_activationDirection = FrameEventActivationDirection::Forward;

    };

} // namespace nau::animation