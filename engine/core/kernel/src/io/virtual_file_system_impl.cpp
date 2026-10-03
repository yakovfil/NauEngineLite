// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "./virtual_file_system_impl.h"

#include <vector>

#include "nau/threading/lock_guard.h"

namespace nau::io
{
    namespace
    {
        struct NAU_ABSTRACT_TYPE DirIteratorImplBase
        {
            using FsNode = VirtualFileSystemImpl::FsNode;

            virtual ~DirIteratorImplBase() = default;

            virtual bool isEnd() = 0;

            virtual bool increment() = 0;

            virtual FsEntry getCurrent() = 0;
        };

        class InnerDirIteratorImpl final : public DirIteratorImplBase
        {
        public:
            InnerDirIteratorImpl(FsPath basePath, FsNode& dir) :
                m_basePath(std::move(basePath)),
                m_dir(dir)
            {
                NAU_ASSERT(m_basePath.isAbsolute());
                m_current = m_dir.getNextChild();
            }

            bool isEnd() override
            {
                return m_current == nullptr;
            }

            bool increment() override
            {
                if (m_current)
                {
                    m_current = m_dir.getNextChild(m_current);
                }

                return m_current != nullptr;
            }

            FsEntry getCurrent() override
            {
                if (!m_current)
                {
                    return {};
                }

                return FsEntry{
                    .path = m_basePath / m_current->getName(),
                    .kind = FsEntryKind::Directory,
                    .size = 0,
                    .lastWriteTime = 0};
            }

        private:
            const FsPath m_basePath;
            FsNode& m_dir;
            const FsNode* m_current = nullptr;
        };

        class MultiFsDirIteratorImpl final : public DirIteratorImplBase
        {
        public:
            MultiFsDirIteratorImpl(FsPath basePath, FsPath relativePath, FsNode& dir) :
                m_basePath(std::move(basePath)),
                m_relativePath(std::move(relativePath)),
                m_dir(dir)
            {
                NAU_ASSERT(m_basePath.isAbsolute());

                m_iter = getNextFsIterator();
            }

            ~MultiFsDirIteratorImpl()
            {
                auto& [fs, fsIter, fsEntry] = m_iter;
                if (fs && fsIter)
                {
                    fs->closeDirIterator(fsIter);
                }
            }

            bool isEnd() override
            {
                return std::get<2>(m_iter).isEmpty();
            }

            bool increment() override
            {
                auto fs = std::get<0>(m_iter);
                auto fsIter = std::get<1>(m_iter);

                if (!(fs && fsIter))
                {
                    return false;
                }

                FsEntry& fsEntry = std::get<2>(m_iter);

                if (fsEntry = fs->incrementDirIterator(fsIter); fsEntry)
                {
                    fsEntry.path = m_basePath / fsEntry.path;
                    return true;
                }

                fs->closeDirIterator(std::exchange(fsIter, nullptr));

                m_iter = getNextFsIterator();

                return !std::get<2>(m_iter).isEmpty();
            }

            FsEntry getCurrent() override
            {
                return std::move(std::get<2>(m_iter));
            }

        private:
            using Iterator = std::tuple<IFileSystem::Ptr, void*, FsEntry>;

            IFileSystem::Ptr getNextFsContainingPath() const
            {
                auto fileSystem = std::get<0>(m_iter);

                do
                {
                    fileSystem = m_dir.getNextMountedFs(fileSystem.get());
                } while (fileSystem && !fileSystem->exists(m_relativePath, FsEntryKind::Directory));

                return fileSystem;
            }

            Iterator getNextFsIterator() const
            {
                auto fs = getNextFsContainingPath();
                if (!fs)
                {
                    return {};
                }

                auto [fsIter, fsEntry] = *fs->openDirIterator(m_relativePath);
                if (!fsIter || !fsEntry)
                {
                    return Iterator{nullptr, nullptr, FsEntry{}};
                }

                NAU_ASSERT(fsIter);

                fsEntry.path = m_basePath / fsEntry.path;

                return {std::move(fs), fsIter, std::move(fsEntry)};
            }

            const FsPath m_basePath;
            const FsPath m_relativePath;
            FsNode& m_dir;
            Iterator m_iter;
        };
    }  // namespace

    std::string_view VirtualFileSystemImpl::FsNode::getName() const
    {
        return m_name;
    }

