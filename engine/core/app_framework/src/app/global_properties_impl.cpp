// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "global_properties_impl.h"

#include "nau/io/file_system.h"
#include "nau/io/stream_utils.h"
#include "nau/serialization/json.h"
#include "nau/string/string_conv.h"
#include "nau/string/string_utils.h"

namespace nau
{
    namespace
    {
        // splitting the path by {parent_path, property_name}
        eastl::tuple<eastl::string_view, eastl::string_view> splitPropertyPath(eastl::string_view propertyPath)
        {
            const auto pos = propertyPath.rfind('/');
            if (pos == eastl::string_view::npos)
            {
                // root path
                return {"/", propertyPath};
            }

            const eastl::string_view parentPath{propertyPath.data(), pos};
            const eastl::string_view propName{propertyPath.data() + pos + 1, propertyPath.size() - pos - 1};

            return {parentPath, propName};
        }
    }  // namespace

    // this function is used to access to the GlobalProperties instance from test projects only
    // without the need to create application.
    eastl::unique_ptr<GlobalProperties> createGlobalProperties()
    {
        return eastl::make_unique<GlobalPropertiesImpl>();
    }

    GlobalPropertiesImpl::GlobalPropertiesImpl() :
        m_propsRoot(serialization::jsonCreateDictionary())
    {
        m_propsRoot->as<serialization::JsonValueHolder&>().setGetStringCallback([this](eastl::string_view str)
        {
            return expandConfigString(str);
        });
    }

    RuntimeValue::Ptr GlobalPropertiesImpl::findValueAtPath(eastl::string_view valuePath) const
    {  // BE AWARE: findValueAtPath requires m_mutex lock !
        if (!m_propsRoot)
        {
            return nullptr;
        }

        RuntimeValue::Ptr current = m_propsRoot;

        for (eastl::string_view propName : strings::split(valuePath, eastl::string_view{"/"}))
        {
            auto* const currentDict = current->as<RuntimeReadonlyDictionary*>();
            if (!currentDict)
            {
                if (diag::hasLogger())
                {  // TODO: remove when logger can handle it
                    NAU_LOG_WARNING("Can not read property ({}) value: the enclosing object is not a dictionary");
                }
                return nullptr;
            }

            current = currentDict->getValue(strings::toStringView(propName));
            if (!current)
            {
                return nullptr;
            }
        }

        return current;
    }

    Result<RuntimeDictionary::Ptr> GlobalPropertiesImpl::getDictionaryAtPath(eastl::string_view valuePath, bool createPath)
    {
        // BE AWARE: getDictionaryAtPath requires m_mutex lock !
        NAU_FATAL(m_propsRoot);

        RuntimeValue::Ptr current = m_propsRoot;

        for (eastl::string_view propName : strings::split(valuePath, eastl::string_view{"/"}))
        {
            auto* const currentDict = current->as<RuntimeDictionary*>();
            if (!currentDict)
            {
                return NauMakeError("The enclosing object is not a dictionary", valuePath);
            }

            std::string_view key = strings::toStringView(propName);

            if (!currentDict->containsKey(key))
            {
                if (!createPath)
                {
                    return NauMakeError("Path not exists");
                }

                NauCheckResult(currentDict->setValue(key, serialization::jsonCreateDictionary()));
            }

            current = currentDict->getValue(key);
        }

        if (current && current->is<RuntimeDictionary>())
        {
            return current;
        }

        return NauMakeError("The enclosing object is not a dictionary ({})", valuePath);
    }

