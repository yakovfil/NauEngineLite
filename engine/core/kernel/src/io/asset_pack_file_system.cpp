// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include "nau/io/asset_pack_file_system.h"

#include <json/json.h>

#include <charconv>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <set>

#include "nau/rtti/rtti_impl.h"

namespace nau::io
{
    Result<std::string> normalizeAssetPackPath(std::string_view path, size_t maxBytes)
    {
        if (path.size() > maxBytes || path.find('\0') != path.npos || path.find(':') != path.npos)
            return NauMakeError("Asset pack: invalid path");
        std::string result;
        size_t begin = 0;
        while (begin < path.size())
        {
            auto end = path.find_first_of("/\\", begin);
            if (end == path.npos)
                end = path.size();
            const auto part = path.substr(begin, end - begin);
            if (part == "..")
                return NauMakeError("Asset pack: parent traversal in path");
            if (!part.empty() && part != ".")
            {
                if (!result.empty())
                    result += '/';
                result += part;
            }
            begin = end + 1;
        }
        return result;
    }

    namespace
    {
        Result<> readExact(IStreamReader& stream, std::byte* bytes, size_t count)
        {
            while (count)
            {
                auto read = stream.read(bytes, count);
                if (!read)
                    return read.getError();
                if (*read == 0 || *read > count)
                    return NauMakeError("Asset pack: truncated stream");
                bytes += *read;
                count -= *read;
            }
            return {};
        }

        Result<uint64_t> unsignedField(const Json::Value& value)
        {
            if (!value.isUInt64())
                return NauMakeError("Asset pack: expected unsigned integer");
            return value.asUInt64();
        }

        struct Backing
        {
            IStreamReader::Ptr stream;
            std::mutex mutex;
        };

        class FileReader final : public IStreamReader
        {
            NAU_CLASS_(FileReader, IStreamReader)
        public:
            explicit FileReader(std::string_view path) :
                m_input(std::filesystem::u8path(path), std::ios::binary)
            {
            }
            bool valid() const
            {
                return m_input.is_open();
            }
            size_t getPosition() const override
            {
                return m_position;
            }
            size_t setPosition(OffsetOrigin origin, int64_t offset) override
            {
                m_input.clear();
                m_input.seekg(offset, origin == OffsetOrigin::Begin ? std::ios::beg : origin == OffsetOrigin::End ? std::ios::end
                                                                                                                  : std::ios::cur);
                auto position = m_input.tellg();
                if (position >= 0 && uint64_t(position) <= std::numeric_limits<size_t>::max())
                    m_position = static_cast<size_t>(position);
                return position < 0 || uint64_t(position) > std::numeric_limits<size_t>::max() ? std::numeric_limits<size_t>::max() : m_position;
            }
            Result<size_t> read(std::byte* bytes, size_t count) override
            {
                if (count > uint64_t(std::numeric_limits<std::streamsize>::max()))
                    return NauMakeError("Asset pack: excessive read size");
                m_input.read(reinterpret_cast<char*>(bytes), static_cast<std::streamsize>(count));
                if (m_input.bad() || (m_input.fail() && !m_input.eof()))
                    return NauMakeError("Asset pack: file read failed");
                const size_t read = static_cast<size_t>(m_input.gcount());
                m_position += read;
                return read;
            }

        private:
            std::ifstream m_input;
            size_t m_position = 0;
        };

        class PackStream final : public IStreamReader
        {
            NAU_CLASS_(PackStream, IStreamReader)
        public:
            PackStream(std::shared_ptr<Backing> backing, BlobData blob) :
                m_backing(std::move(backing)),
                m_blob(blob)
            {
            }
            size_t getPosition() const override
            {
                return m_position;
            }
            size_t setPosition(OffsetOrigin origin, int64_t offset) override
            {
                const size_t base = origin == OffsetOrigin::Begin ? 0 : origin == OffsetOrigin::End ? m_blob.size
                                                                                                    : m_position;
                if (offset < 0)
                {
                    const uint64_t distance = uint64_t(-(offset + 1)) + 1;
                    m_position = distance > base ? 0 : base - static_cast<size_t>(distance);
                }
                else
                    m_position = uint64_t(offset) > m_blob.size - base ? m_blob.size : base + static_cast<size_t>(offset);
                return m_position;
            }
            Result<size_t> read(std::byte* bytes, size_t count) override
            {
                count = std::min(count, m_blob.size - m_position);
                if (!count)
                    return size_t(0);
                if (!bytes)
                    return NauMakeError("Asset pack: null read buffer");
                std::lock_guard guard(m_backing->mutex);
                const size_t absolute = m_blob.offset + m_position;
                if (m_backing->stream->setPosition(OffsetOrigin::Begin, static_cast<int64_t>(absolute)) != absolute)
                    return NauMakeError("Asset pack: backing seek failed");
                NauCheckResult(readExact(*m_backing->stream, bytes, count));
                m_position += count;
                return count;
            }

        private:
            std::shared_ptr<Backing> m_backing;
            BlobData m_blob;
            size_t m_position = 0;
        };

