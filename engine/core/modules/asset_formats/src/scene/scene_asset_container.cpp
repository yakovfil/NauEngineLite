// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "scene_asset_container.h"

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "nau/memory/stack_allocator.h"
#include "nau/memory/standard_aliases.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/rtti/weak_ptr.h"
#include "scene_serialization.h"

namespace nau
{
    // TODO: check what restrictions there are for the SceneAssetContainer to be also SceneAsset.
    class SceneDefaultAsset final : public SceneAsset
    {
        NAU_CLASS_(nau::SceneDefaultAsset, SceneAsset)

    public:
        SceneDefaultAsset(SceneAssetContainer& container) :
            m_container(nau::Ptr{&container})
        {
        }

    private:
        SceneAssetInfo getSceneInfo() const override
        {
            const auto container = m_container.lock();
            NAU_ASSERT(container, "Access to asset whose container is invalid");
            if (!container)
            {
                return {};
            }

            const auto& header = container->getSceneHeader();

            SceneAssetInfo info{
                .assetKind = header.assetKind,
                .name = header.name};

            return info;
        }

        std::optional<Vector<ReferenceField>> getReferencesInfo() const override
        {
            const auto container = m_container.lock();
            NAU_ASSERT(container, "Access to asset whose container is invalid");
            if (!container)
            {
                return std::nullopt;
            }

            const auto& header = container->getSceneHeader();
            return header.referencesInfo;
        }

        void visitScene(ISceneAssetVisitor& visitor) const override
        {
            auto container = m_container.lock();
            NAU_ASSERT(container, "Access to asset whose container is invalid");
            if (!container)
            {
                return;
            }

            const auto& allObjects = container->getSceneObjects();
            const auto& header = container->getSceneHeader();

            for (const unsigned objectId : header.topLevelObjectIds)
            {
                if (auto object = allObjects.find(objectId); object != allObjects.end())
                {
                    visitSceneObjectRecursive(allObjects, nullptr, object->second, visitor);
                }
            }
        }

        void visitSceneObjectRecursive(const SceneAssetContainer::ObjectsMap& allObjects, const SceneObjectAsset* parentObject, const SerializedSceneObject& object, ISceneAssetVisitor& visitor) const
        {
            const Uid parentUid = parentObject ? parentObject->uid : (object.isSceneRoot ? SceneObjectAsset::SceneVirtualRootUid : NullUid);

            visitor.visitSceneObject(parentUid, object);

            for (const auto& component : object.additionalComponents)
            {
                visitor.visitSceneComponent(object.uid, component);
            }

            for (const unsigned childId : object.childLocalIds)
            {
                auto child = allObjects.find(childId);
                if (child != allObjects.end())
                {
                    const SerializedSceneObject& childObject = child->second;
                    visitSceneObjectRecursive(allObjects, &object, childObject, visitor);
                }
                else
                {
                    NAU_LOG_WARNING("Invalid child object id:({})", childId);
                }
            }
        }

        mutable WeakPtr<SceneAssetContainer> m_container;
    };

    SceneAssetContainer::SceneAssetContainer(SceneHeader&& sceneHeader, SceneAssetContainer::ObjectsMap&& sceneObjects) :
        m_sceneHeader(std::move(sceneHeader)),
        m_sceneObjects(std::move(sceneObjects))
    {
    }

    const SceneAssetContainer::ObjectsMap& SceneAssetContainer::getSceneObjects() const
    {
        return m_sceneObjects;
    }

    const SceneHeader& SceneAssetContainer::getSceneHeader() const
    {
        return m_sceneHeader;
    }

    nau::Ptr<> SceneAssetContainer::getAsset([[maybe_unused]] std::string_view path)
    {
        NAU_ASSERT(path.empty(), "Currently only default asset is valid");
        if (!path.empty())
        {
            return nullptr;
        }

        return rtti::createInstance<SceneDefaultAsset>(*this);
    }

    std::vector<std::string> SceneAssetContainer::getContent() const
    {
        return {
            std::string{}};
    }

    std::vector<std::string_view> SceneAssetLoader::getSupportedAssetKind() const
    {
        return {"scene/nscene", "nscene", "nprefab", "scene/prefab"};
    }

    async::Task<IAssetContainer::Ptr> SceneAssetLoader::loadFromStream(io::IStreamReader::Ptr stream, AssetContentInfo info)
    {
        using namespace nau::async;
        using ObjectsMap = SceneAssetContainer::ObjectsMap;

        NAU_FATAL(stream);

        const auto objectsProducer = [](ReadOnlyBuffer buffer) -> Task<ObjectsMap>
        {
            co_await Executor::getDefault();

            io::IStreamReader::Ptr stream = io::createReadonlyMemoryStream(std::span{buffer.data(), buffer.size()});

            // TODO: replace by StackVector, when allocators will support proper alignment
            std::vector<SerializedSceneObject> objects;
            RuntimeValue::Ptr value = *serialization::jsonParse(stream->as<io::IStreamReader&>());
            runtimeValueApply(objects, value).ignore();

            ObjectsMap result;
            for (SerializedSceneObject& object : objects)
            {
                [[maybe_unused]] auto [iter, emplaceOk] = result.emplace(object.localId, std::move(object));
                NAU_ASSERT(emplaceOk);
            }

            co_return result;
        };

        auto [headerValue, dataOffset] = *io::readContainerHeader(stream);
        SceneHeader header;
        runtimeValueApply(header, headerValue).ignore();

        Vector<Task<ObjectsMap>> tasks;
        tasks.reserve(header.objects.size());

        for (const ObjectsBlockInfo& blockInfo : header.objects)
        {
            stream->setPosition(io::OffsetOrigin::Begin, blockInfo.offset + dataOffset);

            BytesBuffer buffer{blockInfo.size};

            auto readResult = stream->read(buffer.data(), buffer.size());
            NAU_ASSERT(readResult);
            NAU_ASSERT(*readResult == buffer.size());

            tasks.emplace_back(objectsProducer(buffer.toReadOnly()));
        }

        co_await whenAll(tasks);

        ObjectsMap allSceneObjects;

        for (Task<ObjectsMap>& result : tasks)
        {
            allSceneObjects.merge(*std::move(result));
        }

        co_return rtti::createInstance<SceneAssetContainer>(std::move(header), std::move(allSceneObjects));
    }

    RuntimeReadonlyDictionary::Ptr SceneAssetLoader::getDefaultImportSettings() const
    {
        return nullptr;
    }

}  // namespace nau