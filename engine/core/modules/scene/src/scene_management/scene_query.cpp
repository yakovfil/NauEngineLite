// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/scene/scene_query.h"

#include <regex>
#include <string_view>

#include "nau/memory/stack_allocator.h"
#include "nau/string/string_conv.h"
#include "nau/string/string_utils.h"

namespace nau::scene
{
    SceneQuery::SceneQuery(QueryObjectCategory inCategory, Uid inUid) :
        category(inCategory),
        uid(inUid)
    {
    }

    void SceneQuery::setType(const rtti::TypeInfo& typeInfo)
    {
        typeHashCode = typeInfo.getHashCode();
    }

    rtti::TypeInfo SceneQuery::getType() const
    {
        NAU_ASSERT(typeHashCode != 0);
        return rtti::makeTypeInfoFromId(typeHashCode);
    }

    bool SceneQuery::hasType() const
    {
        return typeHashCode != 0;
    }

    SceneQuery::SceneQuery(std::string_view queryString)
    {
        parse(strings::toStringView(queryString), *this).ignore();
    }

    bool operator==(const SceneQuery& left, const SceneQuery& right)
    {
        return left.category == right.category &&
               left.uid == right.uid &&
               left.typeHashCode == right.typeHashCode;
    }

    Result<> parse(std::string_view queryStr, SceneQuery& queryData)
    {
        using namespace nau::strings;

        queryData = SceneQuery{};

        if (queryStr.empty())
        {
            return NauMakeError("Invalid query: empty string");
        }

        std::regex re(R"-(,?\s*([A-Za-z0-9_-]*)\s*=\s*([A-Za-z0-9_-]*))-", std::regex_constants::ECMAScript | std::regex_constants::icase);

        std::cmatch match;
        std::string_view currentStr = queryStr;

        while (std::regex_search(currentStr.data(), currentStr.data() + currentStr.size(), match, re))
        {
            NAU_FATAL(match.size() >= 3);

            std::string_view propKey{match[1].first, static_cast<size_t>(match[1].length())};
            std::string_view propValue{match[2].first, static_cast<size_t>(match[2].length())};

            if (icaseEqual(propKey, "category"))
            {
                auto parseCategoryResult = EnumTraits<QueryObjectCategory>::parse(propValue);
                NauCheckResult(parseCategoryResult)
                queryData.category = *parseCategoryResult;
            }
            else if (icaseEqual(propKey, "uid"))
            {
                NauCheckResult(parse(propValue, queryData.uid))
            }
            else if (icaseEqual(propKey, "type_id"))
            {
                queryData.typeHashCode = lexicalCast<size_t>(propValue);
            }
            else
            {
                return NauMakeError("Unknown query param:({})=({})", propKey, propValue);
            }

            const auto& suffix = match.suffix();
            currentStr = std::string_view{suffix.first, static_cast<size_t>(suffix.length())};
        }

        if (!strings::trim(toStringView(currentStr)).empty())
        {
            queryData = SceneQuery{};
            return NauMakeError("Invalid query:({}), unparsed:({})", queryStr, currentStr);
        }

        return ResultSuccess;
    }

    std::string toString(const SceneQuery& queryData)
    {
        using namespace nau::strings;

        StackAllocatorUnnamed;

        std::string resultQueryString;

        const auto appendQueryProperty = [&resultQueryString](std::string_view key, std::string_view value)
        {
            if (!resultQueryString.empty())
            {
                resultQueryString.append(",");
            }

            resultQueryString.append(key.data(), key.size());
            resultQueryString.append("=");
            resultQueryString.append(value.data(), value.size());
        };

        if (queryData.category)
        {
            const std::string_view categoryValue = EnumTraits<QueryObjectCategory>::toString(*queryData.category);
            appendQueryProperty("category", toStringView(categoryValue));
        }

        if (queryData.uid != NullUid)
        {
            const std::string uidValue = toString(queryData.uid);
            appendQueryProperty("uid", toStringView(uidValue));
        }

        if (queryData.typeHashCode != 0)
        {
            const std::string typeIdValue = lexicalCast(queryData.typeHashCode);
            appendQueryProperty("type_id", toStringView(typeIdValue));
        }

        return resultQueryString;
    }
}  // namespace nau::scene
