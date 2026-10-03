// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "script_manager_impl.h"

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "lua_toolkit/lua_interop.h"
#include "lua_toolkit/lua_utils.h"
#include "nau/app/global_properties.h"
#include "nau/io/file_system.h"
#include "nau/io/stream.h"
#include "nau/memory/bytes_buffer.h"
#include "nau/serialization/json_utils.h"
#include "nau/service/service_provider.h"

namespace nau::scripts
{
    namespace
    {
        struct LuaChunkStreamLoader
        {
            std::array<std::byte, 512> buffer;
            io::IStreamReader& streamReader;

            LuaChunkStreamLoader(io::IStreamReader& inStreamReader) :
                streamReader(inStreamReader)
            {
            }

            static const char* read([[maybe_unused]] lua_State* lua, void* data, size_t* size) noexcept
            {
                auto& self = *reinterpret_cast<LuaChunkStreamLoader*>(data);

                Result<size_t> readResult = self.streamReader.read(self.buffer.data(), self.buffer.size());
                if (!readResult)
                {
                    NAU_ASSERT("Fail to read input stream: ({})", readResult.getError()->getMessage());
                    *size = 0;
                    return nullptr;
                }

                *size = *readResult;
                return *size > 0 ? reinterpret_cast<const char*>(self.buffer.data()) : nullptr;
            }
        };

        struct ScriptsGlobalConfig
        {
            std::vector<io::FsPath> searchPaths;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(searchPaths))
        };

        class InplaceBufferReader final : public io::IStreamReader
        {
            NAU_CLASS_(InplaceBufferReader, io::IStreamReader)

        public:
            InplaceBufferReader(std::span<const std::byte> buffer) :
                m_buffer(buffer)
            {
            }

            size_t getPosition() const override
            {
                return m_readOffset;
            }

            size_t setPosition(io::OffsetOrigin origin, int64_t offset) override
            {
                NAU_FATAL(origin == io::OffsetOrigin::Begin);
                m_readOffset = static_cast<size_t>(offset);
                return m_readOffset;
            }

            Result<size_t> read(std::byte* outBuffer, size_t count) override
            {
                NAU_FATAL(m_readOffset <= m_buffer.size());

                const size_t availSize = (m_buffer.size() - m_readOffset);
                const size_t readCount = std::min(availSize, count);

                memcpy(outBuffer, m_buffer.data(), readCount);
                m_readOffset += readCount;
                return readCount;
            }

