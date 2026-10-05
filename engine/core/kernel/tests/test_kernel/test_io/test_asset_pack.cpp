// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include <gtest/gtest.h>
#include <json/json.h>

#include <fstream>
#include <thread>

#include "nau/io/asset_pack_file_system.h"
#include "nau/io/memory_stream.h"

namespace
{
    Json::Value entry(std::string path, uint64_t offset = 0, uint64_t size = 3)
    {
        Json::Value value;
        value["filePath"] = path;
        value["contentCompression"] = "";
        value["clientSize"] = Json::UInt64(size);
        value["blobData"]["offset"] = Json::UInt64(offset);
        value["blobData"]["size"] = Json::UInt64(size);
        return value;
    }
    std::string package(Json::Value entries, std::string payload = "abcdef")
    {
        Json::Value root;
        root["version"] = "0.1";
        root["description"] = "test";
        root["content"] = std::move(entries);
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        auto index = Json::writeString(builder, root);
        return "NauContent-Kind: nau-vfs-pack\nContent-Type: application/json\nContent-Length: " + std::to_string(index.size()) + "\n\n\n" + index + payload;
    }
    std::string valid()
    {
        Json::Value list(Json::arrayValue);
        list.append(entry("nested/a", 0));
        list.append(entry("nested/b", 3));
        list.append(entry("empty", 6, 0));
        return package(list);
    }
    nau::io::IMemoryStream::Ptr owned(std::string_view bytes)
    {
        auto stream = nau::io::createMemoryStream();
        auto written = stream->write(reinterpret_cast<const std::byte*>(bytes.data()), bytes.size());
        EXPECT_TRUE(written);
        return stream;
    }
    nau::Result<nau::io::IFileSystem::Ptr> open(std::string_view bytes, nau::io::AssetPackReadOptions limits = {})
    {
        return nau::io::createAssetPackFileSystem(owned(bytes), bytes.size(), limits);
    }
    std::string read(nau::io::IStreamReader& stream, size_t count)
    {
        std::string data(count, '\0');
        auto result = stream.read(reinterpret_cast<std::byte*>(data.data()), count);
        EXPECT_TRUE(result);
        if (result)
            data.resize(*result);
        return data;
    }
}  // namespace

TEST(AssetPack, LegacyFormatOffsets)
{
    std::ifstream input(std::string(NAU_PACK_FIXTURES) + "/legacy.nau_pack", std::ios::binary);
    std::string bytes((std::istreambuf_iterator<char>(input)), {});
    ASSERT_EQ(bytes.size(), 70000);
    auto index = nau::io::readAssetPackIndex(owned(bytes), bytes.size());
    ASSERT_TRUE(index);
    ASSERT_EQ(index->content.size(), 1);
    EXPECT_EQ(index->content.front().blobData.offset, 269);
    EXPECT_EQ(index->content.front().blobData.size, 926);
    auto fs = open(bytes);
    ASSERT_TRUE(fs);
    auto file = (*fs)->openFile("/materials/billboards.nmat_json", nau::io::AccessMode::Read, nau::io::OpenFileMode::OpenExisting);
    ASSERT_TRUE(file);
    nau::io::IStreamReader::Ptr stream = file->createStream();
    EXPECT_EQ(read(*stream, 1000), bytes.substr(269, 926));
}

TEST(AssetPack, ReadsSeeksAndLifetime)
{
    auto source = owned(valid());
    auto fs = nau::io::createAssetPackFileSystem(source, source->getBufferAsSpan().size());
    ASSERT_TRUE(fs);
    source = nullptr;
    auto file = (*fs)->openFile("nested/a", nau::io::AccessMode::Read, nau::io::OpenFileMode::OpenExisting);
    ASSERT_TRUE(file);
    nau::io::IStreamReader::Ptr first = file->createStream(), second = file->createStream();
    fs = nau::io::IFileSystem::Ptr{};
    file = nullptr;
    EXPECT_EQ(read(*first, 1), "a");
    EXPECT_EQ(read(*second, 3), "abc");
    EXPECT_EQ(read(*first, 20), "bc");
    EXPECT_EQ(read(*first, 1), "");
    EXPECT_EQ(first->setPosition(nau::io::OffsetOrigin::End, -2), 1);
    EXPECT_EQ(read(*first, 3), "bc");
    EXPECT_EQ(first->setPosition(nau::io::OffsetOrigin::Current, INT64_MIN), 0);
    EXPECT_EQ(first->setPosition(nau::io::OffsetOrigin::Begin, INT64_MAX), 3);
}

