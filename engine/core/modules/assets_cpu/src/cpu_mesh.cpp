// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include "nau/assets/cpu_mesh.h"

#include <cmath>
#include <map>

#include "gltf/gltf_mesh_accessor.h"
#include "nau/io/memory_stream.h"
#include "nau/module/module.h"
#include "nau/serialization/json.h"
#include "nau/service/service_provider.h"

namespace nau
{
    namespace
    {
        // The accepted demo supports the recorded non-interleaved, indexed static mesh.
        // Reject features the shared accessor cannot interpret before constructing it.
        Result<> checkJsonLayout(RuntimeValue::Ptr root)
        {
            auto* dict = root ? root->as<RuntimeReadonlyDictionary*>() : nullptr;
            if (!dict)
                return NauMakeError("CPU mesh: glTF root must be an object");
            for (auto key : {"extensionsRequired", "extensionsUsed", "animations", "skins", "materials"})
                if (dict->containsKey(key))
                    return NauMakeError("CPU mesh: unsupported glTF field {}", key);

            for (auto key : {"meshes", "buffers", "bufferViews", "accessors"})
            {
                auto value = dict->getValue(key);
                auto* array = value ? value->as<RuntimeReadonlyCollection*>() : nullptr;
                if (!array || array->getSize() == 0 || array->getSize() > 64)
                    return NauMakeError("CPU mesh: invalid {} array", key);
                for (size_t i = 0; i < array->getSize(); ++i)
                {
                    auto item = array->getAt(i);
                    auto* fields = item ? item->as<RuntimeReadonlyDictionary*>() : nullptr;
                    if (!fields)
                        return NauMakeError("CPU mesh: invalid {} entry", key);
                    if (fields->containsKey("extensions"))
                        return NauMakeError("CPU mesh: extensions are unsupported");
                    if (std::string_view(key) == "accessors")
                    {
                        for (auto required : {"bufferView", "componentType", "count", "type"})
                            if (!fields->containsKey(required))
                                return NauMakeError("CPU mesh: accessor lacks {}", required);
                        for (auto unsupported : {"byteOffset", "sparse", "normalized"})
                            if (fields->containsKey(unsupported))
                                return NauMakeError("CPU mesh: unsupported accessor {}", unsupported);
                    }
                    if (std::string_view(key) == "bufferViews")
                        for (auto required : {"buffer", "byteOffset", "byteLength"})
                            if (!fields->containsKey(required))
                                return NauMakeError("CPU mesh: buffer view lacks {}", required);
                    if (std::string_view(key) == "buffers")
                        for (auto required : {"uri", "byteLength"})
                            if (!fields->containsKey(required))
                                return NauMakeError("CPU mesh: buffer lacks {}", required);
                    if (std::string_view(key) == "meshes")
                    {
                        auto primitives = fields->getValue("primitives");
                        auto* list = primitives ? primitives->as<RuntimeReadonlyCollection*>() : nullptr;
                        if (!list || list->getSize() != 1)
                            return NauMakeError("CPU mesh: exactly one primitive is required");
                        auto primitive = list->getAt(0);
                        auto* primitiveFields = primitive ? primitive->as<RuntimeReadonlyDictionary*>() : nullptr;
                        if (!primitiveFields || !primitiveFields->containsKey("attributes") || !primitiveFields->containsKey("indices"))
                            return NauMakeError("CPU mesh: primitive lacks attributes or indices");
                        for (auto unsupported : {"mode", "targets", "extensions", "material"})
                            if (primitiveFields->containsKey(unsupported))
                                return NauMakeError("CPU mesh: unsupported primitive {}", unsupported);
                    }
                }
            }
            return ResultSuccess;
        }

