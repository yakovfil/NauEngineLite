// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <string>
#include <string_view>
#include <vector>

#include "helpers/my_asset_view.h"
#include "nau/assets/asset_container.h"
#include "nau/assets/asset_content_provider.h"
#include "nau/assets/asset_descriptor_factory.h"
#include "nau/assets/asset_manager.h"
#include "nau/test/helpers/app_guard.h"

namespace nau::test
{
    namespace
    {
        /**
         */
        class TestAssetContainer final : public IAssetContainer
        {
            NAU_CLASS_(TestAssetContainer, IAssetContainer)

            nau::Ptr<> getAsset(std::string_view path) override
            {
                return rtti::createInstance<MyAssetView>(std::string(path));
            }

            std::vector<std::string> getContent() const override
            {
                return {};
            }
        };
    }  // namespace

    /**
     */
    class TestDescriptorFactory : public testing::Test
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
        static IAssetDescriptorFactory& getAssetDescriptorFactory()
        {
            return getServiceProvider().get<IAssetDescriptorFactory>();
        }

        static IAssetManager& getAssetManager()
        {
            return getServiceProvider().get<IAssetManager>();
        }

        AppGuard m_app;
    };

    /**
        Test: adding custom asset container.
            - manager must resolve custom asset path
    */
    TEST_F(TestDescriptorFactory, AddAssetContainer)
    {
        IAssetContainer::Ptr myContainer = rtti::createInstance<TestAssetContainer>();

        getAssetDescriptorFactory().addAssetContainer(AssetPath{"test:my_container1"}, myContainer);
        IAssetDescriptor::Ptr asset = getAssetManager().openAsset(AssetPath{"test:my_container1+[test]"});
        ASSERT_TRUE(asset);

        async::Task<nau::Ptr<MyAssetView>> assetView = asset->getAssetViewTyped<MyAssetView>();
        ASSERT_TRUE(assetView.isReady());
        ASSERT_EQ((*assetView)->getData(), "test");
    }

    /**
          Test: removing previously added custom asset container.
      */
    TEST_F(TestDescriptorFactory, RemoveAssetContainer)
    {
        const std::string_view AssetContainerPath = "test:my_container1";

        IAssetContainer::Ptr myContainer = rtti::createInstance<TestAssetContainer>();
        {
            getAssetDescriptorFactory().addAssetContainer(AssetContainerPath, myContainer);
            IAssetDescriptor::Ptr asset = getAssetManager().openAsset(AssetPath{"test:my_container1+[test]"});
            ASSERT_TRUE(asset);
        }

        getAssetDescriptorFactory().removeAssetContainer(AssetContainerPath);
        IAssetDescriptor::Ptr asset = getAssetManager().openAsset(AssetPath{"test:my_container1+[test]"});
        ASSERT_FALSE(asset);
    }

}  // namespace nau::test
