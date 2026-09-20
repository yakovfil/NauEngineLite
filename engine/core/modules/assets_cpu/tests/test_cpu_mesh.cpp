// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include <gtest/gtest.h>

#include <cmath>
#include <cstring>
#include <fstream>

#include "asset_file_content_provider.h"
#include "asset_manager_impl.h"
#include "nau/assets/cpu_mesh.h"
#include "nau/diag/logging.h"
#include "nau/io/memory_stream.h"
#include "nau/io/virtual_file_system.h"
#include "nau/messaging/messaging.h"
#include "nau/service/service_provider.h"

namespace nau::test
{
    class ControlledView final : public IAssetView
    {
        NAU_CLASS_(nau::test::ControlledView, IAssetView)
    };

    class ControlledViewFactory final : public IAssetViewFactory,
                                        public IRefCounted
    {
        NAU_CLASS_(nau::test::ControlledViewFactory, IAssetViewFactory, IRefCounted)
    public:
        eastl::vector<const rtti::TypeInfo*> getAssetViewTypes() const override
        {
            return {&rtti::getTypeInfo<ControlledView>()};
        }
        async::Task<IAssetView::Ptr> createAssetView(nau::Ptr<>, const rtti::TypeInfo&) override
        {
            ++calls;
            pending = async::TaskSource<IAssetView::Ptr>{};
            return pending.getTask();
        }
        unsigned calls = 0;
        async::TaskSource<IAssetView::Ptr> pending = nullptr;
    };

    class CpuMeshAssets : public testing::Test
    {
    protected:
        std::vector<std::byte> read(const char* path)
        {
            std::ifstream input(path, std::ios::binary | std::ios::ate);
            if (!input)
                return {};
            const auto length = input.tellg();
            if (length <= 0)
                return {};
            std::vector<std::byte> bytes(static_cast<size_t>(length));
            input.seekg(0);
            input.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
            if (!input)
                return {};
            return bytes;
        }
        void SetUp() override
        {
            diag::setLogger(diag::createLogger());
            logSubscription = diag::getLogger().subscribe([this](const diag::LoggerMessage& message)
            {
                diagnostics.emplace_back(message.data.data(), message.data.size());
            });
            setDefaultServiceProvider(createServiceProvider());
            auto& services = getServiceProvider();
            services.addService(AsyncMessageSource::create());
            services.addService<AssetManagerImpl>();
            services.addService<AssetFileContentProvider>();
            services.addService<CpuMeshContainerLoader>();
            services.addService<CpuMeshViewFactory>();
            vfs = io::createVirtualFileSystem();
            services.addService(vfs);
            json = read("fixtures/cube.gltf");
            binary = read("fixtures/cube.bin");
            ASSERT_FALSE(json.empty());
            ASSERT_EQ(binary.size(), 1224);
        }
        void TearDown() override
        {
            vfs = nullptr;
            setDefaultServiceProvider(nullptr);
            logSubscription = nullptr;
            diag::setLogger(nullptr);
        }
        void mount(bool includeBinary = true)
        {
            std::vector<CpuAssetFile> files{
                {"cube.gltf", json}
            };
            if (includeBinary)
                files.push_back({"cube.bin", binary});
            auto fs = createCpuAssetFileSystem(std::move(files));
            ASSERT_TRUE(fs);
            ASSERT_TRUE(vfs->mount("/demo", *fs));
        }
        nau::Ptr<CpuMeshView> load()
        {
            auto descriptor = getServiceProvider().get<AssetManagerImpl>().openAsset(AssetPath("file:/demo/cube.gltf+[mesh/0]"));
            EXPECT_TRUE(descriptor);
            if (!descriptor)
                return nullptr;
            auto task = descriptor->getAssetViewTyped<CpuMeshView>();
            EXPECT_TRUE(task.isReady());
            if (!task.isReady())
                return nullptr;
            auto result = task.asResult();
            if (!result)
            {
                const auto message = result.getError()->getMessage();
                diagnostics.emplace_back(message.data(), message.size());
            }
            return result ? *result : nullptr;
        }
        void expectDiagnostic(std::string_view text)
        {
            EXPECT_TRUE(std::any_of(diagnostics.begin(), diagnostics.end(), [text](const std::string& message)
            {
                return message.find(text) != std::string::npos;
            })) << text;
        }
        void replace(std::string_view from, std::string_view to)
        {
            std::string text(reinterpret_cast<const char*>(json.data()), json.size());
            const auto pos = text.find(from);
            ASSERT_NE(pos, std::string::npos);
            text.replace(pos, from.size(), to);
            json.resize(text.size());
            std::memcpy(json.data(), text.data(), text.size());
        }
        std::vector<std::byte> json, binary;
        io::IVirtualFileSystem::Ptr vfs;
        std::vector<std::string> diagnostics;
        diag::Logger::SubscriptionHandle logSubscription;
    };