    Result<VirtualFileSystemImpl::FsNode*> VirtualFileSystemImpl::FsNode::getChild(std::string_view name)
    {
        lock_(m_mutex);

        auto iter = findChildIter(name);
        if (iter != m_children.end())
        {
            return &(*iter);
        }

        NAU_ASSERT(m_mountedFs.empty());
        if (!m_mountedFs.empty())
        {
            return NauMakeError("Already has mounted fs");
        }

        m_children.emplace_back(std::string{name});

        return &m_children.back();
    }

    VirtualFileSystemImpl::FsNode* VirtualFileSystemImpl::FsNode::findChild(std::string_view name)
    {
        lock_(m_mutex);

        auto iter = findChildIter(name);

        return iter != m_children.end() ? &(*iter) : nullptr;
    }

    VirtualFileSystemImpl::FsNode* VirtualFileSystemImpl::FsNode::getNextChild(const FsNode* current)
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

    IFileSystem::Ptr VirtualFileSystemImpl::FsNode::getNextMountedFs(IFileSystem* current)
    {
        lock_(m_mutex);

        if (current == nullptr)
        {
            return !m_mountedFs.empty() ? m_mountedFs.front().fs : nullptr;
        }

        auto nextFs = m_mountedFs.end();
        for (auto iter = m_mountedFs.begin(); iter != m_mountedFs.end(); ++iter)
        {
            if (iter->fs.get() == current)
            {
                nextFs = ++iter;
                break;
            }
        }

        return nextFs != m_mountedFs.end() ? nextFs->fs : nullptr;
    }

    Result<> VirtualFileSystemImpl::FsNode::mount(IFileSystem::Ptr&& fileSystem, unsigned priority)
    {
        lock_(m_mutex);
        NAU_ASSERT(m_children.empty());
        if (!m_children.empty())
        {
            return NauMakeError("Already child directories");
        }

        if (!fileSystem->isReadOnly())
        {
            const bool hasMutableFs = std::any_of(m_mountedFs.begin(), m_mountedFs.end(), [](const FileSystemEntry& entry)
            {
                return !entry.fs->isReadOnly();
            });
            NAU_ASSERT(!hasMutableFs, "Can not use multiple mutable file system at single mount point");

            if (hasMutableFs)
            {
                return NauMakeError("Can not use multiple mutable file system at single mount point");
            }
        }

        m_mountedFs.emplace_back(std::move(fileSystem), priority);
        return {};
    }
    void VirtualFileSystemImpl::FsNode::unmount(const IFileSystem::Ptr&)
    {
    }

    std::vector<VirtualFileSystemImpl::FileSystemEntry> VirtualFileSystemImpl::FsNode::getMountedFs()
    {
        lock_(m_mutex);
        return m_mountedFs;
    }

    bool VirtualFileSystemImpl::FsNode::hasMounts()
    {
        lock_(m_mutex);
        return !m_mountedFs.empty();
    }

    VirtualFileSystemImpl::VirtualFileSystemImpl() :
        m_root("")
    {
    }

    bool VirtualFileSystemImpl::isReadOnly() const
    {
        return false;
    }

    bool VirtualFileSystemImpl::exists(const FsPath& path, std::optional<FsEntryKind> kind)
    {
        auto [basePath, fsNode] = findFsNodeForPath(path);

        // first check only vurtual path (can be only directory)
        if (basePath == path)
        {
            return !kind || (*kind == FsEntryKind::Directory);
        }

        if (!fsNode || !fsNode->hasMounts())
        {
            return false;
        }

        auto mountedFs = fsNode->getMountedFs();

        return std::any_of(mountedFs.begin(), mountedFs.end(), [relativePath = path.getRelativePath(basePath), kind](const FileSystemEntry& fsEntry)
        {
            return fsEntry.fs->exists(relativePath, kind);
        });
    }

    size_t VirtualFileSystemImpl::getLastWriteTime(const FsPath&)
    {
        return 0;
    }

