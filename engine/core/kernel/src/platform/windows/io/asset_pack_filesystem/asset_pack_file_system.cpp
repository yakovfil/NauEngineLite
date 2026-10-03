// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "./asset_pack_file_system.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <tuple>

#include "./asset_pack_file.h"
#include "nau/io/fs_path.h"
#include "nau/io/nau_container.h"
#include "nau/memory/standard_aliases.h"
#include "nau/memory/stack_allocator.h"
#include "nau/serialization/runtime_value_builder.h"
#include "nau/string/string_utils.h"
#include "nau/utils/preprocessor.h"

#include NAU_PLATFORM_HEADER(diag/win_error.h)

namespace nau::io
{
    namespace
    {
        std::string splitAndMergePath(std::string& path)
        {
            std::replace(path.begin(), path.end(), '\\', '/');

            std::string result;

            for (auto element : strings::split(path, "/"))
            {
                if (!element.empty())
                {
                    result += "/";
                    result.append(element.data(), element.size());
                }
            }

            return result;
        }

        struct AssetPackDirIteratorData
        {
            AssetPackFileSystemImpl::AssetPackNode* root = nullptr;
            AssetPackFileSystemImpl::AssetPackNode* current = nullptr;
            FsPath basePath;
        };

        size_t pageAlignedOffset(size_t offset)
        {
            return (offset / g_PageAlignment) * g_PageAlignment;
        }

        FsEntry assetPackNodeToFsEntry(const FsPath& basePath, AssetPackFileSystemImpl::AssetPackNode* node)
        {
            if (!node)
            {
                return {};
            }
            const auto kind = node->hasContent() ? FsEntryKind::File : FsEntryKind::Directory;
            const size_t size = (kind == FsEntryKind::File) ? node->getContent()->size : 0;

            return FsEntry{
                .path = basePath / node->getFilePath(),
                .kind = kind,
                .size = static_cast<size_t>(size),
                .lastWriteTime = 0};
        }
    }  // namespace

    std::string_view AssetPackFileSystemImpl::AssetPackNode::getFilePath() const
    {
        return m_filePath;
    }

    Result<AssetPackFileSystemImpl::AssetPackNode*> AssetPackFileSystemImpl::AssetPackNode::getChild(std::string_view filePath)
    {
        lock_(m_mutex);

        auto iter = findChildIter(filePath);
        if (iter != m_children.end())
        {
            return &(*iter);
        }

        NAU_ASSERT(m_view.size == 0);
        if (m_view.size != 0)
        {
            return NauMakeError("Already has mapped view");
        }

        m_children.emplace_back(filePath);
        return &m_children.back();
    }

    AssetPackFileSystemImpl::AssetPackNode* AssetPackFileSystemImpl::AssetPackNode::findChild(std::string_view filePath)
    {
        lock_(m_mutex);

        auto iter = findChildIter(filePath);

        return iter != m_children.end() ? &(*iter) : nullptr;
    }

    AssetPackFileSystemImpl::AssetPackNode* AssetPackFileSystemImpl::AssetPackNode::getNextChild(const AssetPackNode* current)
    {
        lock_(m_mutex);
        if (current == nullptr)
        {
            return !m_children.empty() ? &m_children.front() : nullptr;
        }

        auto nextChild = m_children.end();

        for (auto iter = m_children.begin(); iter != m_children.end(); ++iter)
        {
            if (&(*iter) == current)
            {
                nextChild = ++iter;
                break;
            }
        }

        return nextChild != m_children.end() ? &(*nextChild) : nullptr;
    }

    AssetPackFileSystemImpl::MapView* AssetPackFileSystemImpl::AssetPackNode::getContent()
    {
        return &m_view;
    }

    bool AssetPackFileSystemImpl::AssetPackNode::hasContent() const
    {
        return m_view.size != 0;
    }

