// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <compare>
#include <functional>
#include <string>
#include <string_view>

#include "nau/utils/result.h"

namespace nau
{
    /**
     */
    class NAU_COREASSETS_EXPORT AssetPath
    {
    public:
        static bool isValid(std::string_view fullAssetPath);

        AssetPath();
        AssetPath(const AssetPath&);
        AssetPath(AssetPath&&);
        AssetPath(std::string_view fullAssetPath);
        AssetPath(std::string_view scheme, std::string_view assetContainerPath, std::string_view assetInnerPath = {});

        AssetPath& operator=(const AssetPath&);
        AssetPath& operator=(AssetPath&&);

        std::string_view getScheme() const;
        std::string_view getContainerPath() const;
        std::string_view getSchemeAndContainerPath() const;
        std::string_view getAssetInnerPath() const;

        AssetPath& setScheme(std::string_view);
        AssetPath& setContainerPath(std::string_view);
        AssetPath& setAssetInnerPath(std::string_view);

        bool isEmpty() const;
        bool hasScheme(std::string_view scheme) const;
        std::string toString() const;
        Result<AssetPath> resolve() const;

        explicit operator bool() const;
        std::strong_ordering operator<=>(const AssetPath& other) const;
        bool operator==(const AssetPath& other) const;
        bool operator==(std::string_view right) const;

    private:
        size_t getHashCode() const
        {
            return std::hash<std::string>()(m_assetFullPath);
        }

        std::string m_assetFullPath;

        NAU_COREASSETS_EXPORT friend Result<> parse(std::string_view str, AssetPath& assetPath);
        NAU_COREASSETS_EXPORT friend std::string toString(const AssetPath& assetPath);

        template <typename>
        friend struct ::std::hash;
    };
}  // namespace nau

template <>
struct std::hash<::nau::AssetPath>
{
    using is_transparent = void;
    size_t operator()(std::string_view val) const
    {
        return std::hash<std::string_view>{}(val);
    }
    [[nodiscard]]
    size_t operator()(const ::nau::AssetPath& val) const
    {
        return val.getHashCode();
    }
};