    IFile::Ptr VirtualFileSystemImpl::openFile(const FsPath& path, AccessModeFlag accessMode, OpenFileMode openMode)
    {
        NAU_ASSERT((openMode == OpenFileMode::OpenExisting || accessMode && AccessMode::Write), "Specified openMode requires write access also");

        auto [basePath, fsNode] = findFsNodeForPath(path);

        if (!fsNode)
        {
            return nullptr;
        }

        auto relativePath = path.getRelativePath(basePath);

        const bool requireMutableAccess =
            accessMode && AccessMode::Write ||
            openMode != OpenFileMode::OpenExisting;

        for (const auto& mountedFs : fsNode->getMountedFs())
        {
            if (requireMutableAccess && mountedFs.fs->isReadOnly())
            {
                continue;
            }

            if (auto file = mountedFs.fs->openFile(relativePath, accessMode, openMode))
            {
                auto* const fileInternal = file->as<io_detail::IFileInternal*>();
                NAU_ASSERT(fileInternal, "io_detail::IFileInternal must be implemented");
                if (fileInternal)
                {
                    fileInternal->setVfsPath(path);
                }

                return file;
            }
        }

        return nullptr;
    }

    IFileSystem::OpenDirResult VirtualFileSystemImpl::openDirIterator(const FsPath& path)
    {
        auto [basePath, fsNode] = findFsNodeForPath(path);
        if (!fsNode)
        {
            return NauMakeError("Directory does not exists");
        }

        DirIteratorImplBase* dirIteratorImpl = nullptr;

        if (!fsNode->hasMounts())
        {
            NAU_ASSERT(basePath == path);
            dirIteratorImpl = new InnerDirIteratorImpl(std::move(basePath), *fsNode);
        }
        else
        {
            auto relativePath = path.getRelativePath(basePath);
            dirIteratorImpl = new MultiFsDirIteratorImpl(std::move(basePath), std::move(relativePath), *fsNode);
        }

        if (dirIteratorImpl->isEnd())
        {
            delete dirIteratorImpl;
            return {};
        }

        std::tuple result = {
            reinterpret_cast<void*>(dirIteratorImpl),
            dirIteratorImpl->getCurrent()};

        return std::move(result);
    }

    void VirtualFileSystemImpl::closeDirIterator(void* ptr)
    {
        if (!ptr)
        {
            return;
        }

        auto* const iterImpl = reinterpret_cast<DirIteratorImplBase*>(ptr);
        delete iterImpl;
    }

    FsEntry VirtualFileSystemImpl::incrementDirIterator(void* state)
    {
        NAU_ASSERT(state);
        if (!state)
        {
            return {};
        }

        auto* const iterImpl = reinterpret_cast<DirIteratorImplBase*>(state);
        if (iterImpl->increment())
        {
            return iterImpl->getCurrent();
        }

        return {};
    }

    Result<> VirtualFileSystemImpl::createDirectory(const FsPath&)
    {
        return {};
    }

    Result<> VirtualFileSystemImpl::remove(const FsPath&, bool recursive)
    {
        return {};
    }

    Result<> VirtualFileSystemImpl::mount(const FsPath& path, IFileSystem::Ptr fileSystem, unsigned priority)
    {
        FsNode* fsNode = &m_root;

        for (auto name : path.splitElements())
        {
            auto res = fsNode->getChild(name);
            NauCheckResult(res);
            fsNode = *res;
            NAU_ASSERT(fsNode);
        }

        return fsNode->mount(std::move(fileSystem), priority);
    }

    void VirtualFileSystemImpl::unmount(IFileSystem::Ptr)
    {
    }

    std::wstring VirtualFileSystemImpl::resolveToNativePath(const FsPath& path)
    {
        auto [basePath, fsNode] = findFsNodeForPath(path);
        if (!fsNode || !fsNode->hasMounts())
        {
            return {};
        }

        auto fs = fsNode->getNextMountedFs();
        if (INativeFileSystem* const nativeFs = (fs ? fs->as<INativeFileSystem*>() : nullptr); nativeFs)
        {
            return nativeFs->resolveToNativePath(path.getRelativePath(basePath));
        }

        return {};
    }

    std::tuple<FsPath, VirtualFileSystemImpl::FsNode*> VirtualFileSystemImpl::findFsNodeForPath(const FsPath& path)
    {
        FsNode* fsNode = &m_root;
        FsPath basePath{"/"};

        for (auto name : path.splitElements())
        {
            auto* const next = fsNode->findChild(name);
            if (!next)
            {
                break;
            }

            fsNode = next;
            basePath /= name;
        }

        if (!fsNode)
        {
            return {};
        }

        if (basePath != path && !fsNode->hasMounts())
        {
            return {};
        }

        return std::tuple{std::move(basePath), fsNode};
    }

    IVirtualFileSystem::Ptr createVirtualFileSystem()
    {
        return rtti::createInstance<VirtualFileSystemImpl>();
    }
}  // namespace nau::io
