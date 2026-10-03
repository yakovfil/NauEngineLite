// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/assets/asset_path.h"

#include <string>
#include <string_view>
#include <tuple>

#include "nau/assets/asset_manager.h"
#include "nau/service/service_provider.h"
#include "nau/string/string_conv.h"
#include "nau/string/string_utils.h"

namespace nau
{
    namespace
    {
        inline std::tuple<size_t, size_t> getSchemePosAndSize(std::string_view assetFullPath)
        {
            const size_t pos = assetFullPath.find(':');
            return {0, pos};
        }

        inline std::tuple<size_t, size_t> getContainerPathPosAndSize(std::string_view assetFullPath)
        {
            const size_t pos1 = assetFullPath.find(':');
            size_t pos2 = assetFullPath.find('+');

            if (pos2 == std::string::npos)
            {
                pos2 = assetFullPath.size();
            }

            return {pos1 + 1, (pos2 - pos1) - 1};
        }

        inline std::tuple<size_t, size_t> getAssetInnerPathPosAndSize(std::string_view assetFullPath)
        {
            const size_t pos = assetFullPath.find('+');
            if (pos == std::string::npos)
            {
                return {std::string_view::npos, std::string_view::npos};
            }

            static const std::regex re(R"-(\[(.*)\]$)-", std::regex_constants::ECMAScript | std::regex_constants::icase);

            const std::string_view innerPath{assetFullPath.data() + pos + 1, assetFullPath.size() - pos - 1};

            std::cmatch match;
            if (std::regex_match(innerPath.data(), innerPath.data() + innerPath.size(), match, re))
            {
                NAU_FATAL(match.size() > 1);

                const size_t pos = static_cast<size_t>(match[1].first - assetFullPath.data());
                const size_t size = static_cast<size_t>(match[1].second - match[1].first);

                return {pos, size};
            }

            // TODO: commented because logger in tests (without application) is broken.
            // NAU_LOG_WARNING("Invalid asset inner path:({})", innerPathSv);
            return {std::string_view::npos, std::string_view::npos};
        }

        inline bool schemeIsValid(std::string_view scheme)
        {
            return !scheme.empty();
        }

        inline bool containerPathIsValid(std::string_view path)
        {
            return !path.empty();
        }

    }  // namespace

    bool AssetPath::isValid(std::string_view assetPath)
    {
        if (assetPath.empty())
        {
            return false;
        }

        constexpr auto AssetPathReStr = R"-(^([a-zA-Z0-9_\.\-\+\\\/\(\)]+):([a-zA-Z0-9_\.\-\\\/\(\)]+)(\+\[(.*)\])?)-";
        const std::regex re(AssetPathReStr, std::regex_constants::ECMAScript | std::regex_constants::icase);

        const std::string_view innerPathSv = strings::toStringView(assetPath);
        std::cmatch match;
        return std::regex_match(innerPathSv.data(), innerPathSv.data() + innerPathSv.size(), match, re);
    }

    AssetPath::AssetPath() = default;

    AssetPath::AssetPath(const AssetPath&) = default;

    AssetPath::AssetPath(AssetPath&&) = default;

    AssetPath::AssetPath(std::string_view fullAssetPath) :
        m_assetFullPath(fullAssetPath)
    {
        if (!m_assetFullPath.empty())
        {
            const bool pathIsValid = isValid(m_assetFullPath);
            NAU_ASSERT(pathIsValid, "Invalid asset path:({})", m_assetFullPath);
            if (!pathIsValid)
            {
                m_assetFullPath.clear();
            }
            else if (strings::trim(getAssetInnerPath()).empty())
            {
                if (const size_t pos = m_assetFullPath.find('+'); pos != std::string::npos)
                {
                    m_assetFullPath.resize(pos);
                }
            }
        }
        else
        {
            NAU_FAILURE("Empty asset path is invalid, instead use default constructor");
        }
    }

    AssetPath::AssetPath(std::string_view scheme, std::string_view assetContainerPath, std::string_view assetInnerPath) :
        AssetPath(strings::toStringView(::fmt::format("{}:{}+[{}]", scheme, assetContainerPath, assetInnerPath)))
    {
    }

    AssetPath& AssetPath::operator=(const AssetPath&) = default;
    AssetPath& AssetPath::operator=(AssetPath&&) = default;

    std::string_view AssetPath::getScheme() const
    {
        if (m_assetFullPath.empty())
        {
            return {};
        }
        NAU_ASSERT(isValid(m_assetFullPath));

        const auto [pos, size] = getSchemePosAndSize(m_assetFullPath);
        return {m_assetFullPath.data() + pos, size};
    }

    std::string_view AssetPath::getContainerPath() const
    {
        if (m_assetFullPath.empty())
        {
            return {};
        }
        NAU_ASSERT(isValid(m_assetFullPath));

        const auto [pos, size] = getContainerPathPosAndSize(m_assetFullPath);
        return {m_assetFullPath.data() + pos, size};
    }

