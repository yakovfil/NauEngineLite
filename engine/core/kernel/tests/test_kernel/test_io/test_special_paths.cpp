// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <string>

#include "nau/io/special_paths.h"

namespace nau::test
{
    TEST(TestSpecialPaths, NotEmptyWithDefaultPrefixFileName)
    {
        std::u8string tempFileName = io::getNativeTempFilePath();
        ASSERT_FALSE(tempFileName.empty());
    }

    TEST(TestSpecialPaths, NotEmptyWithPrefixFileName)
    {
        std::u8string tempFileName = io::getNativeTempFilePath(u8"TES");
        ASSERT_FALSE(tempFileName.empty());
    }

    TEST(TestSpecialPaths, CorrectPrefixFileName)
    {
        std::u8string tempFileName = io::getNativeTempFilePath(u8"TMP");
        ASSERT_FALSE(tempFileName.find(u8"TMP") == std::string::npos);
    }

    TEST(TestSpecialPaths, KnownFolder_UserDocuments)
    {
        auto path0 = io::getKnownFolderPath(io::KnownFolder::UserDocuments);
        auto path1 = io::getKnownFolderPath(io::KnownFolder::UserDocuments);
        ASSERT_TRUE(!path0.empty());
        ASSERT_EQ(path0, path1);
        ASSERT_TRUE(std::filesystem::is_directory(path0));
    }

    TEST(TestSpecialPaths, KnownFolder_Home)
    {
        auto path0 = io::getKnownFolderPath(io::KnownFolder::UserHome);
        auto path1 = io::getKnownFolderPath(io::KnownFolder::UserHome);
        ASSERT_TRUE(!path0.empty());
        ASSERT_EQ(path0, path1);
        ASSERT_TRUE(std::filesystem::is_directory(path0));
    }

    TEST(TestSpecialPaths, KnownFolder_LocalAppData)
    {
        auto path0 = io::getKnownFolderPath(io::KnownFolder::LocalAppData);
        auto path1 = io::getKnownFolderPath(io::KnownFolder::LocalAppData);
        ASSERT_TRUE(!path0.empty());
        ASSERT_EQ(path0, path1);
        ASSERT_TRUE(std::filesystem::is_directory(path0));
    }

    TEST(TestSpecialPaths, KnownFolder_Temp)
    {
        auto path0 = io::getKnownFolderPath(io::KnownFolder::Temp);
        auto path1 = io::getKnownFolderPath(io::KnownFolder::Temp);
        ASSERT_TRUE(!path0.empty());
        ASSERT_EQ(path0, path1);
        ASSERT_TRUE(std::filesystem::is_directory(path0));
    }

    TEST(TestSpecialPaths, KnownFolder_ExecutableLocation)
    {
        auto path0 = io::getKnownFolderPath(io::KnownFolder::ExecutableLocation);
        auto path1 = io::getKnownFolderPath(io::KnownFolder::ExecutableLocation);
        ASSERT_TRUE(!path0.empty());
        ASSERT_EQ(path0, path1);
        ASSERT_TRUE(std::filesystem::is_directory(path0));
    }

    TEST(TestSpecialPaths, KnownFolder_Current)
    {
        auto path0 = io::getKnownFolderPath(io::KnownFolder::Current);
        auto path1 = io::getKnownFolderPath(io::KnownFolder::Current);
        ASSERT_TRUE(!path0.empty());
        ASSERT_EQ(path0, path1);
        ASSERT_TRUE(std::filesystem::is_directory(path0));
    }
}  // namespace nau::test