        Result<> checkMesh(const GltfFile& file)
        {
            if (file.asset.version != "2.0" || file.meshes.size() != 1 || file.buffers.size() != 1)
                return NauMakeError("CPU mesh: require glTF 2.0, one mesh and one external buffer");
            const auto& buffer = file.buffers.front();
            if (buffer.uri.empty() || buffer.uri == "." || buffer.uri == ".." ||
                buffer.uri.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-") != eastl::string::npos ||
                buffer.byteLength == 0 || buffer.byteLength > 16 * 1024 * 1024)
                return NauMakeError("CPU mesh: invalid external buffer URI or length");
            const auto& primitive = file.meshes.front().primitives.front();
            auto pos = primitive.attributes.find("POSITION");
            auto norm = primitive.attributes.find("NORMAL");
            if (pos == primitive.attributes.end() || norm == primitive.attributes.end() ||
                pos->second >= file.accessors.size() || norm->second >= file.accessors.size() || primitive.indices >= file.accessors.size())
                return NauMakeError("CPU mesh: missing or invalid position, normal or index accessor");
            const auto count = file.accessors[pos->second].count;
            if (count == 0 || count > 65535)
                return NauMakeError("CPU mesh: invalid vertex count");
            const auto checkAccessor = [&](unsigned index, eastl::string_view type, unsigned format, unsigned components, unsigned expectedCount) -> Result<>
            {
                if (index >= file.accessors.size())
                    return NauMakeError("CPU mesh: accessor index out of range");
                const auto& accessor = file.accessors[index];
                if (accessor.type != type || accessor.componentType != format || accessor.count != expectedCount || accessor.bufferView >= file.bufferViews.size())
                    return NauMakeError("CPU mesh: unsupported accessor format or count");
                const auto& view = file.bufferViews[accessor.bufferView];
                const uint64_t bytes = uint64_t(expectedCount) * components * (format == 5123 ? 2 : 4);
                if (view.buffer != 0 || view.byteStride || view.byteLength != bytes || uint64_t(view.byteOffset) + bytes > buffer.byteLength)
                    return NauMakeError("CPU mesh: invalid buffer view range or stride");
                return ResultSuccess;
            };
            for (const auto& [name, index] : primitive.attributes)
            {
                unsigned components = 0;
                if (name == "POSITION" || name == "NORMAL")
                    components = 3;
                else if (name == "TEXCOORD_0")
                    components = 2;
                else if (name == "TANGENT")
                    components = 4;
                else
                    return NauMakeError("CPU mesh: unsupported vertex attribute {}", name);
                const auto type = components == 2 ? "VEC2" : components == 3 ? "VEC3"
                                                                             : "VEC4";
                NauCheckResult(checkAccessor(index, type, 5126, components, count));
            }
            const auto indexCount = file.accessors[primitive.indices].count;
            if (!indexCount || indexCount % 3 || indexCount > 3 * 65535)
                return NauMakeError("CPU mesh: invalid triangle index count");
            return checkAccessor(primitive.indices, "SCALAR", 5123, 1, indexCount);
        }

        class BufferFile final : public io::IFile,
                                 public io_detail::IFileInternal
        {
            NAU_CLASS_(BufferFile, io::IFile, io_detail::IFileInternal)
        public:
            BufferFile(io::FsPath path, std::vector<std::byte> bytes) :
                m_path(std::move(path)),
                m_bytes(std::move(bytes))
            {
            }
            bool supports(FileFeature) const override
            {
                return false;
            }
            bool isOpened() const override
            {
                return true;
            }
            io::IStreamBase::Ptr createStream(std::optional<io::AccessModeFlag> mode) override
            {
                if (mode && mode->has(io::AccessMode::Write))
                    return nullptr;
                return io::createReadonlyMemoryStream({m_bytes.data(), m_bytes.size()});
            }
            io::AccessModeFlag getAccessMode() const override
            {
                return io::AccessMode::Read;
            }
            size_t getSize() const override
            {
                return m_bytes.size();
            }
            io::FsPath getPath() const override
            {
                return m_path;
            }
            void setVfsPath(io::FsPath path) override
            {
                m_path = std::move(path);
            }

        private:
            io::FsPath m_path;
            std::vector<std::byte> m_bytes;
        };

