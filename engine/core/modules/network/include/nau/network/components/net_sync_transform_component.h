// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <string>

#include "nau/scene/components/component_attributes.h"
#include "nau/scene/components/internal/component_internal_attributes.h"
#include "nau/serialization/json_utils.h"
#include "net_sync_base_component.h"

namespace nau
{
    /*
    @brief Data for NetworkTransform component sample
    */
    struct NetworkTransformData
    {
        math::vec3 position = math::vec3::zero();
        math::vec3 scale = math::vec3::zero();
        math::quat rotation = math::quat::identity();

        NAU_CLASS_FIELDS(
            CLASS_FIELD(position),
            CLASS_FIELD(scale),
            CLASS_FIELD(rotation))

        auto operator<=>(const NetworkTransformData&) const = default;

        bool write(std::string& buffer)
        {
            io::InplaceStringWriter<char> writer{buffer};
            serialization::JsonSettings settings;
            auto res = serialization::jsonWrite(writer, makeValueRef(*this, getDefaultAllocator()), settings);
            return res.isSuccess();
        }

        bool read(const std::string& buffer)
        {
            Result<RuntimeValue::Ptr> parseResult = serialization::jsonParseString(buffer, getDefaultAllocator());
            auto root = serialization::jsonParseToValue(buffer);
            if (!root.isError())
            {
                if (parseResult)
                {
                    auto res = RuntimeValue::assign(makeValueRef(*this), *parseResult);
                    return res.isSuccess();
                }
            }
            return false;
        }
    };

    /**
    @brief NetSyncTransformComponent sample
    Sync transform state between network peers
    */
    class NetSyncTransformComponent final : public NetSyncBaseComponent
    {
        NAU_OBJECT(NetSyncTransformComponent, NetSyncBaseComponent)

        NAU_DECLARE_DYNAMIC_OBJECT

        NAU_CLASS_ATTRIBUTES(
            CLASS_ATTRIBUTE(scene::SystemComponentAttrib, true),
            CLASS_ATTRIBUTE(scene::ComponentDisplayNameAttrib, "Net Sync Transform"),
            CLASS_ATTRIBUTE(scene::ComponentDescriptionAttrib, "Net Sync Transform (description)"))

        NAU_CLASS_FIELDS(
            CLASS_NAMED_FIELD(m_transform, "transform"))

        bool wasReplicated() const
        {
            return m_wasReplicated;
        }

    protected:
        void netWrite(BytesBuffer& buffer) override
        {
            auto str = serialization::JsonUtils::stringify(*this);
            size_t length = str.length();
            buffer.resize(length);
            std::byte* data = buffer.data();
            std::memcpy(data, str.data(), length);
        }

        void netRead(const BytesBuffer& buffer) override
        {
            std::u8string str((char8_t*)buffer.data(), (char8_t*)(buffer.data() + buffer.size()));
            auto res = serialization::JsonUtils::parse(*this, str);
            m_wasReplicated = true;
        }

        void netWrite(std::string& buffer) override
        {
            auto& owner = getParentObject();
            m_transform.position = owner.getTranslation();
            m_transform.rotation = owner.getRotation();
            m_transform.scale = owner.getScale();
            m_transform.write(buffer);
        }

        void netRead(const std::string& buffer) override
        {
            m_transform.read(buffer);
            auto& owner = getParentObject();
            owner.setTranslation(m_transform.position);
            owner.setRotation(m_transform.rotation);
            owner.setScale(m_transform.scale);
            m_wasReplicated = true;
        }

        NetworkTransformData m_transform;
        bool m_wasReplicated = false;
    };
}  // namespace nau
