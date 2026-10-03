// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include "nau/rtti/rtti_impl.h"
#include "nau/serialization/json.h"

namespace nau::json_detail
{
    /**
     */
    class NAU_ABSTRACT_TYPE JsonValueHolderImpl : public virtual IRefCounted,
                                                  public serialization::JsonValueHolder
    {
        NAU_INTERFACE(nau::json_detail::JsonValueHolderImpl, IRefCounted, serialization::JsonValueHolder)

    public:
        Json::Value& getRootJsonValue() final
        {
            return m_root ? m_root->getThisJsonValue() : getThisJsonValue();
        }

        const Json::Value& getRootJsonValue() const final
        {
            return m_root ? m_root->getThisJsonValue() : getThisJsonValue();
        }

        void setGetStringCallback(GetStringCallback callback) final
        {
            if (m_root)
            {
                m_root->setGetStringCallback(std::move(callback));
            }
            else
            {
                NAU_ASSERT(!m_getStringCallback);
                m_getStringCallback = std::move(callback);
            }
        }

        std::optional<std::string> transformString(std::string_view str)
        {
            auto& callback = m_root ? m_root->m_getStringCallback : m_getStringCallback;
            if (callback)
            {
                return callback(str);
            }

            return std::nullopt;
        }

        void setMutable(bool isMutable)
        {
            m_isMutable = isMutable;
        }

    protected:
        JsonValueHolderImpl()
        {
            m_jsonValue.emplace<Json::Value>();
        }

        JsonValueHolderImpl(Json::Value&& value)
        {
            m_jsonValue.emplace<Json::Value>(std::move(value));
        }

        JsonValueHolderImpl(Json::Value& value)
        {
            m_jsonValue.emplace<Json::Value*>(&value);
        }

        JsonValueHolderImpl(const nau::Ptr<JsonValueHolderImpl>& root, Json::Value& value) :
            m_root(root)
        {
            NAU_FATAL(root);
            m_jsonValue.emplace<Json::Value*>(&value);
        }

        // Constructor dedicated to using only with null value representation
        JsonValueHolderImpl(const nau::Ptr<JsonValueHolderImpl>& root) :
            m_root(root)
        {
            NAU_FATAL(root);
            m_jsonValue.emplace<Json::Value>();
        }

        Json::Value& getThisJsonValue() final
        {
            NAU_FATAL(!m_jsonValue.valueless_by_exception());

            if (std::holds_alternative<Json::Value>(m_jsonValue))
            {
                return std::get<Json::Value>(m_jsonValue);
            }

            NAU_FATAL(std::holds_alternative<Json::Value*>(m_jsonValue));
            return *std::get<Json::Value*>(m_jsonValue);
        }

        const Json::Value& getThisJsonValue() const final
        {
            NAU_FATAL(!m_jsonValue.valueless_by_exception());

            if (std::holds_alternative<Json::Value>(m_jsonValue))
            {
                return std::get<Json::Value>(m_jsonValue);
            }

            NAU_FATAL(std::holds_alternative<Json::Value*>(m_jsonValue));
            return *std::get<Json::Value*>(m_jsonValue);
        }

        nau::Ptr<JsonValueHolderImpl> getRoot()
        {
            if (m_root)
            {
                return m_root;
            }

            return nau::Ptr{this};
        }

        const nau::Ptr<JsonValueHolderImpl> m_root;
        std::variant<Json::Value, Json::Value*> m_jsonValue;
        bool m_isMutable = true;
        GetStringCallback m_getStringCallback;
    };

    RuntimeValue::Ptr getValueFromJson(const nau::Ptr<JsonValueHolderImpl>& root, Json::Value& jsonValue);

    RuntimeDictionary::Ptr createJsonDictionary(Json::Value&& jsonValue);

    RuntimeCollection::Ptr createJsonCollection(Json::Value&& jsonValue);

    RuntimeDictionary::Ptr wrapJsonDictionary(Json::Value& jsonValue);

    RuntimeCollection::Ptr wrapJsonCollection(Json::Value& jsonValue);

}  // namespace nau::json_detail