        class CpuAssetFileSystem final : public io::IFileSystem
        {
            NAU_CLASS_(CpuAssetFileSystem, io::IFileSystem)
        public:
            explicit CpuAssetFileSystem(std::vector<CpuAssetFile> files)
            {
                for (auto& file : files)
                    m_files.emplace(file.path, rtti::createInstance<BufferFile>(io::FsPath(file.path), std::move(file.bytes)));
            }
            bool isReadOnly() const override
            {
                return true;
            }
            bool exists(const io::FsPath& path, std::optional<io::FsEntryKind> kind) override
            {
                const auto key = path.getString();
                if ((!kind || *kind == io::FsEntryKind::File) && m_files.contains(key))
                    return true;
                if (kind && *kind == io::FsEntryKind::File)
                    return false;
                const auto prefix = key.empty() ? key : key + "/";
                for (const auto& [name, file] : m_files)
                    if (name.starts_with(prefix))
                        return true;
                return false;
            }
            size_t getLastWriteTime(const io::FsPath&) override
            {
                return 0;
            }
            io::IFile::Ptr openFile(const io::FsPath& path, io::AccessModeFlag mode, io::OpenFileMode openMode) override
            {
                if (mode.has(io::AccessMode::Write) || openMode != io::OpenFileMode::OpenExisting)
                    return nullptr;
                auto iter = m_files.find(path.getString());
                return iter == m_files.end() ? nullptr : iter->second;
            }
            OpenDirResult openDirIterator(const io::FsPath& path) override
            {
                const auto key = path.getString();
                const auto prefix = key.empty() ? key : key + "/";
                std::map<std::string, io::FsEntry> entries;
                for (const auto& [name, file] : m_files)
                {
                    if (!name.starts_with(prefix))
                        continue;
                    const auto slash = name.find('/', prefix.size());
                    const auto child = name.substr(0, slash);
                    const bool directory = slash != std::string::npos;
                    entries.emplace(child, io::FsEntry{io::FsPath(child), directory ? io::FsEntryKind::Directory : io::FsEntryKind::File, directory ? 0 : file->getSize(), 0});
                }
                if (entries.empty())
                    return std::tuple<void*, io::FsEntry>{nullptr, {}};
                auto* cursor = new Cursor;
                for (auto& [name, entry] : entries)
                    cursor->entries.push_back(std::move(entry));
                return std::tuple<void*, io::FsEntry>{cursor, cursor->entries.front()};
            }
            void closeDirIterator(void* state) override
            {
                delete static_cast<Cursor*>(state);
            }
            io::FsEntry incrementDirIterator(void* state) override
            {
                auto* cursor = static_cast<Cursor*>(state);
                if (!cursor || ++cursor->index >= cursor->entries.size())
                    return {};
                return cursor->entries[cursor->index];
            }

        private:
            struct Cursor
            {
                std::vector<io::FsEntry> entries;
                size_t index = 0;
            };
            std::map<std::string, io::IFile::Ptr> m_files;
        };

        class CpuMeshContainer final : public IAssetContainer
        {
            NAU_CLASS_(CpuMeshContainer, IAssetContainer)
        public:
            explicit CpuMeshContainer(nau::Ptr<IMeshAssetAccessor> accessor) :
                m_accessor(std::move(accessor))
            {
            }
            nau::Ptr<> getAsset(eastl::string_view path) override
            {
                if (path == "mesh/0")
                    return m_accessor;
                NAU_LOG_WARNING("CPU mesh: unsupported subresource {}", path);
                return nullptr;
            }
            eastl::vector<eastl::string> getContent() const override
            {
                return {"mesh/0"};
            }

        private:
            nau::Ptr<IMeshAssetAccessor> m_accessor;
        };
    }  // namespace

    Result<io::IFileSystem::Ptr> createCpuAssetFileSystem(std::vector<CpuAssetFile> files)
    {
        std::map<std::string, bool> names;
        for (const auto& file : files)
        {
            if (file.path.empty() || file.path.front() == '/' || file.path.find_first_of("\\:") != std::string::npos ||
                file.path == ".." || file.path.starts_with("../") || file.path.ends_with("/..") || file.path.find("/../") != std::string::npos ||
                io::FsPath(file.path).getString() != file.path || !names.emplace(file.path, true).second)
                return NauMakeError("CPU assets: invalid or duplicate package path {}", file.path);
        }
        return rtti::createInstance<CpuAssetFileSystem>(std::move(files));
    }

    eastl::vector<eastl::string_view> CpuMeshContainerLoader::getSupportedAssetKind() const
    {
        return {"gltf"};
    }

