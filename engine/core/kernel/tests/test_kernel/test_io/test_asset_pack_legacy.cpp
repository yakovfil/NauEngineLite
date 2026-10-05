// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include <gtest/gtest.h>

#include <fstream>
#include <thread>

#include "nau/io/asset_pack_file_system.h"
#include "nau/io/memory_stream.h"
#include "nau/io/nau_container.h"
#include "nau/runtime/internal/runtime_state.h"

TEST(AssetPackLegacy, ExistingWindowsReader)
{
    const std::string path = std::string(NAU_PACK_FIXTURES) + "/legacy.nau_pack";
    auto runtime = nau::RuntimeState::create();
    auto fs = nau::io::createAssetPackFileSystem(nau::strings::toU8StringView(path), {.lifetimeOfCache = std::chrono::seconds(0)});
    ASSERT_TRUE(fs);
    auto file = fs->openFile("/materials/billboards.nmat_json", nau::io::AccessMode::Read, nau::io::OpenFileMode::OpenExisting);
    ASSERT_TRUE(file);
    EXPECT_EQ(file->getSize(), 926);
    nau::io::IStreamReader::Ptr stream = file->createStream();
    std::string bytes(926, '\0');
    auto result = stream->read(reinterpret_cast<std::byte*>(bytes.data()), bytes.size());
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, bytes.size());
    std::ifstream input(std::string(NAU_PACK_FIXTURES) + "/legacy_payload.txt", std::ios::binary);
    std::string expected((std::istreambuf_iterator<char>(input)), {});
    EXPECT_EQ(bytes, expected);
    stream = nullptr;
    file = nullptr;
    fs = nullptr;
    auto shutdown = runtime->shutdown();
    while (shutdown())
        std::this_thread::yield();
}
