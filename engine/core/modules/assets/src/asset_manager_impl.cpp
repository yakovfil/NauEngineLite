// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "./asset_manager_impl.h"

#include "nau/assets/import_settings_provider.h"
#include "nau/async/multi_task_source.h"
#include "nau/diag/error.h"
#include "nau/memory/mem_allocator.h"
#include "nau/service/service_provider.h"
#include "nau/string/string_conv.h"

namespace nau
{
    AssetManagerImpl& AssetManagerImpl::getInstance()
    {
        return getServiceProvider().get<AssetManagerImpl>();
    }

    IAssetDescriptor::Ptr AssetManagerImpl::openAsset(const AssetPath& assetPath)
    {
        using namespace nau::async;
        using namespace nau::io;

        // TODO: need to refactor resolveAssetContent and logic below, to using mutex with shared lock (which can be used in most cases).
        lock_(m_mutex);

        // first must to find existing container.
        // this is required because container can be registered through AssetDescriptorFactory::addAssetContainer,
        // in this case resolveAssetContent won't be able to find/resolve asset path (because there is actually no scheme resolvers)
        // auto [assetPathOnly, innerPath] = AssetQuery::cutAssetPath(assetPath);
        if (auto iter = m_assets.find_as(assetPath.getSchemeAndContainerPath()); iter != m_assets.end())
        {
            auto& asset = iter->second;
            return asset->getInnerAsset(assetPath.getAssetInnerPath());
        }

        const ResolvedContentData resolvedContent = resolveAssetContent(assetPath);
        if (!resolvedContent)
        {
            NAU_LOG_WARNING("Can not resolve asset content: ({})", assetPath.toString());
            return nullptr;
        }

        // ATTENTION: m_assets keeps asset container by resolved path.
        // That must guarantee that we can obtain single asset accesed by differrent paths
        auto iter = m_assets.find_as(resolvedContent.assetPath.getSchemeAndContainerPath());
        if (iter == m_assets.end())
        {
            AssetDescriptorImpl::ContainerLoaderFunc loaderFunc = [this, resolvedContent]() -> Task<IAssetContainer::Ptr>
            {
                const auto& [contentProvider, assetFilePath, incomingContentInfo] = resolvedContent;
                const Result<IAssetContentProvider::AssetContent> contentResult = contentProvider->openStreamOrContainer(assetFilePath);

                if (!contentResult)
                {
                    co_return contentResult.getError();
                }

                const auto& [content, contentInfo] = *contentResult;


                // priority is given to content_info that came from the path resolver,
                // since it potentially has more information about the asset (for example assetdb).
                const AssetContentInfo& actualContentInfo = incomingContentInfo ? incomingContentInfo : contentInfo;
                NAU_ASSERT(actualContentInfo, "Content Info not resolved");

                if (IAssetContainer* const container = content->as<IAssetContainer*>(); container)
                {
                    co_return container;
                }
                NAU_ASSERT(content->is<io::IStreamReader>());

                auto stream = content->as<io::IStreamReader*>();
                if (!stream)
                {
                    co_return NauMakeError("Unexpected content type");
                }

                IAssetContainerLoader* const loader = findContainerLoader(actualContentInfo.kind);
                if (!loader)
                {
                    // This may be a case where the content provider returned more up-to-date information about the asset ?
                    // And maybe it's worth trying to request a container_loader again?
                    co_return NauMakeError("Unsupported content kind: ({})", actualContentInfo.kind.c_str());
                }

                Result<IAssetContainer::Ptr> container = co_await loader->loadFromStream(stream, actualContentInfo).doTry();
                if (!container)
                {
                    NAU_LOG_WARNING("Fail to load asset container. Asset kind: ({}), asset filepath: ({}):({})", actualContentInfo.kind.c_str(), assetFilePath.toString(), container.getError()->getMessage());
                }

                co_return container;
            };

            // There is a slight drawback here:
            // if we receive an asset created earlier by a different source path (but which was resolved to a different path), then since we are reusing the asset,
            // the client will receive an asset whose path differs from the one it was requested by (but it is still the same asset).
            nau::Ptr<AssetDescriptorImpl> newAsset = rtti::createInstance<AssetDescriptorImpl>(assetPath.getSchemeAndContainerPath(), std::move(loaderFunc));

            [[maybe_unused]] bool emplaceOk = false;
            eastl::tie(iter, emplaceOk) = m_assets.emplace(resolvedContent.assetPath.getSchemeAndContainerPath(), std::move(newAsset));
            NAU_ASSERT(emplaceOk);
        }

        const nau::Ptr<AssetDescriptorImpl>& asset = iter->second;
        return asset->getInnerAsset(resolvedContent.assetPath.getAssetInnerPath());
    }