    AssetPackFileSystemImpl::AssetPackFileSystemImpl(std::u8string_view assetPackPath, AssetPackFileSystemSettings settings) :
        m_lifetimeOfCache(settings.lifetimeOfCache),
        m_maxCacheSize(settings.maxCacheSize),
        m_assetPackPath(std::move(assetPackPath)),
        m_root(FsPath(m_assetPackPath).getStem())
    {
        m_fileHandle = ::CreateFileW(strings::utf8ToWString(m_assetPackPath).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (m_fileHandle == nullptr)
        {
            NauMakeErrorT(diag::WinCodeError)("File handle is NULL.");
        }

        m_fileSize = GetFileSize(m_fileHandle, nullptr);
        NAU_VERIFY(m_fileSize != 0);

        m_fileMapHandle = CreateFileMappingA(m_fileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
        if (m_fileMapHandle == nullptr)
        {
            NauMakeErrorT(diag::WinCodeError)("File map handle is NULL.");
        }

        FILETIME fileTimeCreated;
        GetFileTime(m_fileHandle, &fileTimeCreated, nullptr, nullptr);
        m_fileTimeCreated = fileTimeCreated.dwHighDateTime;

        const auto& [startPtr, size] = requestRead(0, g_PageAlignment);
        NAU_VERIFY(startPtr);
        auto stream = createReadonlyMemoryStream({reinterpret_cast<std::byte*>(startPtr), size});

        RuntimeValue::Ptr packData;
        size_t headerDataOffset;
        std::tie(packData, headerDataOffset) = *readContainerHeader(stream);

        io::AssetPackIndexData packIndexData;
        auto value = nau::makeValueRef(packIndexData);
        auto res = RuntimeValue::assign(value, packData);

        FsPath rootPath(FsPath(m_assetPackPath).getStem());
        size_t fileCount = 0;
        for (io::AssetPackFileEntry& content : packIndexData.content)
        {
            content.filePath = splitAndMergePath(content.filePath);

            FsPath contentPath(content.filePath);

            AssetPackNode* node = &m_root;
            for (auto name : contentPath.splitElements())
            {
                auto res = node->getChild(name);
                node = *res;
            }
            NAU_ASSERT(node);

            MapView* view = node->getContent();
            view->offset = content.blobData.offset + headerDataOffset;
            view->size = content.blobData.size;
            fileCount++;
        }

        m_memPages.clear();
        m_liveFiles.clear();
        m_memPageSize = pageAlignedOffset(std::min(((m_fileSize - headerDataOffset) / fileCount) * 2, m_fileSize));
    }

    AssetPackFileSystemImpl::~AssetPackFileSystemImpl()
    {
        CloseHandle(m_fileMapHandle);
        CloseHandle(m_fileHandle);
    }

    async::Task<> AssetPackFileSystemImpl::disposeAsync()
    {
        co_await m_taskCollection.awaitCompletion();
    }

    bool AssetPackFileSystemImpl::isReadOnly() const
    {
        return true;
    }

    bool AssetPackFileSystemImpl::exists(const FsPath& path, std::optional<FsEntryKind> kind)
    {
        const auto& [basePath, node] = findAssetPackNodeForPath(path);
        return node != nullptr;
    }

    size_t AssetPackFileSystemImpl::getLastWriteTime(const FsPath&)
    {
        return m_fileTimeCreated;
    }

    IFile::Ptr AssetPackFileSystemImpl::openFile(const FsPath& path, AccessModeFlag accessMode, OpenFileMode openMode)
    {
        NAU_ASSERT((openMode == OpenFileMode::OpenExisting || accessMode && AccessMode::Write), "Specified openMode requires write access also");

        const auto& [basePath, node] = findAssetPackNodeForPath(path);
        if (!node)
        {
            return nullptr;
        }

        MapView* view = node->getContent();
        if (view->size == 0)
        {
            return nullptr;
        }
        return rtti::createInstance<AssetPackFile>(this, view->offset, view->size);
    }

    IFileSystem::OpenDirResult AssetPackFileSystemImpl::openDirIterator(const FsPath& path)
    {
        const auto& [basePath, node] = findAssetPackNodeForPath(path);
        if (!node)
        {
            return {};
        }
        AssetPackFileSystemImpl::AssetPackNode* firstChild = node->getNextChild();
        if (!firstChild)
        {
            return {};
        }
        return {
            new AssetPackDirIteratorData{node, firstChild, path},
            assetPackNodeToFsEntry(path, firstChild)
        };
    }

    void AssetPackFileSystemImpl::closeDirIterator(void* ptr)
    {
        if (!ptr)
        {
            return;
        }
        auto* const data = reinterpret_cast<AssetPackDirIteratorData*>(ptr);
        delete data;
    }

    FsEntry AssetPackFileSystemImpl::incrementDirIterator(void* ptr)
    {
        if (!ptr)
        {
            return {};
        }

        auto* const data = reinterpret_cast<AssetPackDirIteratorData*>(ptr);
        NAU_ASSERT(data->root);
        if (data->current = data->root->getNextChild(data->current); !data->current)
        {
            return {};
        }
        return assetPackNodeToFsEntry(data->basePath, data->current);
    }

    std::tuple<void*, size_t> AssetPackFileSystemImpl::requestRead(size_t offset, size_t size)
    {
        const size_t alignOffset = pageAlignedOffset(offset);
        const size_t pageCount = std::ceil(double(size) / m_memPageSize);

        const MemPages& page = requestMemPages(alignOffset, pageCount);
        const size_t clientOffset = offset - page.m_offset;
        NAU_FATAL(clientOffset < page.m_size);

        size_t availableSize = std::min(size, page.m_size - clientOffset);
        m_currentCacheSize += availableSize;

        return {page.m_ptr + clientOffset, std::min(availableSize, size)};
    }

    void AssetPackFileSystemImpl::notifyStreamCreated(size_t offset, size_t size)
    {
        lock_(m_mutex);
        m_liveFiles.emplace_back(offset, size);
    }

    void AssetPackFileSystemImpl::notifyStreamRemoved(size_t offset, [[maybe_unused]] size_t size)
    {
        {
            lock_(m_mutex);
            auto iter = std::find_if(m_liveFiles.begin(), m_liveFiles.end(), [offset](const LiveFileEntry& f)
            {
                return f.offset == offset;
            });

            NAU_ASSERT(iter != m_liveFiles.end());
            NAU_ASSERT(iter->size == size);

            if (iter != m_liveFiles.end())
            {
                m_liveFiles.erase(iter);
            }
        }

        pendingPagesGC();
    }

    void AssetPackFileSystemImpl::pendingPagesGC()
    {
        if (m_gcIsPending.exchange(true))
        {  // GC already pending
            return;
        }

        auto dropPagesTask = [](AssetPackFileSystemImpl& self) -> async::Task<>
        {
            co_await self.m_lifetimeOfCache;
            scope_on_leave
            {
                self.m_gcIsPending = false;
            };

            self.gcPages(self.m_maxCacheSize);
        }(*this);

        m_taskCollection.push(std::move(dropPagesTask));
    }

    void AssetPackFileSystemImpl::gcPages(size_t maxCacheSize)
    {
        {
            shared_lock_(m_mutex);
            if (m_currentCacheSize < maxCacheSize)
            {
                return;
            }
        }

        lock_(m_mutex);

        StackAllocatorUnnamed;

        // pages with older access times come first
        StackVector<const MemPages*> ageSortedPages;
        ageSortedPages.reserve(m_memPages.size());
        for (const MemPages& pages : m_memPages)
        {
            ageSortedPages.push_back(&pages);
        }

        std::sort(ageSortedPages.begin(), ageSortedPages.end(), [](const MemPages* left, const MemPages* right)
        {
            return left->m_lastAccessTime < right->m_lastAccessTime;
        });

        // Check that the file is partially located inside the page.
        // Or in case of large files (when they may span multiple pages) you need to check if the page range is inside the file range.
        const auto fileOverlapsPages = [](const LiveFileEntry& fileEntry, const MemPages& pages)
        {
            const size_t fileStart = fileEntry.offset;
            const size_t fileEnd = fileEntry.offset + fileEntry.size;

            const size_t pageStart = pages.m_offset;
            const size_t pageEnd = pages.m_offset + pages.m_size;

            if ((pageStart <= fileStart && fileStart < pageEnd) || (pageStart <= fileEnd && fileEnd < pageEnd))
            {
                return true;
            }

            // Large files can overlaps multiple pages. So need to check that page lies inside file range
            return (fileStart <= pageStart && pageStart < fileEnd) || (fileStart <= pageEnd && pageEnd < fileEnd);
        };

        const auto anyLiveFileOverlapsPages = [this, &fileOverlapsPages](const MemPages& pages)
        {
            return std::any_of(m_liveFiles.begin(), m_liveFiles.end(), [&pages, &fileOverlapsPages](const LiveFileEntry& file)
            {
                return fileOverlapsPages(file, pages);
            });
        };

        // Cleanup pages that does not have any overlapped files.
        // First oldest (by access time) pages should be removed. Stops when the desired cache size is reached.
        for (auto pages = ageSortedPages.begin(); pages != ageSortedPages.end() && m_currentCacheSize > maxCacheSize; ++pages)
        {
            if (!anyLiveFileOverlapsPages(**pages))
            {
                m_currentCacheSize -= (*pages)->m_size;
                m_memPages.erase(**pages);
            }
        }
    }

    const MemPages& AssetPackFileSystemImpl::requestMemPages(size_t offset, size_t pageCount)
    {
        {
            shared_lock_(m_mutex);
            if (auto iter = m_memPages.find({nullptr, offset, m_memPageSize}); iter != m_memPages.end())
            {
                return *iter;
            }
        }

        {
            constexpr size_t MinPageCount = 3;
            const size_t targetCacheSize = m_maxCacheSize / std::max(MinPageCount, pageCount);
            gcPages(targetCacheSize);
        }

        lock_(m_mutex);

        auto findOrCreateMemPage = [this](const size_t pageOffset) -> const MemPages&
        {
            if (auto existingPage = m_memPages.find(MemPages{nullptr, pageOffset, 0}); existingPage != m_memPages.end())
            {
                MemPages::updateLastAccessTime(*existingPage);
                return *existingPage;
            }

            const size_t pageSize = (pageOffset + m_memPageSize) <= m_fileSize ? m_memPageSize : (m_fileSize - pageOffset);
            void* const ptr = ::MapViewOfFile(m_fileMapHandle, FILE_MAP_READ, 0, pageOffset, pageSize);
            NAU_VERIFY(ptr);

            [[maybe_unused]] const auto [iter, emplaceOk] = m_memPages.emplace(reinterpret_cast<std::byte*>(ptr), pageOffset, pageSize);
            NAU_FATAL(emplaceOk);

            return *iter;
        };

        for (size_t i = 1; i < pageCount; ++i)
        {
            findOrCreateMemPage(offset + (i * m_memPageSize));
        }

        return findOrCreateMemPage(offset);
    }

    std::tuple<FsPath, AssetPackFileSystemImpl::AssetPackNode*> AssetPackFileSystemImpl::findAssetPackNodeForPath(const FsPath& path)
    {
        AssetPackNode* node = &m_root;
        FsPath basePath{"/"};

        for (auto name : path.splitElements())
        {
            auto* const next = node->findChild(name);
            if (!next)
            {
                break;
            }

            node = next;
            basePath /= name;
        }

        if (!node)
        {
            return {};
        }

        if (basePath != path && !node->hasContent())
        {
            return {};
        }

        return {std::move(basePath), node};
    }

    IFileSystem::Ptr createAssetPackFileSystem(std::u8string_view assetPackPath, AssetPackFileSystemSettings settings)
    {
        NAU_ASSERT(!assetPackPath.empty());
        if (assetPackPath.empty())
        {
            return nullptr;
        }

        return rtti::createInstance<AssetPackFileSystemImpl>(std::move(assetPackPath), std::move(settings));
    }
}  // namespace nau::io