        class PackFile final : public IFile,
                               public io_detail::IFileInternal
        {
            NAU_CLASS_(PackFile, IFile, io_detail::IFileInternal)
        public:
            PackFile(std::shared_ptr<Backing> backing, AssetPackFileEntry entry) :
                m_backing(std::move(backing)),
                m_entry(std::move(entry)),
                m_path(m_entry.filePath)
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
            IStreamBase::Ptr createStream(std::optional<AccessModeFlag> mode) override
            {
                if (mode && mode->has(AccessMode::Write))
                    return nullptr;
                return rtti::createInstance<PackStream>(m_backing, m_entry.blobData);
            }
            AccessModeFlag getAccessMode() const override
            {
                return AccessMode::Read;
            }
            size_t getSize() const override
            {
                return m_entry.blobData.size;
            }
            FsPath getPath() const override
            {
                return m_path;
            }
            void setVfsPath(FsPath path) override
            {
                m_path = std::move(path);
            }

        private:
            std::shared_ptr<Backing> m_backing;
            AssetPackFileEntry m_entry;
            FsPath m_path;
        };

        class PackFileSystem final : public IFileSystem
        {
            NAU_CLASS_(PackFileSystem, IFileSystem)
        public:
            PackFileSystem(IStreamReader::Ptr stream, AssetPackIndexData index) :
                m_backing(std::make_shared<Backing>())
            {
                m_backing->stream = std::move(stream);
                m_directories.insert("");
                for (auto& entry : index.content)
                {
                    for (size_t slash = entry.filePath.find('/'); slash != std::string::npos; slash = entry.filePath.find('/', slash + 1))
                        m_directories.insert(entry.filePath.substr(0, slash));
                    m_files.emplace(entry.filePath, std::move(entry));
                }
            }
            bool isReadOnly() const override
            {
                return true;
            }
            bool exists(const FsPath& path, std::optional<FsEntryKind> kind) override
            {
                auto key = normalizeAssetPackPath(path.getString());
                return key && ((!kind || *kind == FsEntryKind::File) && m_files.contains(*key) || (!kind || *kind == FsEntryKind::Directory) && m_directories.contains(*key));
            }
            size_t getLastWriteTime(const FsPath&) override
            {
                return 0;
            }
            IFile::Ptr openFile(const FsPath& path, AccessModeFlag mode, OpenFileMode openMode) override
            {
                if (mode.has(AccessMode::Write) || openMode != OpenFileMode::OpenExisting)
                    return nullptr;
                auto key = normalizeAssetPackPath(path.getString());
                if (!key)
                    return nullptr;
                auto iter = m_files.find(*key);
                return iter == m_files.end() ? nullptr : rtti::createInstance<PackFile>(m_backing, iter->second);
            }
            OpenDirResult openDirIterator(const FsPath& path) override
            {
                auto key = normalizeAssetPackPath(path.getString());
                if (!key || !m_directories.contains(*key))
                    return NauMakeError("Asset pack: directory does not exist");
                auto cursor = std::make_unique<Cursor>();
                const auto prefix = key->empty() ? "" : *key + "/";
                for (const auto& dir : m_directories)
                    if (dir != *key && dir.starts_with(prefix) && dir.find('/', prefix.size()) == dir.npos)
                        cursor->entries.push_back({FsPath(dir), FsEntryKind::Directory, 0, 0});
                for (const auto& [name, entry] : m_files)
                    if (name.starts_with(prefix) && name.find('/', prefix.size()) == name.npos)
                        cursor->entries.push_back({FsPath(name), FsEntryKind::File, entry.blobData.size, 0});
                if (cursor->entries.empty())
                    return std::tuple<void*, FsEntry>{nullptr, {}};
                auto first = cursor->entries.front();
                return std::tuple<void*, FsEntry>{cursor.release(), std::move(first)};
            }
            void closeDirIterator(void* cursor) override
            {
                delete static_cast<Cursor*>(cursor);
            }
            FsEntry incrementDirIterator(void* state) override
            {
                auto* cursor = static_cast<Cursor*>(state);
                if (!cursor || ++cursor->position >= cursor->entries.size())
                    return {};
                return cursor->entries[cursor->position];
            }

        private:
            struct Cursor
            {
                std::vector<FsEntry> entries;
                size_t position = 0;
            };
            std::shared_ptr<Backing> m_backing;
            std::map<std::string, AssetPackFileEntry> m_files;
            std::set<std::string> m_directories;
        };
    }  // namespace