    IAssetDescriptor::Ptr AssetManagerImpl::preLoadAsset(const AssetPath& assetPath)
    {
        {
            shared_lock_(m_mutex);

            // first must to find existing container.
            // this is required because container can be registered through AssetDescriptorFactory::addAssetContainer,
            // in this case resolveAssetContent won't be able to find/resolve asset path (because there is actually no scheme resolvers)
            // auto [assetPathOnly, innerPath] = AssetQuery::cutAssetPath(assetPath);
            if (auto iter = m_assets.find_as(assetPath.getSchemeAndContainerPath()); iter != m_assets.end())
            {
                auto& asset = iter->second;
                return asset->getInnerAsset(assetPath.getAssetInnerPath());
            }
        }

        AssetDescriptorImpl::ContainerLoaderFunc loaderFunc = [this, assetPath]() -> async::Task<IAssetContainer::Ptr>
        {
            ResolvedContentData resolvedContent;
            {
                lock_(m_mutex);

                resolvedContent = resolveAssetContent(assetPath);
                if (!resolvedContent)
                {
                    co_return NauMakeError("Can not resolve asset content: ({})", assetPath.toString());
                }
            }

            const auto& [contentProvider, assetFilePath, incomingContentInfo] = resolvedContent;
            const Result<IAssetContentProvider::AssetContent> contentResult = contentProvider->openStreamOrContainer(assetFilePath);

            if (!contentResult)
            {
                co_return contentResult.getError();
            }

            const auto& [content, contentInfo] = *contentResult;

            // priority is given to content_info that came from the path resolver,
            // since it potentially has more information about the asset (for example assetdb).
            const AssetContentInfo& actualContentInfo = incomingContentInfo ? incomingContentInfo : contentInfo;
            NAU_ASSERT(actualContentInfo, "Content Info not resolved");

            if (IAssetContainer* const container = content->as<IAssetContainer*>(); container)
            {
                co_return container;
            }
            NAU_ASSERT(content->is<io::IStreamReader>());

            auto stream = content->as<io::IStreamReader*>();
            if (!stream)
            {
                co_return NauMakeError("Unexpected content type");
            }

            IAssetContainerLoader* const loader = findContainerLoader(actualContentInfo.kind);
            if (!loader)
            {
                // This may be a case where the content provider returned more up-to-date information about the asset ?
                // And maybe it's worth trying to request a container_loader again?
                co_return NauMakeError("Unsupported content kind: ({})", actualContentInfo.kind.c_str());
            }

            Result<IAssetContainer::Ptr> container = co_await loader->loadFromStream(stream, actualContentInfo).doTry();
            if (!container)
            {
                NAU_LOG_WARNING("Fail to load asset container. Asset kind: ({}), asset filepath: ({}):({})", actualContentInfo.kind.c_str(), assetFilePath.toString(), container.getError()->getMessage());
            }

            co_return container;
        };

        return rtti::createInstance<AssetDescriptorImpl>(assetPath, std::move(loaderFunc));
    }

    nau::IAssetDescriptor::Ptr AssetManagerImpl::findAsset(const AssetPath& assetPath)
    {
        shared_lock_(m_mutex);
        // TODO: wrong code. Must search by resolved path
        auto it = m_assets.find_as(assetPath);
        if (it != m_assets.end())
        {
            return it->second;
        }
        return nullptr;
    }

    IAssetDescriptor::Ptr AssetManagerImpl::findAssetById(IAssetDescriptor::AssetId assetId)
    {
        shared_lock_(m_mutex);

        for (auto& [key, asset] : m_assets)
        {
            if (asset->getAssetId() == assetId)
            {
                return asset;
            }
        }

        return nullptr;
    }

    void AssetManagerImpl::removeAsset(const AssetPath& assetPath)
    {
        lock_(m_mutex);

        // TODO: wrong code. Must search by resolved path
        auto it = m_assets.find(assetPath);
        if (it != m_assets.end())
        {
            it->second->unload();
            m_assets.erase(assetPath);
        }
    }

    void AssetManagerImpl::unload(UnloadAssets flag)
    {
    }

    Result<AssetPath> AssetManagerImpl::resolvePath(const AssetPath& assetPath)
    {
        ResolvedContentData contentData = resolveAssetContent(assetPath);
        if (!contentData)
        {
            return NauMakeError("Can not resolve path:({})", assetPath.toString());
        }

        return AssetPath{std::move(contentData.assetPath)};
    }