    eastl::optional<eastl::string> GlobalPropertiesImpl::expandConfigString(eastl::string_view str) const
    {
        //  \$([a-zA-Z_0-9\-]*)\{([a-zA-Z_0-9/\-/]*)\}
        //  will parse strings like $[VAR_NAME]{[VAR_VALUE]}
        std::regex re(R"-(\$([a-zA-Z_0-9\-/]*)\{([a-zA-Z_0-9\-/]*)\})-", std::regex_constants::ECMAScript | std::regex_constants::icase);
        std::cmatch match;

        eastl::string_view currentStr = str;
        eastl::string result;

        while (std::regex_search(currentStr.data(), currentStr.data() + currentStr.size(), match, re))
        {
            NAU_FATAL(match.size() >= 3);

            eastl::string_view varKind{match[1].first, static_cast<size_t>(match[1].length())};
            eastl::string_view varValue{match[2].first, static_cast<size_t>(match[2].length())};

            eastl::string replacementStr;

            if (varKind.empty())
            {
                auto propValue = findValueAtPath(varValue);
                if (propValue && propValue->is<RuntimeStringValue>())
                {
                    std::string strValue = propValue->as<const RuntimeStringValue&>().getString();
                    replacementStr.assign(strValue.data(), strValue.size());
                }
            }
            else if (auto resolver = m_variableResolvers.find(varKind); resolver != m_variableResolvers.end())
            {
                eastl::optional<eastl::string> resolvedStr = resolver->second(varValue);
                if (resolvedStr)
                {
                    replacementStr = *std::move(resolvedStr);
                }
                else
                {
                    replacementStr.assign(match[0].first, static_cast<size_t>(match[0].length()));
                }
            }
            else
            {
                replacementStr.assign(match[0].first, static_cast<size_t>(match[0].length()));
            }

            if (const auto& prefix = match.prefix(); prefix.length() > 0)
            {
                result.append(prefix.first, static_cast<size_t>(prefix.length()));
            }

            result.append(replacementStr.data(), replacementStr.size());

            const auto& suffix = match.suffix();
            currentStr = eastl::string_view{suffix.first, static_cast<size_t>(suffix.length())};
        }

        if (result.empty())
        {
            return eastl::nullopt;
        }

        if (!currentStr.empty())
        {
            result.append(currentStr.data(), currentStr.size());
        }

        return result;
    }

    RuntimeValue::Ptr GlobalPropertiesImpl::getRead(eastl::string_view path, [[maybe_unused]] IMemAllocator::Ptr allocator) const
    {
        shared_lock_(m_mutex);
        return findValueAtPath(path);
    }

    bool GlobalPropertiesImpl::contains(eastl::string_view path) const
    {
        shared_lock_(m_mutex);
        return findValueAtPath(path) != nullptr;
    }

    Result<> GlobalPropertiesImpl::set(eastl::string_view path, RuntimeValue::Ptr value)
    {
        NAU_FATAL(m_propsRoot);

        lock_(m_mutex);

        auto [parentPath, propName] = splitPropertyPath(path);

        Result<RuntimeDictionary::Ptr> parentDict = getDictionaryAtPath(parentPath);
        NauCheckResult(parentDict);
        NAU_FATAL(*parentDict);

        return (*parentDict)->setValue(strings::toStringView(propName), value);
    }

    Result<RuntimeValue::Ptr> GlobalPropertiesImpl::getModify(eastl::string_view path, ModificationLock& lock, [[maybe_unused]] IMemAllocator::Ptr allocator)
    {
        NAU_FATAL(m_propsRoot);

        std::unique_lock localLock{m_mutex};

        auto [parentPath, propName] = splitPropertyPath(path);

        Result<RuntimeDictionary::Ptr> parentDict = getDictionaryAtPath(parentPath, false);
        NauCheckResult(parentDict);
        NAU_FATAL(*parentDict);

        if (propName.empty())
        {  // the properties root was requested.
            return parentDict;
        }

        if (!(*parentDict)->containsKey(strings::toStringView(propName)))
        {
            return NauMakeError("To be modifiable the property:() at ({}) must exists first", propName, parentPath);
        }

        RuntimeValue::Ptr childContainer = (*parentDict)->getValue(strings::toStringView(propName));

        const bool propertyIsContainer = childContainer->is<RuntimeDictionary>() || childContainer->is<RuntimeCollection>();
        if (!propertyIsContainer)
        {
            return NauMakeError("Property ({}) expected to be dictionary or collection", propName);
        }

        lock = std::move(localLock);
        return childContainer;
    }

