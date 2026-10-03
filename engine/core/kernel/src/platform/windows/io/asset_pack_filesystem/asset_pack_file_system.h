// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <functional>
#include <list>
#include <string_view>
#include <tuple>
#include <unordered_set>

#include "nau/async/task_collection.h"
#include "nau/io/asset_pack.h"
#include "nau/io/asset_pack_file_system.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/threading/spin_lock.h"

namespace nau::io
{
    constexpr size_t g_PageAlignment = 65536;

    /**
     */
    struct MemPages
    {
        MemPages(std::byte* inPtr, size_t inOffset, size_t inSize) :
            m_ptr(inPtr),
            m_offset(inOffset),
            m_size(inSize),
            m_lastAccessTime(std::chrono::system_clock::now())
        {
        }

        MemPages(const MemPages& other) = delete;
        MemPages(MemPages&& other) = delete;
        MemPages& operator=(const MemPages& other) = delete;
        MemPages& operator=(MemPages&& other) = delete;

        ~MemPages()
        {
            if (m_ptr)
            {
                UnmapViewOfFile(m_ptr);
            }
        }

        static void updateLastAccessTime(const MemPages& memPages)
        {
            lock_(memPages.m_accessTimeMutex);
            memPages.m_lastAccessTime = std::chrono::system_clock::now();
        }

        friend size_t getHashCode(const MemPages& memPages)
        {
            return memPages.m_offset;
        }

        friend bool operator==(const MemPages& left, const MemPages& right)
        {
            return left.m_offset == right.m_offset;
        }

        friend std::strong_ordering operator<=>(const MemPages& left, const MemPages& right)
        {
            return left.m_offset <=> right.m_offset;
        }

        std::byte* const m_ptr = nullptr;
        const size_t m_offset = 0;
        const size_t m_size = 0;

        mutable std::chrono::system_clock::time_point m_lastAccessTime;
        mutable nau::threading::SpinLock m_accessTimeMutex;
    };

}  // namespace nau::io

template <>
struct std::hash<nau::io::MemPages>
{
    [[nodiscard]] size_t operator()(const nau::io::MemPages& memPages) const
    {
        return getHashCode(memPages);
    }
};

namespace nau::io
{

    /**
     */
    class AssetPackFileSystemImpl final : public IFileSystem,
                                          public IAsyncDisposable
    {
        NAU_CLASS_(nau::io::AssetPackFileSystemImpl, IFileSystem, IAsyncDisposable)

    public:
        struct MapView
        {
            size_t offset = 0;
            size_t size = 0;
        };

        class AssetPackNode
        {
        public:
            AssetPackNode(std::string_view filePath) :
                m_filePath(filePath.begin(), filePath.end())
            {
            }

            AssetPackNode(const AssetPackNode&) = delete;

            Result<AssetPackNode*> getChild(std::string_view filePath);

            std::string_view getFilePath() const;

            AssetPackNode* findChild(std::string_view filePath);

            AssetPackNode* getNextChild(const AssetPackNode* current = nullptr);

            MapView* getContent();

            bool hasContent() const;

        private:
            decltype(auto) findChildIter(std::string_view filePath)
            {
                return std::find_if(m_children.begin(), m_children.end(), [filePath](const AssetPackNode& c)
                {
                    return c.m_filePath == filePath;
                });
            }

            const std::string m_filePath;
            MapView m_view{0, 0};
            std::list<AssetPackNode> m_children;
            threading::SpinLock m_mutex;
        };

        AssetPackFileSystemImpl(std::u8string_view assetPacksPath, AssetPackFileSystemSettings settings);
        ~AssetPackFileSystemImpl();

        async::Task<> disposeAsync() override;

        bool isReadOnly() const override;

        bool exists(const FsPath&, std::optional<FsEntryKind> kind) override;

        size_t getLastWriteTime(const FsPath&) override;

        IFile::Ptr openFile(const FsPath&, AccessModeFlag accessMode, OpenFileMode openMode) override;

        OpenDirResult openDirIterator(const FsPath& path) override;

        void closeDirIterator(void*) override;

        FsEntry incrementDirIterator(void*) override;

        std::tuple<void*, size_t> requestRead(size_t offset, size_t size);

        void notifyStreamCreated(size_t offset, size_t size);

        void notifyStreamRemoved(size_t offset, size_t size);

    private:
        void pendingPagesGC();

        void gcPages(size_t maxCacheSize);

        const MemPages& requestMemPages(size_t offset, size_t pageCount);

    private:
        struct LiveFileEntry
        {
            size_t offset;
            size_t size;

            LiveFileEntry(size_t inOffset, size_t inSize) :
                offset(inOffset),
                size(inSize)
            {
            }
        };

        std::tuple<FsPath, AssetPackNode*> findAssetPackNodeForPath(const FsPath& path);

        HANDLE m_fileHandle = nullptr;
        HANDLE m_fileMapHandle = nullptr;
        size_t m_fileSize = 0;
        size_t m_fileTimeCreated = 0;

        Vector<LiveFileEntry> m_liveFiles;
        std::unordered_set<MemPages> m_memPages;

        size_t m_memPageSize = g_PageAlignment;
        size_t m_maxCacheSize = 0;
        size_t m_currentCacheSize = 0;

        std::chrono::seconds m_lifetimeOfCache;
        async::TaskCollection m_taskCollection;
        std::atomic<bool> m_gcIsPending = false;

        const std::u8string_view m_assetPackPath;
        AssetPackNode m_root;

        std::shared_mutex m_mutex;
    };
}  // namespace nau::io