TEST(AssetPack, DirectoriesEmptyFilesAndWriteRejection)
{
    auto fs = open(valid());
    ASSERT_TRUE(fs);
    EXPECT_TRUE((*fs)->exists("nested", nau::io::FsEntryKind::Directory));
    EXPECT_FALSE((*fs)->exists("nested/a", nau::io::FsEntryKind::Directory));
    EXPECT_FALSE((*fs)->exists("absent"));
    auto directory = (*fs)->openDirIterator("nested");
    ASSERT_TRUE(directory);
    auto [cursor, item] = *directory;
    EXPECT_EQ(item.path.getString(), "nested/a");
    EXPECT_EQ((*fs)->incrementDirIterator(cursor).path.getString(), "nested/b");
    EXPECT_FALSE((*fs)->incrementDirIterator(cursor));
    (*fs)->closeDirIterator(cursor);
    auto file = (*fs)->openFile("empty", nau::io::AccessMode::Read, nau::io::OpenFileMode::OpenExisting);
    ASSERT_TRUE(file);
    EXPECT_EQ(file->getSize(), 0);
    nau::io::IStreamReader::Ptr stream = file->createStream();
    EXPECT_EQ(read(*stream, 1), "");
    EXPECT_FALSE(file->createStream(nau::io::AccessMode::Write));
    EXPECT_FALSE((*fs)->openFile("empty", nau::io::AccessMode::Write, nau::io::OpenFileMode::CreateAlways));
}

TEST(AssetPack, ConcurrentBackingReads)
{
    auto fs = open(valid());
    ASSERT_TRUE(fs);
    auto file = (*fs)->openFile("nested/a", nau::io::AccessMode::Read, nau::io::OpenFileMode::OpenExisting);
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i)
        threads.emplace_back([file]
        {
            nau::io::IStreamReader::Ptr stream = file->createStream();
            for (int j = 0; j < 100; ++j)
            {
                stream->setPosition(nau::io::OffsetOrigin::Begin, 0);
                EXPECT_EQ(read(*stream, 3), "abc");
            }
        });
    for (auto& thread : threads)
        thread.join();
}

TEST(AssetPack, InvalidRangesAndPaths)
{
    for (auto path : {"../a", "dir/../a", "C:/a", ""})
    {
        Json::Value list(Json::arrayValue);
        list.append(entry(path));
        EXPECT_FALSE(open(package(list))) << path;
    }
    for (auto offset : {UINT64_MAX, uint64_t(4294967296), uint64_t(7)})
    {
        Json::Value list(Json::arrayValue);
        list.append(entry("a", offset));
        EXPECT_FALSE(open(package(list)));
    }
    for (auto pair : {
             std::pair{    "a",     "/a"},
             std::pair{    "a",    "a/b"},
             std::pair{"dir/a", "dir\\a"}
    })
    {
        Json::Value list(Json::arrayValue);
        list.append(entry(pair.first));
        list.append(entry(pair.second));
        EXPECT_FALSE(open(package(list)));
    }
    Json::Value list(Json::arrayValue);
    auto item = entry("a");
    item["blobData"]["offset"] = -1;
    list.append(item);
    EXPECT_FALSE(open(package(list)));
    list[0]["blobData"]["offset"] = 1.5;
    EXPECT_FALSE(open(package(list)));
    list[0] = entry("a");
    list[0]["contentCompression"] = "gzip";
    EXPECT_FALSE(open(package(list)));
}

TEST(AssetPack, HeaderTruncationAndLimits)
{
    auto bytes = valid();
    for (size_t length : {size_t(0), size_t(20), bytes.size() - 1})
        EXPECT_FALSE(open(std::string_view(bytes).substr(0, length)));
    auto wrong = bytes;
    wrong.replace(wrong.find("nau-vfs-pack"), 12, "wrong-kind!!");
    EXPECT_FALSE(open(wrong));
    EXPECT_FALSE(open(bytes, {.maxHeaderBytes = 8}));
    EXPECT_FALSE(open(bytes, {.maxIndexBytes = 8}));
    EXPECT_FALSE(open(bytes, {.maxEntries = 1}));
    EXPECT_FALSE(open(bytes, {.maxPathBytes = 1}));
    auto badJson = bytes;
    badJson[badJson.find('{')] = '!';
    EXPECT_FALSE(open(badJson));
    auto version = bytes;
    version.replace(version.find("0.1"), 3, "9.9");
    EXPECT_FALSE(open(version));
}