    Result<> GlobalPropertiesImpl::mergeWithValue(const RuntimeValue& value)
    {
        NAU_FATAL(m_propsRoot);

        if (!value.is<RuntimeReadonlyDictionary>())
        {
            return NauMakeError("Dictionary value is expected");
        }

        lock_(m_mutex);

        // const_cast<> is a temporary hack:. currently RuntimeValue::assign accepts nau::Ptr<> that require only non-const values.
        return RuntimeValue::assign(m_propsRoot, nau::Ptr{&const_cast<RuntimeValue&>(value)}, ValueAssignOption::MergeCollection);
    }

    void GlobalPropertiesImpl::addVariableResolver(eastl::string_view kind, VariableResolverCallback resolver)
    {
        NAU_ASSERT(!kind.empty());
        NAU_ASSERT(resolver);

        if (kind.empty() || !resolver)
        {
            return;
        }

        lock_(m_mutex);
        [[maybe_unused]] auto [iter, emplaceOk] = m_variableResolvers.emplace(kind, std::move(resolver));
        NAU_ASSERT(emplaceOk, "Variable resolver ({}) already exists", kind);
    }

    Result<> mergePropertiesFromStream(GlobalProperties& properties, io::IStreamReader& stream, eastl::string_view contentType)
    {
        if (strings::icaseEqual(contentType, "application/json"))
        {
            Result<RuntimeValue::Ptr> parseResult = serialization::jsonParse(stream);
            NauCheckResult(parseResult)

            return properties.mergeWithValue(**parseResult);
        }
        else
        {
            return NauMakeError("Unknown config's content type:({})", contentType);
        }

        return ResultSuccess;
    }

    Result<> mergePropertiesFromFile(GlobalProperties& properties, const std::filesystem::path& filePath, eastl::string_view contentType)
    {
        namespace fs = std::filesystem;
        using namespace nau::io;

        if (!fs::exists(filePath) && fs::is_regular_file(filePath))
        {
            return NauMakeError("Path does not exists or not a file:({})", filePath.string());
        }

        if (contentType.empty())
        {
            if (strings::icaseEqual(std::string_view{filePath.extension().string()}, std::string_view{".json"}))
            {
                contentType = "application/json";
            }
            else
            {
                return NauMakeError("Can not determine file's content type:({})", filePath.string());
            }
        }

        eastl::u8string utf8Path;

        {
            const std::wstring wcsPath = filePath.wstring();
            utf8Path = strings::wstringToUtf8(eastl::wstring_view{wcsPath.data(), wcsPath.size()});
        }

        auto fileStream = createNativeFileStream(reinterpret_cast<const char*>(utf8Path.c_str()), AccessMode::Read, OpenFileMode::OpenExisting);
        if (!fileStream)
        {
            return NauMakeError("Fail to open file:({})", strings::toStringView(utf8Path));
        }

        return mergePropertiesFromStream(properties, fileStream->as<IStreamReader&>(), contentType);
    }

    void dumpPropertiesToStream(GlobalProperties& properties, io::IStreamWriter& stream, eastl::string_view contentType)
    {
        using namespace nau::serialization;

        GlobalProperties::ModificationLock lock;

        auto root = properties.getModify("/", lock);
        NAU_FATAL(root);

        if (strings::icaseEqual(contentType, "application/json"))
        {
            jsonWrite(stream, *root, JsonSettings{.pretty = true, .writeNulls = true}).ignore();
        }
        else
        {
            NAU_FAILURE("Unknown contentType ({})", contentType);
        }
    }

    eastl::string dumpPropertiesToString(GlobalProperties& properties, eastl::string_view contentType)
    {
        eastl::string buffer;
        io::InplaceStringWriter<char> writer{buffer};
        dumpPropertiesToStream(properties, writer);
        return buffer;
    }
}  // namespace nau
