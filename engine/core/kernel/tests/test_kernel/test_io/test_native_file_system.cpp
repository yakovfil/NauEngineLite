// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "nau/io/file_system.h"
#include "nau/io/stream.h"

#ifdef _WIN32
    #include <windows.h>

TEST(TestNativeFileSystem, UnicodeRootAndFileNames)
{
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() / (L"nau_fs_\u043f\u0440\u043e\u0432\u0435\u0440\u043a\u0430_" + std::to_wstring(GetCurrentProcessId()));
    ASSERT_FALSE(fs::exists(root));
    fs::create_directory(root);
    struct Cleanup
    {
        fs::path root;
        ~Cleanup()
        {
            std::error_code error;
            fs::remove_all(root, error);
        }
    } cleanup{root};
    const auto relative = fs::path(u8"nested/\u0442\u0435\u043a\u0441\u0442.txt");
    fs::create_directory(root / "nested");
    std::ofstream(root / relative, std::ios::binary) << "payload";
    const auto rootUtf8 = root.generic_u8string();
    const auto relativeUtf8 = relative.generic_u8string();
    auto fileSystem = nau::io::createNativeFileSystem(std::string(reinterpret_cast<const char*>(rootUtf8.data()), rootUtf8.size()), false);
    ASSERT_TRUE(fileSystem);
    const nau::io::FsPath filePath(std::string(reinterpret_cast<const char*>(relativeUtf8.data()), relativeUtf8.size()));
    EXPECT_TRUE(fileSystem->exists(filePath, nau::io::FsEntryKind::File));
    auto* native = fileSystem->as<nau::io::INativeFileSystem*>();
    ASSERT_TRUE(native);
    EXPECT_EQ(native->resolveToNativePath(filePath), (root / relative).make_preferred().wstring());
    auto file = fileSystem->openFile(filePath, nau::io::AccessMode::Read, nau::io::OpenFileMode::OpenExisting);
    ASSERT_TRUE(file);
    auto stream = file->createStream();
    char payload[7]{};
    ASSERT_TRUE(stream);
    auto* reader = stream->as<nau::io::IStreamReader*>();
    ASSERT_TRUE(reader);
    auto result = reader->read(reinterpret_cast<std::byte*>(payload), sizeof(payload));
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, sizeof(payload));
    EXPECT_EQ(std::string(payload, sizeof(payload)), "payload");
}
#endif