    IAssetDescriptor::Ptr AssetManagerImpl::createAssetDescriptor(IAssetContainer& container, eastl::string_view assetInnerPath)
    {
        lock_(m_mutex);

        for (const auto& [_, asset] : m_assets)
        {
            if (asset->getLoadedContainer() == &container)
            {
                return asset->getInnerAsset(assetInnerPath);
            }
        }

        NAU_FAILURE("Requesting Asset Descriptor from container that is not loaded yet.");

        return nullptr;
    }

    void AssetManagerImpl::addAssetContainer(const AssetPath& assetPath, IAssetContainer::Ptr container)
    {
        using namespace nau::async;

        NAU_ASSERT(container);
        if (!container)
        {
            return;
        }

        lock_(m_mutex);

        const bool containerExists = m_assets.find_as(assetPath) != m_assets.end();
        NAU_ASSERT(!containerExists, "Container already exists:({})", assetPath.toString());
        if (containerExists)
        {
            return;
        }

        AssetDescriptorImpl::ContainerLoaderFunc loaderFunc = [container = std::move(container), path = assetPath]() -> Task<IAssetContainer::Ptr>
        {
            return Task<IAssetContainer::Ptr>::makeResolved(container);
        };

        auto newAsset = rtti::createInstance<AssetDescriptorImpl>(assetPath, std::move(loaderFunc));
        [[maybe_unused]] auto [iter, emplaceOk] = m_assets.emplace(assetPath, std::move(newAsset));
        NAU_ASSERT(emplaceOk);

        // Forcefully initialize container, so that it can be immediately available.
        // (for example to using with createAssetDescriptor).
        iter->second->load();
    }

    void AssetManagerImpl::removeAssetContainer(const AssetPath& assetPath)
    {
        lock_(m_mutex);

        auto iter = m_assets.find_as(assetPath);
        NAU_ASSERT(iter != m_assets.end(), "Container doesn't exists:({})", assetPath.toString());
        if (iter == m_assets.end())
        {
            return;
        }

        iter->second->unload();
        m_assets.erase(iter);
    }

    IAssetContainerLoader* AssetManagerImpl::findContainerLoader(const eastl::string& kind)
    {
        // BE AWARE: findFileContainerLoader requires that m_mutex is locked
        // prior findFileContainerLoader is called
        if (m_containerLoaders.empty())
        {
            for (IAssetContainerLoader* const loader : getServiceProvider().getAll<IAssetContainerLoader>())
            {
                eastl::vector<eastl::string_view> supportedAssetKinds = loader->getSupportedAssetKind();
                for (auto assetKind : supportedAssetKinds)
                {
                    m_containerLoaders.emplace(eastl::string(assetKind), loader);
                }
            }
        }

        // TODO: replace by find_as()
        auto loaderEntry = m_containerLoaders.find(kind);
        if (loaderEntry != m_containerLoaders.end())
        {
            return loaderEntry->second;
        }

        const auto& [key, value] = strings::cut(kind, '/');
        loaderEntry = m_containerLoaders.find(eastl::string(key));
        if (loaderEntry != m_containerLoaders.end())
        {
            return loaderEntry->second;
        }

        return nullptr;
    }

