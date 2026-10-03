// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <string_view>
#include <vector>

#include "helpers/my_asset_view.h"
#include "nau/assets/asset_container.h"
#include "nau/assets/asset_content_provider.h"
#include "nau/assets/asset_descriptor_factory.h"
#include "nau/assets/asset_manager.h"
#include "nau/io/memory_stream.h"
#include "nau/test/helpers/app_guard.h"
#include "nau/test/helpers/assert_catcher_guard.h"

namespace nau::test
{
    namespace
    {
        class MyContainerLoader final : public IAssetContainerLoader
        {
            NAU_TYPEID(MyContainerLoader)
            NAU_CLASS_BASE(IAssetContainerLoader)

            std::vector<std::string_view> getSupportedAssetKind() const override
            {
                return {"test", "broken_asset"};
            }

            async::Task<IAssetContainer::Ptr> loadFromStream(io::IStreamReader::Ptr, AssetContentInfo info) override
            {
                if (info.kind == "broken_asset")
                {
                    co_return NauMakeError("Test failure");
                }

                co_return nullptr;
            }

            RuntimeReadonlyDictionary::Ptr getDefaultImportSettings() const override
            {
                return nullptr;
            }
        };

        /**
         */
        class MyContentProvider : public IAssetContentProvider
        {
            NAU_TYPEID(nau::test::MyContentProvider)
            NAU_CLASS_BASE(IAssetContentProvider)

            static constexpr std::string_view BrokenScheme{"broken_asset"};

            Result<AssetContent> openStreamOrContainer(const AssetPath& assetPath) override
            {
                using namespace nau::io;

                NAU_ASSERT(assetPath.hasScheme(BrokenScheme));
                if (!assetPath.hasScheme(BrokenScheme))
                {
                    return NauMakeError("Unsupported scheme");
                }

                auto stream = io::createMemoryStream();

                AssetContentInfo info = {.kind = "broken_asset"};

                return {std::move(stream), std::move(info)};
            }

            std::vector<std::string_view> getSupportedSchemes() const override
            {
                return {BrokenScheme};
            }
        };
    }  // namespace

    /**
     */
    class TestAssetView : public testing::Test
    {
        void SetUp() final
        {
            m_app.start();
        }

        void TearDown() final
        {
            m_app.stop();
        }

    protected:
        class MyTestApp final : public AppGuard
        {
        public:
            using AppGuard::AppGuard;

        private:
            void setupTestServices() override
            {
                registerServices<MyContainerLoader, MyContentProvider>();
            }
        };

        static IAssetDescriptorFactory& getAssetDescriptorFactory()
        {
            return getServiceProvider().get<IAssetDescriptorFactory>();
        }

        static IAssetManager& getAssetManager()
        {
            return getServiceProvider().get<IAssetManager>();
        }

        MyTestApp m_app;
    };

    /**
        Test: IAssetDescriptor::get
    */
    TEST_F(TestAssetView, ReturnsNullOnContainerLoadFailure)
    {
        auto asset = getAssetManager().openAsset(AssetPath{"broken_asset:/content/white_8x8.png"});

        nau::Ptr<> rawAsset = *asset->getRawAsset();
        ASSERT_FALSE(rawAsset);

        nau::Ptr<> assetView = *asset->getAssetViewTyped<MyAssetView>();
        ASSERT_FALSE(assetView);
    }

    /**
        Test:
            Attempts to load an invalid resource will not cause the application to crash (or assert an error),
            but will log an error (or at least a warning) and return nullptr.
     */
    TEST_F(TestAssetView, GettingInvalidAssetReturnsNullptr)
    {
        bool hasWarnOrError = false;
        auto subscription = diag::getLogger().subscribe([&hasWarnOrError](const diag::LoggerMessage& msg)
        {
            if (msg.level == diag::LogLevel::Error || msg.level == diag::LogLevel::Warning)
            {
                hasWarnOrError = true;
            }
        });

        IAssetDescriptor::Ptr asset = getAssetManager().openAsset(AssetPath{"file:/content/not_exists.png"});

        // currently openAsset will returns IAssetDescriptor::Ptr, but all subsequent calls to it must return nulls
        ASSERT_TRUE(asset);

        Ptr<> rawAsset = *asset->getRawAsset();
        ASSERT_FALSE(rawAsset);

        Ptr<> assetView = *asset->getAssetViewTyped<MyAssetView>();
        
        ASSERT_FALSE(assetView);
        ASSERT_TRUE(hasWarnOrError);
    }

}  // namespace nau::test