    static async::Task<IAssetContainer::Ptr> loadCpuMeshContainer(io::IStreamReader::Ptr stream, AssetContentInfo info)
    {
        if (!stream)
            co_return NauMakeError("CPU mesh: missing glTF stream {}", info.path.getString());
        auto json = serialization::jsonParse(*stream);
        if (!json)
            co_return json.getError();
        co_await checkJsonLayout(*json);
        stream->setPosition(io::OffsetOrigin::Begin, 0);
        GltfFile file{};
        co_await GltfFile::loadFromJsonStream(stream, file);
        co_await checkMesh(file);
        auto* fs = getServiceProvider().find<io::IFileSystem>();
        if (!fs)
            co_return NauMakeError("CPU mesh: no virtual file system");
        const io::FsPath binaryPath = io::FsPath(info.path).getParentPath() / file.buffers.front().uri;
        auto binary = fs->openFile(binaryPath, io::AccessMode::Read, io::OpenFileMode::OpenExisting);
        if (!binary || binary->getSize() != file.buffers.front().byteLength)
            co_return NauMakeError("CPU mesh: missing or wrong-sized buffer {}", binaryPath.getString());
        auto base = binary->createStream(io::AccessMode::Read);
        auto* reader = base ? base->as<io::IStreamReader*>() : nullptr;
        if (!reader)
            co_return NauMakeError("CPU mesh: cannot read buffer {}", binaryPath.getString());
        std::vector<std::byte> bytes(binary->getSize());
        auto copied = io::copyFromStream(bytes.data(), bytes.size(), *reader);
        if (!copied || *copied != bytes.size())
            co_return NauMakeError("CPU mesh: truncated buffer {}", binaryPath.getString());
        eastl::vector<io::IFile::Ptr> buffers{rtti::createInstance<BufferFile>(binaryPath, std::move(bytes))};
        auto accessor = rtti::createInstance<GltfMeshAssetAccessor>(file, 0, buffers);
        co_return rtti::createInstance<CpuMeshContainer>(std::move(accessor));
    }

    async::Task<IAssetContainer::Ptr> CpuMeshContainerLoader::loadFromStream(io::IStreamReader::Ptr stream, AssetContentInfo info)
    {
        const auto path = info.path.getString();
        auto result = co_await loadCpuMeshContainer(std::move(stream), std::move(info)).doTry();
        if (!result)
        {
            auto error = NauMakeError("CPU mesh {}: {}", path, result.getError()->getMessage());
            // CoreAssets converts container failures to an empty view; retain the cause in diagnostics.
            NAU_LOG_ERROR("{}", error->getMessage());
            co_return error;
        }
        co_return *std::move(result);
    }

    eastl::vector<const rtti::TypeInfo*> CpuMeshViewFactory::getAssetViewTypes() const
    {
        return {&rtti::getTypeInfo<CpuMeshView>()};
    }

    async::Task<IAssetView::Ptr> CpuMeshViewFactory::createAssetView(nau::Ptr<> asset, const rtti::TypeInfo& type)
    {
        auto* accessor = asset ? asset->as<IMeshAssetAccessor*>() : nullptr;
        if (!accessor || type != rtti::getTypeInfo<CpuMeshView>())
            co_return NauMakeError("CPU mesh: invalid view request");
        const auto desc = accessor->getDescription();
        auto view = rtti::createInstance<CpuMeshView>();
        view->positions.resize(desc.vertexCount);
        view->normals.resize(desc.vertexCount);
        view->indices.resize(desc.indexCount);
        OutputVertAttribDescription layout[] = {
            {{"POSITION", 0, ElementFormat::Float, AttributeType::Vec3}, view->positions.data(), view->positions.size() * sizeof(view->positions[0]), 0},
            {  {"NORMAL", 0, ElementFormat::Float, AttributeType::Vec3},   view->normals.data(),     view->normals.size() * sizeof(view->normals[0]), 0}
        };
        co_await accessor->copyVertAttribs(layout);
        co_await accessor->copyIndices(view->indices.data(), view->indices.size() * sizeof(uint16_t), ElementFormat::Uint16);
        for (size_t i = 0; i < view->positions.size(); ++i)
        {
            float squaredLength = 0;
            for (size_t axis = 0; axis < 3; ++axis)
            {
                if (!std::isfinite(view->positions[i][axis]) || !std::isfinite(view->normals[i][axis]))
                    co_return NauMakeError("CPU mesh: non-finite position or normal");
                squaredLength += view->normals[i][axis] * view->normals[i][axis];
            }
            if (!std::isfinite(squaredLength) || squaredLength < 1e-8f)
                co_return NauMakeError("CPU mesh: invalid normal length");
        }
        for (auto index : view->indices)
            if (index >= desc.vertexCount)
                co_return NauMakeError("CPU mesh: vertex index out of range");
        co_return view;
    }

    struct CpuAssetsModule : IModule
    {
        nau::string getModuleName() override
        {
            return "CoreAssetsCpu";
        }
        void initialize() override
        {
            NAU_MODULE_EXPORT_SERVICE(CpuMeshContainerLoader);
            NAU_MODULE_EXPORT_SERVICE(CpuMeshViewFactory);
        }
        void postInit() override
        {
        }
        void deinitialize() override
        {
        }
    };
}  // namespace nau
IMPLEMENT_MODULE(nau::CpuAssetsModule);
