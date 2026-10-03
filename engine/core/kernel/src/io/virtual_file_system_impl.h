// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <list>
#include <vector>

#include "nau/io/virtual_file_system.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/threading/spin_lock.h"

namespace nau::io
{
    class VirtualFileSystemImpl final : public IVirtualFileSystem
    {
        NAU_CLASS_(nau::io::VirtualFileSystemImpl, IVirtualFileSystem)

    private:
        struct FileSystemEntry
        {
            IFileSystem::Ptr fs;
            unsigned priority;
        };

    public:
        class FsNode
        {
        public:
            FsNode(std::string name) :
                m_name(std::move(name))
            {
            }

            FsNode(const FsNode&) = delete;

            std::string_view getName() const;

            Result<FsNode*> getChild(std::string_view name);

            FsNode* findChild(std::string_view name);

            FsNode* getNextChild(const FsNode* current = nullptr);

            IFileSystem::Ptr getNextMountedFs(IFileSystem* current = nullptr);

            Result<> mount(IFileSystem::Ptr&&, unsigned priority);

            void unmount(const IFileSystem::Ptr&);

            std::vector<FileSystemEntry> getMountedFs();

            bool hasMounts();

        private:
            decltype(auto) findChildIter(std::string_view name)
            {
                return std::find_if(m_children.begin(), m_children.end(), [name](const FsNode& c)
                {
                    return c.m_name == name;
                });
            }

            const std::string m_name;
            std::list<FsNode> m_children;
            std::vector<FileSystemEntry> m_mountedFs;
            threading::SpinLock m_mutex;
        };

        VirtualFileSystemImpl();

        bool isReadOnly() const override;

        bool exists(const FsPath&, std::optional<FsEntryKind>) override;

        size_t getLastWriteTime(const FsPath&) override;

        IFile::Ptr openFile(const FsPath&, AccessModeFlag accessMode, OpenFileMode openMode) override;

        OpenDirResult openDirIterator(const FsPath& path) override;

        void closeDirIterator(void*) override;

        FsEntry incrementDirIterator(void*) override;

        Result<> createDirectory(const FsPath&) override;

        Result<> remove(const FsPath&, bool recursive = false) override;

        Result<> mount(const FsPath&, IFileSystem::Ptr, unsigned priority) override;

        void unmount(IFileSystem::Ptr) override;

        std::wstring resolveToNativePath(const FsPath& path) override;

    private:
        std::tuple<FsPath, FsNode*> findFsNodeForPath(const FsPath& path);

        FsNode m_root;
    };
}  // namespace nau::io