    Result<AssetPackIndexData> readAssetPackIndex(IStreamReader::Ptr stream, size_t length, AssetPackReadOptions options)
    {
        if (!stream || length > uint64_t(std::numeric_limits<int64_t>::max()) || stream->setPosition(OffsetOrigin::Begin, 0) != 0)
            return NauMakeError("Asset pack: invalid backing stream or length");
        std::string header;
        while (!header.ends_with("\n\n"))
        {
            if (header.size() >= options.maxHeaderBytes || header.size() >= length)
                return NauMakeError("Asset pack: invalid or oversized header");
            char byte;
            NauCheckResult(readExact(*stream, reinterpret_cast<std::byte*>(&byte), 1));
            header += byte;
        }
        std::map<std::string, std::string> fields;
        size_t begin = 0;
        while (begin < header.size() - 1)
        {
            const auto end = header.find('\n', begin);
            auto line = std::string_view(header).substr(begin, end - begin);
            if (line.empty())
                break;
            const auto colon = line.find(':');
            if (colon == line.npos)
                return NauMakeError("Asset pack: malformed header field");
            auto value = line.substr(colon + 1);
            while (!value.empty() && value.front() == ' ')
                value.remove_prefix(1);
            if (!fields.emplace(std::string(line.substr(0, colon)), std::string(value)).second)
                return NauMakeError("Asset pack: duplicate header field");
            begin = end + 1;
        }
        if (fields["NauContent-Kind"] != "nau-vfs-pack" || fields["Content-Type"] != "application/json")
            return NauMakeError("Asset pack: unsupported container kind or content type");
        const auto& number = fields["Content-Length"];
        uint64_t indexLength = 0;
        auto converted = std::from_chars(number.data(), number.data() + number.size(), indexLength);
        if (converted.ec != std::errc{} || converted.ptr != number.data() + number.size() || !indexLength || indexLength > options.maxIndexBytes || indexLength >= length - header.size())
            return NauMakeError("Asset pack: invalid index length");
        // The established writer emits a third newline before the JSON index.
        char separator;
        NauCheckResult(readExact(*stream, reinterpret_cast<std::byte*>(&separator), 1));
        if (separator != '\n')
            return NauMakeError("Asset pack: missing index separator");
        std::string json(static_cast<size_t>(indexLength), '\0');
        NauCheckResult(readExact(*stream, reinterpret_cast<std::byte*>(json.data()), json.size()));
        Json::CharReaderBuilder builder;
        builder["rejectDupKeys"] = true;
        builder["failIfExtra"] = true;
        builder["allowComments"] = false;
        builder["stackLimit"] = 64;
        auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
        Json::Value root;
        std::string errors;
        if (!reader->parse(json.data(), json.data() + json.size(), &root, &errors) || !root.isObject() || root["version"] != "0.1" || !root["content"].isArray())
            return NauMakeError("Asset pack: malformed index or unsupported version");
        if (root["content"].size() > options.maxEntries || !root["description"].isString())
            return NauMakeError("Asset pack: invalid index structure or entry limit");
        AssetPackIndexData index;
        index.version = "0.1";
        index.description = root["description"].asString();
        const size_t payloadOffset = header.size() + 1 + json.size();
        std::set<std::string> paths;
        for (const auto& item : root["content"])
        {
            if (!item.isObject() || !item["filePath"].isString() || !item["contentCompression"].isString() || !item["blobData"].isObject())
                return NauMakeError("Asset pack: malformed entry");
            const auto compression = item["contentCompression"].asString();
            // Older writer assigned size_t(0) to std::string, yielding one NUL.
            if (!compression.empty() && compression != std::string(1, '\0'))
                return NauMakeError("Asset pack: unsupported compression");
            auto path = normalizeAssetPackPath(item["filePath"].asString(), options.maxPathBytes);
            if (!path || path->empty() || !paths.insert(*path).second)
                return NauMakeError("Asset pack: invalid or duplicate entry path");
            auto offset = unsignedField(item["blobData"]["offset"]);
            auto size = unsignedField(item["blobData"]["size"]);
            auto clientSize = unsignedField(item["clientSize"]);
            if (!offset || !size || !clientSize || *size != *clientSize || *offset > length - payloadOffset || *size > length - payloadOffset - *offset)
                return NauMakeError("Asset pack: invalid payload range for {}", *path);
            index.content.push_back({
                *path,
                "",
                static_cast<size_t>(*size),
                {static_cast<size_t>(*size), payloadOffset + static_cast<size_t>(*offset)}
            });
        }
        for (const auto& path : paths)
            for (size_t slash = path.find('/'); slash != path.npos; slash = path.find('/', slash + 1))
                if (paths.contains(path.substr(0, slash)))
                    return NauMakeError("Asset pack: file/directory conflict");
        return index;
    }

    Result<IFileSystem::Ptr> createAssetPackFileSystem(IStreamReader::Ptr stream, size_t length, AssetPackReadOptions options)
    {
        auto index = readAssetPackIndex(stream, length, options);
        if (!index)
            return index.getError();
        return IFileSystem::Ptr(rtti::createInstance<PackFileSystem>(std::move(stream), std::move(*index)));
    }

    Result<IFileSystem::Ptr> openAssetPackFileSystem(std::string_view path, AssetPackReadOptions options)
    {
        auto stream = rtti::createInstance<FileReader>(path);
        if (!stream->valid())
            return NauMakeError("Asset pack: cannot open {}", path);
        const size_t length = stream->setPosition(OffsetOrigin::End, 0);
        auto result = createAssetPackFileSystem(stream, length, options);
        if (!result)
            return NauMakeError("Asset pack {}: {}", path, result.getError()->getMessage());
        return result;
    }
}  // namespace nau::io
