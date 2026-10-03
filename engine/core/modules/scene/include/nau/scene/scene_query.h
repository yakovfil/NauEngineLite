// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include <optional>
#include <string>
#include <string_view>

#include "nau/utils/enum/enum_reflection.h"
#include "nau/utils/result.h"
#include "nau/utils/uid.h"

namespace nau::scene
{
    /**
     */
    NAU_DEFINE_ENUM_(QueryObjectCategory,
                     Component,
                     Object)

    /**
     */
    struct NAU_CORESCENE_EXPORT SceneQuery
    {
        std::optional<QueryObjectCategory> category;
        Uid uid = NullUid;
        size_t typeHashCode = 0;

        SceneQuery() = default;
        SceneQuery(QueryObjectCategory inCategory, Uid inUid = NullUid);
        SceneQuery(const SceneQuery&) = default;
        SceneQuery(std::string_view queryString);
        SceneQuery& operator=(const SceneQuery&) = default;

        template <rtti::WithTypeInfo T>
        void setType();

        void setType(const rtti::TypeInfo& typeInfo);

        rtti::TypeInfo getType() const;

        bool hasType() const;

        NAU_CORESCENE_EXPORT friend bool operator==(const SceneQuery& left, const SceneQuery& right);

        NAU_CORESCENE_EXPORT friend Result<> parse(std::string_view queryStr, SceneQuery& query);

        NAU_CORESCENE_EXPORT friend std::string toString(const SceneQuery& query);
    };

    template <rtti::WithTypeInfo T>
    void SceneQuery::setType()
    {
        typeHashCode = rtti::getTypeInfo<T>().getHashCode();
    }

}  // namespace nau::scene
