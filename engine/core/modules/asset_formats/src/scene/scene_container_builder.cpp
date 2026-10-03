// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "scene_container_builder.h"

#include <optional>
#include <unordered_map>

#include "nau/app/global_properties.h"
#include "nau/assets/scene_asset.h"
#include "nau/io/memory_stream.h"
#include "nau/io/nau_container.h"
#include "nau/serialization/json.h"
#include "scene_serialization.h"

namespace nau
{
    namespace
    {
        class SceneBuilderState : public ISceneAssetVisitor
        {
        public:
            SceneBuilderState() = default;

            const std::unordered_map<Uid, SerializedSceneObject>& getAllObjects() const
            {
                return m_allObjects;
            }

            const std::optional<SerializedSceneObject>& getRootObject() const
            {
                return m_rootObject;
            }

        private:
            bool visitSceneObject(Uid parentObjectUid, const SceneObjectAsset& objectData) override
            {
                [[maybe_unused]] auto [iter, emplaceOk] = m_allObjects.emplace(objectData.uid, SerializedSceneObject{});
                NAU_ASSERT(emplaceOk);
                NAU_ASSERT(parentObjectUid == NullUid || parentObjectUid == SceneObjectAsset::SceneVirtualRootUid || m_allObjects.contains(parentObjectUid));

                SerializedSceneObject& obj = iter->second;
                static_cast<SceneObjectAsset&>(obj) = objectData;
                obj.localId = static_cast<unsigned>(m_allObjects.size());

                if (parentObjectUid != NullUid && parentObjectUid != SceneObjectAsset::SceneVirtualRootUid)
                {
                    auto& parentObj = m_allObjects[parentObjectUid];
                    parentObj.childLocalIds.push_back(obj.localId);
                    obj.parentLocalId = parentObj.localId;
                }
                else
                {
                    obj.parentLocalId = 0;
                    obj.isSceneRoot = parentObjectUid == SceneObjectAsset::SceneVirtualRootUid;
#ifdef NAU_ASSERT_ENABLED
                    if (obj.isSceneRoot)
                    {
                        NAU_ASSERT(m_rootObjectId == 0, "Single root expected");
                        m_rootObjectId = obj.localId;
                    }
#endif
                }

                return true;
            }

            bool visitSceneComponent(Uid parentObjectUid, const ComponentAsset& component) override
            {
                NAU_ASSERT(m_allObjects.contains(parentObjectUid));
                m_allObjects[parentObjectUid].additionalComponents.push_back(component);

                return true;
            }

            std::optional<SerializedSceneObject> m_rootObject;
            std::unordered_map<Uid, SerializedSceneObject> m_allObjects;
            [[maybe_unused]] unsigned m_rootObjectId = 0;
        };
    }  // namespace

    bool SceneContainerBuilder::isAcceptable(nau::Ptr<> asset) const
    {
        return asset && asset->is<SceneAsset>();
    }

    Result<> SceneContainerBuilder::writeAssetToStream(io::IStreamWriter::Ptr stream, nau::Ptr<> asset)
    {
        using namespace nau::io;

        const bool prettyWrite = true;

        NAU_FATAL(asset);

        const SceneAsset& sceneAsset = asset->as<SceneAsset&>();
        const SceneAssetInfo sceneInfo = sceneAsset.getSceneInfo();

        SceneBuilderState visitor;
        sceneAsset.visitScene(visitor);

        SceneHeader header;
        header.assetKind = sceneInfo.assetKind;
        header.name = sceneInfo.name;
        header.version = "1.0.0";
        header.referencesInfo = sceneAsset.getReferencesInfo();

        // Objects inside the container are organized into batches for greater convenience in organizing parallel loading.
        // The number of batches (as well as their size) is choose based on the potential number of simultaneously running workers.
        constexpr size_t WorkerCount = 6;
        constexpr size_t MinBatchSize = 20;
        const size_t ObjectsBatchSize = std::max((visitor.getAllObjects().size() / WorkerCount), MinBatchSize);

        IMemoryStream::Ptr contentStream = createMemoryStream(AccessMode::Write | AccessMode::Read);
        size_t batchCounter = 0;
        size_t offset = contentStream->getPosition();

        Json::Value objects{Json::arrayValue};

        const auto writeObjectsToStream = [&]() -> size_t
        {
            serialization::jsonWrite(*contentStream, objects, serialization::JsonSettings{.pretty = prettyWrite}).ignore();
            objects.clear();

            const size_t currentOffset = contentStream->getPosition();
            header.objects.emplace_back(ObjectsBlockInfo{.offset = offset, .size = currentOffset - offset});
            return currentOffset;
        };

        for (auto& [uid, object] : visitor.getAllObjects())
        {
            const Json::ArrayIndex arrayIndex = static_cast<Json::ArrayIndex>(objects.size());
            serialization::runtimeApplyToJsonValue(objects[arrayIndex], makeValueRef(object)).ignore();

            if (object.parentLocalId == 0)
            {
                header.topLevelObjectIds.push_back(object.localId);
            }

            if (++batchCounter == ObjectsBatchSize)
            {
                offset = writeObjectsToStream();
                batchCounter = 0;
            }
        }

        if (objects.size() > 0)
        {
            [[maybe_unused]] const size_t offset = writeObjectsToStream();
        }

        io::writeContainerHeader(stream, "nau-scene", makeValueRef(header));
        contentStream->setPosition(OffsetOrigin::Begin, 0);
        io::copyStream(*stream, contentStream->as<IStreamReader&>()).ignore();

        return ResultSuccess;
    }

}  // namespace nau