    AssetManagerImpl::ResolvedContentData AssetManagerImpl::resolveAssetContent(const AssetPath& assetPath)
    {
        using namespace nau::strings;

        if (m_schemeHandlers.empty())
        {
            for (IAssetContentProvider* const contentProvider : getServiceProvider().getAll<IAssetContentProvider>())
            {
                eastl::vector<eastl::string_view> supportedAssetSchemes = contentProvider->getSupportedSchemes();
                for (auto assetScheme : supportedAssetSchemes)
                {
                    [[maybe_unused]] auto [iter, emplaceOk] = m_schemeHandlers.emplace(assetScheme, SchemeHandler{contentProvider});
                    NAU_ASSERT(emplaceOk, "Scheme handler duplication ({})", assetScheme);
                }
            }

            for (IAssetPathResolver* const pathResolver : getServiceProvider().getAll<IAssetPathResolver>())
            {
                eastl::vector<eastl::string_view> supportedAssetSchemes = pathResolver->getSupportedSchemes();
                for (auto assetScheme : supportedAssetSchemes)
                {
                    [[maybe_unused]] auto [iter, emplaceOk] = m_schemeHandlers.emplace(assetScheme, SchemeHandler{pathResolver});
                    NAU_ASSERT(emplaceOk, "Scheme handler duplication ({})", assetScheme);
                }
            }
        }

        AssetPath resolvedAssetPath = assetPath;
        IAssetContentProvider* contentProvider = nullptr;
        AssetContentInfo contentInfo;

        do
        {
            auto iter = m_schemeHandlers.find_as(resolvedAssetPath.getScheme());
            if (iter == m_schemeHandlers.end())
            {
                NAU_LOG_ERROR("Can not resolve scheme ({}) handler", resolvedAssetPath.getScheme());
                return {};
            }

            const SchemeHandler& handler = iter->second;
            if (eastl::holds_alternative<IAssetPathResolver*>(handler))
            {
                IAssetPathResolver* const pathResolver = eastl::get<IAssetPathResolver*>(handler);
                NAU_FATAL(pathResolver);

                AssetPath nextAssetPath;
                eastl::tie(nextAssetPath, contentInfo) = pathResolver->resolvePath(resolvedAssetPath);

                if (!nextAssetPath)
                {
                    NAU_LOG_ERROR("Can not resolve path ({})", resolvedAssetPath.toString());
                    return {};
                }

                resolvedAssetPath = std::move(nextAssetPath);
            }
            else
            {
                NAU_FATAL(eastl::holds_alternative<IAssetContentProvider*>(handler));
                contentProvider = eastl::get<IAssetContentProvider*>(handler);

                NAU_FATAL(contentProvider);
            }
        } while (contentProvider == nullptr);

        return ResolvedContentData{
            .contentProvider = contentProvider,
            .assetPath = std::move(resolvedAssetPath),
            .contentInfo = std::move(contentInfo)};
    }

    IAssetViewFactory* AssetManagerImpl::findAssetViewFactory(const rtti::TypeInfo& viewType)
    {
        {
            lock_(m_mutex);
            if (m_assetViewFactories.empty())
            {
                for (auto* const factory : getServiceProvider().getAll<IAssetViewFactory>())
                {
                    auto assetViewTypes = factory->getAssetViewTypes();
                    for (const auto type : assetViewTypes)
                    {
                        m_assetViewFactories.emplace(*type, factory);
                    }
                }
            }
        }

        const auto factoryEntry = m_assetViewFactories.find(viewType);
        return factoryEntry != m_assetViewFactories.end() ? factoryEntry->second : nullptr;
    }

    const eastl::vector<IAssetListener*>& AssetManagerImpl::getAssetListeners()
    {
        {
            shared_lock_(m_mutex);
            if (!m_assetListeners.empty())
            {
                return (m_assetListeners);
            }
        }

        lock_(m_mutex);
        if (m_assetListeners.empty())
        {
            m_assetListeners = getServiceProvider().getAll<IAssetListener>();
        }

        return (m_assetListeners);
    }

    IAssetDescriptor::AssetId AssetManagerImpl::getNextAssetId()
    {
        const auto id = m_nextAssetId.fetch_add(1ULL, std::memory_order_relaxed);
        return id;
    }

    async::Task<> AssetManagerImpl::updateAssetView(IAssetDescriptor::AssetId assetId, const rtti::TypeInfo& viewType, IAssetView::Ptr oldAssetView, IAssetView::Ptr newAssetView)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        NAU_ASSERT(oldAssetView);
        NAU_ASSERT(newAssetView);
        NAU_ASSERT(assetId > 0);

        if (!oldAssetView || !newAssetView || assetId == 0)
        {
            NAU_LOG_ERROR("Invalid data for updateAssetView()");
            co_return;
        }

        ASYNC_SWITCH_EXECUTOR(Executor::getDefault())

        auto& listeners = getAssetListeners();
        Vector<Task<>> updateTasks;

        for (size_t i = 0, size = listeners.size(); i < size; ++i)
        {
            IAssetListener* const listener = listeners[i];
            // clang-format off
            // In case of the last listener will 'move' asset view ptrs to the handler.
            // This also solve the problem when asset view management must be completely transferred to the handler side (if necessary).
            Task<> task = i == (size - 1) ?
                listener->onAssetViewUpdate(assetId, std::move(oldAssetView), std::move(newAssetView)) :
                listener->onAssetViewUpdate(assetId, oldAssetView, newAssetView);
            // clang-format on

            if (task && !task.isReady())
            {
                updateTasks.emplace_back(std::move(task));
            }
        }

        co_await whenAll(updateTasks);
    }

}  // namespace nau