        private:
            std::span<const std::byte> m_buffer;
            size_t m_readOffset = 0;
        };

    }  // namespace

    int ScriptManagerImpl::luaRequire(lua_State* l) noexcept
    {
        const auto selfUpvalueIndex = lua_upvalueindex(1);
        NAU_FATAL(lua_type(l, selfUpvalueIndex) == LUA_TLIGHTUSERDATA);
        ScriptManagerImpl* const self = reinterpret_cast<ScriptManagerImpl*>(lua_touserdata(l, selfUpvalueIndex));

        const int top = lua_gettop(l);
        io::FsPath filePath = *lua::cast<std::string>(l, -1);

        // executeFileInternal will keeps result on stack
        Result<> executeFileResult = self->executeFileInternal(filePath);

        if (executeFileResult)
        {
            const int top2 = lua_gettop(l);
            NAU_ASSERT(top2 >= top);
            return top2 - top;
        }

        NAU_LOG_ERROR("Script module ({}) execution error: {}", filePath.getString(), executeFileResult.getError()->getMessage());
        return 0;

    }

    ScriptManagerImpl::~ScriptManagerImpl()
    {
    }

    lua_State* ScriptManagerImpl::getLua() const
    {
        NAU_FATAL(m_luaState);

        return m_luaState;
    }

    async::Task<> ScriptManagerImpl::preInitService()
    {
        auto luaAlloc = []([[maybe_unused]] void* ud, void* ptr, [[maybe_unused]] size_t osize, size_t nsize) noexcept -> void*
        {
            // TODO: replace with custom allocator
            if (nsize == 0)
            {
                ::free(ptr);
                return nullptr;
            }

            void* const memPtr = ::realloc(ptr, nsize);
            NAU_FATAL(memPtr, "Fail to allocate/reallocate script memory:({}) bytes", nsize);
            return memPtr;
        };

        if (std::optional<ScriptsGlobalConfig> config = getServiceProvider().get<GlobalProperties>().getValue<ScriptsGlobalConfig>("/scripts"))
        {
            for (auto& path : config->searchPaths)
            {
                addScriptSearchPath(std::move(path));
            }
        }

        m_luaState = lua_newstate(luaAlloc, nullptr);
        NAU_FATAL(m_luaState);
        luaL_openlibs(m_luaState);

        lua_pushlightuserdata(m_luaState, this);
        lua_pushcclosure(m_luaState, luaRequire, 1);
        lua_setglobal(m_luaState, "require");

        return async::Task<>::makeResolved();
    }

    void ScriptManagerImpl::dispose()
    {
        if (m_luaState)
        {
            lua_close(m_luaState);
        }
    }

    Result<Ptr<>> ScriptManagerImpl::executeScriptFromBytes(const char* scriptName, std::span<const std::byte> scriptCode)
    {
        // TODO: actually reader must be created through rtti::createInstance
        InplaceBufferReader reader(scriptCode);
        LuaChunkStreamLoader loader{reader};

        auto* const luaState = getLua();

        if (lua_load(luaState, LuaChunkStreamLoader::read, &loader, (scriptName ? scriptName : "unnamed"), "t") == 0)
        {
            if (lua_pcall(luaState, 0, 0, 0) != 0)
            {
                auto err = *lua::cast<std::string>(luaState, -1);
                return NauMakeError("Execution error: {}", err);
            }
        }
        else
        {
            auto err = *lua::cast<std::string>(luaState, -1);
            return NauMakeError("Parse error: {}", err);
        }

        return nullptr;
    }

    Result<> ScriptManagerImpl::executeFileInternal(const io::FsPath& filePath)
    {
        auto* const luaState = getLua();

        io::IFileSystem& fs = getServiceProvider().get<io::IFileSystem>();
        io::FsPath moduleFullPath;
        if (filePath.isAbsolute())
        {
            moduleFullPath = filePath;
        }
        else
        {
            for (const auto& scriptsRoot : m_searchPaths)
            {
                io::FsPath modulePath = (scriptsRoot / filePath);
                modulePath = modulePath + m_scriptFileExtension;

                if (fs.exists(modulePath, io::FsEntryKind::File))
                {
                    moduleFullPath = std::move(modulePath);
                    break;
                }
            }
        }

        if (moduleFullPath.isEmpty() || !fs.exists(moduleFullPath, io::FsEntryKind::File))
        {
            return NauMakeError("Script file path not resolved:({})", filePath.getString());
        }

        auto file = fs.openFile(moduleFullPath, io::AccessMode::Read, io::OpenFileMode::OpenExisting);
        if (!file)
        {
            return NauMakeError("Fail to open script file:({})", moduleFullPath.getString());
        }

        io::IStreamReader::Ptr stream = file->createStream();
        LuaChunkStreamLoader loader{*stream};

        if (lua_load(luaState, LuaChunkStreamLoader::read, &loader, moduleFullPath.getString().c_str(), "t") == 0)
        {
            if (lua_pcall(luaState, 0, LUA_MULTRET, 0) != 0)
            {
                auto err = *lua::cast<std::string>(luaState, -1);
                return NauMakeError("Execution error: {}", err);
            }
        }
        else
        {
            auto err = *lua::cast<std::string>(luaState, -1);
            return NauMakeError("Parse error: {}", err);
        }

        return ResultSuccess;
    }

    Result<Ptr<>> ScriptManagerImpl::executeScriptFromFile(const io::FsPath& filePath)
    {
        auto* const luaState = getLua();
        const lua::StackGuard lstackGuard{luaState};

        NauCheckResult(executeFileInternal(filePath));

        return nullptr;
    }

    void ScriptManagerImpl::registerClass(IClassDescriptor::Ptr classDescriptor)
    {
        lua::initializeClass(getLua(), std::move(classDescriptor), false).ignore();
    }

    Result<Ptr<IDispatch>> ScriptManagerImpl::createScriptInstance(std::string_view scriptClass)
    {
        NAU_FATAL(m_luaState);
        NAU_FAILURE("ScriptManager::createScriptInstance not implemented (under development)");

        return nullptr;
    }

    void ScriptManagerImpl::addScriptSearchPath(io::FsPath path)
    {
        m_searchPaths.emplace_back(std::move(path));
    }

    void ScriptManagerImpl::addScriptFileExtension(std::string_view ext)
    {
        m_scriptFileExtension = ext;
    }

    Result<> ScriptManagerImpl::invokeGlobal(std::string_view method, DispatchArguments args, Functor<void(const nau::Ptr<>& result)> resultCallback)
    {
        auto* const luaState = getLua();
        const lua::StackGuard lstackGuard{luaState};

        const int type = lua_getglobal(m_luaState, std::string{method}.c_str());
        NAU_ASSERT(type == LUA_TFUNCTION);
        if (type != LUA_TFUNCTION)
        {
            return NauMakeError("Global ({}) is not resolved to Function", method);
        }

        for (auto& rtArg : args)
        {
            lua::pushRuntimeValue(m_luaState, rtArg).ignore();
        }

        constexpr int MaxResulCount = 1;

        if (lua_pcall(getLua(), static_cast<int>(args.size()), MaxResulCount, 0) != 0)
        {
            auto err = *lua::cast<std::string>(m_luaState, -1);
            return NauMakeError("Parse error: {}", err);
        }

        if (resultCallback)
        {
            const int top = lua_gettop(luaState);

            if (lstackGuard.top != top)
            {
                NAU_ASSERT(lstackGuard.top < top);
                resultCallback(lua::makeValueFromLuaStack(luaState, top));
            }
        }
        else
        {
            resultCallback(nullptr);
        }

        return ResultSuccess;
    }

}  // namespace nau::scripts