    std::string_view AssetPath::getSchemeAndContainerPath() const
    {
        if (m_assetFullPath.empty())
        {
            return {};
        }
        NAU_ASSERT(isValid(m_assetFullPath));

        const size_t pos = m_assetFullPath.find('+');
        if (pos == std::string::npos)
        {
            // has no asset inner path - just return full path as is.
            return {m_assetFullPath};
        }

        return {m_assetFullPath.data(), pos};
    }

    std::string_view AssetPath::getAssetInnerPath() const
    {
        if (m_assetFullPath.empty())
        {
            return {};
        }
        NAU_ASSERT(isValid(m_assetFullPath));

        const auto [pos, size] = getAssetInnerPathPosAndSize(m_assetFullPath);
        if (pos == std::string_view::npos)
        {
            // has not asset inner path
            return {};
        }

        NAU_FATAL(size != std::string_view::npos);

        return {m_assetFullPath.data() + pos, size};
    }

    AssetPath& AssetPath::setScheme(std::string_view newScheme)
    {
        if (!static_cast<bool>(*this))
        {
            NAU_FAILURE("Can not set scheme for invalid path:({})", m_assetFullPath);
            return *this;
        }

        if (!schemeIsValid(newScheme))
        {
            NAU_FAILURE("Attempt to set invalid Scheme:({})", newScheme);
            return *this;
        }

        const auto [pos, size] = getSchemePosAndSize(m_assetFullPath);
        m_assetFullPath.replace(pos, size, newScheme.data(), newScheme.size());

        return *this;
    }

    AssetPath& AssetPath::setContainerPath(std::string_view newPath)
    {
        if (!static_cast<bool>(*this))
        {
            NAU_FAILURE("Can not set container path for invalid path:({})", m_assetFullPath);
            return *this;
        }

        if (!containerPathIsValid(newPath))
        {
            NAU_FAILURE("Attempt to set invalid asset container path :({})", newPath);
            return *this;
        }

        const auto [pos, size] = getContainerPathPosAndSize(m_assetFullPath);
        m_assetFullPath.replace(pos, size, newPath.data(), newPath.size());

        return *this;
    }

    AssetPath& AssetPath::setAssetInnerPath(std::string_view newInnerPath)
    {
        if (!static_cast<bool>(*this))
        {
            NAU_FAILURE("Can not set asset inner path for invalid path:({})", m_assetFullPath);
            return *this;
        }

        if (newInnerPath.empty())
        {
            // clear inner path
            const size_t pos = m_assetFullPath.find('+');
            if (pos != std::string_view::npos)
            {
                m_assetFullPath.resize(pos);
            }
        }
        else
        {
            const auto [pos, size] = getAssetInnerPathPosAndSize(m_assetFullPath);
            if (pos == std::string_view::npos)
            {
                // has not asset inner path
                const std::string innerPathStr = ::fmt::format("+[{}]", newInnerPath);
                m_assetFullPath.append(innerPathStr.data(), innerPathStr.size());
            }
            else
            {
                m_assetFullPath.replace(pos, size, newInnerPath.data(), newInnerPath.size());
            }
        }

        return *this;
    }

    bool AssetPath::isEmpty() const
    {
        return m_assetFullPath.empty();
    }

    bool AssetPath::hasScheme(std::string_view scheme) const
    {
        return strings::icaseEqual(getScheme(), scheme);
    }

    std::string AssetPath::toString() const
    {
        return m_assetFullPath;
    }

    Result<AssetPath> AssetPath::resolve() const
    {
        if (!isValid(m_assetFullPath))
        {
            return NauMakeError("Path is invalid:({})", m_assetFullPath);
        }

        IAssetManager* const assetManager = hasServiceProvider() ? getServiceProvider().find<IAssetManager>() : nullptr;
        if (!assetManager)
        {
            return NauMakeError("Asset manager is not accessible");
        }

        return assetManager->resolvePath(*this);
    }

    AssetPath::operator bool() const
    {
        return isValid(m_assetFullPath);
    }

    std::strong_ordering AssetPath::operator<=>(const AssetPath& other) const
    {
        const auto& left = m_assetFullPath;
        const auto& right = other.m_assetFullPath;

        return left.compare(right) <=> 0;
    }

    bool AssetPath::operator==(const AssetPath& other) const
    {
        const auto& left = m_assetFullPath;
        const auto& right = other.m_assetFullPath;

        return left == right;
    }

    bool AssetPath::operator==(std::string_view right) const
    {
        return m_assetFullPath == right;
    }

    Result<> parse(std::string_view str, AssetPath& assetPath)
    {
        return ResultSuccess;
    }

    std::string toString(const AssetPath& assetPath)
    {
        std::string str = assetPath.toString();
        return str.empty() ? std::string{} : std::string{str.data(), str.size()};
    }

}  // namespace nau
