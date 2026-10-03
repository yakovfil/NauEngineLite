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
    /**
     * @brief Provides an interface for IComponentNetSync instance serialization and deserialization.
     */
    class NetSyncComponent final : public NetSyncBaseComponent
    {
        NAU_OBJECT(NetSyncComponent, NetSyncBaseComponent)

        NAU_DECLARE_DYNAMIC_OBJECT

        NAU_CLASS_ATTRIBUTES(
            CLASS_ATTRIBUTE(scene::SystemComponentAttrib, true),
            CLASS_ATTRIBUTE(scene::ComponentDisplayNameAttrib, "Net Sync"),
            CLASS_ATTRIBUTE(scene::ComponentDescriptionAttrib, "Net Sync (description)"))

    protected:
        /**
         * @brief Serializes IComponentNetSync instance into a binary buffer.
         * 
         * @param [out] buffer Buffer containing serialized data
         */
        void netWrite(BytesBuffer& buffer) override
        {
            auto str = serialization::JsonUtils::stringify(*this);
            size_t length = str.length();
            buffer.resize(length);
            std::byte* data = buffer.data();
            std::memcpy(data, str.data(), length);
        }

        /**
         * @brief Deserializes the IComponentNetSync instance from the binary buffer.
         * 
         * @param [in] buffer Buffer containing serialized data
         */
        void netRead(const BytesBuffer& buffer) override
        {
            std::u8string str((char8_t*)buffer.data(), (char8_t*)(buffer.data() + buffer.size()));
            auto res = serialization::JsonUtils::parse(*this, str);
        }
    };
}  // namespace nau