    TEST_F(CpuMeshAssets, LoadsRecordedGeometryThroughNauAssets)
    {
        mount();
        auto mesh = load();
        ASSERT_TRUE(mesh);
        ASSERT_EQ(mesh->positions.size(), 24);
        ASSERT_EQ(mesh->normals.size(), 24);
        ASSERT_EQ(mesh->indices.size(), 36);
        for (size_t i = 0; i < mesh->positions.size(); ++i)
        {
            float length = 0;
            for (unsigned axis = 0; axis < 3; ++axis)
            {
                EXPECT_FLOAT_EQ(std::abs(mesh->positions[i][axis]), 1);
                length += mesh->normals[i][axis] * mesh->normals[i][axis];
            }
            EXPECT_FLOAT_EQ(length, 1);
        }
        for (auto index : mesh->indices)
            EXPECT_LT(index, 24);
        auto again = load();
        EXPECT_EQ(again.get(), mesh.get());
        getServiceProvider().get<AssetManagerImpl>().removeAsset(AssetPath("file:/demo/cube.gltf"));
        EXPECT_EQ(mesh->positions.size(), 24);  // A held view owns its decoded CPU data.
    }

    TEST_F(CpuMeshAssets, FactoryFailureSettlesAllWaitersAndAllowsExplicitRetry)
    {
        mount();
        auto factory = rtti::createInstance<ControlledViewFactory>();
        getServiceProvider().addService(factory);
        auto& manager = getServiceProvider().get<AssetManagerImpl>();
        auto descriptor = manager.openAsset(AssetPath("file:/demo/cube.gltf+[mesh/0]"));
        ASSERT_TRUE(descriptor);
        auto first = descriptor->getAssetViewTyped<ControlledView>();
        auto second = descriptor->getAssetViewTyped<ControlledView>();
        EXPECT_FALSE(first.isReady());
        EXPECT_FALSE(second.isReady());
        EXPECT_EQ(factory->calls, 1);
        auto error = NauMakeError("delayed factory failure");
        ASSERT_TRUE(factory->pending.reject(error));
        ASSERT_TRUE(first.isReady());
        ASSERT_TRUE(second.isReady());
        auto firstResult = first.asResult();
        auto secondResult = second.asResult();
        ASSERT_FALSE(firstResult);
        ASSERT_FALSE(secondResult);
        EXPECT_EQ(firstResult.getError().get(), error.get());
        EXPECT_EQ(secondResult.getError().get(), error.get());

        auto retry = descriptor->getAssetViewTyped<ControlledView>();
        auto retryWaiter = descriptor->getAssetViewTyped<ControlledView>();
        EXPECT_EQ(factory->calls, 2);
        EXPECT_FALSE(retry.isReady());
        EXPECT_FALSE(retryWaiter.isReady());
        auto view = rtti::createInstance<ControlledView>();
        ASSERT_TRUE(factory->pending.resolve(view));
        ASSERT_TRUE(retry.isReady());
        ASSERT_TRUE(retryWaiter.isReady());
        auto retryResult = retry.asResult();
        auto waiterResult = retryWaiter.asResult();
        ASSERT_TRUE(retryResult);
        ASSERT_TRUE(waiterResult);
        EXPECT_EQ(retryResult->get(), view.get());
        EXPECT_EQ(waiterResult->get(), view.get());
        descriptor = nullptr;
        manager.removeAsset(AssetPath("file:/demo/cube.gltf"));
    }

    TEST_F(CpuMeshAssets, MissingBufferFails)
    {
        mount(false);
        EXPECT_FALSE(load());
        expectDiagnostic("/demo/cube.bin");
    }
    TEST_F(CpuMeshAssets, TruncatedBufferFails)
    {
        binary.pop_back();
        mount();
        EXPECT_FALSE(load());
        expectDiagnostic("wrong-sized buffer /demo/cube.bin");
    }
    TEST_F(CpuMeshAssets, MalformedJsonFails)
    {
        json = {std::byte{'{'}};
        mount();
        EXPECT_FALSE(load());
        expectDiagnostic("CPU mesh /demo/cube.gltf:");
    }
    TEST_F(CpuMeshAssets, OutOfRangeAccessorFails)
    {
        replace("\"POSITION\": 0", "\"POSITION\": 999");
        mount();
        EXPECT_FALSE(load());
        expectDiagnostic("invalid position, normal or index accessor");
    }
    TEST_F(CpuMeshAssets, UnsupportedStrideFails)
    {
        replace("\"byteOffset\": 0", "\"byteOffset\": 0, \"byteStride\": 12");
        mount();
        EXPECT_FALSE(load());
        expectDiagnostic("invalid buffer view range or stride");
    }
    TEST_F(CpuMeshAssets, InvalidIndexFails)
    {
        binary[1152] = std::byte{255};
        binary[1153] = std::byte{255};
        mount();
        EXPECT_FALSE(load());
        expectDiagnostic("vertex index out of range");
    }
    TEST_F(CpuMeshAssets, NonFinitePositionFails)
    {
        const float invalid = std::numeric_limits<float>::infinity();
        std::memcpy(binary.data(), &invalid, sizeof(invalid));
        mount();
        EXPECT_FALSE(load());
        expectDiagnostic("non-finite position or normal");
    }
    TEST_F(CpuMeshAssets, FileSystemRejectsWritesAndOwnsBytes)
    {
        mount();
        binary.clear();
        EXPECT_FALSE(vfs->openFile("/demo/cube.bin", io::AccessMode::Write, io::OpenFileMode::OpenExisting));
        auto file = vfs->openFile("/demo/cube.bin", io::AccessMode::Read, io::OpenFileMode::OpenExisting);
        ASSERT_TRUE(file);
        EXPECT_EQ(file->getSize(), 1224);
        EXPECT_EQ(file->getPath(), io::FsPath("/demo/cube.bin"));
        EXPECT_FALSE(createCpuAssetFileSystem({
            {"../cube.bin", {}}
        }));
    }
}  // namespace nau::test
